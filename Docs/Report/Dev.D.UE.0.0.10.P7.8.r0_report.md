# Dev.D.UE.0.0.10.P7.8.r0 Report

## 1. 结论

P7.8 已把 P7.6 的暗器 product session 接入真实 active-Run 与 GameMode 生命周期，结论为 **PASS**。

新增 `Fdemo_mapShanmenThrownWeaponProductLifecycle`。它不保存第二份 Run 文档，而是在 begin 时从 ShanmenItems durable receipts 只读重建 `Fdemo_mapShanmenRunCorrelation`，再通过 `Fdemo_mapCombatRunCoordinator` registry 证明 source Actor 就是本 Run 的 canonical player entity。GameMode 在 combat Run 成立后绑定，在 Run 释放前先终止飞行并关闭 session。

本轮未接 1–9 键、Widget、动画或视觉资产；现有通用快捷栏路径未被暗器逻辑劫持。产品层新增的是 device-independent `HotbarIntent` 路由边界，具体设备输入顺延。

## 2. Product lifecycle 契约

生命周期适配器只拥有：

- 一个 P7.6 `Fdemo_mapShanmenThrownWeaponProductSession`；
- 一个 `Udemo_mapShanmenItemAuthoritySubsystem` 弱引用。

`TryBegin` 必须同时满足：

1. authority 可重建完整、有效的 active-Run correlation；
2. correlation RunId 与 CombatRunCoordinator RunId 精确一致；
3. source Actor 可在 coordinator registry 中解析为 player EntityId；
4. 首件真实练习飞刀仍具有 typed `ThrownWeapon` semantic；
5. immutable combat config 可被 P7.6 session 捕获。

exact begin replay 幂等；切换 authority、Run、source Actor 或内容配置均失败关闭。该类没有 Tick、timer、输入绑定、UI、legacy item subsystem 或持久 Run 副本。

## 3. 练习飞刀产品策略

P7.7 已建立真实商品 `Prototype.Item.Consumable.TrainingThrowingKnife`。本轮为其冻结首个直线投掷策略：

```text
ActionDefinitionId = canonical thrown-weapon action
DetectorId         = Detector.ThrownWeapon.TrainingThrowingKnife.Straight
FormulaId          = Combat.Formula.ThrownWeapon.TrainingThrowingKnife.r1
BaseDamage         = 12
TechniqueCoeff     = 0.3
LaunchSpeed        = 900
DamageTag          = Damage.Physical.Slash
RequiredTargetTag  = Target.Living
SourceTag          = Source.Player
RejectSelf         = true
```

策略创建前必须查询 canonical item catalog 并验证 typed semantic；不从 DefinitionId、Category、名称或价格推断能力。

## 4. GameMode 集成

`Ademo_mapGameMode::TryActivateCombatRun` 在原 coordinator 与 M01 entity 注册完成后检查 ready Shanmen item authority。存在 product authority 时创建 thrown-weapon lifecycle；旧 authority/未 cutover 路径保持兼容，不凭空创建另一份 Run。

GameMode 新增四个显式产品入口：

- `RouteThrownWeaponHotbarIntent`；
- `RecoverThrownWeaponCancellation`；
- `InterruptThrownWeaponFlight`；
- `ExpireThrownWeaponRange`。

`ReleaseCombatProductRun` 的顺序为：先关闭 thrown-weapon lifecycle，再关闭 controlled-weapon lifecycle，最后结束 CombatRunCoordinator。活动飞行会先显式 interrupt；若存在 unresolved durable recovery，teardown 失败并保留状态，不静默 reset 或丢失恢复责任。

## 5. 自动化证据

最终无头自动化全部通过；每份 canonical 日志均为一个实际 RunTests、queue-empty、Fail `0`，进程原生退出码为 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `ThrownWeaponProductLifecycle` / `P7.8_Targeted.log` | 3 | 0 | `87AA534F7B2B00CD4D479813002F9FEFD52605CECD00F6C320C5DBDA1626FB81` |
| `Shanmen.0_0_10` / `P7.8_Full.log` | 204 | 0 | `775EBECF6665AA9E033F4001558E714F1DA137591D1C5E1C6DB061045C00BD38` |
| `demo_map.ItemUseAndArmor` / `P7.8_ItemUseAndArmor.log` | 46 | 0 | `DB23FAC76925A45ABA73AA36D5AA587A527F1220F5CAF23DC9A3888E9618697C` |

