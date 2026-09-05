# Dev.D.UE.0.0.10.P20.43.r0 Log

## 阶段

- 任务：P20.43 thrown-weapon Arc preview presentation composition owner；
- 基线：`53a7f2b21659fcfef485b775df06883509f64df8`；
- 分支：`agent/0.0.10-p20-43-thrown-weapon-arc-preview-composition-owner`；
- 目标：共同持有 P20.41 Host 与 P20.42 Adapter，冻结同一 Run/consumer/surface 的 begin、update、recovery、end 顺序。

## 实现记录

1. 新增 Run-scoped presentation composition Owner。
2. Owner 共同持有 delivery Host 与 consumer Adapter。
3. surface pointer 为 non-owning，调用方负责 active scope 生命周期。
4. `TryBegin` 使用 fresh candidate，避免半完成 child bind 泄漏。
5. begin 先绑定 Adapter，再绑定 Host。
6. 只有 child Run、consumer 与 cursor 一致时提交 candidate。
7. exact Run/surface begin replay 幂等成功。
8. active Run 或 surface rotation 在重绑前拒绝。
9. Visible physical surface 不能无证据绑定。
10. `TryEnd` 在 candidate copy 上执行。
11. end 先结束 Host，再结束 Adapter。
12. 只有两个 child 均 empty 时提交 teardown。
13. foreign Run、Visible surface、pending recovery 与 invalid owner 不能 end。
14. 新增 immutable Owner update result。
15. update result 保存 status、diagnostic、Run 与 consumer。
16. update result保存 Host/Adapter call count 与 nested evidence。
17. update result保存 desired state、Host cursor、surface cursor 的 before/after。
18. update result保存 pending command 的 before/after。
19. update result保存 owner-valid-after。
20. update inactive/invalid/reentrant 时 child call 为 0。
21. valid update 只调用一次 Host。
22. Host 只通过 owned Adapter delivery port 投递。
23. Adapter call count 由 Host port response 推导，只允许 0 或 1。
24. Owner 交叉验证 Host 与 Adapter 的 exact command evidence。
25. Applied update 必须同时推进 Host 与 physical surface cursor。
26. exact application replay 不重复 physical mutation。
27. Rejected update 保存 Host exact pending command。
28. pending recovery 期间 newer update 被 fence。
29. 新增 immutable Owner recovery result。
30. recovery result保存 Adapter/Host recovery call count。
31. recovery result保存 Adapter result、Applied receipt 与 Host recovery result。
32. recovery result保存 pending 与两个 cursor 的 before/after。
33. 无 pending 时返回 NoRecoveryPending，两个 child call 均为 0。
34. 有 pending 时只调用 Adapter 一次。
35. Adapter Rejected 时保留 pending，不调用 Host recovery。
36. Adapter Applied 时构造 exact Applied receipt。
37. Applied receipt 只向 Host recovery 转交一次。
38. Host recovery 成功后清除 pending并推进 ledger cursor。
39. recovery 没有内部循环或无限 retry。
40. Host Hidden audit cursor 在 physical surface 边界归一化为 Empty。
41. active Owner 要求 child Run 与 consumer definition 相同。
42. active Owner 要求 normalized Host cursor 等于 physical surface cursor。
43. valid pending rejection 明确不是 synchronized。
44. operation guard 阻止 surface callback 重入 update。
45. operation guard 阻止 surface callback 重入 recovery。
46. operation guard 阻止 surface callback 重入 end。
47. mutated-then-rejected surface 被识别为 cursor divergence。
48. divergence 使 Owner invalid，并阻断自动 recovery。
49. 不伪造外部补偿或 rollback transaction。
50. result 字段私有且构造后不可变。
51. result 构造时执行一次完整验证并缓存 `bValidated`。
52. runtime 在构造 result 前仍完整验证 child 返回证据。
53. 外层 result 后续验证只检查冻结证据关联与状态分类。
54. 没有新增 World、Actor、UI implementation、input、projectile、inventory 或 RNG 依赖。

