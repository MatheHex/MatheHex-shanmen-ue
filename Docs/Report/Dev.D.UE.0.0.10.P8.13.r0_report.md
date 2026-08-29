# Dev.D.UE.0.0.10.P8.13.r0 Report

## 1. 结论

P8.13 已完成阵法 influence 的纯值 dispatch ledger。P8.11 的 Advance transition batch 与 P8.12 的 Prime／Rebase／Reset／Terminal reconciliation batch 现在可以进入同一 deployment-scoped ledger，由该 ledger 统一负责 pending 顺序、执行尝试证据、retryable failure、success acknowledgement、exact replay 与最终 seal。

本阶段结论为 **PASS**：focused `4/4`、`Shanmen.0_0_10` 完整回归 `262/262`、changed-file regression gate、静态边界扫描、Editor Development 与 Game Development 均成功。

## 2. 功能性

### 2.1 统一 source batch 入口

- 接受有效的 `Fdemo_mapShanmenFormationInfluenceTransitionBatch`；
- 接受有效的 `Fdemo_mapShanmenFormationInfluenceReconciliationBatch`；
- 首个 batch 冻结 `RunId + OwnerId + SourceEntityId + DeploymentId` ledger scope；
- 后续 batch 必须属于同一 scope；
- exact source replay 返回 `Replayed`，不重复追加 batch 或 intent；
- 同一 source identity 的不同内容、重复 intent identity 与跨 deployment 输入均 fail-closed。

### 2.2 pending 与 retry acknowledgement

- intents 按 source batch 接受顺序和 batch 内 canonical 顺序进入 pending；
- `TryPeekNextPending` 只暴露当前最早未成功 intent；
- executor 通过 opaque `AttemptId + ExecutorReceiptId` 回报结果；
- `RetryableFailure` 写入不可变 attempt receipt，但 intent 保持 pending；
- 后续独立 attempt 可成功确认并移出 pending；
- exact attempt replay 返回明确 replay 状态；
- 同一 `AttemptId` 改写 executor evidence 或 outcome 被拒绝；
- 已成功 intent 不接受新的 attempt。

### 2.3 seal

- 空 ledger 不可 seal；
- 有 pending intent 时不可 seal；
- valid no-op source batch 可建立 ledger 并立即 seal；
- seal identity 覆盖 ledger、accepted batch records 与全部 attempt receipts；
- sealed ledger 只允许 exact source／acknowledgement／seal replay，不接受新 batch；
- 相同 source 与相同 attempt 序列可重放出相同 ledger ID 与 seal ID。

## 3. 完整性与安全边界

本阶段只建立 delivery authority，不解释或执行 influence 内容。明确未实现：

- ProductHost 编排接入；
- GameplayEffect／GAS handle 或 effect application；
- magnitude、duration、stacking、damage、Buff；
- cadence、Tick、timer、异步任务；
- Actor／World discovery、spawn 或产品对象生命周期；
- persistence、SaveGame、ProfileRepository；
- 输入、UI、AI 与正式阵法 content。

production boundary scan 对 `UWorld`、`AActor`、`UObject`、Tick、timer、Actor discovery、SpawnActor、ApplyDamage、GameplayEffect、AbilitySystem、RNG、async 与 persistence API 均为 `0` matches。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedgerTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

未修改 P8.11/P8.12 planner、现有 ProductHost、World adapter、runtime module、库存或存档权威。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.13-FormationInfluenceDispatch-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceDispatch` | 4 | 0 | 0 | 1 | `C8F7E94F975C47E9B1994171AC6BC98F49B3A4BC7379210C73F38F2AA8AD1169` |
| `P8.13-Shanmen-full-final.log` | `Shanmen.0_0_10` | 262 | 0 | 0 | 1 | `7DBD75A8CD01D39E262290F08303BEE9C2E0DA7ADCB81BEFB5B9BB09656B76AA` |

两份最终日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 focused case：

1. `AcceptSourcesAndReplay`：两种 source batch 共用 scope、顺序与 replay 去重；
2. `RetryThenSuccess`：失败保留 pending、exact retry、attempt conflict、后续成功与 receipt 查询；
3. `NoOpSealAndClosure`：no-op seal、seal replay、seal 后 exact batch replay 与新 batch fence；
4. `DeterministicDrainAndScopeFence`：跨 deployment fence、pending seal fence、完整 drain 与 deterministic seal。

## 6. 首次失败与修正

源码与测试 case 没有失败。首次 changed-file gate 原生退出码为 `1`：测试命令在 `ExecCmds` 中附带 `;Quit`，虽然 focused `4/4` 与 full `262/262` 已完成，但进程在标准 `Automation Test Queue Empty` marker 写入前退出。Gate 正确报告两份 evidence `queue-empty marker missing`。

修正仅移除 `;Quit`，继续使用 `-TestExit="Automation Test Queue Empty"`。重跑后 focused 与 full 都写入唯一 queue-empty marker，原生退出码均为 `0`，最终 gate 通过。该失败属于验收命令证据不完整，不是源码失败或环境内存失败。

## 7. Changed-file regression gate

新增 `FormationInfluenceDispatch` path rule，并要求完整覆盖 dispatch、reconciliation、transition intent、coverage coordinator/tracker/transition、World coverage、Area、Host、World delivery、WorldGameplay 与 FormationDeployment 共十二组契约。

```text
REGRESSION_MAP_JSON: PASS Rules=48
SELF_TEST: PASS 57/57
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=12 Logs=2
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=12 Logs=2
```

第一条为 Source／Scripts gate；第二条为加入 Report／Log 后的最终 exact-staged gate。

- mapping SHA-256：`B3BCC00B0D168CDA2A789E901058417EA733DDE21BAE8C093023D0EB4694BB8D`；
- self-test SHA-256：`115BEC376EC8B0D0E620742F3600630B1AB1D1039A592EEE5869F177B114ED76`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | UBT evidence SHA-256 |
|---|---|---:|---|
| Editor initial source build | Succeeded / 5 actions / 9.68s | 0 | 首次编译输出已核验 |
| Editor final | Succeeded / up to date / 0.89s | 0 | `943C5D502EE0E84D7260AABA70C033DCF5877389B31364A7A3B1500571BA301B` |
| Game final | Succeeded / 4 actions / 21.61s | 0 | `695DBF72F80286FD75C123ACB8F2D12C5EC752D7F3800BE561CB635ACA36D14B` |

- Editor module：`11383808` bytes，SHA-256 `9DCC33E90457175B36D61E5CDA4C42AB96200E505DA504815D17FCE9D84A8EA9`；
- Game executable：`352530944` bytes，SHA-256 `2517CAA9F881E81947E12770C7038B906940D1C461456FBA56F695B5DA825E1A`。

## 9. 兼容性与工作区保护

- ledger 复用 P8.11/P8.12 的既有 immutable intent，不建立第二种 effect intent；
- executor receipt 保持 opaque，不把 GAS 或产品对象依赖反向带入 planner／ledger；
- 旧测试与完整 0.0.10 套件全部通过；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 7 个文件。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、`git diff --check` 与一次必要的最终 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.14 在现有 Formation ProductHost 中编排 P8.11/P8.12 planner 与本 ledger，并把 host response 扩展为可审计 dispatch evidence；仍以 fake/opaque executor 验证，不在同一阶段引入真实 GAS effect authority。
