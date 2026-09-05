# Dev.D.UE.0.0.10.P20.37.r0 Log

## 阶段

- 任务：P20.37 consumer-owned thrown-weapon Arc preview presentation command acknowledgement/cursor ledger；
- 基线：38a764e71578f3960668d4c641b61933237af31d；
- 分支：agent/0.0.10-p20-37-thrown-weapon-arc-preview-command-ledger；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/真实 renderer/可见轨迹/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 immutable presentation command receipt。
2. receipt 绑定 exact command、Run、consumer、outcome 与 outcome code。
3. outcome 只允许 Applied 或 Rejected。
4. ReceiptId 使用固定命名空间与 canonical parts 确定性派生。
5. receipt IsValid 重新派生 identity 并核对嵌套 command。
6. 明确 receipt 是 caller attestation，不证明 widget、renderer 或 visible frame。
7. 新增 consumer-owned in-memory acknowledgement/cursor ledger。
8. TryBegin 绑定 exact Run 与 ConsumerDefinitionId。
9. LedgerId 从 Run 与 consumer 确定性派生。
10. 同 scope begin 幂等，active scope rotation 拒绝。
11. Rejected receipt 保留证据但不推进 cursor。
12. exact rejection replay 幂等，不重复计数。
13. 同 command 的冲突 rejection evidence 失败关闭。
14. Applied receipt 从 exact current cursor 开始并推进一次。
15. exact application replay 幂等，不重复推进。
16. rejected command 后续 Applied 可恢复，保留 rejection evidence。
17. Applied 后 late rejection 被视为冲突。
18. future/out-of-order command 返回 CursorMismatch。
19. RunMismatch 与 ConsumerMismatch 在 mutation 前拒绝。
20. Show、Replace、Hide、NoOp 都能形成连续 applied chain。
21. NoOp 推进 audit cursor，但不声称 renderer mutation。
22. AppliedCommandIds 保存唯一应用顺序。
23. Entries 按 CommandId 保存 rejected/applied receipts。
24. IsValid 从空 cursor 深度重放 applied chain 并核对最终状态。
25. TryEnd 只接受 active exact Run，随后清空全部 consumer state。
26. 新增 typed result 与 accepted/advance/replay 自校验。
27. 新增 8 项自动化；完整 0.0.10 总数 1045→1053。
28. changed-file map 新增 ledger 规则与正/反 fixture；self-test 367→369。
29. 不新增产品 mutation、库存、World、Actor、UI、renderer 或 input 权威。
30. 历史无关未跟踪文件保持未暂存、未修改、未删除。

## 修正记录

候选 Editor build 与专属 ledger 自动化首轮成功。静态复核发现 EmptyCommandId sentinel 只为引用返回服务，没有状态语义；将 GetLastAppliedCommandId 改为按值返回 FGuid 后删除该成员。随后重新执行候选 Editor build（5 actions / native 0）与专属 ledger 测试（8/0），均成功。该轮是无语义冗余清理，不是失败 build/test 修复。

正式证据进一步精简为 3 日志集合：ledger 专属、ItemUseAndArmor legacy、完整 0.0.10。完整套件直接覆盖 23-group 映射中的所有其它上游组，因此没有重复运行 command/session/coordinator 等定向日志。

完整套件在既有 SwordRhythm journal/checkpoint 大型快照区段出现长 tick；UE 明确记录为不惩罚 unresponsive test，进程持续运行并原生完成 1053/0。没有外层 timeout、强制终止或人工推断成功。

## 最终自动化

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| arc_preview_command_ledger_final.log | 8/0 | 0253D055A9689DDFABECC57DC8C557E0EBD32BB09C670D24ED53BD8F09C3C6A8 |
| item_use_and_armor_final.log | 46/0 | F4A1DD6F8C58362A2F321CE42EFC2AD4A70F71D65F4B2A125BA4F3BECBDC756E |
| full_0_0_10_final.log | 1053/0 | FBE8727D20D99B6D321E73CFEFBEC321B15B54915CA7860E484ADD9984C262A8 |

