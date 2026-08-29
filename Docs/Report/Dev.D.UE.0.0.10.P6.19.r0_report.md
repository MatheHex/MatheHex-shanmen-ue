# Dev.D.UE.0.0.10.P6.19.r0 Report

## 1. 结论

P6.19 已完成 Orbit threat 的 latest-completed sample freshness fence，结论为 **PASS**。

P6.18 已把 authority consumption 绑定到 exact item/activation，但 authority 的水位线只知道“最近已消费”的 sample。调用方仍可保留 sample N，在 sample N+1 已开始或已结束但尚未消费时重放 N；也可在飞剑已 Launch、Interrupt 或完成后重放当前 checkpoint。P6.19 将几何生成时序与消费时序闭合：只有当前 activation 最新完成、没有更新窗口在途、且物品仍为 Active Orbiting 的 sample 才能继续生成 policy、presence 或进入 Host consumption。

## 2. 功能性

- `FShanmenControlledWeaponExecution` 新增 latest-completed sample 判定；
- receipt 必须属于当前 action、当前 detector、`ControlledObject` 类型，并满足 `HitOrdinal + 1 == NextEmissionOrdinal`；
- 新 sample 已打开时，上一 sample 的 policy evaluation 与 presence consumption 立即失败关闭；
- 新 sample 已关闭但尚未消费时，上一 sample 同样立即过期，不再依赖 authority 先看到新 checkpoint；
- 只有最新完成 sample 可生成 policy 与 presence intents；
- Run Host consumption 额外要求 Controller 仍为 Active Orbiting 且没有活动 contact window；
- 飞剑进入 Directed、Recalled、Completed 或 Interrupted 后，旧 Orbit presence 不能重放；
- 没有更新 sample 且物品仍在 Orbiting 时，精确最新 sample replay 继续保持幂等；
- freshness 逐 Controller/activation 独立，一件飞剑的新 sample 不会过期另一件飞剑的最新 sample。

## 3. 双层水位线

P6.19 明确区分两类单调约束：

- **生成水位线**：`FShanmenDetectorEmissionSession::NextEmissionOrdinal` 决定哪个已完成 sample 是当前最新；
- **消费水位线**：P6.17/P6.18 authority checkpoint 决定哪个 sample 已经原子消费并可 exact replay。

policy、presence 构造和 Host consumption 必须先通过生成水位线；authority 随后继续负责 idempotency、sample conflict、retained receipts 与累计 revision。这样 sample N+1 一旦开始便可使 N 失效，而不需要伪造 N+1 的空消费或提前修改 authority。

## 4. 失败关闭与兼容性

`IsLatestCompletedOrbitThreatEmission()` 同时验证 Execution 完整性、Orbiting 状态、无活动 emission、receipt 合法性、detector identity、完整 action identity 与 exact ordinal。`TryEvaluateOrbitThreatReceipt()` 还要求 action runtime 仍可发射候选；`TryBuildOrbitThreatPresenceIntents()` 复用同一 freshness 判定。

Run Host 不复制 ordinal 规则，而是调用 Execution 的 canonical 判定，再验证 exact Controller action/item/run identity。任何 freshness 或状态拒绝都不会修改 Controller、authority、checkpoint、retained receipts、累计 revision 或目标 vitality。

既有 public 方法签名保持不变，只新增一个只读 freshness 查询。语义收紧仅影响此前不安全的 stale/in-flight/post-state receipts；当前 sample 的首次消费与精确 replay 保持兼容。

## 5. 测试覆盖

Runtime 测试新增：

- newer Orbit sample 打开期间，旧 emission 不能重新 evaluate；
- newer sample 完成后，旧 emission 不能重新 evaluate；
- newer sample 完成后，旧 policy 不能再生成 presence；
- latest completed policy 仍可正常生成 presence；
- 既有 exact replay、空 sample、状态切换与零 impact 行为保持通过。

产品 `PerItemThreatSampleWatermarks` 与 `OrbitThreatRouting` 新增：

- 更新窗口在途时，旧 presence 不能直接进入 Host consume；
- 更新窗口完成但尚未 finalization 时，旧 presence 已立即过期；
- 新 sample finalization 后 checkpoint 正常推进；
- 另一 item 的独立最新 sample 仍可 replay；
- item Launch 为 Directed 后，即使持有最新已消费的 Orbit presence，也不能 replay。

最终无头自动化：

| Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 8 | 0 | 0 | `0.117s` | `EE2BDEB6AF4A3D383E9E1F476A1A6BFDE981D99327E7EB1C08C6EAC3A83BB4DB` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 31 | 0 | 0 | `0.517s` | `A71468432A2E3125C73FC3903B6F43F6BBC87A74F7592DFA64FE12F0D43BDCD4` |
| `Shanmen.0_0_10` | 164 | 0 | 0 | `8.256s` | `0835EC1D79DB4DD8E263C49AA1688BE1265E6D55CA99685613DEA1DDE715AA0B` |

每份日志均只有一个实际 `Automation RunTests` 命令、一个 queue-empty 终止、Fail `0`、Fatal/unhandled/assert/ensure `0`，原生退出码均为 `0`。

## 6. 改动—回归门禁

CombatRuntime 与 ControlledWeaponRunHost 两类改动路径共同要求 11 个测试组；完整 0.0.10 日志覆盖全部必跑组：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=11 Logs=3
SELF_TEST: PASS 14/14
```

## 7. 构建与静态检查

- `git diff --check`：native exit `0`；
- 首次 Editor Development build：`43/43` actions，`Result: Succeeded`，native exit `0`，`124.15s`；
- 首次 Game Development build：`40/40` actions，`Result: Succeeded`，native exit `0`，`116.14s`；
- 最终状态 Editor 增量 build：`4/4` actions，`Result: Succeeded`，native exit `0`，`5.42s`；
- 最终状态 Game 增量 build：`3/3` actions，`Result: Succeeded`，native exit `0`，`11.32s`；
- Runtime Editor DLL UTC：`2026-08-29T06:18:30.0176487Z`；
- 最终 demo_map Editor DLL UTC：`2026-08-29T06:24:16.5766844Z`；
- 最终 Game executable UTC：`2026-08-29T06:25:38.5233279Z`；
- 新增生产可执行代码扫描 cadence/timer、damage/control/effect commit、spawn、Actor/World、RNG、Cooldown 与 Duration：命中 `0`。

四次构建均为首次执行成功，没有源码、环境、内存、SDK、外层超时或链接失败。Game executable 仅构建，未启动。

## 8. 修改范围

- `Source/ShanmenCombatRuntime/Public/ShanmenControlledWeaponExecution.h`
- `Source/ShanmenCombatRuntime/Private/ShanmenControlledWeaponExecution.cpp`
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenControlledWeaponExecutionTests.cpp`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.cpp`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`
- 本 Report 与同名 Development Log。

Source 新增 `75` 行、删除 `42` 行；删除主要来自以 canonical freshness helper 替换重复 action/detector 字段比较。未修改 cadence owner、威慑效果、damage/control、Actor/World 状态、item authority、Profile schema、存档、GameplayTags、Build.cs、输入或资产；未纳入长期未跟踪历史文件。

## 9. P/F 边界

本 Report 只包含 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-19-threat-sample-freshness/Docs/Report/Dev.D.UE.0.0.10.P6.19.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-19-threat-sample-freshness/Docs/Log/Dev.D.UE.0.0.10.P6.19.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-19-threat-sample-freshness>
