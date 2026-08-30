# Dev.D.UE.0.0.10.P8.21.r0 Report

## 1. 结论

P8.21 已完成 formation influence 的 typed lifecycle command envelope/router。调用方现在可以用冻结的 `CommandId + RunCorrelation + Kind + StepRequest` 明确提交一条 `ExecuteStep`、`PrepareTerminal` 或 `SealAndEnd` 命令；Router 每次只路由一条命令，并在 P8.20 Coordinator 外提供可审计的命令身份、payload conflict、exact replay 与 forward-only teardown recovery。

本阶段结论为 **PASS**：LifecycleCommandRouter `4/4`、LifecycleCoordinator `4/4`、ExecutionService `4/4`、ExecutionRouter `4/4`、ProductRuntime `4/4`、InfluenceHost `4/4`、FormationSession `4/4`、FormationWorldDelivery `4/4`、`Shanmen.0_0_10` 完整回归 `294/294`、changed-file gate、自检 `71/71`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 冻结的 typed command

- `TryCaptureStep` 冻结 `CommandId`、correlation 与完整 ExecutionRequest；
- `TryCapturePrepareTerminal` 与 `TryCaptureSealAndEnd` 不携带伪造的 Step payload；
- Step 的外层 `CommandId` 必须严格等于内层 `RequestId`，禁止同一执行请求被多个外层身份别名化；
- command 字段私有，只能通过校验后的 capture factory 构造。

### 2.2 一次调用只路由一个 lifecycle operation

- `ExecuteStep` 只调用一次 Coordinator `TryExecuteStep`；
- `PrepareTerminal` 只调用一次 `TryPrepareTerminal`，不 drain 新发布的 Remove intents；
- `SealAndEnd` 只调用一次 `TrySealAndEnd`，World 仍由 caller 在调用时提供；
- Router 不发现下一 request，不循环推进产品工作，不后台 retry，也不拥有 Host、World 或输入节奏。

### 2.3 命令身份、冲突与 replay

- accepted result 与 forward-mutating rejection 都写入 durable command record；
- 相同 `CommandId` 的完全一致命令返回既有 receipt，不重新进入 Coordinator；
- 相同 `CommandId` 但 kind、correlation 或 nested request 不同，返回 `CommandIdConflict`；
- `PrepareTerminal` 与 `SealAndEnd` 各只允许一个 durable CommandId；后续不同 ID 返回 `OperationIdentityConflict`；
- 普通前置条件拒绝不占用命令身份，调用方修正顺序后可以显式重提完全相同命令。

### 2.4 Host fence 与 forward-only recovery

- Router 在查找 replay record 前先验证 Host、correlation、Host ledger 与已冻结 Coordinator binding；
- 因此 exact replay 不能借 foreign Host 取回旧成功 receipt；
- `SealAndEnd` 已 seal／end、但 World teardown 失败时，原 CommandId 与 rejection receipt 被持久锁定；
- 只有同一 Host 上的完全相同命令可以显式再次进入 Coordinator 完成 teardown；成功后后续 exact replay 不再重入。

## 3. 完整性与安全边界

本阶段明确未实现：

- 自动读取 pending intent、批量 drain、后台循环、隐式 retry、Tick、timer、async 或线程；
- Host、ledger、session、Coordinator、World 或 Actor 的外部 ownership／discovery；
- spawn、组件 mutation、GAS／GameplayEffect、正式 Buff 数值、伤害或 AI；
- SaveGame、ProfileRepository、Router persistence、跨进程恢复或网络复制；
- UI、输入绑定与正式阵法 content。

新 Router 生产文件扫描：`UWorld` token = `7`（前置声明、参数与函数签名），`World->` = `0`；`AActor/UObject = 0`、`AbilitySystem/GameplayEffect = 0`、`Timer/Async/RNG = 0`、`Spawn/Damage/Persistence = 0`、`Tick = 0`、`while = 0`、`for = 3`。三个 `for` 都只遍历本地有限 command records 以验证 identity／state，不读取 Host pending queue，也不推进任何产品工作。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

P8.20 Coordinator、ProductHost、ExecutionService、ExecutionRouter、ProductRuntime、dispatch ledger、executor、lease executor、session 与 World adapter 的既有生产代码均未修改。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.21-FormationInfluenceLifecycleCommandRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter` | 4 | 0 | 0 | 1 | `A3D099BC4082905DB4F4533AC11069D742C0B86C2B520AF31ACC83718156B23B` |
| `P8.21-FormationInfluenceLifecycleCoordinator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator` | 4 | 0 | 0 | 1 | `65C1CE3E16950B5EF6FCCA5464DABD551DD4074E012826E6F8EE995D1BD1EBB6` |
| `P8.21-FormationInfluenceExecutionService-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionService` | 4 | 0 | 0 | 1 | `9E8F5205F2CF770457B8B78FF895745E0FA227273E7E3428F135C2F6DA05F358` |
| `P8.21-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 0 | 1 | `AA4FFAC8B555CFC829401C11F83117D84D752F3E0BD4D26222660DBA8ED6A528` |
| `P8.21-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 0 | 1 | `DAFCBD84653579E995F86869E2F50A3A7C421B0295108C9B9117AF39AFA89943` |
| `P8.21-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 0 | 1 | `FAEF768D92641945D4B524511A74957F26328F6C200FC11DAA80957B1B135E17` |
| `P8.21-FormationSession-final.log` | `Shanmen.0_0_10.Product.FormationSession` | 4 | 0 | 0 | 1 | `9D542A2518B1245961D8CCE2564829C05D2ECDE517074EED3A7BFA4D1A85A463` |
| `P8.21-FormationWorldDelivery-final.log` | `Shanmen.0_0_10.Product.FormationWorldDelivery` | 4 | 0 | 0 | 1 | `B26A5DF9CDBABEE881D2FFE43E012264E0EC71AEB9BEDA5F609DF3769B609CC1` |
| `P8.21-Shanmen-full-final.log` | `Shanmen.0_0_10` | 294 | 0 | 0 | 1 | `83FFDDD1E1762217FE3E18A777E71E8B0C3FA4C43AB8B1DC0D31CD29FC943A91` |

