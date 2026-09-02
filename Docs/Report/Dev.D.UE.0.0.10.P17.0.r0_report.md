# Dev.D.UE.0.0.10.P17.0.r0 Report

## 1. 结论

P17.0 已把规划中点名的被动保命灵器“护心镜”接入 0.0.10 的真实物品、战斗、防御、生命与持久资源链。它不是名称或类别推断出来的临时特判，而是一个带显式 `LethalInterception` 语义、唯一 Charges 权威和 `PreventLethal` 防御层的正式物品定义。

当敌方 Impact 原本会致死时，护心镜保留 1 点生命并消耗唯一一层 charge；非致死 Impact 不消耗；同一 Impact 的精确重放不会再次消耗。护体法袍与护心镜同时装备时，耐久与 charge 被纳入同一个 durable multi-resource intent，与生命提交形成可恢复终态。

最终结果：

```text
DefenseResourceAdapter focused:    9 Success / 0 Fail
Shanmen.0_0_10 full:             749 Success / 0 Fail
Mapped legacy regressions:       422 Success / 0 Fail
Additional V3.Items:               5 Success / 0 Fail
Regression coverage:              PASS (Changed=14 / Rules=8 / Required=25 / Logs=10)
Regression gate self-test:         PASS 273/273
Game + Editor Development:         PASS / native status 0
```

## 2. 护心镜产品契约

新增 canonical definition：

| 字段 | 值 |
|---|---|
| DefinitionId | `Prototype.Item.Accessory.HeartProtectingMirror` |
| 中文 / 世界标签 | `护心镜` / `HEART-PROTECTING MIRROR` |
| Category / Slot | `AccessoryCategory` / `AccessorySlot` |
| Stack / Durability / Charges | `1 / 0 / 1` |
| Gameplay semantic | `LethalInterception` |
| Effect | `Effect.LethalVitalityFloor = 1.0` |
| 交易 | 不可购买、可出售 100、prototype value 240 |
| Reward pool | `P17.0.Pool.Accessory.HeartMirror`，accessory tag，weight 6 |

目录严格校验要求全仓只有一个 `LethalInterception` 定义，并固定其 ID、类别、槽位、资源、effect 数量和值。物品目录由 41 增至 42；新内容身份为 `CodeB.Content.0.0.10.P17.0`，digest 为 `5C09D58AA2EB206FA39F2BE896F7B071149CE8FE3D4E75780CFE213432638399`。P16.0 身份保留为已知历史版本，迁移与既有语义测试均已更新。

## 3. Impact 与资源顺序

敌方攻击的产品路径保持已有确定性 ImpactId 与生命 CAS，不新增第二套伤害或生命权威。顺序为：

```text
capture base defense
  -> recover any pending resource intent
  -> cancel bounded pre-intent orphans
  -> reserve equipped resource lines
  -> append canonical defense layers
  -> resolve pure Impact
  -> durable multi-resource intent
  -> recover/commit vitality
  -> commit triggered lines + cancel untriggered lines
```

护体法袍只从旧 aggregate armor 中剥离自己的 2 点减伤，然后以 durability-backed `AbsorbPoints` 层重新加入。护心镜不改写 aggregate；它在 `FShanmenDefenseOrder::LethalInterception` 加入 `PreventLethal` 层，并要求 `TargetLiving`。两层都以 reservation ID 作为 LayerId，触发回执因此能精确映射回唯一物品资源。

准备结果由单个 Reserve/ReservationId 升级为同序数组。所有已识别且可用的资源会在返回前逐项完成准备；中途出现结构或持久写入错误时，已创建的临时 reservation 会在同一有界流程中取消。资源耗尽不是伪故障：对应层不进入 Impact；若所有已识别资源都不可用，返回显式 `ResourceUnavailable`。

## 4. 原子性、幂等与重开恢复

每个临时防御 reservation 使用 ImpactId 编码的 `SMDR1_...` purpose。进入 durable intent 后，触发资源行排在前、未触发行排在后，`TriggeredLineCount` 冻结两组边界；同一 intent 同时携带精确生命 command。最终化时只提交触发行，其余行取消。

