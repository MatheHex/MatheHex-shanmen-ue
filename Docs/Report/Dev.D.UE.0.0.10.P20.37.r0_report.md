# Dev.D.UE.0.0.10.P20.37.r0 Report

## 1. 结论

P20.37 已在 P20.36 renderer-neutral Arc preview presentation command 之上建立 consumer-owned acknowledgement/cursor ledger。消费端现在可以为一个精确 CommandId 记录 Applied 或 Rejected receipt；Applied 推进连续 presentation cursor，Rejected 只保留失败证据而不推进 cursor。相同 receipt 重放幂等，冲突证据、越序 command、跨 Run 或跨 consumer 的 receipt 均失败关闭。

receipt 是调用方对消费结果的不可变声明，不是“真实 widget、renderer、component 或可见帧已经发生”的证明。ledger 也不调用 renderer；它只封存 deterministic identity、scope、outcome、outcome code 与连续状态链，为下一阶段的抽象 presentation port 提供可审计边界。

新增 8 项自动化，完整 0.0.10 由 1045 增至 1053/0。按 changed-file 映射采用 3 份正式日志，累计执行记录 1107/0；累计数包含专属、legacy 与全量日志的有意重叠。映射 self-test 为 369/369，Editor 与 Game Development 构建均成功。

本轮没有连接 HUD/UI、真实 renderer、World、Actor/组件、真实设备输入、trace、collision、projectile、launch、库存 reserve/consume 或 Impact。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，未执行截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：38a764e71578f3960668d4c641b61933237af31d（P20.36）；
- 分支：agent/0.0.10-p20-37-thrown-weapon-arc-preview-command-ledger；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Receipt contract

新增 Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt：

1. 只接受一个自校验有效的 P20.36 command；
2. 必须提供稳定、非空的 ConsumerDefinitionId；
3. outcome 只允许 Applied 或 Rejected；
4. 必须提供稳定、非空的 OutcomeCode；
5. ReceiptId 由 CommandId、RunId、consumer、outcome 与 outcome code 确定性派生；
6. IsValid 会重新派生 ReceiptId 并核对嵌套 command；
7. Matches 比较完整 receipt 证据，而不是只比较 GUID；
8. receipt 不持有 renderer、widget、component、World 或产品 authority。

Applied 表示命名 consumer 声明已消费该 command；Rejected 表示该 consumer 给出一个稳定失败代码。两者都是 caller attestation，不把无头测试结果夸大为真实视觉呈现。

## 4. Ledger scope 与生命周期

新增 Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger。TryBegin 将空 ledger 绑定到一个精确 RunId 与 ConsumerDefinitionId，并确定性派生 LedgerId。同 scope 重复 begin 幂等；active 状态下尝试更换 Run 或 consumer 会被拒绝。

TryEnd 只接受 active exact Run，成功后清除 LedgerId、Run、consumer、cursor、entries 与 applied order。错误 Run 不能 teardown。Reset 可显式回到自校验空状态。

IsValid 不只检查字段：它从空 cursor 按 AppliedCommandIds 顺序重放全部 applied receipts，验证每个 command 的 previous/current 状态链、Run/consumer scope、entry 唯一性、rejected/applied 组合与最终 cursor 完全一致。内部证据一旦互相矛盾，后续 Record 返回 LedgerInvalid，不继续变更。

## 5. Applied、Rejected 与恢复语义

Record 的规则如下：

1. Rejected receipt 必须位于当前 cursor；记录后 rejected count 增加，cursor 与 applied count 不变；
2. 精确 Rejected 重放返回 RejectionReplayed，不重复计数；
3. 同一 command 的不同 rejection 证据返回 ReceiptConflict；
4. 一个已 Applied 的 command 不能再接受后到的 Rejected；
5. Applied receipt 必须从当前 cursor 的 exact previous state 开始；
6. 首次 Applied 返回 ApplicationRecorded，并把 cursor 推进到 command current state；
7. 同一 command 先 Rejected、后 Applied 时返回 ApplicationRecovered，只推进一次并保留原 rejection 证据；
8. 精确 Applied 重放返回 ApplicationReplayed，不重复推进；
9. 同一 command 的不同 application 证据失败关闭。

