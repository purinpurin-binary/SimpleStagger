#pragma once

#include "ContinuousHitTracker.h"
#include "MagicHitPolicy.h"

namespace SimpleStagger
{
    class HitHandler final :
        public REX::Singleton<HitHandler>,
        public RE::BSTEventSink<RE::TESHitEvent>
    {
    public:
        /// Register against ScriptEventSourceHolder. Call from kDataLoaded.
        static void Register();

        RE::BSEventNotifyControl ProcessEvent(
            const RE::TESHitEvent*                 a_event,
            RE::BSTEventSource<RE::TESHitEvent>*   a_eventSource) override;

        /// Clear the per-target cooldown maps. Call from kPostLoadGame.
        void ClearCooldowns();

    private:
        /// Build immutable load-order-aware Cloak exclusions and special Voice
        /// attack profiles. Called once at kDataLoaded before sink registration.
        void InitializeMagicData();

        [[nodiscard]] bool IsCloakAssociatedMagic(
            RE::FormID a_source,
            RE::FormID a_magicItem) const;

        [[nodiscard]] std::optional<SpecialMagicProfile>
            GetSpecialMagicProfile(const RE::TESForm* a_sourceForm) const;

        // Weapon cooldown map: target FormID -> last stagger time.
        // Accessed only on the main thread (inside AddTask), so no lock needed.
        std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> m_cooldowns;

        // One event-thread-safe state manager owns the per-target magic attempt
        // cooldown and pending task reservations.
        MagicHitState m_magicHitState;

        // FormIDs referenced by load-order-winning Cloak MGEFs. The set is
        // populated before sink registration and then remains immutable.
        std::unordered_set<RE::FormID> m_cloakAssociatedMagic;

        // Compact structural profiles for Voice spells. Runtime-generated Voice
        // sources use the same lightweight classifier as a cache-miss fallback.
        std::unordered_map<RE::FormID, SpecialMagicProfile> m_specialMagicProfiles;

        const RE::BGSKeyword* m_actorTypeDragon{ nullptr };
        const RE::BGSKeyword* m_actorTypeDwarven{ nullptr };
    };

}  // namespace SimpleStagger
