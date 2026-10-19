#pragma once
#include <SimpleIni.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace SimpleStagger
{
    struct OverrideEntry
    {
        RE::FormID resolvedFormID{ 0 };
        std::string keywordEditorID{};
        std::string rawString{};

        [[nodiscard]] bool isFormID() const noexcept { return resolvedFormID != 0; }
        [[nodiscard]] bool isKeyword() const noexcept { return !keywordEditorID.empty(); }
    };

    class Settings : public REX::Singleton<Settings>
    {
    public:
        using OverrideList = std::vector<OverrideEntry>;
        struct OverrideSnapshot
        {
            OverrideList immune{};
            OverrideList susceptible{};
        };

        // [General]
        std::atomic_bool           enableMelee{ true };
        std::atomic_bool           enableRanged{ true };
        std::atomic_bool           enableMagic{ false };
        std::atomic_bool           enableConcentrationMagic{ false };
        std::atomic_bool           enableDragonBreath{ false };
        std::atomic_bool           enableOtherCreatureAttacks{ false };
        std::atomic_bool           playerCanBeStaggered{ true };
        std::atomic_bool           npcsCanStagger{ true };
        std::atomic<std::uint32_t> staggerCooldownMs{ 0 };
        std::atomic<std::uint32_t> magicCooldownMs{ 200 };

        // [Stagger]
        std::atomic_bool  enablePhysicalBlockBehavior{ true };
        std::atomic_bool  enableWardBlockBehavior{ true };
        std::atomic<int>  blockedHitBehavior{ 0 };
        std::atomic<float> blockedMultiplier{ 0.5f };
        std::atomic<float> normalAttackMagnitude{ 0.25f };
        std::atomic<float> powerAttackMagnitude{ 0.40f };
        std::atomic<float> bashMagnitude{ 0.25f };
        std::atomic<float> rangedAttackMagnitude{ 0.25f };
        std::atomic<float> noviceMagicMagnitude{ 0.10f };
        std::atomic<float> apprenticeMagicMagnitude{ 0.15f };
        std::atomic<float> adeptMagicMagnitude{ 0.25f };
        std::atomic<float> expertMagicMagnitude{ 0.35f };
        std::atomic<float> masterMagicMagnitude{ 0.50f };
        std::atomic<float> breathAttackMagnitude{ 0.25f };

        // [LargeActor]
        std::atomic<float> massThreshold{ 4.0f };
        std::atomic<int>   largeActorMode{ 0 };
        std::atomic<float> largeActorMultiplier{ 0.5f };

        [[nodiscard]] bool Load();
        [[nodiscard]] bool Save();
        [[nodiscard]] bool ResolveOverrides();

        void SetOverrideText(std::string_view a_immune, std::string_view a_susceptible);
        [[nodiscard]] std::pair<std::string, std::string> GetOverrideText() const;

        [[nodiscard]] std::shared_ptr<const OverrideSnapshot> GetOverrideSnapshot() const noexcept;

    private:
        static constexpr const char* kIniPath = "Data/SKSE/Plugins/SimpleStagger.ini";

        static std::vector<std::string> ParseCommaSeparated(const char* a_str);
        static void ParseOverrideEntry(const std::string& a_raw, OverrideEntry& a_out);

        mutable std::mutex       m_overrideRawMutex;
        std::vector<std::string> m_immuneRaw{};
        std::vector<std::string> m_susceptibleRaw{};

        std::atomic<std::shared_ptr<const OverrideSnapshot>> m_overrideSnapshot{
            std::make_shared<const OverrideSnapshot>()
        };
    };
}  // namespace SimpleStagger
