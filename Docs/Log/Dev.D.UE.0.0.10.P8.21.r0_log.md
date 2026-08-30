# Dev.D.UE.0.0.10.P8.21.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.21.r0`；
- 基线：`ff5fc0c578b6b1bdac8eb68ea02b9999370b40e0`（P8.20）；
- 分支：`agent/0.0.10-p8-21-formation-influence-lifecycle-command-router`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

在 P8.20 caller-driven Coordinator 外增加 typed lifecycle command envelope/router。冻结命令身份与 payload，一次调用只映射一项 lifecycle operation，提供 exact replay、payload conflict 与 forward-only World teardown recovery，同时不引入自动 drain、隐式 retry 或 Host/World ownership。

## 设计记录

### command envelope

命令只允许 `ExecuteStep`、`PrepareTerminal` 与 `SealAndEnd`。Step 捕获完整 ExecutionRequest，且外层 CommandId 必须等于内层 RequestId；terminal/end 不允许残留 Step payload。所有字段私有并由 capture factory 校验。

### durable identity

Router 保存 command + lifecycle receipt。成功和 Coordinator 已提交前向状态的 rejection 才能成为 durable record；普通前置条件 rejection 不占用 identity。相同 CommandId 的不同 kind、correlation 或 request 永远冲突。PrepareTerminal 与 SealAndEnd 分别只允许一个 durable ID。

### replay before re-entry, authority before replay

调用先验证 Router、Host、correlation、ledger 与 Coordinator binding，再查询 command record。完全相同的既有命令直接返回 receipt，不重入 Coordinator；foreign Host 即使提交 exact command 也不能取得旧成功 receipt。

### explicit forward recovery

SealAndEnd 在 Host 已 seal/end 后可能因无效 World teardown 失败。此类 `EndRejected` record 锁定命令身份；只有完全相同的命令可由 caller 显式重提并再次进入 Coordinator。成功后 record 被前向更新，后续 exact replay 不再重入。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h`（144 行）；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.cpp`（430 行）；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查既有 execution command router、P8.20 Coordinator、ProductHost 与 World teardown receipt 语义。
2. 定义 typed command kind、capture factory、outer status/result 与 durable record invariants。
3. 实现一次调用一个 operation、exact replay、payload conflict、terminal/end identity fence 与 forward recovery。
4. 新增 typed lifecycle、identity/replay、no implicit progress、forward recovery 四项测试。
5. Editor 首次编译 `5 actions / 23.21s` 成功；Router 首轮 `4/4`。
6. 首轮通过后的静态审查发现 foreign Host exact replay audit 缺口；前移 Host/binding 校验并补测试，review `4/4`。
7. 收紧 Step CommandId 与 RequestId 一致性及 terminal/end 单例身份，完成最终 Editor 编译。
8. 回归规则升至 `55`，PowerShell 7 self-test 升至 `71/71`。
9. 九组最终 Automation 全绿，全量由 `290` 增至 `294`。
10. changed-file gate 通过：Source／Scripts `Changed=5 / Rules=2 / Required=23 / Logs=9`。
11. 完成 boundary scan、`git diff --check` 与最终 Game Development 构建。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.21-FormationInfluenceLifecycleCommandRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter` | 4 | 0 | 1 | 0 | `A3D099BC4082905DB4F4533AC11069D742C0B86C2B520AF31ACC83718156B23B` |
| `P8.21-FormationInfluenceLifecycleCoordinator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator` | 4 | 0 | 1 | 0 | `65C1CE3E16950B5EF6FCCA5464DABD551DD4074E012826E6F8EE995D1BD1EBB6` |
| `P8.21-FormationInfluenceExecutionService-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionService` | 4 | 0 | 1 | 0 | `9E8F5205F2CF770457B8B78FF895745E0FA227273E7E3428F135C2F6DA05F358` |
| `P8.21-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 1 | 0 | `AA4FFAC8B555CFC829401C11F83117D84D752F3E0BD4D26222660DBA8ED6A528` |
| `P8.21-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 1 | 0 | `DAFCBD84653579E995F86869E2F50A3A7C421B0295108C9B9117AF39AFA89943` |
| `P8.21-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 1 | 0 | `FAEF768D92641945D4B524511A74957F26328F6C200FC11DAA80957B1B135E17` |
| `P8.21-FormationSession-final.log` | `Shanmen.0_0_10.Product.FormationSession` | 4 | 0 | 1 | 0 | `9D542A2518B1245961D8CCE2564829C05D2ECDE517074EED3A7BFA4D1A85A463` |
| `P8.21-FormationWorldDelivery-final.log` | `Shanmen.0_0_10.Product.FormationWorldDelivery` | 4 | 0 | 1 | 0 | `B26A5DF9CDBABEE881D2FFE43E012264E0EC71AEB9BEDA5F609DF3769B609CC1` |
| `P8.21-Shanmen-full-final.log` | `Shanmen.0_0_10` | 294 | 0 | 1 | 0 | `83FFDDD1E1762217FE3E18A777E71E8B0C3FA4C43AB8B1DC0D31CD29FC943A91` |

