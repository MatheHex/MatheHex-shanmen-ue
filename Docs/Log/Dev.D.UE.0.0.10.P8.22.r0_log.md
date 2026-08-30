# Dev.D.UE.0.0.10.P8.22.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.22.r0`；
- 基线：`bfa5c6c0ca0db9cbcda9e7e51ea34b3053b043c1`（P8.21）；
- 分支：`agent/0.0.10-p8-22-formation-influence-lifecycle-command-host`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

在 P8.21 lifecycle command Router 外增加 caller-facing CommandHost。预绑定 ProductHost 的 run/ledger identity，提供单条显式提交与 durable receipt 查询入口，同时不持有 Host/World、不复制 receipt 权威、不自动 drain 或 retry。

## 设计记录

### identity-only open

`TryOpen` 只冻结有效 ProductHost 的 `RunCorrelation` 与 `InfluenceLedgerId`。CommandHost 不保存 ProductHost、World 或 engine-object pointer；每次 submit 都由 caller 提供当前对象。

### single submit, existing result

`TrySubmit` 先验证 CommandHost、command、ProductHost、run correlation、authority 与 ledger identity，再向 Router 委派恰好一次。返回 P8.21 的既有 command result，不创建第二个 envelope。

### durable record is the authority

Router 将 durable record 提升为公开只读值类型，并提供按 CommandId 查询副本。普通前置条件 rejection 不可查询；forward-mutating teardown rejection 可查询；exact recovery 原位更新同一 record。

### replay and foreign-host fence

foreign Host 在 Router 前被拒绝。exact replay 只返回已有 receipt，不推进 Coordinator；未知查询清空输出并失败，不泄漏旧值。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.h`（55 行）；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.cpp`（116 行）；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h`（151 行）；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.cpp`（459 行）；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查 P8.21 Router、ProductHost、RunHost 与既有 caller-facing host/session ownership 习惯。
2. 冻结“预绑定 identity、不拥有 Host/World、单提交、单 receipt 权威”的最小契约。
3. 实现 CommandHost `TryOpen`、`TrySubmit` 与 `TryGetReceipt`。
4. 将 Router durable record 公开为只读值类型并增加 `TryGetRecord`。
5. 新增提交/查询、foreign Host fence、rejection visibility、forward receipt update 四项测试。
6. 首次 Editor 编译 `6 actions / 14.71s` 成功；focused 首轮 `4/4`。
7. 首轮通过后加强 foreign Host 测试，使其在任何合法提交之前证明 `0 receipt / unbound Router`。
8. 回归规则升至 `56`，self-test 升至 `73/73`，新 Host rule 要求 24 个契约组。
9. 十组最终 Automation 全绿，全量由 `294` 增至 `298`。
10. 完成 changed-file gate、boundary scan、最终 Editor/Game Development 构建与 `git diff --check`。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.22-FormationInfluenceLifecycleCommandHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost` | 4 | 0 | 1 | 0 | `4F07F3D20EB0D46E186FE7D6D9A37FEC72DFE832525EAEE295C2A00BF054F9D1` |
| `P8.22-FormationInfluenceLifecycleCommandRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter` | 4 | 0 | 1 | 0 | `679A29A74D2D0D8515FE21DAD200B23A361FFB4DA23D67CE620C415A92BB8BB3` |
| `P8.22-FormationInfluenceLifecycleCoordinator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator` | 4 | 0 | 1 | 0 | `F8EFA53D2105576198BF9C719FFA1B7EB537B988B7C93B9C549799BD775505B5` |
| `P8.22-FormationInfluenceExecutionService-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionService` | 4 | 0 | 1 | 0 | `2F9171B439C1E54598C827E5AEF493D22617CEDFE61CE0F45C8AAA0B63A2F1A9` |
| `P8.22-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 1 | 0 | `E0C7A8363A1AEA4F8096C5E8DA2FBBEA13CC21122DE07E47600246E9570F6910` |
| `P8.22-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 1 | 0 | `B550197936C0C76B9B928550A8ABD13D867AEAB7646FC368A620564F47505059` |
| `P8.22-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 1 | 0 | `F171A75879159F19654902402946B622E9D7572BA894C1A2CC3F8EF167D674B6` |
| `P8.22-FormationSession-final.log` | `Shanmen.0_0_10.Product.FormationSession` | 4 | 0 | 1 | 0 | `6DCD850F11E7B7729348D08240E1C4D737759FA4AECC6AEFDDAE14E830B6207B` |
| `P8.22-FormationWorldDelivery-final.log` | `Shanmen.0_0_10.Product.FormationWorldDelivery` | 4 | 0 | 1 | 0 | `D2466775955818021C9E02C1BAFA8BF9D221B2C1F01A37E8B22908AEC7CBB3E9` |
| `P8.22-Shanmen-full-final.log` | `Shanmen.0_0_10` | 298 | 0 | 1 | 0 | `D763E6A40336E5AB100DC148C1C7283DEEA2D2A739AD40313EBC93AFBBC3A389` |

