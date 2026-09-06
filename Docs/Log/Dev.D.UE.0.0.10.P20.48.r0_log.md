# Dev.D.UE.0.0.10.P20.48.r0 Log

## 阶段

- 任务：P20.48 thrown-weapon Arc preview Owner surface handoff half-commit recovery；
- 基线：`4afa53912bf770581f22a4e26366a05f843ec091`；
- 分支：`agent/0.0.10-p20-48-thrown-weapon-arc-preview-owner-surface-handoff-recovery`；
- 目标：在 old surface 已退休、Owner/Adapter pointer 未提交的精确窗口中，以 fresh observation 完成零 mutation 原子恢复。

## 实现记录

1. 新增 immutable recovery checkpoint。
2. checkpoint 只从 valid P20.47 manual-recovery result 创建。
3. source failure 仅允许 post-retirement recoverable status。
4. checkpoint action 固定为 AdoptExact。
5. retirement call count 固定为 1。
6. source normal receipt 必须为空。
7. source Owner 必须处于 invalid half-commit state。
8. old-before 必须是 ticket exact visible snapshot。
9. old-after 必须 Empty。
10. new-after 必须仍匹配 ticket exact candidate snapshot。
11. checkpoint 记录 source response identity 作为 provenance。
12. invalid retirement response 不提供恢复授权。
13. CheckpointId 由完整冻结事实确定性派生。
14. checkpoint 支持自验证与 failed-result exact match。
15. 新增显式 recovery coordinator。
16. 调用方必须提供 exact live old/new surface reference。
17. Owner old pointer 不匹配时零 surface query 拒绝。
18. old/new 同对象时零 surface query 拒绝。
19. operation guard 在首个 surface query 前安装。
20. callback reentry 在 query 前返回 OperationInProgress。
21. old identity 必须等于 checkpoint old identity。
22. old consumer 必须等于 ticket consumer。
23. old cursor 必须 Empty。
24. new identity/consumer/cursor 必须匹配 ticket snapshot。
25. Host 与 Adapter 必须 active 且结构一致。
26. Host/Adapter operation 均不得进行中。
27. Host/Adapter Run 与 consumer 必须匹配 ticket。
28. Host 不得处于 recovery pending。
29. Host logical cursor 的 physical projection 必须匹配 ticket expected cursor。
30. Owner 必须仍是 exact pre-pointer-commit invalid state。
31. legacy binding metadata 只允许全空形态。
32. identity binding metadata 必须精确绑定 old surface。
33. identity binding metadata 只允许 normal/recovery receipt 二选一。
34. 任意 old/new/authority drift 返回 typed rejection。
35. recovery coordinator 不调用 Show。
36. recovery coordinator 不调用 Replace。
37. recovery coordinator 不调用 Hide。
38. recovery coordinator 不调用 ClearToEmpty。
39. recovery coordinator 不调用 RetireForHandoff。
40. 新增 immutable deterministic recovery receipt。
41. receipt 绑定 checkpoint、ticket、Run 与 consumer。
42. receipt 绑定 old/new identity 与 old/new cursor。
43. Owner candidate 同时替换 Owner 与 Adapter pointer。
44. candidate 清空 Adapter LastResult。
45. candidate 写入 new identity binding。
46. candidate 清空 normal handoff receipt。
47. candidate 写入 recovery receipt。
48. candidate joint validation 后一次 assignment 提交。
49. 恢复结果固定报告 DidMutateSurface=false。
50. 最新 checkpoint replay 只读取 current new surface。
51. replay 不读取 old surface。
52. replay 返回原 recovery receipt identity。
53. consumed checkpoint binding drift 返回 CheckpointReplayConflict。
54. Composition Owner identity invariant 要求两类 receipt 严格互斥。
55. TryEnd 清空 normal 与 recovery receipt。
56. 后续 normal P20.47 handoff 清空 recovery receipt。
57. P20.48 不包含 retry loop、自动补偿或 rehydrate。
58. P20.48 checkpoint/receipt 为 in-memory value，不声明 durable persistence。
59. 没有新增 World、Actor、UObject、RNG 或 ApplyDamage 依赖。
60. 没有连接真实 renderer、MainHUD、widget 或 component。

## 自动化新增

- `EvidenceContract`；
- `ZeroMutationCommit`；
- `IdempotentReplay`；
- `OldSurfaceFences`；
- `NewAndAuthorityFences`；
- `ReentrantAndReceiptRotation`。

完整 `Shanmen.0_0_10` 为 1119/0，比 P20.47 增加 6 项。

## Changed-file regression

- 新增 OwnerSurfaceHandoffRecovery exact-path mapping rule；
- P20.47 handoff、CompositionOwner 与 ConsumerAdapter rule 增加 recovery group；
- RequiredGroups：33；
- mapping self-test：389/389；
- changed-file gate：PASS，Changed=10 / Rules=5 / Required=33 / Logs=3；
- focused recovery、legacy ItemUseAndArmor 与完整 Shanmen.0_0_10 均有原生 terminal evidence。

## 验证证据

- focused recovery：6/0，SHA-256 `807CA14D77E87E4FE814FFDC5456BC786EBD177AC34CEAB0569F6C941E4F1027`；
- legacy ItemUseAndArmor：46/0，SHA-256 `D81B27CB1D5BB7DFC5C099C3B7F58F1918A7432EACD58CAF13C488D60DDA4EEC`；
- full Shanmen.0_0_10：1119/0，SHA-256 `F72FC65F4B2712A6647249B6F8D22A20B9450C785CC0EEB75E01BEC57EC3F8FB`；
- formal pass logs total：1171/0，3 native terminal，0 fail；
- full log：102 条既有非惩罚性大时间差通知，Fatal/Unhandled/Ensure=0；
- boundary scan：2 recovery files / 979 lines / forbidden architecture 与 direct surface mutation 0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

## 构建与产物

- initial Editor source integration：11 actions / native 0 / 25.56s；
- test integration Editor：4 actions / native 0 / 8.61s；
- final Editor：0 actions / native 0 / 1.23s；
- final Game：10 actions / native 0 / 23.64s；
- Editor DLL：17459712 bytes / SHA-256 `A876912045ED3BF6E61790058BB9B9958EFEE748A0976D4BEDDB0FAB602D82A3`；
- Game EXE：358523904 bytes / SHA-256 `D3A59F8DCCEEF20B05524BD889B060940E1A727B2997EB022C9843E14CE9EA99`。

## P/F 边界

PASS：trusted checkpoint admission、invalid response non-authority、exact old pointer/snapshot fence、new ticket snapshot fence、Host/Adapter authority fence、zero surface mutation、atomic pointer recovery、separate recovery receipt、receipt mutual exclusion、latest-checkpoint replay、reentry fence、normal handoff receipt rotation、legacy teardown 与 changed-file regression。

未声明：codec、磁盘 journal、process/crash restart、全历史 ledger、跨线程 lifetime、拥有型 renderer handle、自动补偿/rehydrate、真实 renderer/MainHUD/UI/visible frame、真实输入、World、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.49：为 recovery checkpoint/receipt 建立 canonical codec 与 bounded append-only journal；验证 round-trip、corruption/truncation、latest-record selection 和 N-1 migration 后，再允许讨论 process-restart recovery。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-48-thrown-weapon-arc-preview-owner-surface-handoff-recovery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-48-thrown-weapon-arc-preview-owner-surface-handoff-recovery/Docs/Report/Dev.D.UE.0.0.10.P20.48.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-48-thrown-weapon-arc-preview-owner-surface-handoff-recovery/Docs/Log/Dev.D.UE.0.0.10.P20.48.r0_log.md>
