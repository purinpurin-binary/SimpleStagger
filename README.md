# SimpleStagger 1.4.2

SimpleStagger is an SKSE plugin that adds directional stagger to eligible weapon and
Destruction-magic hits. It supports Skyrim SE 1.5.97 and AE 1.6.x from one
CommonLibSSE-NG DLL. It has no ESP or Papyrus scripts.

## Features

- Melee weapons, bows and crossbows have independent enable switches. Classification
  comes from the weapon Form, so Bound Bow and mod-added bows obey `EnableRanged`;
  Staff and unclassified weapons do not enter the melee path. Eligible staff
  enchantments enter the magic path instead.
- Magic stagger is limited to hostile or detrimental Destruction effects emitted
  as an aimed projectile or concentration beam by a normal `Spell`, staff, or
  `Scroll`. Healing, support magic, other schools, abilities, powers, voice powers,
  ordinary shouts, self/touch/target-location magic and non-Destruction scrolls
  are ignored, apart from the structurally selected special attacks below.
- Concentration beams on the normal magic path (Flames, Frostbite, Sparks and
  concentration staffs/scrolls) are controlled by `EnableConcentrationMagic`
  (default `false`); aimed projectiles only need `EnableMagic`. Dragon and creature
  breath streams keep their own switches.
- Damage spells associated with a `Cloak` magic-effect archetype are discovered
  from the winning load-order records and excluded. This applies to vanilla and
  mod-added cloaks without relying on EditorIDs or hard-coded FormIDs.
- Dragon fire/frost breath streams and elemental projectiles can stagger under
  the independent `EnableDragonBreath` switch. Dwarven Centurion steam,
  Forgemaster fire breath/fireballs and Afflicted poison spray use
  `EnableOtherCreatureAttacks`. Both are independent from `EnableMagic` and share
  `BreathAttackMagnitude` (default 0.25). Melee and ranged are enabled by default;
  all three magic-category switches are disabled by default.
- Special attacks are recognized structurally from winning Voice spell records:
  aimed delivery, supported cast type, hostile/detrimental Health damage and a
  projectile. ActorType keywords distinguish dragons and Dwarven fireballs;
  EditorID allowlists are not used. Any source that already contains an
  unconditional Stagger magic effect, including Drain Vitality, is excluded.
  Conditional Stagger effects such as the dual-cast Impact perk do not exclude
  the entire spell.
- Magic magnitude is selected from the eligible effect's `MinimumSkill`:

| Rank | MinimumSkill | Default magnitude |
|---|---:|---:|
| Novice | 0–24 | 0.10 |
| Apprentice | 25–49 | 0.15 |
| Adept | 50–74 | 0.25 |
| Expert | 75–99 | 0.35 |
| Master | 100+ | 0.50 |

- Large actors can be immune or resistant based on `Mass * scale`, with Race/keyword
  overrides.
- Physical shield/weapon blocks and magic absorbed by an active Ward can apply
  `BlockedHitBehavior` independently. The default behavior exempts both from
  SimpleStagger; reduced magnitude is available through `BlockedMultiplier`.
  A broken Ward is not treated as a successful guard.
- Weapon and magic cooldowns are independent. Eligible concentration beams can
  attempt stagger repeatedly while continuously hitting, at the configured
  `MagicCooldownMs` interval. Applied staggers and final resistance/perk rejections
  start the interval; transient actor-state failures remain immediately retryable.
- Voice attack profiles and Cloak exclusions are built once at data load. Magic
  cooldown and continuous-hit tracking share one lock, and expired state is pruned
  lazily instead of growing for the entire session.
- `NPCsCanStagger=false` limits SimpleStagger to the player's attacks. NPC,
  follower, summon, dragon and creature attacks are then ignored.
- Player immunity is checked again immediately before SimpleStagger sends its
  animation event. It does not suppress vanilla or other-mod stagger.
- Dead/dying, bleedout, essential-down, unconscious, kill-move and ragdoll states
  are guarded to avoid interrupted scenes and post-death idle problems.
