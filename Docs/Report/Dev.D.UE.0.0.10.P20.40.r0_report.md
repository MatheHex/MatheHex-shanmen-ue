# Dev.D.UE.0.0.10.P20.40.r0 Report

## 1. 结论

P20.40 已为 thrown-weapon Arc preview presentation delivery Session 建立窄范围的 externally-attested recovery seam。调用方只能向 active Session 提交一个自验证的 Applied receipt；Session 只在私有 ledger 已保存同 Run、同 consumer、同 CommandId、同完整 command 的 Rejected evidence 时接受恢复。

恢复使用 candidate ledger 原子提交，不调用 renderer port 或 P20.38 coordinator。被拒 Hide 恢复为 Applied 后 cursor 从 Visible 推进到 Hidden，此时 graceful end 才成为合法操作；冲突或越界证据保持 fail-closed。

本阶段仍是 P 阶段无头契约。receipt 是确定性的 caller attestation，不是密码学签名，也不证明真实 HUD、renderer、widget、component 或可见帧已经成功更新。

## 2. 基线、分支与改动范围

- 基线：`1339856548f30fa913bfb17e88156da7822d0ddc`（P20.39）；
- 分支：`agent/0.0.10-p20-40-thrown-weapon-arc-preview-attested-recovery`；
- 修改 `demo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession.h/.cpp`；
- 修改 `demo_mapShanmenThrownWeaponProductLifecycleTests.cpp`；
- 新增本 Report 与同名 Log；
- 未修改 P20.37 ledger 的 public API，未公开 mutable ledger；
- 未连接真实 HUD、World、Actor、widget、component、输入、projectile、库存或产品 mutation。

## 3. 恢复入口

新增 `TryRecoverRejected(AppliedReceipt)`，固定执行以下前置条件：

1. Session 必须 valid、active 且不在 delivery callback 内；
2. receipt 必须完整自验证并且 outcome 为 Applied；
3. receipt RunId 必须等于 Session RunId；
4. receipt consumer 必须等于 Session consumer；
5. 私有 ledger 必须已经保存 exact CommandId 的 Rejected receipt；
6. rejected 与 applied receipt 必须携带完全相同的 command；
7. 只有底层返回 `ApplicationRecovered` 或 exact `ApplicationReplayed` 才能通过。

缺少 prior rejection 的普通 Applied command 不能借此入口写入 ledger。Rejected receipt、foreign Run、foreign consumer、未知 CommandId 与并发 delivery callback 均在 ledger mutation 前被拒绝。

## 4. 原子恢复与结果自验证

Session 不直接修改 live ledger：

1. 保存 previous cursor 与计数；
2. 复制 candidate ledger；
3. 对 candidate 恰好调用一次 `Record(AppliedReceipt)`；
4. 检查 nested ledger result、Run/consumer scope 和 candidate invariants；
5. 仅接受 `ApplicationRecovered` 或 `ApplicationReplayed`；
6. 提交后再次构造并验证 typed recovery result；
7. 任一 post-commit invariant 异常时恢复 exact previous ledger。

新增 recovery status/result 区分 inactive、invalid、busy、invalid receipt、非 Applied、Run/consumer mismatch、missing rejection、command mismatch、ledger rejection、Recovered、RecoveryReplayed 与 invariant violation。result 同时约束 receipt、ledger call count、nested ledger result、before/after cursor、applied/rejected count。

## 5. 幂等与冲突语义

- 首次 exact Applied evidence：`Recovered`，applied count 增加 1，rejected count 保留；
- 同一 Applied receipt 重放：`RecoveryReplayed`，cursor 与计数不变；
- 同一 command 的另一份 Applied outcome code：底层 `ReceiptConflict`，Session 返回 `LedgerRejected`；
- rejected 与 applied receipt 都保留，便于审计原始失败和后续恢复；
- 恢复或重放都不会再次调用 port；
- delivery callback 内的恢复返回 `SessionBusy`，ledger record count 为 0。

## 6. Hide 恢复与关闭边界

自动化覆盖了完整状态序列：

1. Show 经 fake port Applied，cursor 为 Visible；
2. Hide 经 fake port Rejected，cursor 继续 Visible，Session 不可关闭；
3. exact external Applied receipt 恢复该 Hide；
4. cursor 唯一推进到 Hidden，applied/rejected 为 2/1；
5. exact recovery replay 不推进；
6. Hidden cursor 允许 `TryEnd` 原子清空 Session 与 ledger。

因此“收到一份恢复请求”本身不能放行关闭，只有 accepted recovery result 和 Hidden/Empty cursor 共同成立才可关闭。

## 7. 自动化覆盖

Session 聚焦组由 7 项增至 10 项，新增：

1. `RecoveryPreflight`：inactive、invalid、非 Applied、无 prior rejection、foreign consumer 与 foreign Run；
2. `RejectedHideRecovery`：Rejected Hide 的 exact Applied 恢复、幂等 replay 与 Hidden end；
3. `RecoveryConflict`：同 command 不同 Applied evidence 冲突且 live ledger 不变。

既有 `ReentrantPortBlocked` 追加 callback 内 recovery 注入，验证在 second ledger/port/coordinator effect 前返回 `SessionBusy`。完整 `Shanmen.0_0_10` 从 1068 增至 1071 项。

## 8. Changed-file 回归映射

