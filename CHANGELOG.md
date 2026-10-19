# SimpleStagger 更新履歴

## 1.4.2

### 近接と遠距離のよろめき強度を分離

- `RangedAttackMagnitude`（既定`0.25`）を追加しました。弓・クロスボウの命中は
  この値を使います。
- `NormalAttackMaginitude`と`PowerAttackMagnitude`は近接専用になりました。
- 弓での殴り（バッシュ）は近接動作として扱い、従来どおり`BashMagnitude`を使います。
  パワーバッシュはこれまでどおり`PowerAttackMagnitude`です。
- 既存のINIに`RangedAttackMagnitude`がない場合は、`NormalAttackMaginitude`の値を
  引き継ぎます。更新しても遠距離のよろめき強度は変わりません。
- SMFのStagger Magnitudesに「Ranged Attack Magnitude」スライダーを追加しました。

### Nexus公開版1.3.1からの変更まとめ

1.3.1から更新する場合、1.3.2・1.4.1・1.4.2の変更がすべて含まれます。
各項目の詳細は下の各バージョンを参照してください。

**新しい設定**

- `RangedAttackMagnitude`: 弓・クロスボウのよろめき強度を近接から分離（1.4.2）
- `NPCsCanStagger`: オフでプレイヤーの攻撃だけがよろめきを発生（1.4.1）
- `EnableConcentrationMagic`: 火炎・凍結・雷撃などの集中放射魔法を個別に切替、
  既定Off（1.4.1）
- `EnableDragonBreath`: ドラゴンの火炎／冷気ブレスと火球／氷弾、既定Off（1.4.1）
- `EnableOtherCreatureAttacks`: センチュリオンの蒸気、フォージマスターの火炎攻撃、
  Afflictedの毒噴射、既定Off（1.4.1）
- `BreathAttackMagnitude`: 上記2カテゴリ共通の強度、既定`0.25`（1.4.1）
- `BlockedHitBehavior`／`BlockedMultiplier`を復元し、
  `EnablePhysicalBlockBehavior`／`EnableWardBlockBehavior`で物理ガードと魔力盾を
  個別に切替（1.4.1）
- 上記すべてをSMFに追加

**挙動の変更**

- 初期設定は近接・弓のみOnになり、魔法系の設定はすべてOffになりました。
  既存のINIの値はそのまま引き継がれます（1.4.1）。
- 盾・武器でのガード成功、魔力盾で吸収した魔法は、初期設定でよろめかなく
  なりました。破られた魔力盾はガード成功として扱いません（1.4.1）。
- 魔法のよろめきを、通常の魔法・杖・巻物から撃つ破壊魔法の弾と集中放射に
  限定しました。杖に対応し、自己・接触・地点対象・ルーン魔法などは対象外です（1.3.2）。
- 集中放射を当て続けている間、`MagicCooldownMs`ごとによろめき判定を
  繰り返すようにしました（1.4.1）。
- 生命力吸収など、元々よろめき効果を持つ魔法とは重複しません（1.4.1）。

**不具合修正**

- 炎の精霊のマントなどに触れている間、何度もよろめく問題を修正しました。
  バニラと他MODのマント魔法を自動で対象外にします（1.3.2）。
- 雷撃などで最初の判定が失敗すると、その放射全体でよろめかなくなる問題を
  修正しました（1.4.1）。
- 二連の唱え（Impactパーク）の条件付き効果のせいで、サンダーボルトなどが誤って
  対象外になっていた問題を修正しました（1.4.1）。
- 物理ガードの判定が攻撃武器の一致に依存していて、ガード成功を見逃す場合が
  あった問題を修正しました（1.4.1）。

**パフォーマンス**

- 魔法の分類をゲーム読み込み時に1回だけ行うようにしました。長時間プレイでも
  内部データが増え続けないようにしました（1.4.1）。

## 1.4.1

### NPCによるよろめき付与のON/OFF

- `NPCsCanStagger`（既定`true`）を追加しました。`false`にすると、
  プレイヤーの攻撃だけがSimpleStaggerのよろめきを発生させます。
  敵NPC、フォロワー、召喚、ドラゴン、クリーチャーの攻撃はすべて対象外になります。
- バニラや他MODのよろめきには影響しません。
- SMFのGeneralに「NPCs Can Stagger」チェックボックスを追加しました。

### 集中放射魔法のON/OFF

- `EnableConcentrationMagic`（既定`false`）を追加しました。`EnableMagic`経路の
  集中放射魔法（Flames、Frostbite、Sparks、放射型の杖・巻物）だけを切り替えます。
  投射型の破壊魔法は`EnableMagic`だけで対象になります。