Show、Replace、Hide 与 NoOp 都是合法可确认命令。NoOp 虽不要求 renderer mutation，仍推进审计 cursor，因为其 current state/revision 已被 consumer 接受；这避免隐藏 tombstone 或其它无绘制更新在后续命令链中丢失。

## 6. 顺序、幂等与隔离

ledger 以 command previous state 必须匹配当前 cursor 为唯一顺序门。尚未应用 Show 时，未来 Replace 不能跳过；Show 成功后相应 Replace 才能推进。Hide 后可继续接受 hidden NoOp，完整链保持可重放。

RunMismatch 与 ConsumerMismatch 在任何 entry mutation 前返回；CursorMismatch、ReceiptConflict 同样保持 cursor 与计数不变。AppliedCommandIds 保存唯一应用顺序，Entries 按 CommandId 保存至多一份 rejected 与一份 applied receipt。相同输入重复记录不会产生第二次副作用。

## 7. 自动化覆盖

Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommandLedger 新增 8 项：

1. ScopeAndTeardown：空状态、幂等 begin、scope rotation 与 exact teardown；
2. ReceiptDeterminism：receipt identity、outcome 与 consumer 隔离、自校验；
3. ShowApplication：Show 首次应用、cursor 推进与 stored receipt；
4. RejectionRecovery：reject 不推进、reject replay、later apply 恢复；
5. ReplayAndConflict：Applied replay 幂等、alternate evidence 与 late rejection 冲突；
6. OrderAndReplace：未来 Replace 被拒，Show 后 Replace 连续推进；
7. HideAndNoOp：Show、Hide、hidden NoOp 构成连续 audit cursor；
8. RunConsumerRotation：跨 consumer/Run receipt 隔离及 teardown 后新 scope。

专属测试最终为 8/0。候选 build 与专属测试首轮均成功；静态复核后删除了冗余 EmptyCommandId sentinel，并把空 last-applied identity 改为按值返回无效 FGuid。该无语义冗余清理后重新执行候选 build 与专属测试，仍为 8/0；没有失败 build/test 修复轮。完整 0.0.10 为 1053/0。

## 8. Changed-file 回归映射

新增 ThrownWeaponArcPreviewPresentationCommandLedger 规则，覆盖 ledger、presentation command、session、update coordinator、presentation、product bridge、capture、composition、choice projection、product lifecycle/session/controller、run command/host/world delivery/item adapter、Combat Run coordinator、items、world gameplay、combat runtime/core、broad Shanmen.0_0_10 与 legacy ItemUseAndArmor，共 23 个 required groups。

正反 mapping self-test 由 367 增至 369/369。最终 gate 对 5 个实现、测试与流程路径求并集：Changed=5 Rules=2 Required=23 Logs=3，全部具备健康证据。

本轮进一步精简为 3 份证据日志：ledger 专属、legacy ItemUseAndArmor 与完整 Shanmen.0_0_10。完整日志直接覆盖其余映射组，因此无需重复启动 command/session/coordinator 等上游定向组；验证仍由改动路径推导，覆盖强度不降低。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| arc_preview_command_ledger_final.log | Product.ThrownWeaponArcPreviewPresentationCommandLedger | 8/0 | 0253D055A9689DDFABECC57DC8C557E0EBD32BB09C670D24ED53BD8F09C3C6A8 |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | F4A1DD6F8C58362A2F321CE42EFC2AD4A70F71D65F4B2A125BA4F3BECBDC756E |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1053/0 | FBE8727D20D99B6D321E73CFEFBEC321B15B54915CA7860E484ADD9984C262A8 |

正式日志累计 1107/0，包含定向、legacy 与全量重叠；独立完整套件为 1053/0。完整套件在既有 SwordRhythm journal/checkpoint 大型快照区段出现长 tick，UE 将其记录为不惩罚 unresponsive test，进程随后原生完成；没有外层 timeout 或人工中止替代结果。

