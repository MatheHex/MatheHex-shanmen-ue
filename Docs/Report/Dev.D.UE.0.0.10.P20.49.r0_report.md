# Dev.D.UE.0.0.10.P20.49.r0 Report

## 1. 结论

P20.49 已为 Arc preview Owner surface handoff recovery 建立独立可解码的 canonical metadata journal。journal 以严格有界、append-only 的记录链保存 `CheckpointPrepared` 与 `RecoveryCommitted` 证据，支持确定性字节布局、完整性自证、最新记录选择、幂等重放、冲突拒绝与 N-1 迁移。

本阶段刻意不序列化 P20.48 checkpoint/receipt 内部的私有 presentation payload。解码结果只是 candidate-bound audit/rebinding manifest，不能凭磁盘字节制造 recovery authorization，也不能直接执行 surface recovery。调用方仍必须持有或通过后续受信机制重建有效 P20.48 checkpoint/receipt，并用 manifest 做 exact match；恢复时仍需 P20.48 的 fresh live old/new surface 与 Host/Adapter authority 复核。

codec 不读写文件、不持有 surface/Owner、不执行 recovery、不重试、不调度。它只把稳定身份与 cursor 证据转换为 canonical bytes，并在解码时 fail closed。

## 2. 基线、分支与改动范围

- 基线：`644e5f36e02f3eede8390d7b10f0b310266bc128`（P20.48）；
- 分支：`agent/0.0.10-p20-49-thrown-weapon-arc-preview-handoff-recovery-journal-codec`；
- 新增 `demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.h/.cpp`；
- 扩展既有 ProductLifecycle automation，新增 6 项 journal/codec 测试；
- 扩展 changed-file regression mapping 与 mapping self-test；
- 新增本 Report 与同名 Log；
- 未修改 P20.48 recovery coordinator、surface mutator、Host/Adapter authority 或产品运行路径；
- 未连接文件系统、网络、真实 renderer、MainHUD、widget、component 或 World。

## 3. Append-only recovery metadata journal

每条 record 保存可跨编码边界稳定比较的事实：

- record kind、sequence、previous record identity 与 deterministic RecordId；
- CheckpointId、TransitionTicketId、RunId 与 consumer definition digest；
- source retirement response identity 与 source failure status；
- previous/new surface instance identity；
- previous/new visible cursor identity；
- committed record 的 RecoveryReceiptId。

journal 的合法链严格交替为：

1. 偶数 sequence：`CheckpointPrepared`；
2. 紧随其后的奇数 sequence：同 checkpoint 的 `RecoveryCommitted`；
3. 每条非首记录必须引用前一 RecordId；
4. JournalId 由有序 RecordId 列表确定性派生。

空 journal 也有稳定、非零、可 round-trip 的 JournalId。任意 record identity、顺序、predecessor、checkpoint binding 或 journal identity 不一致都会使结构无效。

## 4. Append、重放、冲突与容量

`AppendCheckpoint` 与 `AppendRecoveryReceipt` 只接受有效 P20.48 evidence：

- 空尾或上一 recovery 已完成时才能追加 checkpoint；
- pending checkpoint 只能由 exact matching receipt 关闭；
- 同一最新 checkpoint/receipt 再次提交返回 `Replayed`，不追加、不改写；
- foreign checkpoint 不能替换 pending tail；
- 无 pending checkpoint 的 receipt fail closed；
- completed checkpoint replay 不会重新打开事务。

容量固定为 16 条，即 8 个完整 checkpoint/receipt recovery pair。达到上限后第 17 条返回 `CapacityExceeded`；旧记录、RecordId 顺序与 JournalId 均保持不变，不驱逐、不环绕、不压缩、不覆盖。

## 5. Canonical current schema

current schema 使用显式 big-endian 固定宽度布局：

- magic：`SMAPRJNL`；
- schema version：2；
- header：32 bytes；
- current record：204 bytes；
- header 保存 schema、record count 与 JournalId；
- record 保存 kind、sequence、RecordId、PreviousRecordId、全部 semantic GUID 与 source failure status。

同一 journal 重复编码产生完全相同的 bytes；decode 后再 encode 仍字节一致。两记录完成态大小固定为 440 bytes，16 记录最大态固定为 3296 bytes。

decoder 严格拒绝：空输入、magic 错误、未知 schema、越界 record count、长度不符、尾随 bytes、枚举超出 `uint8`、record digest 不匹配、链不连续、journal identity 不匹配与整体 invariant 失败。

## 6. N-1 迁移

previous schema version 1 只允许一个初始 pending checkpoint：

