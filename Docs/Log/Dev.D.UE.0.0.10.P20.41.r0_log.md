# Dev.D.UE.0.0.10.P20.41.r0 Log

## 阶段

- 任务：P20.41 thrown-weapon Arc preview presentation delivery Host；
- 基线：`ea52b9b4af0f1484d7b73cb0d43c5a10f96550a9`；
- 分支：`agent/0.0.10-p20-41-thrown-weapon-arc-preview-delivery-host`；
- 目标：由一个 consumer-owned Run-scoped Host 组合 state Session、command projector 和 delivery Session，冻结有界顺序与 state/cursor recovery invariants。

## 实现记录

1. 新增 Host status 与 self-validating update result。
2. 新增 recovery status 与 self-validating recovery result。
3. Host 私有拥有一个 presentation state Session。
4. Host 私有拥有一个 presentation delivery Session。
5. Host 私有保存 exact pending rejected command。
6. Host 私有 operation-in-progress guard 覆盖 callback 重入。
7. `TryBegin` 同时绑定两个 Session 的 Run scope。
8. exact Run/consumer begin replay 幂等成功。
9. active scope rotation 在任何部分重绑前拒绝。
10. `TryUpdate` inactive/invalid/reentrant 时零 delegated call。
11. pending rejection 时较新 update 返回 `RecoveryRequired`。
12. update 只构造一个 candidate Host。
13. candidate 对 state Session 最多调用一次 `TryUpdate`。
14. state accepted 后对 pure projector 最多调用一次 `Project`。
15. command projected 后对 delivery Session 最多调用一次 `TryDeliver`。
16. 三个调用严格保持 state -> projection -> delivery 顺序。
17. state/projection/pre-delivery rejection 不修改 live Host。
18. Applied delivery 提交同步 desired state 与 cursor。
19. exact application replay 不重复推进 ledger/cursor。
20. renderer Rejected 提交 rejection evidence 与 desired state。
21. renderer Rejected 保持 previous cursor。
22. renderer Rejected 保存 exact pending command。
23. pending command 清除前禁止 graceful end。
24. recovery 只接受 exact pending command 的 Applied receipt。
25. foreign Run/consumer/CommandId/payload receipt 在 ledger 前拒绝。
26. recovery 使用 candidate delivery Session，不调用 port。
27. exact recovery 清除 pending 并恢复 state/cursor 同步。
28. exact recovery replay 幂等接受且不推进 cursor。
29. Rejected Hide recovery 后 cursor 唯一推进到 Hidden。
30. `TryEnd` 要求 scope 匹配、无 pending、同步且 Hidden/Empty。
31. end 同时清空两个 Session，避免半关闭状态。
32. typed results 绑定 nested results、调用计数和 before/after 状态。
33. Host `IsValid` 区分正常同步与有 exact rejection evidence 的合法 pending divergence。
34. callback 内 reentrant update/end 在 second effect 前拒绝。
35. 不公开 mutable Session/ledger，也不新增 Reset 绕过入口。
36. 没有新增 World、Actor、UI、renderer implementation、input 或 RNG 依赖。

## 自动化新增

- `Lifecycle`；
- `OrderedAppliedReplay`；
- `PreflightAndStateReject`；
- `AtomicDeliveryReject`；
- `RejectionRecoveryFence`；
- `HideRecoveryEnd`；
- `ReentrantPortBlocked`。

focused Host：7/0；完整 0.0.10：1078/0，比 P20.40 增加 7 项。

## Changed-file regression

- 新增 Host exact-path mapping rule；
- RequiredGroups：26；
- mapping self-test：375/375；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=26 / Logs=3；
- Host focused、legacy ItemUseAndArmor、完整 Shanmen.0_0_10 均有 native terminal evidence。

## 验证证据

- formal focused：7/0，SHA-256 `D1860CCCFE23AA3989B8261CB7B73E370994BD445D26F5180A06D627D2432DFE`；
- legacy ItemUseAndArmor：46/0，SHA-256 `598F327381657F5EE5D59F03116AAF318561B89422559BA0543C847615F757AA`；
- full Shanmen.0_0_10：1078/0，SHA-256 `8A60427794D71FBD8A6C7BDD7944D99B58BED469330CA520E221ADE6FD503A19`；
- formal logs total：1131/0，3 native terminal，0 fail；
- boundary scan：2 Host files / 1139 lines / forbidden 0 / state update 1 / projector 1 / delivery 1 / recovery 1；
- `git diff --check`：PASS，0 whitespace errors。

完整套件的既有 SwordRhythm journal/checkpoint/manifest codec 慢区段产生非惩罚性 unresponsive 通知；进程持续使用 CPU、恢复逐项进展，最后由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束。

## 构建与产物

- candidate Editor：新增 Host 与测试完成编译，native 0；
- final Editor：0 actions / native 0 / 1.50s；
- final Game：4 actions / native 0 / 24.40s；
- Editor DLL：17053696 bytes / SHA-256 `D3AFA14AD665418510D8313993C331F15CFDDDA48B2EB1442F825A3322FD588D`；
- Game EXE：358198784 bytes / SHA-256 `6F6C8B21D6239592FA3AC06AD02203222FB7E81E6C37CA4FACFFE7C8541864F6`。

## 修正记录

focused 候选初次运行在 `OrderedAppliedReplay` 暴露深层 `IsValid()` 重复遍历 receipt/ledger/state 的高计算成本。保持 contract 不变，改为单 candidate commit、单次 nested classification，并移除重复 candidate validity 检查；临时诊断日志随后全部删除。正式 focused 与 full 均使用最终实现重跑。

静态扫描初稿以 `Consume` 子串匹配，误把 `ConsumerDefinitionId` 计为 forbidden。最终扫描改用 identifier 单词边界，结果为 forbidden 0。

## P/F 边界

PASS：Run scope、bounded order、candidate state、fake port、Applied/replay、Rejected recovery fence、exact recovery/replay、Hide end、typed result、reentrancy 与 changed-file regression。

未声明：真实 HUD/UI/renderer/visible frame、receipt 来源真实性、外部副作用补偿、线程/崩溃持久性、真实输入、World、投掷、Impact、库存消费、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.42：建立 renderer-facing Arc preview consumer adapter contract，把 Show/Replace/Hide/NoOp 映射到单一 consumer surface，冻结 applied/rejected response 与 surface cursor；先使用无头 fake surface，真实 MainHUD/widget 绑定后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-41-thrown-weapon-arc-preview-delivery-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-41-thrown-weapon-arc-preview-delivery-host/Docs/Report/Dev.D.UE.0.0.10.P20.41.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-41-thrown-weapon-arc-preview-delivery-host/Docs/Log/Dev.D.UE.0.0.10.P20.41.r0_log.md>
