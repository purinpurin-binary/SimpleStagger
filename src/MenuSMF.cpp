// MenuSMF.cpp — SKSE Menu Framework (soft dependency) integration for SimpleStagger.
//
// SMF API origin:
//   Repository : Thiago099/SKSE-Menu-Framework-2-Example
//   File       : include/SKSEMenuFramework.h
//   Commit SHA : cd1f277200b7f7a4740ab039115a6e629014a102 (2024-06-29)
//   Saved at   : extern/SKSEMenuFramework/SKSEMenuFramework.h
//
// SMF uses GetProcAddress-based dynamic linking; every ImGui call goes through
// exported symbols in SKSEMenuFramework.dll. The header provides thin inline
// wrappers. No .lib link is needed; the DLL presence check is runtime-only.

#include "MenuSMF.h"
#include "Settings.h"

// SMF header must come before any #include that already pulled in <windows.h>
// via SKSE/CommonLib, because it re-declares GetModuleHandle with the same
// global static.  Including it early (before pch pull-in here) avoids ODR
// issues — the static is defined once in this TU only.
#pragma warning(push)
#pragma warning(disable: 4244)   // possible loss of data (SMF uses int/float casts freely)
#pragma warning(disable: 5054)   // deprecated enum operator (ImGui enums)
#include "../extern/SKSEMenuFramework/SKSEMenuFramework.h"
#pragma warning(pop)

namespace SimpleStagger::MenuSMF
{
    // -----------------------------------------------------------------------
    // Render callback — called by SMF on its render thread each frame the
    // menu is open.  Writes directly to Settings singleton members (all
    // primitive types; reads in HitHandler are on the game thread but the
    // values are written atomically by the CPU for native word-sized types,
    // which is sufficient for this use-case).
    // -----------------------------------------------------------------------

    static void __stdcall RenderPage()
    {
        auto* s = SimpleStagger::Settings::GetSingleton();

        // ── General ──────────────────────────────────────────────────────
        ImGui::SeparatorText("General");

        ImGui::Checkbox("Enable Melee",             &s->enableMelee);
        ImGui::SameLine();
        ImGui::Checkbox("Enable Ranged",            &s->enableRanged);
        ImGui::SameLine();
        ImGui::Checkbox("Player Can Be Staggered",  &s->playerCanBeStaggered);

        {
            int cooldown = static_cast<int>(s->staggerCooldownMs);
            if (ImGui::SliderInt("Stagger Cooldown (ms)  [0 = off]", &cooldown, 0, 2000)) {
                s->staggerCooldownMs = static_cast<std::uint32_t>(cooldown);
            }
        }

        // ── Stagger magnitudes ────────────────────────────────────────────
        ImGui::SeparatorText("Stagger Magnitudes");

        ImGui::SliderFloat("Stagger Magnitude",        &s->staggerMagnitude,     0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Power Attack Magnitude",   &s->powerAttackMagnitude, 0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Bash Magnitude",           &s->bashMagnitude,        0.0f, 2.0f, "%.2f");

        // ── Blocked hit ───────────────────────────────────────────────────
        ImGui::SeparatorText("Blocked Hit");

        {
            static const char* blockedItems[] = { "No Stagger", "Reduced (apply multiplier)" };
            ImGui::Combo("Blocked Hit Behavior", &s->blockedHitBehavior, blockedItems, 2);
        }
        ImGui::SliderFloat("Blocked Multiplier", &s->blockedMultiplier, 0.0f, 1.0f, "%.2f");

        // ── Large actor ───────────────────────────────────────────────────
        ImGui::SeparatorText("Large Actor");

        ImGui::SliderFloat("Mass Threshold", &s->massThreshold, 0.0f, 20.0f, "%.2f");

        {
            static const char* largeItems[] = { "Immune (never stagger)", "Reduced (apply multiplier)" };
            ImGui::Combo("Large Actor Mode", &s->largeActorMode, largeItems, 2);
        }
        ImGui::SliderFloat("Large Actor Multiplier", &s->largeActorMultiplier, 0.0f, 1.0f, "%.2f");

        // ── Actions ───────────────────────────────────────────────────────
        ImGui::SeparatorText("Actions");

        if (ImGui::Button("Save to INI")) {
            s->Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload from INI")) {
            s->Load();
        }

        // ── Note about Overrides ──────────────────────────────────────────
        ImGui::Spacing();
        ImGui::TextDisabled("[Overrides] section (Immune / Susceptible lists) must be edited");
        ImGui::TextDisabled("directly in Data/SKSE/Plugins/SimpleStagger.ini — not editable here.");
    }

    // -----------------------------------------------------------------------

    void Register()
    {
        // Runtime check: is SKSEMenuFramework.dll present?
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::info("[SimpleStagger] SMF not found — in-game menu disabled, INI only");
            return;
        }

        SKSEMenuFramework::SetSection("SimpleStagger");
        SKSEMenuFramework::AddSectionItem("Settings", RenderPage);

        logger::info("[SimpleStagger] Registered settings page with SKSE Menu Framework");
    }

}  // namespace SimpleStagger::MenuSMF
