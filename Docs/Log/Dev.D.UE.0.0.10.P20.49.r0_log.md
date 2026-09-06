# Dev.D.UE.0.0.10.P20.49.r0 Log

## 阶段

- 任务：P20.49 thrown-weapon Arc preview Owner surface handoff recovery metadata journal/codec；
- 基线：`644e5f36e02f3eede8390d7b10f0b310266bc128`；
- 分支：`agent/0.0.10-p20-49-thrown-weapon-arc-preview-handoff-recovery-journal-codec`；
- 目标：为 P20.48 checkpoint/receipt 建立不伪造恢复授权的 canonical metadata manifest、append-only bounded journal 与 N-1 migration。

## 实现记录

1. 新增独立 recovery journal record。
2. record kind 仅允许 CheckpointPrepared 或 RecoveryCommitted。
3. record 保存 sequence 与 previous RecordId。
4. RecordId 由完整稳定字段确定性派生。
5. record 保存 CheckpointId 与 TransitionTicketId。
6. record 保存 RunId 与 consumer definition digest。
7. record 保存 source retirement response identity。
8. record 保存 P20.48 source failure status。
9. record 保存 old/new surface instance identity。
10. record 保存 old/new visible cursor identity。
11. committed record 保存 RecoveryReceiptId。
12. prepared record 必须无 RecoveryReceiptId。
13. record 可以 exact match 原 P20.48 checkpoint。
14. committed record 可以 exact match原 checkpoint/receipt pair。
15. 新增 caller-owned append-only journal。
16. journal 不拥有 surface、Owner、scheduler、retry 或 file handle。
17. journal chain 固定 prepared/committed 交替。
18. 非首记录必须引用前一 RecordId。
19. JournalId 由有序 RecordId 列表确定性派生。
20. 空 journal 保持 valid 且有 canonical identity。
21. AppendCheckpoint 只接受 valid P20.48 checkpoint。
22. AppendRecoveryReceipt 只接受 matching valid receipt。
23. 同一最新 checkpoint append 返回 Replayed。
24. 同一最新 receipt append 返回 Replayed。
25. completed replay 不重新打开 pending transaction。
26. foreign checkpoint 不能覆盖 pending tail。
27. foreign receipt 不能关闭 pending checkpoint。
28. 无 pending checkpoint 的 receipt fail closed。
29. journal 最大容量固定为 16 records。
30. capacity failure 不驱逐、覆盖或改写旧记录。
31. 新增 big-endian canonical binary codec。
32. magic 固定为 SMAPRJNL。
33. current schema 固定为 version 2。
34. header 固定 32 bytes。
35. current record 固定 204 bytes。
36. current bytes 显式保存 sequence、predecessor 与 RecordId。
37. current encode 对同一 journal 字节确定。
38. decoded current journal re-encode 字节一致。
39. 空 journal 有 32-byte canonical representation。
40. 2-record completed journal 固定为 440 bytes。
41. 16-record maximum journal 固定为 3296 bytes。
42. decoder 拒绝空输入与截断 header。
43. decoder 拒绝 magic mismatch。
44. decoder 拒绝 unknown schema。
45. decoder 拒绝 record count 越界。
46. decoder 拒绝 exact-size mismatch 与 trailing bytes。
47. decoder 拒绝超出 uint8 的 enum 值。
48. decoder 重算 RecordId 并拒绝 semantic corruption。
49. decoder 核验 predecessor chain。
50. decoder 重算 JournalId 并拒绝 envelope corruption。
51. decoder 不执行 recovery 或任何 surface mutation。
52. codec 不执行文件或网络 IO。
53. previous schema 固定为 version 1。
54. previous record 固定 168 bytes。
55. N-1 只允许一个初始 pending checkpoint。
56. N-1 legacy digest 覆盖可迁移 semantic fields。
57. N-1 decode 补齐 current sequence/predecessor/RecordId。
58. migrated journal 可 canonical re-encode 为 current schema。
59. completed/multi-record journal 不能降级写为 N-1。
60. decoded manifest 不制造 P20.48 checkpoint 或 recovery authorization。
61. 后续执行仍必须 rebind valid checkpoint/receipt 与 live authority。
62. 未连接真实 renderer、MainHUD、widget、component 或 World。