Fatal／Unhandled／Ensure：十份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## 首轮与审查证据

- 初始 Editor：`6 actions / 14.71s / exit 0`，SHA-256 `D2D33E88FB1FB7BF2C4759D0E19D90001DC8618479C61DC884C2597E65968E60`；
- CommandHost initial：`4/4 / Queue 1 / exit 0`，SHA-256 `CFC72E882757B105B04A686C2B2A1A5A96629D1F33701DF67A316A60EF9425B0`；
- 最终 Editor exact source：`5 actions / 9.02s / exit 0`，SHA-256 `2C2917DF742A2F8B0C5DA60D1C74805172A0E7CB71C13D55A68499EFB3E525AD`；
- 最终 Game：`5 actions / 26.35s / exit 0`，SHA-256 `364D715C52CD2C8CC97F86F1E0D8FF423BCF26C4C52601E0F0E0DC27861BA1DF`。

源码无编译失败，Automation case 无失败。首轮通过后的唯一变更是加强测试证据与修正注释术语。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=56
SELF_TEST: PASS 73/73
REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=24 Logs=10
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=24 Logs=10
```

- mapping SHA-256：`E74F4790EF87B819BDC23DDCF189B13EE3695005ABE0D1C8CFFED0092072856E`；
- self-test SHA-256：`B4708F925DE25586D7AA2EA4C9D6F0B4A2AEFC7B93125374318FE9B5C13C1CEF`。

第二条 coverage 是 Report／Log 加入 exact stage 后的最终 gate。

## 静态边界

新 Host 与改动 Router 生产文件扫描：

```text
UWorld token = 10 (forward declaration, parameters, signatures)
World-> = 0
AActor/UObject = 0
AbilitySystem/GameplayEffect = 0
Timer/Async/RNG = 0
Spawn/Damage/Persistence = 0
Tick = 0
while = 0
for = 4 (Router bounded local durable-record validation/lookup only)
CommandHost loops = 0
```

`git diff --check`：PASS。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial | 6 actions / 14.71s / Succeeded | 0 | `D2D33E88FB1FB7BF2C4759D0E19D90001DC8618479C61DC884C2597E65968E60` |
| Editor final exact source | 5 actions / 9.02s / Succeeded | 0 | `2C2917DF742A2F8B0C5DA60D1C74805172A0E7CB71C13D55A68499EFB3E525AD` |
| Game final | 5 actions / 26.35s / Succeeded | 0 | `364D715C52CD2C8CC97F86F1E0D8FF423BCF26C4C52601E0F0E0DC27861BA1DF` |

- `UnrealEditor-demo_map.dll`：`11686400` bytes，SHA-256 `973A5E42BF82EE220DFD1939A4ADD170ECA055690DDE49930E498F10B66B86EF`；
- `demo_map.exe`：`352808960` bytes，SHA-256 `2F81025BEFF99D78110CE38E6BF461CBFE5B6AA18F12D7CFB4B1648B67FBB0A9`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.23：停止继续叠加 orchestration wrapper，转入纯值 authored influence semantics。定义 formation influence modifier specification 与 deterministic evaluation receipt，覆盖 tags、channel、magnitude 与 stack policy；仍不做 Actor mutation、正式 GAS、UI 或 persistence。
