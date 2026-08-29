# Dev.D.UE.0.0.10.P8.8.r0 Report

## 1. 结论

P8.8 已建立显式驱动的 Formation coverage baseline tracker，结论为 **PASS**。

新增 `Fdemo_mapShanmenFormationCoverageTracker`：`Prime` 建立第一份 valid immutable coverage baseline；`Advance(expectedBaselineId, currentCoverage)` 以 compare-and-swap 方式调用 P8.7 transition reducer，只有 reducer 成功时才原子替换 baseline；Area 或主体生命周期变化必须由调用方显式 `Rebase`。Tracker 不自行 Tick，不读取 World／Actor，也不施加 damage、Buff 或 effect。

最终 focused `4/4`、0.0.10 full `243/243`、changed-file gate、49/49 映射自测、Editor Development 与 Game Development 均通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. Tracker 命令模型

- `Prime(coverage)`：未初始化时建立 baseline；完全相同的重复命令返回 `PrimeReplayed`；活动 baseline 不同则以 `AlreadyPrimed` 失败关闭。
- `Advance(expected, current)`：要求 expected 与当前 baseline receipt identity 精确一致；P8.7 成功后才提交新 baseline。
- `Rebase(coverage)`：由外层明确承认 Area replacement、主体集合或生命周期边界，并替换 baseline。
- `Reset()`：幂等清除 baseline 与最近一次 replay evidence。
- `TryGetBaseline(out)`：只返回值拷贝，不向调用方暴露可能因后续命令失效的内部地址。

Tracker 是非 UObject 值层状态机；同一实例的命令必须由一个调用方串行化。

## 3. 原子性与失败围栏

`Advance` 在任何输入或 reducer 失败时都不修改 baseline，也不覆盖 replay slot：

- expected identity 无效、tracker 未 Prime、current coverage 无效分别独立拒绝；
- expected 与当前 baseline 不一致返回 `BaselineConflict`；
- P8.7 的 Area／subject-set 拒绝统一返回 `TransitionRejected`，并保留原 baseline；
- 只有 transition receipt 自验证成功后，baseline 与 replay evidence 才一起提交。

因此 stale caller、Area replacement 或主体集合漂移不能静默推进状态。

## 4. 有界幂等语义

Tracker 只保留最近一次成功 `Advance` 的命令身份与 transition receipt。对完全相同的 `(expectedBaselineId, currentCoverageId)` 立即重试时返回 `AdvanceReplayed`，receipt identity 与首次成功完全相同。

后续 `Advance` 一旦成功，更早的命令不再保留；旧 expected 会得到 `BaselineConflict`。该边界支持常见的“发送成功但回执丢失”重试，同时避免把 tracker 扩成无界历史账本。`Prime`、`Rebase` 与 `Reset` 会按各自语义清理 replay slot。

## 5. 自动化证据

两份日志均完成目标队列，Fail `0`、fatal／unhandled `0`，进程原生退出码 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationCoverageTracker` / `FormationCoverageTracker.log` | 4 | 0 | `F5E43EAE2CBED31AA0C29A8BBD84F26676EC35E3296D57F32071FC8B4E7EA1A2` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 243 | 0 | `30929123E200DE271CE4E00F772E685A02A77370B9AAAE6DF9D807C6DB773C52` |

Focused 四项覆盖：

1. Prime、Enter／Leave Advance、exact replay 与值拷贝 baseline 读取；
2. 未 Prime、无效 expected/current、stale conflict、Area／subject mismatch 的原子失败围栏；
3. 显式跨 Area Rebase、Rebase 后推进、Reset 与重复 Reset；
4. 连续两次 CAS 推进、旧命令冲突、最新命令稳定重放与最终 baseline。

完整 suite 从 P8.7 的 `239` 增至 `243`，此前测试全部继续通过。本轮没有测试或编译失败。

## 6. 改动—回归与静态门禁

新增 `FormationCoverageTracker` mapping rule，要求八组证据：Tracker、Transitions、WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment。

- regression map JSON：PASS，`44` rules；
- mapping self-test：`49/49 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=8 Logs=2`；
- map SHA-256：`4F112183071A13C52EB715FF6D32D43C85F5E73B518216A5594F83DA39B5A1B5`；
- self-test SHA-256：`EF8E3A80E3AC27B337CCD7C20D70A2CAB8CFFD295F8D1EAFA3E28A36B865A365`；
- production boundary scan：唯一文本命中是说明注释中的 `non-UObject`；代码没有 UWorld／AActor／UObject ownership、Tick／timer、RNG、damage／effect、AbilitySystem 或 item/profile subsystem 调用；
- `git diff --check` 与最终 staged `git diff --cached --check`：PASS。

## 7. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Native exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source rebuild（值拷贝 API 后） | Succeeded | 0 | 8.41s | 后续 final check 覆盖 |
| Editor final check | Succeeded / up to date | 0 | 0.93s | `BFAAE9C7FCFC39C71E2D6DC5F19AF7D5D9B56C79BF04EFA2C66F40C2B699B4A3` |
| Game final | Succeeded | 0 | 15.01s | `93C07287F6F0D868C40F2C74F4259864DC9D6BA6FE300E2305848B532D240DD5` |

- Editor module：`11160064` bytes，UTC `2026-08-29T19:00:24.8923593Z`，SHA-256 `C145B4BE0691FB0E1A781F93ECDB8126D8BD9CF6A8137B611D63C82D933F1050`；
- Game executable：`352355328` bytes，UTC `2026-08-29T19:03:24.1402567Z`，SHA-256 `AE2D874645527B3F8D84058BF7C151E5510FEE1BF2683FD14C48DD580D1F8A22`。

## 8. 权威、完整性与兼容性

P8.8 只拥有一份 coverage baseline 与最近一次 replay evidence：

- Area 与 membership 真值仍由 P8.5 提供；
- live World location 投影仍由 P8.6 提供；
- transition facts 仍由 P8.7 reducer 提供；
- sampling cadence、Actor lifecycle、tracker instance ownership 与 effect policy 继续后置；
- transition receipt 仍只是可重放事实，不是效果指令。

没有修改 P8.0—P8.7、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链，没有建立第二套 World／Actor／effect 系统。

## 9. 修改范围与 P/F 边界

新增：

- `demo_mapShanmenFormationCoverageTracker.h/.cpp`；
- `demo_mapShanmenFormationCoverageTrackerTests.cpp`。

更新 regression map、自测、本 Report 与同名 Development Log。长期未跟踪用户与 0.0.9B 工件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一步与 GitHub

P8.9 建议建立窄的、显式调用的 coverage coordinator：一次 caller command 先调用 P8.6 `Sample` 取得 current receipt，再按 mode 调用 P8.8 `Prime`／`Advance`／`Rebase`；coordinator 只组合既有结果，不自行 Tick、不发现 Actor、不施加 effect。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-8-formation-coverage-tracker/Docs/Report/Dev.D.UE.0.0.10.P8.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-8-formation-coverage-tracker/Docs/Log/Dev.D.UE.0.0.10.P8.8.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-8-formation-coverage-tracker>
