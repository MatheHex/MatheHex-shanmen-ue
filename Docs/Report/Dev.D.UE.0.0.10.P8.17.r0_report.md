# Dev.D.UE.0.0.10.P8.17.r0 Report

## 1. 结论

P8.17 已完成 formation influence 的窄 product runtime composition。Runtime 长期持有 P8.16 concrete lease executor，并在首次合法执行时冻结 `RunCorrelation + LedgerId`；ProductHost 继续由调用方持有，所有 intent order、acknowledgement、retry、seal 与 teardown 仍由原 Host／ledger contract 决定。

本阶段结论为 **PASS**：product runtime `4/4`、lease executor `4/4`、executor adapter `4/4`、influence Host `4/4`、`Shanmen.0_0_10` 完整回归 `278/278`、changed-file regression gate、自检 `63/63`、静态边界扫描、Editor Development 与 Game Development 均成功。

## 2. 功能性

### 2.1 单步 product composition

- `TryExecuteOne` 每次接受一个 caller-owned `IntentId + AttemptId` command；
- 每次调用最多委托 P8.15 adapter 一次，不循环 peek、drain、retry、seal 或 teardown；
- concrete executor 作为 runtime 私有成员保持稳定生命周期；
- Runtime 不持有 ProductHost、World、Actor 或 UObject 指针。

### 2.2 immutable Host binding

- 首次可执行 Host 冻结完整 `Fdemo_mapShanmenRunCorrelation` 与 Host ledger ID；
- 后续不同 correlation 或不同 ledger 在进入 executor 前 fail-closed；
- 第一次 out-of-order command 不调用 executor，并回滚临时 binding；
- runtime 只把 identity fence 绑定到 Host evidence，不复制 Host state 或 ledger entries。

### 2.3 late-attachment safety

- unbound runtime 只可绑定到 `pending intent count == total intent count` 的 ledger；
- 因此已有 success acknowledgement 的 Host 会被拒绝，避免 runtime 缺失此前已生效的 semantic lease；
- retry-only history 没有发生 semantic mutation，仍可通过 Host exact retry replay 安全绑定；
- retry replay 不调用 concrete executor，随后 caller 提供的新 attempt 才执行真正 Apply。

### 2.4 lifecycle 与 replay

- exact success replay 由 Host ledger直接恢复，owned executor attempt count 不增加；
- Prime Apply 与 Terminal Remove 通过同一 runtime 单步执行；
- active lease 清空后 Host 才 seal／teardown；
- seal 与 teardown 后 exact Remove replay 仍不重入 executor。

## 3. 完整性与安全边界

本阶段明确未实现：

- 自动 drain loop、后台 retry、Tick、timer、cadence 或 async；
- GameplayEffect、GAS、magnitude、duration、stacking 或正式 Buff 数值；
- Actor／World discovery、spawn、对象绑定或 component mutation；
- ProductHost ownership、复制 ledger、第二套 pending queue 或 seal authority；
- persistence、SaveGame、ProfileRepository 或 runtime reconstruction；
- 输入、UI、AI 与正式阵法 content。

新生产文件对 `UWorld`、`AActor`、`UObject`、AbilitySystem／GameplayEffect、timer、async、RNG、spawn、damage 与 persistence API 均为 `0` matches。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceProductRuntime.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceProductRuntime.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

ProductHost、dispatch ledger、P8.15 adapter 与 P8.16 lease executor 生产代码均未修改。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.17-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 0 | 1 | `2CE1A2426B12BAE929EE22FA7004173E3112CD99891C440BB06B485D1483A910` |
| `P8.17-FormationInfluenceLeaseExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor` | 4 | 0 | 0 | 1 | `8A8CC9CDF80E56C28CDAC980FF923650D8C33E90D6E252B112D7CF688F1FE687` |
| `P8.17-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 0 | 1 | `3D2F0C8A2AA5189D47D84404FDF6E82DE54B0E4E1FEBF0D576734A44553B6955` |
| `P8.17-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 0 | 1 | `46AA8D150B7D235A496BFFA894E9F01A99E6B8EEE18E55CC3BFD381A6976204A` |
| `P8.17-Shanmen-full-final.log` | `Shanmen.0_0_10` | 278 | 0 | 0 | 1 | `AD07B56C02513915A9839350412DFCE4967745915F9EC940C7C68DA0979AC4D6` |

五份最终日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 runtime focused case：

1. `SingleStepAndReplay`：首次 out-of-order 不留 binding；两条 pending intent 必须两次显式调用，exact replay 不重入 executor；
2. `BindingAndCorrelationFence`：foreign Host／ledger 与 stale correlation 均在 mutation 前拒绝；
3. `LateAttachmentFence`：success history 拒绝，retry-only history允许 replay 后用新 attempt 继续；
4. `TerminalDrainReplay`：Apply／Remove、seal／teardown 与 sealed exact replay 完整闭环。

## 6. 首次失败与修正

本阶段没有 Automation case、源码编译或最终构建失败。新 runtime focused 首轮即 `4/4`。首轮后仅增加 retry-only late attachment 正向证据，用于证明 safety fence 只拒绝已发生 semantic mutation 的历史；产品实现未因失败而修改，增强后 focused 仍为 `4/4`、full 为 `278/278`。

## 7. Changed-file regression gate

新增 `FormationInfluenceProductRuntime` path rule，并把 runtime contract 加入 ProductHost、dispatch、executor adapter 与 lease executor 的 owned-contract 覆盖。最终结果：

```text
REGRESSION_MAP_JSON: PASS Rules=51
SELF_TEST: PASS 63/63
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=19 Logs=5
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=19 Logs=5
```

第一条 coverage 结果为 Source／Scripts gate；第二条为加入 Report／Log 后的最终 exact-staged gate。

- mapping SHA-256：`E6ED280705479499B1E0407C3AEC514C6B4A3968CE12ACADC16950FCD818ACEF`；
- self-test SHA-256：`641BB8076A3B54706643F966B9E6F5CC65451607F1C25DD5A03607C7A7127E76`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | UBT evidence SHA-256 |
|---|---|---:|---|
| Editor initial implementation | Succeeded / 5 actions / 16.35s | 0 | console evidence verified |
| Editor strengthened tests | Succeeded / 4 actions / 7.08s | 0 | console evidence verified |
| Editor final | Succeeded / up to date / 0.92s | 0 | `F6AF8ACEBFC6D9B0C7BFE1A185950CE14BF18BD98F4E359079292AB165DF21EC` |
| Game final | Succeeded / 4 actions / 24.24s | 0 | `6FF61D3D21E416AD70D0BE1BC3F79342321A0A3BAAC8BDF77009A28B08287737` |

- Editor module：`11522048` bytes，SHA-256 `CCC5A79E4BDC500B3C88CF91A902E120B5DAE4E19E1C443D933EB27C9A67A0C2`；
- Game executable：`352660480` bytes，SHA-256 `F95DAD5AEAEAD0DE008924997F2CFDBE5AC317473043A148BB89FE407E380D30`。

## 9. 兼容性与工作区保护

- runtime 只调用 P8.15 public adapter，没有 friend、mutable ledger 或 Host pointer；
- P8.14 Host、P8.15 adapter 与 P8.16 lease semantics 的 focused groups均保持 `4/4`；
- 直接注入其它 executor 的旧调用路径保持可用；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 7 个文件。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与一次必要的最终 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.18 在 runtime 前增加纯值 execution command router：把 caller `RequestId` 确定性冻结到当时 canonical pending `IntentId + AttemptId`，支持 exact request replay并拒绝 request payload conflict；仍不自动 drain、不持有 Host，也不接 Actor／GAS。
