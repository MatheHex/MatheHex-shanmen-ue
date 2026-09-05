# Dev.D.UE.0.0.10.P20.41.r0 Report

## 1. 结论

P20.41 已建立 consumer-owned、Run-scoped 的 thrown-weapon Arc preview presentation delivery Host。Host 组合 P20.35 state Session、P20.36 command projector 与 P20.39/40 delivery Session，把一次更新冻结为最多一次 `state update -> command projection -> delivery`，并使产品侧 desired state、consumer cursor 与 rejected-command recovery fence 由同一 owner 管理。

Applied 与 exact replay 只在 candidate Host 完整自验证后提交同步状态。renderer 拒绝会保留 rejected receipt、目标 state 与 exact pending command，并在 recovery 完成前拒绝所有较新更新；被拒 Hide 恢复后 cursor 才能推进为 Hidden 并允许 graceful end。

本阶段仍是 P 阶段无头契约：使用 fake port 验证顺序、计数、回放、拒绝、恢复与重入边界，没有连接真实 HUD、widget、component、World 或可见帧。

## 2. 基线、分支与改动范围

- 基线：`ea52b9b4af0f1484d7b73cb0d43c5a10f96550a9`（P20.40）；
- 分支：`agent/0.0.10-p20-41-thrown-weapon-arc-preview-delivery-host`；
- 新增 `demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost.h/.cpp`；
- 扩展 `demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`；
- 扩展 changed-file regression map 与其自测；
- 新增本 Report 与同名 Log；
- 未修改既有 state Session、projector、delivery Session 或 renderer port 的 public contract；
- 未连接真实 HUD、World、Actor、widget、component、输入、projectile、库存或产品 mutation。

## 3. Run scope 与组合顺序

`TryBegin(RunId, ConsumerDefinitionId)` 同时绑定两个 owned Session。exact scope replay 幂等成功；active Host 上的 Run 或 consumer rotation fail-closed，不产生部分重绑。

每次 `TryUpdate` 固定执行：

1. 检查 Host active、valid、非重入且没有 pending rejection；
2. 在 candidate Host 上调用一次 state Session update；
3. 仅在 state update accepted 时调用一次 pure command projector；
4. 仅在 projected command valid 时调用一次 delivery Session；
5. 校验 nested results、调用计数、Run/consumer、state/cursor 与 pending invariants；
6. 只有 typed Host result 自验证通过后才替换 live Host。

因此 inactive、invalid、reentrant、recovery-required、state rejection 与 projection rejection 都不会触达 renderer port。单轮调用预算由 result 明确记录为 0/1。

## 4. Applied、Replay 与拒绝原子性

- Applied：desired state 与 delivery cursor 同步提交；
- ApplicationReplayed：不重复推进 ledger/cursor，仍保持同步；
- delivery preflight rejection：live state、cursor、ledger 与 pending command 全部不变；
- renderer Rejected：提交 delivery rejection evidence、对应 desired state 和 exact pending command，cursor 保持旧值；
- pending 存在时，所有较新 update 在 state/projector/port 调用前返回 `RecoveryRequired`；
- 不可能的 post-port invariant failure 不提交 C++ live Host。

最后一条只保证本地状态不提交；外部 renderer 如果已经产生不可逆副作用，Host 不提供补偿事务。这是当前 port contract 的显式边界。

## 5. Narrow recovery seam

`TryRecoverRejected(AppliedReceipt)` 只接受与 pending rejected command 完全一致的 Applied receipt，并继续受 Run、consumer、CommandId 与完整 command payload 约束。

恢复在 candidate delivery Session 上执行：首次 exact Applied evidence 清除 pending 并使 state/cursor 重新一致；exact recovery replay 幂等接受；foreign、mismatched、无 pending 或冲突 receipt 在 live mutation 前拒绝。恢复不再次调用 renderer port。

恢复 receipt 是 caller attestation，不是密码学签名，也不证明真实 renderer 已经显示该状态。

## 6. 生命周期与重入边界

- Host operation guard 覆盖 update、recovery 与 end；
- fake port callback 内尝试 reentrant update/end 时，在 second state、projector、delivery 或 teardown effect 前被拒绝；
- `TryEnd` 要求 expected Run 匹配、没有 pending rejection、state/cursor 同步且二者为 Hidden/Empty；
- rejected Hide 会阻止 end；exact recovery 后 Hidden cursor 才允许两个 Session 原子结束；
- end 后 Host 回到 valid empty 状态，可由新 Run 重新绑定。

