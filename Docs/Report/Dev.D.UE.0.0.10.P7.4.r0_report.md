# Dev.D.UE.0.0.10.P7.4.r0 Report

## 1. 结论

P7.4 已完成一次性直线暗器的 product Run command router，结论为 **PASS**。

本轮把“冻结 action/暗器参数 → P7.1 durable Quantity prepare → action commit point → P7.3 spawn-and-launch”收口为一个 command/result。命令身份直接使用 action `ActivationId`，无法通过替换外部 IntentId 绕过幂等；成功、发射前已取消失败、取消失败需恢复三类 durable terminal 都由 router 记录。exact replay 只返回原 receipt，不重复库存 I/O、不生成第二个 Actor。

输入键位、物品选择策略、角色属性采样、视觉资产、发射动画、弧线、追踪、转向与召回仍未接入。

## 2. 冻结命令契约

新增 `Fdemo_mapShanmenThrownWeaponRunCommandIntent`：

- 保存完整 `RunCorrelation`、只读 action、暗器 definition、offense snapshot、origin、canonical aim direction 与 maximum distance；
- `IntentId` 固定等于 action `ActivationId`，不再接受可独立伪造的第二身份；
- capture 时规范化方向，并验证 active Run、owner、exact source item、canonical thrown action 与 Run inventory membership；
- `Matches` 比较 correlation、action、definition、offense、origin、direction 与 range 的完整 payload；
- World、projectile class、authority、coordinator 和 source Actor 属于产品执行上下文，不进入输入 intent，也不由输入层拥有。

同一 ActivationId 搭配不同方向或其它 payload 会返回 `IntentIdConflict`，不执行 Prepare、Spawn 或 vitality 操作。

## 3. 一次性执行与重放

`Fdemo_mapShanmenThrownWeaponRunCommandRouter::TryRoute` 固定顺序如下：

1. 验证 coordinator、Run、source entity、router 与 empty host；
2. 创建 action runtime 与纯 `FShanmenThrownWeaponExecution`；
3. 通过 P7.1 `PrepareActiveRun` 持久准备 exact active-Run Quantity；
4. action 从 `Startup` 跨过唯一 commit point 到 `Active`；
5. 调用 P7.3 `TrySpawnAndLaunchPrepared`，由既有 P7.2 stage/commit/publication 完成 durable consume 与 Actor flight；
6. 记录完整 startup/active/preparation/host-start/launch evidence。

成功结果要求 item finalize 为 commit、Host 已采用 in-flight projectile、LaunchId 一致。router 随后可在 Host active 或 terminal 时返回 exact replay；重放不依赖 Host 再次 empty，也不执行任何 effectful step。

## 4. 失败关闭与恢复

Prepare 成功之后的失败不会遗留无主 pending intent：

- action commit 或 spawn/stage/authority commit 在外部不可逆点之前失败时，router 自动调用 P7.1 `CancelBeforeLaunch`；
- cancellation durable 成功后记录 `LaunchRejectedCancelled`，同一 action 永久不能借 replay 重新发射；
- cancellation 持久化失败时记录 `RecoveryRequired`，阻止重新 Prepare/Spawn；
- `TryRecoverCancellation` 只重试该 exact pre-launch cancellation，绝不发射；
- 如果 item commit 已成功但 Host adoption 失败，保留 post-commit `RecoveryRequired`，明确拒绝用 cancellation 伪装回滚或重新发射。

恢复自动化真实注入 `WriteTemp` 失败：首次 cancellation 回滚且 pending intent 保持不变；解除注入后只补做 cancellation，authority revision 前进一次；后续 route 为无副作用 replay。

## 5. 自动化证据

最终 `-Unattended -NullRHI` 自动化全部通过；两份日志都只有一个实际 RunTests、一个 queue-empty、Fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.ThrownWeaponRunCommand` | 4 | 0 | `3F6DFE3DFF3FB566B5A6E0362762D0E1A658F6B66F66281B0D37896A5D080159` |
| `Shanmen.0_0_10` | 192 | 0 | `791A9FCEF7A511EC759F345CB1D21FA01BBA7E3A1BBF21DDF5281D9BC94C0F06` |

完整 suite 从 P7.3 的 188 增至 192。四项新增测试覆盖 immutable intent 与冲突、成功执行/重放、普通 pre-launch cancel、持久化失败与显式 cancellation recovery。

## 6. 改动—回归与静态门禁

- 新增 `ThrownWeaponRunCommandRouter` 精确路径映射；
- `REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=8 Logs=2`；
- mapping self-test：`20/20 PASS`，同时证明 full suite 可覆盖全部 seam、coordinator-only 日志不能伪装覆盖；
- `git diff --check`：native exit `0`；
- 新 router 无 `EKeys`、`UInputAction`、`ApplyDamage`、legacy `demo_mapItemSubsystem`、RNG、直接 vitality commit 或 `ConsumePreparedRunItemDurable`；
- effectful 调用仅委托给 P7.1 `PrepareActiveRun/CancelBeforeLaunch` 与 P7.3 `TrySpawnAndLaunchPrepared`；
- 未修改 schema、Build.cs、GameplayTags、Content、GameMode、Profile、CodeB 或既有 combat/item authority。

## 7. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor integration：`5/5`，Succeeded，native exit `0`，`30.49s`；
- Game：`4/4`，Succeeded，native exit `0`，`23.63s`；
- Editor product DLL UTC：`2026-08-29T11:32:56.3020808Z`；
- Game executable UTC：`2026-08-29T11:35:06.2722338Z`。

本轮没有源码编译失败或自动化失败。两次测试进程 native exit 都为 `0`；discovery 前的项目既有 `Condition failed` 启动噪声不属于 Controller test result，192 个最终结果全部 Success。

## 8. 修改范围、P/F 边界与下一阶段

本轮生产范围仅新增 thrown weapon Run command intent/router；测试新增四项 effectful product contract；流程范围只增加该路径的回归映射与正反 self-test。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 和用户文件未纳入 stage。

只执行 P 阶段源码、静态检查、无头 Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

P7.5 建议建立 thrown weapon product controller/capture facade：由 Run owner 持有 activation sequence，从已选择 exact item 与冻结角色数值生成 action/definition/offense，再提交本轮 router。具体键位、UI、动画和表现继续后置，使输入层最终只提交选择意图。

## 9. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-4-thrown-weapon-command-router/Docs/Report/Dev.D.UE.0.0.10.P7.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-4-thrown-weapon-command-router/Docs/Log/Dev.D.UE.0.0.10.P7.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-4-thrown-weapon-command-router>
