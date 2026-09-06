# Dev.D.UE.0.0.10.P20.47.r0 Log

## 阶段

- 任务：P20.47 thrown-weapon Arc preview Owner surface handoff commit；
- 基线：`00fc0fac8a43b068408ae57cb92592c7fc2e8569`；
- 分支：`agent/0.0.10-p20-47-thrown-weapon-arc-preview-owner-surface-handoff`；
- 目标：安全消费 P20.46 binding ticket，显式 retirement 旧 surface，并原子更新 Composition Owner 与 Consumer Adapter 的非拥有 pointer。

## 实现记录

1. 新增 narrow handoff-surface capability。
2. capability 组合 renderer surface、ownership candidate 与 retirement。
3. handoff 调用方必须提供 live old/new surface reference。
4. transaction 不从 Owner 裸指针恢复 C++ 对象。
5. old base pointer 地址不匹配时零 surface callback 拒绝。
6. old 与 new 必须是不同对象。
7. old 与 new SurfaceInstanceId 必须不同。
8. P20.46 ticket 必须是 BindFresh 或 AdoptExact。
9. invalid ticket 在所有 surface callback 前拒绝。
10. inactive Owner 在所有 surface callback 前拒绝。
11. Host/Adapter joint structure 在 callback 前检查。
12. Host/Adapter operation-in-progress 在 callback 前检查。
13. operation guard 覆盖全部 identity/consumer/cursor query。
14. operation guard 覆盖 retirement callback。
15. callback reentry 返回 OperationInProgress。
16. reentry 不读取 surface。
17. reentry 不调用 retirement。
18. 新增 immutable retirement response。
19. response 保存 deterministic ResponseId。
20. response 绑定 exact transition TicketId。
21. response 绑定 retired surface identity。
22. response 绑定 replacement surface identity。
23. response 绑定 outcome 与 outcome code。
24. response 绑定 retirement 前后 cursor。
25. Applied 只允许 visible-to-empty。
26. Rejected 只允许 cursor unchanged。
27. response 重新匹配 ticket replacement identity。
28. response 重新匹配 ticket observed cursor。
29. transaction callback 后复查 old identity。
30. transaction callback 后复查 new identity。
31. transaction callback 后复查 new consumer。
32. transaction callback 后复查 new cursor。
33. response cursor 与实际 old cursor 必须一致。
34. BindFresh 要求 Host physical cursor Empty。
35. BindFresh 要求 old surface Empty。
36. BindFresh 要求 new ticket snapshot Empty。
37. BindFresh retirement call count 固定为 0。
38. BindFresh 不调用 Show/Replace/Hide/ClearToEmpty。
39. BindFresh 在临时 Owner copy 内准备联合提交。
40. AdoptExact 要求 Host physical cursor visible exact。
41. AdoptExact 要求 old surface visible exact。
42. AdoptExact 要求 new ticket snapshot visible exact。
43. AdoptExact retirement call count 最多 1。
44. retirement Applied 后 old physical cursor 必须 Empty。
45. retirement Applied 后 new surface 必须保持 exact visible。
46. retirement 合法 Rejected 后 Owner 保持旧 binding。
47. 合法 Rejected 返回 exact external retry signal。
48. transaction 内不包含 retry loop。
49. invalid retirement response 不提交 pointer。
50. retirement evidence drift 不提交 pointer。
51. replacement drift 不提交 pointer。
52. post-mutation failure 暴露 Owner validity。
53. Owner 已失配时暴露 NeedsManualRecovery。
54. P20.47 不伪造补偿 response。
55. P20.47 不自动 rehydrate 新旧 surface。
56. 新增 immutable handoff receipt。
57. receipt 保存 deterministic ReceiptId。
58. receipt 绑定 TicketId、Run 与 consumer。
59. receipt 绑定 old/new surface identity。
60. receipt 绑定 action 与 outcome。
61. receipt 绑定 retirement call count 与 response。
62. receipt 绑定 old-before、old-after 与 new cursor。
63. FreshBound receipt 要求零 retirement 与三个 Empty cursor。
64. ExactAdopted receipt 要求一次 Applied retirement。
65. Owner 与 Adapter pointer 在一次 Owner assignment 中联合提交。
66. Host authority state 不在 handoff 中修改。
67. Host delivery cursor 不在 handoff 中修改。
68. Adapter LastResult 在 replacement binding 上清空。
69. Owner 保存 concrete bound SurfaceInstanceId。
70. Owner 保存最近一次 handoff receipt。
71. identity-bearing Owner validity 复查 pointer/identity/receipt。
72. legacy TryBegin 仍可绑定普通 non-identity surface。
73. TryEnd 清空 identity surface、bound identity 与 receipt。
74. exact latest ticket replay 不调用 retirement。
75. replay 返回原 receipt identity。
76. replay 复查当前 pointer、identity、consumer 与 cursor。
77. consumed-ticket binding drift 返回 TicketReplayConflict。
78. 不宣称全历史或持久化 ticket ledger。
79. 不改变 surface 外部 lifetime ownership 契约。
80. 没有新增 World、Actor、UObject、RNG 或 ApplyDamage 依赖。
81. handoff 不直接 dispatch Show、Replace、Hide 或 ClearToEmpty。
82. 唯一 mutation capability 是旧 surface 的 RetireForHandoff。

