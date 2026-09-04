# Dev.D.UE.0.0.10.P20.4.r0 Report

## 1. 结论

P20.4 已让既有 `Fdemo_mapShanmenThrownWeaponRunCommandRouter` 接受显式的 Straight 或 BallisticArc 命令，同时只保留一套 Action、物品 Prepare/Commit/Cancel、Host 发布、幂等回放与恢复权威。

Arc 命令直接冻结 P20.0 纯求解器产生的不可变 `FShanmenThrownWeaponArcPlan`；Router 不根据直线参数猜测目标、弧高、重力或飞行时间，也不重算计划身份。旧 Straight 捕获 API 与语义保持不变。

本轮只关闭 RunCommand seam。产品控制器、会话、生命周期和输入适配器仍只构造 Straight 命令，因此不声称玩家已能通过场景输入发射弧线暗器。

本轮为 P 阶段。没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`db442bf49e51eb06109b9291feff387e73b37a39`（P20.3）；
- 分支：`agent/0.0.10-p20-4-thrown-weapon-arc-run-command`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 显式命令轨迹契约

新增 `Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind`：`Invalid`、`Straight`、`BallisticArc`。

- 原 `TryCapture` 只生成 Straight intent，继续冻结规范化方向与有限正最大距离；
- 新 `TryCaptureArc` 只接收有效 Arc action、匹配 definition 与已求解 Arc plan；
- Arc intent 的直线 origin/direction/distance 槽保持空值，Straight intent 不携带有效 Arc plan；
- `IsValid` 按判别类型失败关闭，并要求 Arc plan 内的完整 action snapshot 与 command action 精确一致；
- `Matches` 先比较共同身份、内容与 offense，再按轨迹比较直线载荷或 Arc plan 的确定性身份及完整求解结果。

因此不能把 Arc action 塞进旧 Straight 入口，不能把一个 Arc plan 重新绑定到另一 activation，也不能用相同 ActivationId 携带不同目标或计划。

## 4. 单一事务与发布路径

Router 的共同流程未分叉：Run/source/router gate → Action Startup → execution creation → durable item prepare → Action Active commit point。

只在 Host 启动处根据 intent 的显式轨迹选择：

- Straight → `TrySpawnAndLaunchPrepared`；
- BallisticArc → `TrySpawnAndLaunchPreparedArc`。

两者之后继续共享同一结果判定、terminal record、pre-launch cancel 和 cancellation recovery。Arc 没有第二套 inventory I/O、Actor、execution、ledger 或恢复路径。

## 5. 幂等、冲突与取消

命令账本仍以 Action `ActivationId` 为唯一 intent key。

- 完全相同 Arc plan 的重放返回原 terminal receipts，不再准备/提交物品，也不生成第二 Actor；
- 相同 ActivationId 携带不同目标所生成的 Arc plan 会被 `IntentIdConflict` 拒绝，authority snapshot 与当前 Host 不变；
- Arc spawn 在持久 commit 前失败时，走原 `CancelBeforeLaunch`，Quantity 从准备状态恢复，失败终态可无副作用重放；
- 原 Straight recovery 测试继续覆盖 cancellation persistence failure 与显式恢复，证明扩展没有另起恢复账本。

## 6. 自动化结果

新增 2 项完整产品测试，并扩展原 intent contract：