本轮不需要增加 mapping rule：P20.39 的 `ThrownWeaponArcPreviewPresentationDeliverySession` 规则已经覆盖 Session header/cpp；既有 `ThrownWeaponProductLifecycle` 规则覆盖承载测试的 cpp。两条规则求并集仍要求 25 个 group。

- mapping self-test：373/373，SHA-256 `5B007DFB655FF35AC8A644555BCA03E2A362250611F3BDA49E52A5E1ED1F4F77`；
- changed-file gate：PASS，Changed=3 / Rules=2 / Required=25 / Logs=3，SHA-256 `A8D84178C3550D9BF6D39B0B6ECE3A12FCBB5414FDADF0C0818F7A3281122EA3`。

## 9. 自动化、静态检查与构建

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| delivery_session_final.log | Product.ThrownWeaponArcPreviewPresentationDeliverySession | 10/0 | 1A4E344AFB6F57F9DD8E73989CA690228E31F8E934738D95B8CFD680371CAA0F |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 59322664593D078115CDD3A9CDB17C40EDBDFDED8758E85B2E6D26FBE4F4BF5C |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1071/0 | 1F57BC6A08EF51D4276E1A32BFA1ACB663F9AE72373FFC6664010D668276B255 |

正式日志累计 1127/0，包含定向、legacy 与完整套件重叠；独立完整套件为 1071/0。完整套件在既有 SwordRhythm journal/checkpoint/manifest codec 区段出现非惩罚性 unresponsive 通知，随后原生完成；没有外层 timeout、拆组或人工中止替代结果。

- automation audit：PASS，3 logs / 1127 success / 0 fail-or-not-run / 3 terminal / 0 fatal / native 0，SHA-256 `F16D0D174EAF217C45EF23DD6C8B3BD3A01F5DF3AA1EADCC2F4C4815AEB4BF88`；
- boundary scan：PASS，2 files / 1155 lines / forbidden 0 / includes 3 / coordinator entrypoint 1 / recovery entrypoint 1 / ledger record entrypoint 1 / prior-rejection check 1 / reentrancy guard 1 / mutable ledger access 0 / map rule 1 / required groups 25 / focused tests 10，SHA-256 `57A17D1539662F6913AF51D8F56F368AA32D6721BD7F20CA8267BE5303E3546B`；
- candidate Editor：5 actions / native 0 / 24.09 秒，SHA-256 `3B3897EF513ECA638E5CD165C8694FE735CA250763D781DD2D88703943B96636`；
- candidate focused：10/0，SHA-256 `EBAA89C9BFF76333AF6ADEBDF8AE9AE8E510EEE584021BAB8B09AEBCDB9C6153`；
- final Editor：up to date / 0 actions / native 0 / 1.71 秒，SHA-256 `28DB7BBAD752ED74133069E8ABB5EC2591B41F78AC0A0D02232CBE0C0DD2A8F0`；
- final Game：4 actions / native 0 / 29.99 秒，SHA-256 `6574582E43E4BF680AC1274D3B889D63B96F2E6B8DE82B4848859B1EE5E93F7C`；
- git diff check：PASS，5 个本轮文件、0 个空白错误，SHA-256 `F78A637E22311A1320F3CB9DDF344FAF2752B8CC41CBA016E010BF9BDFC00F6A`；
- staged diff check：PASS，5 个精确暂存文件、0 个未暂存已跟踪文件、103 个历史无关未跟踪文件保持在外、0 个空白错误，SHA-256 `D886C0243C7BC72908D9D770153C2F68760F8B9A60FF82C6570D8F54C3640E13`。

证据采集修正：首次候选 focused 调用使用 `-log` 而非绝对日志参数，进程 native 0 但没有生成指定证据文件，因此未计入验证；随后改用 `-AbsLog` 完整重跑并取得 10/0 与原生终止标记。

产物：

- `Binaries/Win64/UnrealEditor-demo_map.dll`：16980992 bytes，SHA-256 `F97112BCE372B95D91CBA03906F99B1C1719A803005D1B61BA031BCFAF759B0B`；
- `Binaries/Win64/demo_map.exe`：358141952 bytes，SHA-256 `E1C0607EDC6E38FAEB135431880D0689EFFA27203A0AEDAAF0123C720F68CE89`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：exact prior-rejection gate、Applied-only recovery、Run/consumer/command fence、candidate-ledger atomic commit、typed self-validating result、exact recovery replay、conflicting receipt rejection、delivery callback busy fence、Recovered Hide → Hidden → graceful end，以及完整 changed-file 回归。

未验证：receipt 来源真实性或密码学签名、真实 renderer 是否应用命令、真实 HUD/widget/component/visible frame、跨线程同步、进程崩溃持久 exactly-once、设备输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

建议 P20.41 建立 consumer-owned Arc preview presentation delivery Host：在同一 Run scope 内组合 P20.35 state Session、P20.36 command projector 与 P20.39/40 delivery Session，冻结 update → command → delivery 的单次有界顺序，并以 fake port 验证 state/cursor 一致性；真实 HUD adapter 继续后置。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-40-thrown-weapon-arc-preview-attested-recovery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-40-thrown-weapon-arc-preview-attested-recovery/Docs/Report/Dev.D.UE.0.0.10.P20.40.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-40-thrown-weapon-arc-preview-attested-recovery/Docs/Log/Dev.D.UE.0.0.10.P20.40.r0_log.md>
