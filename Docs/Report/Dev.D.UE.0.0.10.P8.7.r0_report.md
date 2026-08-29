# Dev.D.UE.0.0.10.P8.7.r0 Report

## 1. 结论

P8.7 已建立纯 Formation coverage transition reducer，结论为 **PASS**。

新增 `Fdemo_mapShanmenFormationCoverageTransitionReducer`：只消费同一 P8.5 Area、相同 canonical subject set 的前后 immutable coverage receipts，输出逐主体 transition facts 与完整 batch receipt。该层不读取 World、Actor、Registry，不拥有 baseline 或 cadence，也不施加 damage、Buff、effect 或生命周期动作。

最终 focused `4/4`、0.0.10 full `239/239`、changed-file gate、47/47 映射自测、Editor Development 与 Game Development 均通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. Transition 语义

P8.5 已明确 Boundary 属于 covered，因此 P8.7 的四态事实为：

- `Entered`：Outside → Boundary／Inside；
- `StayedCovered`：Boundary／Inside → Boundary／Inside；
- `Left`：Boundary／Inside → Outside；
- `RemainedOutside`：Outside → Outside。

`GetChangedCount()` 只统计 Entered + Left。Boundary 与 Inside 的内部切换仍保留在前后 membership evidence 中，但不会伪装成进入或离开。

## 3. Area 与主体生命周期边界

Reducer 要求：

- Previous 与 Current coverage receipts 都必须自验证；
- 两者必须属于同一个 exact AreaId；
- 两者的 canonical subject set 必须完全相同。

Area replacement 不等于 coverage movement；主体缺失也不能自动推断为 Leave，因为它可能只是未采样、Actor 销毁或筛选策略变化。上述情况分别以 `AreaMismatch` 或 `SubjectSetMismatch` 失败关闭，要求外层显式处理 rebase／lifecycle。

## 4. Immutable evidence 与确定性身份

Batch receipt 保留完整 PreviousCoverage 与 CurrentCoverage，并为每个 canonical subject 生成 fact。Fact identity 绑定 AreaId、SubjectEntityId、前后 membership receipt IDs 与 transition kind；batch identity 再绑定前后 coverage IDs、四类计数与所有 fact IDs。

`IsValid()` 会重新验证：

- 两份嵌套 coverage evidence；
- subject 对位与 fact canonical order；
- transition kind 是否由前后 `IsCovered()` 唯一推导；
- 四类计数守恒；
- fact 与 batch deterministic IDs。

因此 kind、计数、fact 顺序、嵌套位置或任一 identity 漂移都不能继续冒充有效 receipt。

## 5. 自动化证据

两份日志均为 one command、one queue-empty、Fail `0`、fatal／unhandled／ensure `0`，进程原生退出码 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationCoverageTransitions` / `FormationCoverageTransitions.log` | 4 | 0 | `12D879181042AF56A532AC695702E173979A13A06F83B3B87F499E7C6E04D1E9` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 239 | 0 | `DA6C2BD5CDE3D74E09B9135FE6B968E627D19694136E0842E6651BDC3CD37D35` |

四项 focused 测试覆盖：

1. Outside／Boundary／Inside 的全部九种前后组合；
2. shuffled query source、canonical fact order、完整 evidence 保留与 replay-stable identity；
3. invalid previous/current、Area replacement、主体缺失与主体替换围栏；
4. count、kind、fact ID、fact order 与嵌套 membership 变异封印。

完整 suite 从 P8.6 的 `235` 增至 `239`，此前测试全部继续通过。本轮无测试或编译失败。

## 6. 改动—回归与静态门禁

新增 `FormationCoverageTransitions` mapping rule，要求七组证据：Transitions、WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment。

- regression map JSON：PASS，`43` rules；
- mapping self-test：`47/47 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=7 Logs=2`；
- map SHA-256：`6FDD8896C7A1EF1ED27C0F6D3D32E18437FBC32CC80DD69F1B043F09F753B233`；
- self-test SHA-256：`E66A5CD09BD6E1DA66695510DD79F2D3A6DAF3D805807421C147B32EA24BCF4C`；
- production boundary scan：无 UWorld、AActor、UObject ownership、Tick／timer、RNG、damage／effect、input／UI 或 item subsystem；
- `git diff --check` 与最终 staged `git diff --cached --check`：PASS。

## 7. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Native exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source build | Succeeded | 0 | 11.28s | 后续 final check 覆盖 |
| Editor final check | Succeeded / up to date | 0 | 0.91s | `7E4426BAFB43599E65A733E9B7CE08D5C09F194E1C5459FCC6EC774ED13B7A1C` |
| Game final | Succeeded | 0 | 15.66s | `AE7591BB2B8E4EB81814B3F367B1D0AF785E31E74934F96C4A8EB7E302EDA3EE` |

- Editor module：`11123200` bytes，UTC `2026-08-29T18:27:34.8998746Z`；
- Game executable：`352324608` bytes，UTC `2026-08-29T18:30:15.4512542Z`。

## 8. 权威、完整性与兼容性

P8.7 不拥有 Area、World location、subject discovery、baseline 或 effect authority：

- Area 与 membership 真值仍由 P8.5 提供；
- live location 投影仍由 P8.6 提供；
- baseline 存储、rebase、Actor 生命周期与采样 cadence 后置；
- transition facts 只是可重放证据，不是效果指令。

没有修改 P8.0—P8.6、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链，没有建立第二套 World／Actor／effect 系统。

## 9. 修改范围与 P/F 边界

新增：

- `demo_mapShanmenFormationCoverageTransitionReducer.h/.cpp`；
- `demo_mapShanmenFormationCoverageTransitionReducerTests.cpp`。

更新 regression map、自测、本 Report 与同名 Development Log。长期未跟踪用户与 0.0.9B 工件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一步与 GitHub

P8.8 建议建立显式驱动的 coverage tracker：`Prime` 保存第一份 valid coverage baseline，`Advance` 调用 P8.7 并原子替换 baseline，Area／subject lifecycle 变化必须显式 `Rebase`；tracker 仍不自行 Tick 或施加 effect。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-7-formation-coverage-transitions/Docs/Report/Dev.D.UE.0.0.10.P8.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-7-formation-coverage-transitions/Docs/Log/Dev.D.UE.0.0.10.P8.7.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-7-formation-coverage-transitions>
