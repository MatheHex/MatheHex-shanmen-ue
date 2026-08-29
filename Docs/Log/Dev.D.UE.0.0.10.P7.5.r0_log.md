# Dev.D.UE.0.0.10.P7.5.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.5.r0`；
- 基线提交：`2117547fb197bf59574fe356ffc9f48ae57e0327`（P7.4）；
- 分支：`agent/0.0.10-p7-5-thrown-weapon-product-controller`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与关键决策

P7.4 已把一个冻结 action 路由到 durable Quantity 与真实 World launch，但调用方仍需自行构造 action、选择 exact item 并管理 activation sequence。P7.5 增加产品 controller，使未来输入只能提交设备无关 selection，不能直接拥有库存事务、Actor 生命周期或 action identity。

身份分为两层：`SelectionId` 表示一次产品选择，Run-owned `ActivationSequence/ActivationId` 表示一次物理 action。首次有效 selection 由 coordinator 分配 sequence；exact retry 永久复用冻结 command；同 SelectionId 不同 payload 在任何 effect 前拒绝且不消耗 sequence。

## Product controller 流程

新 controller 先验证 active Run correlation 与 prepared/run inventory membership，再从 ready ShanmenItems snapshot 读取 exact item 的 owner/scope、definition、Quantity capability 与 thrown tag。产品 capture 只提供 combat definition/offense/source tags，item 类型证据不能由调用方替代。

构造 action 时合并产品 tags、`Source.Player` 与权威 item tags，然后捕获 P7.4 command。controller 保存 selection、product、sequence 与 command 的完整 immutable 映射，并以 `IsValid` 重算 ActivationId、检查 source tags 和唯一性。

## Quantity 归属与修复记录

首次定向测试为 `1 Success / 3 Fail`。根因是 controller 读取 loose `Item.Quantity` 并要求其大于零；active Run 已把 Quantity 迁入 committed reservation，loose 值为零仍合法。修复删除该重复 availability 判断，由 P7.1 继续唯一拥有 active-Run 可用量与 pending-intent 排除。

保留失败日志：

| 日志 | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `P7.5_Targeted_FirstFailure.log` | 1 | 3 | 0 | `01AAA1709DFDF0607083807D107F60535DD6286293DD1068FCB4D618CCE6E0B2` |

原生退出码 `0` 不覆盖 Automation Controller 的失败事实。

## 自动化增量

- `CaptureContract`：selection canonicalization、payload identity、无效 capture 与 product capture；
- `SubmitReplayConflict`：真实 active Run/item authority/World submit、exact replay 无 I/O/Actor 重复、payload conflict 与 unprepared item 不消耗 sequence；
- `TransientRetrySequence`：HostBusy 捕获 sequence `2`，释放 Host 后 exact retry 仍用 `2`，next sequence 保持 `3`；
- `SelectionRecovery`：对 cancellation 注入 `WriteTemp` 失败，selection-only recovery 只补 cancellation，Host 保持 empty，绝不 launch。

## 最终自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P7.5_Targeted.log` | `Shanmen.0_0_10.Product.ThrownWeaponProductController` | 4 | 0 | 0 | `52DFF287C6ACC76BCC31E59034DC240592A225DF03BF753555D8C58A382AE393` |
| `P7.5_Full.log` | `Shanmen.0_0_10` | 196 | 0 | 0 | `D4919EEDC2CB788898610A77ADC11E1C0856BF21B9B494C04316E5F960790724` |

两份 canonical 日志均有一个 RunTests、一个 queue-empty、Fail `0`、fatal/unhandled/ensure marker `0`。Automation discovery 前存在项目既有启动噪声，但目标 ControllerResults 为 `4/4` 与 `196/196` Success。

## Changed-file gate

新增 `ThrownWeaponProductController` 映射，要求 product controller、P7.4 command、RunHost、World delivery、P7.1 adapter、coordinator、Items、WorldGameplay、CombatRuntime 九组。

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=9 Logs=2
SELF_TEST: PASS 22/22
```

正向 self-test 证明 full suite 覆盖九个 seam；反向 self-test 证明 coordinator-only evidence 必须失败。

## 构建时间线

统一使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor integration：`43/43`，Succeeded，exit `0`，`136.97s`；
- 首次 Game integration：`42/42`，Succeeded，exit `0`，`129.25s`；
- 最终 source-tag invariant 后 Editor：`4/4`，Succeeded，exit `0`，`11.53s`；
- 最终 Game：`3/3`，Succeeded，exit `0`，`11.95s`。

没有失败构建。首次测试失败在源码修复后完整重跑，不以增量断言代替全 suite。

## 静态、范围与兼容性

- 工作区与最终 staged `git diff --check`：native exit `0`；
- 新 controller 无 EKeys/UInputAction、ApplyDamage、legacy item subsystem、RNG、直接 vitality commit 或直接 Run consumption；
- inventory availability 只由 P7.1 判断，launch/terminal 只由 P7.4/P7.3 执行；
- 未改 schema、Build.cs、GameplayTags 配置、Content、输入、GameMode、Profile、CodeB 或已有 authority 实现；
- 0.0.10 full suite `196/196`；
- 长期未跟踪的 0.0.9B 与用户文件保持未跟踪且未 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook 或 Package。

## 下一阶段

P7.6 建立 active-Run selection source/session owner：从产品已有热栏/装备选择状态获得 exact item，从角色战斗状态冻结 ProductCapture，并调用 P7.5 controller。保持设备无关；键位、UI、动画与视觉表现继续后置。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-5-thrown-weapon-product-controller/Docs/Report/Dev.D.UE.0.0.10.P7.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-5-thrown-weapon-product-controller/Docs/Log/Dev.D.UE.0.0.10.P7.5.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-5-thrown-weapon-product-controller>
