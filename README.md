# SimpleStagger

SKSE plugin that makes weapon hits stagger the target — improving the feel of impact in melee and ranged combat. Designed to pair with **(SGC) Scriptfree Behavior Staggerlock - SBS** for chainable stagger combos.

One DLL supports **Skyrim SE (1.5.97) and AE (1.6.x)** via CommonLibSSE-NG + Address Library. No ESP, no Papyrus scripts, no per-mod patches.

## Features

- Melee and bow/crossbow hits apply a directional stagger to the hit actor (any attacker → any target, PC and NPC alike).
- **Large-actor resistance**: actors whose physics mass (`Mass` actor value × scale) exceeds a configurable threshold (giants, dragons, mammoths…) are immune — or optionally just resistant. Works automatically for mod-added creatures with no patches; per-race/keyword overrides available in the INI.
- Blocked hits cause no stagger by default (configurable to reduced stagger).
- Separate magnitudes for normal attacks, power attacks, and bashes.
- Optional per-target cooldown (default off — full combo potential with SBS).
- **In-game configuration** via [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) (optional soft dependency) with save/reload to `SimpleStagger.ini`.

## Requirements

- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

Optional:
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) — in-game settings menu
- [(SGC) Scriptfree Behavior Staggerlock - SBS](https://www.nexusmods.com/skyrimspecialedition/mods/33481) + [Nemesis](https://www.nexusmods.com/skyrimspecialedition/mods/60033) — allows staggering an already-staggering actor (stagger combos). Run Nemesis after installing SBS.

## Installation

Install with MO2/Vortex. The archive contains `SKSE/Plugins/SimpleStagger.dll` and `SimpleStagger.ini`.

For stagger combos: install SBS, run Nemesis, done. No compatibility patches needed for creature mods — resistance is computed at runtime from each race's mass.

## Configuration

Edit `Data/SKSE/Plugins/SimpleStagger.ini` (created with defaults on first run), or use the SKSE Menu Framework page ("SimpleStagger" → "Settings") in game. See the comments in the INI for every option. `[Overrides]` entries (race FormIDs `0x123~Plugin.esp` or keyword EditorIDs like `ActorTypeDragon`) are INI-only.

## Verification checklist (in-game)

1. Normal attack on a humanoid → target flinches in the direction of the blow.
2. With SBS installed: consecutive fast attacks chain staggers (combo).
3. Giants / dragons / mammoths do not flinch from normal hits (default `MassThreshold=4.0`).
4. Hits on a blocking target cause no stagger (default).
5. Bow/crossbow hits stagger; magic hits do not.
6. SMF menu changes apply immediately; "Save to INI" persists them.

Log: `Documents/My Games/Skyrim Special Edition/SKSE/SimpleStagger.log`

---

## 日本語

攻撃が当たったActorにスタッガー(よろめき)を発生させ、ヒット感を向上させるSKSEプラグインです。**SBS**と併用すると、よろめき中の敵へ連続攻撃でスタッガーを重ねられます(コンボ)。

- SE(1.5.97)/AE(1.6.x)両対応・1DLL(ESP不要・スクリプト不要)
- 近接+弓/クロスボウ対応(魔法は対象外)。ブロック中の被弾は既定でスタッガーなし
- **大型敵耐性**: レースの質量(Mass AV)×スケールがしきい値以上のアクター(巨人・ドラゴン等)は自動で免疫。MOD追加クリーチャーも**個別パッチ完全不要**。INIの`[Overrides]`でレース/キーワード単位の強制指定も可能
- 設定はINIまたは[SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)(任意導入)のゲーム内メニューで変更可能

**必須**: SKSE64、Address Library
**推奨**: SBS+Nemesis(スタッガーコンボ用。SBS導入後にNemesisを実行してください)

## Building from source

```
cmake --preset x64-release
cmake --build --preset x64-release
```

Requires MSVC (VS2022+; verified on VS2026 / MSVC 14.51), CMake, vcpkg (`VCPKG_ROOT`), and the `lib/commonlibsse-ng` submodule (alandtse/CommonLibVR branch `ng`).

## Credits

- CommonLibSSE-NG (alandtse and contributors)
- Stagger technique attested in powerof3's Papyrus Extender, UselessFenixUtils, and valhallaCombat
- distar66 — (SGC) Stagger on hit / SBS, the design inspiration
- Thiago — SKSE Menu Framework
