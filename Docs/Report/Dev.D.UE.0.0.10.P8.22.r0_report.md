# Dev.D.UE.0.0.10.P8.22.r0 Report

## 1. 结论

P8.22 已完成 formation influence lifecycle 的 caller-facing CommandHost。外部调用方现在可以先从有效 ProductHost 冻结 `RunCorrelation + InfluenceLedgerId`，随后通过一个入口显式提交单条 lifecycle command，并按 `CommandId` 查询 P8.21 Router 的 durable command/receipt record。

本阶段结论为 **PASS**：CommandHost `4/4`、Router `4/4`、Coordinator `4/4`、ExecutionService `4/4`、ExecutionRouter `4/4`、ProductRuntime `4/4`、InfluenceHost `4/4`、FormationSession `4/4`、WorldDelivery `4/4`、`Shanmen.0_0_10` 完整回归 `298/298`，changed-file gate、自检 `73/73`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 预绑定、非拥有式 CommandHost

- `TryOpen` 只从有效 ProductHost 冻结 run correlation 与 influence ledger identity；
- CommandHost 不保存 ProductHost、World、Actor 或其它 engine-object pointer；
- 每次提交仍要求 caller 显式传入当前 ProductHost 与 World；
- foreign Host、foreign correlation、ledger mismatch 与无 authority Host 在进入 Router 前 fail-closed。

### 2.2 单条提交入口

- `TrySubmit` 每次只向 P8.21 Router 委派一条 command；
- 沿用 Router 的 command result，不增加第二层 envelope 或第二份状态语义；
- 不发现下一命令、不 drain pending intents、不自动 retry、不循环推进 lifecycle；
- CommandHost 不接管输入节奏、Host 生命周期或 World teardown ownership。

### 2.3 durable receipt 查询

- Router 新增公开只读 record：冻结 command 与其最终 result；
- `TryGetRecord` / CommandHost `TryGetReceipt` 返回副本，不暴露可变内部存储；
- accepted result 与 forward-mutating teardown rejection 可查询；
- 普通前置条件 rejection 不形成 durable record，因此不会伪装成已提交工作；
- teardown exact recovery 原位更新同一 CommandId record，receipt count 不增加。

### 2.4 replay 与查询边界

- exact replay 返回既有 receipt，不重入 Coordinator；
- foreign Host 即使持有相同 command，也不能提交或读取跨 authority 的结果；
- 未知 CommandId 查询失败并清空输出；
- terminal 顺序被 caller 修复后，相同普通拒绝命令可以显式重提并形成 durable receipt。

## 3. 完整性与安全边界

本阶段明确未实现：

- 自动队列 drain、后台循环、隐式 retry、Tick、timer、async 或线程；
- ProductHost／World／Actor ownership、发现、缓存或生命周期托管；
- spawn、组件 mutation、正式 GAS／GameplayEffect、Buff 数值、伤害或 AI；
- Router／CommandHost persistence、SaveGame、ProfileRepository、网络复制或跨进程恢复；
- UI、输入绑定与正式阵法 content。

新 Host 与改动 Router 扫描：`UWorld` token = `10`（前置声明、参数与签名），`World-> = 0`；`AActor/UObject = 0`、`AbilitySystem/GameplayEffect = 0`、`Timer/Async/RNG = 0`、`Spawn/Damage/Persistence = 0`、`Tick = 0`、`while = 0`、`for = 4`。四个 `for` 全部位于 Router，只遍历本地有限 durable records 做 identity／query 校验；新 CommandHost 自身没有循环。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

P8.21 以外的 Coordinator、ProductHost、ExecutionService、ExecutionRouter、ProductRuntime、dispatch ledger、executor、lease executor、session 与 World adapter 生产代码均未修改。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.22-FormationInfluenceLifecycleCommandHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost` | 4 | 0 | 0 | 1 | `4F07F3D20EB0D46E186FE7D6D9A37FEC72DFE832525EAEE295C2A00BF054F9D1` |
| `P8.22-FormationInfluenceLifecycleCommandRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter` | 4 | 0 | 0 | 1 | `679A29A74D2D0D8515FE21DAD200B23A361FFB4DA23D67CE620C415A92BB8BB3` |
| `P8.22-FormationInfluenceLifecycleCoordinator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator` | 4 | 0 | 0 | 1 | `F8EFA53D2105576198BF9C719FFA1B7EB537B988B7C93B9C549799BD775505B5` |
| `P8.22-FormationInfluenceExecutionService-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionService` | 4 | 0 | 0 | 1 | `2F9171B439C1E54598C827E5AEF493D22617CEDFE61CE0F45C8AAA0B63A2F1A9` |
| `P8.22-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 0 | 1 | `E0C7A8363A1AEA4F8096C5E8DA2FBBEA13CC21122DE07E47600246E9570F6910` |
| `P8.22-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 0 | 1 | `B550197936C0C76B9B928550A8ABD13D867AEAB7646FC368A620564F47505059` |
| `P8.22-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 0 | 1 | `F171A75879159F19654902402946B622E9D7572BA894C1A2CC3F8EF167D674B6` |
| `P8.22-FormationSession-final.log` | `Shanmen.0_0_10.Product.FormationSession` | 4 | 0 | 0 | 1 | `6DCD850F11E7B7729348D08240E1C4D737759FA4AECC6AEFDDAE14E830B6207B` |
| `P8.22-FormationWorldDelivery-final.log` | `Shanmen.0_0_10.Product.FormationWorldDelivery` | 4 | 0 | 0 | 1 | `D2466775955818021C9E02C1BAFA8BF9D221B2C1F01A37E8B22908AEC7CBB3E9` |
| `P8.22-Shanmen-full-final.log` | `Shanmen.0_0_10` | 298 | 0 | 0 | 1 | `D763E6A40336E5AB100DC148C1C7283DEEA2D2A739AD40313EBC93AFBBC3A389` |

