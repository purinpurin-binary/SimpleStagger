#include "Settings.h"

namespace SimpleStagger
{
    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    std::vector<std::string> Settings::ParseCommaSeparated(const char* a_str)
    {
        std::vector<std::string> result;
        if (!a_str || *a_str == '\0') {
            return result;
        }
        std::string_view sv(a_str);
        std::size_t start = 0;
        while (start < sv.size()) {
            auto comma = sv.find(',', start);
            if (comma == std::string_view::npos) {
                comma = sv.size();
            }
            auto token = sv.substr(start, comma - start);
            // Trim leading/trailing whitespace
            while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) {
                token.remove_prefix(1);
            }
            while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) {
                token.remove_suffix(1);
            }
            if (!token.empty()) {
                result.emplace_back(token);
            }
            start = comma + 1;
        }
        return result;
    }

    void Settings::ParseOverrideEntry(const std::string& a_raw, OverrideEntry& a_out)
    {
        a_out.rawString = a_raw;
        a_out.resolvedFormID = 0;
        a_out.keywordEditorID.clear();

        // Detect FormID pattern: starts with "0x" or contains "~"
        // Format: "0x123~Plugin.esp" or just "0x00012345~Skyrim.esm"
        auto tilde = a_raw.find('~');
        if (tilde != std::string::npos) {
            // FormID entry — defer resolution to ResolveOverrides()
            // Leave resolvedFormID = 0, rawString contains the full spec
            return;
        }

        // Otherwise treat as keyword EditorID string
        a_out.keywordEditorID = a_raw;
    }

    // ---------------------------------------------------------------------------
    // Load / Save
    // ---------------------------------------------------------------------------

    void Settings::Load()
    {
        CSimpleIniA ini;
        ini.SetUnicode();

        const auto rc = ini.LoadFile(kIniPath);
        const bool fileExisted = (rc >= SI_OK);

        // [General]
        enableMelee            = ini.GetBoolValue("General", "EnableMelee",            enableMelee);
        enableRanged           = ini.GetBoolValue("General", "EnableRanged",           enableRanged);
        playerCanBeStaggered   = ini.GetBoolValue("General", "PlayerCanBeStaggered",   playerCanBeStaggered);
        staggerCooldownMs      = static_cast<std::uint32_t>(
            ini.GetLongValue("General", "StaggerCooldownMs",
                static_cast<long>(staggerCooldownMs)));

        // [Stagger]
        staggerMagnitude       = static_cast<float>(ini.GetDoubleValue("Stagger", "StaggerMagnitude",     staggerMagnitude));
        powerAttackMagnitude   = static_cast<float>(ini.GetDoubleValue("Stagger", "PowerAttackMagnitude", powerAttackMagnitude));
        bashMagnitude          = static_cast<float>(ini.GetDoubleValue("Stagger", "BashMagnitude",        bashMagnitude));
        blockedHitBehavior     = static_cast<int>(ini.GetLongValue("Stagger", "BlockedHitBehavior",      blockedHitBehavior));
        blockedMultiplier      = static_cast<float>(ini.GetDoubleValue("Stagger", "BlockedMultiplier",    blockedMultiplier));

        // [LargeActor]
        massThreshold          = static_cast<float>(ini.GetDoubleValue("LargeActor", "MassThreshold",         massThreshold));
        largeActorMode         = static_cast<int>(ini.GetLongValue("LargeActor", "LargeActorMode",            largeActorMode));
        largeActorMultiplier   = static_cast<float>(ini.GetDoubleValue("LargeActor", "LargeActorMultiplier",   largeActorMultiplier));

        // [Overrides]
        {
            const char* immuneStr      = ini.GetValue("Overrides", "Immune",      "");
            const char* susceptibleStr = ini.GetValue("Overrides", "Susceptible", "");
            immuneRaw      = ParseCommaSeparated(immuneStr);
            susceptibleRaw = ParseCommaSeparated(susceptibleStr);
        }

        if (!fileExisted) {
            // Write a default INI on first run
            Save();
            logger::info("[SimpleStagger] INI not found – wrote defaults to {}", kIniPath);
        }

        logger::info("[SimpleStagger] Settings loaded: Melee={} Ranged={} PlayerStagger={} CooldownMs={} "
                     "StaggerMag={:.2f} PowerMag={:.2f} BashMag={:.2f} "
                     "BlockedBehavior={} BlockedMult={:.2f} "
                     "MassThreshold={:.2f} LargeMode={} LargeMult={:.2f}",
            enableMelee, enableRanged, playerCanBeStaggered, staggerCooldownMs,
            staggerMagnitude, powerAttackMagnitude, bashMagnitude,
            blockedHitBehavior, blockedMultiplier,
            massThreshold, largeActorMode, largeActorMultiplier);
    }

    void Settings::Save()
    {
        CSimpleIniA ini;
        ini.SetUnicode();
        // Load existing file first so we don't clobber user edits
        ini.LoadFile(kIniPath);

        // [General]
        ini.SetBoolValue("General", "EnableMelee",          enableMelee,          "; Enable stagger on melee hits");
        ini.SetBoolValue("General", "EnableRanged",         enableRanged,         "; Enable stagger on ranged hits");
        ini.SetBoolValue("General", "PlayerCanBeStaggered", playerCanBeStaggered, "; Allow the player character to be staggered");
        ini.SetLongValue("General", "StaggerCooldownMs",    static_cast<long>(staggerCooldownMs),
            "; Minimum milliseconds between staggers for the same target (0 = disabled)");

        // [Stagger]
        ini.SetDoubleValue("Stagger", "StaggerMagnitude",     staggerMagnitude,     "; Base stagger magnitude (normal hit)");
        ini.SetDoubleValue("Stagger", "PowerAttackMagnitude", powerAttackMagnitude, "; Stagger magnitude for power attacks");
        ini.SetDoubleValue("Stagger", "BashMagnitude",        bashMagnitude,        "; Stagger magnitude for bashes");
        ini.SetLongValue  ("Stagger", "BlockedHitBehavior",   blockedHitBehavior,
            "; 0 = no stagger on blocked hits; 1 = stagger with BlockedMultiplier applied");
        ini.SetDoubleValue("Stagger", "BlockedMultiplier",    blockedMultiplier,
            "; Magnitude multiplier when hit is blocked (used if BlockedHitBehavior=1)");

        // [LargeActor]
        ini.SetDoubleValue("LargeActor", "MassThreshold",        massThreshold,
            "; Actors whose (kMass * scale) >= this value are treated as large");
        ini.SetLongValue  ("LargeActor", "LargeActorMode",       largeActorMode,
            "; 0 = never stagger large actors; 1 = stagger with LargeActorMultiplier");
        ini.SetDoubleValue("LargeActor", "LargeActorMultiplier", largeActorMultiplier,
            "; Magnitude multiplier for large actors (used if LargeActorMode=1)");

        // [Overrides]
        {
            std::string immuneStr;
            for (std::size_t i = 0; i < immuneRaw.size(); ++i) {
                if (i) immuneStr += ',';
                immuneStr += immuneRaw[i];
            }
            std::string susceptibleStr;
            for (std::size_t i = 0; i < susceptibleRaw.size(); ++i) {
                if (i) susceptibleStr += ',';
                susceptibleStr += susceptibleRaw[i];
            }
            ini.SetValue("Overrides", "Immune",      immuneStr.c_str(),
                "; Comma-separated list of Race FormIDs (0xABCD~Plugin.esp) or keyword EditorIDs that are NEVER staggered");
            ini.SetValue("Overrides", "Susceptible", susceptibleStr.c_str(),
                "; Comma-separated list of Race FormIDs or keyword EditorIDs that ALWAYS stagger (ignore LargeActor check)");
        }

        const auto rc = ini.SaveFile(kIniPath);
        if (rc < SI_OK) {
            logger::warn("[SimpleStagger] Failed to save INI (error {})", static_cast<int>(rc));
        }
    }

    // ---------------------------------------------------------------------------
    // Resolve overrides (call from kDataLoaded)
    // ---------------------------------------------------------------------------

    void Settings::ResolveOverrides()
    {
        auto resolveList = [](const std::vector<std::string>& rawList,
                              std::vector<OverrideEntry>&      outList) {
            outList.clear();
            outList.reserve(rawList.size());

            auto* dh = RE::TESDataHandler::GetSingleton();

            for (const auto& raw : rawList) {
                OverrideEntry entry;
                ParseOverrideEntry(raw, entry);

                if (!entry.keywordEditorID.empty()) {
                    // Keyword EditorID: validate it exists (optional, just log if not found)
                    outList.push_back(std::move(entry));
                    continue;
                }

                // FormID entry: parse "0xABCD~Plugin.esp"
                auto tilde = raw.find('~');
                if (tilde == std::string::npos) {
                    logger::warn("[SimpleStagger] Override '{}' has no '~' separator, skipping", raw);
                    continue;
                }

                const std::string hexPart    = raw.substr(0, tilde);
                const std::string pluginPart = raw.substr(tilde + 1);

                RE::FormID localID = 0;
                try {
                    localID = static_cast<RE::FormID>(std::stoul(hexPart, nullptr, 16));
                } catch (...) {
                    logger::warn("[SimpleStagger] Override '{}': invalid FormID hex '{}', skipping",
                        raw, hexPart);
                    continue;
                }

                if (!dh) {
                    logger::warn("[SimpleStagger] TESDataHandler not available, skipping override '{}'", raw);
                    continue;
                }

                auto* form = dh->LookupForm(localID, pluginPart);
                if (!form) {
                    logger::warn("[SimpleStagger] Override '{}': form not found (plugin '{}' localID 0x{:X}), skipping",
                        raw, pluginPart, localID);
                    continue;
                }

                entry.resolvedFormID = form->GetFormID();
                outList.push_back(std::move(entry));
            }
        };

        resolveList(immuneRaw,      immuneEntries);
        resolveList(susceptibleRaw, susceptibleEntries);

        logger::info("[SimpleStagger] Overrides resolved: {} immune, {} susceptible",
            immuneEntries.size(), susceptibleEntries.size());
    }

}  // namespace SimpleStagger
