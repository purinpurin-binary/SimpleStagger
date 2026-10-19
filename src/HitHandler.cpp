#include "HitHandler.h"
#include "MagicHitPolicy.h"
#include "Resistance.h"
#include "Settings.h"
#include "Stagger.h"

namespace SimpleStagger
{
    using EventResult = RE::BSEventNotifyControl;

    namespace
    {
        struct ResolvedMagicSource
        {
            const RE::MagicItem* magicItem{ nullptr };
            MagicSourceKind sourceKind{ MagicSourceKind::kOther };
        };

        [[nodiscard]] bool IsPositiveFinite(float a_value)
        {
            return std::isfinite(a_value) && a_value > 0.0f;
        }

        [[nodiscard]] bool HitDataMatchesEvent(
            const RE::HitData* a_hitData,
            RE::Actor* a_target,
            RE::Actor* a_cause)
        {
            if (!a_hitData || !a_target || !a_cause) {
                return false;
            }

            const auto targetHandle = a_target->GetHandle();
            const auto causeHandle = a_cause->GetHandle();
            return a_hitData->target &&
                   a_hitData->target == targetHandle &&
                   a_hitData->aggressor &&
                   a_hitData->aggressor == causeHandle;
        }

        [[nodiscard]] bool IsSuccessfullyGuardedHit(
            const RE::TESHitEvent& a_event,
            RE::Actor* a_target,
            RE::Actor* a_cause,
            bool a_isMagic)
        {
            using EventFlag = RE::TESHitEvent::Flag;
            using HitFlag = RE::HitData::Flag;

            GuardHitEvidence evidence{};
            evidence.eventFlag =
                a_event.flags.any(EventFlag::kHitBlocked);

            const auto* process =
                a_target ? a_target->GetMiddleHighProcess() : nullptr;
            if (!process) {
                return SimpleStagger::WasSuccessfullyGuarded(evidence);
            }

            if (a_isMagic) {
                evidence.wardAbsorbed =
                    process->wardState ==
                    RE::MagicSystem::WardState::kAbsorb;
            } else {
                const auto* hitData = process->lastHitData;
                evidence.hitDataMatches = HitDataMatchesEvent(
                    hitData, a_target, a_cause);
                if (evidence.hitDataMatches) {
                    evidence.hitDataBlocked =
                        hitData->flags.any(HitFlag::kBlocked);
                    evidence.hasBlockedDamage =
                        std::isfinite(hitData->percentBlocked) &&
                        hitData->percentBlocked > 0.0f;
                    evidence.blockCandidate =
                        hitData->flags.any(HitFlag::kBlockCandidate);
                    evidence.defenderBlocking = a_target->IsBlocking();
                }
            }

            return SimpleStagger::WasSuccessfullyGuarded(evidence);
        }

        [[nodiscard]] bool HasUnconditionalNativeStagger(
            const RE::Effect* a_effect)
        {
            if (!a_effect || !a_effect->baseEffect) {
                return false;
            }
            return IsUnconditionalNativeStagger(
                a_effect->baseEffect->HasArchetype(
                    RE::EffectArchetype::kStagger),
                static_cast<bool>(a_effect->conditions),
                static_cast<bool>(a_effect->baseEffect->conditions));
        }

        [[nodiscard]] MagicCastKind GetMagicCastKind(
            RE::MagicSystem::CastingType a_castingType)
        {
            switch (a_castingType) {
            case RE::MagicSystem::CastingType::kFireAndForget:
                return MagicCastKind::kFireAndForget;
            case RE::MagicSystem::CastingType::kConcentration:
                return MagicCastKind::kConcentration;
            default:
                return MagicCastKind::kOther;
            }
        }

