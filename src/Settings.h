#pragma once
#include <SimpleIni.h>
#include <vector>
#include <string>

namespace SimpleStagger
{
    /// Parsed override entry: either a Race FormID (plugin~local) or a keyword EditorID string.
    struct OverrideEntry
    {
        // For FormID entries (populated in kDataLoaded after resolution)
        RE::FormID resolvedFormID{ 0 };
        // For keyword string entries
        std::string keywordEditorID{};
        // Raw string as parsed from INI, e.g. "0x123~Plugin.esp" or "ActorTypeDragon"
        std::string rawString{};

        [[nodiscard]] bool isFormID() const noexcept { return resolvedFormID != 0; }
        [[nodiscard]] bool isKeyword() const noexcept { return !keywordEditorID.empty(); }
    };

    class Settings : public REX::Singleton<Settings>
    {
    public:
        // [General]
        bool enableMelee{ true };
        bool enableRanged{ true };
        bool playerCanBeStaggered{ true };
        std::uint32_t staggerCooldownMs{ 0 };  // 0 = disabled

        // [Stagger]
        float staggerMagnitude{ 0.25f };
        float powerAttackMagnitude{ 0.4f };
        float bashMagnitude{ 0.25f };
        int   blockedHitBehavior{ 0 };   // 0=none, 1=reduce
        float blockedMultiplier{ 0.5f };

        // [LargeActor]
        float massThreshold{ 4.0f };
        int   largeActorMode{ 0 };       // 0=skip, 1=reduce
        float largeActorMultiplier{ 0.5f };

        // [Overrides] – raw strings parsed from INI
        std::vector<std::string> immuneRaw{};
        std::vector<std::string> susceptibleRaw{};

        // Resolved in kDataLoaded
        std::vector<OverrideEntry> immuneEntries{};
        std::vector<OverrideEntry> susceptibleEntries{};

        /// Load from Data/SKSE/Plugins/SimpleStagger.ini.
        /// If the file is absent, writes defaults and returns.
        void Load();

        /// Write current values back to the INI (for future in-game menu use).
        void Save();

        /// Resolve raw FormID strings ("0x123~Plugin.esp") and keyword EditorIDs.
        /// Call this from kDataLoaded after the data is ready.
        void ResolveOverrides();

    private:
        static constexpr const char* kIniPath = "Data/SKSE/Plugins/SimpleStagger.ini";

        static std::vector<std::string> ParseCommaSeparated(const char* a_str);
        static void ParseOverrideEntry(const std::string& a_raw, OverrideEntry& a_out);
    };

}  // namespace SimpleStagger
