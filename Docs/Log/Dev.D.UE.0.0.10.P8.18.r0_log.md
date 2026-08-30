# Dev.D.UE.0.0.10.P8.18.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.18.r0`；
- 基线：`80b9f3711476af0c585c744bf96c48e1a80abdd3`（P8.17）；
- 分支：`agent/0.0.10-p8-18-formation-influence-execution-router`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

在 P8.17 Runtime 前建立纯值 command router，把 caller `RequestId + ExpectedIntentId` 确定性冻结为 Host-ledger-scoped execution command；支持 exact request replay、payload conflict fence 与历史 evidence recovery，同时不持有或修改 Host／executor。

## 设计记录

### deterministic command identity

Attempt identity 由 `Shanmen.Formation.InfluenceExecutionAttempt.r1` 命名空间和 ordered canonical parts `LedgerId / RequestId / ExpectedIntentId` 派生。Router `IsValid()` 会重算 identity；任意 record 漂移都会使自身 fail-closed。

### caller request fence

RequestId 是 caller-owned idempotency key。相同完整 request 返回已冻结 command；相同 RequestId 配不同 ExpectedIntentId 返回 `RequestConflict`。失败路径从不写 Router。

### Host remains authority

新 command 只接受 Host 当前第一 pending intent。Router 使用 `const ProductHost&`，不调用 acknowledgement、seal 或 executor。Host ledger 继续是 ordering、retry、attempt receipt 与 terminal authority。

### historical reconstruction

如果 deterministic attempt 已在 Host ledger 中存在，fresh Router 记录同一 command 并返回 `HostEvidenceRecovered`。这允许 pending 推进或 sealed terminal 后恢复路由 identity，但实际 semantic replay仍由 P8.15 adapter与 P8.17 Runtime完成。

### immutable binding and transactional mutation

首次成功 route 冻结 correlation 与 ledger ID。Router record 写入候选副本，候选完整有效才替换当前状态；foreign Host、stale correlation、order conflict 与 collision 都不改变原对象。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutionRouter.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutionRouter.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查现有 thrown／controlled command router、P8.14 Host ledger、P8.15 adapter 与 P8.17 Runtime。
2. 定义纯值 request、route status/result、immutable binding 与 replay-stable deterministic attempt identity。
3. 实现 pending-order route、exact request replay、payload conflict、Host evidence recovery 与候选副本提交。
4. 在现有 ProductHost fixture 中新增 route/replay、conflict/order、binding/recovery、retry/terminal 四项测试。
5. 初次 Editor build 因重放诊断多余右括号 `C2059` 失败；修正后成功。
6. Router focused 首轮 `4/4`，随后加入 sealed terminal 下原 Router replay 与 fresh Router recovery 断言。
7. 增强测试块第一次锚定到旧 adapter test，产生 `C2065`；移动到 Router test 后 Editor成功。
8. 最终六组 Automation 全绿，full `282/282`。
9. 回归规则升至 `52`，self-test 升至 `65/65`，changed-file gate Required `20`。
10. 完成 boundary scan、`git diff --check` 与最终 Editor／Game Development 构建。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.18-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 1 | 0 | `E8D8B42045819B50D6594A14CBE0D8114E10FDB0A09632CA13EC366D5C375B7B` |
| `P8.18-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 1 | 0 | `333FB27CB80A8B64C419BCB0B86764E4998E374A67298321624CEEDF047E05F2` |
| `P8.18-FormationInfluenceLeaseExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor` | 4 | 0 | 1 | 0 | `FEF8B58A6CBBDFED9A5D1EC433A442242A2B13D6FDDEFCB0B30DAAB780B096B2` |
| `P8.18-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 1 | 0 | `03E1EC64757ABDCC0D8EEDED93C72CC1D775D4E1EFC8E1FF85F71AD2D8435B78` |
| `P8.18-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 1 | 0 | `9B5134AF9E574272AB9017BEC335F3F4CCF1023AD2EBA2CDEDA09244033F0543` |
| `P8.18-Shanmen-full-final.log` | `Shanmen.0_0_10` | 282 | 0 | 1 | 0 | `16E2BFA8E05C4627A7A7B27CB06D186D0E9E07E8A742100135BD7CD52AA5AE3D` |

Fatal／Unhandled／Ensure：六份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=52
SELF_TEST: PASS 65/65
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=20 Logs=6
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=20 Logs=6
```

第一条 coverage 为 Source／Scripts，第二条为加入 Report／Log 后的最终 exact-staged set。

## 首次失败证据

1. Editor initial：`demo_mapShanmenFormationInfluenceExecutionRouter.cpp(133,67)`，MSVC `C2059`，UBT `OtherCompilationError`，原生退出码 `1`。原因是字符串赋值多一个 `)`；修正后 Editor `4 actions`、退出码 `0`。
2. Editor strengthened-test initial：测试 patch 锚点误落到旧 adapter terminal case，未声明 `Router`、`RemoveRequest`、`Runtime`，MSVC `C2065`，UBT `OtherCompilationError`，原生退出码 `1`。移动代码块后 Editor `4 actions / 6.88s`、退出码 `0`。

Automation case 无失败：Router 首轮与最终均 `4/4`。

## 静态边界

新 Router 生产文件扫描：

```text
UWorld/AActor/UObject = 0
AbilitySystem/GameplayEffect = 0
Timer/Async/RNG = 0
Spawn/Damage/Persistence = 0
Host mutation / executor ownership / automatic drain = 0
```

`git diff --check`：PASS。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial Router | `C2059` / Failed | 1 | console evidence |
| Editor corrected Router | 4 actions / Succeeded | 0 | console evidence |
| Editor strengthened-test initial | `C2065` / Failed | 1 | console evidence |
| Editor corrected strengthened test | 4 actions / 6.88s / Succeeded | 0 | console evidence |
| Editor final | up to date / 0.89s / Succeeded | 0 | `37405834F1C95BDAA7E1C433C11FB4A318B0CB059F41EFC2E341BC421A4064BF` |
| Game final | 3 actions / 19.20s / Succeeded | 0 | `2802775450F679BF76754CE6156DBF3B133074E1A62A24572079F9DC5A7B193C` |

- `UnrealEditor-demo_map.dll`：`11553280` bytes，SHA-256 `95BFA7E0883E25BFDCB806F50997CF4107365D253CD41DE46755100DB71A65F7`；
- `demo_map.exe`：`352690176` bytes，SHA-256 `D401F956D87B3F52B02FB9D4C8F5AFE2705CE80E100FEF4115CA7FAC75952F7B`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.19：用窄 single-step execution service组合 Router 与 Runtime；每次 caller request最多 route／execute一条 intent，保留显式节奏与 Host 唯一 authority，不引入 drain loop、Actor discovery、GAS 或正式数值。
