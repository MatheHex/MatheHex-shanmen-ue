# Dev.D.UE.0.0.10.P20.45.r0 Report

## 1. 结论

P20.45 已建立 Arc preview renderer surface lifecycle executor。它消费 P20.44 签发的 exact permit，把 `BindFresh`、`AdoptExact` 与 `ClearToEmpty` 转换成有界、可验证的执行结果，但仍不接管 Composition Owner pointer，也不执行 rehydrate。

`BindFresh` 与 `AdoptExact` 是零清理调用的 readiness 操作：executor 重新核对 consumer 与双 cursor snapshot 后，生成 deterministic `BindingReady` receipt。`ClearToEmpty` 是唯一允许调用 lifecycle surface 的动作，每次 `Execute` 最多调用一次 `ClearToEmpty`，随后同时核对 renderer attestation 与实际 before/after cursor。

清理被应用后，旧 permit 因 surface cursor 已改变而失效，调用方必须重新经过 P20.44 policy；清理被拒绝且 surface 保持不变时，返回 durable rejection receipt，调用方可以在另一次明确调用中重试同一 permit。executor 内部没有 retry loop。

本阶段仍是 P 阶段无头契约：只使用 fake lifecycle surface 验证协议，不连接 MainHUD、widget、component、真实 renderer 或 World。

## 2. 基线、分支与改动范围

- 基线：`854a18336c499dc7118215045532d6bd85cdc379`（P20.44）；
- 分支：`agent/0.0.10-p20-45-thrown-weapon-arc-preview-surface-lifecycle-executor`；
- 新增 `demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.h/.cpp`；
- 扩展 `demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`；
- 扩展 changed-file regression map 与其自测；
- 新增本 Report 与同名 Log；
- 未修改 P20.44 Policy、P20.43 Owner、P20.42 Adapter 或 P20.41 Host 的 public contract；
- 未执行 rehydrate、Owner/Adapter pointer bind/replacement、产品状态修改或真实 renderer 操作。

## 3. Narrow lifecycle capability

新增 `Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycle`，只暴露三个能力：

1. 读取 consumer definition identity；
2. 读取当前 physical surface cursor；
3. 对一个 exact cleanup permit 执行一次 `ClearToEmpty`。

接口不拥有 delivery command、Host cursor、ledger、retry loop、Composition Owner pointer 或 rehydrate policy。executor 自身也不保存 surface 指针；每次操作都由调用方显式提供一个 capability。

## 4. Renderer response attestation

cleanup surface 必须返回 immutable response，结果只有 `Applied` 或 `Rejected`。response 保存：

- deterministic `ResponseId`；
- exact `PermitId`；
- typed outcome 与非空 outcome code；
- previous physical cursor；
- reported current physical cursor。

合法 `Applied` 必须从 permit 绑定的 visible cursor 转为 Empty；合法 `Rejected` 必须保持同一 visible cursor。response identity 绑定 permit、outcome、code 与双 cursor；foreign permit、非法 cursor 或不符合 transition semantics 的 attestation 无法通过 deterministic validation。

response 不是单方面真值。executor 会在 surface callback 返回后重新读取 physical cursor，并要求 attestation 的 before/after 与实际观察完全一致。

## 5. Deterministic execution receipt

receipt outcome 固定为三种：

| Receipt | Surface cleanup calls | 语义 |
|---|---:|---|
| BindingReady | 0 | exact fresh/adopt permit 已完成执行前核验，可交给后续绑定事务 |
| CleanupApplied | 1 | renderer 明确应用清理，且实际 cursor 已 Empty |
| CleanupRejected | 1 | renderer 明确拒绝清理，且实际 cursor 保持原 visible state |

`ReceiptId` 由 permit、outcome、surface call count、可选 response 与双 cursor 派生。receipt 会重新验证 permit snapshot、response identity、call budget 与 outcome transition，因此不能把零调用伪装成 cleanup，也不能把 rejection 冒充 applied cleanup。

## 6. Executor fail-closed 与重入边界

执行顺序固定为：

1. 在任何 surface callback 前检查 operation guard；
2. 用 guard 包围 consumer/cursor preflight、cleanup callback 与 postflight cursor read；
3. 验证 permit；
4. 验证 consumer identity；
5. 验证 P20.44 expected/observed snapshot；
6. binding permit 生成零 mutation receipt；
7. cleanup permit 最多调用一次 surface；
8. 核对 response 与实际 cursor，再生成 receipt。

最终自查把 guard 从首次 cursor 读取之后前移到任何 surface callback 之前。这样即使 surface 的只读 query 或 cleanup callback 尝试递归调用同一 executor，内部请求也会在读取 surface 之前返回 `OperationInProgress`，不会形成 query-recursion 缺口。

invalid permit、consumer mismatch 与 snapshot drift 都在 cleanup 前拒绝。invalid/foreign response 返回 `SurfaceResponseInvalid`；Applied-without-physical-clear 或 mutated-then-Rejected 返回 `SurfaceInvariantViolation`。这两类 surface 违约均不生成 receipt。