- 魔法系のよろめき設定（`EnableMagic`、`EnableConcentrationMagic`、
  `EnableDragonBreath`、`EnableOtherCreatureAttacks`）はすべて既定Offです。
- ドラゴン／クリーチャーのブレスは従来どおり`EnableDragonBreath`／
  `EnableOtherCreatureAttacks`で制御します。マント魔法は常に対象外です。
- SMFのGeneralに「Concentration Magic」チェックボックスを追加しました。

### ガード設定・連続魔法・初期設定の修正

- 物理ガード判定から攻撃武器Formの完全一致条件を除去し、盾・武器による
  成功ガードを`TESHitEvent`、一致する`HitData`、防御状態から段階的に判定します。
- `BlockedHitBehavior`と`BlockedMultiplier`を復元しました。
  `EnablePhysicalBlockBehavior`と`EnableWardBlockBehavior`により、物理ガードと
  魔法Wardへの適用を個別に切り替えられます。
- 集中放射魔法の「1回の照射につき1回」制限を廃止し、照射を続けている間も
  `MagicCooldownMs`ごとにstagger判定を行います。
- `EnableDragonBreath`と`EnableOtherCreatureAttacks`を`EnableMagic`から独立させました。
- 初期設定は近接・弓のみOnとし、通常魔法・Dragon Breath・Other Creature Attackは
  Offに変更しました。

### 装備盾・魔力盾のブロック成功時免除

- `TESHitEvent::kHitBlocked`だけに依存せず、同じ攻撃の`HitData::kBlocked`、
  `percentBlocked`、`kBlockCandidate`と防御状態、命中時の`WardState::kAbsorb`も
  使ってブロック成功を判定します。
- 装備盾／武器ガードと魔力盾には、それぞれ個別の適用スイッチがあります。
  既定の`BlockedHitBehavior=0`ではSimpleStaggerを免除し、`1`では
  `BlockedMultiplier`を適用します。`WardState::kBreak`は成功扱いしません。
- `BlockedHitBehavior`と`BlockedMultiplier`を復元し、SMFとINI保存にも戻しました。

### 雷撃の再試行と防御済み魔法
- 雷撃などの継続放射は、イベント受信時には処理を予約するだけに変更しました。
  最初のメインスレッド処理が一時的なActor状態で適用できなかった場合、同じ放射中の
  後続ヒットが再試行できるため、最初の失敗で放射全体が無効になる問題を修正しました。
- 実際によろめきが適用された時と、ResistanceまたはPerkによる最終的な拒否時に、
  対象単位の魔法クールダウンを開始します。照射を継続したままでも、設定時間の
  経過後に次の判定を行います。
- `TESHitEvent::kHitBlocked` が付いた魔法を常に除外する基礎対応を追加しました。
  装備盾・魔力盾の実機再報告を受け、上記の複合判定へ拡張しています。
- Lightning Boltなどに含まれる、二連の唱え／Impact Perkを条件とするStagger効果を
  「常にネイティブStaggerを持つ魔法」と誤判定して魔法全体を除外していた問題を
  修正しました。重複防止で魔法全体を除外するのは、条件のないStagger効果だけです。

### ドラゴンブレスと選定クリーチャー攻撃

- ドラゴンの火炎／冷気の集中ブレスと火球／氷弾をよろめき対象へ追加しました。
  `EnableDragonBreath`で独立してOn/Offできます。
- ドワーフ・センチュリオンの蒸気、フォージマスターの火炎ブレス／火球、
  Afflictedの毒噴射を追加しました。これらは
  `EnableOtherCreatureAttacks`で独立してOn/Offできます。
- 両設定は`EnableMagic`から独立しており、共通の
  `BreathAttackMagnitude=0.25`を使用します。SMFにも2つのチェックボックスと
  共通Magnitudeスライダーを追加しました。
- EditorIDの固定リストではなく、Voice・Aimed・詠唱種別・敵対的／有害な
  Healthダメージ・Projectile・属性・術者ActorTypeを組み合わせて判定します。
  ロード順で勝っているVoice Spellを`kDataLoaded`時に分類するため、
  同じ構造を持つMOD追加攻撃にも対応します。
- 既存のStagger Archetype効果を1つでも含む魔法は除外します。
  このためDrain VitalityなどにSimpleStaggerのよろめきが重複しません。
- マント、壁、ルーン、罠、Hazard、自己中心魔法、武器付呪の重複経路、
  爆発蜘蛛の投射物、一般のクリーチャー投射物は従来どおり対象外です。

### パフォーマンスと状態管理

- Voice攻撃の構造分類とCloak除外をデータロード時に一度だけ実行し、
  ヒットごとの全レコード走査を行いません。
