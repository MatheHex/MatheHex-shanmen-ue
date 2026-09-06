# Dev.D.UE.0.0.10.P20.50.r0 Log

## 阶段

- 任务：P20.50 thrown-weapon Arc preview Owner surface handoff recovery checkpoint payload envelope；
- 基线：`fb85117ba4a7121ce89e8007eced858316c8fb06`；
- 分支：`agent/0.0.10-p20-50-thrown-weapon-arc-preview-recovery-checkpoint-payload-envelope`；
- 目标：在 P20.49 pending metadata journal 之上完整重建 P20.48 checkpoint evidence，同时不从持久化字节制造 recovery authorization。

## 实现记录

1. 新增独立 checkpoint payload envelope。
2. envelope 只接受 valid pending journal 的 exact latest checkpoint。
3. envelope 保存 schema、EnvelopeId、JournalId、PayloadDigest 与 checkpoint。
4. EnvelopeId 绑定 schema、journal、checkpoint 与 payload digest。
5. 同一 evidence 重复 wrap/encode 字节确定。
6. committed 或 foreign journal 不能 unwrap envelope。
7. stale bytes 可解码为历史证据但不能重新授权。
8. 新增 big-endian canonical binary codec。
9. magic 固定为 SMARCPAY。
10. current schema 固定为 version 1。
11. header 固定 64 bytes。
12. declared total size 必须等于输入长度。
13. 最大输入固定 65536 bytes。
14. 单字符串 UTF-8 最大 2048 bytes。
15. source GameplayTags 最多 64 条。
16. tag 先排序并要求严格唯一。
17. decoder 拒绝非法 UTF-8 与 embedded NUL。
18. double 显式按 IEEE bits 以 big-endian 编码。
19. `-0.0` 在写入时规范为 `+0.0`，读取端拒绝非 canonical negative zero。
20. payload 保存 checkpoint 与 transition ticket 稳定字段。
21. payload 保存 presentation、choice、preview、plan 与 request identity。
22. payload 保存 action owner/source/content 与 GameplayTags。
23. payload 保存 Arc technique、origin、target 与 ballistic limits。
24. 不保存可从 plan 重建的 position/segment 数组。
25. 新增 input choice `TryRehydrate`。
26. choice factory 重算 StateId 并执行原 `IsValid`。
27. 新增 visible presentation `TryRehydrateVisible`。
28. presentation factory 从 preview 重建 segment identities 与派生显示值。
29. 新增 transition ticket `TryRehydrate`。
30. ticket factory重算 TicketId 并执行原 `IsValid`。
31. 新增 recovery checkpoint `TryRehydrate`。
32. checkpoint factory 重算 CheckpointId 并执行原 `IsValid`。
33. decoder 使用既有 action `TryCapture` 重建 action snapshot。
34. decoder 使用既有 Arc request `TryCapture` 重建 request。
35. decoder重新执行 Arc planner 并核对 PlanId。
36. decoder 重新执行 preview sampler 并核对 PreviewId。
37. decoder 逐层重建 choice、presentation、ticket、checkpoint。
38. 每层 expected identity mismatch 均 fail closed。
39. reconstructed checkpoint 重新编码后必须与输入 payload 逐字节一致。
40. payload digest mismatch 在语义重建前拒绝。
41. empty、短 header、超限输入、magic/schema/size mismatch 均分类拒绝。
42. 整份 canonical envelope 每个 byte 的单 bit corruption 均拒绝。
43. 全部 strict-prefix truncation 与 trailing byte 均拒绝。
44. codec 不执行文件或网络 IO。
45. codec 不拥有或修改 surface。
46. codec 不创建 recovery receipt 或提交 journal。
47. codec 不调度、不重试、不自动执行恢复。
48. decoded evidence 仍需当前 journal latest-pending exact match。
49. extracted checkpoint 仍需 P20.48 fresh old/new surface 与 Host/Adapter authority gate。
50. payload 首个 schema 暂无 N-1；P20.49 metadata journal migration 不变。
51. 未连接真实 renderer、MainHUD、widget、component 或 World。

## 自动化新增

