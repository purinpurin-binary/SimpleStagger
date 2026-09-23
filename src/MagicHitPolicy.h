#pragma once

#include <cstdint>

namespace SimpleStagger
{
    enum class MagicSourceKind : std::uint8_t
    {
        kHandSpell,
        kStaff,
        kScroll,
        kOther
    };

    enum class MagicCastKind : std::uint8_t
    {
        kFireAndForget,
        kConcentration,
        kOther
    };

    struct MagicHitTraits
    {
        MagicSourceKind source{ MagicSourceKind::kOther };
        MagicCastKind cast{ MagicCastKind::kOther };
        bool aimed{ false };
        bool hasEmittedDestructionEffect{ false };
        bool isCloakAssociated{ false };
        // Conditional Stagger effects, such as the Impact perk effect, must
        // not exclude the entire spell.
        bool hasNativeStagger{ false };
    };

    [[nodiscard]] constexpr bool IsUnconditionalNativeStagger(
        bool a_hasStaggerArchetype,
        bool a_effectHasConditions,
        bool a_baseEffectHasConditions) noexcept
    {
        return a_hasStaggerArchetype &&
               !a_effectHasConditions &&
               !a_baseEffectHasConditions;
    }

    [[nodiscard]] constexpr bool IsEligibleMagicStagger(
        const MagicHitTraits& a_traits) noexcept
    {
        const bool supportedSource =
            a_traits.source == MagicSourceKind::kHandSpell ||
            a_traits.source == MagicSourceKind::kStaff ||
            a_traits.source == MagicSourceKind::kScroll;
        const bool supportedCast =
            a_traits.cast == MagicCastKind::kFireAndForget ||
            a_traits.cast == MagicCastKind::kConcentration;

        return supportedSource &&
               supportedCast &&
               a_traits.aimed &&
               a_traits.hasEmittedDestructionEffect &&
               !a_traits.isCloakAssociated &&
               !a_traits.hasNativeStagger;
    }

    /// Concentration beams from normal spells, staffs, and scrolls can be
    /// disabled independently. Special Voice attacks use their own switches.
    [[nodiscard]] constexpr bool IsMagicCastEnabled(
        MagicCastKind a_cast,
        bool a_enableConcentration) noexcept
    {
        return a_cast != MagicCastKind::kConcentration ||
               a_enableConcentration;
    }

    struct SpecialMagicProfile
    {
        MagicCastKind cast{ MagicCastKind::kOther };
        bool aimed{ false };
        bool hasHostileHealthProjectile{ false };
        bool hasFireDamage{ false };
        bool hasFrostDamage{ false };
        bool hasNativeStagger{ false };
    };

    enum class SpecialMagicCategory : std::uint8_t
    {
        kNone,
        kDragonBreath,
        kOtherCreatureAttack
    };

    enum class BlockedHitAction : std::uint8_t
    {
        kKeep,
        kReject,
        kReduce
    };

    struct GuardHitEvidence
    {
        bool eventFlag{ false };
        bool hitDataMatches{ false };
        bool hitDataBlocked{ false };
        bool hasBlockedDamage{ false };
        bool blockCandidate{ false };
        bool defenderBlocking{ false };
        bool wardAbsorbed{ false };
    };

    /// TESHitEvent::kHitBlocked is not populated by every successful shield or
    /// Ward path. Matching HitData and the transient Ward absorb state provide
    /// event-time fallbacks without treating a broken Ward as a successful
    /// guard.
    [[nodiscard]] constexpr bool WasSuccessfullyGuarded(
        const GuardHitEvidence& a_evidence) noexcept
    {
        return a_evidence.eventFlag ||
               (a_evidence.hitDataMatches &&
                (a_evidence.hitDataBlocked ||
                 a_evidence.hasBlockedDamage ||
                 (a_evidence.blockCandidate &&
                  a_evidence.defenderBlocking))) ||
               a_evidence.wardAbsorbed;
    }

    /// When handling is disabled, a successful guard is treated like an
    /// ordinary hit. Behavior 0 rejects it; behavior 1 reduces its magnitude.
    [[nodiscard]] constexpr BlockedHitAction GetBlockedHitAction(
        bool a_isBlocked,
        bool a_applyBehavior,
        int a_behavior) noexcept
    {
        if (!a_isBlocked || !a_applyBehavior) {
            return BlockedHitAction::kKeep;
        }
        return a_behavior == 1 ?
            BlockedHitAction::kReduce :
            BlockedHitAction::kReject;
    }

    [[nodiscard]] constexpr bool IsMagicCategoryEnabled(
        SpecialMagicCategory a_category,
        bool a_enableMagic,
        bool a_enableDragonBreath,
        bool a_enableOtherCreatureAttacks) noexcept
    {
        switch (a_category) {
        case SpecialMagicCategory::kDragonBreath:
            return a_enableDragonBreath;
        case SpecialMagicCategory::kOtherCreatureAttack:
            return a_enableOtherCreatureAttacks;
        case SpecialMagicCategory::kNone:
        default:
            return a_enableMagic;
        }
    }

    [[nodiscard]] constexpr SpecialMagicCategory ClassifySpecialMagic(
        const SpecialMagicProfile& a_profile,
        bool a_casterIsDragon,
        bool a_casterIsDwarven) noexcept
    {
        if (!a_profile.aimed ||
            !a_profile.hasHostileHealthProjectile ||
            a_profile.hasNativeStagger) {
            return SpecialMagicCategory::kNone;
        }

        if (a_profile.cast == MagicCastKind::kConcentration) {
            return a_casterIsDragon ?
                SpecialMagicCategory::kDragonBreath :
                SpecialMagicCategory::kOtherCreatureAttack;
        }

        if (a_profile.cast != MagicCastKind::kFireAndForget ||
            (!a_profile.hasFireDamage && !a_profile.hasFrostDamage)) {
            return SpecialMagicCategory::kNone;
        }

        if (a_casterIsDragon) {
            return SpecialMagicCategory::kDragonBreath;
        }
        if (a_casterIsDwarven && a_profile.hasFireDamage) {
            return SpecialMagicCategory::kOtherCreatureAttack;
        }
        return SpecialMagicCategory::kNone;
    }
}