- 対象単位の魔法判定クールダウンと保留中タスクを、1つのスレッドセーフな
  状態管理へ統合しました。同じ対象へ複数タスクを同時予約せず、
  `MagicCooldownMs`の経過後は集中放射中でも次のタスクを生成します。
- 1024件を目安に256観測ごと、30秒またはクールダウン値の4倍より古い状態を
  遅延削除します。100万回の反復観測と期限切れ削除を自動テストへ追加しました。
- 初期分類件数と所要時間は起動時に1回だけ記録し、ヒットごとのINFOログは
  追加していません。

## 1.3.2

### マント魔法の除外と魔法対象の明確化

- 炎の精霊などの常時発動マントに接触している間、ダメージ用の集中魔法が
  `TESHitEvent`を繰り返し発生させ、`MagicCooldownMs`経過ごとに何度でも
  よろめかされる問題を修正しました。
- バニラ`crAtronachCloakFlameDmg`（`06445B:Skyrim.esm`）が
  `Concentration / Aimed`で、敵対的かつ有害な破壊魔法効果を持つことを
  実レコードで確認しました。従来の対象条件を満たしていたため、報告は事実です。
- ロード順で勝っている全Magic Effectを走査し、`Cloak` Archetypeが関連付ける
  ダメージ魔法をSimpleStaggerの対象から完全に除外します。EditorIDや
  固定FormIDに依存しないため、同じ構造のMOD追加マントにも適用されます。
- 魔法よろめきを、通常Spell・杖・Scrollから発射される、敵対的または有害な
  破壊魔法の`Aimed`な弾・集中放射だけに限定しました。自己対象、接触、
  `TargetLocation`、能力、Power、Voice、Shoutなどは対象外です。
- 杖のObject Effectを魔法経路として判定するようにしました。
- ドラゴンの火炎・冷気ブレスおよび火球・氷弾は`Voice` Spellであり、
  従来からSimpleStaggerの対象外です。この挙動は変わりません。
- 対象となる集中放射は「対象・術者・魔法Form」の組ごとに連続命中を追跡し、
  途切れない1回の命中区間では最初の1回だけよろめきを許可します。
- 魔法対象ポリシー、連続命中の抑止、無命中期間後の再武装、キー分離、
  セッション状態のクリアを自動テストで検証します。

## 1.3.0（1.2.0からの変更）

### 重要な修正

- Skyrim AE 1.6.1170でよろめき発動時にCTDする問題を修正しました。
  `IsInBleedout()`と`IsEssentialDown()`への未解決のランタイム呼び出しを廃止し、
  SE/AEで安全に参照できるActorのライフステートから状態を判定します。
- ヘルゲン導入部のアルドゥイン襲撃など、スクリプト演出中のよろめきで
  操作不能またはフリーズする問題を防止しました。拘束状態のActorと、
  移動・視点・戦闘操作が無効化されているプレイヤーには
  SimpleStaggerのAnimation Eventを送信しません。
- 弓の判定が飛び道具イベントの有無に依存し、`EnableRanged=false`でも
  よろめきが発生する場合がある問題を修正しました。武器Formの種類から
  Bow/Crossbowを判定するため、Bound BowとMOD追加弓も`EnableRanged`だけに従います。
- `PlayerCanBeStaggered=false`が一部の経路や設定変更直後に反映されない可能性を
  修正しました。イベント受信時、ゲームスレッド上のタスク実行時、
  Animation Event送信直前の各段階で再確認します。
- 最終Magnitudeが0以下または非有限値でも`staggerStart`が送信されていた問題を
  修正しました。基本値が0の場合はクールダウン更新やタスク生成も行いません。
- バニラのパーク`Tower of Strength`を取得していてもSimpleStaggerの
  よろめきを防げない問題を修正しました。重装を1つ以上装備し、
  軽装を装備していない場合、50%の確率でよろめきを完全に無効化します。
- 眠っている／起床中のドラウグルへ`staggerStart`を送ると、専用の
  Furniture／起床Idleと競合して停止・浮遊などを起こし得る経路を修正しました。

### 安全性

- 死亡・死亡遷移中、Bleedout、Essential Down、Unconscious、Kill Move中、
  Ragdoll中のActorにはSimpleStaggerのよろめきを適用しないようにしました。
- Kill Move中の攻撃者からのヒットも除外し、演出の中断を防止しました。
- イベント処理中は安全なランタイムデータだけを読み、エンジン関数を必要とする
  状態判定はゲームスレッドへ移動しました。
- Sleepへの移行中、Sleep中、起床中、およびFurnitureを占有中のActorには
  SimpleStaggerのAnimation Eventを送信しません。DraugrのRaceや個別FormIDを
  ハードコードせず、同じ危険がある他のFurniture演出も保護します。
