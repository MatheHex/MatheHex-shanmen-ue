# Dev.D.UE.0.0.10.P8.14.r0 Report

## 1. 结论

P8.14 已把 P8.11 coverage transition planner、P8.12 lifecycle reconciliation planner 与 P8.13 dispatch ledger 接入既有 Formation ProductHost。Host 现在以单一事务编排 sample／tracker／planner／ledger，并返回不可变 coverage、plan、dispatch、acknowledgement 与 seal 证据；任何中间失败都不会推进 Host、tracker 或 ledger 状态。

本阶段结论为 **PASS**：新 Host focused `4/4`、既有 ProductHost `6/6`、`Shanmen.0_0_10` 完整回归 `266/266`、changed-file regression gate、静态边界扫描、Editor Development 与 Game Development 均成功。

## 2. 功能性

### 2.1 Host-owned 原子编排

- `Prime`：先提交 coverage baseline，再由 Prime reconciliation 生成 Apply intents 并进入 Host 私有 ledger；
- `Advance`：要求保留同一 policy，以 tracker transition receipt 生成 Entered／Left 对应 intents；
- `Rebase`：显式允许 policy 替换，以 previous/current scope 生成 Remove + Apply reconciliation；
- 三条路径均先在 Host 副本中完成 coverage、planner、dispatch 与 `IsValid()`，全部成功后才替换正式 Host；
- raw coverage API 在 influence authority 建立后被 fence，调用方不能绕过 planner／ledger 单独推进 baseline。

### 2.2 lifecycle reconciliation

- `Reset` 原子执行 coverage reset 与 Remove dispatch，清除 active scope，并保留 exact reset replay evidence；
- `Terminal` 显式发布 Remove intents，但在成功 acknowledgement 与 ledger seal 前不允许 End／Cancel teardown；
- teardown 成功后仍可 exact replay Terminal plan、acknowledgement 与 End；
- policy drift、stale correlation、planner 拒绝、dispatch 拒绝和未完成 terminal drain 均 fail-closed。

### 2.3 acknowledgement 与 seal

- Host 只暴露 pending intent 的只读 peek 与 `const` ledger evidence；
- retryable failure 由 Host 包装并保留 pending，exact retry 返回 replay；
- 后续 success acknowledgement 清除 pending；
- Reset 或已准备 Terminal 的 drained ledger 才可 seal；
- active scope 未准备 Terminal 时禁止 seal，防止 World teardown 越过 influence 清理。

## 3. 完整性与安全边界

本阶段只建立 Host orchestration 与可审计 delivery evidence，明确未实现：

- GameplayEffect／GAS handle 或真实 effect application；
- magnitude、duration、stacking、damage、Buff 数值解释；
- executor adapter、cadence、Tick、timer 或异步任务；
- 新的 Host、tracker、planner 或 executor authority；
- persistence、SaveGame、ProfileRepository；
- 输入、UI、AI 与正式阵法 content。

ProductHost 原本即负责 World／Actor placement 与 coverage，因此不把既有 `UWorld`／`AActor` 依赖误报为新增边界污染。针对本轮禁止引入的 `GameplayEffect`、`AbilitySystem`、`ApplyDamage`、RNG、async、SaveGame 与 `ProfileRepository` 扫描为 `0` matches。

## 4. 修改范围

更新：

- `Source/demo_map/demo_mapShanmenFormationProductHost.h`；
- `Source/demo_map/demo_mapShanmenFormationProductHost.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

未修改 P8.11/P8.12/P8.13 的 planner／ledger 实现、World adapter、runtime module、库存或存档权威。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.14-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 0 | 1 | `246095C99E01B6EDE365A0C844D292B738A32584240B8D75966FAB0C795A9B34` |
| `P8.14-FormationProductHost-final.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 6 | 0 | 0 | 1 | `CCD167D180688B29467B3446DC69CC5068A9DEECA5BD1355404F09C0F5349AD4` |
| `P8.14-Shanmen-full-final.log` | `Shanmen.0_0_10` | 266 | 0 | 0 | 1 | `6D319EBAB9468CEB56026D7A9B2AE9D28A290B44A85E6FA2D3462C04FD1E7A99` |

三份最终日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项新 focused case：

1. `PrimeAdvanceReplay`：Prime Apply、Advance Left Remove、exact replay 与 raw coverage fence；
2. `PolicyRebase`：显式 policy replacement、Remove + Apply、旧 policy Advance 拒绝且状态不推进；
3. `RetryResetSeal`：retry replay、后续 success、Reset Remove、reset replay、seal 与 teardown；
4. `TerminalDrainGate`：Terminal 前、pending 时与 seal 前均禁止 End，drain + seal 后允许 teardown 和全部 exact replay。

## 6. 首次失败与修正

本阶段没有 Automation case、源码编译或最终构建失败。实现完成后的静态自查在首次执行新测试前修正了一处测试观察时点：`TerminalDrainGate` 现在在 blocked End 后立即冻结 Session state，而不是在后续成功 End 后再读取同一对象。该修正只避免测试自身时序造成假阴性，不改变产品代码。

## 7. Changed-file regression gate

Formation ProductHost path rule新增 Host orchestration 及其全部直接 owned contracts，最终要求十六组契约证据。

```text
REGRESSION_MAP_JSON: PASS Rules=48
SELF_TEST: PASS 57/57
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=16 Logs=3
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=16 Logs=3
```

第一条 coverage 结果为 Source／Scripts gate；第二条为加入 Report／Log 后的最终 exact-staged gate。

- mapping SHA-256：`538E9BC9CEBC43E38F207E100D6D8C602055C30BBC927BB68CC7609622C268B1`；
- self-test SHA-256：`6823D0927413A4DFE29E8B1D04AB2C9179383DE25B9C77252FEC3A4AB2667BF9`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | UBT evidence SHA-256 |
|---|---|---:|---|
| Editor initial implementation | Succeeded / 5 actions / 16.74s | 0 | console evidence verified |
| Editor after focused tests | Succeeded / 4 actions / 5.96s | 0 | console evidence verified |
| Editor final | Succeeded / up to date / 0.96s | 0 | `376269C5757E07BF63F89D8CB9A2F4291DE329CC1D43C4B1FCDFAA9BC70839C4` |
| Game final | Succeeded / 4 actions / 20.68s | 0 | `96BF9715C89C431E76C41925BCB57E0BD02CB3A88DBCA023095CB0AF17072BEB` |

- Editor module：`11431424` bytes，SHA-256 `5843D332144DCCD335525B318B9FC19FC5E3E0B16C46B02F49AA2BAE56A9A15B`；
- Game executable：`352572416` bytes，SHA-256 `EDB02966F0A1DB6A938DC82BC36AB137DE6FDFDE2F31E69A829C89ADD838162F`。

## 9. 兼容性与工作区保护

- 复用 P8.11 transition、P8.12 reconciliation 与 P8.13 ledger，没有建立第二种 intent／tracker／ledger；
- 原有无 influence ProductHost workflow 保持可用，既有 focused `6/6` 全部通过；
- influence authority 建立后才启用更严格 fence，因此旧调用路径不会被无条件改变；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 7 个文件。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与一次必要的最终 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.15 在 Host 之外定义一个窄、可注入的 opaque influence executor adapter，并用 fake executor 驱动 `peek → attempt → acknowledgement` drain loop；仍不在同一阶段引入 GameplayEffect、GAS magnitude 或正式 content authority。
