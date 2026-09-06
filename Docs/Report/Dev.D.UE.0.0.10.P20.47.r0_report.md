# Dev.D.UE.0.0.10.P20.47.r0 Report

## 1. 结论

P20.47 已建立 Arc preview Composition Owner surface handoff commit。P20.46 的 ownership ticket 现在可以在一个有界 transaction 中被重新核验并消费：`BindFresh` 在两个 Empty physical surface 之间零 mutation 切换；`AdoptExact` 先要求仍存活的旧 surface 返回显式 retirement response，再把 Owner 与 Adapter 的非拥有 pointer 一起提交到新 surface。

本阶段没有把裸指针改造成拥有型对象，也没有假设旧 pointer 仍可解引用。调用方必须显式提供仍存活、且地址与 Owner 当前绑定完全相同的旧 surface reference；transaction 在地址匹配前不调用旧、新 surface。地址匹配、operation guard 和 snapshot 复核后，才允许读取 identity/consumer/cursor 或调用一次 `RetireForHandoff`。

提交成功后，Owner 保存 concrete `SurfaceInstanceId` 与 immutable handoff receipt。最近一次同 ticket 重放只有在当前 pointer、instance identity、consumer 与 cursor 仍匹配时才返回 `Replayed`，不会再次 retirement。冲突重放、surface substitution、cursor drift、consumer drift 与回调重入全部 fail closed。

本阶段仍是 P 阶段无头契约：只使用 fake surface 验证 pointer handoff 与 evidence，不连接 MainHUD、widget、component、真实 renderer 或 World。

## 2. 基线、分支与改动范围

- 基线：`00fc0fac8a43b068408ae57cb92592c7fc2e8569`（P20.46）；
- 分支：`agent/0.0.10-p20-47-thrown-weapon-arc-preview-owner-surface-handoff`；
- 新增 `demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.h/.cpp`；
- 扩展 P20.43 Composition Owner，使 Owner/Adapter 可在 friend transaction 内联合提交，并保存新 surface identity 与最近 handoff receipt；
- 扩展 P20.42 Consumer Adapter，仅授权 handoff transaction 更新其非拥有 surface pointer；
- 扩展 `demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`；
- 扩展 changed-file regression map 与自测；
- 新增本 Report 与同名 Log；
- 未修改 Host 的 authority state、pending command、presentation state 或 delivery cursor；
- 未调用 Show、Replace、Hide、ClearToEmpty、rehydrate 或产品逻辑作为 handoff 捷径。

## 3. Handoff surface capability 与旧对象存活边界

新增 `Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface`，组合三类现有能力：

- P20.42 renderer-facing Show/Replace/Hide surface；
- P20.46 identity-bearing ownership candidate；
- P20.47 `RetireForHandoff`。

handoff 不从 Owner 的旧裸指针自行恢复对象。调用方必须同时传入 `ExpectedOldSurface` 与 `NewSurface` 的 live reference。transaction 先比较转换后的 base pointer 地址；不相同则返回 `OldSurfaceMismatch`，且两边 surface query 与 retirement 均为 0。

这消除了 handoff transaction 主动解引用未知旧 pointer 的路径，但不改变 active Owner 原有“外部 surface 必须比 Owner 活得久”的全局生命周期契约。真正拥有型 renderer handle 或跨线程 lifetime lease 仍属于后续范围。

## 4. Immutable retirement response

`AdoptExact` 的旧 surface 必须返回 self-validating response，固定包含：

- deterministic ResponseId；
- exact transition TicketId；
- retired 与 replacement SurfaceInstanceId；
- Applied 或 Rejected outcome 与非空 outcome code；
- retirement 前后 physical cursor。

Applied 只允许 exact visible-to-empty；Rejected 只允许 cursor 完全不变。response identity 由全部字段确定性派生，并重新绑定 P20.46 ticket 的 replacement identity 与 observed cursor。

transaction 不只相信 response：callback 返回后再次读取旧、新 surface。response invalid、旧 identity 漂移、旧 cursor 与 response 不一致、新 identity/consumer/cursor 漂移都会返回 typed invariant failure，不提交 pointer。

