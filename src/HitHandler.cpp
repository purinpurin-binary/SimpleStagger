#include "HitHandler.h"
#include "Settings.h"
#include "Stagger.h"
#include "Resistance.h"
#include <chrono>

namespace SimpleStagger
{
    using EventResult = RE::BSEventNotifyControl;

    void HitHandler::Register()
    {
        auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
        if (!holder) {
            logger::error("[SimpleStagger] ScriptEventSourceHolder is null – TESHitEvent sink NOT registered");
            return;
        }
        holder->AddEventSink<RE::TESHitEvent>(GetSingleton());
        logger::info("[SimpleStagger] TESHitEvent sink registered");
    }

    void HitHandler::ClearCooldowns()
    {
        m_cooldowns.clear();
        logger::debug("[SimpleStagger] Cooldown map cleared");
    }

    EventResult HitHandler::ProcessEvent(
        const RE::TESHitEvent*               a_event,
        RE::BSTEventSource<RE::TESHitEvent>*)
    {
        // 1. Mandatory null-guard (event pointer is forwarded verbatim by the engine)
        if (!a_event) {
            return EventResult::kContinue;
        }

        // 2. Resolve target and cause as actors
        auto* target = a_event->target ? a_event->target->As<RE::Actor>() : nullptr;
        auto* cause  = a_event->cause  ? a_event->cause->As<RE::Actor>()  : nullptr;
        if (!target || !cause) {
            return EventResult::kContinue;
        }

        // 3. Target must be alive
        if (target->IsDead()) {
            return EventResult::kContinue;
        }

        const auto* cfg = Settings::GetSingleton();

        // 4. Player stagger toggle
        if (!cfg->playerCanBeStaggered && target->IsPlayerRef()) {
            return EventResult::kContinue;
        }

        // 5. Source must be a weapon (filters out magic hits naturally)
        auto* sourceForm = RE::TESForm::LookupByID(a_event->source);
        if (!sourceForm || !sourceForm->Is(RE::FormType::Weapon)) {
            return EventResult::kContinue;
        }

        // 6. Ranged vs melee toggle
        const bool isRanged = (a_event->projectile != 0);
        if (isRanged && !cfg->enableRanged) {
            return EventResult::kContinue;
        }
        if (!isRanged && !cfg->enableMelee) {
            return EventResult::kContinue;
        }

        // 7. Determine magnitude from hit flags
        using HitFlag = RE::TESHitEvent::Flag;
        const bool hitBlocked   = a_event->flags.any(HitFlag::kHitBlocked);
        const bool isPowerAtk   = a_event->flags.any(HitFlag::kPowerAttack);
        const bool isBash       = a_event->flags.any(HitFlag::kBashAttack);

        // Blocked hit handling
        float magnitude = cfg->staggerMagnitude;
        if (hitBlocked) {
            if (cfg->blockedHitBehavior == 0) {
                return EventResult::kContinue;  // no stagger on blocked
            }
            // behavior == 1: apply with reduced magnitude (multiplier applied below)
        }

        // Select base magnitude
        if (isPowerAtk) {
            magnitude = cfg->powerAttackMagnitude;
        } else if (isBash) {
            magnitude = cfg->bashMagnitude;
        }

        // Apply blocked multiplier after base selection
        if (hitBlocked && cfg->blockedHitBehavior == 1) {
            magnitude *= cfg->blockedMultiplier;
        }

        // 8. Marshal stagger to main thread.
        //    Capture handles (NOT raw pointers) to stay safe across async dispatch.
        const RE::ActorHandle targetHandle = target->GetHandle();
        const RE::ActorHandle causeHandle  = cause->GetHandle();

        SKSE::GetTaskInterface()->AddTask([targetHandle, causeHandle, magnitude, cfg]() mutable {
            auto targetPtr = targetHandle.get();
            auto causePtr  = causeHandle.get();
            if (!targetPtr || !causePtr) {
                return;
            }

            // Cooldown check (main thread only – no lock needed)
            if (cfg->staggerCooldownMs > 0) {
                const RE::FormID targetID = targetPtr->GetFormID();
                const auto now = std::chrono::steady_clock::now();
                auto& handler = *HitHandler::GetSingleton();
                auto  it      = handler.m_cooldowns.find(targetID);
                if (it != handler.m_cooldowns.end()) {
                    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - it->second).count();
                    if (elapsed < static_cast<long long>(cfg->staggerCooldownMs)) {
                        return;  // still on cooldown
                    }
                }
                handler.m_cooldowns[targetID] = now;
            }

            // Resistance check (may modify magnitude in-place)
            const ResistResult resist = CheckResistance(targetPtr.get(), magnitude);
            if (resist == ResistResult::kBlock) {
                return;
            }

            ApplyStagger(targetPtr.get(), causePtr.get(), magnitude);
        });

        return EventResult::kContinue;
    }

}  // namespace SimpleStagger
