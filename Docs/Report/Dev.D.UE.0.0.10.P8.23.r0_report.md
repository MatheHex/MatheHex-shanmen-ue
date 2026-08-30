# Dev.D.UE.0.0.10.P8.23.r0 Report

## 1. 结论

P8.23 已完成 formation influence 的纯值 authored modifier specification 与 deterministic evaluation receipt。阵法影响现在可以用稳定 policy/content identity、GameplayTag channel、subject tag filters、signed integer magnitude、stack group／policy／priority 表达；Evaluator 对输入顺序做 canonicalization，并为每一条规格保留 applied、filtered 或 suppressed 决策。

本阶段结论为 **PASS**：ModifierEvaluator `4/4`、CombatCore `9/9`、InfluenceIntents `4/4`、`Shanmen.0_0_10` 完整回归 `302/302`、changed-file gate、自检 `77/77`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 冻结的 authored specification

- capture 同时锁定 policy、influence、modifier definition、content stamp、channel、required/blocked subject tags、magnitude、stack group、stack policy 与 priority；
- `SpecificationId` 由上述 canonical values 确定性派生；字段私有，只能通过校验后的 `TryCreate` 构造；
- magnitude 使用 signed `int32` evidence，不使用浮点。单位比例由后续 channel consumer 决定，Evaluator 只做精确整数运算；
- 零 magnitude、无效 channel／identity、required 与 blocked 的 exact contradiction、未知 stack policy 均 fail-closed。

### 2.2 GameplayTag channel 与 subject filters

- 新增 native channel hierarchy：`Shanmen.Influence`、`Offense`、`Offense.Power`、`Defense`、`Defense.Guard`；
- channel 必须 exact match，防止父级聚合误把其它数值域混入同一次求值；
- required/blocked subject tags 使用 GameplayTag parent matching，支持例如子类目标满足父级 `Target.Living`；
- channel mismatch、missing required tags 与 blocked tags 都保留为显式零贡献 decision，不从审计证据中消失。

### 2.3 明确的 stack policy

- `Additive`：同组全部 eligible 规格贡献；
- `StrongestMagnitude`：先比较绝对 magnitude，再比较 priority，完全相等由 canonical SpecificationId 决胜；
- `HighestPriority`：先比较 priority，再比较绝对 magnitude，完全相等由 canonical SpecificationId 决胜；
- 同一 exact channel／stack group 混用不同 policy 会整次拒绝，不按输入先后猜测；
- suppressed decision 精确记录 winning SpecificationId。

### 2.4 deterministic self-validating receipt

- 输入规格先按 SpecificationId 排序，因此数组顺序不影响 decision order、总值或 ReceiptId；
- receipt 内保留完整 Context、完整 Specification snapshots、逐条 decision、contribution 与最终 `int64` magnitude；
- `IsValid` 从 receipt 内的完整规格重新运行 canonical evaluation，再比对每个 decision、总值和 ReceiptId；
- duplicate SpecificationId、同名 modifier 的不同 payload、mixed content 与 stack policy conflict 都拒绝生成 receipt；
- 空规格集产生稳定的零值 receipt，而不是伪造错误或隐式默认 Buff。

## 3. 完整性与安全边界

本阶段明确未实现：

- 把 evaluation receipt 写入现有 lease、executor invocation 或 Host ledger；
- GameplayEffect／GAS、Actor component mutation、World 查询、spawn、damage 或 AI；
- 自动队列 drain、Tick、timer、async、线程、RNG 或后台 retry；
- duration、实时过期、SaveGame、ProfileRepository、网络复制或 content asset loader；
- UI、输入绑定或正式阵法数值表。

新 Evaluator 生产文件扫描：`UWorld/AActor/UObject = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`Timer/Async/RNG = 0`、`float/double = 0`、`Spawn/Damage/Persistence = 0`、`Tick = 0`、`while = 0`、`for = 15`。循环全部遍历 caller 提供的有限纯值规格／decisions，执行 canonicalization、冲突检查、stack 选择或 receipt 自验证，不推进产品生命周期。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceModifierEvaluator.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceModifierEvaluator.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceModifierEvaluatorTests.cpp`。

更新：

- `Source/ShanmenCombatCore/Public/ShanmenCombatTags.h`；
- `Source/ShanmenCombatCore/Private/ShanmenCombatTags.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

P8.11 IntentPlanner、P8.16 LeaseExecutor、P8.15 ExecutorAdapter、ProductHost、dispatch ledger、Coordinator、World adapter 与既有战斗结算生产代码均未修改。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.23-FormationInfluenceModifierEvaluator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceModifierEvaluator` | 4 | 0 | 0 | 1 | `5AC4B6AF24A99A57F9F963A81E56E435F1847EB5B979A4990DAB7B42F7C39890` |
| `P8.23-CombatCore-final.log` | `Shanmen.0_0_10.CombatCore` | 9 | 0 | 0 | 1 | `A91FFF0E3B90423E43BBCCBE31F26DA0983DD5B375BAA88C9697842D25BAB6D9` |
| `P8.23-FormationInfluenceIntents-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceIntents` | 4 | 0 | 0 | 1 | `F92C4C3778A5753E0F77EC909A1C32D898B1C5BE52418B53DDF0E6D41EF62BCF` |
| `P8.23-Shanmen-full-final.log` | `Shanmen.0_0_10` | 302 | 0 | 0 | 1 | `FB77DDC8FF2BC89280768DA501E7450C60F869EFF36E5BDC408FC68CD21D4370` |