## 5. BindFresh 与 AdoptExact 提交顺序

`BindFresh` 固定顺序为：

1. 验证 active Owner/Host/Adapter 结构；
2. 地址匹配 exact live old surface；
3. 读取 old/new identity、consumer 与 cursor；
4. 复核 Host physical cursor、旧 surface 与 P20.46 ticket 均为 Empty；
5. 不调用 retirement；
6. 在临时 Owner copy 内同时替换 Owner 与 Adapter pointer；
7. 写入新 identity 与 FreshBound receipt；
8. candidate joint validation 通过后一次赋值提交。

`AdoptExact` 在第 5 步只调用一次旧 surface retirement。只有 response Applied、旧 surface 变为 Empty、新 surface 仍保持 ticket 的 exact visible snapshot 时，才继续联合 pointer commit。这样不会把旧 surface 留在可见状态后再采用新 surface。

## 6. Rejection、半提交与恢复信号

旧 surface 若返回合法 Rejected 且保持原 cursor，Owner 仍指向旧 surface并保持有效，结果提供 `CanRetryExactTicket`；重试必须由调用方发起，transaction 内没有循环。

若旧 surface 已 mutation，却返回 invalid response，或 response/实际 cursor 不一致，transaction 不伪造补偿、不切换 pointer。结果区分：

- `RetirementResponseInvalid`；
- `RetirementInvariantViolation`；
- `CommitInvariantViolation`。

当旧 surface 已清空而 Owner authority 仍为 visible，`IsOwnerValidAfter=false` 与 `NeedsManualRecovery=true` 明确暴露半提交状态。P20.47 不擅自 rehydrate 新旧 surface，也不依据不可信 response 完成 pointer rotation。

## 7. Idempotent replay 与 operation guard

Owner 保存最近一次成功 receipt 与当前 concrete surface identity。相同 ticket 再次提交时，只有以下条件同时成立才返回 `Replayed`：

- Owner 与 Adapter 当前 pointer 都是传入的新 surface；
- Owner 保存的 instance identity 等于 ticket replacement identity；
- 新 surface 当前 identity、consumer 与 cursor 仍匹配 ticket；
- Owner joint invariant 仍有效。

replay retirement call count 为 0，并返回同一个 receipt identity。若最近 receipt 匹配 ticket、但当前 binding 不匹配，则返回 `TicketReplayConflict`。

operation guard 在任何已匹配 surface callback 前安装。旧 surface 的 retirement callback 递归进入同一 Owner handoff 时，内部请求在 surface query 与 retirement 前返回 `OperationInProgress`；外层仍可按一次调用完成。

本阶段只记忆最近一次成功 ticket，不宣称跨多次 handoff、进程重启或崩溃边界的全历史 ticket ledger。

## 8. Composition Owner invariant 扩展

legacy `TryBegin` 仍可绑定普通 surface，不强迫现有调用方立即提供 concrete identity。只有成功 handoff 后，Owner 才进入 identity-bearing binding：

- `IdentitySurface` 的 base pointer 必须等于 Owner surface pointer；
- identity query 必须等于 `BoundSurfaceInstanceId`；
- 最近 receipt 必须匹配 Owner Run、consumer 与 bound instance；
- Adapter physical cursor 继续与 Host physical cursor 同步。

`TryEnd` 在 Host 与 Adapter 原子结束后同时清空 identity surface、bound instance 与 receipt，恢复默认 empty invariant。

## 9. 自动化与 changed-file 证据

新增 6 项 focused automation：

1. `EvidenceContract`：invalid ticket 在任何 surface callback 前拒绝；
2. `BindFreshCommit`：两个 Empty surface 零 retirement 原子切换、幂等 replay 与后续 dispatch；
3. `AdoptExactCommit`：旧 visible surface 单次 retirement、receipt 与后续 replacement dispatch；
4. `RetirementRejectedRetry`：合法 unchanged rejection 与一次外部显式 retry；
5. `IdentityAndSnapshotFences`：错误旧 pointer 与新 consumer substitution 的零 retirement fence；
6. `InvariantAndReentrant`：mutation + invalid evidence 的 manual-recovery signal，以及 callback reentry fence。

