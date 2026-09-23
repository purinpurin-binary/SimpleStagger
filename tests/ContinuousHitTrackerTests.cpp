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

    return EXIT_SUCCESS;
}
