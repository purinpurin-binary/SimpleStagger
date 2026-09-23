#pragma once

namespace SimpleStagger
{
    /// Event-thread-safe subset of the stagger gate. It only reads runtime
    /// actor data through CommonLib's version-aware accessors.
    [[nodiscard]] bool CanQueueSimpleStagger(
        const RE::Actor* a_target,
        const RE::Actor* a_aggressor = nullptr);

    /// Full game-thread state gate used when the queued task runs and
    /// immediately before graph notification. This intentionally does not
    /// reject an already-staggering actor so SBS stagger chains keep working.
    /// Sleeping/waking actors and actors that still occupy furniture are
    /// rejected so their interaction graph can finish first.
    [[nodiscard]] bool CanApplySimpleStagger(
        const RE::Actor* a_target,
        const RE::Actor* a_aggressor = nullptr);

    /// Apply a stagger animation to a_target.
    /// a_aggressor may be nullptr (direction defaults to 0.0 = forward stagger).
    ///
    /// Graph variable sequence (confirmed from UselessFenixUtils / valhallaCombat source):
    ///   1. target->GetHeadingAngle(aggressor->GetPosition(), false)  => heading in [-180,180)
    ///   2. normalise to [0, 1]: dir = (heading >= 0) ? heading/360 : (360+heading)/360
    ///   3. target->SetGraphVariableFloat("staggerDirection", dir)
    ///   4. target->SetGraphVariableFloat("staggerMagnitude", mag)
    ///   5. target->NotifyAnimationGraph("staggerStart")
    ///
    /// NOTE: po3 PapyrusExtender (current master) does NOT contain a StaggerActor function.
    /// The implementation below is derived from UselessFenixUtils (fenix31415) and
    /// valhallaCombat (D7ry), both of which implement the same variable/event sequence.
    void ApplyStagger(RE::Actor* a_target, RE::Actor* a_aggressor, float a_magnitude);

}  // namespace SimpleStagger
