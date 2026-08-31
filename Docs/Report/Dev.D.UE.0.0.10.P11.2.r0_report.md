# Dev.D.UE.0.0.10.P11.2.r0 Report

## 1. 结论

P11.2 **PASS / READY_FOR_0_0_10_P11_3**。本阶段在 P11.0 普通武器格挡窗口和 P11.1 精准格挡时序之上增加了独立的方向／夹角资格策略。上层显式冻结“格挡朝向”和“防御者指向威胁”的两个三维向量，CombatRuntime 只做规范化、点积与内容阈值比较，不查询 World、Actor transform、输入或碰撞。

方向合格时，策略原样透传 P11.1 已经互斥选出的唯一 PerfectGuard 或普通 Guard layer；方向不合格时返回可审计的 `OutsideArc` 结果且不携带 layer。策略不重新计算时序、不改写减伤层，也不创建第二套动作生命周期。

正式验证为 WeaponGuardArc `6/6`、WeaponPerfectGuard `6/6`、WeaponGuard 前缀组 `12/12`、ActionLifecycle `1/1`、CombatCore `9/9`、CombatRuntime `105/105`、`Shanmen.0_0_10` 全量 `473/473`，合计 `612` Success、`0` Fail。91-rule mapping、142/142 mapping self-test、changed-file gate、边界扫描、`git diff --cached --check`、Editor Development 与 Game Development 全部通过。

本阶段没有冻结产品角度、度数或默认阈值；`MinimumFacingDot` 由内容提供并采用闭区间 `[-1,1]`。测试中的 `0.0 / 0.25 / 0.5` 仅为测试内容。没有实现武器耐久、体力／灵力、真实输入、Actor/World 适配、GAS ability、动画、音效或命中特效。

## 2. 功能性

### 2.1 内容拥有的夹角政策

`FShanmenWeaponGuardArcPolicy::TryCapture` 冻结：

- P11.0 的 exact `FShanmenWeaponGuardWindowReceipt`；
- 非空 `ArcRuleId`；
- 内容提供的 inclusive `MinimumFacingDot`。

阈值必须有限且位于 `[-1,1]`。Runtime 不把点积换算为角度，不提供隐式默认值，也不根据武器类别猜测平衡参数。`dot >= MinimumFacingDot` 合格，精确边界也属于可格挡区域。

### 2.2 显式威胁方向样本

`FShanmenWeaponGuardThreatSample::TryCapture` 接受一个既有 `FShanmenHitCandidate`，以及调用方明确给出的：

- `GuardFacing`：防御者当前格挡朝向；
- `DirectionToThreat`：从防御者指向威胁的方向。

两个向量必须三轴有限且非零，capture 时统一规范化为三维单位向量并消除负零。策略刻意不从 `HitNormal` 推断正反面，因为既有 candidate 只承诺接触法线，没有冻结其符号对应“朝向攻击者”还是“朝向防御者”。World 查询、transform 读取与向量来源归上层适配器所有。

### 2.3 时序层透传与 OutsideArc

`FShanmenWeaponGuardArcEvaluator::TryEvaluate` 要求：

1. P11.0 guard window 对传入 action runtime 仍为 Active；
2. arc policy、P11.1 timing receipt 与实际 window 的 exact identity 一致；
3. threat sample 有效；
4. candidate 的 TargetEntityId 精确等于 guard action 的 SourceEntityId。

通过以上绑定后只计算一次规范点积：

- `Qualified`：原样复制 P11.1 `GetLayer()`，因此 Perfect/Ordinary 的 LayerId、operation、order、magnitude、tags、filters、source item 与 commit 语义全部不变；
- `OutsideArc`：生成有效、可重放的 evaluation receipt，但保持默认无效 layer，`HasLayer()==false`；
- window、receipt、target 或 lifecycle 不一致：失败关闭，不生成伪造证据。

前方精准格挡继续得到 `PerfectGuarded / PreventedDamage=100 / FinalDamage=0`；前方普通格挡继续得到 `Mitigated / PreventedDamage=25 / FinalDamage=75`，两条路径都保持 CombatCore 守恒。背后攻击不会把 P11.1 已选 layer 交给 resolver。

### 2.4 确定性身份

