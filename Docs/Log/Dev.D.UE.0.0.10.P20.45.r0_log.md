# Dev.D.UE.0.0.10.P20.45.r0 Log

## 阶段

- 任务：P20.45 thrown-weapon Arc preview surface lifecycle executor；
- 基线：`854a18336c499dc7118215045532d6bd85cdc379`；
- 分支：`agent/0.0.10-p20-45-thrown-weapon-arc-preview-surface-lifecycle-executor`；
- 目标：消费 P20.44 exact permit，以零或一次 lifecycle mutation 生成确定性执行回执。

## 实现记录

1. 新增 narrow surface lifecycle capability interface。
2. capability 暴露 consumer identity query。
3. capability 暴露 physical cursor query。
4. capability 只暴露一个 `ClearToEmpty` mutation。
5. capability 不暴露 Show、Replace、Hide 或 rehydrate。
6. executor 不保存 surface pointer。
7. executor 不保存 Composition Owner pointer。
8. executor 不保存 Host、Adapter 或 ledger pointer。
9. 新增 lifecycle response outcome enum。
10. response outcome 只允许 Applied 或 Rejected。
11. response 保存 exact PermitId。
12. response 保存 non-empty outcome code。
13. response 保存 previous physical cursor。
14. response 保存 reported current physical cursor。
15. response identity 由 permit、outcome、code 与双 cursor 派生。
16. response deterministic replay identity 稳定。
17. response 只接受 cleanup permit。
18. Applied response 只接受 visible-to-empty transition。
19. Rejected response 只接受 unchanged visible transition。
20. foreign/invalid permit response 无法匹配。
21. 新增 lifecycle receipt outcome enum。
22. receipt 区分 BindingReady。
23. receipt 区分 CleanupApplied。
24. receipt 区分 CleanupRejected。
25. receipt 保存 exact PermitId。
26. receipt 保存 surface mutation call count。
27. receipt 保存可选 renderer response。
28. receipt 保存 observed before/after cursor。
29. receipt identity 绑定全部执行 evidence。
30. BindingReady 强制零 cleanup call。
31. BindingReady 强制无 cleanup response。
32. CleanupApplied 强制一次 call 与 Applied response。
33. CleanupRejected 强制一次 call 与 Rejected response。
34. receipt 重新验证 P20.44 permit snapshot。
35. executor 新增 OperationInProgress status。
36. executor 新增 PermitInvalid status。
37. executor 新增 ConsumerMismatch status。
38. executor 新增 SnapshotMismatch status。
39. executor 新增 SurfaceResponseInvalid status。
40. executor 新增 SurfaceInvariantViolation status。
41. executor 新增 SurfaceRejected status。
42. executor 新增 BindingReady status。
43. executor 新增 Cleared status。
44. operation guard 位于任何 surface callback 之前。
45. guard 包围 preflight query、cleanup callback 与 postflight query。
46. callback reentry 在读取 surface 之前拒绝。
47. invalid permit 在 cleanup 前拒绝。
48. consumer mismatch 在 cleanup 前拒绝。
49. cursor snapshot drift 在 cleanup 前拒绝。
50. BindFresh permit 只生成 readiness receipt。
51. AdoptExact permit 只生成 readiness receipt。
52. binding path 不调用 `ClearToEmpty`。
53. cleanup path 每次 Execute 最多调用一次 surface。
54. executor callback 返回后重新读取 physical cursor。
55. response before/after 必须匹配实际观察。
56. invalid/foreign response 不生成 receipt。
57. Applied-without-clear 不生成 receipt。
58. mutated-then-Rejected 不生成 receipt。
59. accepted cleanup 生成 CleanupApplied receipt。
60. rejected unchanged cleanup 生成 CleanupRejected receipt。
61. rejected cleanup 不由 executor 内部自动重试。
62. caller 可在以后显式重试 unchanged exact permit。
63. applied cleanup 标记必须重新经过 policy evaluation。
64. applied 后旧 permit 因 cursor drift 无法重放。
65. `EmptyNeedsRehydrate` fence 未被绕过。
66. 未修改 P20.44 Policy public contract。
67. 未修改 P20.43 Composition Owner public contract。
68. 未进行 Owner/Adapter pointer bind 或 replacement。
69. 没有新增 World、Actor、UObject、RNG 或 ApplyDamage 依赖。