## 自动化新增

- `EvidenceContract`；
- `BindFreshCommit`；
- `AdoptExactCommit`；
- `RetirementRejectedRetry`；
- `IdentityAndSnapshotFences`；
- `InvariantAndReentrant`。

首次 focused 为 5/1：测试在检查错误旧 pointer 的零读取计数前执行了下一次 new-surface drift 尝试，导致测试计数被自身污染。生产结果状态正确。首次日志保留为 `P20.47_Focused_FirstFail.log`；修正断言取样时点后 focused 为 6/0。

完整 `Shanmen.0_0_10` 为 1113/0，比 P20.46 增加 6 项。

## Changed-file regression

- 新增 OwnerSurfaceHandoff exact-path mapping rule；
- RequiredGroups：32；
- mapping self-test：387/387；
- changed-file gate：PASS，Changed=8 / Rules=4 / Required=32 / Logs=3；
- focused OwnerSurfaceHandoff、legacy ItemUseAndArmor 与完整 Shanmen.0_0_10 均有 native terminal evidence。

## 验证证据

- first focused failure：5/1，SHA-256 `8A215F3779599736ED93CC3D5803A374F22DDB6EA98CCB506B3F8CA1768B4E76`；
- formal focused：6/0，SHA-256 `17C821048BA40A4F102AC282B44E5B7F5DABB455E79ED9BA226C19B2408CE218`；
- legacy ItemUseAndArmor：46/0，SHA-256 `7BE432D4040B6EABF4BF28ECEA80358E9E63CE2DA8B3F8E5B39315E5A6B958EA`；
- full Shanmen.0_0_10：1113/0，SHA-256 `979CBCBB1D2CB6BFBD1027171A8E56B2336367146899B35841427D2240233303`；
- formal pass logs total：1165/0，3 native terminal，0 fail；
- full log：94 条既有非惩罚性 unresponsive，Fatal/Unhandled/Ensure=0；
- boundary scan：2 handoff files / 1167 lines / forbidden architecture 与 direct renderer command dispatch 0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

## 构建与产物

- initial Editor integration：10 actions / native 0 / 32.84s；
- test integration Editor：5 actions / native 0 / 10.15s；
- focused-fix Editor：4 actions / native 0 / 13.91s；
- final Editor：0 actions / native 0 / 1.44s；
- final Game：9 actions / native 0 / 23.77s；
- Editor DLL：17399808 bytes / SHA-256 `B925B375554B4756F974C7BB3E70EA46C10DCDCDAD595396C17AA26BD2A5848C`；
- Game EXE：358477312 bytes / SHA-256 `2C0119E2AED1898459608A5B001D2689D5FCDC046B5EDF891B03DBDF43D3E135`。

## P/F 边界

PASS：live-old address fence、identity-bearing replacement、ticket consumption、fresh zero-mutation commit、exact retirement、deterministic response/receipt、Owner+Adapter atomic pointer replacement、latest-ticket replay、rejected retry、manual-recovery signal、snapshot/identity/consumer fences、callback reentry fence、legacy teardown 与 changed-file regression。

未声明：全历史/持久化 ticket ledger、crash-window recovery、跨线程 lifetime、拥有型 renderer handle、自动补偿/rehydrate、真实 renderer/MainHUD/UI/visible frame、真实输入、World、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.48：建立 handoff recovery journal/checkpoint，为“旧 surface 已退休、pointer commit 未完成”的窗口提供持久化、可审计、显式触发的恢复证据；invalid retirement response 不得直接升级为可信 commit。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-47-thrown-weapon-arc-preview-owner-surface-handoff>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-47-thrown-weapon-arc-preview-owner-surface-handoff/Docs/Report/Dev.D.UE.0.0.10.P20.47.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-47-thrown-weapon-arc-preview-owner-surface-handoff/Docs/Log/Dev.D.UE.0.0.10.P20.47.r0_log.md>