Fatal／Unhandled／Ensure：九份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## 首轮与审查证据

- 初始 Editor：`5 actions / 23.21s / exit 0`，SHA-256 `0327196D6BEBC57126A7117DC4D0902CEE38FE6C6BBE14BF680F6268FEA67939`；
- Router initial：`4/4 / Queue 1 / exit 0`，SHA-256 `6E466C56288ECD91864171B547BABE406CE82CBB7906CD13AC502BA64CD1334B`；
- Host replay fence 后 Editor：`5 actions / 10.64s / exit 0`，SHA-256 `DE07FC60CFA4CC0A01B7D176F1B85787FD705673887FB621A5329F4C0064BA9D`；
- Router review：`4/4 / Queue 1 / exit 0`，SHA-256 `2E23AF4A2B8D901B43DC6F5AB9A564E649CDE60C3262F0131CA6A942BD6F13C1`。

源码无编译失败，Automation case 无失败。首轮成功后的代码审查主动修复了 foreign Host replay audit 语义，并非测试失败驱动。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=55
SELF_TEST: PASS 71/71
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=23 Logs=9
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=23 Logs=9
```

- mapping SHA-256：`F724C16152AC9E6ED078B370E31ADE7022BD2A27281DD3D3DBDF1E643108AD32`；
- self-test SHA-256：`E4C46C435A217F6C79410E30F97224D9D1B8B5AE987D224F6F244B8A016655D4`。

一次无效证据调用使用 Windows PowerShell 双引号嵌套 `pwsh`，导致外层提前展开 PowerShell 变量，并产生 `Rules=0`／coverage 参数缺失。丢弃该输出后，以直接 PowerShell 7 调用得到上述有效结果；产品代码、测试和 mapping 本身没有失败。

## 静态边界

新 Router 生产文件扫描：

```text
UWorld token = 7 (forward declaration, parameters, signatures)
World-> = 0
AActor/UObject = 0
AbilitySystem/GameplayEffect = 0
Timer/Async/RNG = 0
Spawn/Damage/Persistence = 0
Tick = 0
while = 0
for = 3 (bounded local command-record validation/lookup only)
```

`git diff --check`：PASS。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial | 5 actions / 23.21s / Succeeded | 0 | `0327196D6BEBC57126A7117DC4D0902CEE38FE6C6BBE14BF680F6268FEA67939` |
| Editor review | 5 actions / 10.64s / Succeeded | 0 | `DE07FC60CFA4CC0A01B7D176F1B85787FD705673887FB621A5329F4C0064BA9D` |
| Editor final | 4 actions / 7.05s / Succeeded | 0 | `A932D70B0780A001305D9CFE70E3FB13DD0BEAA8F0B60A4D3000C2CC2292F4AF` |
| Game final | 4 actions / 23.99s / Succeeded | 0 | `D3DC0B8A6EC7B79645003183FDD62D9AEC06204F98738E33DAF746D6C1611F4C` |

- `UnrealEditor-demo_map.dll`：`11656704` bytes，SHA-256 `BC00BBFFEE5565F81F8DB7AE88A51E31404D9677B59997CE231631D73EDE70C7`；
- `demo_map.exe`：`352782848` bytes，SHA-256 `69AD8EE0B8F252EBDBEB9F282AB0037242781B7BF689287A998540C2550FE11B`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.22：增加 caller-facing lifecycle command host/session seam，使外部提交与 durable receipt 查询有单一入口；继续保持一命令一调用、无自动 drain、无隐式 retry、无 Host/World ownership、无 persistence 与无正式 GAS 数值。
