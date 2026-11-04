#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace SimpleStagger
{
    struct ContinuousHitKey
    {
        std::uint32_t target{ 0 };
        std::uint32_t cause{ 0 };
        std::uint32_t source{ 0 };

        [[nodiscard]] bool operator==(const ContinuousHitKey&) const noexcept = default;
    };

    struct MagicHitReservation
    {
        ContinuousHitKey key{};
        std::uint64_t token{ 0 };
    };

    enum class MagicHitCompletion : std::uint8_t
    {
        /// A transient main-thread gate rejected the queued hit. A later event
        /// may reserve another attempt without waiting for MagicCooldownMs.
        kRetry,
        /// Resistance or a perk rejected the completed gameplay attempt.
        kRejected,
        /// Stagger was applied.
        kApplied
    };

    class MagicHitState
    {
    public:
        using Clock = std::chrono::steady_clock;
        static constexpr std::size_t kPruneThreshold = 1024;
        static constexpr std::size_t kPruneInterval = 256;
        static constexpr auto kMinimumExpiry = std::chrono::seconds(30);

        /// Reserve one main-thread attempt for a target. Concentration and
        /// projectile hits intentionally share the same per-target cooldown.
        [[nodiscard]] std::optional<MagicHitReservation> TryReserve(
            const ContinuousHitKey& a_key,
            Clock::time_point a_now,
            std::chrono::milliseconds a_cooldown)
        {
            std::scoped_lock lock(m_mutex);
            PruneIfNeeded(a_now, a_cooldown);

            if (m_pendingTargets.contains(a_key.target)) {
                return std::nullopt;
            }

            if (a_cooldown > std::chrono::milliseconds::zero()) {
                if (const auto it = m_targetCooldowns.find(a_key.target);
                    it != m_targetCooldowns.end() &&
                    a_now - it->second < a_cooldown) {
                    return std::nullopt;
                }
            }

            const auto token = NextToken();
            m_pendingTargets.emplace(
                a_key.target, PendingReservation{ a_key, token });
            return MagicHitReservation{ a_key, token };
        }

        /// Every completed gameplay attempt starts MagicCooldownMs, regardless
        /// of whether stagger was applied or finally rejected. Transient state
        /// failures release only the pending reservation.
        void Complete(
            const MagicHitReservation& a_reservation,
            Clock::time_point a_now,
            std::chrono::milliseconds a_cooldown,
            MagicHitCompletion a_completion)
        {
            std::scoped_lock lock(m_mutex);
            const auto pending = m_pendingTargets.find(a_reservation.key.target);
            if (pending == m_pendingTargets.end() ||
                pending->second.token != a_reservation.token ||
                !(pending->second.key == a_reservation.key)) {
                return;
            }
            m_pendingTargets.erase(pending);

            if (a_completion != MagicHitCompletion::kRetry &&
                a_cooldown > std::chrono::milliseconds::zero()) {
                m_targetCooldowns[a_reservation.key.target] = a_now;
            }
        }

        void Clear()
        {
            std::scoped_lock lock(m_mutex);
            m_targetCooldowns.clear();
            m_pendingTargets.clear();
            m_observations = 0;
        }

        [[nodiscard]] std::pair<std::size_t, std::size_t> Sizes() const
        {
            std::scoped_lock lock(m_mutex);
            return { m_targetCooldowns.size(), m_pendingTargets.size() };
        }

    private:
        struct PendingReservation
        {
            ContinuousHitKey key{};
            std::uint64_t token{ 0 };
        };

        [[nodiscard]] std::uint64_t NextToken() noexcept
        {
            ++m_nextToken;
            if (m_nextToken == 0) {
                ++m_nextToken;
            }
            return m_nextToken;
        }

        void PruneIfNeeded(
            Clock::time_point a_now,
            std::chrono::milliseconds a_cooldown)
        {
            ++m_observations;
            if (m_targetCooldowns.size() < kPruneThreshold ||
                m_observations % kPruneInterval != 0) {
                return;
            }

            const auto expiry = std::max(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    kMinimumExpiry),
                a_cooldown * 4);
            std::erase_if(m_targetCooldowns, [&](const auto& a_entry) {
                return !m_pendingTargets.contains(a_entry.first) &&
                    a_now - a_entry.second > expiry;
            });
        }

        mutable std::mutex m_mutex;
        std::unordered_map<std::uint32_t, Clock::time_point> m_targetCooldowns;
        std::unordered_map<std::uint32_t, PendingReservation> m_pendingTargets;
        std::uint64_t m_observations{ 0 };
        std::uint64_t m_nextToken{ 0 };
    };

    struct WeaponHitTicket
    {
        ContinuousHitKey key{};
        std::uint64_t token{ 0 };
    };

    struct CoalescedWeaponHit
    {
        bool guarded{ false };
        float magnitude{ 0.0f };
    };

    /// One weapon swing can raise several TESHitEvents for the same target,
    /// attacker and weapon in one frame: an enchanted weapon reports its
    /// enchantment first, with the weapon as source and no block data, then
    /// the physical hit. Events observed before the queued main-thread task
    /// runs are folded into one hit, and any guarded event guards the hit.
    class WeaponHitCoalescer
    {
    public:
        /// Returns a ticket when this event opens a new hit; the caller queues
        /// one task for it. Returns nullopt when the event was merged into a
        /// hit that is still pending.
        [[nodiscard]] std::optional<WeaponHitTicket> Observe(
            const ContinuousHitKey& a_key,
            bool a_guarded,
            float a_magnitude)
        {
            std::scoped_lock lock(m_mutex);
            if (const auto it = FindPending(a_key); it != m_pending.end()) {
                it->hit.guarded = it->hit.guarded || a_guarded;
                it->hit.magnitude = std::max(it->hit.magnitude, a_magnitude);
                return std::nullopt;
            }

            ++m_nextToken;
            if (m_nextToken == 0) {
                ++m_nextToken;
            }
            m_pending.push_back(
                PendingHit{ a_key, m_nextToken, { a_guarded, a_magnitude } });
            return WeaponHitTicket{ a_key, m_nextToken };
        }

        /// Close the hit and return everything merged into it. Returns nullopt
        /// when the hit was discarded by Clear().
        [[nodiscard]] std::optional<CoalescedWeaponHit> Take(
            const WeaponHitTicket& a_ticket)
        {
            std::scoped_lock lock(m_mutex);
            const auto it = FindPending(a_ticket.key);
            if (it == m_pending.end() || it->token != a_ticket.token) {
                return std::nullopt;
            }
            const auto hit = it->hit;
            *it = m_pending.back();
            m_pending.pop_back();
            return hit;
        }

        void Clear()
        {
            std::scoped_lock lock(m_mutex);
            m_pending.clear();
        }

        [[nodiscard]] std::size_t Size() const
        {
            std::scoped_lock lock(m_mutex);
            return m_pending.size();
        }

    private:
        struct PendingHit
        {
            ContinuousHitKey key{};
            std::uint64_t token{ 0 };
            CoalescedWeaponHit hit{};
        };

        // Only hits queued in the current frame are pending, so a linear scan
        // over a few entries beats hashing.
        [[nodiscard]] std::vector<PendingHit>::iterator FindPending(
            const ContinuousHitKey& a_key)
        {
            return std::ranges::find(m_pending, a_key, &PendingHit::key);
        }

        mutable std::mutex m_mutex;
        std::vector<PendingHit> m_pending;
        std::uint64_t m_nextToken{ 0 };
    };
}  // namespace SimpleStagger
