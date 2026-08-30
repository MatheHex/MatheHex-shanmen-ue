# Dev.D.UE.0.0.10.P8.25.r0 Report

## 1. 结论

P8.25 已在 P8.24 的 immutable evaluation receipt／active lease 之上增加首个 formation influence product consumer projection。`Influence.Offense.Power` 现在可以被纯值映射到既有 `AttackPower` additive attribute seam，并生成确定性、可审计、可撤销的 `Fdemo_mapModifierHandle`；本阶段仍不调用 attribute component，不把 fixed-point 证据转换为浮点，也不引入 GAS／Actor mutation。

本阶段结论为 **PASS**：consumer 专项 `3/3`、既有 attribute 契约 `3/3`、完整 influence 链 `59/59`、`Shanmen.0_0_10` 全量 `308/308`、changed-file regression gate、自检 `80/80`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 首个显式 consumer 定义

- 新增 `Fdemo_mapShanmenFormationInfluenceConsumerDefinition`；
- 首个 factory 只允许 `Influence.Offense.Power -> Fdemo_mapAttributeIds::AttackPower / Add`，不接受任意 channel 或任意 attribute；
- authored definition 显式携带 definition ID、每属性点的 magnitude units、priority 与 content stamp；
- 比例属于内容契约，不硬编码在 evaluator，也不让旧属性组件解释原始 influence units。

### 2.2 exact fixed-point projection

- projection 封存完整 active lease snapshot、evaluation receipt 与 consumer definition；
- 数值保持有符号有理证据：`FinalMagnitudeUnits / MagnitudeUnitsPerAttributePoint`；
- 正值、负值和非整除比例都不提前舍入；
- evaluation 最终值为 `0` 时返回成功的 `NoContribution`，不会制造无效 handle 或零值 modifier。

### 2.3 确定性、可撤销 handle

- 复用既有 `Fdemo_mapModifierHandle`，没有建立第二种产品 handle 类型；
- handle 由 lease、Apply intent、evaluation receipt、完整 lease key、consumer definition、channel、target attribute、operation、signed magnitude、比例、priority 与 content 确定性派生；
- `SourceId` 由 handle 派生，同一证据重放得到同一 handle；subject、receipt、magnitude、比例或 authored identity 变化都会产生不同 handle；
- 不使用 `FGuid::NewGuid`、RNG 或调用时序作为身份输入。

### 2.4 Apply／Remove 纯值命令

- `TryBuildCommand` 采用 Candidate-then-commit；无效 projection 或未知 operation 失败并清空输出；
- Apply 与 Remove 拥有不同 command ID，但携带同一个 deterministic modifier handle；
- 命令只表达后续 adapter 应执行的意图，本阶段不拥有 product component state、重试、Host acknowledgement 或 lease authority。

## 3. 完整性与安全边界

projection 只接受 `LeaseSnapshot::IsValid()` 的 active lease，并再次核对 lease/definition content 与 evaluation channel。失效 lease、无效 definition、跨 content 与 unsupported channel 分别产生明确 fail-closed status，且输出保持 canonical empty。

本阶段没有建立第二套 evaluator、Host、ledger、lease 或 attribute authority。P8.23 evaluator 仍是 magnitude 计算来源，P8.24 lease 仍是 active effect authority，旧 `Udemo_mapAttributeComponent` 完全未修改。

新生产文件静态扫描：`UWorld/AActor/UObject = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`Timer/Async/RNG = 0`、`float/double = 0`、`Spawn/ApplyDamage/SaveGame/ProfileRepository = 0`、`Tick/while = 0`、`AddModifier/RemoveModifier/Fdemo_mapModifierSpec = 0`。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProjection.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProjection.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProjectionTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

生产／测试／流程代码在文档加入前共 `833` 行新增；既有 product source 未被机械改写。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---|---|
| `P8.25-FormationInfluenceConsumerProjection-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection` | 3 | 0 | 0 | observed | `8E1060ABA1F8C3126C0A823567EDCA4D1FE963CCE234461BE62984828C769558` |
| `P8.25-Attributes-final.log` | `demo_map.V3.Attributes` | 3 | 0 | 0 | observed | `800DE3D47667DE2E571AD8B0342CA1FF2247114CE770F9FDE4B161374DA77B89` |
| `P8.25-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 59 | 0 | 0 | observed | `94082224315A5B762C37A00711181193987D3D0423AB55E9B8689E53DCFD0519` |
| `P8.25-Shanmen-full-final.log` | `Shanmen.0_0_10` | 308 | 0 | 0 | observed | `C5412F67531EBF493D3795AAC732BD9AF1ABA2D1F1F496BFB350DCAB75C7C189` |

