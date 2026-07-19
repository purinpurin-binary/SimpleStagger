#include "Stagger.h"

namespace SimpleStagger
{
    void ApplyStagger(RE::Actor* a_target, RE::Actor* a_aggressor, float a_magnitude)
    {
        if (!a_target) {
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