四份最终 Automation 日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 Evaluator focused case：

1. `FilteringAndAdditive`：exact channel、hierarchical required/blocked tags、signed additive 与完整 filtered decisions；
2. `StackingAndOrderIndependence`：两类非加法 stack、secondary tie-break、winning ID 与输入重排相同 receipt；
3. `FailClosedContracts`：invalid context/spec、duplicate identity/name、mixed content、policy conflict、contradictory filters 与 zero magnitude；
4. `ReceiptIntegrity`：canonical empty receipt，以及 total、decision、context、winner 四类篡改检测。

首次 focused run 即 `4/4 / Queue 1 / exit 0`，SHA-256 `59CBCF5ED116E7D5101DF2C89584AD0985E1CBD4776DCED4F040E090052C535C`。明确 secondary tie-break 与加强 duplicate-definition 测试后的 review run 仍为 `4/4`，SHA-256 `3F41FAA45235D52118C10949D48E391FC4B4CD67C52E028B8D72D0065CB2156F`。

## 6. 首次运行与审查修正

产品源码首次 Editor 编译成功（`43 actions / 131.53s / exit 0`），Evaluator Automation 首轮即 `4/4`；没有源码编译失败或 Automation case 失败。43 actions 来自 CombatCore public tag header 的正常依赖传播，不是高并发、内存或页面文件故障。

首轮通过后的静态审查把完全相等前的 secondary tie-break 从“直接 canonical ID”明确为 authored 语义：Strongest 以 priority 二次比较，HighestPriority 以 absolute magnitude 二次比较；最终 canonical ID 只处理真正全等。随后补充“同一 modifier definition 不得携带两个 payload”测试。提交前审阅又发现失败 capture 会留下无效但部分填充的输出，已改为 local Candidate 完整验证后才提交，并新增失败输出清零断言；最终构建与回归全部重跑通过。

## 7. Changed-file regression gate

新增 `CombatTags` 与 `FormationInfluenceModifierEvaluator` path rules，并把 Evaluator contract 加入既有 `FormationInfluenceIntents` rule。任何 tags 变更都必须同时覆盖 CombatCore 与 Evaluator；任何 Evaluator 变更都必须覆盖自身、policy intents 与 CombatCore。结果：

```text
REGRESSION_MAP_JSON: PASS Rules=58
SELF_TEST: PASS 77/77
REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=3 Logs=4
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=3 Logs=4
```

- mapping SHA-256：`4ABE2DC6A29386ADFF60CD7B11A069DC531E3818C77A5AB4E7FE0337B3F42043`；
- self-test SHA-256：`B673258DD410E459D6CE66AC9C1207C5D6250F09453A55CFFFEE56BA14C4E788`。

第二条 coverage 是加入 Report／Log 后对 exact-staged 9 文件执行的最终 gate。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial | Succeeded / 43 actions / 131.53s | 0 | `6D6BCD40F541C60B12929B357B1532514293B332A413BA8D018031FAF6A98FAF` |
| Editor final exact source | Succeeded / 5 actions / 16.92s | 0 | `65AF48F95BBFC1E186719C5E6772646C58D1DBB01AA74EA41AA63B20188B9AEF` |
| Game final exact source | Succeeded / 4 actions / 22.54s | 0 | `AB875239568298E28DC520C092EE8E965E262F83CC6F994746D5BE034CC2A888` |

- Editor `demo_map` module：`11758592` bytes，SHA-256 `31869857F4062D3A4AB33ABEE11663EC23C79FE9CD052BEC1CD9D4ADB7A6588D`；
- Editor `ShanmenCombatCore` module：`181248` bytes，SHA-256 `0195D61D73E7E6A375CC93E69506A6358230224959E3C71944EF6BF0D565C261`；
- Game executable：`352871424` bytes，SHA-256 `3A3A4D0D96BB010043B25F7DAC31C847CD4DCFD1DD1D322BA2EA79C7908990E4`。

## 9. 兼容性与工作区保护

- 既有 influence policy 与 intent wire shape 完全不变，Evaluator 通过 ID/content 做单向匹配；
- 既有 executor/lease/Host 不会在本阶段突然要求数值输入或改变 replay 语义；
- native tags 只扩展 CombatCore public API，不改已有 Damage／Defense tags；
- evaluator 不创建第二份 lease、pending queue、acknowledgement 或 lifecycle authority；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 9 个文件。

`git diff --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.24 把本阶段 receipt 直接绑定进现有 execution command／invocation 与 active lease：Apply 必须携带和 intent 的 run/subject/policy/content 精确匹配的 evaluated receipt，Remove 复用 lease 中冻结的 receipt，不重新采样；继续保持 ProductHost/ledger 的唯一排序与 acknowledgement 权威，仍不接 GAS 或 Actor mutation。