## 自动化新增

- `EvidenceContract`；
- `CanonicalRoundTrip`；
- `CorruptionAndTruncation`；
- `LatestSelectionAndConflicts`；
- `PreviousSchemaMigration`；
- `BoundedAppendOnly`。

当前两记录文件执行 440 次逐字节单 bit corruption 与 440 个 strict-prefix truncation，接受数均为 0；尾随 byte 被拒绝。N-1 200-byte 文件执行 200 次逐字节单 bit corruption，接受数为 0。容量测试建立 8 个完整 recovery pair，确认第 17 条拒绝且先前 16 条 identity 不变。

## Changed-file regression

- 新增 RecoveryJournal exact-path mapping rule；
- Recovery、OwnerSurfaceHandoff、CompositionOwner 与 ConsumerAdapter rule 增加 journal focused group；
- RequiredGroups：34；
- mapping JSON：220 rules / parse PASS；
- mapping self-test：391/391；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=34 / Logs=3；
- focused journal、legacy ItemUseAndArmor 与完整 Shanmen.0_0_10 均提供原生 terminal evidence。

## 验证证据

- focused journal：6/0，268691 bytes，SHA-256 `3A818BB34502514B9182CE370540A412446D6AE36F15012F1A6A309538A16584`；
- legacy ItemUseAndArmor：46/0，303195 bytes，SHA-256 `76E9FE5AA7A26FA41FE759285D40E69EA3748CEB66D3029142D2E0DCBF2B4D16`；
- full Shanmen.0_0_10：1125/0，1636153 bytes，SHA-256 `897DC70CF37793EF49D8F757F5A7C62532C5CF38E4F1817E588D06F8EC1FAF7E`；
- formal pass logs total：1177/0，3 native terminal，0 fail；
- focused/legacy/full non-penalty notices：6/1/106；
- Fatal/Unhandled/Ensure：0；
- boundary scan：2 journal files / 1302 lines / forbidden Engine-World、direct surface mutation 与 file IO 均为 0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

## 构建与产物

- initial Editor source integration：4 actions / native 0 / 28.42s；
- test integration Editor：4 actions / native 0 / 12.90s；
- final Editor：0 actions / native 0 / 1.54s；
- final Game：4 actions / native 0 / 25.15s；
- Editor DLL：17539584 bytes / SHA-256 `9CB8054042EE5B2998F90E4F2B9DDD28EAAAFBE6AB93118D3655905A5C522664`；
- Game EXE：358586880 bytes / SHA-256 `BAF08662D7FC3C647E6744E6852CB01C0E40F7831B7E72E479A16B376881CAD7`。

## P/F 边界

PASS：candidate-bound metadata manifest、record/journal deterministic identity、strict alternating hash chain、append/replay/conflict、16-record no-eviction capacity、current canonical bytes、empty/max round-trip、corruption/truncation/trailing rejection、N-1 pending migration、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未声明：private checkpoint/receipt payload codec、decoded-only checkpoint construction、decoded-only recovery authorization、disk journal、atomic replace/fsync、process/crash restart、全历史 ledger、跨线程同步、拥有型 renderer handle、真实 renderer/MainHUD/UI/visible frame、真实输入、World、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.50 建议建立完整 recovery evidence payload envelope，使 checkpoint 的不可变 presentation/ticket 状态可受版本控制地重建，同时仍要求 live surface/Host/Adapter rebind；随后再接 caller-owned atomic file storage adapter。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-49-thrown-weapon-arc-preview-handoff-recovery-journal-codec>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-49-thrown-weapon-arc-preview-handoff-recovery-journal-codec/Docs/Report/Dev.D.UE.0.0.10.P20.49.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-49-thrown-weapon-arc-preview-handoff-recovery-journal-codec/Docs/Log/Dev.D.UE.0.0.10.P20.49.r0_log.md>
