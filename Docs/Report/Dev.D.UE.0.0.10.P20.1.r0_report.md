# Dev.D.UE.0.0.10.P20.1.r0 Report

## 1. 结论

P20.1 已把 P20.0 的中阶暗器弧线计划接入既有 `FShanmenThrownWeaponExecution`，并保留 P7 初阶直线投掷的原行为与 API。

弧线没有另建伤害执行器、命中账本或生命周期。直线与弧线现在共享同一个 exact item、Action、Detector emission、Impact ledger、Defense resolver 和 spent-state 权威；两者只在不可变 launch receipt 中携带不同的运动证明。

本轮只关闭 CombatRuntime 组合边界。没有修改 Actor、World adapter、ProjectileMovement、产品路由、鼠标滚轮、碰撞或库存；因此不声称场景中的暗器已经按弧线飞行。

本轮为 P 阶段。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`e6db6f549d92f3206e1df0dffa02bddec4650381`（P20.0）；
- 分支：`agent/0.0.10-p20-1-thrown-weapon-arc-execution`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 设计决策

审计发现 P20.0 的 `Combat.Action.ThrownWeapon.Arc01` 不能进入 P7 只接受 `Combat.Action.ThrownWeapon.Straight01` 的执行权威。若为弧线新建平行 execution，会复制伤害公式、命中序号、幂等账本和终止逻辑，形成第二套暗器权威。

本轮改为扩展原执行器：

1. `FShanmenThrownWeaponDefinition` 只接受明确的 `Straight01` 或 `Arc01`，继续拒绝 generic projectile；
2. `EShanmenThrownWeaponTrajectoryKind` 显式区分 `Straight` 与 `BallisticArc`，没有增加 bool soup；
3. 原 `TryLaunchStraight` 保持不变，并明确拒绝 Arc action；
4. 新 `TryLaunchArc` 消费 P20.0 的完整自校验 plan；
5. 发射后的 emission、Impact 与终止路径完全复用原实现。

## 4. 统一动作与速度边界

`CanonicalActionDefinitionId()` 仍是旧直线动作的兼容别名；新增 `StraightActionDefinitionId()` 与 `ArcActionDefinitionId()`，让两条操作边界可被明确审计。

定义中的 `LaunchSpeed` 对直线仍表示固定发射速度；对弧线表示 execution 允许的最大初速度。弧线发射同时检查：

- action runtime 已进入可发射阶段；
- plan 自身有效且与 execution 的完整 frozen action 相同；
- plan request 的最大速度 envelope 没有超过 definition；
- 求解后的实际初速度没有超过 definition；
- 初速度可规范化为有限的单位方向。

因此调用方不能用一个更宽松的 planning envelope 绕过内容侧速度上限。

## 5. 轨迹收据与确定性身份

`FShanmenThrownWeaponLaunchReceipt` 现在携带轨迹种类，并提供统一读取：

- `GetInitialVelocity()`；
- `GetGravityAcceleration()`；
- `GetFlightTimeSeconds()`；
- `GetArcPlan()`。

直线收据继续保存 origin、canonical direction 与固定 speed，gravity/time 返回零。弧线收据直接嵌入 P20.0 的 `FShanmenThrownWeaponArcPlan`，不复制弹道公式或另存一份可漂移的几何真值。

弧线 launch identity 使用命名空间 `Shanmen.ThrownWeapon.ArcLaunch.r1`，绑定完整 action provenance 与 `PlanId`。相同计划可幂等重放；另一顶点、另一内容摘要或另一 action 的计划不能在飞行中改道。

## 6. Impact 与生命周期连续性

弧线发射成功后进入原 `InFlight` 状态，继续使用：

- `TryBeginEmission` / `TryEndEmission`；
- `TryResolveCandidate`；
- `FShanmenImpactLedger`；
- `FShanmenDefenseResolver`；
- `TryFinishFlight` 与 termination cleanup。

测试证明弧线 contact 仍按原公式得到 `12 + 40 * 0.3 = 24` 原始伤害，并由同一 ledger 拒绝重复 callback。弧线没有自己的伤害公式、target policy 或第二份 accepted-impact 状态。

## 7. 自动化结果

新增 exact tests 4 项：

