# Dev.D.UE.0.0.10.P20.1.r0 Development Log

## 1. 目标与基线

- 基线：`e6db6f549d92f3206e1df0dffa02bddec4650381`（P20.0）；
- 分支：`agent/0.0.10-p20-1-thrown-weapon-arc-execution`；
- 目标：把 P20.0 的确定性弧线 plan 组合进既有暗器 execution，同时保持 P7 直线行为与单一 Impact 权威；
- 约束：不修改 Actor、World、ProjectileMovement、产品输入、库存或自动寻路。

## 2. 接入前审计

P7 的 `FShanmenThrownWeaponExecution` 严格绑定 `Combat.Action.ThrownWeapon.Straight01`。它已经拥有 exact source item、Action runtime、projectile detector emission、Impact ledger、damage formula 与终止状态。

P20.0 的 planner 则严格绑定 `Combat.Action.ThrownWeapon.Arc01`，输出完整自校验 plan。两者的 action ID 精确边界使 plan 无法直接进入旧 execution。

评估过另建 `FShanmenThrownWeaponArcExecution` 的方案，但那会复制 P7 的 damage、target policy、hit ordinal、ledger 与 spent-state，形成第二权威。本轮选择扩展原 execution，并把差异限制在 launch proof。

## 3. 明确动作类型

`FShanmenThrownWeaponDefinition` 新增：

- `StraightActionDefinitionId()`；
- `ArcActionDefinitionId()`；
- 旧 `CanonicalActionDefinitionId()` 继续返回 straight，保持现有 P7 调用方兼容。

定义捕获只允许这两个明确动作。`Combat.Action.Projectile.Generic` 与其它动作继续失败关闭。

同一 `LaunchSpeed` 字段按动作具有明确语义：straight 为固定速度，arc 为 execution 允许的最大速度。没有用额外布尔字段表达模式。

## 4. 判别式 launch receipt

新增 `EShanmenThrownWeaponTrajectoryKind`：

- `Straight`；
- `BallisticArc`。

原 straight receipt 的 origin/direction/speed 结构与 launch identity 均保留。arc receipt 保存完整 `FShanmenThrownWeaponArcPlan`，并提供 initial velocity、gravity acceleration 与 flight time 的统一读取接口。

没有把 plan 的顶点、目标、重力和时间复制为第二组可编辑字段。`IsValid()` 对 arc 分支重新验证 plan、本地 action、origin、规范方向、float/double 速度投影以及 deterministic LaunchId。

## 5. 弧线发射门

`TryLaunchArc` 依次要求：

1. definition 的动作严格为 `Arc01`；
2. action runtime 有效且处于 `CanEmitCandidates`；
3. plan 自校验通过；
4. plan action 与 execution action 的 Run/owner/activation/source item/content/source tags 全部相同；
5. request maximum speed 与 solved launch speed 均不超过 definition ceiling；
6. initial velocity 有限且可规范化。

成功后只冻结一次 launch receipt 并进入原 `InFlight`。相同 plan 重放返回同一 receipt；另一 `PlanId` 无法替换已冻结发射。

弧线 launch identity 使用 `Shanmen.ThrownWeapon.ArcLaunch.r1`，纳入 action provenance 与 `PlanId`。直线继续使用原 `Shanmen.ThrownWeapon.StraightLaunch.r1`。

## 6. 单一 Impact 与生命周期权威

发射后的代码没有分叉：arc 与 straight 均调用原 emission session、candidate validation、damage packet、Impact ledger、Defense resolver 和 flight completion。

这保证：

- detector kind 仍为 Projectile；
- hit ordinal 仍由同一 emission session 推进；
- exact item 与 action identity 不丢失；
- 重复 callback 仍由同一 ledger 拒绝；
- termination cleanup 与 spent-state 不复制。

## 7. 新增测试

新增 `ShanmenThrownWeaponArcExecutionTests.cpp`，4 项：

- `ActionBoundary`：只允许明确 Arc action，straight API、空 plan 与 generic projectile 失败关闭；
- `LaunchAndReplay`：plan 被完整嵌入、目标可重构、同计划幂等、异计划不可改道、spent 不可复活；
- `IdentityAndEnvelopeFences`：Startup gate、foreign content plan 与放宽速度 envelope 被拒绝；
- `SharedImpactAuthority`：自目标拒绝、24 点 damage、重复 contact 拒绝与原生命周期终止。

首次 Editor 编译和首次 exact 4/4 均通过，没有失败日志。随后 `CombatRuntime.ThrownWeapon` 14/14 同时覆盖旧直线 4、P20 planner 6 与新 execution 4。