- `EvidenceAndJournalBinding`；
- `CanonicalRoundTripAndRecovery`；
- `CorruptionTruncationAndTrailing`；
- `SemanticIdentityAndTags`；
- `BoundsAndUnknownSchema`；
- `ForeignJournalAndCommittedFence`。

开发首轮 focused：4/2。失败仅为两条测试 oracle 把 fixture 前置正常 surface 展示调用计入 payload 阶段；改为操作前后 counter snapshot 后，生产逻辑零改动，正式 focused 为 6/0。首轮失败日志由 UE 自动保留为 `P20.50_Focused-backup-2026.09.06-07.31.06.log`。

## Changed-file regression

- 新增 RecoveryCheckpointPayloadEnvelope exact-path mapping rule；
- Presentation、RecoveryJournal、Recovery、OwnerSurfaceHandoff、SurfaceOwnershipTransition、CompositionOwner、ConsumerAdapter 与 InputChoiceReducer rule 增加 payload focused group；
- RequiredGroups：36；
- mapping JSON：221 rules / parse PASS；
- mapping self-test：393/393；
- changed-file gate：PASS，Changed=15 / Rules=6 / Required=36 / Logs=3；
- focused payload、legacy ItemUseAndArmor 与完整 Shanmen.0_0_10 均提供原生 terminal evidence。

## 验证证据

- focused payload：6/0 / native 0 / 266,917 bytes / SHA-256 `26312494154031548CA7A428867F38B10FF396EC6A9EF69FD82B8968906D9123`；
- legacy ItemUseAndArmor：46/0 / native 0 / 303,003 bytes / SHA-256 `A5A0270D8449F2E8763162AD8DE0757A512A1479EE04DC1A7647757D92BA8977`；
- full Shanmen.0_0_10：1131/0 / native 0 / 1,648,044 bytes / SHA-256 `D118F0B5FF66A954F899099FCF625FB4ABD8CB474FCA64439DBD085C38EB4B7E`；
- formal pass logs total：1183/0；
- focused/legacy/full non-penalty notices：6 / 0 / 112；
- Fatal/Unhandled/Ensure：0 / 0 / 0；
- first-failure focused log：4/2 / terminal -1（process 255）/ 267,579 bytes / SHA-256 `8B7BABA55B1653CAABC802DC5BBC8BB49D202158042CA12E714739A9D18E3BFD`；
- boundary scan：2 payload files / 1097 physical lines / forbidden Engine-World、surface mutation、file IO、scheduler/retry 均为 0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

## 构建与产物

- final Editor：0 actions / native 0 / 2.81 s；
- final Game：0 actions / native 0 / 1.19 s；
- Editor DLL：17,634,816 bytes / SHA-256 `72330953C1F828C2A7C1BFFD911165E6E3D37521510D34CC97D63712631D6FB2`；
- Game EXE：358,653,440 bytes / SHA-256 `9858D9E88C898F47DB3A19EBD4DE5D990E08D8B674AEB2CD9BDDCC85C7DEEF8C`。

## P/F 边界

PASS：pending journal-bound payload、deterministic envelope identity、canonical schema 1、完整 semantic reconstruction、GameplayTag preservation、decode/re-encode、逐字节 corruption、全部 strict-prefix truncation、trailing/magic/schema/size rejection、foreign/committed journal fence、P20.48 live recovery gate、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未声明：N-1 payload migration、disk/atomic/fsync、process/crash restart、自动 recovery、全历史 archive、跨线程同步、renderer ownership、真实 renderer/MainHUD/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.51 建议建立 caller-owned atomic file storage adapter，只做 bounded temp-write/flush/replace/read 与错误分类；读取结果仍必须经过 P20.50 journal gate 和 P20.48 live authority gate，storage 层不得自动执行 recovery。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-50-thrown-weapon-arc-preview-recovery-checkpoint-payload-envelope>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-50-thrown-weapon-arc-preview-recovery-checkpoint-payload-envelope/Docs/Report/Dev.D.UE.0.0.10.P20.50.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-50-thrown-weapon-arc-preview-recovery-checkpoint-payload-envelope/Docs/Log/Dev.D.UE.0.0.10.P20.50.r0_log.md>