- `ActionBoundary`；
- `LaunchAndReplay`；
- `IdentityAndEnvelopeFences`；
- `SharedImpactAuthority`。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | `CombatRuntime.ThrownWeaponArcExecution` | 4/0 | `F2EC5B9B4347827B45062B26075E367E8EDBAEE8DD351F928ADE2D76E702E592` |
| `automation_thrown_runtime.log` | `CombatRuntime.ThrownWeapon` | 14/0 | `02A82DEF1DE7ABDBA0F1480F8BCCAE06B2B727E4901FC5A9393AD84BF3F1B0F2` |
| `automation_combat_core.log` | `CombatCore` | 9/0 | `684F7F6F4D97AEEB8D4D63CF7CD270363FA16DFDF181DEEDD35A8C48E50381C3` |
| `automation_full.log` | `Shanmen.0_0_10` | 845/0 | `9610AC67E1851D2CD9C38379702881888F37043A8BCE64339A44FA74CD8FB24C` |
| `automation_legacy_attributes.log` | `demo_map.V3.Attributes` | 4/0 | `8CDA304A3FD9587F2A709C8F18303DA45737446943403AAFE51594BB0CD4EDDB` |
| `automation_legacy_enemy.log` | `demo_map.EnemySkillFramework` | 44/0 | `47D474EBFD6A3B2BF8E071DE0369F5E41EDA1ADDFB69795767E578DF64540325` |
| `automation_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | `69106D10935C758771156BFF267A26489F84275731F8E523ADEEBF40AA3D4C52` |
| `automation_legacy_item_use_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `5340D952C4582CC1F0CE75FF149ECDB9FCDC6CD7D26BBF5DD01D63D7C174AB4E` |

全量由 P20.0 的 841 增加到 845。证据审计确认 8 份日志均只有一个 canonical `RunTests` command、精确预期 Success、Fail 0、一个 native terminal、Fatal/Unhandled/Ensure 0：`PASS Logs=8 RecordedSuccess=988`，SHA-256 `F7104482AC214BE85A75042D75F25B258F0C843BBA321D6C09AD8BB647237CAB`。

## 8. 改动推导门禁、边界与构建

新增 `ThrownWeaponArcExecution` regression mapping，要求弧线 execution、P20 planner、旧直线执行、CombatCore 与宽域 CombatRuntime 证据。真实 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=5 Logs=8
```

- gate SHA-256：`FCF7083CC7B474334219CF44863EFECD25E66EFE1E45F0E71D1775854AB00430`；
- mapping self-test：`311/311 PASS`，SHA-256 `FA2080DEDFF6839EFC49BCAE8B237513200368D02733E109492682D5E1B56016`；
- production boundary scan：`PASS Files=2 Matches=0`，SHA-256 `E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`；
- `git diff --check`：native 0。

构建：

- Game final：45 actions / 70.70s / native 0，SHA-256 `B3849478C6D0FAFE6DD52F0098E1094EEE0FACDA3A07F51CCBEDE0C52D615067`；
- Editor final：up to date / 1.09s / native 0，SHA-256 `A332FB7ED032418D893D72E30317E335505D27D1D1121382D45C7BDD59F182D8`。

产物：

- `demo_map.exe`：357,071,360 bytes，SHA-256 `094DFAB8318042489690286C0CD6967092A01F98DB5906FB55CFE7F8EC8B5474`；
- `UnrealEditor-ShanmenCombatRuntime.dll`：2,012,672 bytes，SHA-256 `C26F2DAE275E874C3D3F58D44FE86ECFD5D864DC6A54FCDB9DC92126F0AE9B6B`。

首次 Editor 编译、4 项 exact、14 项暗器族及全量均一次通过；本轮没有需要保留的失败日志。全量约 36 分钟，既有 Sword Rhythm checkpoint/envelope 长时段始终响应并持续产生成功结果，未重启、未裁剪。

## 9. P/F 边界与下一步

P20.1 关闭的是“一个合法弧线计划如何进入唯一暗器执行权威，并继续使用原命中与伤害链”。它没有声称产品 Actor 已消费 gravity/flight-time，也没有声称玩家能够用滚轮选择弧度。

建议 P20.2 让既有 World delivery/projectile 读取统一 launch receipt：直线维持零重力，弧线使用 plan 的 initial velocity 与 gravity；继续保留 item reserve/commit 的 durable gate。产品输入、轨迹可视反馈、场景越障与手感验收仍应由后续明确阶段完成。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-1-thrown-weapon-arc-execution>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-1-thrown-weapon-arc-execution/Docs/Report/Dev.D.UE.0.0.10.P20.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-1-thrown-weapon-arc-execution/Docs/Log/Dev.D.UE.0.0.10.P20.1.r0_log.md>
