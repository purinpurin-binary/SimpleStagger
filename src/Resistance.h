#pragma once

namespace SimpleStagger
{
    /// Result of the resistance check.
    enum class ResistResult
    {
        kAllow,     ///< Apply stagger with the given magnitude (unchanged).
        kReduce,    ///< Apply stagger but reduce magnitude by LargeActorMultiplier.
        kBlock,     ///< Do not apply stagger at all.
    };

    /// Check whether a_target should receive a stagger, and optionally adjust the magnitude.
    ///
    /// Evaluation order:
    ///   1. Susceptible override (Race FormID or keyword) -> kAllow (skip mass check).
    ///   2. Immune override (Race FormID or keyword)      -> kBlock.
    ///   3. Mass × scale >= MassThreshold:
    ///      LargeActorMode=0 -> kBlock
    ///      LargeActorMode=1 -> kReduce
    ///   4. Otherwise -> kAllow.
    ///
    /// a_magnitude is updated in-place when kReduce is returned.
    ResistResult CheckResistance(RE::Actor* a_target, float& a_magnitude);

}  // namespace SimpleStagger
