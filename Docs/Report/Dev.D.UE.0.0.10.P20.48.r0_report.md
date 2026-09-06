# Dev.D.UE.0.0.10.P20.48.r0 Report

## 1. 结论

P20.48 已建立 Arc preview Owner surface handoff 的显式半提交恢复事务。它只接受 P20.47 已经自证为 `NeedsManualRecovery` 的特定失败形态：旧 surface 确实从 exact visible 退休为空、新 surface 仍保持 ticket 的 exact visible snapshot，而 Owner/Adapter pointer 尚未提交。

恢复不会把失效或矛盾的 retirement response 当作授权。P20.47 结果只用于生成 deterministic immutable checkpoint；每次恢复仍独立重读 exact live old/new surface，并重新核验 Host、Adapter、Run、consumer、logical/physical cursor 和既有 binding metadata。全部事实匹配后，才在临时 Owner copy 中原子切换 Owner/Adapter pointer。

恢复事务不调用任何 surface mutator。成功后保存独立 deterministic recovery receipt，并清除 normal handoff receipt；Owner 的 identity-bearing 状态要求 normal/recovery receipt 严格二选一。最新 checkpoint 重放只读取当前 new surface，不再读取 old surface，也不再次改变 pointer。

本阶段 checkpoint 与 receipt 仅为进程内不可变值对象，不宣称磁盘持久化、进程重启恢复或 crash journal durability。

## 2. 基线、分支与改动范围

- 基线：`4afa53912bf770581f22a4e26366a05f843ec091`（P20.47）；
- 分支：`agent/0.0.10-p20-48-thrown-weapon-arc-preview-owner-surface-handoff-recovery`；
- 新增 `demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.h/.cpp`；
- P20.47 result 新增只读 transition ticket accessor，供 checkpoint 冻结原始事实；
- Composition Owner 新增 recovery receipt，并扩展 identity-bearing invariant 与 teardown；
- Consumer Adapter 只向 recovery coordinator 开放受限 friend commit；
- normal P20.47 handoff 成功时清除旧 recovery receipt；
- 扩展 ProductLifecycle automation、changed-file mapping 与 mapping self-test；
- 新增本 Report 与同名 Log；
- 未修改 Host authority、pending command、presentation state 或 delivery cursor；
- 未调用真实 renderer、MainHUD、widget、component 或 World。

## 3. Immutable recovery checkpoint

`TryCreate` 只接受一个 valid P20.47 manual-recovery result，并要求：

- failure status 属于明确的 post-retirement recoverable failure；
- action 为 `AdoptExact`；
- retirement call count 严格为 1；
- normal handoff receipt 不存在；
- P20.47 结束时 Owner 无效；
- old/new surface identity 均有效、不同，并与 ticket 对齐；
- old-before 为 ticket 的 exact visible cursor；
- old-after 为 Empty；
- new-after 仍为 ticket 的 exact candidate snapshot。

checkpoint 保存 deterministic CheckpointId、ticket、source failure status、source response identity、old/new identity 与三个 cursor。source response 只作为失败来源的 provenance 参与 checkpoint identity，不授权 pointer commit。

同一份失败事实生成同一 CheckpointId；字段篡改、普通 rejection、未退休、new snapshot 漂移、已产生 receipt 或 Owner 仍有效均不能生成 checkpoint。

## 4. 独立双 surface 与 authority 复核

恢复调用方必须显式提供仍存活的 `ExpectedRetiredSurface` 与 `NewSurface`。事务首先比较 exact base pointer：Owner 必须仍指向传入 old surface，且 old/new 必须为不同对象；此条件失败时 surface read count 为 0。

operation guard 在第一条 surface query 前安装。随后按固定顺序读取：

1. old identity、consumer、cursor，必须是 checkpoint 的 old identity 与 Empty；
2. new identity、consumer、cursor，必须仍匹配 P20.46 transition ticket；
3. Host 与 Adapter 必须 active、结构一致、无 pending recovery/operation；
4. Run 与 consumer 必须与 ticket 相同；
5. Host logical cursor 投影出的 physical cursor 必须仍等于 ticket expected visible snapshot；
6. Owner 必须恰处于 P20.47 的半提交无效态，而不是另一个有效或任意损坏状态；
7. legacy metadata 必须全空，或 identity metadata 必须精确绑定 old surface 且只有一种 receipt。

任一复核失败均返回 typed status，不切换 pointer，不修改 surface。

## 5. Zero-mutation atomic pointer commit

复核通过后，coordinator 先生成 deterministic recovery receipt，再复制 Owner 形成 candidate：

- Owner surface pointer 指向 new surface；
- Adapter surface pointer 同步指向 new surface；
- Adapter 上一次 application result 清空；
- Owner identity surface 与 bound instance 改为 new identity；
- normal handoff receipt 清空；
- recovery receipt 写入。

只有 candidate `IsValid()` 与 `IsSynchronized()` 同时通过，才以一次 Owner assignment 提交。恢复文件中 direct Show/Replace/Hide/ClearToEmpty/RetireForHandoff 调用均为 0；old 维持 Empty，new 维持 exact visible。

## 6. Recovery receipt、重放与重入