| 场景 | 生命结果 | 物品结果 |
|---|---|---|
| 3 HP 接受 5 damage，镜 charge=1 | 1 HP | charge 1 -> 0 |
| 5 HP 接受 3 damage，镜 charge=1 | 2 HP | reservation cancel，charge 保持 1 |
| 10 HP，法袍减 2 后镜拦截 | 1 HP | durability 20 -> 19，charge 1 -> 0 |
| exact Impact 重放 | 保持已提交 after-state | prepare/finalize replay，不再扣 charge |
| reserve 后、intent 前重开 | 不提交生命 | orphan reservation 取消一次，charge 保持 1 |
| pending intent 重开 | 只接受 exact before/after vitality | 从 durable metadata 恢复并完成唯一终态 |

Run 启动时只要准备栏中的护体法袍或护心镜要求资源防御，Coordinator 就把 ShanmenItems 权威设为硬依赖。权威未 ready、准备物品身份丢失、资源层不匹配或恢复状态歧义时均失败关闭，不退回无资源的同名防御。

## 5. Automation 覆盖

新增四个产品资源测试：

- `HeartMirrorTriggeredCommit`：验证致死判定、1 HP floor、唯一 charge 提交及 exact replay；
- `HeartMirrorUntriggeredCancel`：验证非致死 Impact 的 reservation 取消与 charge 不变；
- `HeartMirrorSpiritGuardCoexistence`：验证法袍先减伤、护心镜后拦截，并在一个 intent 中提交两个资源；
- `HeartMirrorPreIntentRestart`：重建 authority 后取消 orphan charge reservation，第二次恢复为 no-op。

既有五个 DefenseResourceAdapter 测试继续覆盖单资源触发、未触发取消、durable lifecycle、恢复歧义与 pre-intent restart；focused suite 为 9/0。完整 `Shanmen.0_0_10` 从 P16.9 的 745 增至 749，结果 749/0。Full suite 首末 Success 时间为 `2026.09.02 16:47:53.114 -> 17:15:30.959 UTC`，约 27m37.85s。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| DefenseResourceAdapter focused | 9 | 0 | `D92E28AE3BC4D6AD0ABE884D81913A93D1EECFC637F36A0BD75EE51159FBB27C` |
| `Shanmen.0_0_10` full | 749 | 0 | `874C80FF3D0B4CC50D31031250984438B448FBBA7E2A65DB28FAF67FFE7D2D0C` |
| CodeB | 60 | 0 | `D3DEE54926480B962799ADF564D9BE3248ED0EE6420FEF1DA7AEB83825BE0B3F` |
| EnemySkillFramework | 44 | 0 | `D85CB181DDB0C5385636FA86B82BE82D1B96DB3CF86B67BDF11E019E4E6B8BEE` |
| ItemEconomySchema | 24 | 0 | `50843E4089CAEF9EEC2FD73A994140B81DFEE093868531C1BD5D0DFF35E59E84` |
| ItemUseAndArmor | 46 | 0 | `37FAAD6C7E0303039A9D38FA967CFA0C2A722E69A15008FE89E53156D02FB967` |
| P4.Hotbar | 7 | 0 | `ECD22403EC315F22377ED7D924A26D2CFE8777F71A0C55EF4C8E14DAF072EBB4` |
| Profile | 211 | 0 | `2D978585C21DDE5708E99DE01709AD117EAE04BA77124F6F358CCBDF23A3E0A3` |
| V2RangedCompatibility | 22 | 0 | `285E612862524AEBF96876317338769F3892897FA8C59D27C0CCADB2C42CD9CF` |
| V3.Attributes | 4 | 0 | `F7F5252BA2E31EA63263BE4271C4F94095CF910FD090C712E4D68394E483079A` |
| V3.Items | 5 | 0 | `DE1C5CD1F0ACE0E87FD6F120341BFAEB29A58288D829BC1C4954EE8E9F91109A` |
| V3.WorldInteraction | 4 | 0 | `A87D142E7FF2806823877599E9BB1877DF5DEC2268F1A887118547E221491C7D` |

