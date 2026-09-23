#include "Resistance.h"
#include "Settings.h"
#include <SKSE/Impl/Util.h>

namespace SimpleStagger
{
    namespace
    {
        constexpr RE::FormID kTowerOfStrengthFormID = 0x00058F6C;
        constexpr auto kIncomingStaggerEntryPoint =
            RE::BGSEntryPoint::ENTRY_POINT::kModIncomingStagger;

        constexpr std::string_view kSkyrimPlugin = "Skyrim.esm";
        constexpr std::string_view kMasterOfOnePlugin = "master_of_one.esp";
        constexpr std::string_view kVokriiPlugin =
            "Vokrii - Minimalistic Perks of Skyrim.esp";
        constexpr std::string_view kOrdinatorPlugin =
            "Ordinator - Perks of Skyrim.esp";

        struct IncomingStaggerPerkCache
        {
            std::array<RE::BGSPerk*, 12> perks{};
            std::size_t count{ 0 };
            bool initialized{ false };
        };

        IncomingStaggerPerkCache g_incomingStaggerPerks;

        [[nodiscard]] constexpr float IncomingMultiplierToBlockChance(
            float a_multiplier) noexcept
        {
            if (a_multiplier <= 0.0f) {
                return 1.0f;
            }
            if (a_multiplier >= 1.0f) {
                return 0.0f;
            }
            return 1.0f - a_multiplier;
        }

        [[nodiscard]] constexpr bool IsIncomingStaggerBlockRoll(
            float a_blockChance,
            float a_roll) noexcept
        {
            return a_roll < a_blockChance;
        }

        static_assert(IncomingMultiplierToBlockChance(1.0f) == 0.0f);
        static_assert(IncomingMultiplierToBlockChance(0.5f) == 0.5f);
        static_assert(IncomingMultiplierToBlockChance(0.0f) == 1.0f);
        static_assert(IncomingMultiplierToBlockChance(-1.0f) == 1.0f);
        static_assert(IncomingMultiplierToBlockChance(2.0f) == 0.0f);
        static_assert(IsIncomingStaggerBlockRoll(0.5f, 0.499f));
        static_assert(!IsIncomingStaggerBlockRoll(0.5f, 0.5f));

        [[nodiscard]] bool HasExpectedWinningFile(
            const RE::BGSPerk* a_perk,
            const RE::TESFile* a_expectedWinningFile)
        {
            return a_perk &&
                   a_expectedWinningFile &&
                   a_perk->GetFile() == a_expectedWinningFile;
        }

        [[nodiscard]] bool HasIncomingStaggerEntry(const RE::BGSPerk* a_perk)
        {
            if (!a_perk) {
                return false;
            }

            for (auto* perkEntry : a_perk->perkEntries) {
                if (!perkEntry ||
                    perkEntry->GetType() != RE::PERK_ENTRY_TYPE::kEntryPoint) {
                    continue;
                }

                const auto* entryPoint =
                    static_cast<const RE::BGSEntryPointPerkEntry*>(perkEntry);
                if (entryPoint->IsEntryPoint(kIncomingStaggerEntryPoint)) {
                    return true;
                }
            }
            return false;
        }

        void RegisterIncomingStaggerPerk(
            RE::BGSPerk* a_perk,
            const RE::TESFile* a_expectedWinningFile,
            std::string_view a_label)
        {
            if (!a_perk) {
                logger::warn(
                    "[SimpleStagger] {} compatibility perk was not found", a_label);
                return;
            }

            if (!HasExpectedWinningFile(a_perk, a_expectedWinningFile)) {
                logger::warn(
                    "[SimpleStagger] {} compatibility disabled: "
                    "the winning plugin is not the expected supported plugin",
                    a_label);
                return;
            }

            if (!HasIncomingStaggerEntry(a_perk)) {
                logger::warn(
                    "[SimpleStagger] {} compatibility disabled: "
                    "ModIncomingStagger entry is absent",
                    a_label);
                return;
            }

            auto& cache = g_incomingStaggerPerks;
            if (std::find(
                    cache.perks.begin(),
                    cache.perks.begin() + static_cast<std::ptrdiff_t>(cache.count),
                    a_perk) !=
                cache.perks.begin() + static_cast<std::ptrdiff_t>(cache.count)) {
                return;
            }

            if (cache.count >= cache.perks.size()) {
                logger::error(
                    "[SimpleStagger] Incoming-stagger perk cache is full; "
                    "{} was not registered",
                    a_label);
                return;
            }

            cache.perks[cache.count++] = a_perk;
            logger::debug(
                "[SimpleStagger] Registered {} ({:#010x})",
                a_label,
                a_perk->GetFormID());
        }

