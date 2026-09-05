# Dev.D.UE.0.0.10.P20.42.r0 Log

## 阶段

- 任务：P20.42 thrown-weapon Arc preview presentation consumer adapter；
- 基线：`16912bb62683bcea3bd72425c09d1a0a37d44d0e`；
- 分支：`agent/0.0.10-p20-42-thrown-weapon-arc-preview-consumer-adapter`；
- 目标：把既有 delivery port 的 Show/Replace/Hide/NoOp 映射到单一 renderer-facing surface，冻结 applied/rejected response 与 physical surface cursor。

## 实现记录

1. 新增 immutable surface response outcome：Invalid、Applied、Rejected。
2. surface response 绑定 deterministic ResponseId。
3. response 同时绑定 CommandId、CommandKind 与稳定 outcome code。
4. response 保存 previous/after physical surface cursor。
5. physical cursor 只允许 Empty 或 Visible。
6. Hidden audit state 在 surface 边界归一化为空。
7. Show Applied 固定为 Empty -> Visible。
8. Replace Applied 固定为 Visible -> different Visible。
9. Hide Applied 固定为 Visible -> Empty。
10. Rejected response 必须保持 cursor 不变。
11. NoOp 无法创建 surface response。
12. 新增 narrow surface interface。
13. surface 分别公开 Show、Replace、Hide，禁止 kind reinterpretation。
14. surface 不公开 NoOp mutation。
15. 新增 Run-scoped consumer Adapter，实现既有 delivery port。
16. Adapter 保存非 owning surface pointer，调用方负责 active scope 生命周期。
17. `TryBegin` 要求有效 Run、稳定 consumer identity 与 Empty surface。
18. exact begin replay 幂等成功。
19. active Run/surface rotation 在重绑前拒绝。
20. `TryEnd` 要求 expected Run、无 operation 与 Empty surface。
21. Apply inactive/invalid/reentrant 时零 surface call。
22. foreign Run 在 surface call 前拒绝。
23. consumer identity drift 在 surface call 前拒绝。
24. command previous cursor mismatch 在 surface call 前拒绝。
25. NoOp 生成 Applied port response，surface call count 为 0。
26. mutating command 只执行一个对应 typed surface method。
27. 每次 dispatch 后重新读取实际 surface cursor。
28. valid Applied response 映射为 Applied port response。
29. valid Rejected response 映射为 Rejected port response。
30. Rejected response 不推进物理 cursor。
31. foreign/invalid response fail-closed。
32. Applied-without-mutation fail-closed。
33. Rejected-with-mutation fail-closed。
34. operation guard 阻止 surface callback 内 Apply/end 重入。
35. Adapter result 绑定 status、diagnostic、Run、consumer 与 command。
36. Adapter result 绑定 surface call count、surface response 与 port response。
37. Adapter result 绑定 before/after physical cursor。
38. result `IsValid()` 交叉验证状态分类、调用预算与证据转换。
39. fake surface 支持 Applied、Rejected、InvalidResponse、AppliedWithoutMutation 与 MutatedThenRejected。
40. fake reentrant surface 验证 callback fence。
41. 通过真实 delivery Session 集成验证 Show/Replace/NoOp/Hide。
42. exact delivery replay 不重复触发 surface mutation。
43. 不公开 mutable ledger、retry loop 或产品 authority。
44. 没有新增 World、Actor、UI implementation、input、projectile、inventory 或 RNG 依赖。

## 自动化新增

- `SurfaceResponseContract`；
- `LifecycleAndPreflight`；
- `ShowReplaceNoOpHide`；
- `SurfaceRejectionReplay`；
- `SurfaceInvariantFailures`；
- `ReentrantSurfaceBlocked`。

focused Adapter：6/0；完整 0.0.10：1084/0，比 P20.41 增加 6 项。

## Changed-file regression

- 新增 Adapter exact-path mapping rule；
- RequiredGroups：27；
- mapping self-test：377/377；
- changed-file gate：PASS，Changed=5 / Rules=2 / Required=27 / Logs=3；
- Adapter focused、legacy ItemUseAndArmor、完整 Shanmen.0_0_10 均有 native terminal evidence。

## 验证证据

- formal focused：6/0，SHA-256 `CA7EEF451B5919E218C75A92D3E49BABE29BEEB297A9198F1403541EA8517374`；
- legacy ItemUseAndArmor：46/0，SHA-256 `6067EECBDF368CB35108FF046E1AF41EF90450554D8E8F69C2140F3CC2C91256`；
- full Shanmen.0_0_10：1084/0，SHA-256 `6FFBCA7D727B5490FDB1D5DD69E53B12A225D055ABEE1DB1213BDCCC51E75194`；
- formal logs total：1136/0，3 native terminal，0 fail；
- boundary scan：2 Adapter files / 1076 lines / renderer-architecture forbidden identifiers 0 / Show 1 / Replace 1 / Hide 1；
- `git diff --check`：PASS，0 whitespace errors。

完整套件的既有 SwordRhythm journal/checkpoint/manifest codec 慢区段产生非惩罚性 unresponsive 通知；进程持续使用 CPU、恢复逐项进展，最后由 UE 原生 `TEST COMPLETE. EXIT CODE: 0` 结束。

## 构建与产物

- candidate Editor：新增 Adapter 与测试完成编译，native 0；
- final Editor：4 actions / native 0 / 34.38s；
- final Game：4 actions / native 0 / 31.49s；
- Editor DLL：17120256 bytes / SHA-256 `728A4578584769F9AB12B92C3214B83A2CC5EA4942141021B6D9D94437B0D68C`；
- Game EXE：358251520 bytes / SHA-256 `83A8099EAB8A17CDACCE8C4361E60AD09B18E0F450FFF6D4B4F9673466F2A96C`。

## P/F 边界

PASS：surface response、Run/consumer scope、physical cursor normalization、typed dispatch、zero-call NoOp、Applied/Rejected mapping、delivery replay、invariant failure、typed result、reentrancy 与 changed-file regression。

未声明：真实 MainHUD/UI/renderer/visible frame、surface evidence 来源真实性、外部副作用补偿、跨线程或崩溃持久性、Host/Adapter 共同 owner 与 recovery reconciliation、真实输入、World、投掷、Impact、库存消费、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.43：建立 Run-scoped composition owner，把 P20.41 delivery Host 与 P20.42 consumer Adapter 绑定到同一 Run/consumer/surface，冻结 begin/update/end 顺序和 rejection recovery / physical surface reconciliation；继续使用无头 fake surface。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-42-thrown-weapon-arc-preview-consumer-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-42-thrown-weapon-arc-preview-consumer-adapter/Docs/Report/Dev.D.UE.0.0.10.P20.42.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-42-thrown-weapon-arc-preview-consumer-adapter/Docs/Log/Dev.D.UE.0.0.10.P20.42.r0_log.md>
