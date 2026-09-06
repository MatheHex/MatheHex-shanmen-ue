# Dev.D.UE.0.0.10.P20.46.r0 Log

## 阶段

- 任务：P20.46 thrown-weapon Arc preview surface ownership transition；
- 基线：`713769270165cdbd97cff595708de768443b2947`；
- 分支：`agent/0.0.10-p20-46-thrown-weapon-arc-preview-surface-ownership-transition`；
- 目标：在同一 candidate 上组合 P20.44 policy 与 P20.45 lifecycle evidence，签发绑定到 concrete surface instance 的 ownership transition ticket。

## 实现记录

1. 新增 identity-bearing ownership candidate capability。
2. candidate 继承 P20.45 narrow lifecycle capability。
3. candidate 只新增 immutable `SurfaceInstanceId` query。
4. 重建后的 concrete surface 必须使用新 instance identity。
5. transaction 不保存 candidate pointer。
6. transaction 不获得 renderer surface 所有权。
7. 新增 immutable ownership transition request。
8. request 保存 deterministic RequestId。
9. request 绑定 exact Run identity。
10. request 绑定 expected consumer definition。
11. request 绑定 Host-authoritative cursor。
12. request 绑定 exact surface instance identity。
13. request 只允许 BindFresh、AdoptExact 或 ClearToEmpty。
14. Inspect 与 Invalid action 不能创建 request。
15. foreign-Run authoritative cursor 不能创建 request。
16. request deterministic replay identity 稳定。
17. request 本身不等同于 policy permit。
18. transaction operation guard 位于任何 candidate callback 之前。
19. callback reentry 在 identity query 前拒绝。
20. invalid request 在任何 candidate callback 前拒绝。
21. transaction 首先读取 initial concrete surface identity。
22. initial identity mismatch 在 policy snapshot 前拒绝。
23. consumer 与 cursor 从同一个 candidate 读取。
24. P20.44 Policy 使用实际 observed consumer/cursor。
25. 只有 Policy Authorized 才进入 lifecycle executor。
26. transaction 不绕过 P20.44 action/disposition fence。
27. P20.45 exact permit preflight 保持有效。
28. lifecycle 完成后再次读取 surface identity。
29. final identity drift 返回 typed failure。
30. identity drift 永不签发 ownership ticket。
31. policy/lifecycle 间 cursor drift 永不签发 ticket。
32. 新增 immutable ownership transition ticket。
33. ticket 只允许 BindFresh 或 AdoptExact。
34. ticket 保存 deterministic TicketId。
35. ticket 绑定 RequestId。
36. ticket 绑定 P20.44 PolicyDecisionId。
37. ticket 绑定 P20.44 PermitId。
38. ticket 绑定 P20.45 LifecycleReceiptId。
39. ticket 绑定 Run 与 consumer。
40. ticket 绑定 concrete SurfaceInstanceId。
41. ticket 绑定 expected/observed physical cursor。
42. ticket identity 由全部 handoff evidence 派生。
43. ticket 支持 exact candidate snapshot recheck。
44. replacement surface identity 无法复用旧 ticket。
45. consumer substitution 无法复用旧 ticket。
46. cursor substitution 无法复用旧 ticket。
47. BindFresh 只在 FreshEmpty policy + BindingReady receipt 后签票。
48. AdoptExact 只在 ExactVisible policy + BindingReady receipt 后签票。
49. fresh/adopt transaction 不调用 renderer mutation。
50. ClearToEmpty 仍只由 P20.45 Executor dispatch。
51. P20.46 没有第二条 ClearToEmpty 调用路径。
52. CleanupApplied 不生成 binding ticket。
53. CleanupApplied 强制后续重新创建 request 并评估。
54. old cleanup request 在 Empty surface 上被 policy 拒绝。
55. CleanupRejected 不生成 binding ticket。
56. unchanged rejected cleanup 保留 exact external retry signal。
57. transaction 内部没有 retry loop。
58. lifecycle invalid/snapshot mismatch fail closed。
59. EmptyNeedsRehydrate 仍由 P20.44 拒绝。
60. P20.46 不新增 rehydrate capability。
61. transaction 不调用 Composition Owner。
62. transaction 不调用 Consumer Adapter bind/end。
63. transaction 不旋转 Owner 的非拥有 pointer。
64. transaction 不尝试读取可能已销毁的旧 surface。
65. ticket 是后续 handoff 授权，不是 pointer commit receipt。
66. 实际 Owner commit 留给显式 one-shot consumption 阶段。
67. 未修改 P20.45/P20.44/P20.43/P20.42/P20.41 public contract。
68. 没有新增 World、Actor、UObject、RNG 或 ApplyDamage 依赖。
69. 没有新增 direct Show、Replace、Hide 或 ClearToEmpty dispatch。

