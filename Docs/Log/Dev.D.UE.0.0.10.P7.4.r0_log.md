# Dev.D.UE.0.0.10.P7.4.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.4.r0`；
- 基线提交：`47b358282229d7e59008924a7be813dac1a12fd5`（P7.3）；
- 分支：`agent/0.0.10-p7-4-thrown-weapon-command-router`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与关键决策

P7.1 已提供 exact active-Run Quantity prepare/finalize，P7.3 已提供真实 World spawn、durable launch 和 Host lifecycle，但未来输入若依次调用两者，仍会直接拥有库存事务与 Actor 生命周期。

P7.4 因此增加 Run command router。Intent 保存冻结产品语义，执行上下文保存 World/authority/Host。最重要的身份决策是不用额外随机或调用方 IntentId：`IntentId == Action.ActivationId`。P7.1 本来就以 ActivationId 作为 durable Quantity intent；两者统一后，同一个物理 action 无法换身份重复生成或重复消费。

## Router 执行顺序

`TryRoute` 先处理 exact replay 与 payload conflict，再检查 transient source/host。新执行依次：

1. `FShanmenActionOrchestrator::TryStart`；
2. `FShanmenThrownWeaponExecution::TryCreate`；
3. P7.1 `PrepareActiveRun`；
4. action `Startup -> Active`；
5. P7.3 `TrySpawnAndLaunchPrepared`；
6. 保存 terminal record。

这样 exact replay 即使发生在 Host in-flight 时也直接返回原结果，不再要求 Host empty，不重复任何 I/O 或 Actor spawn。payload conflict 在 transient context 检查前失败关闭。

## Saga 失败窗口

Prepare 成功、launch 尚未 durable commit 的失败统一执行 `CancelBeforeLaunch`。成功 cancellation 记为永久 terminal；未来用有效 projectile class 重放仍返回取消结果。

若 cancellation 写盘失败，router 保存 `RecoveryRequired`。`TryRecoverCancellation` 只允许 exact payload，且只重试 cancellation。post-commit adoption failure 不允许借该入口取消已经消费的 Quantity，返回 `RecoveryNotApplicable`，保留原 durable evidence 供更高层恢复。

## 自动化增量

- `IntentContract`：方向 canonicalize、ActivationId 统一身份、同身份不同 payload 冲突、非法 range 拒绝；
- `ApplyReplayConflict`：真实 product authority + active Run + GamePreview World，单命令 prepare/commit/spawn/adopt，exact replay authority snapshot 与 Actor identity 不变；
- `PreLaunchCancel`：invalid projectile class 触发真实 prepare 后 durable cancel，resource `3 -> 3`，后续 valid-class route 仍不能重发；
- `CancellationRecovery`：先预备 exact intent，再对 cancellation 注入 `WriteTemp` 失败；状态保持 pending，解除注入后 recovery 只补 cancellation，最终 replay 无副作用。

## 最终自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P7.4_Targeted.log` | `Shanmen.0_0_10.Product.ThrownWeaponRunCommand` | 4 | 0 | 0 | `3F6DFE3DFF3FB566B5A6E0362762D0E1A658F6B66F66281B0D37896A5D080159` |
| `P7.4_Full.log` | `Shanmen.0_0_10` | 192 | 0 | 0 | `791A9FCEF7A511EC759F345CB1D21FA01BBA7E3A1BBF21DDF5281D9BC94C0F06` |

两份日志均有一个 RunTests、一个 queue-empty、Fail `0`、fatal/unhandled/ensure marker `0`。Automation discovery 前保留项目既有 `Condition failed` 启动噪声，但目标 ControllerResults 为 `4/4` 和 `192/192` Success。

## Changed-file gate

新增 `ThrownWeaponRunCommandRouter` 映射，要求 router、RunHost、World delivery、item adapter、coordinator、Items、WorldGameplay、CombatRuntime 八组。full suite 作为父组覆盖全部子组，定向日志额外证明新组确实发现并执行四项测试。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=8 Logs=2
SELF_TEST: PASS 20/20
```

正向 self-test 证明新路径可由 full suite 覆盖；反向 self-test 证明 coordinator-only evidence 必须失败。

## 构建时间线

统一使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor integration：`5/5`，Succeeded，exit `0`，`30.49s`；
- Game：`4/4`，Succeeded，exit `0`，`23.63s`。

没有失败构建或失败测试，无源码修复重跑。首次编译与首次定向测试均通过。

## 静态、范围与兼容性

- staged `git diff --check`：native exit `0`；
- 新生产文件无 EKeys/UInputAction、ApplyDamage、legacy item subsystem、RNG、直接 vitality commit 或直接 Run consumption；
- effectful authority 只经 P7.1/P7.3 既有边界；
- 未改 schema、Build.cs、GameplayTags、Content、输入、GameMode、Profile、CodeB 或已有 combat/item 实现；
- 0.0.10 full suite `192/192`；
- 长期未跟踪的 0.0.9B 与用户文件保持未跟踪且未 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook 或 Package。

## 下一阶段

P7.5 建立 thrown weapon product controller/capture facade，由 Run owner 管理 activation sequence，并从 exact selected item、冻结角色 offense 与产品 definition 生成本轮 intent。输入绑定、UI 与视觉表现继续后置。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-4-thrown-weapon-command-router/Docs/Report/Dev.D.UE.0.0.10.P7.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-4-thrown-weapon-command-router/Docs/Log/Dev.D.UE.0.0.10.P7.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-4-thrown-weapon-command-router>