## 自动化新增

- `EvidenceContract`；
- `BindingReady`；
- `CleanupApplied`；
- `CleanupRejectedRetry`；
- `InvariantFailures`；
- `PreflightAndReentrant`。

focused LifecycleExecutor：6/0；完整 0.0.10：1101/0，比 P20.44 增加 6 项。

## Changed-file regression

- 新增 SurfaceLifecycleExecutor exact-path mapping rule；
- RequiredGroups：30；
- mapping self-test：383/383；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=30 / Logs=3；
- focused LifecycleExecutor、legacy ItemUseAndArmor 与完整 Shanmen.0_0_10 均有 native terminal evidence。

## 验证证据

- formal focused：6/0，SHA-256 `5EC7C1294C739C4C605ABEB55F9B6B18C50E054AFB4C3F496D0D1E4990D50B49`；
- legacy ItemUseAndArmor：46/0，SHA-256 `0ABF04B3B511A51BD9A6A204CA02D4517704F038512F339695C5834B6EE59379`；
- full Shanmen.0_0_10：1101/0，SHA-256 `195F8320286F55DCB0A7C569025A68454A2326D7D12684C4B5D201FB7F7543CB`；
- formal logs total：1153/0，3 native terminal，0 fail；
- boundary scan：2 LifecycleExecutor files / 958 lines / forbidden architecture 与 direct Show/Replace/Hide 0；
- production `Surface.ClearToEmpty` dispatch：1；
- test declarations：6；
- `git diff --check`：PASS，0 whitespace errors。

完整套件产生 90 条既有 SwordRhythm journal/checkpoint/manifest codec 非惩罚性 unresponsive 通知；进程保持推进，最终由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束。

## 构建与产物

- initial candidate Editor：5 actions / native 0 / 23.67s；
- guard-corrected Editor：5 actions / native 0 / 13.01s；
- final Editor：0 actions / native 0 / 1.73s；
- final Game：4 actions / native 0 / 23.29s；
- Editor DLL：17273344 bytes / SHA-256 `7B3F9A90ED6895BF1104C746EB4B7439CEC3EDF3B372B66FF88B4F4F2EE03903`；
- Game EXE：358380032 bytes / SHA-256 `E300C3C0FB02F956A6D36F4AE386E67A629BA713BD886E73BFB08A862939FC3B`。

## P/F 边界

PASS：exact permit consumption、consumer/snapshot preflight、deterministic response/receipt、zero-call binding readiness、single-call cleanup、applied/rejected invariants、invalid attestation fence、caller retry、post-cleanup reevaluation、callback reentry fence 与 changed-file regression。

未声明：真实 renderer/MainHUD/UI/visible frame、Composition Owner pointer transition、rehydrate、旧 surface retirement、补偿、跨线程或崩溃恢复、真实输入、World、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.46：建立 surface ownership transition transaction，组合 P20.44 policy 与 P20.45 receipt，有界提交 Owner bind/adopt 或 cleanup 后 replacement；继续保持 renderer mutation 与 pure policy 分离。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-45-thrown-weapon-arc-preview-surface-lifecycle-executor>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-45-thrown-weapon-arc-preview-surface-lifecycle-executor/Docs/Report/Dev.D.UE.0.0.10.P20.45.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-45-thrown-weapon-arc-preview-surface-lifecycle-executor/Docs/Log/Dev.D.UE.0.0.10.P20.45.r0_log.md>
