#include "Resistance.h"
#include "Settings.h"

namespace SimpleStagger
{
    namespace
    {
        /// Returns true if a_actor matches the override entry (race FormID or keyword).
        bool MatchesOverride(RE::Actor* a_actor, const OverrideEntry& a_entry)
        {
            if (!a_actor) {
                return false;
            }

            // FormID match: compare the actor's race FormID
            if (a_entry.isFormID()) {
                const auto* race = a_actor->GetRace();
                if (race && race->GetFormID() == a_entry.resolvedFormID) {
                    return true;
                }
            }

            // Keyword match: HasKeywordString
            if (a_entry.isKeyword()) {
                if (a_actor->HasKeywordString(a_entry.keywordEditorID)) {
                    return true;
                }
            }

            return false;
        }

        bool MatchesAnyOverride(RE::Actor* a_actor, const std::vector<OverrideEntry>& a_entries)
        {
            for (const auto& entry : a_entries) {
                if (MatchesOverride(a_actor, entry)) {
                    return true;
                }
            }
            return false;
        }
    }

    ResistResult CheckResistance(RE::Actor* a_target, float& a_magnitude)
    {
        if (!a_target) {
            return ResistResult::kBlock;
        }

        const auto* cfg = Settings::GetSingleton();

        // 1. Susceptible override -> always allow (bypass mass check)
        if (MatchesAnyOverride(a_target, cfg->susceptibleEntries)) {
            return ResistResult::kAllow;
        }

        // 2. Immune override -> always block
        if (MatchesAnyOverride(a_target, cfg->immuneEntries)) {
            return ResistResult::kBlock;
        }

        // 3. Mass threshold check
        // RE::ActorValue::kMass = 36 (confirmed from alandtse/CommonLibVR ng ActorValues.h)
        const auto* avOwner = a_target->AsActorValueOwner();
        if (avOwner) {
            const float mass  = avOwner->GetActorValue(RE::ActorValue::kMass);
            const float scale = a_target->GetScale();
            if (mass * scale >= cfg->massThreshold) {
                if (cfg->largeActorMode == 0) {
                    return ResistResult::kBlock;
                } else {
                    // Mode 1: reduce magnitude
                    a_magnitude *= cfg->largeActorMultiplier;
                    return ResistResult::kReduce;
                }
            }
        }

        return ResistResult::kAllow;
    }

}  // namespace SimpleStagger