## 自动化新增

- `EvidenceContract`；
- `BindFresh`；
- `AdoptExact`；
- `CleanupOutcomes`；
- `IdentityAndPolicyFences`；
- `IdentityDriftAndReentrant`。

focused OwnershipTransition：6/0；完整 0.0.10：1107/0，比 P20.45 增加 6 项。

## Changed-file regression

- 新增 SurfaceOwnershipTransition exact-path mapping rule；
- RequiredGroups：31；
- mapping self-test：385/385；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=31 / Logs=3；
- focused OwnershipTransition、legacy ItemUseAndArmor 与完整 Shanmen.0_0_10 均有 native terminal evidence。

## 验证证据

- formal focused：6/0，SHA-256 `A4B90D3852DB419415343EB7F93CE38D341D69EAA4A2DA6F63F142AA6F167E3E`；
- legacy ItemUseAndArmor：46/0，SHA-256 `62A0EAE6AA4C027C5D659E316A7295340A4B2D6F1EA53B847A73CBDD9FD480EB`；
- full Shanmen.0_0_10：1107/0，SHA-256 `B187F109FED56EE9F0C61CC492F4636E047E15BD14769233FEE47D267622CBB2`；
- formal logs total：1159/0，3 native terminal，0 fail；
- boundary scan：2 OwnershipTransition files / 938 lines / forbidden architecture 与 direct renderer mutation 0；
- focused test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

完整套件产生 91 条既有 SwordRhythm journal/checkpoint/envelope/manifest codec 非惩罚性 unresponsive 通知；进程保持推进，最终由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束。

## 构建与产物

- initial Editor：5 actions / native 0 / 15.46s；
- final Editor：0 actions / native 0 / 1.73s；
- final Game：4 actions / native 0 / 35.09s；
- Editor DLL：17328128 bytes / SHA-256 `B52EA73E4BC85658F0887535AACB34B51EF38A552DE0F0791D2C6623A3E996B6`；
- Game EXE：358420992 bytes / SHA-256 `FB86576F4CFA630ABD5D5B3E32F09C6FFB0949780174051A9A1BDBF19EAE7B0E`。

## P/F 边界

PASS：identity-bearing candidate、immutable request、same-candidate policy/lifecycle composition、deterministic ownership ticket、surface identity/consumer/cursor substitution fences、zero-mutation fresh/adopt、bounded cleanup、applied re-evaluation、rejected retry、rehydrate fence、callback reentry fence 与 changed-file regression。

未声明：实际 Owner ticket consumption、Owner/Adapter pointer replacement、旧 surface retirement、dangling pointer 消除、one-shot ticket ledger、真实 renderer/MainHUD/UI/visible frame、rehydrate、跨线程、崩溃恢复、补偿、真实输入、World、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.47：建立 Owner surface handoff commit。只有在旧 surface 可安全读取或已有显式 retirement attestation 时才消费 P20.46 ticket；原子更新 Owner/Adapter binding，记录 one-shot/idempotent ticket consumption，并阻止旧 pointer、ticket replay 与双 surface 可见性。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-46-thrown-weapon-arc-preview-surface-ownership-transition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-46-thrown-weapon-arc-preview-surface-ownership-transition/Docs/Report/Dev.D.UE.0.0.10.P20.46.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-46-thrown-weapon-arc-preview-surface-ownership-transition/Docs/Log/Dev.D.UE.0.0.10.P20.46.r0_log.md>