正式日志累计 1107/0，包含定向、legacy 与全量重叠；独立完整套件为 1053/0。

## 流程与静态证据

- automation audit：PASS；3 logs / 1107 success / 0 fail-or-not-run；SHA-256 95CBA2B2764123826442D7A5DE001882648FC7D8895AE1D6196DE664DAE41F92；
- regression self-test：369/369；SHA-256 481ABBE22FD5A878BB1AE9DF03A54F425ECADAE9728EFC8FCDF59956A1CD019D；
- boundary scan：PASS；2 files / 848 lines / forbidden 0 / deterministic ID factories 2 / World-UI include 0 / placeholder 0 / record entrypoints 1 / map rule 1/23 groups；SHA-256 78C3AE2972E433258E5B7D4811227A91C44D172B32B04CBA49A01CCE1AA6CF7A；
- changed-file gate：PASS Changed=5 Rules=2 Required=23 Logs=3；SHA-256 C05E9C407E6DB0406B48C309C829CF699F1F0E1EFD6807A6F8D9682A215A9E47；
- git diff check：PASS；7 个本轮文件、0 个空白错误；SHA-256 13FE89CEEE4B9AB5F58D08E77DA87FC586A6303DC09FD2DF83BC91B6310A26D6；
- staged diff check：PASS；7 个精确暂存文件、0 个未暂存已跟踪文件、103 个历史无关未跟踪文件保持在外、0 个空白错误；SHA-256 B872C65927DAEA927073BB7698AA1E89DBDD0C3D2553C4ADE73C6A4520A6B6FD。

## 构建与产物

- candidate Editor（静态复核后）：5 actions / native 0 / 19.74 秒；SHA-256 D62EDE693DAA6BED9F770620E487941A0C22EDD1714CD3FA3D4D2FBEE103450C；
- candidate focused ledger（静态复核后）：8/0；SHA-256 6D90C66AF0AA24CB431DF80D0211545CD8352E3393724D92A82DE3A23B4FADBD；
- final Editor：up to date / native 0 / 1.19 秒；SHA-256 AA67061234B587391418750649DE3A623797370827E0FED5104529A8E8B12AFF；
- final Game：4 actions / native 0 / 28.98 秒；SHA-256 03D556E63C8FFC7CF07D1A0B81AE5912F3144DFB9829A4FEAA9DAE24642DC96C；
- Editor artifact：16,835,584 bytes；SHA-256 3DC416EE6034583BF8AFD1940DFBBAA9D0727E5B5068F411A0A8307F30ACEC5A；
- Game artifact：358,021,120 bytes；SHA-256 B63F825E2522D0CDA36125E51D998ECE44B00841DF56BCCD939808F772B7CDDB。

## P/F

PASS：receipt deterministic identity、自校验、Run/consumer scope、Applied/Rejected、rejection recovery、exact replay、conflict/out-of-order fail-closed、Show/Replace/Hide/NoOp cursor、exact teardown 与完整 changed-file 回归。

未验证：真实 renderer/port、HUD/UI、widget/component/visible frame、delivery coordinator、World trace/collision/occlusion、真实设备 choice、可见轨迹、Editor UI/PIE/Standalone、产品启动、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

## 下一步

P20.38：建立 renderer-neutral presentation port 与 bounded delivery coordinator；每条 command 最多调用一次抽象 port，把结果封存为 Applied/Rejected receipt 并写入本 ledger。先使用 fake port 自动化，不连接真实 HUD、renderer 或 World。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-37-thrown-weapon-arc-preview-command-ledger>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-37-thrown-weapon-arc-preview-command-ledger/Docs/Report/Dev.D.UE.0.0.10.P20.37.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-37-thrown-weapon-arc-preview-command-ledger/Docs/Log/Dev.D.UE.0.0.10.P20.37.r0_log.md>