## 8. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `automation_exact.log` | 4/0 | `F2EC5B9B4347827B45062B26075E367E8EDBAEE8DD351F928ADE2D76E702E592` |
| `automation_thrown_runtime.log` | 14/0 | `02A82DEF1DE7ABDBA0F1480F8BCCAE06B2B727E4901FC5A9393AD84BF3F1B0F2` |
| `automation_combat_core.log` | 9/0 | `684F7F6F4D97AEEB8D4D63CF7CD270363FA16DFDF181DEEDD35A8C48E50381C3` |
| `automation_full.log` | 845/0 | `9610AC67E1851D2CD9C38379702881888F37043A8BCE64339A44FA74CD8FB24C` |
| `automation_legacy_attributes.log` | 4/0 | `8CDA304A3FD9587F2A709C8F18303DA45737446943403AAFE51594BB0CD4EDDB` |
| `automation_legacy_enemy.log` | 44/0 | `47D474EBFD6A3B2BF8E071DE0369F5E41EDA1ADDFB69795767E578DF64540325` |
| `automation_legacy_v2_ranged.log` | 22/0 | `69106D10935C758771156BFF267A26489F84275731F8E523ADEEBF40AA3D4C52` |
| `automation_legacy_item_use_armor.log` | 46/0 | `5340D952C4582CC1F0CE75FF149ECDB9FCDC6CD7D26BBF5DD01D63D7C174AB4E` |

证据审计：

```text
EVIDENCE_AUDIT: PASS Logs=8 RecordedSuccess=988
```

审计 SHA-256：`F7104482AC214BE85A75042D75F25B258F0C843BBA321D6C09AD8BB647237CAB`。

## 9. Changed-file regression gate

为 execution header/cpp、旧 execution tests 与新 arc execution tests 增加 `ThrownWeaponArcExecution` 映射。所需证据为：

- `Shanmen.0_0_10.CombatRuntime.ThrownWeaponArcExecution`；
- `Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc`；
- `Shanmen.0_0_10.CombatRuntime.ThrownWeapon`；
- `Shanmen.0_0_10.CombatCore`；
- 通用 `Shanmen.0_0_10.CombatRuntime`。

真实 gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=5 Logs=8
```

- gate SHA-256：`FCF7083CC7B474334219CF44863EFECD25E66EFE1E45F0E71D1775854AB00430`；
- self-test 新增完整正例和“只有 arc-execution exact 不足”的失败关闭反例；
- self-test：`311/311 PASS`；
- self-test SHA-256：`FA2080DEDFF6839EFC49BCAE8B237513200368D02733E109492682D5E1B56016`。

## 10. 静态边界与构建

生产 execution header/cpp 扫描禁止 `demo_map`、`UWorld`、`AActor`、ProjectileMovement、spawn/destroy、Enhanced Input、Tick/Timer 和 RNG：

```text
BOUNDARY_SCAN: PASS Files=2 Matches=0
```

boundary SHA-256：`E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`。`git diff --check` native 0。

最终构建：

- Game：45 actions / 70.70s / native 0，log SHA-256 `B3849478C6D0FAFE6DD52F0098E1094EEE0FACDA3A07F51CCBEDE0C52D615067`；
- Editor：up to date / 1.09s / native 0，log SHA-256 `A332FB7ED032418D893D72E30317E335505D27D1D1121382D45C7BDD59F182D8`。

产物：

- `demo_map.exe`：357,071,360 bytes / `094DFAB8318042489690286C0CD6967092A01F98DB5906FB55CFE7F8EC8B5474`；
- `UnrealEditor-ShanmenCombatRuntime.dll`：2,012,672 bytes / `C26F2DAE275E874C3D3F58D44FE86ECFD5D864DC6A54FCDB9DC92126F0AE9B6B`。

## 11. 长时全量观察

全量从 canonical command 到 native terminal 约 36 分钟。约 650–707 项期间进入既有 Sword Rhythm retry/checkpoint/envelope 长时单例；进程始终 Responding，CPU 累计持续增长，并逐项输出 Success。最终 845/845，不曾重启、跳过或用部分结果替代完成证据。

## 12. P/F 边界与后续判断

本轮没有运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P20.1 只完成 Arc plan 到唯一 thrown execution 的 runtime seam。下一 P-stage 可让现有 World delivery/projectile 消费统一 receipt 的 initial velocity 与 gravity，同时保持 P7 的 durable item commit gate。实际滚轮手感、可视轨迹、场景越障和命中体验仍属于后续明确的产品/F 阶段；高阶辅助寻路仍未实现。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-1-thrown-weapon-arc-execution>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-1-thrown-weapon-arc-execution/Docs/Report/Dev.D.UE.0.0.10.P20.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-1-thrown-weapon-arc-execution/Docs/Log/Dev.D.UE.0.0.10.P20.1.r0_log.md>