PolicyId 纳入 window receipt、window、rule 与阈值 double bits。SampleId 纳入 candidate 的 activation/source/target/detector/kind/ordinal/contact 数据，以及规范后的两组方向 double bits。EvaluationId 纳入 policy、timing receipt、sample、status、alignment dot 与最终 layer identity；OutsideArc 使用明确的 `NO_LAYER` 身份组成。

等价比例向量会规范为相同 SampleId 与 EvaluationId；阈值变化会产生不同 PolicyId，并可把同一样本稳定分类为另一个结果。所有公开结构只暴露只读字段。

## 3. 完整性与兼容性

- 复用 P11.0 window 与 `FShanmenActionOrchestrator`，没有复制动作状态机；
- 复用 P11.1 已选择的 timing layer，不重新编码 Perfect/Ordinary 时序；
- 复用 `FShanmenHitCandidate` 作为接触身份，不创建第二候选协议；
- 方向政策不会修改 P11.0/P11.1 layer，也不会绕过 CombatCore resolver；
- P11.0 普通格挡和 P11.1 精准格挡既有测试保持通过；
- 没有修改 CombatCore enum、tag tree、resolver、Impact、action snapshot 或现有资源权威；
- 没有 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage`、`GetWorld`、RNG、Timer 或 Tick callback 依赖。

## 4. 关键不变量

1. Arc policy 必须绑定一个有效的 exact P11.0 guard window；
2. ArcRuleId 必须非空，阈值必须有限且位于 `[-1,1]`；
3. GuardFacing 与 DirectionToThreat 必须有限、非零并规范为三维单位向量；
4. Runtime 不从 HitNormal 猜测方向语义；
5. candidate 必须命中 guard action 的 source entity；
6. window、policy 与 timing receipt 必须属于同一个 exact window；
7. action runtime 仍是唯一 Active/Recovery/Interrupted 权威；
8. 点积边界采用 inclusive `>=`；
9. Qualified 必须逐字段等同 P11.1 selected layer；
10. OutsideArc 必须是有效 receipt 且不携带任何 layer；
11. evaluator 不查询 World、transform、input 或 collision；
12. evaluator 不拥有 Tick、Timer 或可变历史；
13. 等价规范输入重放相同身份；
14. rule、阈值、candidate、方向、status 或 selected layer 变化必须进入 identity；
15. 方向合格路径继续满足 CombatCore 伤害守恒。

## 5. 测试覆盖

新增 `Shanmen.0_0_10.CombatRuntime.WeaponGuardArc` 六个测试：

- `PolicyAndSample`：policy 边界、空 rule、越界 threshold、向量规范化与零向量拒绝；
- `QualifiedPerfect`：正面精准层逐身份透传、Perfect/Guard tags 互斥及 100 点完全防御守恒；
- `QualifiedOrdinary`：正面普通层逐身份透传及 25/75 结算守恒；
- `OutsideArc`：背面威胁产生 `OutsideArc`、alignment `-1` 与无 layer receipt；
- `BindingAndLifecycle`：外来 target/window、Recovery、Interrupted 均失败关闭；
- `BoundaryAndReplay`：点积精确等于阈值时合格、比例向量稳定重放、阈值变化产生不同 OutsideArc 证据。

同时执行 P11.1 WeaponPerfectGuard、P11.0 WeaponGuard、ActionLifecycle、CombatCore、CombatRuntime 宽回归与 0.0.10 全量回归。由于 `WeaponGuard` 是父前缀，加入 `WeaponGuardArc` 后该组由 6 增至 12；CombatRuntime 由 99 增至 105，全量由 467 增至 473。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenWeaponGuardArc.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenWeaponGuardArc.cpp`。

测试与回归规则：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenWeaponGuardArcTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

