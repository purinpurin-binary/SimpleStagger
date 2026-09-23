#include "Settings.h"

namespace SimpleStagger
{
    namespace
    {
        constexpr float kDefaultNormalMagnitude = 0.25f;
        constexpr float kDefaultPowerMagnitude = 0.40f;
        constexpr float kDefaultBashMagnitude = 0.25f;
        constexpr float kDefaultNoviceMagicMagnitude = 0.10f;
        constexpr float kDefaultApprenticeMagicMagnitude = 0.15f;
        constexpr float kDefaultAdeptMagicMagnitude = 0.25f;
        constexpr float kDefaultExpertMagicMagnitude = 0.35f;
        constexpr float kDefaultMasterMagicMagnitude = 0.50f;
        constexpr float kDefaultBreathAttackMagnitude = 0.25f;

        [[nodiscard]] std::uint32_t ReadUInt(
            const CSimpleIniA& a_ini,
            const char* a_section,
            const char* a_key,
            std::uint32_t a_default)
        {
            const auto value = a_ini.GetLongValue(a_section, a_key, static_cast<long>(a_default));
            return static_cast<std::uint32_t>(std::clamp<long>(
                value, 0, std::numeric_limits<long>::max()));
        }

        [[nodiscard]] float ReadFloat(
            const CSimpleIniA& a_ini,
            const char* a_section,
            const char* a_key,
            float a_default)
        {
            return static_cast<float>(a_ini.GetDoubleValue(a_section, a_key, a_default));
        }

        [[nodiscard]] float ClampMagnitude(float a_value)
        {
            if (!std::isfinite(a_value)) {
                return 0.0f;
            }
            return std::clamp(a_value, 0.0f, 2.0f);
        }

        [[nodiscard]] float ClampMultiplier(float a_value)
        {
            if (!std::isfinite(a_value)) {
                return 0.5f;
            }
            return std::clamp(a_value, 0.0f, 1.0f);
        }

        [[nodiscard]] std::string JoinCommaSeparated(const std::vector<std::string>& a_values)
        {
            std::string result;
            for (const auto& value : a_values) {
                if (!result.empty()) {
                    result += ',';
                }
                result += value;
            }
            return result;
        }
    }