- header 仍为 32 bytes；
- previous record 为 168 bytes；
- 单记录文件总长 200 bytes；
- legacy digest 覆盖全部可迁移语义字段；
- decode 后补齐 current sequence、predecessor 与 RecordId，生成 current valid chain；
- 迁移后的 journal 可重新编码为 schema 2，并保持 candidate binding 与 JournalId；
- completed 或 multi-record journal 不允许降级写成 N-1。

此迁移只恢复 metadata manifest，不恢复私有 checkpoint payload，也不授权自动 recovery。

## 7. 自动化验证

新增 6 项 focused automation：

1. `EvidenceContract`：合法证据、顺序、deterministic append 与幂等重放；
2. `CanonicalRoundTrip`：current/empty canonical bytes 与 re-encode 稳定性；
3. `CorruptionAndTruncation`：当前 440-byte 文件逐字节单 bit 翻转、全部严格前缀截断与尾随 byte 拒绝；
4. `LatestSelectionAndConflicts`：pending/committed latest selection、foreign evidence 与 completed replay；
5. `PreviousSchemaMigration`：200-byte N-1 逐字节单 bit 翻转、迁移与 current re-encode；
6. `BoundedAppendOnly`：16 条完整链、容量拒绝不驱逐与最大文件 round-trip。

验证结果：

| Log | Group | Success/Fail | Native exit | SHA-256 |
|---|---|---:|---:|---|
| P20.49_Focused.log | Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal | 6/0 | 0 | `3A818BB34502514B9182CE370540A412446D6AE36F15012F1A6A309538A16584` |
| P20.49_Legacy.log | demo_map.ItemUseAndArmor | 46/0 | 0 | `76E9FE5AA7A26FA41FE759285D40E69EA3748CEB66D3029142D2E0DCBF2B4D16` |
| P20.49_Full.log | Shanmen.0_0_10 | 1125/0 | 0 | `897DC70CF37793EF49D8F757F5A7C62532C5CF38E4F1817E588D06F8EC1FAF7E` |

三份正式日志累计 1177/0，包含 focused、legacy 与完整套件重叠；每份均有 UE 原生 `TEST COMPLETE. EXIT CODE: 0`。完整套件从 P20.48 的 1119 增至 1125/0。Focused/Legacy/Full 分别出现 6/1/106 条 UE 既有“not penalizing unresponsive tests”提示，队列持续推进并正常结束；Fatal/Unhandled/Ensure 均为 0。

## 8. Changed-file 与架构证据

- mapping JSON 解析：PASS，220 rules；
- mapping self-test：391/391；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=34 / Logs=3；
- boundary scan：2 journal files / 1302 lines；
- `UWorld/AActor/UObject/ApplyDamage/FMath::Rand/FRandomStream`：0；
- direct `Show/Replace/Hide/ClearToEmpty/RetireForHandoff`：0；
- `FFileHelper/IFileManager`：0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

## 9. 构建与产物

- initial Editor source integration：4 actions / native 0 / 28.42 秒；
- test integration Editor：4 actions / native 0 / 12.90 秒；
- final Editor：0 actions / native 0 / 1.54 秒；
- final Game：4 actions / native 0 / 25.15 秒；
- Editor DLL：17539584 bytes，SHA-256 `9CB8054042EE5B2998F90E4F2B9DDD28EAAAFBE6AB93118D3655905A5C522664`；
- Game EXE：358586880 bytes，SHA-256 `BAF08662D7FC3C647E6744E6852CB01C0E40F7831B7E72E479A16B376881CAD7`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：candidate-bound recovery metadata、deterministic record/journal identity、strict alternating append chain、latest selection、idempotent replay、foreign evidence conflict、16-record capacity、canonical current bytes、empty/max journal round-trip、逐字节 corruption、全部 strict-prefix truncation、trailing bytes、N-1 single-pending migration、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未验证且不声明：私有 checkpoint/receipt payload 序列化、仅凭 decoded bytes 重建 checkpoint、仅凭 journal 授权或执行 recovery、磁盘文件写入、atomic replace、fsync、process/machine restart、crash durability、全历史 ledger、跨线程同步、拥有型 renderer handle、真实 renderer/MainHUD/UI/visible frame、真实输入、World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

建议 P20.50 先建立完整 recovery evidence payload envelope：以 canonical、版本化、可校验的方式保存重建 checkpoint 所需的不可变 presentation/ticket 状态，同时继续要求 live surface authority rebind。只有这一步完成后，磁盘 atomic storage adapter 才能从“审计持久化”升级为真正有意义的 process-restart recovery 基础。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-49-thrown-weapon-arc-preview-handoff-recovery-journal-codec>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-49-thrown-weapon-arc-preview-handoff-recovery-journal-codec/Docs/Report/Dev.D.UE.0.0.10.P20.49.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-49-thrown-weapon-arc-preview-handoff-recovery-journal-codec/Docs/Log/Dev.D.UE.0.0.10.P20.49.r0_log.md>