        [[nodiscard]] std::optional<SpecialMagicProfile>
            BuildSpecialMagicProfile(const RE::MagicItem* a_magicItem)
        {
            if (!a_magicItem ||
                a_magicItem->GetSpellType() !=
                    RE::MagicSystem::SpellType::kVoicePower) {
                return std::nullopt;
            }

            SpecialMagicProfile profile{
                GetMagicCastKind(a_magicItem->GetCastingType()),
                a_magicItem->GetDelivery() == RE::MagicSystem::Delivery::kAimed
            };
            if (!profile.aimed || profile.cast == MagicCastKind::kOther) {
                return std::nullopt;
            }

            for (const auto* effect : a_magicItem->effects) {
                if (!effect || !effect->baseEffect) {
                    continue;
                }

                const auto* baseEffect = effect->baseEffect;
                if (HasUnconditionalNativeStagger(effect)) {
                    profile.hasNativeStagger = true;
                }

                const bool damagesHealth =
                    baseEffect->data.primaryAV == RE::ActorValue::kHealth ||
                    baseEffect->data.secondaryAV == RE::ActorValue::kHealth;
                if ((!baseEffect->IsHostile() && !baseEffect->IsDetrimental()) ||
                    !damagesHealth ||
                    baseEffect->data.delivery != RE::MagicSystem::Delivery::kAimed ||
                    GetMagicCastKind(baseEffect->data.castingType) ==
                        MagicCastKind::kOther ||
                    !baseEffect->data.projectileBase) {
                    continue;
                }

                profile.hasHostileHealthProjectile = true;
                profile.hasFireDamage =
                    profile.hasFireDamage ||
                    baseEffect->data.resistVariable ==
                        RE::ActorValue::kResistFire;
                profile.hasFrostDamage =
                    profile.hasFrostDamage ||
                    baseEffect->data.resistVariable ==
                        RE::ActorValue::kResistFrost;
            }
            return profile;
        }

        [[nodiscard]] std::optional<ResolvedMagicSource> ResolveMagicSource(
            RE::TESForm* a_sourceForm)
        {
            if (!a_sourceForm) {
                return std::nullopt;
            }

            if (a_sourceForm->Is(RE::FormType::Spell) ||
                a_sourceForm->Is(RE::FormType::Scroll)) {
                const auto* magicItem = a_sourceForm->As<RE::MagicItem>();
                if (!magicItem) {
                    return std::nullopt;
                }

                switch (magicItem->GetSpellType()) {
                case RE::MagicSystem::SpellType::kSpell:
                    return ResolvedMagicSource{
                        magicItem, MagicSourceKind::kHandSpell
                    };
                case RE::MagicSystem::SpellType::kScroll:
                    return ResolvedMagicSource{
                        magicItem, MagicSourceKind::kScroll
                    };
                default:
                    return std::nullopt;
                }
            }

            const auto* weapon = a_sourceForm->As<RE::TESObjectWEAP>();
            if (!weapon || !weapon->IsStaff() || !weapon->formEnchanting) {
                return std::nullopt;
            }
            if (weapon->formEnchanting->GetSpellType() !=
                RE::MagicSystem::SpellType::kStaffEnchantment) {
                return std::nullopt;
            }

            return ResolvedMagicSource{
                weapon->formEnchanting, MagicSourceKind::kStaff
            };
        }

        [[nodiscard]] std::optional<std::int32_t> GetDestructionMinimumSkill(
            const ResolvedMagicSource& a_source,
            bool a_isCloakAssociated)
        {
            const auto* magicItem = a_source.magicItem;
            if (!magicItem) {
                return std::nullopt;
            }

            bool found = false;
            bool hasNativeStagger = false;
            bool allCostsKnown = true;
            float lowestCost = std::numeric_limits<float>::max();
            float highestCost = std::numeric_limits<float>::lowest();
            std::int32_t highestSkill = 0;
            std::int32_t highestCostSkill = 0;

            for (const auto* effect : magicItem->effects) {
                if (!effect || !effect->baseEffect) {
                    continue;
                }

                const auto* baseEffect = effect->baseEffect;
                if (HasUnconditionalNativeStagger(effect)) {
                    hasNativeStagger = true;
                }
                if (baseEffect->GetMagickSkill() != RE::ActorValue::kDestruction ||
                    (!baseEffect->IsHostile() && !baseEffect->IsDetrimental()) ||
                    baseEffect->data.delivery != RE::MagicSystem::Delivery::kAimed ||
                    GetMagicCastKind(baseEffect->data.castingType) ==
                        MagicCastKind::kOther ||
                    !baseEffect->data.projectileBase) {
                    continue;
                }

                found = true;
                const auto skill = std::max(0, baseEffect->GetMinimumSkillLevel());
                highestSkill = std::max(highestSkill, skill);

                const bool costKnown = std::isfinite(effect->cost) && effect->cost > 0.0f;
                allCostsKnown = allCostsKnown && costKnown;
                if (costKnown) {
                    lowestCost = std::min(lowestCost, effect->cost);
                    if (effect->cost > highestCost) {
                        highestCost = effect->cost;
                        highestCostSkill = skill;
                    } else if (effect->cost == highestCost) {
                        highestCostSkill = std::max(highestCostSkill, skill);
                    }
                }
            }

            const MagicHitTraits traits{
                a_source.sourceKind,
                GetMagicCastKind(magicItem->GetCastingType()),
                magicItem->GetDelivery() == RE::MagicSystem::Delivery::kAimed,
                found,
                a_isCloakAssociated,
                hasNativeStagger
            };
            if (!IsEligibleMagicStagger(traits)) {
                return std::nullopt;
            }

            // Equal or unavailable cost information falls back to the highest
            // MinimumSkill among eligible Destruction effects.
            if (!allCostsKnown || highestCost == lowestCost) {
                return highestSkill;
            }
            return highestCostSkill;
        }

