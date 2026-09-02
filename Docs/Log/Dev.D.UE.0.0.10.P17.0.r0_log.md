# Dev.D.UE.0.0.10.P17.0.r0 Development Log

## 1. 目标与基线

- 基线提交：`4835cdcf09c8b0914a19de2849d891315b26917d`（P16.9 Meridian Shock recovery lifecycle）；
- 分支：`agent/0.0.10-p17-0-heart-mirror-lethal-interception`；
- 目标：实现规划已点名的护心镜，使一次被动致死拦截由真实 accessory charge 支付，并进入既有可恢复 Impact 事务；
- 约束：不建立第二套物品、生命、伤害或持久化权威；本轮只做 P 阶段。

## 2. 起始审计

P0 已有数据驱动的有序 `FShanmenDefenseLayer`、`PreventLethal` operation、目标当前/最大生命快照和守恒回执，因此不需要修改 CombatCore resolver。P5.4 已有法袍 durability 与生命的 recoverable saga，但准备结果只能携带一个 reservation，无法表达法袍与护心镜并存。

物品目录已有 Accessory 槽与 Charges 资源，但没有显式的被动保命语义和内容定义。CombatRun 只在准备了 Spirit Guard Robe 时把 item authority 设为硬依赖；普通 accessory 缺失资源权威仍可能静默走无资源路径。

据此把 P17.0 定义为“内容 + 多资源防御接入 + durable recovery”最小纵切，不改纯结算数学。

## 3. 内容实现

新增：

- `Edemo_mapItemGameplaySemantic::LethalInterception`；
- `Fdemo_mapItemIds::HeartProtectingMirror`；
- `Fdemo_mapItemEffectIds::LethalVitalityFloor`；
- 护心镜正式 definition 与 `P17.0.Pool.Accessory.HeartMirror` 生成池条目；
- `CodeB.Content.0.0.10.P17.0` 当前 identity 与 digest；P16.0 加入 historical identity allowlist。

目录数量从 41 更新到 42。严格校验要求只有一个合法的致死拦截定义，且必须是单格、单件、AccessorySlot、0 durability、1 charge、唯一 floor=1 effect。ItemEconomySchema、V3.Items、WorldInteraction、migration 与跨适配器 identity fixtures 同步更新。

## 4. 多资源防御实现

`Fdemo_mapShanmenDefenseResourcePreparationResult` 的单数 request/command/id 改为同序数组，并让 `HasResourceLayer()` 校验数组数量与每个 GUID。`PrepareImpactDefense()` 现在执行：

1. 恢复任何未完成 durable intent；
2. 有界取消同 Run、同 owner、已准备装备上的临时 durability/charges orphan；
3. 读取唯一 active Run correlation 与 authority snapshot；
4. 为护体法袍 reserve 1 durability，并恢复其显式 2 点 `AbsorbPoints` 层；
5. 为护心镜 reserve 1 charge，并添加 floor=1 的 `PreventLethal` 层；
6. 任一硬错误时取消本次已成功的临时 reservations；资源耗尽则只省略该资源层；
7. 将所有资源层以一个 ordered intent 交给原有生命 + item finalize saga。

触发行在 intent 中优先排列，未触发行随后排列；finalize 因而能在一次 durable 决策中提交法袍和镜，或提交一方、取消另一方。pre-intent orphan scanner 从 durability 扩展到 durability/charges，但仍只识别 `SMDR1_` purpose、prepared equipment、exact Run/owner 范围。

CombatRun 绑定增加护心镜识别：准备栏存在镜时 item authority 成为硬依赖。实际敌方 Impact 仍先准备资源、后重新采样 vitality CAS、再由 pure resolver 结算；pre-delivery 失败会取消全部临时资源。

## 5. Automation 变更

fixture 可分别部署法袍、护心镜或二者，并允许指定 raw damage。新增：

- `HeartMirrorTriggeredCommit`；
- `HeartMirrorUntriggeredCancel`；
- `HeartMirrorSpiritGuardCoexistence`；
- `HeartMirrorPreIntentRestart`。

关键断言：

```text
lethal:      CurrentVitality 3, RawDamage 5 -> FinalDamage 2, vitality 1, charge 0
nonlethal:   CurrentVitality 5, RawDamage 3 -> FinalDamage 3, vitality 2, charge 1
coexistence: CurrentVitality 10, RawDamage 10 -> robe prevents 2, mirror prevents 4,
             FinalDamage 4, vitality 1, durability 19, charge 0
replay:      Prepare=Replayed, Finalize=Replayed, vitality/charge unchanged
restart:     one orphan cancelled on first recovery, zero on second, charge remains 1
```

## 6. 故障与修正

实现期间将旧单数 reservation 调用方统一升级为数组语义。内容 identity fixtures 同步更新为 P17.0/current + P16.0/historical，完整与增量编译均通过。