        [[nodiscard]] bool HasAnySupportedIncomingStaggerPerk(RE::Actor* a_actor)
        {
            const auto& cache = g_incomingStaggerPerks;
            for (std::size_t index = 0; index < cache.count; ++index) {
                if (a_actor->HasPerk(cache.perks[index])) {
                    return true;
                }
            }
            return false;
        }

        void AppendEnvironmentName(
            std::string& a_environment,
            std::string_view a_name)
        {
            if (!a_environment.empty()) {
                a_environment += " + ";
            }
            a_environment += a_name;
        }

        /// Returns true if a_actor matches the override entry (race FormID or keyword).
        bool MatchesOverride(RE::Actor* a_actor, const OverrideEntry& a_entry)
        {
            if (!a_actor) {
                return false;
            }

            // FormID match: compare the actor's race FormID
            if (a_entry.isFormID()) {
                const auto* race = a_actor->GetRace();
                if (race && race->GetFormID() == a_entry.resolvedFormID) {
                    return true;
                }
            }

            // Keyword match: HasKeywordString
            if (a_entry.isKeyword()) {
                if (a_actor->HasKeywordString(a_entry.keywordEditorID)) {
                    return true;
                }
            }

            return false;
        }

        bool MatchesAnyOverride(RE::Actor* a_actor, const Settings::OverrideList& a_entries)
        {
            for (const auto& entry : a_entries) {
                if (MatchesOverride(a_actor, entry)) {
                    return true;
                }
            }
            return false;
        }
    }

    bool InitializeIncomingStaggerPerkSupport()
    {
        g_incomingStaggerPerks = {};

        auto* const dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            logger::error(
                "[SimpleStagger] TESDataHandler is unavailable; "
                "incoming-stagger perk support is disabled");
            return false;
        }

        const auto* const skyrimFile =
            dataHandler->LookupModByName(kSkyrimPlugin);
        const auto* const masterOfOneFile =
            dataHandler->LookupModByName(kMasterOfOnePlugin);
        const auto* const vokriiFile =
            dataHandler->LookupModByName(kVokriiPlugin);
        const auto* const ordinatorFile =
            dataHandler->LookupModByName(kOrdinatorPlugin);

        std::string environment;
        if (masterOfOneFile) {
            AppendEnvironmentName(environment, "Master of One");
        }
        if (vokriiFile) {
            AppendEnvironmentName(environment, "Vokrii");
        }
        if (ordinatorFile) {
            AppendEnvironmentName(environment, "Ordinator");
        }
        if (environment.empty()) {
            environment = "Vanilla";
        }

        // BGSPerk does not retain its EditorID at runtime. Validate the exact
        // FormID by its winning source file instead. This distinguishes the
        // vanilla record from Vokrii/Ordinator repurposes without relying on
        // Skyrim's sparse runtime EditorID map.
        RegisterIncomingStaggerPerk(
            RE::TESForm::LookupByID<RE::BGSPerk>(kTowerOfStrengthFormID),
            skyrimFile,
            "Vanilla Tower of Strength");

        if (masterOfOneFile) {
            constexpr std::array<std::string_view, 5> labels{
                "Master of One Tower of Strength rank 1",
                "Master of One Tower of Strength rank 2",
                "Master of One Tower of Strength rank 3",
                "Master of One Tower of Strength rank 4",
                "Master of One Tower of Strength rank 5"
            };

            for (std::size_t index = 0; index < labels.size(); ++index) {
                const auto localFormID =
                    static_cast<RE::FormID>(0x001F0F28 + index);
                RegisterIncomingStaggerPerk(
                    dataHandler->LookupForm<RE::BGSPerk>(
                        localFormID, kMasterOfOnePlugin),
                    masterOfOneFile,
                    labels[index]);
            }
        }