        [[nodiscard]] float GetMagicMagnitude(
            const Settings& a_settings,
            std::int32_t a_minimumSkill)
        {
            if (a_minimumSkill >= 100) {
                return a_settings.masterMagicMagnitude.load();
            }
            if (a_minimumSkill >= 75) {
                return a_settings.expertMagicMagnitude.load();
            }
            if (a_minimumSkill >= 50) {
                return a_settings.adeptMagicMagnitude.load();
            }
            if (a_minimumSkill >= 25) {
                return a_settings.apprenticeMagicMagnitude.load();
            }
            return a_settings.noviceMagicMagnitude.load();
        }
    }

    void HitHandler::Register()
    {
        auto* handler = GetSingleton();
        handler->InitializeMagicData();

        auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
        if (!holder) {
            logger::error("[SimpleStagger] ScriptEventSourceHolder is null - TESHitEvent sink not registered");
            return;
        }
        holder->AddEventSink<RE::TESHitEvent>(handler);
        logger::info("[SimpleStagger] TESHitEvent sink registered");
    }

    void HitHandler::InitializeMagicData()
    {
        const auto started = std::chrono::steady_clock::now();
        m_cloakAssociatedMagic.clear();
        m_specialMagicProfiles.clear();
        m_actorTypeDragon = nullptr;
        m_actorTypeDwarven = nullptr;

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            logger::error(
                "[SimpleStagger] TESDataHandler is null - magic classification unavailable");
            return;
        }

        m_actorTypeDragon = dataHandler->LookupForm<RE::BGSKeyword>(
            0x35D59, "Skyrim.esm");
        m_actorTypeDwarven = dataHandler->LookupForm<RE::BGSKeyword>(
            0x1397A, "Skyrim.esm");

        for (const auto* effect :
             dataHandler->GetFormArray<RE::EffectSetting>()) {
            if (!effect ||
                !effect->HasArchetype(RE::EffectArchetype::kCloak) ||
                !effect->data.associatedForm) {
                continue;
            }
            m_cloakAssociatedMagic.insert(
                effect->data.associatedForm->GetFormID());
        }

        std::size_t nativeStaggerProfiles = 0;
        for (const auto* spell : dataHandler->GetFormArray<RE::SpellItem>()) {
            const auto profile = BuildSpecialMagicProfile(spell);
            if (!profile) {
                continue;
            }
            m_specialMagicProfiles.emplace(spell->GetFormID(), *profile);
            nativeStaggerProfiles += profile->hasNativeStagger ? 1u : 0u;
        }