十份最终 Automation 日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 CommandHost focused case：

1. `SubmissionAndReceiptQuery`：提交、查询冻结 record、exact replay 不改写 durable record；
2. `BindingAndForeignHostFence`：invalid/foreign Host 与 invalid command 在 Router 前 fail-closed；
3. `RejectionVisibility`：普通顺序拒绝不可查询，修复顺序后的正式提交可查询；
4. `ForwardReceiptUpdate`：teardown failure 可查询，exact recovery 原位更新同一 receipt。

首次 focused run 即 `4/4 / Queue 1 / exit 0`，SHA-256 `CFC72E882757B105B04A686C2B2A1A5A96629D1F33701DF67A316A60EF9425B0`。首轮成功后的测试审查把 foreign-Host 断言前移到任何合法提交之前，以明确证明拒绝留下 `0 receipt / unbound Router`；最终 focused 仍为 `4/4`。

## 6. 首次运行与审查修正

产品源码首次 Editor 编译成功（`6 actions / 14.71s / exit 0`），CommandHost Automation 首轮即 `4/4`；没有源码编译失败或 Automation case 失败。

首轮通过后的审查仅加强测试证据：foreign Host 必须在 Router 形成任何 binding 或 record 之前被 CommandHost 拒绝。产品行为无需修正。最终源码文本中一个注释的 `UObject` 泛称改为更准确的 `engine-object pointer`，随后重新执行最终 Editor 构建；没有掩盖或删除失败证据。

## 7. Changed-file regression gate

新增 `FormationInfluenceLifecycleCommandHost` path rule，并把新 Host contract 加入 Router、Coordinator、ExecutionService、ExecutionRouter、ProductRuntime、lease executor、executor、InfluenceHost、dispatch、reconciliation、coverage、ProductHost、WorldDelivery 与 Session 等直接 authority seam。结果：

```text
REGRESSION_MAP_JSON: PASS Rules=56
SELF_TEST: PASS 73/73
REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=24 Logs=10
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=24 Logs=10
```

- mapping SHA-256：`E74F4790EF87B819BDC23DDCF189B13EE3695005ABE0D1C8CFFED0092072856E`；
- self-test SHA-256：`B4708F925DE25586D7AA2EA4C9D6F0B4A2AEFC7B93125374318FE9B5C13C1CEF`；
- 新 Host rule 要求 `24` 个下游契约组，重复项 `0`；
- 第二条 coverage 是加入 Report／Log 后对 exact-staged 9 文件执行的最终 gate。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial CommandHost | Succeeded / 6 actions / 14.71s | 0 | `D2D33E88FB1FB7BF2C4759D0E19D90001DC8618479C61DC884C2597E65968E60` |
| Editor final exact source | Succeeded / 5 actions / 9.02s | 0 | `2C2917DF742A2F8B0C5DA60D1C74805172A0E7CB71C13D55A68499EFB3E525AD` |
| Game final | Succeeded / 5 actions / 26.35s | 0 | `364D715C52CD2C8CC97F86F1E0D8FF423BCF26C4C52601E0F0E0DC27861BA1DF` |

- Editor module：`11686400` bytes，SHA-256 `973A5E42BF82EE220DFD1939A4ADD170ECA055690DDE49930E498F10B66B86EF`；
- Game executable：`352808960` bytes，SHA-256 `2F81025BEFF99D78110CE38E6BF461CBFE5B6AA18F12D7CFB4B1648B67FBB0A9`。

## 9. 兼容性与工作区保护

- CommandHost 只组合 ProductHost identity 与 P8.21 Router public API，不改变任何既有独立调用路径；
- ProductHost 继续拥有 ledger、pending order、attempt receipt、seal 与 teardown authority；
- caller 继续拥有 command identity、调用节奏、实际 Host 与 World；
- Router durable record 是 receipt 的唯一权威，CommandHost 不复制日志；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 9 个文件。

`git diff --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.23 不再叠加 orchestration wrapper，而转入纯值 authored influence semantics：定义带 tags／channel／magnitude／stack policy 的 formation influence modifier specification 与确定性 evaluation receipt，先保持纯值、无 Actor mutation、无正式 GAS；后续再由既有 executor seam 消费。
