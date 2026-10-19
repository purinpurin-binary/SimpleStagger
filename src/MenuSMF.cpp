// SKSE Menu Framework integration (optional soft dependency).

#include "MenuSMF.h"
#include "Settings.h"

#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 5054)
#include "../extern/SKSEMenuFramework/SKSEMenuFramework.h"
#pragma warning(pop)

namespace SimpleStagger::MenuSMF
{
    namespace
    {
        constexpr std::size_t kOverrideBufferSize = 4096;
        std::array<char, kOverrideBufferSize> g_immuneText{};
        std::array<char, kOverrideBufferSize> g_susceptibleText{};
        bool g_overrideTextInitialized = false;

        std::atomic<std::shared_ptr<const std::string>> g_status{
            std::make_shared<const std::string>("Ready")
        };

        void SetStatus(std::string a_status)
        {
            g_status.store(std::make_shared<const std::string>(std::move(a_status)));
        }

        void CopyToBuffer(std::string_view a_text, std::array<char, kOverrideBufferSize>& a_buffer)
        {
            const auto count = std::min(a_text.size(), a_buffer.size() - 1);
            std::copy_n(a_text.data(), count, a_buffer.data());
            a_buffer[count] = '\0';
            std::fill(a_buffer.begin() + static_cast<std::ptrdiff_t>(count + 1), a_buffer.end(), '\0');
        }

        void SyncOverrideText(Settings& a_settings)
        {
            const auto [immune, susceptible] = a_settings.GetOverrideText();
            CopyToBuffer(immune, g_immuneText);
            CopyToBuffer(susceptible, g_susceptibleText);
            g_overrideTextInitialized = true;
        }

        void QueueOverrideResolution(bool a_loadSucceeded = true)
        {
            SKSE::GetTaskInterface()->AddTask([a_loadSucceeded]() {
                const bool resolved = Settings::GetSingleton()->ResolveOverrides();
                if (a_loadSucceeded && resolved) {
                    SetStatus("Overrides applied");
                } else if (!a_loadSucceeded) {
                    SetStatus("INI reload failed; existing overrides re-resolved");
                } else {
                    SetStatus("Overrides applied with errors (see log)");
                }
            });
        }

        void AtomicCheckbox(const char* a_label, std::atomic_bool& a_value)
        {
            bool value = a_value.load();
            if (ImGui::Checkbox(a_label, &value)) {
                a_value.store(value);
            }
        }

        void AtomicSliderFloat(
            const char* a_label,
            std::atomic<float>& a_value,
            float a_min,
            float a_max,
            const char* a_format = "%.2f")
        {
            float value = a_value.load();
            if (ImGui::SliderFloat(a_label, &value, a_min, a_max, a_format)) {
                a_value.store(value);
            }
        }

        void AtomicSliderInt(
            const char* a_label,
            std::atomic<std::uint32_t>& a_value,
            int a_min,
            int a_max)
        {
            int value = static_cast<int>(std::min<std::uint32_t>(
                a_value.load(), static_cast<std::uint32_t>(a_max)));
            if (ImGui::SliderInt(a_label, &value, a_min, a_max)) {
                a_value.store(static_cast<std::uint32_t>(value));
            }
        }

        void AtomicCombo(
            const char* a_label,
            std::atomic<int>& a_value,
            const char* const* a_items,
            int a_itemCount)
        {
            int value = std::clamp(a_value.load(), 0, a_itemCount - 1);
            if (ImGui::Combo(a_label, &value, a_items, a_itemCount)) {
                a_value.store(value);
            }
        }