        const auto elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started).count();
        logger::info(
            "[SimpleStagger] Magic classification initialized in {} ms: "
            "{} Cloak source(s), {} Voice profile(s), "
            "{} with unconditional native Stagger",
            elapsedMs, m_cloakAssociatedMagic.size(),
            m_specialMagicProfiles.size(), nativeStaggerProfiles);
        if (!m_actorTypeDragon || !m_actorTypeDwarven) {
            logger::error(
                "[SimpleStagger] ActorTypeDragon/ActorTypeDwarven keyword lookup failed; "
                "the affected special attack category will remain disabled");
        }
    }

    bool HitHandler::IsCloakAssociatedMagic(
        RE::FormID a_source,
        RE::FormID a_magicItem) const
    {
        return m_cloakAssociatedMagic.contains(a_source) ||
               m_cloakAssociatedMagic.contains(a_magicItem);
    }

    std::optional<SpecialMagicProfile> HitHandler::GetSpecialMagicProfile(
        const RE::TESForm* a_sourceForm) const
    {
        if (!a_sourceForm || !a_sourceForm->Is(RE::FormType::Spell)) {
            return std::nullopt;
        }
        if (const auto it =
                m_specialMagicProfiles.find(a_sourceForm->GetFormID());
            it != m_specialMagicProfiles.end()) {
            return it->second;
        }
        return BuildSpecialMagicProfile(a_sourceForm->As<RE::MagicItem>());
    }

    void HitHandler::ClearCooldowns()
    {
        m_cooldowns.clear();
        m_magicHitState.Clear();
        logger::debug("[SimpleStagger] Cooldown maps cleared");
    }

    EventResult HitHandler::ProcessEvent(
        const RE::TESHitEvent* a_event,
        RE::BSTEventSource<RE::TESHitEvent>*)
    {
        if (!a_event) {
            return EventResult::kContinue;
        }

        auto* target = a_event->target ? a_event->target->As<RE::Actor>() : nullptr;
        auto* cause = a_event->cause ? a_event->cause->As<RE::Actor>() : nullptr;
        if (!target || !cause || !CanQueueSimpleStagger(target, cause)) {
            return EventResult::kContinue;
        }

        auto* sourceForm = RE::TESForm::LookupByID(a_event->source);
        if (!sourceForm) {
            return EventResult::kContinue;
        }

        auto* handler = GetSingleton();
        const auto* settings = Settings::GetSingleton();
        const auto specialProfile =
            handler->GetSpecialMagicProfile(sourceForm);
        auto specialCategory = SpecialMagicCategory::kNone;
        if (specialProfile) {
            const bool casterIsDragon =
                handler->m_actorTypeDragon &&
                cause->HasKeyword(handler->m_actorTypeDragon);
            const bool casterIsDwarven =
                handler->m_actorTypeDwarven &&
                cause->HasKeyword(handler->m_actorTypeDwarven);
            specialCategory = ClassifySpecialMagic(
                *specialProfile, casterIsDragon, casterIsDwarven);
        }
        const auto magicSource = ResolveMagicSource(sourceForm);
        const bool isSpecialMagic =
            specialCategory != SpecialMagicCategory::kNone;
        const bool isMagic = isSpecialMagic || magicSource.has_value();
        float magnitude = 0.0f;

        using HitFlag = RE::TESHitEvent::Flag;
        if (isMagic) {
            if (!IsMagicCategoryEnabled(
                    specialCategory,
                    settings->enableMagic.load(),
                    settings->enableDragonBreath.load(),
                    settings->enableOtherCreatureAttacks.load())) {
                return EventResult::kContinue;
            }

            if (isSpecialMagic) {
                magnitude = settings->breathAttackMagnitude.load();
            } else {
                const auto* magicItem = magicSource->magicItem;
                if (!IsMagicCastEnabled(
                        GetMagicCastKind(magicItem->GetCastingType()),
                        settings->enableConcentrationMagic.load())) {
                    return EventResult::kContinue;
                }
                const bool isCloakAssociated =
                    handler->IsCloakAssociatedMagic(
                        sourceForm->GetFormID(), magicItem->GetFormID());
                const auto minimumSkill =
                    GetDestructionMinimumSkill(*magicSource, isCloakAssociated);
                if (!minimumSkill) {
                    return EventResult::kContinue;
                }
                magnitude = GetMagicMagnitude(*settings, *minimumSkill);
            }
        } else {
            auto* weapon = sourceForm->As<RE::TESObjectWEAP>();
            if (!weapon) {
                return EventResult::kContinue;
            }

            const bool ranged = weapon->IsBow() || weapon->IsCrossbow();
            if (ranged) {
                if (!settings->enableRanged.load()) {
                    return EventResult::kContinue;
                }
            } else {
                if (!weapon->IsMelee() || !settings->enableMelee.load()) {
                    return EventResult::kContinue;
                }
            }

            // A bow bash is a melee action and keeps the melee magnitude order.
            const bool bash = a_event->flags.any(HitFlag::kBashAttack);
            if (ranged && !bash) {
                magnitude = settings->rangedAttackMagnitude.load();
            } else if (a_event->flags.any(HitFlag::kPowerAttack)) {
                magnitude = settings->powerAttackMagnitude.load();
            } else if (bash) {
                magnitude = settings->bashMagnitude.load();
            } else {
                magnitude = settings->normalAttackMagnitude.load();
            }
        }

        // Reject zero/invalid base values before either cooldown is changed.
        if (!IsPositiveFinite(magnitude)) {
            return EventResult::kContinue;
        }

        const bool successfullyGuarded =
            IsSuccessfullyGuardedHit(*a_event, target, cause, isMagic);
        const bool applyBlockBehavior = isMagic ?
            settings->enableWardBlockBehavior.load() :
            settings->enablePhysicalBlockBehavior.load();
        const auto blockedAction = GetBlockedHitAction(
            successfullyGuarded,
            applyBlockBehavior,
            settings->blockedHitBehavior.load());
        if (blockedAction == BlockedHitAction::kReject) {
            return EventResult::kContinue;
        }
        if (blockedAction == BlockedHitAction::kReduce) {
            magnitude *= settings->blockedMultiplier.load();
            if (!IsPositiveFinite(magnitude)) {
                return EventResult::kContinue;
            }
        }

        std::optional<MagicHitReservation> magicReservation;
        std::chrono::milliseconds magicCooldown{ 0 };
        if (isMagic) {
            const auto cooldown = settings->magicCooldownMs.load();
            magicCooldown = std::chrono::milliseconds(cooldown);
            const auto now = std::chrono::steady_clock::now();
            const ContinuousHitKey key{
                target->GetFormID(),
                cause->GetFormID(),
                sourceForm->GetFormID()
            };
            magicReservation = handler->m_magicHitState.TryReserve(
                key, now, magicCooldown);
            if (!magicReservation) {
                return EventResult::kContinue;
            }
        }

        const RE::ActorHandle targetHandle = target->GetHandle();
        const RE::ActorHandle causeHandle = cause->GetHandle();

        SKSE::GetTaskInterface()->AddTask(
            [targetHandle, causeHandle, magnitude, isMagic,
             magicReservation, magicCooldown]() mutable {
                auto finishMagic =
                    [&](MagicHitCompletion a_completion) {
                        if (magicReservation) {
                            HitHandler::GetSingleton()->m_magicHitState.Complete(
                                *magicReservation,
                                std::chrono::steady_clock::now(),
                                magicCooldown,
                                a_completion);
                        }
                    };

                auto targetPtr = targetHandle.get();
                auto causePtr = causeHandle.get();
                if (!targetPtr || !causePtr ||
                    !CanApplySimpleStagger(targetPtr.get(), causePtr.get())) {
                    finishMagic(MagicHitCompletion::kRetry);
                    return;
                }

                const auto* currentSettings = Settings::GetSingleton();
                const ResistResult resist = CheckResistance(targetPtr.get(), magnitude);
                if (resist == ResistResult::kBlock || !IsPositiveFinite(magnitude)) {
                    finishMagic(MagicHitCompletion::kRejected);
                    return;
                }

                if (ShouldBlockStaggerFromPerks(
                        targetPtr.get(), causePtr.get())) {
                    finishMagic(MagicHitCompletion::kRejected);
                    return;
                }

                // Weapon cooldown is main-thread-only and is updated only for a
                // hit that has survived resistance and final magnitude checks.
                if (!isMagic) {
                    const auto cooldown = currentSettings->staggerCooldownMs.load();
                    if (cooldown > 0) {
                        const auto targetID = targetPtr->GetFormID();
                        const auto now = std::chrono::steady_clock::now();
                        auto& handler = *HitHandler::GetSingleton();
                        const auto it = handler.m_cooldowns.find(targetID);
                        if (it != handler.m_cooldowns.end()) {
                            const auto elapsed =
                                std::chrono::duration_cast<std::chrono::milliseconds>(
                                    now - it->second).count();
                            if (elapsed < static_cast<long long>(cooldown)) {
                                finishMagic(MagicHitCompletion::kRetry);
                                return;
                            }
                        }
                        handler.m_cooldowns[targetID] = now;
                    }
                }

                if (!CanApplySimpleStagger(targetPtr.get(), causePtr.get())) {
                    finishMagic(MagicHitCompletion::kRetry);
                    return;
                }
                ApplyStagger(targetPtr.get(), causePtr.get(), magnitude);
                finishMagic(MagicHitCompletion::kApplied);
            });

        return EventResult::kContinue;
    }
}  // namespace SimpleStagger
