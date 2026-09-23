#include "Stagger.h"
#include "Settings.h"

namespace SimpleStagger
{
    namespace
    {
        [[nodiscard]] bool IsPlayer(const RE::Actor* a_actor)
        {
            return a_actor &&
                (a_actor == RE::PlayerCharacter::GetSingleton() ||
                 a_actor->GetFormID() == 0x14);
        }

        [[nodiscard]] bool HasUnsafeLifeState(const RE::Actor* a_actor)
        {
            if (!a_actor) {
                return true;
            }

            // ActorState moved from 0xB8 to 0xC0 on AE 1.6.629+. Calling the
            // inherited ActorState method directly applies the compile-time
            // base offset; AsActorState() performs the version-aware cast.
            const auto* actorState = a_actor->AsActorState();
            if (!actorState) {
                return true;
            }

            switch (actorState->GetLifeState()) {
            case RE::ACTOR_LIFE_STATE::kDying:
            case RE::ACTOR_LIFE_STATE::kDead:
            case RE::ACTOR_LIFE_STATE::kUnconcious:
            case RE::ACTOR_LIFE_STATE::kRestrained:
            case RE::ACTOR_LIFE_STATE::kEssentialDown:
            case RE::ACTOR_LIFE_STATE::kBleedout:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] bool HasUnsafeSleepState(const RE::Actor* a_actor)
        {
            const auto* actorState = a_actor ? a_actor->AsActorState() : nullptr;
            if (!actorState) {
                return true;
            }

            switch (actorState->GetSitSleepState()) {
            case RE::SIT_SLEEP_STATE::kWantToSleep:
            case RE::SIT_SLEEP_STATE::kWaitingForSleepAnim:
            case RE::SIT_SLEEP_STATE::kIsSleeping:
            case RE::SIT_SLEEP_STATE::kWantToWake:
                return true;
            default:
                return false;
            }
        }
    }

    bool CanQueueSimpleStagger(const RE::Actor* a_target, const RE::Actor* a_aggressor)
    {
        if (!a_target ||
            HasUnsafeLifeState(a_target) ||
            HasUnsafeSleepState(a_target) ||
            a_target->IsInKillMove()) {
            return false;
        }

        if (a_aggressor && a_aggressor->IsInKillMove()) {
            return false;
        }

        const auto* settings = Settings::GetSingleton();
        if (!settings->playerCanBeStaggered.load() && IsPlayer(a_target)) {
            return false;
        }
        // Followers and summons are NPC attackers too.
        if (!settings->npcsCanStagger.load() && !IsPlayer(a_aggressor)) {
            return false;
        }
        return true;
    }

    bool CanApplySimpleStagger(const RE::Actor* a_target, const RE::Actor* a_aggressor)
    {
        if (!CanQueueSimpleStagger(a_target, a_aggressor)) {
            return false;
        }

        // Scripted scenes disable one or more of these control groups while
        // they own the player animation graph. Sending staggerStart during
        // that window can leave the scene and player-control state waiting on
        // different animations (notably Alduin's Helgen arrival).
        if (IsPlayer(a_target)) {
            const auto* controlMap = RE::ControlMap::GetSingleton();
            if (!controlMap ||
                !controlMap->IsMovementControlsEnabled() ||
                !controlMap->IsLookingControlsEnabled() ||
                !controlMap->IsFightingControlsEnabled()) {
                return false;
            }
        }

        // Ambush draugr use a linked interaction marker and furniture-specific
        // enter/exit idles (including forceFurnExit). Sending staggerStart while
        // that graph owns the actor can interrupt the wake sequence and leave
        // the actor stuck. This is intentionally actor-state based rather than
        // a Draugr race/FormID exclusion, so other furniture interactions are
        // protected as well.
        if (a_target->GetOccupiedFurniture()) {
            return false;
        }

        // IsInRagdollState() calls an engine function. Keep it on the game
        // thread; TESHitEvent dispatch is not guaranteed to be main-thread.
        return !a_target->IsInRagdollState();
    }

    void ApplyStagger(RE::Actor* a_target, RE::Actor* a_aggressor, float a_magnitude)
    {
        if (!CanApplySimpleStagger(a_target, a_aggressor) ||
            !std::isfinite(a_magnitude) || a_magnitude <= 0.0f) {
            return;
        }

        // Calculate stagger direction relative to the aggressor.
        // Direction is normalised to [0, 1]:
        //   0.0 / 1.0 = front stagger
        //   0.5       = back stagger
        //
        // GetHeadingAngle(pos, false) returns degrees in [-180, 180).
        // Normalise: dir = (heading >= 0) ? heading/360 : (360+heading)/360
        //
        // Source confirmation:
        //   UselessFenixUtils (fenix31415/UselessFenixUtils) include/UselessFenixUtils.h
        //     template void stagger(RE::Actor* victim, RE::Actor* attacker)
        //   valhallaCombat (D7ry/valhallaCombat) src/bin/reactionHandler.cpp
        //     void reactionHandler::triggerStagger(...)
        float staggerDir = 0.0f;
        if (a_aggressor && a_aggressor->GetHandle() != a_target->GetHandle()) {
            const float heading = a_target->GetHeadingAngle(a_aggressor->GetPosition(), false);
            staggerDir = (heading >= 0.0f) ? heading / 360.0f : (360.0f + heading) / 360.0f;
        }

        a_target->SetGraphVariableFloat("staggerDirection", staggerDir);
        a_target->SetGraphVariableFloat("staggerMagnitude", a_magnitude);
        a_target->NotifyAnimationGraph("staggerStart");

        logger::debug("[SimpleStagger] Stagger applied: target={:#010x} dir={:.3f} mag={:.3f}",
            a_target->GetFormID(), staggerDir, a_magnitude);
    }

}  // namespace SimpleStagger
