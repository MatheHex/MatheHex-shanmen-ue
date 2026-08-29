# Dev.D.UE.0.0.10.P8.16.r0 Report

## 1. 结论

P8.16 已在 P8.15 的 injected interface 后实现纯值、内存内的 formation influence lease executor。它把 canonical influence intent 投影为 semantic lease，负责 Apply／Remove mutation、deterministic opaque receipt、exact attempt replay、lost-ack recovery、active-lease conflict 与 stale Remove 隔离；ProductHost ledger 仍唯一负责 pending、顺序、acknowledgement、retry 与 seal。

本阶段结论为 **PASS**：lease executor `4/4`、executor adapter `4/4`、influence Host `4/4`、`Shanmen.0_0_10` 完整回归 `274/274`、changed-file regression gate、自检 `61/61`、静态边界扫描、Editor Development 与 Game Development 均成功。

## 2. 功能性

### 2.1 semantic lease identity

- lease key 冻结 `Run / Owner / Source / Deployment / Area / Subject / Policy / Influence / Content`；
- `CauseId` 与 operation 不进入 key，因此不同 lifecycle evidence 可以对同一个 semantic lease 执行 Apply／Remove；
- `LeaseId` 由 canonical key 确定性派生，不接受调用方任意注入；
- active snapshot 只暴露 immutable key、lease identity 与 owning Apply intent。

### 2.2 Apply／Remove authority

- Apply 仅在相同 key 没有 active lease 时成功，并记录 active snapshot 与 completed-intent tombstone；
- 不同 Apply intent 不能夺取同一 active lease；
- Remove 必须命中相同 key 的 active lease，成功后移除 active snapshot并记录 tombstone；
- missing Remove、identity conflict 与内部一致性错误均 fail-closed，且不会伪造成功 receipt。

### 2.3 attempt replay 与 lost acknowledgement

- exact `AttemptId + invocation` 返回原结果，不新增 attempt 或重复 lease mutation；
- 相同 AttemptId 被不同 invocation 使用时直接拒绝，不污染既有 evidence；
- completed intent 使用新 AttemptId 重试时生成新 opaque receipt，但不重复 Apply／Remove mutation；
- 因此 executor 已成功而 Host acknowledgement 丢失时，adapter 可以用新 attempt 恢复 Host ledger；
- 已完成旧 Remove 在相同 key 被新 Apply 激活后重试，只恢复旧完成证据，不会删除较新的 lease。

### 2.4 自验证 evidence

- executor 每次 mutation 后校验 active lease、completed intent 与 attempt records 的确定性回指；
- success attempt 必须对应 completed intent，receipt 必须匹配 ledger／intent／attempt；
- rejected attempt 必须保持 invalid receipt；
- active Apply 必须能回溯到相同 key 的成功 completed intent；
- 自验证失败会整体回滚到调用前快照。

## 3. 完整性与安全边界

本阶段明确未实现：

- GameplayEffect、GAS handle、magnitude、duration、stacking 或正式 Buff 数值；
- Actor／World discovery、对象绑定、spawn 或 component mutation；
- Tick、timer、cadence、异步任务或后台 retry；
- persistence、SaveGame、ProfileRepository 或跨进程恢复；
- 第二套 Host、pending queue、dispatch ledger、planner 或 seal authority；
- 输入、UI、AI 与正式阵法 content。

新生产文件对 `UWorld`、`AActor`、AbilitySystem／GameplayEffect、timer、async 与 RNG API 均为 `0` matches。Executor 只拥有 semantic lease 与 executor-side replay evidence；Host ledger authority 未被复制。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceLeaseExecutor.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLeaseExecutor.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