首次 focused 为 5/1。唯一失败是测试取样顺序错误：测试在断言“错误旧 pointer 不读取新 surface”前先执行了下一次 consumer-drift 尝试，污染了 query counter；生产结果状态正确。首次失败日志已原样保留。移动断言取样点后，正式 focused 为 6/0。

新增 OwnerSurfaceHandoff exact-path mapping rule。8 个产品/测试/流程改动路径命中 4 条规则，求并集要求 32 个测试组；3 份正式日志全部满足映射。

- mapping self-test：387/387；
- changed-file gate：PASS，Changed=8 / Rules=4 / Required=32 / Logs=3；
- boundary scan：PASS，handoff 2 files / 1167 lines；UWorld/AActor/UObject/ApplyDamage/RNG 与 direct Show/Replace/Hide/ClearToEmpty dispatch 均为 0；
- handoff 的唯一新 mutation capability 为一次显式 `RetireForHandoff`；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| P20.47_Focused_FirstFail.log | Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff | 5/1 | `8A215F3779599736ED93CC3D5803A374F22DDB6EA98CCB506B3F8CA1768B4E76` |
| P20.47_Focused.log | Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff | 6/0 | `17C821048BA40A4F102AC282B44E5B7F5DABB455E79ED9BA226C19B2408CE218` |
| P20.47_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | `7BE432D4040B6EABF4BF28ECEA80358E9E63CE2DA8B3F8E5B39315E5A6B958EA` |
| P20.47_Full.log | Shanmen.0_0_10 | 1113/0 | `979CBCBB1D2CB6BFBD1027171A8E56B2336367146899B35841427D2240233303` |

正式通过日志累计 1165/0，包含 focused、legacy 与完整套件重叠；独立完整套件从 P20.46 的 1107 增至 1113/0。完整套件出现 94 条既有 SwordRhythm journal/checkpoint/envelope/manifest codec 非惩罚性 unresponsive 通知；进程持续推进，最终由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束，Fatal/Unhandled/Ensure 为 0。

构建：

- initial Editor source integration：10 actions / native 0 / 32.84 秒；
- test integration Editor：5 actions / native 0 / 10.15 秒；
- focused-fix Editor：4 actions / native 0 / 13.91 秒；
- final Editor：0 actions / native 0 / 1.44 秒；
- final Game：9 actions / native 0 / 23.77 秒；
- Editor DLL：17399808 bytes，SHA-256 `B925B375554B4756F974C7BB3E70EA46C10DCDCDAD595396C17AA26BD2A5848C`；
- Game EXE：358477312 bytes，SHA-256 `2C0119E2AED1898459608A5B001D2689D5FCDC046B5EDF891B03DBDF43D3E135`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：exact live-old pointer preflight、identity-bearing replacement、P20.46 ticket consumption、BindFresh zero-mutation commit、AdoptExact old-surface retirement、deterministic retirement response/receipt、Owner+Adapter atomic pointer replacement、latest-ticket idempotent replay、unchanged rejection retry、post-mutation manual-recovery signal、consumer/cursor/identity substitution fence、callback reentry fence、legacy Owner teardown 与 changed-file regression。

未验证：全历史/持久化 ticket ledger、进程崩溃窗口恢复、跨线程同步、拥有型 renderer handle、自动补偿或 rehydrate、真实 renderer/MainHUD/widget/component/visible frame、真实输入、World、Editor UI/PIE/Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.48 建立 handoff recovery journal/checkpoint：在 retirement 已发生而 pointer commit 未完成的窗口持久化 bounded recovery evidence，明确“旧已空、新仍 exact visible、Owner authority 未变”时可采取的显式恢复动作，并保持 invalid response 不能直接升级为可信 commit。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-47-thrown-weapon-arc-preview-owner-surface-handoff>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-47-thrown-weapon-arc-preview-owner-surface-handoff/Docs/Report/Dev.D.UE.0.0.10.P20.47.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-47-thrown-weapon-arc-preview-owner-surface-handoff/Docs/Log/Dev.D.UE.0.0.10.P20.47.r0_log.md>
