# Dev.D.UE.0.0.10.P8.20.r0 Report

## 1. 结论

P8.20 已完成 formation influence 的 caller-driven lifecycle coordinator。调用方现在可以在一个冻结的 `RunCorrelation + LedgerId` 边界内，显式执行单条 influence request、显式准备 canonical terminal Remove intents，并在调用方逐条 drain 后显式 seal 与 teardown。

本阶段结论为 **PASS**：LifecycleCoordinator `4/4`、ExecutionService `4/4`、ExecutionRouter `4/4`、ProductRuntime `4/4`、InfluenceHost `4/4`、FormationSession `4/4`、FormationWorldDelivery `4/4`、`Shanmen.0_0_10` 完整回归 `290/290`、changed-file gate、自检 `69/69`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 显式生命周期

- `TryExecuteStep` 每次只接受并执行一条 caller-owned request；
- `TryPrepareTerminal` 只让 Host 生成 canonical Remove intents，不执行、不循环、不 drain；
- `TrySealAndEnd` 只在 Host ledger 已无 pending intent 时 seal，并把 caller-owned `UWorld*` 原样转发给 Host teardown；
- Coordinator 不读取或构造下一 request，不拥有 ProductHost，也不引入 Tick、timer、async 或后台调度。

### 2.2 组合 binding

- Coordinator 首次成功生命周期操作冻结一个 `RunCorrelation + LedgerId`；
- 允许“Coordinator 已绑定、ExecutionService 尚未绑定”，以支持先准备 terminal、后显式处理 Apply／Remove；
- 禁止“ExecutionService 已绑定、Coordinator 未绑定”以及两个子状态绑定不一致；
- foreign Host、stale correlation、缺失 ledger 与内部不变量破坏均 fail-closed。

### 2.3 顺序与 replay

- terminal preparation 可以发生在 Apply drain 前，但 Host pending 顺序仍保持 Apply 在 Remove 前；
- 过早 Remove 返回 `IntentOutOfOrder`，不会越过 pending Apply；
- exact terminal preparation 返回 `TerminalReplayed`，不重复生成 Remove intents；
- 完成后的 exact step replay 返回既有 route／execution receipt，不重新调用 executor；
- 已由外部 Service 成功执行的 Host history 不能被全新 Coordinator 半绑定接管。

### 2.4 forward-only completion recovery

- seal 成功后，Coordinator 提交 immutable binding；
- 若随后 World teardown 失败，Host 的 sealed／ended 前向状态与 Coordinator binding 均保留，不伪装成原子回滚；
- 调用方用同一 correlation 与有效 World 重试即可完成 teardown；
- 完成后的再次调用返回 `CompletionReplayed`。

## 3. 完整性与安全边界

本阶段明确未实现：

- 自动读取 pending intent、批量循环、后台 drain、自动 retry、Tick、timer、cadence、async 或线程；
- Host、ledger、session、ExecutionService、executor 或 World 的外部 ownership；
- Actor／World discovery、spawn、组件 mutation、GAS／GameplayEffect 或正式 Buff 数值；
- SaveGame、ProfileRepository、Coordinator persistence、跨进程恢复或网络复制；
- UI、输入、AI 与正式阵法 content。

新 Coordinator 生产文件扫描：`UWorld` token = `3`（前置声明与两个函数签名），`World->` = `0`；`AActor/UObject = 0`、`AbilitySystem/GameplayEffect = 0`、`Timer/Async/RNG = 0`、`Spawn/Damage/Persistence = 0`、loop = `0`。因此 World 只作为 caller-owned teardown 参数转发，没有 discovery 或直接调用。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCoordinator.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCoordinator.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

ProductHost、ExecutionService、ExecutionRouter、ProductRuntime、dispatch ledger、executor adapter、lease executor、session 与 World adapter 的既有生产代码均未修改。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.20-FormationInfluenceLifecycleCoordinator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator` | 4 | 0 | 0 | 1 | `6DFFCDDA3A53CCE357D681189C11599B94D4B26FA2A895DFE5330A40276DA7FE` |
| `P8.20-FormationInfluenceExecutionService-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionService` | 4 | 0 | 0 | 1 | `171A8536390C727CB749A250A8D1FACCCE3FCEA05B258E88636EED08FA7AF640` |
| `P8.20-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 0 | 1 | `4025EF3C05F14679CBBEEFD332DFEE1910EBE447A01B307819777178AB867E1E` |
| `P8.20-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 0 | 1 | `C7F59BB2AE395426CE6A292D62FB67782D41F8DE6A20B7B327BBC14FA8847838` |
| `P8.20-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 0 | 1 | `B5513322E9EBFA88F3F988D0474F7B653A61BC0B874234FBC280DE2B7E9C5349` |
| `P8.20-FormationSession-final.log` | `Shanmen.0_0_10.Product.FormationSession` | 4 | 0 | 0 | 1 | `EBA1A528102A7F1F6878FCEED8C3539195E5248795428A173D38278F6AC2B32D` |
| `P8.20-FormationWorldDelivery-final.log` | `Shanmen.0_0_10.Product.FormationWorldDelivery` | 4 | 0 | 0 | 1 | `2BC92649837D1F4A4358621E2FDAAC10D17A0E1FA5929BD667EBD3925670A1F9` |
| `P8.20-Shanmen-full-final.log` | `Shanmen.0_0_10` | 290 | 0 | 0 | 1 | `8003F026C506654F2D7C9E41A803284528B514268DDF806AFC63BE50F9F74A47` |