ProductHost、dispatch ledger、reconciliation planner 与 P8.15 adapter 生产代码均未修改。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.16-FormationInfluenceLeaseExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor` | 4 | 0 | 0 | 1 | `0981F243F9E681E1E96624B683EA907A8C6192E75758E5F628EB57D64BB02B23` |
| `P8.16-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 0 | 1 | `0ADAD9661F50B1362391A622257F58212B155A34260DEA5B9D0A38B7F6ED50E2` |
| `P8.16-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 0 | 1 | `7C447DE861E58518E64AC992A7F05CE1A8812590735E5FEC4B8B42BD743856DF` |
| `P8.16-Shanmen-full-final.log` | `Shanmen.0_0_10` | 274 | 0 | 0 | 1 | `ABB0C7545DDF17677BF229B17001FA983E46C4B50B8270F5EF02962D5DAD06E2` |

四份最终日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 lease focused case：

1. `ApplyRemoveIntegration`：真实 Host + adapter + concrete executor 完成 Prime Apply、Terminal Remove、seal 与 teardown；
2. `LostAckRecovery`：exact executor replay稳定，新 attempt 恢复丢失的 Host acknowledgement，active lease 不重复；
3. `ConflictAndMissing`：missing Remove、exact rejected replay、AttemptId collision、competing Apply 均 fail-closed；
4. `StaleRemoveAfterReapply`：旧 Remove 的新-attempt recovery 不会删除后续 Apply 的 active lease。

## 6. 首次失败与修正

本阶段没有 Automation case、源码编译或最终构建失败。新 lease focused 首轮即 `4/4`。首轮通过后仅补强 evidence：增加 stored-attempt readback 与 same-AttemptId／different-invocation collision assertion；产品实现未因失败而修改，增强后 focused 仍为 `4/4`、full 为 `274/274`。

## 7. Changed-file regression gate

新增 `FormationInfluenceLeaseExecutor` path rule，并把 lease contract 加入 ProductHost、dispatch 与 executor adapter 的 owned-contract 覆盖。最终结果：

```text
REGRESSION_MAP_JSON: PASS Rules=50
SELF_TEST: PASS 61/61
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=18 Logs=4
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=18 Logs=4
```

第一条 coverage 结果为 Source／Scripts gate；第二条为加入 Report／Log 后的最终 exact-staged gate。

- mapping SHA-256：`974B72CDE0F974C3A7D23540797F692D9FF6614C9589205C5C6A7F2069068688`；
- self-test SHA-256：`8D2577A68EB29AED6C4D704567936D13E296E68958D8CDFB2D5277735EDD8EBE`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | UBT evidence SHA-256 |
|---|---|---:|---|
| Editor initial implementation | Succeeded / 5 actions / 30.48s | 0 | console evidence verified |
| Editor strengthened tests | Succeeded / 4 actions / 6.75s | 0 | console evidence verified |
| Editor final | Succeeded / up to date / 0.90s | 0 | `DCBD515091923ECC82D4570455981EA593BD5A54586E97723514513880D346E2` |
| Game final | Succeeded / 4 actions / 24.57s | 0 | `D214D1B74FB3314F85BDD067ABE06E5A4961BF786FC36586243F2EA0F7238B59` |

- Editor module：`11496960` bytes，SHA-256 `F2055794146A79036EBE8AF79F7BFEE2BFDFD8768E3DBF05BD803300CB82D174`；
- Game executable：`352634880` bytes，SHA-256 `CF8DDF347C9B6D15AC5A9922DDCE5264B6D65D814A07591667E466715C6D8252`。

## 9. 兼容性与工作区保护

- concrete executor 只实现 P8.15 interface，没有改写 adapter 或 Host public contract；
- P8.13 dispatch、P8.14 Host lifecycle 与 P8.15 adapter focused groups均保持 `4/4`；
- 原有无 concrete executor 的调用路径仍可注入其它实现；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 7 个文件。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与一次必要的最终 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.17 增加一个窄的 product composition owner，把现有 ProductHost 与 concrete lease executor 组合为 caller-driven 的单步 `TryExecuteNextInfluence`；AttemptId 继续由调用方提供，仍不引入自动 drain loop、timer、Actor、GAS 或正式 magnitude content。