## 7. Typed self-validating evidence

Host update result 绑定 status、diagnostic、Run、consumer、三个调用计数、三个 nested result、before/after desired state、before/after cursor 与 before/after pending command。

Host recovery result绑定 delivery recovery 调用计数、nested recovery result、state、before/after cursor 与 before/after pending command。`IsValid()` 会交叉验证 status 分类、调用预算和所有状态转换，避免调用方只凭枚举值误判成功。

## 8. 自动化覆盖

新增 7 项 focused automation：

1. `Lifecycle`：begin replay、scope rotation fence、hidden end 与新 Run；
2. `OrderedAppliedReplay`：Show/Replace/NoOp、exact replay 与调用顺序；
3. `PreflightAndStateReject`：inactive、产品不可用与零 port effect；
4. `AtomicDeliveryReject`：consumer mismatch 的 candidate rollback；
5. `RejectionRecoveryFence`：rejected command、较新更新阻断、foreign receipt 与 exact recovery/replay；
6. `HideRecoveryEnd`：Rejected Hide 阻止 end，恢复后允许 end；
7. `ReentrantPortBlocked`：callback 内 update/end 重入封锁。

完整 `Shanmen.0_0_10` 从 P20.40 的 1071 增至 1078 项。

## 9. Changed-file 回归与验证证据

新增 `ThrownWeaponArcPreviewPresentationDeliveryHost` mapping rule。5 个改动路径命中 2 条规则，求并集要求 26 个测试组；3 份正式日志全部满足映射。

- mapping self-test：375/375；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=26 / Logs=3；
- boundary scan：PASS，Host 2 files / 1139 lines / forbidden 0 / state update 1 / projection 1 / delivery 1 / recovery 1；
- `git diff --check`：PASS，0 whitespace errors。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| host_final.log | Product.ThrownWeaponArcPreviewPresentationDeliveryHost | 7/0 | D1860CCCFE23AA3989B8261CB7B73E370994BD445D26F5180A06D627D2432DFE |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 598F327381657F5EE5D59F03116AAF318561B89422559BA0543C847615F757AA |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1078/0 | 8A60427794D71FBD8A6C7BDD7944D99B58BED469330CA520E221ADE6FD503A19 |

正式日志累计 1131/0，包含 focused、legacy 与完整套件重叠；独立完整套件为 1078/0。完整套件在既有 SwordRhythm journal/checkpoint/manifest codec 区段出现非惩罚性 unresponsive 通知，随后持续消耗 CPU、逐项成功并由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束；没有外层 timeout、拆组或人工中止替代结果。

构建：

- final Editor：up to date / 0 actions / native 0 / 1.50 秒；
- final Game：4 actions / native 0 / 24.40 秒；
- Editor DLL：17053696 bytes，SHA-256 `D3AFA14AD665418510D8313993C331F15CFDDDA48B2EB1442F825A3322FD588D`；
- Game EXE：358198784 bytes，SHA-256 `6F6C8B21D6239592FA3AC06AD02203222FB7E81E6C37CA4FACFFE7C8541864F6`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：Run/consumer scope、单次有界 update -> project -> deliver 顺序、candidate commit、Applied/replay 同步、Rejected pending recovery fence、exact Applied recovery/replay、Hide recovery end、typed result、callback reentrancy 与 changed-file regression。

未验证：真实 renderer/HUD/widget/component/visible frame、receipt 来源真实性或签名、外部 renderer 副作用补偿、跨线程同步、进程崩溃持久 exactly-once、真实输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件启动、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

建议 P20.42 建立 renderer-facing Arc preview consumer adapter contract：把 Show/Replace/Hide/NoOp 映射到一个 consumer surface，并冻结 applied/rejected response 与 surface cursor；继续使用无头 fake surface，真实 MainHUD/widget 绑定后置。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-41-thrown-weapon-arc-preview-delivery-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-41-thrown-weapon-arc-preview-delivery-host/Docs/Report/Dev.D.UE.0.0.10.P20.41.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-41-thrown-weapon-arc-preview-delivery-host/Docs/Log/Dev.D.UE.0.0.10.P20.41.r0_log.md>
