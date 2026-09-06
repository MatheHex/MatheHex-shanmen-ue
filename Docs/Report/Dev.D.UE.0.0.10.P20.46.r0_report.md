# Dev.D.UE.0.0.10.P20.46.r0 Report

## 1. 结论

P20.46 已建立 Arc preview renderer surface ownership transition transaction。它在同一个受保护调用中组合 P20.44 recreation policy 与 P20.45 lifecycle executor，并把 `BindingReady` 收敛为绑定到具体 surface instance 的 deterministic transition ticket。

本阶段没有直接替换 P20.43 Composition Owner 中的非拥有 surface pointer。审计发现：现有 Owner 的 `IsValid` 会读取旧 surface，直接旋转一个可能已经销毁的裸指针不安全；如果旧 surface 仍存活且可见，直接采用新 surface 又可能产生双重显示。因此 P20.46 先冻结“谁、以什么状态、基于哪份 policy/receipt 可以进入后续 Owner handoff”，实际 pointer commit 与旧 surface retirement 留给下一阶段的一次性消费事务。

每个 renderer surface 必须暴露生命周期内不可变的 `SurfaceInstanceId`。transaction 在 policy 前和 lifecycle 后各读取一次 identity；consumer/cursor、permit 与 receipt 即使完全相同，只要 concrete surface identity 不同或执行中漂移，就不会签发 ticket。

本阶段仍是 P 阶段无头契约：只使用 fake surface 验证 ownership authorization，不连接 MainHUD、widget、component、真实 renderer 或 World。

## 2. 基线、分支与改动范围

- 基线：`713769270165cdbd97cff595708de768443b2947`（P20.45）；
- 分支：`agent/0.0.10-p20-46-thrown-weapon-arc-preview-surface-ownership-transition`；
- 新增 `demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.h/.cpp`；
- 扩展 `demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`；
- 扩展 changed-file regression map 与其自测；
- 新增本 Report 与同名 Log；
- 未修改 P20.45 Executor、P20.44 Policy、P20.43 Owner、P20.42 Adapter 或 P20.41 Host 的 public contract；
- 未执行 Owner/Adapter pointer bind、surface replacement、rehydrate、真实 renderer mutation 或产品状态修改。

## 3. Identity-bearing ownership candidate

新增 `Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipCandidate`，继承 P20.45 narrow lifecycle capability，并只增加 `GetSurfaceInstanceId`。

`SurfaceInstanceId` 表示一个具体 renderer surface 实例，必须在该实例生命周期内保持不变。renderer 重建必须产生新 identity，即使新旧 surface 的 consumer definition 与 physical cursor 相同。这样 ownership handoff 不会把“状态相同”误判为“对象相同”。

transaction 不保存 candidate pointer，不获得 surface 所有权，也不把 identity 当成可变序号。identity、consumer 与 cursor 都由同一个显式传入的 candidate capability 提供。

## 4. Immutable transition request

request 固定绑定：

- deterministic `RequestId`；
- Run identity；
- expected consumer definition；
- Host-authoritative cursor；
- exact surface instance identity；
- `BindFresh`、`AdoptExact` 或 `ClearToEmpty` 中的一个动作。

`Inspect`、`Invalid`、无效 Run/consumer/surface identity，以及属于其它 Run 的 authoritative cursor 都不能创建 request。request identity 由全部字段确定性派生，重复输入得到同一 `RequestId`。

request 不是 policy permit，也不自行授权 mutation。transaction 必须读取 candidate 当前 consumer/cursor，再让 P20.44 对实际 snapshot 分类和签发 exact permit。

## 5. Deterministic ownership transition ticket

只有 P20.45 返回 exact `BindingReady` receipt 时才能签发 ticket。ticket 绑定：

- RequestId；
- P20.44 PolicyDecisionId 与 PermitId；
- P20.45 LifecycleReceiptId；
- Run、consumer 与 surface instance identity；
- BindFresh/AdoptExact action；
- expected 与 observed physical cursor。

`TicketId` 由上述全部证据派生。ticket 可通过 `MatchesCandidateSnapshot` 重新核对 exact identity、consumer 与 cursor；替换 surface instance、consumer 或 cursor 都会失配。

ticket 只是后续 Owner handoff 的不可变授权，不会自行修改 pointer。后续提交层仍必须在自己的 operation guard 下重新读取 candidate snapshot，并对 ticket 进行一次性消费或幂等记账。

## 6. Bounded transaction 与 fail-closed 边界

执行顺序固定为：

1. 在任何 candidate callback 前检查并安装 transaction operation guard；
2. 验证 immutable request；
3. 第一次读取 concrete surface identity，必须匹配 request；
4. 从同一个 candidate 读取 consumer 与 physical cursor；
5. 调用 P20.44 Policy 评估 exact snapshot；
6. 只有 Policy Authorized 才把 permit 交给 P20.45 Executor；
7. lifecycle 返回后第二次读取 surface identity；
8. identity 稳定且为 BindingReady 时签发 ticket；cleanup 则只返回清理结果，不签发 ticket。

query callback 或 cleanup callback 试图递归调用同一 transaction 时，内部调用在任何 candidate query 前返回 `OperationInProgress`。invalid request 与 initial identity mismatch 也在 policy/surface mutation 前拒绝。