四份最终日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 固定 `Condition failed` 各 `13` 条，与既有阶段一致，不属于项目 Automation case。

新增三项专项 case：

1. `DeterministicApplyRemove`：同一 lease 重放得到同一 handle，Apply／Remove command identity 不同但共享可撤销 handle；
2. `FailClosedContracts`：invalid lease、invalid definition、content mismatch、unsupported channel 与未知 command operation 全部拒绝并清空输出；
3. `ZeroAndIdentityFence`：零和为成功 no-op，负值保持 exact rational，subject 或 authored scale 变化不能 alias handle。

## 6. 首轮与审查结果

首次 Editor 编译即成功（`5 actions / 28.73s / exit 0`）；consumer 专项首次 `3/3`，legacy attributes `3/3`，influence 父组由 P8.24 的 `56` 增至 `59/59`，全量由 `305` 增至 `308/308`。没有源码编译失败或 Automation case 失败。

提交前审查确认：projection identity 已覆盖 consumer definition、signed magnitude、比例、priority 与 content；Remove 沿用同一 handle，而 command identity纳入 operation；零值不创建 handle。审查只把边界注释中的 `timer` 字样改为行为描述，未改变实现语义。

## 7. Changed-file regression gate

新增 `FormationInfluenceConsumerProjection` path rule。任何 projection 修改现在必须提供 consumer、evaluation binding、modifier evaluator、lease executor、legacy attributes 与 CombatCore 六组证据。

```text
REGRESSION_MAP_JSON: PASS Rules=60
SELF_TEST: PASS 80/80
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=6 Logs=4
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=6 Logs=4
```

- mapping SHA-256：`2461D4A01279DB86B365916F68F95A3E63051746B3B3731CAAD89715490FBEA9`；
- self-test SHA-256：`443078BC92E7505D2BD2122295CD63BF230C427FD7134C2CE8733FF50B84B3DF`。

第二条 coverage 是 Report／Log 加入 exact stage 后的最终 gate；Docs 路径按映射规则不增加产品测试要求。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial | Succeeded / 5 actions / 28.73s | 0 | `7BD95400C9069BFFB9963916B03E42BC55F5CE6CCB39E3F2A9DB237469B372A8` |
| Editor final | Succeeded / 5 actions / 7.17s | 0 | `FAD108399D180030E104BB51C3EFD679603A1601895772AA5BD238D9238291E6` |
| Game final | Succeeded / 4 actions / 23.81s | 0 | `4ABEF6B710228D8B17084588267A2BC4DB61A1B04CA48B0DD58FBD49F8C1510D` |

- `UnrealEditor-demo_map.dll`：`11829248` bytes，SHA-256 `C88E20F793DED94F8AFDD28305CC73893E1B9F4925CBCE0C3BBB539FE62152A3`；
- `demo_map.exe`：`352925696` bytes，SHA-256 `13E28F7B475B9771389979F6125FE9A317BF8AD12ADEDFDE601B32B0AB2F97A0`。

## 9. 兼容性与工作区保护

- P8.23 specification/evaluator/receipt wire shape 未修改；
- P8.24 evaluation binding、execution request、lease 与 replay 语义未修改；
- 既有 `Fdemo_mapModifierHandle` 与 `AttackPower` identity 被复用，但旧 component API、float modifier spec 与 insertion ordering 未被调用或改写；
- projection 不改变 ProductHost pending 排序、ledger acknowledgement 或 Remove 的 frozen receipt 规则；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；本阶段只 exact-stage 上述七个文件。

`git diff --cached --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.26 在该 projection contract 上增加纯值、幂等的 consumer application registry：记录 Apply／Remove command 与 active handle snapshot，验证 replay、collision、stale remove 和 teardown drain；仍不直接修改 Actor component。待该状态边界稳定后，再以窄 adapter 对接既有 attribute component。