- SBSによる連続よろめきとの互換性を維持するため、すでによろめき中であることを
  理由とした除外は追加していません。

### 武器ヒット

- 近接攻撃は`TESObjectWEAP::IsMelee()`に該当する武器だけを対象にしました。
  Staffなど分類外の武器は近接攻撃として扱いません。
- Blocked倍率とLarge Actor倍率の適用後にもMagnitudeを検証し、
  最終値が0以下ならAnimation Eventを送信しません。

### 魔法

- よろめき対象を、敵対的または有害な破壊魔法のSpell／Scrollに限定しました。
  回復、補助、変性、幻惑、召喚、Ability、Power、Voice Power、Shout、
  非破壊Scrollは対象外です。
- 魔法Magnitudeを効果の`MinimumSkill`に応じた5段階へ変更しました。

| ランク | MinimumSkill | デフォルトMagnitude |
|---|---:|---:|
| Novice | 0～24 | 0.10 |
| Apprentice | 25～49 | 0.15 |
| Adept | 50～74 | 0.25 |
| Expert | 75～99 | 0.35 |
| Master | 100以上 | 0.50 |

- 複数の対象効果を持つ魔法では、最も高コストな対象効果のランクを使用します。
  コストが同値または不明な場合は、最大の`MinimumSkill`を使用します。
- 魔法用クールダウン`MagicCooldownMs=200`の既定値は維持しています。

### INIと設定移行

- 通常攻撃Magnitudeの正式キーを`NormalAttackMaginitude`へ変更しました。
  この`Maginitude`という綴りは1.3.0の仕様です。
- 旧`StaggerMagnitude`は新キーがない場合のみ読み込みます。
- 旧`MagicMagnitude`しかないINIは、その値をAdept基準として次の倍率で移行します。

| 新しいキー | 旧値に対する倍率 |
|---|---:|
| `NoviceMagicMagnitude` | 0.4 |
| `ApprenticeMagicMagnitude` | 0.6 |
| `AdeptMagicMagnitude` | 1.0 |
| `ExpertMagicMagnitude` | 1.4 |
| `MasterMagicMagnitude` | 2.0 |

- 移行後の値は0.0～2.0に制限されます。新しいランク別キーが1つでも存在する場合、
  旧`MagicMagnitude`は無視され、欠けているランクにはデフォルト値を使用します。
- SMFからINIを保存すると、旧`StaggerMagnitude`と`MagicMagnitude`を削除し、
  1.3.0のキーだけを書き出します。

### SKSE Menu Framework

- 実行時設定をスレッドセーフにし、SMFの描画スレッドとヒット処理間の
  データ競合を解消しました。
- `Immune`、`Susceptible`、5段階の魔法Magnitudeをメニューへ追加しました。
- Overrideをカンマ区切りテキストで編集できるようにし、
  `Apply Overrides`でゲームスレッド上の解決処理を実行します。
- `Save to INI`、`Reload from INI`、`Apply Overrides`の結果を
  メニュー内に表示するようにしました。詳細はSimpleStaggerのログへ記録します。

### パフォーマンス

- 無効な攻撃カテゴリ、対象外の魔法、Magnitude 0のヒットでは
  ゲームスレッドタスクを生成しません。
- 解決済みOverrideを不変スナップショットとして公開し、
  ヒット処理でmutexやファイルI/Oを行わないようにしました。

### 互換性と既知の制限

- SimpleStaggerは引き続きAnimation Graphへ直接値を書き込み、
  `staggerStart`を送信します。
- Skyrim.esmの`Tower of Strength`については、重装装備時に
  50%の確率でよろめきを完全無効化する挙動を明示的に再現します。
  Enderal固有Perkや、他MODが追加する`ModIncomingStagger`／
  `ModTargetStagger` Entry Pointの汎用適用はまだ行いません。
- Long Awaited - Attack Interruptsとはファイル競合しませんが、
  攻撃中断・よろめき機能が重複します。Magic、Player、Rangedの各カテゴリでは、
  どちらか一方のMODだけがAnimation Eventを担当する設定を推奨します。

### 更新時の注意

- 1.2.0のINIはそのまま読み込めます。SMFで保存すると自動的に1.3.0形式へ移行します。
- 手動でINIを置き換える場合は、既存の`Immune`／`Susceptible`設定を
  新しいINIへコピーしてください。

## 1.2.0

- Spell／Scrollヒットによる魔法よろめきを追加しました。
- 武器用とは独立した`EnableMagic`、`MagicCooldownMs`、
  `MagicMagnitude`設定を追加しました。