policy 与 lifecycle 之间发生 cursor drift 时，P20.45 snapshot check 返回 `LifecycleRejected`，没有 ticket。lifecycle 之后发生 identity drift 时返回 `SurfaceIdentityDrift`，即使 consumer/cursor 看似兼容也不签发 ticket。

## 7. Cleanup、retry 与 re-evaluation

`ClearToEmpty` 仍完全委托 P20.45 执行，P20.46 不新增第二条 renderer mutation 路径：

- `CleanupApplied`：最多一次 surface mutation，结果不含 ticket，明确要求调用方以 Empty surface 重新创建 request 并重新评估；旧 cleanup request 再执行会在 policy 层被拒绝；
- `CleanupRejected`：surface identity 与 cursor 保持不变，结果不含 ticket，但保留 `CanRetryExactRequest`，调用方可在另一次显式 transaction 中重试；
- lifecycle invalid、snapshot drift 或 evidence divergence：fail closed，不补偿、不循环重试、不签发 ticket；
- `EmptyNeedsRehydrate`：仍由 P20.44 拒绝，P20.46 不提供 rehydrate 入口。

## 8. 自动化覆盖

新增 6 项 focused automation：

1. `EvidenceContract`：request deterministic identity、action fence 与 invalid-request zero-callback；
2. `BindFresh`：fresh Empty candidate、零 mutation ticket、exact identity match 与 deterministic replay；
3. `AdoptExact`：exact visible adoption、零 mutation ticket、cursor/consumer substitution fence；
4. `CleanupOutcomes`：Applied 后 re-evaluation/stale request fence，以及 Rejected 后外部显式 retry；
5. `IdentityAndPolicyFences`：initial identity mismatch、rehydrate fence 与 policy/lifecycle 间 cursor drift；
6. `IdentityDriftAndReentrant`：post-lifecycle identity drift suppresses ticket，以及首次 identity callback 的 transaction reentry fence。

完整 `Shanmen.0_0_10` 从 P20.45 的 1101 增至 1107 项。

## 9. Changed-file 回归与验证证据

新增 `ThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition` exact-path mapping rule。5 个产品/测试/流程改动路径命中 2 条规则，求并集要求 31 个测试组；3 份最终日志全部满足映射。

- mapping self-test：385/385；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=31 / Logs=3；
- boundary scan：PASS，OwnershipTransition 2 files / 938 lines；World/Actor/UObject/RNG/ApplyDamage 与 direct Show/Replace/Hide/ClearToEmpty dispatch 均为 0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| P20.46_Focused.log | Product.ThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition | 6/0 | `A4B90D3852DB419415343EB7F93CE38D341D69EAA4A2DA6F63F142AA6F167E3E` |
| P20.46_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | `62A0EAE6AA4C027C5D659E316A7295340A4B2D6F1EA53B847A73CBDD9FD480EB` |
| P20.46_Full.log | Shanmen.0_0_10 | 1107/0 | `B187F109FED56EE9F0C61CC492F4636E047E15BD14769233FEE47D267622CBB2` |

正式日志累计 1159/0，包含 focused、legacy 与完整套件重叠；独立完整套件为 1107/0。完整套件出现 91 条既有 SwordRhythm journal/checkpoint/envelope/manifest codec 非惩罚性 unresponsive 通知；进程持续推进并由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束，没有用拆组、外层 timeout 或人工中止替代最终结果。

构建：

- initial Editor：5 actions / native 0 / 15.46 秒；
- final Editor：0 actions / native 0 / 1.73 秒；
- final Game：4 actions / native 0 / 35.09 秒；
- Editor DLL：17328128 bytes，SHA-256 `B52EA73E4BC85658F0887535AACB34B51EF38A552DE0F0791D2C6623A3E996B6`；
- Game EXE：358420992 bytes，SHA-256 `FB86576F4CFA630ABD5D5B3E32F09C6FFB0949780174051A9A1BDBF19EAE7B0E`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：identity-bearing surface candidate、immutable exact request、policy/lifecycle same-candidate composition、deterministic ownership ticket、initial/final surface identity fence、consumer/cursor substitution fence、zero-mutation fresh/adopt authorization、single-call cleanup delegation、applied re-evaluation、rejected explicit retry、rehydrate fence、callback reentry fence 与 changed-file regression。

未验证：P20.43 Owner 对 ticket 的实际消费、Owner/Adapter pointer replacement、旧 surface 的安全 retirement、dangling pointer 消除、同一 ticket 的 one-shot ledger、真实 renderer/MainHUD/widget/component/visible frame、rehydrate、跨线程同步、renderer 崩溃恢复、补偿事务、真实输入、World、Editor UI/PIE/Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.47 建立 Owner surface handoff commit：在旧 surface 仍可安全读取或有显式 retirement attestation 的前提下消费 P20.46 ticket，原子更新 Owner/Adapter 绑定，并为 ticket replay、旧 pointer 失效和双 surface 可见性建立 fail-closed 证据。若无法证明旧 surface 存活，不应让现有 Owner 继续解引用它，而应先引入 identity/cursor snapshot 与显式 lease。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-46-thrown-weapon-arc-preview-surface-ownership-transition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-46-thrown-weapon-arc-preview-surface-ownership-transition/Docs/Report/Dev.D.UE.0.0.10.P20.46.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-46-thrown-weapon-arc-preview-surface-ownership-transition/Docs/Log/Dev.D.UE.0.0.10.P20.46.r0_log.md>
