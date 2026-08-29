# Dev.D.UE.0.0.10.P8.15.r0 Report

## 1. 结论

P8.15 已完成阵法 influence 的窄、可注入 executor adapter。现有 ProductHost ledger 的 canonical pending intent 可以通过只读 invocation 交给外部 executor；executor 只返回 opaque receipt 与 `Succeeded`／`RetryableFailure` outcome，adapter 校验 ledger、intent、attempt 三重回指后才调用 Host acknowledgement。

本阶段结论为 **PASS**：executor focused `4/4`、dispatch `4/4`、influence Host `4/4`、`Shanmen.0_0_10` 完整回归 `270/270`、changed-file regression gate、静态边界扫描、Editor Development 与 Game Development 均成功。

## 2. 功能性

### 2.1 窄 executor contract

- execution command 只携带 caller-owned `IntentId + AttemptId`；
- adapter 从 Host ledger 读取完整 immutable intent 与 `LedgerId`，组成只读 invocation；
- executor receipt 必须精确回指 invocation 的 ledger、intent 与 attempt；
- `ExecutorReceiptId` 保持 opaque，adapter 不解释 effect handle、magnitude、duration、stacking 或产品对象；
- executor 显式拒绝或返回错配／无效 evidence 时，Host ledger 不产生 acknowledgement。

### 2.2 canonical pending 与顺序 fence

- 新 attempt 只能执行 ledger 的第一条 canonical pending intent；
- 请求后续 intent 会在 executor 调用前返回 `IntentOutOfOrder`；
- adapter 本身无字段、无 pending queue、无 retry counter、无 receipt store；
- Host ledger 继续作为唯一顺序、retry、acknowledgement 与 seal authority。

### 2.3 retry 与 exact replay

- `RetryableFailure` 通过 Host acknowledgement 记录，intent 保持 pending；
- 新 `AttemptId` 可再次执行同一 pending intent并成功；
- adapter 先查询 ledger 的 immutable attempt receipt；
- exact attempt replay 直接重建 executor evidence 并调用 Host exact acknowledgement replay，不再次调用 executor；
- exact success 在 intent 已离开 pending、甚至 ledger 已 seal 与 Host 已 teardown 后仍可安全 replay。

### 2.4 Terminal drain

- Prime Apply 与 Terminal Remove 均通过同一 adapter；
- Terminal Remove 成功后 ledger 才可 seal；
- seal 后 exact Remove replay 不触发第二次 executor side effect；
- P8.14 的 terminal teardown gate 保持有效。

## 3. 完整性与安全边界

本阶段明确未实现：

- GameplayEffect／GAS handle 或真实 effect application；
- magnitude、duration、stacking、damage、Buff 数值解释；
- Actor／World discovery、对象绑定或 spawn；
- Tick、timer、cadence、异步任务或后台重试；
- persistence、SaveGame、ProfileRepository；
- 新的 Host、queue、ledger、planner 或 retry authority；
- 输入、UI、AI 与正式阵法 content。

新 adapter 生产文件对 `UWorld`、`AActor`、`UObject`、Tick、timer、SpawnActor、ApplyDamage、GameplayEffect、AbilitySystem、RNG、async 与 persistence API 均为 `0` matches。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutorAdapter.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutorAdapter.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

ledger 只新增 immutable intent／attempt receipt 查询，不改变 Accept、pending、Acknowledge 或 Seal 语义。ProductHost 生产代码未修改。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.15-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 0 | 1 | `C2DCA9460CEF5E1A895FC92231BEDC66F1E733A8EA9ADAC256D70643D03F3B42` |
| `P8.15-FormationInfluenceDispatch-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceDispatch` | 4 | 0 | 0 | 1 | `83879F62313E0263A3113A166C78A990E3491FE671598F00044042525DE26C7F` |
| `P8.15-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 0 | 1 | `DA9239BF63A7FBAA6AD04E5AE8F5A75E17B63F611EA41B0384B22597B8BFE82A` |
| `P8.15-Shanmen-full-final.log` | `Shanmen.0_0_10` | 270 | 0 | 0 | 1 | `310171421F11B8EB8FC99FE178E31AF226A34307E7C74E04B90EF076DA5EC450` |

四份最终日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 executor focused case：

1. `SuccessReplay`：成功 acknowledgement、immutable receipt query、exact success replay 不重复调用 executor；
2. `RetryThenSuccess`：retry 保持 pending、exact retry 无 executor 调用、独立新 attempt 成功；
3. `OrderAndEvidenceFence`：out-of-order 在调用前拒绝、executor rejection 与错配 evidence 均不推进 Host、相同未记录 command 可恢复；
4. `TerminalDrainSeal`：Apply／Terminal Remove 走同一 adapter，seal 后 exact Remove replay 无 side effect，teardown 成功。

## 6. 首次失败与修正

本阶段没有 Automation case、源码编译或最终构建失败。新 executor focused 首轮即 `4/4`。首次 focused 后仅加强测试证据：在 executor rejection 和 corrupt receipt 后立即冻结 pending 数，并把 Terminal exact replay 移到 seal／teardown 之后，以直接证明错误 evidence 不推进 Host、sealed replay 不重入 executor；产品代码未因此修改。

## 7. Changed-file regression gate

新增 `FormationInfluenceExecutor` path rule，并把 executor contract加入 ProductHost 与 dispatch ledger 的 owned-contract 覆盖。最终结果：

```text
REGRESSION_MAP_JSON: PASS Rules=49
SELF_TEST: PASS 59/59
REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=17 Logs=4
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=17 Logs=4
```

第一条 coverage 结果为 Source／Scripts gate；第二条为加入 Report／Log 后的最终 exact-staged gate。

- mapping SHA-256：`4935E8CEE167201E86B1EC9BDDCDE91D02D0C9FE25750DAD5E89942406C3C0C4`；
- self-test SHA-256：`7E994F2DB140CEE469C96170F3AE59E94AB434F78DADA06B5B656BB009A320E5`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | UBT evidence SHA-256 |
|---|---|---:|---|
| Editor initial implementation | Succeeded / 8 actions / 41.50s | 0 | console evidence verified |
| Editor strengthened tests | Succeeded / 4 actions / 6.39s | 0 | console evidence verified |
| Editor final | Succeeded / up to date / 0.91s | 0 | `5B22E56100E5FCEE487E55FD1C44DC1F06029C709D3F0977EE48BA59368D3F38` |
| Game final | Succeeded / 7 actions / 30.12s | 0 | `5E48F09A90AEBBEDC5215894C369A52BC3DA780040CE4AEB3CEEEBB3063C25C7` |

- Editor module：`11459072` bytes，SHA-256 `4292E0631D8FD0C61B2870A84406717BF2557D9C9971C496105DDC4941CB2ADA`；
- Game executable：`352599552` bytes，SHA-256 `8FC98BF4E14141F5F8E3193DA8920CFD1B424504707F1DFE4EA2620BF4A5D001`。

## 9. 兼容性与工作区保护

- adapter 只使用 ProductHost 的 public read/acknowledge contract，没有 friend、mutable ledger 或 second queue；
- P8.13 ledger 的 existing semantics保持不变，dispatch focused `4/4`；
- P8.14 Host lifecycle／terminal gate保持不变，influence Host focused `4/4`；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 9 个文件。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与一次必要的最终 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.16 在该接口后实现一个纯值、内存内的 influence lease executor：以 deployment／area／subject／policy 为 lease key，冻结 Apply／Remove 的幂等与冲突规则并产出 opaque receipts；仍不接 Actor、GAS 或正式数值 content。
