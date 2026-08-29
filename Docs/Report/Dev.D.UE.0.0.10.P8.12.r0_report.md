# Dev.D.UE.0.0.10.P8.12.r0 Report

## 1. 结论

P8.12 已补齐 Formation influence 的非 Advance 生命周期 reconciliation，结论为 **PASS**。

新增纯值 `Fdemo_mapShanmenFormationInfluenceReconciliationPlanner`：Prime 对当前 covered 集合发布 `Apply`；同 effect scope 的 Rebase 只发布覆盖集合差；Area/policy scope 更换时先撤销旧 covered scope，再施加新 covered scope；Reset 与 Terminal 对旧 covered 集合发布 `Remove`。全部输出都形成自包含、确定性、自验证 batch。

最终 focused `5/5`、0.0.10 full `258/258`、changed-file regression gate、mapping self-test、Editor Development 与 Game Development 全部通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 生命周期证据形状

`Fdemo_mapShanmenFormationInfluenceReconciliationBatch` 使用显式 optional scope：

- Prime：只有 `Current`；
- Rebase：同时存在 `Previous` 与 `Current`；
- Reset／Terminal：只有 `Previous`。

每个 scope 内嵌 immutable Area、authored policy 与完整 coverage receipt。Coverage 必须属于该 Area，policy content stamp 必须与 Area 一致。缺失、额外或部分伪造的 scope 不能通过 `IsValid()`。

## 3. Prime 与 Clear 语义

Prime 按 coverage receipt 的 canonical membership 顺序，对 Inside 与 Boundary subject 发布 Apply；Outside 不产生意图。精确重放保持相同 reconciliation ID、intent IDs 与 batch ID。

Reset 与 Terminal 都撤销旧 scope 的全部 covered subject，但 mode 进入 evidence identity，因此两者不会共享 reconciliation ID。全 Outside 的合法 scope 返回 sealed no-op batch，不把“无需操作”伪装成失败。

## 4. Rebase 语义

同一个 Run、Owner、Deployment、Area 与 policy scope 内，Rebase 做 covered set difference：

- previous covered、current uncovered/不存在：Remove；
- current covered、previous uncovered/不存在：Apply；
- 两侧都 covered：无操作。

若 Area identity 或 policy identity/content 改变，则旧 effect scope 与新 scope 不可视为同一个实例：先按 previous canonical order Remove 全部旧 covered，再按 current canonical order Apply 全部新 covered。

Rebase 严禁跨 Run、Owner 或 Deployment。该围栏同时存在于 Planner、evidence ID 派生与 Batch `IsValid()`，不能通过手工构造 batch 绕过。

## 5. 确定性与职责边界

Reconciliation evidence 使用 `Shanmen.Formation.InfluenceReconciliationEvidence.r1`，Batch 使用 `Shanmen.Formation.InfluenceReconciliationBatch.r1`。每个 intent 的 `CauseId` 精确等于 reconciliation ID；Remove 始终在 Apply 前，所有 IDs、计数、顺序、scope 与 nested intent 都被 mutation seal 覆盖。

P8.11 intent 增加统一 `Make(...)` factory，transition planner 与 lifecycle planner 共用同一 identity 实现，避免复制第二套 intent 哈希算法。

本轮不修改 Host、tracker 或 coordinator 状态，不 dispatch/ack 意图，不接 GAS、GameplayEffect、damage、Buff、数值、duration 或 stacking，也不自动 Tick/timer。

## 6. 自动化证据

