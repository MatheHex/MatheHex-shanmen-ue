# Dev.D.UE.0.0.10.P8.0.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.0.r0`；
- 基线：`6a08ffe19dbac3330797831558173f728635a5f8`（P7.9）；
- 分支：`agent/0.0.10-p8-0-formation-deployment-core`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与边界

P7 已闭合初级暗器直线投掷纵切。P8.0 按 0.0.10 规划进入阵法体系，只建立阵图、阵眼、材料 requirements 与部署状态机的纯契约。

明确后置：ShanmenItems reserve/commit/cancel、真实阵图商品、阵眼 Actor、区域选择、交互、远程投料、一次成阵、阵法效果、UI、正式配方与数值。

## 设计决策

### 复用既有权威

Deployment 复用 `FShanmenCombatActionSnapshot` 绑定 Run/owner/activation/content，并由 `FShanmenActionOrchestrator::CanEmitCandidates()` 作为部署 begin/commit 的 action commit fence。没有新建 Run subsystem 或物品 authority。

### 可扩展 diagram

阵眼和每个阵眼的材料需求都是带显式 `Order` 的数组。capture 验证连续 order、唯一 ID、有限 geometry 与正数量，然后写入 private/read-only frozen structs。没有 `AnchorA/AnchorB` 或固定材料字段。

### Evidence seam

外部材料 evidence 携带 exact physical item lines、authority revision 与完整 identity envelope。Runtime 只核验 line 聚合与 requirement 精确守恒；不读取或写入 ShanmenItems，也不把传入 evidence 宣称为 durable authority receipt。

## 实现

新增：

- `Source/ShanmenCombatRuntime/Public/ShanmenFormationDeployment.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenFormationDeployment.cpp`；
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenFormationDeploymentTests.cpp`。

状态机允许 Planned → Deploying → Active → Ended，以及完成前 Cancelled。事件 receipts 使用连续 sequence；begin/cancel/end 按 deployment 派生，anchor commit 同时绑定排序后的 exact material lines。replay 不增加 ledger，conflict 不修改状态。

`IsValid` 重算完整 diagram-aware DeploymentId、每个 AnchorInstanceId、world geometry、commit receipt 和状态转换序列。receipt replay 也先执行全对象自校验并验证 ActionRuntime。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationDeployment-Targeted.log` | `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | 0 | `DF89D012FDD02D0A5B01FCFD9E90161B957D2829C86F4AA0E7F9EBF6399BE60F` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 211 | 0 | 0 | `0995B59ECADB6CBAAE316019F7E8EC5E626B8E0BE964EEE3FD467CCF006A5380` |

最终日志均只有一个 RunTests、queue-empty、Fail `0`、fatal/assert/ensure `0`，原生退出码 `0`。

## 测试内容

1. `DiagramAndIdentity`：capture 排序、结构拒绝、平面几何、content 与 diagram structure identity；
2. `ProgressAndReplay`：commit fence、begin replay、多 stack material conservation、final activation 与 conflict；
3. `FulfillmentFailClosed`：错误 identity、缺量、额外 definition、duplicate item、unknown anchor 全部 mutation-free；
4. `Termination`：planned cancellation、active end、replay 和 foreign Run。

## 首次失败时间线

### Editor integration

首次编译：

```text
ShanmenFormationDeploymentTests.cpp(31): C2653 FShanmenCombatIdFactory is not a class or namespace name
ShanmenFormationDeploymentTests.cpp(31): C3861 MakeActivationId identifier not found
Result: Failed (OtherCompilationError)
Native exit: 6
Total: 28.39s
SHA-256: 3716853B722D3A2BAE893411BCB21074A0BE533C2A55DEEB9877F892F10FA058
```

生产文件与 UHT 已编译；测试缺少直接声明头。补 `ShanmenCombatResolver.h` 后成功。

### Targeted automation

首次 targeted：`1 Success / 2 Fail`，第四测试因夹具前置 `check` 终止，native exit `3`；日志 SHA-256：

```text
38737C76DBF5191327E66DC64A0DA29B16BED4F7112ABC20F2AEAB2C1DFC85BC
```

测试自身未真正构造 duplicate material；产品 receipt 又错误禁止非最终 commit 合法的 `Deploying -> Deploying`。修正测试行，并把同态状态仅限于 `CommitAnchor`。材料守恒、identity 和失败关闭没有放宽。

## Changed-file regression gate

新增 `FormationDeploymentCore` 映射，要求 formation focused group；同时保留 CombatRuntime parent requirement。self-test 新增 broad full 正向场景和 unrelated thrown-runtime 反向场景。

```text
REGRESSION_MAP_JSON: PASS
SELF_TEST: PASS 32/32
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=2 Logs=2
```

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | `OtherCompilationError` | 6 | 28.39s | `3716853B722D3A2BAE893411BCB21074A0BE533C2A55DEEB9877F892F10FA058` |
| Editor final | Succeeded | 0 | 9.90s | `AC642983764D499C8CABE63AD4FB2A4800762E55A37D737D1286A76903B103D4` |
| Game final | Succeeded | 0 | 11.41s | `BD643EEE510758A4B33CFB449B56710D10199FA0754D4B176C7001B2D01006B3` |

CombatRuntime Editor DLL：`660480` bytes，UTC `2026-08-29T14:57:40Z`。Game executable：`352009728` bytes，UTC `2026-08-29T14:58:57Z`。

## 静态与兼容性

- JSON parse 与 `git diff --check`：PASS / exit `0`；
- boundary scan：无 UWorld、Actor、Tick、timer、spawn、damage、item subsystem 或 RNG；
- 未修改 Build.cs、GameplayTags、schema、Content、ShanmenItems、GameMode 或输入；
- P0–P7 的 207 项既有测试仍通过；
- 长期用户未跟踪文件未修改、未 stage。

## P/F 边界与下一步

本轮只执行 P 阶段源码、静态检查、`-NullRHI` Automation、Editor/Game Development build。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

P8.1 应以单向 adapter 将每个阵眼 requirement 转为 ShanmenItems durable active-Run quantity reserve/commit/cancel；只有 durable commit 才能生成 fulfillment evidence。阵眼 Actor、交互方式、阵法效果和正式 content 继续后置。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-0-formation-deployment-core/Docs/Report/Dev.D.UE.0.0.10.P8.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-0-formation-deployment-core/Docs/Log/Dev.D.UE.0.0.10.P8.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-0-formation-deployment-core>