- automation audit：PASS，3 logs / 1107 success / 0 fail-or-not-run，SHA-256 95CBA2B2764123826442D7A5DE001882648FC7D8895AE1D6196DE664DAE41F92；
- regression self-test：369/369，SHA-256 481ABBE22FD5A878BB1AE9DF03A54F425ECADAE9728EFC8FCDF59956A1CD019D；
- boundary scan：PASS，files 2、lines 848、forbidden 0、deterministic ID factories 2、World/UI include 0、placeholder 0、record entrypoints 1、map rule 1/23 groups，SHA-256 78C3AE2972E433258E5B7D4811227A91C44D172B32B04CBA49A01CCE1AA6CF7A；
- changed-file gate：PASS Changed=5 Rules=2 Required=23 Logs=3，SHA-256 C05E9C407E6DB0406B48C309C829CF699F1F0E1EFD6807A6F8D9682A215A9E47；
- git diff check：PASS，7 个本轮文件、0 个空白错误，SHA-256 13FE89CEEE4B9AB5F58D08E77DA87FC586A6303DC09FD2DF83BC91B6310A26D6；
- staged diff check：PASS，7 个精确暂存文件、0 个未暂存已跟踪文件、103 个历史无关未跟踪文件保持在外、0 个空白错误，SHA-256 B872C65927DAEA927073BB7698AA1E89DBDD0C3D2553C4ADE73C6A4520A6B6FD；
- candidate Editor（静态复核后）：5 actions / native 0 / 19.74 秒，SHA-256 D62EDE693DAA6BED9F770620E487941A0C22EDD1714CD3FA3D4D2FBEE103450C；
- candidate focused ledger（静态复核后）：8/0，SHA-256 6D90C66AF0AA24CB431DF80D0211545CD8352E3393724D92A82DE3A23B4FADBD；
- final Editor：up to date / native 0 / 1.19 秒，SHA-256 AA67061234B587391418750649DE3A623797370827E0FED5104529A8E8B12AFF；
- final Game：4 actions / native 0 / 28.98 秒，SHA-256 03D556E63C8FFC7CF07D1A0B81AE5912F3144DFB9829A4FEAA9DAE24642DC96C。

产物：

- Binaries/Win64/UnrealEditor-demo_map.dll：16,835,584 bytes，SHA-256 3DC416EE6034583BF8AFD1940DFBBAA9D0727E5B5068F411A0A8307F30ACEC5A；
- Binaries/Win64/demo_map.exe：358,021,120 bytes，SHA-256 B63F825E2522D0CDA36125E51D998ECE44B00841DF56BCCD939808F772B7CDDB。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：command receipt 的确定性 identity 与自校验、consumer/Run scope、Applied/Rejected 记录、rejection recovery、exact replay 幂等、conflict 与 out-of-order fail-closed、Show/Replace/Hide/NoOp 连续 cursor、teardown/reset、完整 ledger replay validation，以及完整 0.0.10 与 changed-file 回归。

未验证：真实 renderer/port 调用、HUD/UI presentation、widget/component/visible frame、delivery coordinator、真实设备输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。receipt 只能描述 consumer 声明，不能描述真实视觉验收。

建议 P20.38 建立 renderer-neutral presentation port 与 bounded delivery coordinator：每条 command 最多调用一次抽象 port，把返回值封存为 Applied/Rejected receipt，再交给本 ledger；以 fake port 覆盖成功、拒绝、重放与异常顺序，仍不连接真实 HUD、renderer 或 World。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-37-thrown-weapon-arc-preview-command-ledger>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-37-thrown-weapon-arc-preview-command-ledger/Docs/Report/Dev.D.UE.0.0.10.P20.37.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-37-thrown-weapon-arc-preview-command-ledger/Docs/Log/Dev.D.UE.0.0.10.P20.37.r0_log.md>