修正后两份验收日志均为一次命令、一次 queue completion、Fail `0`、fatal／unhandled `0`、进程原生退出码 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationInfluenceReconciliation` / `FormationInfluenceReconciliation.log` | 5 | 0 | `713B8068608A4D1EC428C132AC56AFE5FCC1DC501B9F86A5D6F3FE24464FED36` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 258 | 0 | `3012C6EE9A684CFA081841253729093392940593D75779177655D832CFD68478` |

新增五项测试：

1. Prime covered apply 与 exact replay；
2. 同 scope Rebase set-difference；
3. Area identity replacement 的 old Remove + new Apply；
4. Reset／Terminal identity 分离与 no-op clear；
5. source/scope/mode/authority 围栏，以及 count/order/evidence/shape/intent/batch mutation seal。

完整 suite 从 `253` 增至 `258`。

## 7. 改动—回归与静态门禁

新增 Reconciliation rule，并因 P8.11 factory 修改同时命中 InfluenceIntents rule。两条规则的并集要求十一组证据：Reconciliation、InfluenceIntents、Coordinator、Tracker、Transitions、WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment。

```text
REGRESSION_MAP_JSON: PASS Rules=47
SELF_TEST: PASS 55/55
REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=11 Logs=2
```

- map SHA-256：`3A6CF4240AEF0E6279D2D9CA703792E8C0F171D3D6F12AA13C959D9CD7DCEF7C`；
- self-test SHA-256：`CE9E489007653CDF2025FA61C8CC091C016D4430472DCAF9C095C54C1E3F029F`；
- production boundary scan 对 `UWorld`、`AActor`、`UObject`、Tick/timer、Actor discovery、SpawnActor、damage/effect、AbilitySystem、RNG、async 与 persistence API 的匹配为 `0`；
- `git diff --check`：PASS。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Native exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor initial source build | Succeeded / 7 actions | 0 | 14.84s | `327132795F8FF3DC45201D2DE974B62428C10EE364914AB34AB82716460CB243` |
| Editor post-review rebuild | Succeeded / 4 actions | 0 | 5.34s | `EFD72865FA329CF21BC8749255F62802ED00C7D70AB0F915C2F90B384D9EDAE1` |
| Editor final check | Succeeded / up to date | 0 | 0.88s | `ADC813D98BF3A79611C561F928E5942E4040F7E01153E782D277306EC83CEF6D` |
| Game final | Succeeded / 6 actions | 0 | 19.44s | `73CDBCD6BE449FE0760790506B3695AEAFF52EDB65058DA8A3B11DA641883006` |

- Editor module：`11320320` bytes，UTC `2026-08-29T21:11:35.7758833Z`，SHA-256 `D46F77CFDFC2893E502DF886D8EA128524B0EF760AC9B0A14E1C7598C716177C`；
- Game executable：`352480256` bytes，UTC `2026-08-29T21:13:32.6802366Z`，SHA-256 `25594E6C6C3C860BD0B16B6E747160DA7936A8F62F1C3D4ECD9083CF01B3FC97`。

所有构建均成功；post-review rebuild 是因自审后强化 Batch 跨 authority 自验证，并非源码或环境失败重试。

## 9. 修改范围与 P/F 边界

新增：

- `demo_mapShanmenFormationInfluenceReconciliationPlanner.h/.cpp`；
- `demo_mapShanmenFormationInfluenceReconciliationPlannerTests.cpp`。

更新：

- `demo_mapShanmenFormationInfluenceIntentPlanner.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

没有修改 GameplayTags、Content、schema、Build.cs、GameMode、Host、WorldAdapter、tracker、coordinator、输入、UI、物品或 persistence 权威。长期未跟踪用户与 0.0.9B 文件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build；未执行真实输入、截图、Smoke、Cook、Package 或产品回归。

## 10. 下一步与 GitHub

P8.13 建议把 P8.11 transition batch 与 P8.12 lifecycle batch 接入现有 ProductHost 的返回证据：Host 仍只编排并返回 intents，不执行效果；同时引入 host-owned idempotent dispatch ledger，冻结 accept/ack/replay/terminal drain 语义后再接真实 GAS executor。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-12-formation-influence-reconciliation/Docs/Report/Dev.D.UE.0.0.10.P8.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-12-formation-influence-reconciliation/Docs/Log/Dev.D.UE.0.0.10.P8.12.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-12-formation-influence-reconciliation>
