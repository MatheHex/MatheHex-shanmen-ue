# Dev.D.UE.0.0.10.P8.24.r0 Report

## 1. 结论

P8.24 已把 P8.23 的 immutable formation influence evaluation receipt 接入既有 execution request／command／invocation 与 lease executor。Apply 现在必须携带和 Host-owned intent 精确匹配的已求值回执；Remove 必须携带 canonical empty binding，并复用 active lease 冻结的 Apply 回执，不会重新采样或接受另一份数值证据。

本阶段结论为 **PASS**：评价绑定专项 `3/3`、完整 influence 执行链 `56/56`、`Shanmen.0_0_10` 全量 `305/305`、changed-file regression gate、自检 `78/78`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 单一评价绑定契约

- 新增 `Fdemo_mapShanmenFormationInfluenceEvaluationBinding`，只表达两种合法形态：Apply 的非空自验证 receipt，或 Remove 的 canonical empty binding；
- Apply capture 使用 Candidate-then-commit；无效、空 decisions 或被篡改的 receipt 都失败并清空输出；
- receipt 必须匹配 intent 的 run、subject、policy definition、influence definition 与 content stamp；
- binding 不自行求值、不访问 World，也不拥有 pending、acknowledgement 或 lease authority。

### 2.2 Request 到 Invocation 的不可丢失传递

- execution request、command 与 executor invocation 都携带同一 binding；
- Router 先从 ProductHost／ledger 取得真实 pending 或 historical intent，再校验 binding，不信任 caller 复制的 intent 字段；
- Adapter 在首次执行与历史 replay 两条路径都重新把 binding 对齐 Host intent；不匹配时 fail-closed；
- request replay 会比较完整 binding，同一个 RequestId 换另一份合法 receipt 会返回 `RequestConflict`；
- rejected mismatch 不绑定 Router，也不写 command record。

### 2.3 确定性身份升级

- execution AttemptId 纳入 evaluation receipt identity；Remove 使用固定语义值 `RemoveUsesActiveLeaseReceipt`；
- active LeaseId 与 executor receipt identity 纳入冻结的 Apply receipt ID；
- 上述 canonical payload 发生变化，因此命名空间从 `r1` 升至 `r2`，防止新旧身份规范产生同域碰撞；
- replay 继续由 RequestId、AttemptId、IntentId 与完整 binding 同时约束。

### 2.4 Apply 冻结、Remove 复用

- Apply 成功时 active lease 保存完整 evaluation receipt；
- completion history 同时保存 Apply／Remove 对应的冻结 receipt；
- Remove invocation 自身不允许携带 receipt，只能读取 active lease 已冻结证据；
- exact completed-intent replay 读取原 completion receipt，不重复 lease mutation；
- lease consistency audit 会核对 key、content、receipt、Apply attempt 与 completion history。

## 3. 完整性与安全边界

本阶段没有建立第二套 evaluator、queue、Host、ledger 或 acknowledgement authority，也没有让 execution layer 修改 authored specification。评价计算仍由 P8.23 的纯值 evaluator 完成；P8.24 只绑定并消费其 immutable receipt。

明确未实现：GameplayEffect／GAS、Actor component mutation、World 查询、spawn、damage、AI、Tick、timer、async、RNG、SaveGame、ProfileRepository、网络复制、UI、输入绑定或正式阵法数值资产。

新 binding 生产文件静态扫描：`UWorld/AActor/UObject = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`Timer/Async/RNG = 0`、`float/double = 0`、`Spawn/ApplyDamage/SaveGame/ProfileRepository = 0`、`Tick = 0`、`while = 0`。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceEvaluationBinding.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceEvaluationBinding.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutionRouter.h/.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutorAdapter.h/.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLeaseExecutor.h/.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---|---|
| `P8.24-FormationInfluenceEvaluationBinding-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceEvaluationBinding` | 3 | 0 | 0 | observed | `06089066B43A3732C9556EDC19FB6C562BFF22DE64337FFA789149CA481B03FB` |
| `P8.24-FormationInfluenceModifierEvaluator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceModifierEvaluator` | 4 | 0 | 0 | observed | `CFC3ED2DA4B92234760A7F8A9499EEA90F69ADC6CD0336688A95C17A3CD62F39` |
| `P8.24-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 56 | 0 | 0 | observed | `4D33AB2F1B9FB317EFE80ABDC2E84604ABEB3A592CD821A33CB0E299D1987EEA` |
| `P8.24-Shanmen-full-final.log` | `Shanmen.0_0_10` | 305 | 0 | 0 | observed | `3127FAE643E53ADA32476377AF8EB91B8EBA651444FA9838E301386441640DB7` |