        void __stdcall RenderPage()
        {
            auto& settings = *Settings::GetSingleton();
            if (!g_overrideTextInitialized) {
                SyncOverrideText(settings);
            }

            ImGui::SeparatorText("General");
            AtomicCheckbox("Enable Melee", settings.enableMelee);
            ImGui::SameLine();
            AtomicCheckbox("Enable Ranged", settings.enableRanged);
            ImGui::SameLine();
            AtomicCheckbox("Enable Magic", settings.enableMagic);
            ImGui::SameLine();
            AtomicCheckbox("Player Can Be Staggered", settings.playerCanBeStaggered);
            ImGui::SameLine();
            AtomicCheckbox("NPCs Can Stagger", settings.npcsCanStagger);
            AtomicCheckbox(
                "Concentration Magic", settings.enableConcentrationMagic);
            ImGui::SameLine();
            AtomicCheckbox("Dragon Breath", settings.enableDragonBreath);
            ImGui::SameLine();
            AtomicCheckbox(
                "Other Creature Attacks", settings.enableOtherCreatureAttacks);
            AtomicSliderInt(
                "Weapon Cooldown (ms)  [0 = off]", settings.staggerCooldownMs, 0, 2000);
            AtomicSliderInt(
                "Magic Cooldown (ms)  [0 = off]", settings.magicCooldownMs, 0, 3000);

            ImGui::SeparatorText("Stagger Magnitudes");
            AtomicSliderFloat(
                "Normal Attack Maginitude", settings.normalAttackMagnitude, 0.0f, 2.0f);
            AtomicSliderFloat(
                "Power Attack Magnitude", settings.powerAttackMagnitude, 0.0f, 2.0f);
            AtomicSliderFloat("Bash Magnitude", settings.bashMagnitude, 0.0f, 2.0f);
            AtomicSliderFloat(
                "Ranged Attack Magnitude", settings.rangedAttackMagnitude, 0.0f, 2.0f);

            ImGui::SeparatorText("Magic Magnitudes (MinimumSkill)");
            AtomicSliderFloat(
                "Novice (0-24)", settings.noviceMagicMagnitude, 0.0f, 2.0f);
            AtomicSliderFloat(
                "Apprentice (25-49)", settings.apprenticeMagicMagnitude, 0.0f, 2.0f);
            AtomicSliderFloat(
                "Adept (50-74)", settings.adeptMagicMagnitude, 0.0f, 2.0f);
            AtomicSliderFloat(
                "Expert (75-99)", settings.expertMagicMagnitude, 0.0f, 2.0f);
            AtomicSliderFloat(
                "Master (100+)", settings.masterMagicMagnitude, 0.0f, 2.0f);
            AtomicSliderFloat(
                "Breath / Creature Attack",
                settings.breathAttackMagnitude, 0.0f, 2.0f);

            ImGui::SeparatorText("Block");
            AtomicCheckbox(
                "Physical Shield / Weapon Block",
                settings.enablePhysicalBlockBehavior);
            ImGui::SameLine();
            AtomicCheckbox(
                "Magic Ward",
                settings.enableWardBlockBehavior);
            static const char* blockedItems[]{ "No Stagger", "Reduced" };
            AtomicCombo(
                "Blocked Hit Behavior", settings.blockedHitBehavior,
                blockedItems, static_cast<int>(std::size(blockedItems)));
            ImGui::BeginDisabled(settings.blockedHitBehavior.load() != 1);
            AtomicSliderFloat(
                "Blocked Multiplier", settings.blockedMultiplier, 0.0f, 1.0f);
            ImGui::EndDisabled();

            ImGui::SeparatorText("Large Actor");
            AtomicSliderFloat("Mass Threshold", settings.massThreshold, 0.0f, 20.0f);
            static const char* largeItems[]{ "Immune", "Reduced" };
            AtomicCombo(
                "Large Actor Mode", settings.largeActorMode, largeItems,
                static_cast<int>(std::size(largeItems)));
            AtomicSliderFloat(
                "Large Actor Multiplier", settings.largeActorMultiplier, 0.0f, 1.0f);

            ImGui::SeparatorText("Overrides");
            ImGui::InputText(
                "Immune", g_immuneText.data(), g_immuneText.size());
            ImGui::InputText(
                "Susceptible", g_susceptibleText.data(), g_susceptibleText.size());
            ImGui::TextDisabled(
                "Comma-separated: 0xABCD~Plugin.esp or keyword EditorID");
            if (ImGui::Button("Apply Overrides")) {
                settings.SetOverrideText(g_immuneText.data(), g_susceptibleText.data());
                SetStatus("Applying overrides...");
                QueueOverrideResolution();
            }

            ImGui::SeparatorText("Actions");
            if (ImGui::Button("Save to INI")) {
                SetStatus(settings.Save() ? "INI saved" : "INI save failed (see log)");
            }
            ImGui::SameLine();
            if (ImGui::Button("Reload from INI")) {
                const bool loaded = settings.Load();
                SyncOverrideText(settings);
                SetStatus(loaded ? "INI reloaded; resolving overrides..." :
                    "INI reload failed; re-resolving existing overrides...");
                QueueOverrideResolution(loaded);
            }

            const auto status = g_status.load();
            ImGui::Text("Status: %s", status->c_str());
        }
    }

    void Register()
    {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::info("[SimpleStagger] SMF not found - in-game menu disabled");
            return;
        }

        SKSEMenuFramework::SetSection("SimpleStagger");
        SKSEMenuFramework::AddSectionItem("Settings", RenderPage);
        logger::info("[SimpleStagger] Registered settings page with SKSE Menu Framework");
    }
}  // namespace SimpleStagger::MenuSMF