    std::vector<std::string> Settings::ParseCommaSeparated(const char* a_str)
    {
        std::vector<std::string> result;
        if (!a_str || *a_str == '\0') {
            return result;
        }

        std::string_view text(a_str);
        std::size_t start = 0;
        while (start < text.size()) {
            auto comma = text.find(',', start);
            if (comma == std::string_view::npos) {
                comma = text.size();
            }

            auto token = text.substr(start, comma - start);
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
        if (a_raw.find('~') == std::string::npos) {
            a_out.keywordEditorID = a_raw;
        }
    }

    bool Settings::Load()
    {
        CSimpleIniA ini;
        ini.SetUnicode();

        const bool fileExisted = std::filesystem::exists(kIniPath);
        const auto rc = ini.LoadFile(kIniPath);
        if (fileExisted && rc < SI_OK) {
            logger::error("[SimpleStagger] Failed to load INI '{}' (error {})", kIniPath, static_cast<int>(rc));
            return false;
        }

        enableMelee.store(ini.GetBoolValue("General", "EnableMelee", true));
        enableRanged.store(ini.GetBoolValue("General", "EnableRanged", true));
        enableMagic.store(ini.GetBoolValue("General", "EnableMagic", false));
        enableConcentrationMagic.store(
            ini.GetBoolValue("General", "EnableConcentrationMagic", false));
        enableDragonBreath.store(
            ini.GetBoolValue("General", "EnableDragonBreath", false));
        enableOtherCreatureAttacks.store(
            ini.GetBoolValue("General", "EnableOtherCreatureAttacks", false));
        playerCanBeStaggered.store(ini.GetBoolValue("General", "PlayerCanBeStaggered", true));
        npcsCanStagger.store(ini.GetBoolValue("General", "NPCsCanStagger", true));
        staggerCooldownMs.store(ReadUInt(ini, "General", "StaggerCooldownMs", 0));
        magicCooldownMs.store(ReadUInt(ini, "General", "MagicCooldownMs", 200));

        enablePhysicalBlockBehavior.store(ini.GetBoolValue(
            "Stagger", "EnablePhysicalBlockBehavior", true));
        enableWardBlockBehavior.store(ini.GetBoolValue(
            "Stagger", "EnableWardBlockBehavior", true));
        blockedHitBehavior.store(static_cast<int>(std::clamp<long>(
            ini.GetLongValue("Stagger", "BlockedHitBehavior", 0), 0, 1)));
        blockedMultiplier.store(ClampMultiplier(ReadFloat(
            ini, "Stagger", "BlockedMultiplier", 0.5f)));

        // "Maginitude" is the requested v1.3.0 public spelling. The old key is
        // accepted only as a migration fallback.
        if (ini.GetValue("Stagger", "NormalAttackMaginitude", nullptr)) {
            normalAttackMagnitude.store(
                ReadFloat(ini, "Stagger", "NormalAttackMaginitude", kDefaultNormalMagnitude));
        } else {
            normalAttackMagnitude.store(
                ReadFloat(ini, "Stagger", "StaggerMagnitude", kDefaultNormalMagnitude));
        }
        powerAttackMagnitude.store(
            ReadFloat(ini, "Stagger", "PowerAttackMagnitude", kDefaultPowerMagnitude));
        bashMagnitude.store(ReadFloat(ini, "Stagger", "BashMagnitude", kDefaultBashMagnitude));

        constexpr std::array rankKeys{
            "NoviceMagicMagnitude",
            "ApprenticeMagicMagnitude",
            "AdeptMagicMagnitude",
            "ExpertMagicMagnitude",
            "MasterMagicMagnitude"
        };
        const bool hasAnyRankKey = std::ranges::any_of(rankKeys, [&](const char* a_key) {
            return ini.GetValue("Stagger", a_key, nullptr) != nullptr;
        });

        if (hasAnyRankKey) {
            noviceMagicMagnitude.store(ReadFloat(
                ini, "Stagger", rankKeys[0], kDefaultNoviceMagicMagnitude));
            apprenticeMagicMagnitude.store(ReadFloat(
                ini, "Stagger", rankKeys[1], kDefaultApprenticeMagicMagnitude));
            adeptMagicMagnitude.store(ReadFloat(
                ini, "Stagger", rankKeys[2], kDefaultAdeptMagicMagnitude));
            expertMagicMagnitude.store(ReadFloat(
                ini, "Stagger", rankKeys[3], kDefaultExpertMagicMagnitude));
            masterMagicMagnitude.store(ReadFloat(
                ini, "Stagger", rankKeys[4], kDefaultMasterMagicMagnitude));
        } else if (ini.GetValue("Stagger", "MagicMagnitude", nullptr)) {
            const float legacy = ReadFloat(
                ini, "Stagger", "MagicMagnitude", kDefaultAdeptMagicMagnitude);
            noviceMagicMagnitude.store(ClampMagnitude(legacy * 0.4f));
            apprenticeMagicMagnitude.store(ClampMagnitude(legacy * 0.6f));
            adeptMagicMagnitude.store(ClampMagnitude(legacy));
            expertMagicMagnitude.store(ClampMagnitude(legacy * 1.4f));
            masterMagicMagnitude.store(ClampMagnitude(legacy * 2.0f));
            logger::info("[SimpleStagger] Migrated legacy MagicMagnitude to ranked values");
        } else {
            noviceMagicMagnitude.store(kDefaultNoviceMagicMagnitude);
            apprenticeMagicMagnitude.store(kDefaultApprenticeMagicMagnitude);
            adeptMagicMagnitude.store(kDefaultAdeptMagicMagnitude);
            expertMagicMagnitude.store(kDefaultExpertMagicMagnitude);
            masterMagicMagnitude.store(kDefaultMasterMagicMagnitude);
        }
        breathAttackMagnitude.store(ReadFloat(
            ini, "Stagger", "BreathAttackMagnitude",
            kDefaultBreathAttackMagnitude));

        massThreshold.store(ReadFloat(ini, "LargeActor", "MassThreshold", 4.0f));
        largeActorMode.store(static_cast<int>(
            ini.GetLongValue("LargeActor", "LargeActorMode", 0)));
        largeActorMultiplier.store(
            ReadFloat(ini, "LargeActor", "LargeActorMultiplier", 0.5f));

        {
            auto immune = ParseCommaSeparated(ini.GetValue("Overrides", "Immune", ""));
            auto susceptible = ParseCommaSeparated(ini.GetValue("Overrides", "Susceptible", ""));
            std::scoped_lock lock(m_overrideRawMutex);
            m_immuneRaw = std::move(immune);
            m_susceptibleRaw = std::move(susceptible);
        }

        bool success = true;
        if (!fileExisted) {
            success = Save();
            if (success) {
                logger::info("[SimpleStagger] INI not found - wrote v1.4.1 defaults to {}", kIniPath);
            }
        }

        logger::info(
            "[SimpleStagger] Settings loaded: Melee={} Ranged={} Magic={} "
            "Concentration={} DragonBreath={} OtherCreature={} PlayerStagger={} NPCsCanStagger={} "
            "PhysicalBlock={} WardBlock={} BlockedBehavior={} BlockedMult={:.2f} "
            "CooldownMs={} MagicCooldownMs={} NormalMag={:.2f} PowerMag={:.2f} BashMag={:.2f} "
            "MagicMag=[{:.2f},{:.2f},{:.2f},{:.2f},{:.2f}] BreathMag={:.2f} "
            "MassThreshold={:.2f} LargeMode={} LargeMult={:.2f}",
            enableMelee.load(), enableRanged.load(), enableMagic.load(),
            enableConcentrationMagic.load(), enableDragonBreath.load(), enableOtherCreatureAttacks.load(),
            playerCanBeStaggered.load(), npcsCanStagger.load(),
            enablePhysicalBlockBehavior.load(), enableWardBlockBehavior.load(),
            blockedHitBehavior.load(), blockedMultiplier.load(),
            staggerCooldownMs.load(), magicCooldownMs.load(),
            normalAttackMagnitude.load(), powerAttackMagnitude.load(), bashMagnitude.load(),
            noviceMagicMagnitude.load(), apprenticeMagicMagnitude.load(), adeptMagicMagnitude.load(),
            expertMagicMagnitude.load(), masterMagicMagnitude.load(),
            breathAttackMagnitude.load(), massThreshold.load(), largeActorMode.load(),
            largeActorMultiplier.load());
        return success;
    }

    bool Settings::Save()
    {
        CSimpleIniA ini;
        ini.SetUnicode();
        if (std::filesystem::exists(kIniPath)) {
            const auto loadRc = ini.LoadFile(kIniPath);
            if (loadRc < SI_OK) {
                logger::error(
                    "[SimpleStagger] Refusing to overwrite unreadable INI (error {})",
                    static_cast<int>(loadRc));
                return false;
            }
        }

        ini.SetBoolValue("General", "EnableMelee", enableMelee.load(),
            "; Enable stagger on melee weapon hits");
        ini.SetBoolValue("General", "EnableRanged", enableRanged.load(),
            "; Enable stagger on bow and crossbow hits");
        ini.SetBoolValue("General", "EnableMagic", enableMagic.load(),
            "; Normal spell, staff, and scroll stagger (special attacks are independent)");
        ini.SetBoolValue("General", "EnableConcentrationMagic",
            enableConcentrationMagic.load(),
            "; Allow concentration beams under EnableMagic (false = projectiles only)");
        ini.SetBoolValue("General", "EnableDragonBreath", enableDragonBreath.load(),
            "; Dragon fire/frost breath streams and elemental projectiles");
        ini.SetBoolValue("General", "EnableOtherCreatureAttacks",
            enableOtherCreatureAttacks.load(),
            "; Centurion steam, Forgemaster fire, and Afflicted poison attacks");
        ini.SetBoolValue("General", "PlayerCanBeStaggered", playerCanBeStaggered.load(),
            "; Allow SimpleStagger to stagger the player");
        ini.SetBoolValue("General", "NPCsCanStagger", npcsCanStagger.load(),
            "; Allow NPC/creature attacks to cause SimpleStagger (false = player attacks only)");
        ini.SetLongValue("General", "StaggerCooldownMs",
            static_cast<long>(staggerCooldownMs.load()),
            "; Minimum milliseconds between weapon staggers for one target (0 = disabled)");
        ini.SetLongValue("General", "MagicCooldownMs",
            static_cast<long>(magicCooldownMs.load()),
            "; Minimum milliseconds between magic stagger attempts for one target (0 = disabled)");

        ini.SetBoolValue(
            "Stagger", "EnablePhysicalBlockBehavior",
            enablePhysicalBlockBehavior.load(),
            "; Apply BlockedHitBehavior to successful shield or weapon blocks");
        ini.SetBoolValue(
            "Stagger", "EnableWardBlockBehavior",
            enableWardBlockBehavior.load(),
            "; Apply BlockedHitBehavior to magic absorbed by a Ward");
        ini.SetLongValue(
            "Stagger", "BlockedHitBehavior",
            static_cast<long>(blockedHitBehavior.load()),
            "; 0 = no SimpleStagger on guarded hits; 1 = apply BlockedMultiplier");
        ini.SetDoubleValue(
            "Stagger", "BlockedMultiplier", blockedMultiplier.load(),
            "; Guarded-hit magnitude multiplier when BlockedHitBehavior=1");
        ini.SetDoubleValue("Stagger", "NormalAttackMaginitude", normalAttackMagnitude.load(),
            "; Base stagger magnitude for normal attacks");
        ini.SetDoubleValue("Stagger", "PowerAttackMagnitude", powerAttackMagnitude.load(),
            "; Stagger magnitude for power attacks");
        ini.SetDoubleValue("Stagger", "BashMagnitude", bashMagnitude.load(),
            "; Stagger magnitude for bashes");
        ini.SetDoubleValue("Stagger", "NoviceMagicMagnitude", noviceMagicMagnitude.load(),
            "; Destruction magic magnitude for MinimumSkill 0-24");
        ini.SetDoubleValue("Stagger", "ApprenticeMagicMagnitude", apprenticeMagicMagnitude.load(),
            "; Destruction magic magnitude for MinimumSkill 25-49");
        ini.SetDoubleValue("Stagger", "AdeptMagicMagnitude", adeptMagicMagnitude.load(),
            "; Destruction magic magnitude for MinimumSkill 50-74");
        ini.SetDoubleValue("Stagger", "ExpertMagicMagnitude", expertMagicMagnitude.load(),
            "; Destruction magic magnitude for MinimumSkill 75-99");
        ini.SetDoubleValue("Stagger", "MasterMagicMagnitude", masterMagicMagnitude.load(),
            "; Destruction magic magnitude for MinimumSkill 100+");
        ini.SetDoubleValue("Stagger", "BreathAttackMagnitude",
            breathAttackMagnitude.load(),
            "; Shared magnitude for dragon breath and selected creature attacks");
        ini.Delete("Stagger", "StaggerMagnitude");
        ini.Delete("Stagger", "MagicMagnitude");

        ini.SetDoubleValue("LargeActor", "MassThreshold", massThreshold.load(),
            "; Actors whose Mass * scale reaches this value are large");
        ini.SetLongValue("LargeActor", "LargeActorMode", largeActorMode.load(),
            "; 0 = never stagger large actors; 1 = apply LargeActorMultiplier");
        ini.SetDoubleValue("LargeActor", "LargeActorMultiplier", largeActorMultiplier.load(),
            "; Magnitude multiplier for large actors");

        const auto [immune, susceptible] = GetOverrideText();
        ini.SetValue("Overrides", "Immune", immune.c_str(),
            "; Comma-separated Race FormIDs (0xABCD~Plugin.esp) or keyword EditorIDs");
        ini.SetValue("Overrides", "Susceptible", susceptible.c_str(),
            "; Comma-separated overrides that bypass the large-actor check");

        const auto rc = ini.SaveFile(kIniPath);
        if (rc < SI_OK) {
            logger::error("[SimpleStagger] Failed to save INI (error {})", static_cast<int>(rc));
            return false;
        }
        logger::info("[SimpleStagger] Settings saved to {}", kIniPath);
        return true;
    }

    void Settings::SetOverrideText(std::string_view a_immune, std::string_view a_susceptible)
    {
        const std::string immuneText(a_immune);
        const std::string susceptibleText(a_susceptible);
        auto immune = ParseCommaSeparated(immuneText.c_str());
        auto susceptible = ParseCommaSeparated(susceptibleText.c_str());
        std::scoped_lock lock(m_overrideRawMutex);
        m_immuneRaw = std::move(immune);
        m_susceptibleRaw = std::move(susceptible);
    }

    std::pair<std::string, std::string> Settings::GetOverrideText() const
    {
        std::scoped_lock lock(m_overrideRawMutex);
        return { JoinCommaSeparated(m_immuneRaw), JoinCommaSeparated(m_susceptibleRaw) };
    }

    std::shared_ptr<const Settings::OverrideSnapshot> Settings::GetOverrideSnapshot() const noexcept
    {
        return m_overrideSnapshot.load();
    }

    bool Settings::ResolveOverrides()
    {
        std::vector<std::string> immuneRaw;
        std::vector<std::string> susceptibleRaw;
        {
            std::scoped_lock lock(m_overrideRawMutex);
            immuneRaw = m_immuneRaw;
            susceptibleRaw = m_susceptibleRaw;
        }

        bool success = true;
        auto resolveList = [&](const std::vector<std::string>& a_rawList) {
            auto result = std::make_shared<OverrideList>();
            result->reserve(a_rawList.size());
            auto* dataHandler = RE::TESDataHandler::GetSingleton();

            for (const auto& raw : a_rawList) {
                OverrideEntry entry;
                ParseOverrideEntry(raw, entry);
                if (!entry.keywordEditorID.empty()) {
                    result->push_back(std::move(entry));
                    continue;
                }

                const auto tilde = raw.find('~');
                if (tilde == std::string::npos || tilde == 0 || tilde + 1 >= raw.size()) {
                    logger::warn("[SimpleStagger] Invalid override '{}': expected 0xABCD~Plugin.esp", raw);
                    success = false;
                    continue;
                }

                RE::FormID localID = 0;
                try {
                    localID = static_cast<RE::FormID>(
                        std::stoul(raw.substr(0, tilde), nullptr, 16));
                } catch (...) {
                    logger::warn("[SimpleStagger] Invalid override FormID '{}'", raw);
                    success = false;
                    continue;
                }

                if (!dataHandler) {
                    logger::error("[SimpleStagger] TESDataHandler unavailable while resolving '{}'", raw);
                    success = false;
                    continue;
                }

                const auto plugin = raw.substr(tilde + 1);
                auto* form = dataHandler->LookupForm(localID, plugin);
                if (!form) {
                    logger::warn(
                        "[SimpleStagger] Override '{}' not found (plugin '{}', local 0x{:X})",
                        raw, plugin, localID);
                    success = false;
                    continue;
                }

                entry.resolvedFormID = form->GetFormID();
                result->push_back(std::move(entry));
            }
            return result;
        };

        auto snapshot = std::make_shared<OverrideSnapshot>();
        snapshot->immune = std::move(*resolveList(immuneRaw));
        snapshot->susceptible = std::move(*resolveList(susceptibleRaw));
        const auto immuneCount = snapshot->immune.size();
        const auto susceptibleCount = snapshot->susceptible.size();
        m_overrideSnapshot.store(std::move(snapshot));

        logger::info(
            "[SimpleStagger] Overrides resolved: {} immune, {} susceptible ({})",
            immuneCount, susceptibleCount,
            success ? "success" : "with errors");
        return success;
    }
}  // namespace SimpleStagger
