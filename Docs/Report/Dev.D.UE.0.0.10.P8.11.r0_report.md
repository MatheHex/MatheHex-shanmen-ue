# Dev.D.UE.0.0.10.P8.11.r0 Report

## 1. 结论

P8.11 已建立纯值 `Fdemo_mapShanmenFormationInfluenceIntentPlanner`，结论为 **PASS**。

Planner 消费 P8.5 immutable Area、P8.7 coverage transition receipt、稳定 source EntityId 与显式 authored influence policy，把 `Entered` 映射为 `Apply`、`Left` 映射为 `Remove`；`StayedCovered` 与 `RemainedOutside` 生成一份可验证的 no-op batch。它只发布后续效果权威可消费的确定性意图，不施加 GAS、伤害、Buff、数值、持续时间或堆叠规则。

最终 focused `4/4`、0.0.10 full `253/253`、changed-file regression gate、mapping self-test、Editor Development 与 Game Development 全部通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 通用 influence intent envelope

新增的 immutable intent 固定以下身份：

- `IntentId`、Run、Owner、source entity、Deployment、Area 与 subject entity；
- authored policy definition 与 influence definition；
- `Apply`／`Remove` operation；
- exact `CauseId`；P8.11 中它必须等于对应 transition fact ID；
- versioned content stamp。

Intent 不携带 magnitude、duration、stacking、GameplayEffect class 或 executor。后续 baseline、lifecycle 或 effect authority 可以复用 envelope，但必须发布自己的自验证 batch，不能把不同来源伪装成 P8.11 transition evidence。

## 3. 自包含 transition batch

每份 batch 内嵌完整 Area、source、policy 与 transition receipt，并封印：

- `BatchId`；
- canonical intent 顺序；
- `ApplyCount`／`RemoveCount`；
- 每个 intent 与 fact 的 subject、operation、cause、scope 和 content 对应关系；
- 全部 intent ID 唯一性。

Batch 必须满足 `ApplyCount == EnteredCount`、`RemoveCount == LeftCount`，且 intent 数量等于两者之和。任何 count、operation、cause、顺序、policy、nested evidence 或 ID 漂移都会使 `IsValid()` 失败。

## 4. 确定性与失败关闭

Intent ID 使用 `Shanmen.Formation.InfluenceIntent.r1` canonical namespace；Batch ID 使用 `Shanmen.Formation.InfluenceTransitionBatch.r1`。精确重放保留相同 ID，改变 influence definition 会改变 batch identity。

Planner 对以下输入分别失败关闭：invalid Area、缺失 source EntityId、malformed policy、invalid transition、foreign Area 与 content mismatch。拒绝结果不发布可验证 batch；输出在返回前再次执行完整 deterministic self-validation。

## 5. 明确未承担的权威

本轮没有：

- 自动 Tick、timer、World/Actor discovery 或 cadence；
- GAS、GameplayEffect、AbilitySystem、damage、Buff 或数值结算；
- inventory、item、AI、输入、UI 或 persistence；
- 初始 Prime、Rebase、Reset/terminal 的 effect reconciliation；
- effect dispatch、ack、retry 或 applied-state ledger。

因此本阶段证明“coverage delta 可以形成稳定、可审计的操作意图”，不宣称任何产品效果已被应用。

## 6. 自动化证据

两份 Automation 日志均为一次命令、一次 queue completion、Fail `0`、fatal／unhandled `0`、进程原生退出码 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationInfluenceIntents` / `FormationInfluenceIntents.log` | 4 | 0 | `3B44AEBCC2A3E2AE3712ABEFD0F703E3B26F5577AD9B7CB811894D0AF4386D9C` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 253 | 0 | `517D4842EC47E297E4CB2E050C78C712341075B185E433E1E883EEFB7C69F550` |

新增四项测试：

1. mixed Entered／Left／Stay／Outside transition 只形成一个 Apply 与一个 Remove；
2. exact replay、policy identity 分离与 unchanged no-op batch；
3. Area、source、policy、transition、scope 与 content 输入围栏；
4. count、operation、cause、order、policy、nested evidence、batch ID 与 standalone intent mutation seal。

完整 suite 从 `249` 增至 `253`。

## 7. 改动—回归与静态门禁

新增 `FormationInfluenceIntents` mapping rule，要求八组证据：InfluenceIntents、CoverageTransitions、WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment。Focused 提供精确子组证据，full suite 覆盖全部上游契约。

```text
REGRESSION_MAP_JSON: PASS Rules=46
SELF_TEST: PASS 53/53
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=8 Logs=2
```

- map SHA-256：`DD7AE44544B1EAE717B960C2B145A1914C780832ABE1E98BA4AE323FB93526EA`；
- self-test SHA-256：`A75BB903625DCCD0DE2EDB725F6C5DDC59ACFFEB7F944827ACB9E09C4A978551`；
- production boundary scan 对 World/Actor/object、Tick/timer、Actor discovery、SpawnActor、damage/effect、AbilitySystem、RNG、async 与 persistence API 的匹配为 `0`；
- `git diff --check`：PASS。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Native exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source build | Succeeded / 5 actions | 0 | 9.67s | `2AEC702F5670C163334E3348AC10A27EAF16D860E4C1F9684A884B70DFB88E4E` |
| Editor final check | Succeeded / up to date | 0 | 0.91s | `62C61560BA67F3097F835E550E9C616C31E17CBEC247387943CB61F02465D345` |
| Game final | Succeeded / 4 actions | 0 | 20.72s | `A666F01B71143FA298383402D4533DDC1F215843D3D677B81EFC71F6154B90DD` |

- Editor module：`11267584` bytes，UTC `2026-08-29T20:37:42.4502417Z`，SHA-256 `889CFE50AF8A067DBAF68178BAB42CF7B3E8955C2BCEDEAC7ED8B22CA2614834`；
- Game executable：`352442368` bytes，UTC `2026-08-29T20:40:18.2444790Z`，SHA-256 `6B54735D60DEB379917A0BEE77270AE77DE2A10577C7A0CA4BEF49801A5CFAB0`。

三次构建均首次成功，没有源码失败或环境重试。

## 9. 修改范围与 P/F 边界

新增：

- `demo_mapShanmenFormationInfluenceIntentPlanner.h/.cpp`；
- `demo_mapShanmenFormationInfluenceIntentPlannerTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

没有修改 GameplayTags、Content、schema、Build.cs、GameMode、Host、WorldAdapter、输入、UI、物品权威或旧产品链。长期未跟踪用户与 0.0.9B 文件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 10. 下一步与 GitHub

P8.12 建议补齐显式 influence reconciliation：为 Prime baseline、Rebase 与 Reset/terminal 建立各自的 immutable evidence batch 与 deterministic apply/remove intents。先冻结完整生命周期的撤销语义，再把 intent dispatch ledger 接入现有 product host；仍不自动 cadence，也不直接接 GAS。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-11-formation-influence-intents/Docs/Report/Dev.D.UE.0.0.10.P8.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-11-formation-influence-intents/Docs/Log/Dev.D.UE.0.0.10.P8.11.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-11-formation-influence-intents>