新增三项测试覆盖：

1. 真实内容策略、durable correlation、source registry 与 exact begin replay；
2. 真实飞刀 hotbar route、authority revision、SelectionId replay/conflict、flight teardown；
3. inactive/foreign coordinator 与未绑定 lifecycle 的 mutation-free fail-closed 边界。

完整 suite 从 P7.7 的 `201` 增至 `204`。

## 6. 首次失败与修复

首次 Editor 编译原生退出码 `1`，UBT 结果 `OtherCompilationError`。唯一错误为 GameMode 调用了 subsystem 不存在的 `Authority->IsReady()`。改用现有枚举契约 `GetLifecycleState() == Ready` 后 Editor 成功。该错误是源码接口错误，不是内存、页面文件或环境故障。

首次 targeted automation 为 `1 Success / 2 Fail`，进程原生退出码 `0`；失败日志 SHA-256：`B48A5E9A5226E50B33076279179068428D4E6F1D83F7827DBD25FDA6AA05CF8F`。新增测试错误地把 active-Run reservation 后的资源当普通库存 Quantity 读取，并把该假设并入组合断言。修复为核验 authority revision 与完整 snapshot：首次 launch 恰好产生两次 durable revision，exact replay、foreign coordinator 与 inactive lifecycle 均保持 snapshot 不变。产品代码未因测试失败而放宽。

## 7. 改动—回归与静态门禁

- `M01GameMode` 映射现在覆盖其实际拥有的 combat/thrown authority seams；
- 新增 `ThrownWeaponProductLifecycle` 映射；
- mapping self-test 增加正向与反向场景，`28/28 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=13 Logs=3`；
- regression map JSON parse：PASS；
- boundary scan：product lifecycle 无 Tick、timer、input binding、legacy item subsystem、ApplyDamage 或 RNG；
- `git diff --check`：native exit `0`。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| 构建 | Result | Native exit | 时间 | 日志 SHA-256 |
|---|---|---:|---:|---|
| Editor first integration | `OtherCompilationError` | 1 | 111.63s | 未保留独立 UBT 文件；错误原文已记录 |
| Editor final incremental | Succeeded | 0 | 5.86s | `DA9F34A745E5F7A3D068138F4D90C02E403901E4B7452B42A6D59139DAA49EF3` |
| Game final integration | Succeeded | 0 | 104.86s | `97FE173873901B10261DB898A7C823A3CAE7BC403A80CB742B2CF061E9BC2C54` |

- Editor library：`248818` bytes，UTC `2026-08-29T13:51:17Z`；
- Game executable：`351897088` bytes，UTC `2026-08-29T13:56:17Z`。

## 9. 兼容性与完整性

- 未修改 save schema、Content 资产、GameplayTags 配置或 Build.cs；
- 未改变现有 1–9 快捷栏消费路径、Code B 或 legacy inventory authority；
- 未增加第二个 GameMode、Run owner、hotbar、item catalog 或持久状态；
- P7.1–P7.7 的 item gate、world delivery、host、router、controller、session 与真实商品保持原契约；
- 长期未跟踪的 0.0.9B 与用户文件未 stage、未覆盖。

## 10. P/F 边界、下一阶段与 GitHub

本轮只执行 P 阶段源码、静态检查、无头 Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook 或 Package。

P7.9 建议建立唯一 input adapter：先按 frozen active-Run hotbar exact item 判断 typed semantic；暗器才采样 source transform / aim 并生成稳定 SelectionId，普通消耗品继续原路径。该阶段仍不把输入、瞄准或设备状态写入 Session，也不提前加入动画/视觉资产。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-8-thrown-weapon-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P7.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-8-thrown-weapon-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P7.8.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-8-thrown-weapon-lifecycle>
