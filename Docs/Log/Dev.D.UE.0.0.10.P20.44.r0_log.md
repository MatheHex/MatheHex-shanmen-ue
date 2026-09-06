# Dev.D.UE.0.0.10.P20.44.r0 Log

## 阶段

- 任务：P20.44 thrown-weapon Arc preview surface recreation/adoption policy；
- 基线：`fb66833964ea3549651dcebc89b484e1c18fd3b0`；
- 分支：`agent/0.0.10-p20-44-thrown-weapon-arc-preview-surface-recreation-policy`；
- 目标：对 renderer 重建后的 surface 做纯值分类，只为 exact fresh/adopt/cleanup 状态签发确定性 permit。

## 实现记录

1. 新增 surface recreation disposition enum。
2. disposition 包含 InputRejected 与 ConsumerMismatch。
3. disposition 包含 FreshEmpty 与 ExactVisible。
4. disposition 包含 EmptyNeedsRehydrate。
5. disposition 分开 ResidualVisible、ForeignVisible、ConflictingVisible。
6. 新增 Inspect、BindFresh、AdoptExact、ClearToEmpty action。
7. 新增 Rejected、Inspected、Authorized outcome。
8. Policy 输入绑定 expected Run 与 expected consumer。
9. Policy 输入保存 Host-authoritative cursor。
10. Policy 输入保存 observed consumer 与 physical cursor。
11. authoritative Empty 映射为 expected physical Empty。
12. authoritative Hidden 映射为 expected physical Empty。
13. authoritative Visible 保留 exact state identity。
14. observed physical cursor 只允许 Empty 或 valid Visible。
15. observed Hidden 明确拒绝。
16. wrong-Run authoritative cursor 明确拒绝。
17. consumer mismatch 可 Inspect，但不能获得 permit。
18. Empty/Empty 分类为 FreshEmpty。
19. Hidden/Empty 分类为 FreshEmpty。
20. exact same visible state 分类为 ExactVisible。
21. visible authority / empty observed 分类为 EmptyNeedsRehydrate。
22. empty authority / same-Run visible 分类为 ResidualVisible。
23. observed visible foreign Run 分类为 ForeignVisible。
24. same-Run different visible state 分类为 ConflictingVisible。
25. Inspect 从不生成 permit。
26. BindFresh 只允许 FreshEmpty。
27. AdoptExact 只允许 ExactVisible。
28. ClearToEmpty 只允许三类 visible conflict。
29. action/disposition 错配返回 typed rejection。
30. EmptyNeedsRehydrate 不允许 bind。
31. EmptyNeedsRehydrate 不允许 adopt。
32. EmptyNeedsRehydrate 不允许 clear。
33. 新增 immutable recreation permit。
34. PermitId 由 action、disposition、Run、consumer 与双 cursor 派生。
35. permit 区分 binding 与 cleanup capability。
36. permit 保存 normalized expected physical cursor。
37. permit 保存评估时 exact observed cursor。
38. `MatchesSnapshot` 重新核对 Run 与 consumer。
39. `MatchesSnapshot` 重新核对 expected 与 observed cursor。
40. surface drift 使旧 permit 不匹配。
41. 新增 immutable recreation result。
42. DecisionId 绑定 outcome/action/disposition 与全部输入。
43. exact request replay 产生相同 DecisionId/PermitId。
44. 不同 action 产生不同 DecisionId。
45. result 保存 diagnostic 与可选 permit。
46. result 构造时 self-validate 并缓存结果。
47. invalid input 仍形成可审计 rejection evidence。
48. Policy 不接收 surface object 或 callback。
49. Policy 不调用 Show、Replace、Hide 或其它 renderer mutation。
50. Policy 不修改 Owner、Host、Adapter 或 ledger。
51. permit 不是 mutation receipt，也不声明动作已经执行。
52. rehydrate executor、cleanup executor 与 pointer rebind 全部后置。
53. 没有新增 World、Actor、UObject、UI implementation、RNG 或 ApplyDamage 依赖。

## 自动化新增

- `ClassificationMatrix`；
- `BindingPermits`；
- `ExplicitCleanupPermits`；
- `RehydrateFence`；
- `InvalidInputs`；
- `ActionFence`。

focused Policy：6/0；完整 0.0.10：1095/0，比 P20.43 增加 6 项。

## Changed-file regression

- 新增 SurfaceRecreationPolicy exact-path mapping rule；
- RequiredGroups：29；
- mapping self-test：381/381；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=29 / Logs=3；
- Policy focused、legacy ItemUseAndArmor、完整 Shanmen.0_0_10 均有 native terminal evidence。

## 验证证据

- formal focused：6/0，SHA-256 `0064F12FB683147043691D730C458F0AA5B8336A7C72C056666B55272312018A`；
- legacy ItemUseAndArmor：46/0，SHA-256 `6F7B15123F4F13912BC362BF54735E93462C9D81941ABC92522CC41D347F1D4E`；
- full Shanmen.0_0_10：1095/0，SHA-256 `0422CDD841C5A4DE03A32FA1A8BFFB835EA4499B1C149309DB244916E5122AE8`；
- formal logs total：1147/0，3 native terminal，0 fail；
- boundary scan：2 Policy files / 658 lines / forbidden architecture and direct surface calls 0；
- `git diff --check`：PASS，0 whitespace errors。

完整套件在既有 SwordRhythm journal/checkpoint/manifest codec 慢区段产生 91 条非惩罚性 unresponsive 通知；进程持续使用 CPU、恢复逐项进展，最后由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束。

## 构建与产物

- candidate Editor：5 actions / native 0 / 25.71s；
- final Editor：up to date / 0 actions / native 0 / 1.19s；
- final Game：4 actions / native 0 / 33.38s；
- Editor DLL：17219072 bytes / SHA-256 `49DB418DB1C7E6E715B78CEF8E4DB97D83FCF72EB6FC5ACC8A1509D5399B9062`；
- Game EXE：358337024 bytes / SHA-256 `B88465A336E1184AB49DFD2E2E016EE392B157BC786C5D1B9F07DDD442A0DFE5`。

## P/F 边界

PASS：surface recreation classification、Hidden normalization、fresh/exact/cleanup authorization、rehydrate fence、consumer fence、deterministic decision/permit、snapshot drift fence 与 changed-file regression。

未声明：permit execution、surface mutation、rehydrate、Owner/Adapter pointer replacement、旧 surface retirement、真实 renderer/MainHUD/UI/visible frame、补偿、跨线程或崩溃持久性、真实输入、World、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.45：建立 narrow attested surface lifecycle executor，消费 exact permit，生成执行 receipt；ClearToEmpty 最多调用一次独立 lifecycle capability，仍不修改 Owner pointer。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-44-thrown-weapon-arc-preview-surface-recreation-policy>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-44-thrown-weapon-arc-preview-surface-recreation-policy/Docs/Report/Dev.D.UE.0.0.10.P20.44.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-44-thrown-weapon-arc-preview-surface-recreation-policy/Docs/Log/Dev.D.UE.0.0.10.P20.44.r0_log.md>
