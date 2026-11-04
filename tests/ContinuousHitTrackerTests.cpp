#include "ContinuousHitTracker.h"
#include "MagicHitPolicy.h"

#include <cstdlib>
#include <iostream>

namespace
{
    using SimpleStagger::BlockedHitAction;
    using SimpleStagger::ClassifySpecialMagic;
    using SimpleStagger::ContinuousHitKey;
    using SimpleStagger::GetBlockedHitAction;
    using SimpleStagger::GuardHitEvidence;
    using SimpleStagger::IsEligibleMagicStagger;
    using SimpleStagger::IsMagicCastEnabled;
    using SimpleStagger::IsMagicCategoryEnabled;
    using SimpleStagger::IsUnconditionalNativeStagger;
    using SimpleStagger::MagicCastKind;
    using SimpleStagger::MagicHitCompletion;
    using SimpleStagger::MagicHitState;
    using SimpleStagger::MagicHitTraits;
    using SimpleStagger::MagicSourceKind;
    using SimpleStagger::SpecialMagicCategory;
    using SimpleStagger::SpecialMagicProfile;
    using SimpleStagger::WasSuccessfullyGuarded;
    using SimpleStagger::WeaponHitCoalescer;
    using namespace std::chrono_literals;

    void Require(bool a_condition, const char* a_message)
    {
        if (!a_condition) {
            std::cerr << "FAILED: " << a_message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
}

int main()
{
    constexpr MagicHitTraits handProjectile{
        MagicSourceKind::kHandSpell,
        MagicCastKind::kFireAndForget,
        true,
        true,
        false
    };
    static_assert(IsEligibleMagicStagger(handProjectile));
    static_assert(IsEligibleMagicStagger({
        MagicSourceKind::kStaff,
        MagicCastKind::kConcentration,
        true,
        true,
        false
    }));
    static_assert(IsEligibleMagicStagger({
        MagicSourceKind::kScroll,
        MagicCastKind::kFireAndForget,
        true,
        true,
        false
    }));
    static_assert(!IsEligibleMagicStagger({
        MagicSourceKind::kHandSpell,
        MagicCastKind::kConcentration,
        true,
        true,
        true
    }));
    static_assert(!IsEligibleMagicStagger({
        MagicSourceKind::kOther,
        MagicCastKind::kFireAndForget,
        true,
        true,
        false
    }));
    static_assert(!IsEligibleMagicStagger({
        MagicSourceKind::kHandSpell,
        MagicCastKind::kOther,
        true,
        true,
        false
    }));
    static_assert(IsMagicCastEnabled(MagicCastKind::kFireAndForget, false));
    static_assert(IsMagicCastEnabled(MagicCastKind::kConcentration, true));
    static_assert(!IsMagicCastEnabled(MagicCastKind::kConcentration, false));
    static_assert(IsUnconditionalNativeStagger(true, false, false));
    static_assert(!IsUnconditionalNativeStagger(true, true, false));
    static_assert(!IsUnconditionalNativeStagger(true, false, true));

    static_assert(
        GetBlockedHitAction(false, true, 0) == BlockedHitAction::kKeep);
    static_assert(
        GetBlockedHitAction(true, false, 0) == BlockedHitAction::kKeep);
    static_assert(
        GetBlockedHitAction(true, true, 0) == BlockedHitAction::kReject);
    static_assert(
        GetBlockedHitAction(true, true, 1) == BlockedHitAction::kReduce);
    static_assert(
        GetBlockedHitAction(true, true, 99) == BlockedHitAction::kReject);
    static_assert(WasSuccessfullyGuarded(
        GuardHitEvidence{ .eventFlag = true }));
    static_assert(WasSuccessfullyGuarded(GuardHitEvidence{
        .hitDataMatches = true,
        .hitDataBlocked = true
    }));
    static_assert(WasSuccessfullyGuarded(GuardHitEvidence{
        .hitDataMatches = true,
        .hasBlockedDamage = true
    }));
    static_assert(WasSuccessfullyGuarded(GuardHitEvidence{
        .hitDataMatches = true,
        .blockCandidate = true,
        .defenderBlocking = true
    }));
    static_assert(!WasSuccessfullyGuarded(GuardHitEvidence{
        .hitDataMatches = true,
        .blockCandidate = true
    }));
    static_assert(!WasSuccessfullyGuarded(GuardHitEvidence{
        .hitDataMatches = false,
        .hitDataBlocked = true,
        .hasBlockedDamage = true,
        .blockCandidate = true,
        .defenderBlocking = true
    }));
    static_assert(WasSuccessfullyGuarded(
        GuardHitEvidence{ .wardAbsorbed = true }));
    static_assert(!WasSuccessfullyGuarded(GuardHitEvidence{}));

    constexpr SpecialMagicProfile continuousBreath{
        MagicCastKind::kConcentration, true, true, false, false, false
    };
    static_assert(
        ClassifySpecialMagic(continuousBreath, true, false) ==
        SpecialMagicCategory::kDragonBreath);
    static_assert(
        ClassifySpecialMagic(continuousBreath, false, false) ==
        SpecialMagicCategory::kOtherCreatureAttack);
    static_assert(
        ClassifySpecialMagic({
            MagicCastKind::kFireAndForget, true, true, true, false, false
        }, true, false) == SpecialMagicCategory::kDragonBreath);
    static_assert(
        ClassifySpecialMagic({
            MagicCastKind::kFireAndForget, true, true, true, false, false
        }, false, true) == SpecialMagicCategory::kOtherCreatureAttack);
    static_assert(
        ClassifySpecialMagic({
            MagicCastKind::kFireAndForget, true, true, false, true, false
        }, false, true) == SpecialMagicCategory::kNone);
    static_assert(
        ClassifySpecialMagic({
            MagicCastKind::kConcentration, true, true, true, false, true
        }, true, false) == SpecialMagicCategory::kNone);

    static_assert(IsMagicCategoryEnabled(
        SpecialMagicCategory::kNone, true, false, false));
    static_assert(!IsMagicCategoryEnabled(
        SpecialMagicCategory::kNone, false, true, true));
    static_assert(IsMagicCategoryEnabled(
        SpecialMagicCategory::kDragonBreath, false, true, false));
    static_assert(!IsMagicCategoryEnabled(
        SpecialMagicCategory::kDragonBreath, true, false, true));
    static_assert(IsMagicCategoryEnabled(
        SpecialMagicCategory::kOtherCreatureAttack, false, false, true));
    static_assert(!IsMagicCategoryEnabled(
        SpecialMagicCategory::kOtherCreatureAttack, true, true, false));

    MagicHitState tracker;
    const ContinuousHitKey flameHit{ 0x14, 0x1234, 0x6445B };
    const auto start = MagicHitState::Clock::time_point{};

    auto reservation = tracker.TryReserve(flameHit, start, 200ms);
    Require(reservation.has_value(), "the first concentration hit must pass");
    Require(!tracker.TryReserve(flameHit, start + 1ms, 200ms),
        "a target must not queue two pending attempts");
    tracker.Complete(
        *reservation, start + 10ms, 200ms, MagicHitCompletion::kRetry);
    reservation = tracker.TryReserve(flameHit, start + 11ms, 200ms);
    Require(reservation.has_value(),
        "a transient failure must be immediately retryable");
    tracker.Complete(
        *reservation, start + 20ms, 200ms, MagicHitCompletion::kApplied);
    Require(!tracker.TryReserve(flameHit, start + 219ms, 200ms),
        "an uninterrupted beam must respect the 200 ms attempt cooldown");
    reservation = tracker.TryReserve(flameHit, start + 220ms, 200ms);
    Require(reservation.has_value(),
        "an uninterrupted beam must reattempt when 200 ms elapses");
    tracker.Complete(
        *reservation, start + 221ms, 200ms, MagicHitCompletion::kRejected);
    Require(!tracker.TryReserve(flameHit, start + 420ms, 200ms),
        "a final resistance or perk rejection must start cooldown");
    Require(tracker.TryReserve(flameHit, start + 421ms, 200ms).has_value(),
        "a rejected beam attempt must rearm after cooldown");

    tracker.Clear();
    reservation = tracker.TryReserve(flameHit, start, 60ms);
    Require(reservation.has_value(), "the first 60 ms beam attempt must pass");
    tracker.Complete(
        *reservation, start, 60ms, MagicHitCompletion::kApplied);
    Require(!tracker.TryReserve(flameHit, start + 59ms, 60ms),
        "the 60 ms cooldown must suppress early beam events");
    Require(tracker.TryReserve(flameHit, start + 60ms, 60ms).has_value(),
        "the beam must reattempt exactly at the 60 ms boundary");

    tracker.Clear();
    const ContinuousHitKey firstTarget{ 0x20, 0x30, 0x40 };
    const ContinuousHitKey secondTarget{ 0x21, 0x30, 0x40 };
    reservation = tracker.TryReserve(firstTarget, start, 200ms);
    Require(reservation.has_value(), "the first target must reserve");
    tracker.Complete(
        *reservation, start, 200ms, MagicHitCompletion::kApplied);
    Require(tracker.TryReserve(secondTarget, start + 1ms, 200ms).has_value(),
        "a different target must have an independent magic cooldown");

    tracker.Clear();
    reservation = tracker.TryReserve(flameHit, start, 0ms);
    Require(reservation.has_value(), "zero cooldown must allow an attempt");
    tracker.Complete(
        *reservation, start, 0ms, MagicHitCompletion::kRejected);
    Require(tracker.TryReserve(flameHit, start, 0ms).has_value(),
        "zero cooldown must allow the next event immediately");
    Require(tracker.Sizes().first == 0,
        "zero cooldown must not allocate cooldown state");

    tracker.Clear();
    for (std::uint32_t i = 1; i <= 1100; ++i) {
        const ContinuousHitKey key{ i, i + 1, i + 2 };
        const auto item = tracker.TryReserve(key, start, 1ms);
        Require(item.has_value(), "a unique target must reserve");
        tracker.Complete(
            *item, start, 1ms, MagicHitCompletion::kApplied);
    }
    for (std::uint32_t i = 1; i <= 512; ++i) {
        const ContinuousHitKey key{ 0x10000u + i, i, i };
        const auto item = tracker.TryReserve(key, start + 31s, 1ms);
        Require(item.has_value(), "a new target must reserve after pruning");
        tracker.Complete(
            *item, start + 31s, 1ms, MagicHitCompletion::kApplied);
    }
    Require(tracker.Sizes().first < MagicHitState::kPruneThreshold,
        "expired target cooldowns must be lazily pruned");

    tracker.Clear();
    Require(tracker.TryReserve(flameHit, start, 0ms).has_value(),
        "the first pending reservation must pass");
    for (std::uint32_t i = 0; i < 1'000'000; ++i) {
        (void) tracker.TryReserve(
            flameHit, start + std::chrono::milliseconds(i), 0ms);
    }
    const auto [cooldownSize, pendingSize] = tracker.Sizes();
    Require(cooldownSize == 0 && pendingSize == 1,
        "repeated pending observations must retain bounded state");

    WeaponHitCoalescer weaponHits;
    const ContinuousHitKey fireSword{ 0x14, 0x1234, 0x28E87 };

    // Enchanted swing: unguarded enchantment event, then guarded physical hit.
    auto ticket = weaponHits.Observe(fireSword, false, 0.25f);
    Require(ticket.has_value(), "the first event of a swing must open a hit");
    Require(!weaponHits.Observe(fireSword, true, 0.25f),
        "a second event of the same swing must merge");
    auto hit = weaponHits.Take(*ticket);
    Require(hit && hit->guarded,
        "a guarded event must guard the whole merged swing");
    Require(weaponHits.Size() == 0, "taking a hit must close it");

    // Unguarded enchanted power attack keeps the strongest magnitude.
    ticket = weaponHits.Observe(fireSword, false, 0.25f);
    Require(!weaponHits.Observe(fireSword, false, 0.40f),
        "the physical event must merge into the enchantment event");
    hit = weaponHits.Take(*ticket);
    Require(hit && !hit->guarded && hit->magnitude == 0.40f,
        "an unguarded merged swing must keep the strongest magnitude");

    // The next swing after the task ran is a separate hit.
    ticket = weaponHits.Observe(fireSword, false, 0.25f);
    Require(ticket.has_value(), "a swing after Take must open a new hit");
    Require(weaponHits.Take(*ticket).has_value(), "the new hit must be taken");

    // Other targets and weapons are independent.
    const auto firstTicket = weaponHits.Observe(fireSword, false, 0.25f);
    const auto otherTicket =
        weaponHits.Observe({ 0x15, 0x1234, 0x28E87 }, true, 0.25f);
    Require(firstTicket && otherTicket, "different targets must not merge");
    Require(!weaponHits.Take(*firstTicket)->guarded,
        "another target's guard must not leak");
    Require(weaponHits.Take(*otherTicket)->guarded,
        "each target must keep its own guard");

    // Clear discards pending hits, and a stale ticket takes nothing.
    ticket = weaponHits.Observe(fireSword, true, 0.25f);
    weaponHits.Clear();
    Require(!weaponHits.Take(*ticket), "a cleared hit must not be taken");
    const auto freshTicket = weaponHits.Observe(fireSword, false, 0.25f);
    Require(!weaponHits.Take(*ticket),
        "a stale ticket must not take a newer hit");
    Require(weaponHits.Take(*freshTicket).has_value(),
        "the newer hit must still be taken by its own ticket");

    // Cost of one enchanted swing: two Observe calls and one Take.
    constexpr int kSwings = 1'000'000;
    const auto benchStart = std::chrono::steady_clock::now();
    for (int i = 0; i < kSwings; ++i) {
        const ContinuousHitKey key{ 0x14, 0x1234, static_cast<std::uint32_t>(i) };
        const auto swing = weaponHits.Observe(key, false, 0.25f);
        (void) weaponHits.Observe(key, true, 0.25f);
        (void) weaponHits.Take(*swing);
    }
    const auto benchNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - benchStart).count();
    Require(weaponHits.Size() == 0, "the benchmark must leave no pending hits");
    std::cout << "WeaponHitCoalescer: " << (benchNs / kSwings)
              << " ns per enchanted swing (2 Observe + 1 Take)\n";

    return EXIT_SUCCESS;
}