recovery receipt 固定绑定 ReceiptId、CheckpointId、TicketId、Run、consumer、old/new identity、old Empty cursor 与 new exact cursor。它与 normal P20.47 receipt 是两种不同证据，不伪装成一次正常 handoff。

同 checkpoint 的最近一次重放要求 Owner/Adapter 已绑定相同 new surface、normal receipt 为空、recovery receipt 与 checkpoint 匹配，且 new surface 当前 snapshot 仍 exact。成功返回 `Replayed` 与原 ReceiptId，只读取 new surface一次；old surface read 与任意 mutator 调用均为 0。

若 checkpoint 已被另一 binding 消费，或 new snapshot 漂移，则返回 `CheckpointReplayConflict`。surface callback 递归进入同一 Owner 时，内部恢复在任何 query 前返回 `OperationInProgress`，外层仍可有界完成。

## 7. Owner invariant 与后续 normal handoff

identity-bearing Owner 现在要求：

- identity surface pointer、base surface pointer 与 bound instance 三者一致；
- normal receipt 与 recovery receipt 严格二选一；
- 存在的 receipt 必须匹配当前 Run、consumer 与 bound instance；
- Adapter pointer 与 Host/Adapter cursor 继续同步。

`TryEnd` 同时清除两类 receipt。恢复成功后若继续执行合法 P20.47 normal handoff，新 normal receipt 会取代 recovery receipt，证明证据随当前 binding 正常轮换，不会长期并存。

## 8. 自动化与 changed-file 证据

新增 6 项 focused automation：

1. `EvidenceContract`：只允许可信 manual-recovery failure 生成 deterministic checkpoint；
2. `ZeroMutationCommit`：双 surface 复核、零 surface mutation、Owner/Adapter 原子提交与后续只向 new dispatch；
3. `IdempotentReplay`：最近 checkpoint 重放只读取 new surface并复用 receipt；
4. `OldSurfaceFences`：错误 old pointer、identity、consumer 与非 Empty cursor fail closed；
5. `NewAndAuthorityFences`：new snapshot 与 Host/Adapter authority 漂移 fail closed；
6. `ReentrantAndReceiptRotation`：callback reentry fence，以及后续 normal handoff 的 receipt 轮换。

验证结果：

- mapping self-test：389/389；
- changed-file gate：PASS，Changed=10 / Rules=5 / Required=33 / Logs=3；
- boundary scan：2 recovery files / 979 lines；UWorld/AActor/UObject/ApplyDamage/RNG 与所有 direct surface mutation 调用均为 0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| P20.48_Focused.log | Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery | 6/0 | `807CA14D77E87E4FE814FFDC5456BC786EBD177AC34CEAB0569F6C941E4F1027` |
| P20.48_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | `D81B27CB1D5BB7DFC5C099C3B7F58F1918A7432EACD58CAF13C488D60DDA4EEC` |
| P20.48_Full.log | Shanmen.0_0_10 | 1119/0 | `F72FC65F4B2712A6647249B6F8D22A20B9450C785CC0EEB75E01BEC57EC3F8FB` |

正式通过日志累计 1171/0，包含 focused、legacy 与完整套件重叠；独立完整套件从 P20.47 的 1113 增至 1119/0。完整日志出现 102 条既有 SwordRhythm 大时间差非惩罚性通知，测试持续推进并最终由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束；Fatal/Unhandled/Ensure 为 0。

## 9. 构建与产物

- initial Editor source integration：11 actions / native 0 / 25.56 秒；
- test integration Editor：4 actions / native 0 / 8.61 秒；
- final Editor：0 actions / native 0 / 1.23 秒；
- final Game：10 actions / native 0 / 23.64 秒；
- Editor DLL：17459712 bytes，SHA-256 `A876912045ED3BF6E61790058BB9B9958EFEE748A0976D4BEDDB0FAB602D82A3`；
- Game EXE：358523904 bytes，SHA-256 `D3A59F8DCCEEF20B05524BD889B060940E1A727B2997EB022C9843E14CE9EA99`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：manual-recovery checkpoint admission、deterministic checkpoint/receipt、invalid response 非授权、exact old pointer fence、old Empty/new exact snapshot 复核、Host/Adapter authority 复核、零 surface mutation、Owner+Adapter atomic pointer recovery、normal/recovery receipt 互斥、latest-checkpoint idempotent replay、callback reentry fence、后续 normal handoff receipt rotation、legacy teardown 与 changed-file regression。

未验证：checkpoint/receipt 序列化、磁盘 journal、进程/机器重启、崩溃后文件恢复、全历史 checkpoint ledger、跨线程同步、拥有型 renderer handle、自动 rehydrate/补偿、真实 renderer/MainHUD/UI/visible frame、真实输入、World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.49 为 recovery checkpoint/receipt 建立 canonical codec 与 append-only bounded journal，先验证 round-trip、corruption/truncation、latest-record selection 与 N-1 migration，再讨论真正的 process-restart recovery。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-48-thrown-weapon-arc-preview-owner-surface-handoff-recovery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-48-thrown-weapon-arc-preview-owner-surface-handoff-recovery/Docs/Report/Dev.D.UE.0.0.10.P20.48.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-48-thrown-weapon-arc-preview-owner-surface-handoff-recovery/Docs/Log/Dev.D.UE.0.0.10.P20.48.r0_log.md>
