# Dev.D.UE.0.0.10.P7.6.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.6.r0`；
- 基线提交：`6be71d9c00ec9ccfc8b48db00494e092479aff6d`（P7.5）；
- 分支：`agent/0.0.10-p7-6-thrown-weapon-product-session`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与冻结决策

P7.5 已能把 exact item selection 变为 deterministic action，但调用方仍需自行取得 item GUID 和 combat stat。P7.6 新增 active-Run session owner，只接受 1-based 热栏槽位与轨迹；exact item 必须来自冻结 Run correlation，TechniquePower 必须来自 source Actor 的既有 `CaptureAttackPower`。

关键边界：

1. 不读取或写入 legacy `Udemo_mapItemSubsystem`；
2. 不建立第二个 hotbar cache；
3. 不接受调用方提供 item GUID、AttackPower、sequence 或 ActivationId；
4. 一次 SelectionId 只采样一次属性，重试永远复用；
5. Session 只拥有 Host/Router/Controller 的组合生命周期，真实 Quantity、Run、World 与 vitality authority 不变。

## 实现

新增文件：

- `demo_mapShanmenThrownWeaponProductSession.h`：180 行；
- `demo_mapShanmenThrownWeaponProductSession.cpp`：627 行；
- `demo_mapShanmenThrownWeaponProductSessionTests.cpp`：770 行。

主要流程：

1. `HotbarIntent::TryCapture` canonicalize selection、槽位和轨迹；
2. `SessionConfig::TryCapture` 以 P7.5 ProductCapture 验证 immutable definition/source tags；
3. `TryBegin` 复制 exact Run correlation，弱引用 source Actor；
4. `TrySubmitHotbar` 先处理 replay/conflict，再解析 frozen hotbar slot；
5. 新 selection 调用 `Fdemo_mapPlayerCombat::CaptureAttackPower` 一次并保存 ProductCapture；
6. `RouteCaptured` 委托 P7.5 controller，并按 transient/durable terminal 规则管理 Host reset；
7. `TryRecoverCancellation` 只委托 selection recovery；
8. `TryEnd` 拒绝 in-flight/recovery-required 状态，成功后原子清空组合状态。

`IsValid` 反向重算槽位—item 绑定、Run、轨迹、ProductCapture、Controller command 的 exact item 和 TechniquePower，并检查 Host/Router/Controller 都属于同一 Run。

## 自动化增量

- `ContractAndBinding`：intent/config canonicalization、幂等 Begin、空槽 no-op、binding end；
- `ResolveFreezeReplayConflict`：exact hotbar item、当前 AttackPower、Actor launch、重放无 I/O、属性不漂移、slot conflict 不消耗 sequence；
- `BusyRetryTerminalRollover`：第二个 selection 在 HostBusy 时冻结 sequence/stat，Host terminal 后复用原值；
- `RecoveryEndGate`：预准备 action 的取消持久化失败、session teardown gate、selection-only recovery。

## 首次失败与修复

### Editor integration

首次 Editor 编译 native exit `6`，`OtherCompilationError`。`Fdemo_mapShanmenThrownWeaponProductSession::TryRecoverCancellation` 调用 P7.5 controller 时漏传 Router；补齐既有 Router owner 后 Editor 编译成功。失败日志：

- `P7.6_EditorBuild_FirstAttempt.log`；
- `10.04s`；
- SHA-256 `DDFFBB649D29A407D703B6801EF66DAD87C503F99EE5B58419BD8BBCB4EEC67D`。

### Targeted automation

首次定向结果为 `3 Success / 1 Fail`，进程 native exit `0`，但 ControllerResults 按失败处理。恢复测试把 `WriteTemp` 注入放在首次 prepare 前，故障被 prepare 事务消费，后续 null-class cancellation 已无注入，因而错误地期待 recovery-required。

修复测试场景：先用一个 selection 占用 Host；第二个 selection 以 HostBusy 冻结 command；终止 Host 并显式预准备第二个 action；此时再注入 `WriteTemp`，确保失败发生在取消持久化。该路径与 P7.4/P7.5 已验证恢复语义一致，没有修改产品代码来迎合断言。

