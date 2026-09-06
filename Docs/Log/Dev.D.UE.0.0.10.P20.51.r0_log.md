# Dev.D.UE.0.0.10.P20.51.r0 Log

## 阶段

- 任务：P20.51 thrown-weapon Arc preview recovery checkpoint payload storage；
- 基线：`979464df35192d302501193a281e9aefe3effe61`；
- 分支：`agent/0.0.10-p20-51-thrown-weapon-arc-preview-recovery-payload-storage`；
- 目标：为 P20.50 envelope 提供 caller-owned bounded atomic file protocol，同时禁止 storage 自行获得 recovery authority。

## 实现记录

1. 新增独立 payload storage context。
2. context 只接受 caller 提供的 absolute root。
3. root 空、relative、embedded NUL 或超过 1024 characters 均拒绝。
4. context 必须绑定 valid expected JournalId。
5. primary filename 由 JournalId canonical digits 派生。
6. temp 固定为同一 primary 路径追加 `.tmp`。
7. primary/temp 在结构上位于同一目录。
8. storage 不选择全局 product root。
9. 新增 caller-owned filesystem seam。
10. seam 只提供目录、temp 删除、write/flush、bounded read 与 atomic replace。
11. 新增真实 local filesystem backend。
12. local write 使用 platform file handle。
13. write 后执行 `Flush(true)`。
14. bounded read 在分配前读取 handle size。
15. 大于 65536 bytes 时直接返回 TooLarge。
16. 读取期间 size 漂移会 fail closed。
17. local replace 使用同目录 single Move/replace。
18. Save 先验证 context。
19. Save 再验证 envelope。
20. expected JournalId mismatch 在任何 filesystem callback 前拒绝。
21. envelope 通过 P20.50 canonical codec 编码。
22. storage directory 只创建一次。
23. stale temp 只尝试删除一次。
24. temp open/write/flush 分别分类。
25. temp read-back failure 单独分类。
26. temp size、exact bytes 与 codec envelope 三重校验。
27. temp 校验前不调用 replace。
28. `AtomicReplace` 每次 Save 最多调用一次。
29. replace false 按 filesystem seam 契约保持 destination。
30. replace 成功后标记 primary 已改变。
31. committed primary 再做 bounded read-back。
32. committed primary 再做 exact bytes 与 codec envelope 校验。
33. committed read failure 显式要求 caller Load。
34. committed validation failure 显式要求 caller Load。
35. postcommit failure 不回滚、不重试。
36. precommit failure 不删除 primary。
37. precommit failure 报告 temp 可能残留。
38. stale temp 留给下一次显式 Save 清理。
39. Load 区分 missing/read/size/decode/journal mismatch。
40. Load 只返回 P20.50 envelope evidence。
41. Load 不接受 journal 参数。
42. Load 不接受 recovery coordinator 参数。
43. Load 不接受 CompositionOwner、Host、Adapter 或 surface。
44. Load 不调用 `TryUnwrapForJournal`。
45. storage 不 append checkpoint 或 recovery receipt。
46. storage 不调用 surface Show/Replace/Hide/Clear/Retire。
47. storage 不调度、不循环、不自动恢复。
48. committed journal 仍拒绝历史磁盘 envelope。
49. local integration root 由测试独占并在断言后清理。
50. 未修改 P20.50 schema 或 P20.49 journal schema。

## 自动化新增

- `ContextAndJournalFence`；
- `AtomicRoundTripAndEvidenceOnly`；
- `PrecommitFailureAtomicity`；
- `PostcommitOutcomeRequiresLoad`；
- `LoadBoundsDecodeAndJournalFence`；
- `LocalFileAtomicReplaceAndCommittedFence`。

开发 focused 首轮 6/0；加强 cleanup 前 temporary-file absence 断言后，正式 focused 仍为 6/0。

## Changed-file regression

- 新增 PayloadStorage exact-path mapping rule；
- PayloadEnvelope 与 RecoveryJournal rule 增加 storage focused group；
- RequiredGroups：36；
- mapping JSON：222 rules / parse PASS；
- mapping self-test：395/395；
- changed-file gate：PASS，Changed=7 / Rules=2 / Required=36 / Logs=3。

## 验证证据

- focused payload storage：6/0，native 0，265,901 bytes，SHA-256 `20B0ACC9B95E8680DA88C77280C27E13A9C3C5A9E793366C7532221D02E10BC9`；
- legacy ItemUseAndArmor：46/0，native 0，303,003 bytes，SHA-256 `A676EDD9404A103AFB5C4230403336936717C77B55C06FC5A177094CC1D20F20`；
- full Shanmen.0_0_10：1137/0，native 0，1,660,714 bytes，SHA-256 `7EF1B89EAC227CB13D6D0DECB141F0FDAE833E81AF155A14F0D8558233ECE0B1`；
- formal pass logs total：1189/0；
- focused/legacy/full non-penalty notices：6 / 0 / 119；
- Fatal/Unhandled/Ensure：0 / 0 / 0；
- boundary scan：2 storage files / 703 physical lines / World、authority execution、surface mutation、retry 与 primary delete 均为 0；
- production `AtomicReplace` call site：1；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

## 构建与产物

- final Editor：0 actions / native 0 / 1.79 s；
- final Game：0 actions / native 0 / 1.15 s；
- Editor DLL：17,700,352 bytes / SHA-256 `688CE434ADF2CB5C0181B45B0F11F1C2B8748F60300427708F39EB802FE1B717`；
- Game EXE：358,699,008 bytes / SHA-256 `10FFEC03408BC39010AD10E95EB3FF8E3376331242CBFDB590D036116662DB8E`。

## P/F 边界

PASS：caller-owned absolute root、JournalId slot、bounded temp/primary reads、full flush、exact codec verification、single replace、precommit preservation、postcommit load-required classification、真实 local round-trip/replacement、journal committed fence、changed-file regression、legacy ItemUseAndArmor 与完整 0.0.10 suite。

未声明：power-loss/directory-fsync、OS-crash rename、network filesystem、concurrent writer locks、backup/rollback、journal+payload transaction、auto recovery、真实 renderer/MainHUD/UI/input/World、Editor UI、PIE、Standalone、产品启动、截图、Smoke、Cook 或 Package。

## 下一阶段

P20.52 建议建立 canonical recovery bundle，将 P20.49 journal bytes 与 P20.50 payload bytes 以 generation/digest 配对，避免 restart 时只有 payload 没有 journal；bundle 加载后仍不得自动执行 recovery。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-51-thrown-weapon-arc-preview-recovery-payload-storage>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-51-thrown-weapon-arc-preview-recovery-payload-storage/Docs/Report/Dev.D.UE.0.0.10.P20.51.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-51-thrown-weapon-arc-preview-recovery-payload-storage/Docs/Log/Dev.D.UE.0.0.10.P20.51.r0_log.md>