## 7. Retry、reevaluation 与 ownership fence

executor 没有隐式 retry：一次 `Execute` 的 mutation budget 恒为 0 或 1。

- `CleanupRejected`：surface 未改变，result 明确标记 `CanRetryExactPermit`；是否再次尝试由外部调用方决定；
- `CleanupApplied`：surface 已 Empty，result 明确标记 `NeedsReevaluation`；旧 cleanup permit 的 snapshot 不再匹配；
- invalid response / invariant violation：没有可信 receipt，不宣称 cleanup 成功，也不自动补偿；
- binding readiness：只证明可以进入下一阶段，不修改 Owner 或 Adapter pointer。

rehydrate 仍受 P20.44 fence 保护。`EmptyNeedsRehydrate` 不会获得本 executor 可消费的 binding/cleanup permit，本阶段也没有新增绕过入口。

## 8. 自动化覆盖

新增 6 项 focused automation：

1. `EvidenceContract`：response/receipt deterministic identity、applied/rejected transition 与 permit 类型边界；
2. `BindingReady`：fresh/adopt 零 cleanup 调用、deterministic replay 与 cursor 不变；
3. `CleanupApplied`：一次清理、applied receipt、reevaluation 要求与旧 permit stale fence；
4. `CleanupRejectedRetry`：一次拒绝、unchanged cursor、durable receipt 与外部显式 retry；
5. `InvariantFailures`：invalid response、Applied-without-mutation、mutated-then-Rejected 全部 fail closed；
6. `PreflightAndReentrant`：invalid permit、consumer mismatch、snapshot drift 与 callback reentry 前置防线。

完整 `Shanmen.0_0_10` 从 P20.44 的 1095 增至 1101 项。

## 9. Changed-file 回归与验证证据

新增 `ThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor` exact-path mapping rule。5 个改动路径命中 2 条规则，求并集要求 30 个测试组；3 份最终日志全部满足映射。

- mapping self-test：383/383；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=30 / Logs=3；
- boundary scan：PASS，LifecycleExecutor 2 files / 958 lines；World/Actor/UObject/RNG/ApplyDamage 与 direct Show/Replace/Hide 0；生产 `.ClearToEmpty` dispatch 1；
- `git diff --check`：PASS，0 whitespace errors。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| surface_lifecycle_executor_final.log | Product.ThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor | 6/0 | `5EC7C1294C739C4C605ABEB55F9B6B18C50E054AFB4C3F496D0D1E4990D50B49` |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | `0ABF04B3B511A51BD9A6A204CA02D4517704F038512F339695C5834B6EE59379` |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1101/0 | `195F8320286F55DCB0A7C569025A68454A2326D7D12684C4B5D201FB7F7543CB` |

正式日志累计 1153/0，包含 focused、legacy 与完整套件重叠；独立完整套件为 1101/0。完整套件出现 90 条既有 SwordRhythm journal/checkpoint/manifest codec 非惩罚性 unresponsive 通知；进程持续推进并由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束，没有用拆组、外层 timeout 或人工中止替代最终结果。

构建：

- initial candidate Editor：5 actions / native 0 / 23.67 秒；
- guard-corrected Editor：5 actions / native 0 / 13.01 秒；
- final Editor：up to date / 0 actions / native 0 / 1.73 秒；
- final Game：4 actions / native 0 / 23.29 秒；
- Editor DLL：17273344 bytes，SHA-256 `7B3F9A90ED6895BF1104C746EB4B7439CEC3EDF3B372B66FF88B4F4F2EE03903`；
- Game EXE：358380032 bytes，SHA-256 `E300C3C0FB02F956A6D36F4AE386E67A629BA713BD886E73BFB08A862939FC3B`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：P20.44 exact permit consumption、narrow lifecycle capability、consumer/snapshot preflight、deterministic response/receipt、fresh/adopt zero-cleanup readiness、single-call cleanup、applied/rejected cursor invariants、invalid attestation fence、caller-owned retry、post-cleanup reevaluation、query/cleanup callback reentry fence 与 changed-file regression。

未验证：真实 renderer attestation、真实 MainHUD/widget/component/visible frame、Composition Owner/Adapter pointer bind 或 replacement、旧 surface retirement、rehydrate、跨线程同步、renderer 崩溃恢复、补偿事务、真实输入、World、Editor UI/PIE/Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.46 建立 surface ownership transition transaction：组合 P20.44 policy 与 P20.45 receipt，在 exact `BindingReady` 后有界地提交 Owner bind/adopt，在 `CleanupApplied` 后重新评估再提交 replacement；仍以 capability/receipt 驱动，不把 renderer callback 或 pointer mutation塞回纯 Policy。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-45-thrown-weapon-arc-preview-surface-lifecycle-executor>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-45-thrown-weapon-arc-preview-surface-lifecycle-executor/Docs/Report/Dev.D.UE.0.0.10.P20.45.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-45-thrown-weapon-arc-preview-surface-lifecycle-executor/Docs/Log/Dev.D.UE.0.0.10.P20.45.r0_log.md>