四份最终日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

新增三项专项 case：

1. `ApplyRemoveFreeze`：Apply lease 精确冻结回执，Remove 无重采样并继承同一 completion evidence；
2. `RequestIdentityFence`：缺失、跨 subject 与 RequestId 换 receipt 均拒绝，失败不污染 Router；
3. `TamperAndOperationFence`：篡改、空 decisions、跨 intent 与 Apply/Remove 形态错误全部 fail-closed。

## 6. 首轮与审查修正

核心接线后首次 Editor 编译成功（`15 actions / 51.43s / exit 0`），既有 influence 父组首轮 `53/53`。加入三项专项测试后的增量 Editor 编译成功（`4 actions / 7.37s / exit 0`），专项首轮即 `3/3`。

提交前审查发现 deterministic payload 已增加 receipt identity，而三个 namespace 仍标记 `r1`。已把 execution attempt、lease 与 executor receipt namespace 全部升至 `r2`，随后重新执行最终 Editor、Game 与四组 Automation；没有源码编译失败或 Automation case 失败。

## 7. Changed-file regression gate

新增 `FormationInfluenceEvaluationBinding` path rule，并把专项 contract 加入本轮实际修改的 ProductHost tests、ExecutorAdapter、LeaseExecutor、ExecutionRouter 与 LifecycleCommandRouter 规则。任何绑定或消费链修改现在都必须同时提供 evaluator 与 execution-chain 证据。

```text
REGRESSION_MAP_JSON: PASS Rules=59
SELF_TEST: PASS 78/78
REGRESSION_COVERAGE: PASS Changed=12 Rules=6 Required=26 Logs=4
REGRESSION_COVERAGE: PASS Changed=14 Rules=6 Required=26 Logs=4
```

- mapping SHA-256：`EB0BA58BBD7EE49AD47EC895ACCD56B30CD55C04F4367E2D71EC84EED3C09862`；
- self-test SHA-256：`FFE5C762D96EEDFE690B7905658736176FDA8E55303529802ACADF0BB1C57871`。

第二条 coverage 是 Report／Log 加入 exact stage 后的最终 gate；Docs 路径按映射规则不要求新增产品测试。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor final | Succeeded / 5 actions / 7.30s | 0 | `174387E46D79B62E45E89AF4B2FC3DE3E62699F7ECAE8FF0980A89D24874BF6C` |
| Game final | Succeeded / 12 actions / 42.28s | 0 | `B23C1558C224D493FBC3FF0C03766FA27B4C4A5915469A3DCD0F099231A9B3CB` |

- `UnrealEditor-demo_map.dll`：`11791360` bytes，SHA-256 `452456AD45557274128BC15092029EFAECCB5759C350B5E3B0731F61B6A1BDF0`；
- `demo_map.exe`：`352899072` bytes，SHA-256 `89549F04F6E6934227E531B7D07855A12B4F89EE0692C5A285A8A9C07B611FA5`。

## 9. 兼容性与工作区保护

- ProductHost／influence ledger 仍是唯一 pending 排序与 acknowledgement authority；
- P8.23 evaluator 的 specification、receipt wire shape 与纯函数语义未修改；
- Remove 的 caller contract 由“没有数值输入”显式化为 canonical empty binding，既有 active lease 决定实际回执；
- exact replay 不重复 executor mutation；stale completed Remove 不触碰同 key 的后续 lease；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；本阶段只 exact-stage 上述本轮文件。

`git diff --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.25 在不改变该 receipt/lease authority 的前提下，为具体 formation channel 增加第一个纯值 consumer projection：把冻结 magnitude 映射为可审计、可撤销的产品侧 modifier handle，但仍不直接接 GAS 或 Actor mutation。
