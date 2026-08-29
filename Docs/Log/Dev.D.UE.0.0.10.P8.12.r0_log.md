# Dev.D.UE.0.0.10.P8.12.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.12.r0`；
- 基线：`7a14d2ba96b1403ff93ddd60e1e314d7dd8637e1`（P8.11）；
- 分支：`agent/0.0.10-p8-12-formation-influence-reconciliation`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.12 为 Prime、Rebase、Reset 与 Terminal 建立独立 lifecycle reconciliation evidence，把完整 coverage scope 映射为 deterministic Apply/Remove intents；Advance 继续由 P8.11 transition planner 负责。

明确后置：Host orchestration、intent dispatch/ack ledger、正式 cadence、GAS/GameplayEffect executor、damage／Buff、AI、输入/UI 与正式阵法 content／数值。

## 设计决策

### 不伪造 transition

Prime 与 Clear 没有 previous/current transition facts；Rebase 允许 Area 或 subject set 变化，也不满足 P8.7 同 Area、同 subject set 契约。因此三类生命周期使用自己的 optional old/new scope evidence，不制造假 transition receipt。

### Rebase 分两类

相同 effect scope 使用 covered set difference，避免持续覆盖 subject 被无意义 Remove/Apply。Area 或 policy scope 改变时，即使 subject 相同也必须旧 Remove、新 Apply，因为 intent identity 已改变。

### authority 围栏双重化

Planner 拒绝跨 Run、Owner、Deployment Rebase。自审发现仅在工厂拒绝仍不足，因此将同一围栏下沉到 reconciliation evidence 派生与 Batch `IsValid()`，使手工构造也无法绕过。

### 单一 intent factory

P8.11 intent 增加公共 `Make(...)`，transition 与 lifecycle planner 复用同一 identity 算法。没有复制 hash namespace 或建立第二种 intent。

## 实现范围

新增生产与测试：

- `Source/demo_map/demo_mapShanmenFormationInfluenceReconciliationPlanner.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceReconciliationPlanner.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceReconciliationPlannerTests.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationInfluenceIntentPlanner.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

## 执行记录

1. 审查 tracker/coordinator 的 Prime、Advance、Rebase、Reset 结果与 Host terminal cleanup。
2. 确认 Advance 已有完整 P8.7 evidence，非 Advance 生命周期必须独立建模。
3. 定义 immutable scope、mode-shaped optional evidence、reconciliation ID 与 batch seal。
4. 实现 Prime Apply、same-scope Rebase set-difference、scope replacement、Reset/Terminal Remove。
5. 将 P8.11 intent identity 构造抽成统一 factory。
6. 新增五项 lifecycle、authority、replay、no-op 与 mutation 测试。
7. 首次 Editor source build、focused `5/5`、full `258/258` 均成功。
8. 自审发现 Batch `IsValid()` 需独立执行跨 authority 围栏；修正后 Editor rebuild 成功，并重新生成 focused/full 验收日志，仍为 `5/5` 与 `258/258`。
9. regression map、自检、changed-file gate、静态门禁与最终 Editor/Game 全部通过。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

最终验收证据：

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `FormationInfluenceReconciliation.log` | `Shanmen.0_0_10.Product.FormationInfluenceReconciliation` | 5 | 0 | 0 | 1 | `713B8068608A4D1EC428C132AC56AFE5FCC1DC501B9F86A5D6F3FE24464FED36` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 258 | 0 | 0 | 1 | `3012C6EE9A684CFA081841253729093392940593D75779177655D832CFD68478` |

两份日志均 fatal／unhandled `0`。引擎启动时 UnifiedError self-test 的固定 `Condition failed` 与此前日志一致，不属于项目 Automation case。

## 测试内容

### PrimeAndReplay

- Inside 与 Boundary 都 Apply，Outside 不输出；
- 所有 intent cause 指向 reconciliation evidence；
- exact replay 保持 evidence 与 batch identity。

### SameScopeRebase

- 持续 covered subject 不输出；
- 离开 coverage 只 Remove，新进入 coverage 只 Apply；
- canonical Remove 在 Apply 前。

### ScopeReplacement

- 同 Run/Owner/Deployment 下改变 Area identity；
- 同 subject 的旧 scope Remove 与新 scope Apply 保持不同 intent identity。

### ResetAndTerminal

- 两种 clear 都撤销全部 covered subject；
- mode 进入 evidence identity；
- 全 Outside scope 返回 valid no-op。

### FencesAndSeal

- missing source、invalid/mismatched scope、invalid clear mode、cross-deployment Rebase 拒绝；
- count、order、evidence、scope shape、nested intent 与 batch ID 漂移拒绝。

## Changed-file regression gate

新增 Reconciliation rule，并与修改后的 InfluenceIntents rule 求并集。Focused 只提供 Reconciliation 精确证据，full suite 覆盖十一组依赖契约。

```text
REGRESSION_MAP_JSON: PASS Rules=47
SELF_TEST: PASS 55/55
REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=11 Logs=2
```

- map SHA-256：`3A6CF4240AEF0E6279D2D9CA703792E8C0F171D3D6F12AA13C959D9CD7DCEF7C`；
- self-test SHA-256：`CE9E489007653CDF2025FA61C8CC091C016D4430472DCAF9C095C54C1E3F029F`。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor initial source build | Succeeded / 7 actions | 0 | 14.84s | `327132795F8FF3DC45201D2DE974B62428C10EE364914AB34AB82716460CB243` |
| Editor post-review rebuild | Succeeded / 4 actions | 0 | 5.34s | `EFD72865FA329CF21BC8749255F62802ED00C7D70AB0F915C2F90B384D9EDAE1` |
| Editor final check | Succeeded / up to date | 0 | 0.88s | `ADC813D98BF3A79611C561F928E5942E4040F7E01153E782D277306EC83CEF6D` |
| Game final | Succeeded / 6 actions | 0 | 19.44s | `73CDBCD6BE449FE0760790506B3695AEAFF52EDB65058DA8A3B11DA641883006` |

- Editor module：`11320320` bytes，UTC `2026-08-29T21:11:35.7758833Z`，SHA-256 `D46F77CFDFC2893E502DF886D8EA128524B0EF760AC9B0A14E1C7598C716177C`；
- Game executable：`352480256` bytes，UTC `2026-08-29T21:13:32.6802366Z`，SHA-256 `25594E6C6C3C860BD0B16B6E747160DA7936A8F62F1C3D4ECD9083CF01B3FC97`。

## 静态、兼容性与 P/F 边界

- map JSON、55/55 mapping self-test、changed-file gate 与 `git diff --check`：PASS；
- production boundary scan 对 World/Actor/object、Tick/timer、Actor discovery、SpawnActor、damage/effect、AbilitySystem、RNG、async 与 persistence API：0 matches；
- 没有增加第二套 host、tracker、coordinator、effect executor、inventory 或 persistence authority；
- 长期未跟踪用户和 0.0.9B 文件未修改、未 stage；
- 未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.13 建议让现有 ProductHost 编排 transition/lifecycle planners 并返回完整 intent evidence，同时建立 host-owned idempotent dispatch/ack ledger；依旧不执行真实效果。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-12-formation-influence-reconciliation/Docs/Report/Dev.D.UE.0.0.10.P8.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-12-formation-influence-reconciliation/Docs/Log/Dev.D.UE.0.0.10.P8.12.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-12-formation-influence-reconciliation>