## 修正记录

- 首次 candidate focused run 在 `OrderedAppliedReplay` 出现高 CPU 长耗时；
- 定位为外层 `IsValid()` 对深层 Host/Adapter evidence 的重复递归验证；
- 该 candidate 被有界停止，日志保留且没有作为正式成功证据；
- 将 immutable result 的验证结果缓存，并消除外层重复 deep validation；
- 修正后 focused 5/0，完整套件 1089/0。

未完成 candidate 日志：262409 bytes / SHA-256 `B423EC9ADE87D0346ACD9606AD1044E1BBD5F14AF4583F59346422A2E604F76C` / 无 native terminal。

## 自动化新增

- `LifecycleAndPreflight`；
- `OrderedAppliedReplay`；
- `RejectionRecovery`；
- `DivergenceDetected`；
- `ReentrantSurfaceBlocked`。

focused Owner：5/0；完整 0.0.10：1089/0，比 P20.42 增加 5 项。

## Changed-file regression

- 新增 CompositionOwner exact-path mapping rule；
- RequiredGroups：28；
- mapping self-test：379/379；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=28 / Logs=3；
- Owner focused、legacy ItemUseAndArmor、完整 Shanmen.0_0_10 均有 native terminal evidence。

## 验证证据

- formal focused：5/0，SHA-256 `067CC0E3981D3273F6371FD2A5C71E480D3951B4493F5A248ADCC0D21EA81581`；
- legacy ItemUseAndArmor：46/0，SHA-256 `DD0456A0702979E5B7FC198D4C5948881B7F7D995A38550C1F0D97EF6043851A`；
- full Shanmen.0_0_10：1089/0，SHA-256 `F88FE76ADEE7462654048C6E4607314DE8A66448D364A44053F8B7C6D37F63D7`；
- formal logs total：1140/0，3 native terminal，0 fail；
- boundary scan：2 Owner files / 1183 lines / World-architecture forbidden identifiers 0 / Host update 1 / Adapter apply 1 / Host recovery 1 / Host end 1 / Adapter end 1；
- `git diff --check`：PASS，0 whitespace errors。

完整套件的既有 SwordRhythm journal/checkpoint/manifest codec 慢区段产生非惩罚性 unresponsive 通知；进程持续使用 CPU、恢复逐项进展，最后由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束。

## 构建与产物

- candidate Editor：4 actions / native 0 / 8.52s；
- final Editor：up to date / 0 actions / native 0 / 2.80s；
- final Game：4 actions / native 0 / 33.16s；
- Editor DLL：17179648 bytes / SHA-256 `7589147548605108C846A9269A290B966E9659E8ABB3549A0ABB5793554773C8`；
- Game EXE：358302720 bytes / SHA-256 `7084116AD413A4966AB48646B38B22442080E6DAF6AB6612991087A80C5BF4B6`。

## P/F 边界

PASS：atomic Host/Adapter scope、single-path update、bounded rejection recovery、pending fence、receipt forwarding、cursor normalization、outer replay、typed evidence、divergence detection、reentrancy 与 changed-file regression。

未声明：真实 MainHUD/UI/renderer/visible frame、surface lifetime enforcement、renderer recreation/adoption、外部补偿、跨线程或崩溃持久性、真实输入、World、投掷、Impact、库存消费、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.44：建立 renderer surface recreation/adoption policy，对 Empty、same-Run Visible、foreign Visible startup state 做显式分类，只允许有证据的 exact-state adoption 或显式 Hide cleanup；继续无头 fake surface 验证。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-43-thrown-weapon-arc-preview-composition-owner>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-43-thrown-weapon-arc-preview-composition-owner/Docs/Report/Dev.D.UE.0.0.10.P20.43.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-43-thrown-weapon-arc-preview-composition-owner/Docs/Log/Dev.D.UE.0.0.10.P20.43.r0_log.md>