        if (vokriiFile) {
            RegisterIncomingStaggerPerk(
                RE::TESForm::LookupByID<RE::BGSPerk>(0x00105F1F),
                vokriiFile,
                "Vokrii Hunter's Focus");
            RegisterIncomingStaggerPerk(
                dataHandler->LookupForm<RE::BGSPerk>(0x002DF673, kVokriiPlugin),
                vokriiFile,
                "Vokrii Stoneheart");
            RegisterIncomingStaggerPerk(
                RE::TESForm::LookupByID<RE::BGSPerk>(0x0003AFA7),
                vokriiFile,
                "Vokrii Berserker");
        }

        if (ordinatorFile) {
            RegisterIncomingStaggerPerk(
                dataHandler->LookupForm<RE::BGSPerk>(0x00118005, kOrdinatorPlugin),
                ordinatorFile,
                "Ordinator Focus on the Prey");
            RegisterIncomingStaggerPerk(
                dataHandler->LookupForm<RE::BGSPerk>(0x000878E5, kOrdinatorPlugin),
                ordinatorFile,
                "Ordinator Witchmaster stagger immunity");
            RegisterIncomingStaggerPerk(
                dataHandler->LookupForm<RE::BGSPerk>(0x000C8CBF, kOrdinatorPlugin),
                ordinatorFile,
                "Ordinator Bedrock Proc");
        }

        g_incomingStaggerPerks.initialized = true;
        logger::info(
            "[SimpleStagger] Incoming-stagger perk environment: {}; "
            "{} validated perk(s)",
            environment,
            g_incomingStaggerPerks.count);
        return g_incomingStaggerPerks.count > 0;
    }

    ResistResult CheckResistance(RE::Actor* a_target, float& a_magnitude)
    {
        if (!a_target) {
            return ResistResult::kBlock;
        }

        const auto* cfg = Settings::GetSingleton();

        // 1. Susceptible override -> always allow (bypass mass check)
        const auto overrides = cfg->GetOverrideSnapshot();
        if (MatchesAnyOverride(a_target, overrides->susceptible)) {
            return ResistResult::kAllow;
        }

        // 2. Immune override -> always block
        if (MatchesAnyOverride(a_target, overrides->immune)) {
            return ResistResult::kBlock;
        }

        // 3. Mass threshold check
        // RE::ActorValue::kMass = 36 (confirmed from alandtse/CommonLibVR ng ActorValues.h)
        const auto* avOwner = a_target->AsActorValueOwner();
        if (avOwner) {
            const float mass  = avOwner->GetActorValue(RE::ActorValue::kMass);
            const float scale = a_target->GetScale();
            if (mass * scale >= cfg->massThreshold.load()) {
                if (cfg->largeActorMode.load() != 1) {
                    return ResistResult::kBlock;
                } else {
                    // Mode 1: reduce magnitude
                    a_magnitude *= cfg->largeActorMultiplier.load();
                    return ResistResult::kReduce;
                }
            }
        }

        return ResistResult::kAllow;
    }

    bool ShouldBlockStaggerFromPerks(
        RE::Actor* a_target,
        RE::Actor* a_aggressor)
    {
        const auto& cache = g_incomingStaggerPerks;
        if (!a_target ||
            !cache.initialized ||
            cache.count == 0 ||
            !a_target->GetMiddleHighProcess() ||
            !HasAnySupportedIncomingStaggerPerk(a_target)) {
            return false;
        }

        float incomingStaggerMultiplier = 1.0f;
        RE::TESForm* conditionAggressor = nullptr;
        if (a_aggressor && a_aggressor != a_target) {
            conditionAggressor = static_cast<RE::TESForm*>(a_aggressor);
        }

        // ModIncomingStagger has one condition target (the aggressor), followed
        // by the output value. Skyrim evaluates the perk's own CTDAs here,
        // including armor counts, attack state, blocking, bow draw state, and
        // temporary Proc perks.
        RE::BGSEntryPoint::HandleEntryPoint(
            kIncomingStaggerEntryPoint,
            a_target,
            conditionAggressor,
            std::addressof(incomingStaggerMultiplier));

        if (!std::isfinite(incomingStaggerMultiplier)) {
            return false;
        }

        const float blockChance =
            IncomingMultiplierToBlockChance(incomingStaggerMultiplier);
        if (blockChance <= 0.0f) {
            return false;
        }
        if (blockChance >= 1.0f) {
            return true;
        }

        const float roll =
            SKSE::stl::RNG::GetSingleton()->Generate<float>(0.0f, 1.0f);
        return IsIncomingStaggerBlockRoll(blockChance, roll);
    }

}  // namespace SimpleStagger
