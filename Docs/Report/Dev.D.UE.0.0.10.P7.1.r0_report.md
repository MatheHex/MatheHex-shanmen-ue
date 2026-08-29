# Dev.D.UE.0.0.10.P7.1.r0 Report

## 1. 结论

P7.1 已完成 P7.0 初级直线暗器执行与现有 `ShanmenItems` 权威之间的两阶段 Quantity 事务桥，结论为 **PASS**。

暗器 activation 现在以 exact `SourceItemInstanceId` 和 exact `ActivationId` 建立持久意图：发射前只 prepare，不扣数量；只有同一 action 的有效 P7.0 launch receipt 才能 commit；发射前取消则释放意图且数量不变。重复 prepare/finalize 精确重放，身份或 payload 冲突失败关闭。

本轮没有引入 Actor、World、输入或第二套库存。P7.0 Runtime 仍不直接写物品；产品 adapter 只把不可变战斗证据映射到既有 GameInstance item authority。

## 2. 两阶段 Quantity 权威

新增两个 append-only 操作：

- `PreparePreparedRunQuantityIntent`：冻结 active-Run、exact item、数量、CAS 前值、用途和外部 IntentId，不改变 committed balance；
- `FinalizePreparedRunQuantityIntent`：校验 exact prepare 后选择 commit 或 cancel；commit 才追加一次数量消耗，cancel 保持原量。

核心约束如下：

- IntentId 对暗器固定为 action `ActivationId`；
- 同一 active Run/item 同时最多一个 pending Quantity intent；
- pending 期间禁止另一 intent、旧 `ConsumePreparedRunItem` 直扣和 Run terminal finalize；
- prepare/finalize 各有独立确定性 fingerprint，RequestId 重放必须 payload 完全相同；
- commit receipt 记录 `ResourceBefore/After`，并与旧 direct consume 共用一条按 authority revision 排序的守恒链；
- 持久写失败由现有 authority service 回滚，重启后可继续 finalization，且不会双扣。

## 3. 产品暗器事务适配器

新增 `Fdemo_mapShanmenThrownWeaponItemAdapter`，它只接受：

- 有效且未 final 的 active-Run correlation；
- Action exact source item 属于该 Run 的 prepared inventory；
- item definition 同时具有 Quantity capability 与 exact `Item.Weapon.Thrown` tag；
- P7.0 canonical straight-thrown action 和一致 Content stamp；
- matching action/activation/item 的有效 launch receipt。

固定用途为 `Shanmen.ThrownWeapon.StraightLaunch.r1`。普通消耗品不能借用暗器语义；另一 activation 的 launch 不能提交当前 intent；数量不足、陈旧 snapshot、错误 Run、错误 item 或错误 definition 均失败关闭。

## 4. 重启与投影修复

active-Run Quantity 的真值由“准备时 committed reservation 数量减去 append-only 消耗 receipts”重建，不依赖已耗尽的持久 item tombstone。

交叉审查发现既有 `BuildRuntimePlan` 只统计旧 `ConsumePreparedRunItem`，会在重启 materialization 时漏掉已 commit 的两阶段暗器消耗。现已同时统计：

- exact active Run 的 direct consume；
- `FinalizePreparedRunQuantityIntent` 且 phase 为 `Committed` 的 terminal receipt。

新增 `QuantityIntentRestartProjection` 证明：3 个 prepared item commit 1 个后，authority 重启和 Run resume 仍 materialize 为 2；不会把已投掷物品恢复回来。

本轮没有新增持久字段或 schema version；新操作复用既有 receipt 结构，旧 direct-use ledger 与 terminal settlement 行为保持兼容。

## 5. 自动化证据