首次 `demo_map.V3.Items` 自动化得到 4 Success / 1 Fail：`DefinitionRegistry` 的 deterministic order 断言仍认为 `AncientToken` 位于旧 index 8。护心镜按 canonical prototype 顺序插入后，该夹具失效。保留失败日志 `P17_0_V3_Items-backup-2026.09.02-16.46.05.log`，SHA `0EB3324161104FA3024D531ACAFE5A6528F9256779E9C12EFFAE1085E1BD3BE2`；只修正测试为护心镜 index 7、AncientToken index 9，没有改生产排序。重编译后复跑 5/0。

## 7. 最终测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `P17_0_DefenseResourceAdapter.log` | `Shanmen.0_0_10.Items.DefenseResourceAdapter` | 9/0 | `D92E28AE3BC4D6AD0ABE884D81913A93D1EECFC637F36A0BD75EE51159FBB27C` |
| `P17_0_ShanmenFull.log` | `Shanmen.0_0_10` | 749/0 | `874C80FF3D0B4CC50D31031250984438B448FBBA7E2A65DB28FAF67FFE7D2D0C` |
| `P17_0_CodeB.log` | `demo_map.CodeB` | 60/0 | `D3DEE54926480B962799ADF564D9BE3248ED0EE6420FEF1DA7AEB83825BE0B3F` |
| `P17_0_EnemySkillFramework.log` | `demo_map.EnemySkillFramework` | 44/0 | `D85CB181DDB0C5385636FA86B82BE82D1B96DB3CF86B67BDF11E019E4E6B8BEE` |
| `P17_0_ItemEconomySchema.log` | `demo_map.ItemEconomySchema` | 24/0 | `50843E4089CAEF9EEC2FD73A994140B81DFEE093868531C1BD5D0DFF35E59E84` |
| `P17_0_ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `37FAAD6C7E0303039A9D38FA967CFA0C2A722E69A15008FE89E53156D02FB967` |
| `P17_0_P4_Hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | `ECD22403EC315F22377ED7D924A26D2CFE8777F71A0C55EF4C8E14DAF072EBB4` |
| `P17_0_Profile.log` | `demo_map.Profile` | 211/0 | `2D978585C21DDE5708E99DE01709AD117EAE04BA77124F6F358CCBDF23A3E0A3` |
| `P17_0_V2RangedCompatibility.log` | `demo_map.V2RangedCompatibility` | 22/0 | `285E612862524AEBF96876317338769F3892897FA8C59D27C0CCADB2C42CD9CF` |
| `P17_0_V3_Attributes.log` | `demo_map.V3.Attributes` | 4/0 | `F7F5252BA2E31EA63263BE4271C4F94095CF910FD090C712E4D68394E483079A` |
| `P17_0_V3_Items.log` | `demo_map.V3.Items` | 5/0 | `DE1C5CD1F0ACE0E87FD6F120341BFAEB29A58288D829BC1C4954EE8E9F91109A` |
| `P17_0_V3_WorldInteraction.log` | `demo_map.V3.WorldInteraction` | 4/0 | `A87D142E7FF2806823877599E9BB1877DF5DEC2268F1A887118547E221491C7D` |

最终十二份日志全部有 native terminal-success marker，Fail/Fatal/Unhandled/Ensure 为 0。focused 是 full 的子集；mapped legacy 为 422/0；V3.Items 为额外验证。

## 8. 门禁与构建

```text
REGRESSION_COVERAGE: PASS Changed=14 Rules=8 Required=25 Logs=10
SELF_TEST: PASS 273/273
JSON_PARSE: PASS
PRODUCTION_BOUNDARY_SCAN: PASS HITS=0
CONTENT_CONTRACT_SCAN: PASS Version=P17.0 Catalog=42 LethalDefinitions=1
GIT_DIFF_CHECK: PASS
```

- regression map SHA：`00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`；
- Game Development：Succeeded / 192 actions / 494.92s / native 0 / log SHA `8C6DA2F74AEF364E1E5C60BB7450111E7D4581EB2AF39822394F6BCC2072B388`；
- Editor Development：Succeeded, up to date / 0 actions / 1.29s / native 0 / log SHA `7F1ACDDB335BB3DAC9DCFE6A8F7DB1952AEA99FE95D09261160C9682BF735CF5`；
- `demo_map.exe`：356,131,328 bytes / SHA `8F1332E83E2192119CF8A91A217713E6236E49A758B87E75FDDCF81A61F96352`；
- `UnrealEditor-demo_map.dll`：14,818,304 bytes / SHA `1BE2EF96573B741108C0856B010386D49310D5FF64104F3C06CC024E27E34FFE`。

## 9. 边界与提交范围

本阶段不改 CombatCore resolver，因为 P0 契约已能表达致死拦截；不改 ShanmenItems ledger/schema，因为现有 multi-line intent 已能承载两个资源；不改 Profile、地图、UI 或输入。没有运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

计划提交 14 个已跟踪源码文件与本 Report/Log。长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料保持未暂存；`Saved/Codex/P17.0` 原始日志不入 Git。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p17-0-heart-mirror-lethal-interception>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-0-heart-mirror-lethal-interception/Docs/Report/Dev.D.UE.0.0.10.P17.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-0-heart-mirror-lethal-interception/Docs/Log/Dev.D.UE.0.0.10.P17.0.r0_log.md>