- Sleeping/waking actors and actors that still occupy furniture are ignored. This
  protects ambush draugr while their alcove/throne wake-up animation owns the graph.
- Restrained actors are ignored. The player is also ignored while scripted scenes
  have movement, looking, or fighting controls disabled, preventing `staggerStart`
  from taking ownership of the animation graph during sequences such as Alduin's
  arrival at Helgen.
- Optional [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)
  integration exposes every runtime setting and both override lists.

Already-staggering actors are intentionally not rejected, preserving chained stagger
behavior with [(SGC) Scriptfree Behavior Staggerlock (SBS)](https://www.nexusmods.com/skyrimspecialedition/mods/33481).

## Requirements and installation

- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- SKSE Menu Framework is optional.

Install the archive with MO2/Vortex. It contains:

```text
SKSE/Plugins/SimpleStagger.dll
SKSE/Plugins/SimpleStagger.ini
```

## Configuration and migration

Edit `Data/SKSE/Plugins/SimpleStagger.ini`, or use the SimpleStagger page in SMF.
SMF Save, Reload and Apply Overrides operations show a short status; details go to
`Documents/My Games/Skyrim Special Edition/SKSE/SimpleStagger.log`.

The default INI enables only `EnableMelee` and `EnableRanged`. `EnableMagic`,
`EnableConcentrationMagic`, `EnableDragonBreath`, and `EnableOtherCreatureAttacks`
must be enabled separately.

`EnablePhysicalBlockBehavior` and `EnableWardBlockBehavior` independently decide
whether a successful guard uses `BlockedHitBehavior`. Behavior `0` suppresses
SimpleStagger and behavior `1` applies `BlockedMultiplier`. Turning a category
switch off treats that guarded hit like an ordinary hit.

The v1.3.0 normal-attack key is intentionally spelled
`NormalAttackMaginitude`. If it is absent, the old `StaggerMagnitude` value is read.

Bow and crossbow hits use `RangedAttackMagnitude`; `NormalAttackMaginitude` and
`PowerAttackMagnitude` apply to melee only. Bow bashes still use `BashMagnitude`.
If `RangedAttackMagnitude` is absent, the `NormalAttackMaginitude` value is used,
so existing INIs keep their previous ranged behavior.

If no ranked magic key exists, old `MagicMagnitude` is treated as the Adept value
and migrated as follows (clamped to 0.0–2.0): Novice ×0.4, Apprentice ×0.6,
Adept ×1.0, Expert ×1.4, Master ×2.0. If any ranked key exists, the old key is
ignored and missing ranks use the defaults in the table. Saving removes both old
keys and writes only the v1.3.0 keys.

Overrides accept comma-separated Race FormIDs (`0xABCD~Plugin.esp`) or keyword
EditorIDs. In SMF, edit `Immune` / `Susceptible`, then select **Apply Overrides**.
Resolution runs on the game thread; the hit path reads immutable snapshots without
taking a mutex.

## Compatibility

SimpleStagger still writes the animation graph variables and `staggerStart` event
directly. Incoming-stagger perk support is detected at data load for vanilla
Skyrim, **Master of One**, **Vokrii**, and **Ordinator**. Known perk forms are
validated by their winning source plugin and by their `ModIncomingStagger` entry
before use. This avoids Skyrim's sparse runtime EditorID map while ensuring that
the Tower of Strength FormID reused by Vokrii and Ordinator is not mistaken for
the vanilla perk.

Skyrim evaluates each supported perk's own conditions. The resulting incoming
stagger multiplier is treated as the chance that a SimpleStagger stagger succeeds:
`1.0` never blocks, `0.5` blocks half of attempts, and `0.0` always blocks. This
preserves vanilla Tower of Strength while also supporting Master of One's armor
count scaling, Vokrii's conditional resistance perks, and Ordinator's temporary
immunity procs. `ModTargetStagger` remains outside this compatibility path.

[Long Awaited - Attack Interrupts](https://www.nexusmods.com/skyrimspecialedition/mods/169495)
does not have a DLL/file conflict, but its attack interruption and recoil behavior
overlaps functionally. Configure only one mod to own the animation reaction for each
of Magic, Player and Ranged categories. When comparing configurations, test
Long Awaited alone, SimpleStagger alone, both at defaults, and Long Awaited Recoil
mode, watching for double reactions and kill-move interruption.

## In-game verification

1. With `EnableRanged=false` and `EnableMelee=true`, test Bow, Crossbow, Bound Bow
   and a mod-added bow; none should stagger, while a melee weapon should.
2. Toggle `PlayerCanBeStaggered=false` and test melee, ranged and Destruction magic.
3. Set each normal/power/bash/ranked-magic magnitude to zero, plus the large-actor
   multiplier where applicable; no `staggerStart` should be produced.
4. Successfully block melee and arrow/bolt hits with an equipped shield or weapon,
   then absorb eligible magic with a Ward. At the default block settings none
   should trigger `staggerStart`. Test each block-category switch independently,
   test reduced mode, then break the Ward and confirm the penetrating hit is not
   mistaken for a successful guard.
5. Compare aimed Destruction spells, staffs and scrolls with cloaks, runes,
   healing, support, summon, illusion, shout and non-Destruction scroll sources.
   Only the first group should trigger SimpleStagger.
6. Set `EnableDragonBreath=true`, then test dragon fire/frost streams and fire/frost
   balls. Disable it and confirm only SimpleStagger's reaction disappears.
7. Test Dwarven Centurion steam, Forgemaster fire breath/fireballs and Afflicted
   poison spray with `EnableOtherCreatureAttacks` on and off.
8. Verify Drain Vitality does not receive a second stagger, and that cloak, wall,
   rune, trap, hazard, weapon enchantment and explosive-spider attacks remain out.
9. Complete the Helgen opening with player staggering enabled.
10. Repeat lethal hits, kill moves, ragdoll and bleedout cases; verify no post-death
   standing/floating and no kill-move interruption.
11. With SBS installed, verify normal chained stagger remains possible.
12. Verify SMF values match INI and survive Save, Reload, Apply Overrides and restart.
13. Verify a legacy INI migrates and old keys disappear after Save.
14. Test the detected perk environment:
    - Vanilla Tower of Strength: roughly half of eligible staggers are ignored.
    - Master of One: each rank and Heavy Armor count changes the complete-block rate.
    - Vokrii: Hunter's Focus, Stoneheart, and Berserker work only in their stated states.
    - Ordinator: Focus on the Prey, Bedrock, and Witchmaster immunity work only while active.
    In every case, successful staggers retain their full configured magnitude.
15. Attack ambush draugr in alcoves, on thrones and in slouched/lying poses. Their
    wake-up animation must complete without a stuck pose, floating, or crash.
16. Hold a concentration flame on one target with `MagicCooldownMs=60` and `200`;
    confirm stagger attempts continue at the configured interval without releasing
    the cast.
17. With `EnableMagic=true`, set `EnableConcentrationMagic=false`; Flames/Frostbite/Sparks must not
    stagger while Firebolt/Ice Spike still do, and dragon breath still follows
    `EnableDragonBreath`.
18. Set `NPCsCanStagger=false`; the player's attacks still stagger NPCs, while
    enemy, follower, summon and dragon attacks no longer trigger SimpleStagger.

## Building

```powershell
cmake --preset x64-release
cmake --build --preset x64-release
```

Requires MSVC, CMake, vcpkg (`VCPKG_ROOT`) and the
`lib/commonlibsse-ng` submodule.

## License

SimpleStagger is released under the [MIT License](LICENSE).

Third-party components:

- [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR) v4.38.0 (git submodule) — MIT License
- `extern/SKSEMenuFramework/SKSEMenuFramework.h` from
  [SKSE-Menu-Framework-2-Example](https://github.com/Thiago099/SKSE-Menu-Framework-2-Example) —
  MIT License, see [extern/SKSEMenuFramework/LICENSE](extern/SKSEMenuFramework/LICENSE)