最终无头 `-NullRHI` 自动化全部通过。各日志只有一个实际 RunTests、queue-empty、Fail `0`、Fatal/assert/ensure `0`，原生退出码均为 `0`。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Items.RunLifecycle.QuantityIntentRestartProjection` | 1 | 0 | `418F19CF44F1880E51D9869EDEA3D5134F5BDF5791EC18DEC30F91528F839097` |
| `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter` | 4 | 0 | `F468A1178D42307705BCFF8BCA8B9E5F506C1D67382DC6D3AF78D377BAEA662E` |
| `Shanmen.0_0_10.Items` | 72 | 0 | `2915D4412BB9626E3CA3DA983F9C5B1E75C62320A374A4D27F1381A12BCFEC61` |
| `Shanmen.0_0_10.CombatRuntime` | 30 | 0 | `89D9577D992040A1F84952D080925C479F1F32C652CE0649D15EC5C3ACFBE285` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `D948F972307BE86AFA0BA7DFB97EBFB3707FF2D2223E2CDB0CAB6753024BDECC` |
| `demo_map.P4.Hotbar` | 7 | 0 | `09CD63DCD0EEC119EBD19FBCEFCE98E6B19C741627BB51700D6157E19A9C3327` |
| `demo_map.Profile` | 211 | 0 | `714A89B2634E83EFD584EA13AF13931E7BA6B591C40D97C941E31DFBFD6FEA19` |
| `demo_map.CodeB` | 60 | 0 | `8E8F33A15867F488909B29207AC73EC531ABFFE1CF87DE87218825AB643EECF6` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `84EEA7106E92076AB8AA567E8ACA5F8689021CFC2F4C433DB185D505A91FE127` |
| `Shanmen.0_0_10` | 182 | 0 | `D611CB58A163EF1EF8A2B73A3FD97158CA72DAA34F71E1BB61919A651493FC07` |

这些 group 存在包含关系，Success 不作相加。完整 suite 从 P7.0 的 175 增至 182。

## 6. 改动—回归与静态门禁

- `REGRESSION_COVERAGE: PASS Changed=17 Rules=4 Required=9 Logs=9`；
- mapping self-test：`17/17 PASS`；
- `git diff --check`：native exit `0`；
- 临时 `P71INV`、TODO、FIXME、HACK 扫描命中 `0`；
- 新 adapter 没有 World/Actor/ApplyDamage/Spawn/Tick/timer/RNG 或 legacy inventory 写入；
- 未修改 Build.cs、GameplayTags、资产或 schema version。

本轮特意按实际改动文件运行 `Profile`、`CodeB`、`V2RangedCompatibility`、ItemUse 和 Hotbar，而不是只按“暗器主题”选择新测试。

## 7. 构建与首次失败记录

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 初次 core Editor integration：`68/68` actions，Succeeded，native exit `0`，`191.56s`；
- 首次 product adapter compile：native exit `1`，`OtherCompilationError`；原因为把 `FName Version` 直接加入 `TArray<FString>`，改为 `.Version.ToString()`；
- 最终 Editor：`6/6` actions，Succeeded，native exit `0`，`11.73s`；
- 最终 Game：`65/65` actions，Succeeded，native exit `0`，`185.19s`；
- Editor product DLL UTC：`2026-08-29T10:02:28Z`；Game executable UTC：`2026-08-29T10:10:20Z`。

保留两份首次自动化失败：

1. `p71_quantity_intent_initial.log`：`0 Success / 1 Fail`，SHA-256 `77BCD940FE466A3CA1926A8B0CE5A3836DC45CE67B8434A84AD0136D93BE1B00`。Run finalization 会把原 Quantity reservation 从 `Committed` 转为 `Released`，初版 invariant 未接受该合法 terminal 状态；修正后 reload/settlement 通过。
2. `p71_focused_initial.log`：`4 Success / 1 Fail`，SHA-256 `FC82222558F9544344374A4ABF7FA13C654BD243ABEE64B1087CD14C391D5A42`。测试把 `GameInstanceSubsystem` 建在无效 Outer 上触发 handled ensure；改为真实 transient `UGameInstance` 生命周期后 4/4 product adapter 与全部最终组通过。

以上均为源码或测试夹具问题，已修正并完整重跑；没有把失败描述成环境错误。Game executable 只构建，未启动。

## 8. 修改范围与兼容性

- `ShanmenItems`：操作/错误/request、repository、durable service 与测试；
- `demo_mapShanmenItemAuthoritySubsystem`：Game Thread durable facade；
- `demo_mapShanmenThrownWeaponItemAdapter`：P7.0 launch 到 item authority 的单向桥和测试；
- `demo_mapShanmenRunLifecycleAdapter`：重启 balance projection；
- `demo_mapShanmenPreparationAdapterTests`：真实 restart materialization 回归；
- 回归映射及 self-test；
- 本 Report 与同名 Development Log。

P0–P7.0 的 CombatCore、Runtime、Items、御器、Vitality、Defense、旧 consumable use 和 terminal settlement API 均保留。P7.1 不修改 P7.0 execution，也不让 action/Actor 直接扣库存。

## 9. P/F 边界与后续

本轮只执行 P 阶段源码、静态检查、无头 Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

下一阶段建议 P7.2 建立一次性直线投掷的 World delivery/Actor 生命周期：只消费 P7.0 immutable launch receipt，物理命中回送 Projectile candidate，并通过 P7.1 commit seam 确认真实发射；输入、弧线和自动路径继续后置。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-1-thrown-item-transaction/Docs/Report/Dev.D.UE.0.0.10.P7.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-1-thrown-item-transaction/Docs/Log/Dev.D.UE.0.0.10.P7.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-1-thrown-item-transaction>
