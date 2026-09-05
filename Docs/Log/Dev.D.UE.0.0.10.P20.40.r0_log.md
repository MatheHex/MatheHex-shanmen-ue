# Dev.D.UE.0.0.10.P20.40.r0 Log

## 阶段

- 任务：P20.40 thrown-weapon Arc preview externally-attested rejected-command recovery；
- 基线：`1339856548f30fa913bfb17e88156da7822d0ddc`；
- 分支：`agent/0.0.10-p20-40-thrown-weapon-arc-preview-attested-recovery`；
- 目标：让 P20.39 Session 在不公开 mutable ledger、不重试 port 的前提下，安全接收 exact rejected command 的后续 Applied receipt。

## 实现记录

1. 在 delivery Session header 增加 recovery status 与 self-validating result。
2. 新增 `TryRecoverRejected` 单一 public recovery seam。
3. Session inactive 在任何 ledger mutation 前拒绝。
4. delivery callback 期间返回 `SessionBusy`。
5. invalid receipt 在 ledger mutation 前拒绝。
6. Rejected outcome 不能伪装 recovery；只接受 Applied。
7. receipt RunId 必须等于 Session RunId。
8. receipt consumer 必须等于 Session consumer。
9. ledger 必须已有 exact CommandId 的 Rejected receipt。
10. rejected/applied receipt 的完整 command 必须匹配。
11. 普通未拒绝 command 不能通过 recovery seam 首次写入。
12. live ledger 在 recovery 前复制为 candidate。
13. candidate 对 Applied receipt 恰好调用一次 `Record`。
14. `ApplicationRecovered` 映射为 Session `Recovered`。
15. `ApplicationReplayed` 映射为 `RecoveryReplayed`。
16. 其它 accepted ledger status 视为 invariant violation。
17. ledger conflict 映射为 typed `LedgerRejected`。
18. candidate 继续验证 active Run 与 consumer scope。
19. typed result 绑定 receipt、nested ledger result 与 before/after cursor。
20. typed result 绑定 ledger record call count 0/1。
21. typed result 绑定 applied/rejected before/after count。
22. post-commit result 异常恢复 exact previous ledger。
23. recovery 不调用 P20.38 coordinator。
24. recovery 不调用 renderer port。
25. exact Applied receipt replay 幂等且不推进 cursor。
26. 同 command 的不同 Applied outcome code 触发 `ReceiptConflict`。
27. rejected 与 applied evidence 同时保留供审计。
28. Rejected Hide 恢复后 cursor 从 Visible 推进到 Hidden。
29. Hidden cursor 才允许 graceful end。
30. 既有 reentrant fake port 增加 recovery 注入断言。
31. ledger 仍只以 const getter 暴露，无 public Session Reset。
32. 没有新增 World、Actor、UI、renderer、input 或 RNG 依赖。

## 自动化新增

- `RecoveryPreflight`；
- `RejectedHideRecovery`；
- `RecoveryConflict`；
- `ReentrantPortBlocked` 增加 callback recovery busy fence。

focused Session：10/0；完整 0.0.10：1071/0，比 P20.39 增加 3 项。

## 验证证据

- formal focused：10/0，SHA-256 `1A4E344AFB6F57F9DD8E73989CA690228E31F8E934738D95B8CFD680371CAA0F`；
- legacy ItemUseAndArmor：46/0，SHA-256 `59322664593D078115CDD3A9CDB17C40EDBDFDED8758E85B2E6D26FBE4F4BF5C`；
- full Shanmen.0_0_10：1071/0，SHA-256 `1F57BC6A08EF51D4276E1A32BFA1ACB663F9AE72373FFC6664010D668276B255`；
- automation audit：1127/0、3 terminal、0 fatal、native 0，SHA-256 `F16D0D174EAF217C45EF23DD6C8B3BD3A01F5DF3AA1EADCC2F4C4815AEB4BF88`；
- mapping self-test：373/373，SHA-256 `5B007DFB655FF35AC8A644555BCA03E2A362250611F3BDA49E52A5E1ED1F4F77`；
- changed-file gate：Changed=3 / Rules=2 / Required=25 / Logs=3，SHA-256 `A8D84178C3550D9BF6D39B0B6ECE3A12FCBB5414FDADF0C0818F7A3281122EA3`；
- boundary scan：Files=2 / Lines=1155 / Forbidden=0 / Coordinator=1 / Recovery=1 / LedgerRecord=1 / PriorRejection=1 / Guard=1 / MutableLedger=0 / RequiredGroups=25 / FocusedTests=10，SHA-256 `57A17D1539662F6913AF51D8F56F368AA32D6721BD7F20CA8267BE5303E3546B`；
- git diff check：5 files / 0 whitespace errors，SHA-256 `F78A637E22311A1320F3CB9DDF344FAF2752B8CC41CBA016E010BF9BDFC00F6A`；
- staged diff check：5 staged / 0 unstaged tracked / 103 unrelated untracked preserved / 0 whitespace errors，SHA-256 `D886C0243C7BC72908D9D770153C2F68760F8B9A60FF82C6570D8F54C3640E13`。

正式测试总计 1127/0 包含定向、legacy 与完整套件重叠。完整套件慢速 SwordRhythm journal/checkpoint/manifest codec 段产生非惩罚性 unresponsive 通知，最终由 UE 原生完成。

## 构建与产物

- candidate Editor：5 actions / native 0 / 24.09s，SHA-256 `3B3897EF513ECA638E5CD165C8694FE735CA250763D781DD2D88703943B96636`；
- candidate focused：10/0，SHA-256 `EBAA89C9BFF76333AF6ADEBDF8AE9AE8E510EEE584021BAB8B09AEBCDB9C6153`；
- final Editor：0 actions / native 0 / 1.71s，SHA-256 `28DB7BBAD752ED74133069E8ABB5EC2591B41F78AC0A0D02232CBE0C0DD2A8F0`；
- final Game：4 actions / native 0 / 29.99s，SHA-256 `6574582E43E4BF680AC1274D3B889D63B96F2E6B8DE82B4848859B1EE5E93F7C`；
- Editor DLL：16980992 bytes / SHA-256 `F97112BCE372B95D91CBA03906F99B1C1719A803005D1B61BA031BCFAF759B0B`；
- Game EXE：358141952 bytes / SHA-256 `E1C0607EDC6E38FAEB135431880D0689EFFA27203A0AEDAAF0123C720F68CE89`。

## 修正记录

首次候选 focused 调用使用 `-log`，进程 native 0 但指定 evidence file 未生成。该运行未计入正式结果；改用 `-AbsLog` 后完整重跑，得到 10/0 和 native terminal marker。

## P/F 边界

PASS：exact rejected-command gate、Applied-only receipt、Run/consumer/command fence、atomic recovery、typed result、replay、conflict rejection、callback busy fence、Recovered Hide 安全结束，以及 changed-file regression。

未声明：真实 receipt 来源、真实 renderer/HUD/UI 可见结果、线程或崩溃持久性、真实输入、World、投掷、Impact、库存消费、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 下一阶段

P20.41：建立 consumer-owned Arc preview presentation delivery Host，在一个 Run scope 内组合 state Session、command projector 与 delivery Session，以 fake port 冻结 update → command → delivery 顺序和 state/cursor 一致性；真实 HUD adapter 后置。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-40-thrown-weapon-arc-preview-attested-recovery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-40-thrown-weapon-arc-preview-attested-recovery/Docs/Report/Dev.D.UE.0.0.10.P20.40.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-40-thrown-weapon-arc-preview-attested-recovery/Docs/Log/Dev.D.UE.0.0.10.P20.40.r0_log.md>
