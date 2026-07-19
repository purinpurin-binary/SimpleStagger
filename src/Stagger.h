#pragma once

namespace SimpleStagger
{
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