- `P7.6_Targeted_FirstFailure.log`；
- `3 Success / 1 Fail`；
- SHA-256 `E4076D3199F2E7499F9E775D2B4CEAC7AB1A418E7B635BB277B676F1EBF454A8`。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P7.6_Targeted.log` | `Shanmen.0_0_10.Product.ThrownWeaponProductSession` | 4 | 0 | 0 | `5902A582853237B1A2268C94F84BD8B5C3CF03C4462F498ACDF57AC9DCBBAAC0` |
| `P7.6_Full.log` | `Shanmen.0_0_10` | 200 | 0 | 0 | `B76A227158AFCACB4E8D931ADD080CF4A1C79DAFC4E651AEB05B5038C5751034` |
| `P7.6_ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `C2F0C0485D0575291A6C24E8B559C8086BBF482D81B0E9156239A5F0D2C91C98` |

每份 canonical 日志包含一个实际 RunTests、queue-empty、Fail `0`，且 fatal/unhandled/ensure marker 为 `0`。Automation discovery 前的非 Win64 SDK 提示是项目既有启动噪声，不影响 Win64 SDK VALID 或测试执行。

## Changed-file gate

新增 `ThrownWeaponProductSession` 映射，要求十一组证据：Session、ProductController、RunCommand、RunHost、WorldDelivery、ItemAdapter、CombatRunCoordinator、Items、WorldGameplay、CombatRuntime 与 `demo_map.ItemUseAndArmor`。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=3
SELF_TEST: PASS 24/24
```

正向 self-test 证明 full suite + legacy combat group 覆盖全部 seam；反向 self-test 证明只有 full suite、缺 `ItemUseAndArmor` 时必须失败。

## 构建时间线

统一使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| 构建 | Result | Native exit | 时间 | 日志 SHA-256 |
|---|---|---:|---:|---|
| Editor first attempt | Failed / OtherCompilationError | 6 | 10.04s | `DDFFBB649D29A407D703B6801EF66DAD87C503F99EE5B58419BD8BBCB4EEC67D` |
| Editor after source fix | Succeeded | 0 | 5.52s | `728115249ED5F9FF68B6CC3006BD948BCBD8020EE9FEB5AC15D2FED3FF9F6DF7` |
| Editor final | Succeeded | 0 | 5.74s | `21731682561B9BB871C509464BAE580AF8A151865C2C7F33E018918579FDC785` |
| Game final | Succeeded | 0 | 16.76s | `324C46211CF146A337023D76673C3A4AE4DB5339500A94F536D23333EF2F36EB` |

- Editor product DLL UTC：`2026-08-29T12:39:59Z`；
- Game executable UTC：`2026-08-29T12:42:18Z`。

## 静态、范围与兼容性

- regression map JSON parse：PASS；
- mapping self-test：`24/24 PASS`；
- boundary scan：PASS；
- working tree 与 staged `git diff --check`：native exit `0`；
- 无 input/UI、legacy item subsystem、ApplyDamage、RNG、直接 Run consumption 或 vitality write；
- 未改 schema、Build.cs、GameplayTags、Content、GameMode、Profile、CodeB 或旧 authority；
- 长期未跟踪的 0.0.9B 与用户文件保持未跟踪且未 stage。

## P/F 边界与下一阶段

只执行 P 阶段源码、静态检查、无头 Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook 或 Package。

P7.7 可建立 source-Actor/Run 生命周期适配器，把已有产品热栏选择事件转换为本轮 device-independent intent。事件 owner 和 SelectionId 生命周期必须显式冻结；不得在 Session 内新增 Tick/polling，也不应同时混入键位、Widget、动画或视觉资源。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-6-thrown-weapon-product-session/Docs/Report/Dev.D.UE.0.0.10.P7.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-6-thrown-weapon-product-session/Docs/Log/Dev.D.UE.0.0.10.P7.6.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-6-thrown-weapon-product-session>
