#pragma once

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

        /// Clear the per-target cooldown map. Call from kPostLoadGame.
        void ClearCooldowns();

    private:
        // Cooldown map: target FormID -> last stagger time.
        // Accessed only on the main thread (inside AddTask), so no lock needed.
        std::unordered_map<RE::FormID, std::chrono::steady_clock::time_point> m_cooldowns;
    };

}  // namespace SimpleStagger
