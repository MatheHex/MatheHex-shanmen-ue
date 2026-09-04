# Dev.D.UE.0.0.10.P20.4.r0 Development Log

## 1. 目标与基线

- 基线：`db442bf49e51eb06109b9291feff387e73b37a39`（P20.3）；
- 分支：`agent/0.0.10-p20-4-thrown-weapon-arc-run-command`；
- 目标：让已有 RunCommand 接受明确 Arc plan，同时复用唯一物品事务、Host 和幂等/恢复账本；
- 约束：不接 controller/session/lifecycle/input/UI，不建立第二 Actor、库存、伤害、碰撞、计划或回放权威。

## 2. 接入前审计

P20.3 已让 RunHost 正确发布 Arc flight，但 `Fdemo_mapShanmenThrownWeaponRunCommandIntent` 仍硬编码 Straight action，并只携带 origin、normalized aim direction 与 maximum distance。Router 也无条件调用 Straight Host API。

Arc planner 已提供可重算校验的 `FShanmenThrownWeaponArcPlan`：其 `PlanId` 绑定 request identity 与完整求解结果，`Matches` 同时核对 request、初速度、重力、apex、速度和飞行时间。结论是命令层直接冻结该计划，不复制或重算其身份。

## 3. 判别 intent

在原 intent 内新增 `TrajectoryKind` 与 `ArcPlan`，没有建立第二 command class 或 Router。

原 `TryCapture` 明确写入 Straight；新 `TryCaptureArc` 明确写入 BallisticArc。共同字段仍冻结 correlation、action、definition 与 offense。

`IsValid` 先验证 Run、owner、item、definition 和准备/局内 inventory correlation，再执行互斥轨迹规则：

- Straight 必须使用 Straight action、规范化方向、有限正距离，且不存在有效 Arc plan；
- Arc 必须使用 Arc action、有效 plan、空直线载荷，且 plan request 的 action snapshot 与命令 action 逐字段一致；
- Invalid 或其它组合失败关闭。

## 4. 身份与匹配

Intent key 继续等于 Action ActivationId。共同 `Matches` 比较 correlation、完整 action、definition 和 technique power，然后：

- Straight 精确比较 origin、normalized direction、maximum distance；
- Arc 调用 plan 自身的确定性 `Matches`。

同一 activation 的不同 Arc target 会生成不同 request/plan identity，并在任何 item I/O 前触发 `IntentIdConflict`。

## 5. Router 接线

Router 在 intent 有效性、replay/conflict、source、Host empty、Action Startup、execution、durable prepare 与 Action Active commit point上保持一条共同管线。

唯一分支位于 Host 调用：Straight 使用旧 API，Arc 使用 P20.3 `TrySpawnAndLaunchPreparedArc`。后续 success、post-commit recovery、pre-launch cancel、terminal record 与 cancellation recovery 代码完全共享。

成功诊断改为 trajectory-neutral 的 `explicit flight committed exactly once`，避免 Arc 成功时仍声称 straight。

## 6. 自动化扩展

`IntentContract` 增加：

1. Arc intent 只携带有效 plan，直线槽为空；
2. Arc action 无法进入 Straight capture；
3. Arc plan 无法重新绑定到另一 action snapshot。

新增 `ArcApplyReplayConflict`：

1. 在 transient `UWorld` 和真实持久 item authority 上路由 Arc；
2. 验证 prepare/commit 后 Quantity `3 → 2`；
3. 验证 Host lifetime 为 `ArcFlightTime`，distance 为 0，Actor lifespan 等于 plan flight time；
4. 精确重放验证 authority snapshot、Actor 与 ledger 数量不变；
5. 同 activation 异 target plan 验证冲突且无变更。

新增 `ArcPreLaunchCancel`：null projectile class 触发 pre-launch cancel，Quantity `3 → 3`；随后用有效 class 重放仍不提交或发射。

## 7. 首次结果

首次 Editor 编译：35 actions / 58.32s / succeeded。首次 RunCommand exact：6/6 / native 0。没有 production 或 test failure，也没有需要删除或覆盖的失败日志。

最终 Editor 为 up to date；Game 重编 34 actions 成功。最终代码随后通过 exact、0.0.10 全量与四组旧版兼容回归。