- `ArcApplyReplayConflict`：真实 transient `UWorld`、`UGameInstance` authority 与 projectile，验证 Quantity `3 → 2`、Arc flight-time lifetime、精确重放和同 ID 异 plan 冲突；
- `ArcPreLaunchCancel`：验证无 projectile class 时持久取消 `3 → 3`，随后重放不扣物品、不发射；
- `IntentContract`：新增 Arc-only payload、Arc/straight action 隔离、plan/action 不可重绑断言。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `runcommand_final.log` | `Product.ThrownWeaponRunCommand` | 6/0 | `63E69FC180E75C09D15C7BD2128FD32E27E166898770EA2295FB2BE2F2BB98D3` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | 851/0 | `1A190F906CB6B7863043A7F59A7F3199C99CDEB65A71A9A9657EF6CF9204756A` |
| `legacy_v3_attributes_final.log` | `demo_map.V3.Attributes` | 4/0 | `F02E3EC564D811773643CF856554B75119BF929D0D959D92FAF82C0FEA8623B3` |
| `legacy_enemy_skill_final.log` | `demo_map.EnemySkillFramework` | 44/0 | `29330046C05C77E30844241428AD4E9996AA52F44FFADB94B25DE5DCF477C6C3` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | 22/0 | `4FEAA10891E3E1799FCDF8BFDB2C9762C4B6862EE1450A862D79DAFF66537977` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | 46/0 | `B71A7E730E9331884E196BFA4D170E7CD27FF4EDFC4EAECDC60745754447D324` |

0.0.10 全量从 P20.3 的 849 增至 851。证据审计确认 6 份日志各只有一个 canonical `RunTests` command、Fail 0、一个 native terminal、Fatal/Unhandled/Ensure 0：`PASS Logs=6 RecordedSuccess=973`，审计 SHA-256 `4FA08F18D3CC474EE977B502ADFACE6C5065AE77A4877A3776686F3C2532BA0B`。

## 7. 改动门禁与静态边界

最终 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=8 Logs=2
```

8 个要求组为 RunCommand、RunHost、WorldDelivery、ItemAdapter、CombatRunCoordinator、Items、WorldGameplay 与 CombatRuntime。gate SHA-256：`845D782F21C429C95155EAE934E8CC587CE5C977890D94CC413032B095F4093A`。

对 2 个 production 文件扫描 direct damage、direct inventory mutation、RNG、trace/sweep ownership、输入、PlayerController 与 GameMode 接入：`PASS Files=2 Matches=0`，SHA-256 `E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`。

`git diff --cached --check` 对最终 5 个提交文件原生通过，记录 SHA-256 `DEF32071D1819B6D5C86A1649832413D9040EA5E3385968E9078C1FAECAAC2A3`。首次 Editor 编译与首次 RunCommand exact 均成功，没有失败测试需要隐藏或重写。

## 8. 构建与产物

- final Editor：up to date / native 0，SHA-256 `7F0EAA4DD7BE40B44CD16106EB1C95E2FEA622B2F81264007BB7ACB462F08E66`；
- final Game：34 actions / 62.85s / native 0，SHA-256 `122121D8E0400DD4FE4812BFC461965602D6F51E4005A4CD96549FFEB97C2EF1`。

产物：

- `Binaries/Win64/demo_map.exe`：357,111,808 bytes，SHA-256 `AF874114D37578B502E9924E419FCBB8B1AA080DCA6D3F74D233EAC4BC9CEA1F`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,765,504 bytes，SHA-256 `8FFBD1BC5127936069CA3FACD870CB58F7BEB1CEA888C7AE1078AA889CB5A6D2`。

## 9. P/F 边界与下一步

P20.4 证明的是“已有 Arc action 与 solver plan 如何通过唯一 RunCommand 事务、幂等账本和 RunHost 发布”。它没有选择目标、生成 plan，也没有改变产品控制器、会话、生命周期或输入语义。

建议 P20.5 扩展既有 ProductController 的 typed selection/command contract，使调用方显式提供 Arc plan 并复用本轮 Router；仍不接 session、lifecycle、滚轮弧度输入或视觉反馈。不得让控制器从旧 `AimDirection + MaximumDistance` 隐式推断 Arc。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-4-thrown-weapon-arc-run-command>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-4-thrown-weapon-arc-run-command/Docs/Report/Dev.D.UE.0.0.10.P20.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-4-thrown-weapon-arc-run-command/Docs/Log/Dev.D.UE.0.0.10.P20.4.r0_log.md>