全部最终日志均包含 UE 5.8 native terminal-success marker，Fail、Fatal、Unhandled 与 Ensure 计数为 0。mapped legacy 合计 422/0；V3.Items 是映射要求之外的额外目录验证。

首次 V3.Items 执行真实暴露了新增定义导致的 deterministic registry order 夹具下标过期，结果 4 Success / 1 Fail。失败原始日志已保留，SHA-256 为 `0EB3324161104FA3024D531ACAFE5A6528F9256779E9C12EFFAE1085E1BD3BE2`。修正断言为镜 index 7、AncientToken index 9 后重编译并得到最终 5/0；没有修改生产排序来迎合旧夹具。

## 7. 改动—回归门禁与静态边界

14 个修改源码路径命中 8 条映射规则，25 个必跑组由 10 份健康日志覆盖：

```text
REGRESSION_COVERAGE: PASS Changed=14 Rules=8 Required=25 Logs=10
SELF_TEST: PASS 273/273
JSON_PARSE: PASS
PRODUCTION_BOUNDARY_SCAN: PASS HITS=0
CONTENT_CONTRACT_SCAN: PASS Version=P17.0 Catalog=42 LethalDefinitions=1
GIT_DIFF_CHECK: PASS
```

regression map 未修改，SHA-256 为 `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。生产 DefenseResourceAdapter 静态扫描确认不依赖 `UWorld`、`AActor`、`ApplyDamage`、RNG、旧 `demo_mapItemSubsystem` 或 Profile；测试 fixture 有意使用旧 Profile/ItemSubsystem 完成真实 run cutover，这不是生产依赖。

## 8. 构建证据

最终源码使用 `-WaitMutex -NoUBA -MaxParallelActions=1` 构建，原生退出码均为 0。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 192 / 494.92s | `8C6DA2F74AEF364E1E5C60BB7450111E7D4581EB2AF39822394F6BCC2072B388` |
| Editor Development | Succeeded / up to date | 0 / 1.29s | `7F1ACDDB335BB3DAC9DCFE6A8F7DB1952AEA99FE95D09261160C9682BF735CF5` |

最终产物：

- `demo_map.exe`：356,131,328 bytes，SHA-256 `8F1332E83E2192119CF8A91A217713E6236E49A758B87E75FDDCF81A61F96352`；
- `UnrealEditor-demo_map.dll`：14,818,304 bytes，SHA-256 `1BE2EF96573B741108C0856B010386D49310D5FF64104F3C06CC024E27E34FFE`。

## 9. 修改范围与 P/F 边界

本轮修改物品内容/语义、资源防御适配器、CombatRun Coordinator、目录/迁移/产品测试共 14 个源码文件，并新增本 Report 与 Development Log，计划提交 16 个文件。没有修改 ShanmenItems authority/ledger schema、生命组件、CombatCore resolver、Profile schema、GameMode、PlayerController、地图、资源、Windows、UE Engine 或用户配置。

本 Report 证明 P 阶段 pure resolver 接入、持久资源 reserve/intent/finalize、authority 重建、精确重放、unattended Automation、mapped regressions、静态门禁与 Development builds。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package；没有把这些未执行项目描述为成功。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF 与用户资料未修改、未暂存、未提交。raw build/test logs 只保存在本地 `Saved/Codex/P17.0`。

## 10. 下一阶段

P17.1 应补一条 Coordinator 级的无头产品纵切：从准备栏部署护心镜开始，由真实 M01 enemy attack product route 产生致死 Impact，验证第一次攻击保留 1 HP、第二个新 Impact 在 charge 耗尽后不再获得拦截，并覆盖 run teardown/restart 后的最终物品快照。该阶段继续复用现有生命与 ShanmenItems 权威，不扩展 UI 或数值平衡。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p17-0-heart-mirror-lethal-interception>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-0-heart-mirror-lethal-interception/Docs/Report/Dev.D.UE.0.0.10.P17.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-0-heart-mirror-lethal-interception/Docs/Log/Dev.D.UE.0.0.10.P17.0.r0_log.md>