九份最终 Automation 日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 Router focused case：

1. `TypedLifecycle`：两条 Apply、terminal prepare、两条 Remove 与 seal/end 都由 typed command 显式推进；
2. `IdentityConflictAndReplay`：exact replay 不重入，foreign Host、CommandId payload conflict、kind conflict、Step alias 与 invalid command 均 fail-closed；
3. `NoImplicitProgress`：terminal 不 drain，过早 Remove／End 不占用身份，caller 可显式重提；
4. `ForwardCompletionRecovery`：World teardown 失败锁定 end identity，只有 exact command 可前向恢复并 replay。

首次 focused run 为 `4/4 / Queue 1 / exit 0`，SHA-256 `6E466C56288ECD91864171B547BABE406CE82CBB7906CD13AC502BA64CD1334B`；Host replay fence 修正后的 review run 仍为 `4/4 / Queue 1 / exit 0`，SHA-256 `2E23AF4A2B8D901B43DC6F5AB9A564E649CDE60C3262F0131CA6A942BD6F13C1`。

## 6. 首次运行、审查修正与调用器错误

产品源码首次 Editor 编译成功（`5 actions / 23.21s / exit 0`），Router Automation 首轮即 `4/4`；没有源码编译失败或 Automation case 失败。

首轮通过后的 replay 审查发现：若先按 CommandId 返回 record、后验证 Host，调用方可能用 foreign Host 获得旧成功 receipt。该路径不会修改产品状态，但审计语义错误。验证顺序已前移为“Command／Router／Host／correlation／ledger／binding，再查 replay”，并新增 foreign-Host exact replay 断言。随后进一步要求 Step `CommandId == RequestId`，并冻结 terminal/end 单例操作身份；review 与最终回归均通过。

流程验证出现一次调用器错误：从 Windows PowerShell 的双引号命令中嵌套调用 `pwsh`，外层提前展开 `$map/$changed/$logs`，产生无效 `Rules=0` 与 coverage 参数缺失输出。该证据被丢弃；改为直接使用 PowerShell 7 执行后，map、自检与 coverage 全部通过。这不是 mapping、产品源码、Automation、内存或构建故障。

## 7. Changed-file regression gate

新增 `FormationInfluenceLifecycleCommandRouter` path rule，并把 Router contract 加入 Coordinator、ExecutionService、ExecutionRouter、ProductRuntime、lease executor、executor、Host、dispatch、ProductHost、WorldDelivery 与 Session 等直接 authority seam。结果：

```text
REGRESSION_MAP_JSON: PASS Rules=55
SELF_TEST: PASS 71/71
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=23 Logs=9
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=23 Logs=9
```

- mapping SHA-256：`F724C16152AC9E6ED078B370E31ADE7022BD2A27281DD3D3DBDF1E643108AD32`；
- self-test SHA-256：`E4C46C435A217F6C79410E30F97224D9D1B8B5AE987D224F6F244B8A016655D4`。

第二条 coverage 是加入 Report／Log 后对 exact-staged 7 文件执行的最终 gate。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial Router | Succeeded / 5 actions / 23.21s | 0 | `0327196D6BEBC57126A7117DC4D0902CEE38FE6C6BBE14BF680F6268FEA67939` |
| Editor review | Succeeded / 5 actions / 10.64s | 0 | `DE07FC60CFA4CC0A01B7D176F1B85787FD705673887FB621A5329F4C0064BA9D` |
| Editor final | Succeeded / 4 actions / 7.05s | 0 | `A932D70B0780A001305D9CFE70E3FB13DD0BEAA8F0B60A4D3000C2CC2292F4AF` |
| Game final | Succeeded / 4 actions / 23.99s | 0 | `D3DC0B8A6EC7B79645003183FDD62D9AEC06204F98738E33DAF746D6C1611F4C` |

- Editor module：`11656704` bytes，SHA-256 `BC00BBFFEE5565F81F8DB7AE88A51E31404D9677B59997CE231631D73EDE70C7`；
- Game executable：`352782848` bytes，SHA-256 `69AD8EE0B8F252EBDBEB9F282AB0037242781B7BF689287A998540C2550FE11B`。

## 9. 兼容性与工作区保护

- Router 只组合 P8.20 Coordinator 的 public API，不改任何既有独立调用路径；
- Host 继续拥有 ledger、pending order、attempt receipt、seal 与 teardown authority；
- caller 继续拥有 command identity、调用节奏、Host 与 World；
- replay 前的 Host fence 防止 receipt 跨 authority seam 泄漏；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 7 个文件。

`git diff --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.22 增加 caller-facing lifecycle command host/session seam，把外部 command submission 与本 Router 的 durable receipt 查询组合起来；继续禁止自动 drain、隐式 retry、Actor/World discovery、持久化与正式 GAS 数值。