## 8. Changed-file regression gate

最终改动 3 个源码/测试文件，命中现有 `ThrownWeaponRunCommandRouter` 规则，无需修改映射表。要求组：

- `Shanmen.0_0_10.Product.ThrownWeaponRunCommand`；
- `Shanmen.0_0_10.Product.ThrownWeaponRunHost`；
- `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery`；
- `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter`；
- `Shanmen.0_0_10.Product.CombatRunCoordinator`；
- `Shanmen.0_0_10.Items`；
- `Shanmen.0_0_10.WorldGameplay`；
- `Shanmen.0_0_10.CombatRuntime`。

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=8 Logs=2
```

gate SHA-256：`845D782F21C429C95155EAE934E8CC587CE5C977890D94CC413032B095F4093A`。

## 9. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `runcommand_final.log` | 6/0 | `63E69FC180E75C09D15C7BD2128FD32E27E166898770EA2295FB2BE2F2BB98D3` |
| `full_0_0_10_final.log` | 851/0 | `1A190F906CB6B7863043A7F59A7F3199C99CDEB65A71A9A9657EF6CF9204756A` |
| `legacy_v3_attributes_final.log` | 4/0 | `F02E3EC564D811773643CF856554B75119BF929D0D959D92FAF82C0FEA8623B3` |
| `legacy_enemy_skill_final.log` | 44/0 | `29330046C05C77E30844241428AD4E9996AA52F44FFADB94B25DE5DCF477C6C3` |
| `legacy_v2_ranged_final.log` | 22/0 | `4FEAA10891E3E1799FCDF8BFDB2C9762C4B6862EE1450A862D79DAFF66537977` |
| `legacy_item_armor_final.log` | 46/0 | `B71A7E730E9331884E196BFA4D170E7CD27FF4EDFC4EAECDC60745754447D324` |

全量 canonical command 从发现 851 项到 native terminal 约 34 分 15 秒。约 647–710 项进入既有 Sword Rhythm retry/checkpoint/manifest 长时单例；日志持续产生开始/完成记录，所有项目最终成功，没有重启、裁剪或用部分结果代替。

证据审计：`PASS Logs=6 RecordedSuccess=973`，SHA-256 `4FA08F18D3CC474EE977B502ADFACE6C5065AE77A4877A3776686F3C2532BA0B`。

## 10. 静态边界、构建与产物

production boundary：

```text
BOUNDARY_SCAN: PASS Files=2 Matches=0
```

- boundary SHA-256：`E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`；
- `git diff --cached --check`：最终 5 文件 PASS / native 0，SHA-256 `DEF32071D1819B6D5C86A1649832413D9040EA5E3385968E9078C1FAECAAC2A3`；
- final Editor build：up to date / native 0，SHA-256 `7F0EAA4DD7BE40B44CD16106EB1C95E2FEA622B2F81264007BB7ACB462F08E66`；
- final Game build：34 actions / 62.85s / native 0，SHA-256 `122121D8E0400DD4FE4812BFC461965602D6F51E4005A4CD96549FFEB97C2EF1`。

产物：

- `Binaries/Win64/demo_map.exe`：357,111,808 bytes / `AF874114D37578B502E9924E419FCBB8B1AA080DCA6D3F74D233EAC4BC9CEA1F`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,765,504 bytes / `8FFBD1BC5127936069CA3FACD870CB58F7BEB1CEA888C7AE1078AA889CB5A6D2`。

## 11. P/F 边界与后续判断

没有运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

现有 ProductController selection 仍只有 aim direction 与 maximum distance，并只调用 Straight `TryCapture`。下一阶段应给该既有控制器增加 typed Arc selection/command seam，要求调用方提供已验证 plan 并调用本轮 Router；session、lifecycle、玩家滚轮弧度输入和视觉反馈继续后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-4-thrown-weapon-arc-run-command>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-4-thrown-weapon-arc-run-command/Docs/Report/Dev.D.UE.0.0.10.P20.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-4-thrown-weapon-arc-run-command/Docs/Log/Dev.D.UE.0.0.10.P20.4.r0_log.md>