八份最终 Automation 日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 Coordinator focused case：

1. `ExplicitLifecycle`：两条 Apply、terminal prepare、两条 Remove 与 seal/end 全部由 caller 显式推进；
2. `NoImplicitDrainAndOrderFence`：terminal prepare 不执行请求，过早 completion 与 Remove 均 fail-closed；
3. `BindingAndLateAttachmentFence`：terminal replay、foreign/stale binding 与成功历史 late attach 均受约束；
4. `ForwardOnlyCompletionRecovery`：seal 后 World teardown 失败保留前向状态，同 correlation 重试完成并可 exact replay。

首次 focused run 亦为 `4/4`，Queue = `1`，原生退出码 `0`；日志 SHA-256：`7F78C8250266EC976F6B946766CEBC77A57C8BFF0351893BF5568E230D120836`。

## 6. 首次失败与修正

产品源码首次 Editor 编译成功（`5 actions / 19.30s / exit 0`），Coordinator Automation 首轮即 `4/4`；没有源码编译失败或 Automation case 失败。

流程验证出现三次调用器／证据选择错误：第一次用 Windows PowerShell 5.1 启动采用 PowerShell 7 行首管道语法的 self-test，解析阶段退出 `1`；改用 `pwsh` 后通过 `69/69`。第一次从 Windows PowerShell 向外部 `pwsh -File` 传递两个数组时，数组被展开为额外位置参数，coverage gate 在参数绑定阶段退出 `1`；改为直接在 `pwsh` shell 内调用后通过。最终 exact-staged gate 首次使用过宽的 `*-final.log` 通配符，误把 Editor／Game 构建日志当作 Automation evidence，校验器因无 RunTests／Queue marker 正确退出 `1`；收窄为八份 Automation 日志后通过 `Changed=7 / Rules=2 / Required=22 / Logs=8`。三次均发生在脚本执行前、参数绑定或证据集合选择阶段，不是 mapping、产品源码、Automation、内存或构建故障。

## 7. Changed-file regression gate

新增 `FormationInfluenceLifecycleCoordinator` path rule，并把 Coordinator contract 加入 ProductSession、WorldDelivery、ProductHost、dispatch、executor adapter、lease executor、product runtime、ExecutionRouter 与 ExecutionService 的 required groups。结果：

```text
REGRESSION_MAP_JSON: PASS Rules=54
SELF_TEST: PASS 69/69
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=22 Logs=8
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=22 Logs=8
```

第一条 coverage 为 Source／Scripts gate；第二条为加入 Report／Log 后的 exact-staged gate。

- mapping SHA-256：`A15E23C1A3DBCC99B7F1F3E5A7571E448F7B79A3D0DDC346C442458B652DCC7C`；
- self-test SHA-256：`D0BD4510617CD9F0D7505936C869637F24B0CC74D36D0B3A4326712F11A880E3`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Evidence |
|---|---|---:|---|
| Editor initial Coordinator | Succeeded / 5 actions / 19.30s | 0 | console evidence |
| Editor final | Succeeded / up to date / 0.93s | 0 | `DCF5C61FED637CCD0352D1631463C4BD4569E6152026FC25B3DC0A3D3D2883F0` |
| Game final | Succeeded / 4 actions / 23.46s | 0 | `F1B5D60F4A58D42C9B36D4096CDAC36A524C45C43F648FD0C9B23337A3078454` |

- Editor module：`11607040` bytes，SHA-256 `86705B586DE08AB72B51C8905FE90BB84F87C61474254F3EB2A6FF304D8EF60D`；
- Game executable：`352740864` bytes，SHA-256 `7411ECC27CD86C7289C88217D233D6FE035F5A79E51F768ED79ADBDF6268D699`。

## 9. 兼容性与工作区保护

- Coordinator 只组合既有 public Host／Service API，不改任何既有独立调用路径；
- Host 继续拥有 pending order、attempt receipts、acknowledgement、seal 与 teardown authority；
- caller 继续拥有 request identity、调用节奏、ProductHost、World 与 lifecycle command；
- 失败结果明确公开 Host／Service 嵌套 receipt 和 Coordinator state 是否提交；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 7 个文件。

`git diff --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.21 增加 typed lifecycle command envelope/router，把 caller 的 `Step / PrepareTerminal / SealAndEnd` 意图映射到本 Coordinator，同时继续保持一条命令一次调用、无自动 drain、无隐式 retry、无 World discovery 与无正式 GAS 数值。