实现与门禁为 `5 files / +1109 / -0`；加入本 Report 与同名 Development Log 后 exact stage 为 `7` 个文件。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 文档与用户文件均未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P11.2-WeaponGuardArc-final.log` | `WeaponGuardArc` | 6 | 0 | `A1CBABA476E1D1BCFAC1C9E1C92A869CBEE91BF484832BDB589210FB5140019D` |
| `P11.2-WeaponPerfectGuard-final.log` | `WeaponPerfectGuard` | 6 | 0 | `994FF9794D1338E28B5BFBF778E03427679FDC546070A6EDCC7AAC3F9493E672` |
| `P11.2-WeaponGuard-final.log` | `WeaponGuard` | 12 | 0 | `AD9E84B18E143F2F3EBC9519E1192FAD070ED39A54EECA981B6D2B470AE29F16` |
| `P11.2-ActionLifecycle-final.log` | `ActionLifecycle` | 1 | 0 | `4C7179BB8FF0D4B24D5F685C303504FC72ED225A06F44C94CA8FAB840E94031A` |
| `P11.2-CombatCore-final.log` | `CombatCore` | 9 | 0 | `ED485D221FC6D89970BAF6364B1F820AF8AF66AE966EB4A211DA679B0D0153DD` |
| `P11.2-CombatRuntime-final.log` | `CombatRuntime` | 105 | 0 | `C39AB6169204E057A2270245E10477D352F09AA23DB89E10406F83C3B94E2D5F` |
| `P11.2-Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 473 | 0 | `17B3F738C19DF74617202D9BDEED4D6F7C0DE611FBBC00851AD4E1799C57D49C` |

每份最终日志都包含唯一目标 RunTests、原生 terminal queue-empty、selected Fail `0`、fatal/unhandled/ensure `0`，进程原生退出码均为 `0`。

```text
REGRESSION_MAP_JSON: PASS Rules=91
SELF_TEST: PASS 142/142
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=6 Logs=7
git diff --cached --check: PASS
BOUNDARY_SCAN: PASS hits=0
```

- mapping SHA-256：`A15A1B1FFF780AF254143A111EBB24507F9BA8676CA42B7CD9795683460F8B6D`；
- self-test SHA-256：`E71A81D0A58ED3B083B8B5F177DD728438C0B04C0478462672E5C4FADCAD6B00`。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development | Succeeded | 6 / 29.43s | 0 | `42763954497C857523C50B71C6FCA26A5C58F14BFCFF7F180AD69EA87F0A2676` |
| Game Development | Succeeded | 5 / 28.49s | 0 | `B4976FB2F58A3554D6AA264DEB599CB8D6C7593C9F9BDE211D654EE2F0C8CBB3` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`1489408` bytes，SHA-256 `FA3CC218B92F4A843FE4E5F9358EF8F63802374055559504CCA62F5CDC76221D`；
- `demo_map.exe`：`354086912` bytes，SHA-256 `128652480B69F81DFB2916780C87644E5CCD674CAB798AB8EEA178C9E1A4E016`。

## 9. 真实异常

最终源码的 UHT、C++、七组 Automation、changed-file gate、Editor 与 Game 构建全部成功，没有源码、测试、内存、页面文件、外层超时或 Win64 SDK 失败。

首次七组测试本身均 Success、原生退出码均为 `0`，但命令使用 `Automation RunTests <group>;Quit`，changed-file gate 将 `;Quit` 解析为测试组名的一部分，因此报告六个 required group 缺失并返回门禁失败。该问题只影响证据格式。修正为 canonical `Automation RunTests <group>` 并由 `-TestExit` 在队列清空后退出，随后完整重跑七组；最终表格和 SHA 均来自重跑结果，没有放宽 parser、测试或 required groups。

Automation 启动日志包含 UE 5.8 自带 UnifiedErrorTest 初始化噪声，以及非 Win64 平台 SDK metadata 无效信息；Win64 为 VALID，selected tests 全部 Success、队列正常清空、进程退出 `0`，不属于产品故障。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值契约、代码审查、无头 Automation、静态／路径门禁、Editor Development 与 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P11.3 建议建立唯一的产品方向采样适配边界：由现有实体／Actor 权威显式生成 GuardFacing 与 DirectionToThreat，再调用本阶段纯 evaluator；适配层不得从 HitNormal 猜符号，也不得复制 P11.0/P11.1 lifecycle。若优先接武器耐久，则必须通过现有 ShanmenItems durable prepare/commit/cancel 事务，不允许 direction evaluator 直接修改库存或耐久。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-2-weapon-guard-arc/Docs/Report/Dev.D.UE.0.0.10.P11.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p11-2-weapon-guard-arc/Docs/Log/Dev.D.UE.0.0.10.P11.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-2-weapon-guard-arc>
