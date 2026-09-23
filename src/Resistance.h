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

    /// Detect the active supported perk overhaul(s) and cache their incoming
    /// stagger perks. Call once after kDataLoaded, before registering the
    /// hit-event sink.
    ///
    /// Supported environments:
    ///   - Vanilla Skyrim
    ///   - Master of One
    ///   - Vokrii
    ///   - Ordinator
    ///
    /// Every cached form is validated by its winning source plugin and by the
    /// presence of a ModIncomingStagger entry. A record overridden by an
    /// unsupported plugin therefore does not grant unrelated stagger resistance.
    [[nodiscard]] bool InitializeIncomingStaggerPerkSupport();

    /// Return true when a supported incoming-stagger perk blocks this stagger.
    ///
    /// Skyrim evaluates the perk entry conditions and produces the final
    /// incoming-stagger multiplier. SimpleStagger interprets that multiplier
    /// as the probability that the stagger succeeds:
    ///   1.0 -> never block
    ///   0.5 -> block 50% of attempts
    ///   0.0 -> always block
    ///
    /// This must run on the game thread because it calls engine functions.
    [[nodiscard]] bool ShouldBlockStaggerFromPerks(
        RE::Actor* a_target,
        RE::Actor* a_aggressor);

}  // namespace SimpleStagger
