# Dev.D.UE.0.0.10.P20.36.r0 Report

## 1. 结论

P20.36 已在 P20.35 consumer-owned Arc preview presentation session result 之上建立 renderer-neutral presentation command。每个有效且已接受的 session result 被纯投影为 Show、Replace、Hide 或 NoOp；拒绝结果不会产生可执行命令。未来 HUD/renderer 只需消费明确视觉差量，不必重新推断 previous/current state 的含义。

命令携带确定性 CommandId、RunId、previous state 与 current state，并自校验 Run scope、item/player scope、严格 revision 与 transition kind。可见 geometry 首次出现为 Show，严格更新为 Replace，清除为 Hide；empty、精确重复与 hidden tombstone revision 推进均为 NoOp。NoOp 可以推进审计身份，但明确不要求 renderer mutation。

新增 8 项自动化，完整 0.0.10 由 1037 增至 1045/0。按 changed-file 映射执行 5 份正式日志，累计执行记录 1115/0；累计数包含定向日志与全量日志的有意重叠。映射 self-test 为 367/367，Editor 与 Game Development 构建均成功。

本轮没有连接 HUD/UI、World renderer、Actor/组件、真实设备输入、trace、collision、projectile、launch、库存 reserve/consume 或 Impact。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，未执行截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：de59a9cf6ec040c1f2abe7bb6b1117ab23fe7978（P20.35）；
- 分支：agent/0.0.10-p20-36-thrown-weapon-arc-preview-presentation-command；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Renderer-neutral command contract

新增 Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand：

1. Show：previous state 不可见、current state 可见；
2. Replace：previous/current 均可见，且 current 是同 scope 的严格新 revision；
3. Hide：previous 可见、current 不可见；
4. NoOp：视觉状态无需改变，包括 empty、精确 duplicate 与 hidden tombstone advance；
5. Invalid 永远不能成为有效命令；
6. RequiresRenderMutation 仅对 Show、Replace、Hide 返回真；
7. command 不拥有 renderer、widget、component、Actor、World、timer、input 或产品 authority。

命令同时保留 previous/current presentation state，使消费端获得完整差量语义。Hide 保留被清除的 previous visible state；Show/Replace 的 current state 保留可见 geometry；NoOp 仍可审计，但不得诱导重复绘制。

## 4. Deterministic identity 与 scope

CommandId 使用固定命名空间及以下 canonical parts 派生：command kind、RunId、previous presentation state identity 或 EMPTY、current presentation state identity 或 EMPTY。没有随机 GUID。

IsValid 重新派生并核对 CommandId，同时要求：

- RunId 有效；
- 非空 state 自身有效且属于同一 Run；
- previous/current 同时有效时，player 与 source item scope 精确相同；
- 非 duplicate transition 的 current choice revision 严格大于 previous revision；
- 实际 previous/current 组合重新分类后必须等于声明的 command kind。

相同 accepted session result 重放产生相同 CommandId；Run 变化必然改变命令身份。命令不能跨 Run、跨 player 或跨 item 伪装为同一视觉操作。

## 5. Projection 与拒绝边界

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector 是纯函数式投影器，只接受 P20.35 session result：

1. 无效 session result 返回 SessionResultInvalid；
2. 有效但未接受的 session update 返回 SessionUpdateRejected；
3. previous/current 不能形成合法视觉 transition 时返回 TransitionRejected；
4. 投影后命令不能自校验时返回 CommandRejected；
5. 只有 Projected 同时携带一个有效 command。

project result 保留完整 source session result、typed status、diagnostic 与 command。IsValid 会交叉验证拒绝/成功状态、嵌套结果、Run scope 以及命令中的 previous/current state，不能仅凭枚举值宣称成功。

## 6. NoOp 与清除语义

NoOp 区分“业务更新已接受”和“视觉无须 mutation”。empty 到 empty 不产生 Show/Hide；同一可见 state 的 duplicate 不产生 Replace；hidden tombstone 的严格新 revision 仍可形成新的确定性 NoOp command，但不会重新触发 renderer 清除。

Hide 只在 previous visible 且 current non-visible 时成立。这样消费端无需读取产品 lifecycle 或 choice 类型来猜测是否清除，也不会因产品 shutdown 后的合法 teardown 再次请求 geometry。

## 7. 自动化覆盖

Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommand 新增 8 项：

1. Rejection：invalid/rejected session result 不产生 command；
2. Show：首次 visible state 投影为 Show，且重放身份稳定；
3. Replace：同 scope 严格新 revision 投影为 Replace；
4. Hide：visible 到 cleared state 投影为 Hide；
5. EmptyNoOp：empty 到 empty 为 NoOp；
6. DuplicateNoOp：精确重复 visible state 为 NoOp；
7. HiddenAdvanceNoOp：hidden tombstone revision 推进仍为无绘制 NoOp；
8. RunIdentity：相同输入可重放，不同 Run 的 CommandId 隔离。

专属测试首轮即为 8/0，没有生产代码或 fixture 修正轮。完整 0.0.10 为 1045/0。

## 8. Changed-file 回归映射

新增 ThrownWeaponArcPreviewPresentationCommand 规则，覆盖 command、session、update coordinator、presentation、product bridge、capture、composition、choice projection、product lifecycle/session/controller、run command/host/world delivery/item adapter、Combat Run coordinator、items、world gameplay、combat runtime/core、broad Shanmen.0_0_10 与 legacy ItemUseAndArmor，共 22 个 required groups。

正反 mapping self-test 由 365 增至 367/367。最终 gate 对 5 个实现、测试与流程路径求并集：Changed=5 Rules=2 Required=22 Logs=5，全部具备健康证据。

本轮采用改动驱动的精简证据集：command、session、update coordinator 三组直接契约日志，ItemUseAndArmor legacy 日志，以及一份完整 Shanmen.0_0_10 日志。完整日志覆盖其余 required groups，因此不再为每个上游组重复启动同一测试；覆盖强度不降低，日志数量由 P20.35 的 10 份降为 5 份。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| arc_preview_presentation_command_final.log | Product.ThrownWeaponArcPreviewPresentationCommand | 8/0 | C176CB917EEB38B452A9AE8EB7CFCE6BD7394191BC164FDA2B2F9D67543A81F1 |
| arc_preview_presentation_session_final.log | Product.ThrownWeaponArcPreviewPresentationSession | 8/0 | AF1DFBE97DBA7592D1EE6F03DB5ED60E8F8DC578B32A41D85F7A2A97E14232B9 |
| arc_preview_update_coordinator_final.log | Product.ThrownWeaponArcPreviewUpdateCoordinator | 8/0 | 156E7DC21C345F9A4E74BE7949E7D415EE603265BE3CDE10B58259099EBB8CF0 |
| item_use_and_armor_final.log | demo_map.ItemUseAndArmor | 46/0 | 5C703A703DDC406F3E889E6D5F191AFB853A7410E1B40F3B3BA5BD4B46EFB68B |
| full_0_0_10_final.log | Shanmen.0_0_10 | 1045/0 | 2CC1D297E967E9D2952A147D2F31FD2AC37ED55C3C562ED3809EE917D1A86DB8 |

正式日志累计 1115/0，包含定向与全量重叠；独立完整套件为 1045/0。

- automation audit：PASS，5 logs / 1115 success / 0 fail-or-not-run，SHA-256 6E117704F00C8F4F2A912C3E10A948F04C3238F03B380B8906FE1C85824B87D5；
- regression self-test：367/367，SHA-256 3F5F2B02F5B82F4E8A26F5292C131CAA774E050F9741300F9D94AEC8244FFD82；
- boundary scan：PASS，files 2、lines 452、forbidden mutation/World call 0、deterministic ID factory 1、World/UI include 0、placeholder 0、state fields 2、map rule 1/22 groups，SHA-256 B0CB5385B5074FF2BE1E15F146DCA7DBE36E6F37CE5D1DF3E357D752AF211A74；
- changed-file gate：PASS Changed=5 Rules=2 Required=22 Logs=5，SHA-256 99E85903F338A80C71E1087E3C317F760EBE8467D880719FFAF775DE50300639；
- git diff check：PASS，7 个本轮文件、0 个空白错误，SHA-256 13FE89CEEE4B9AB5F58D08E77DA87FC586A6303DC09FD2DF83BC91B6310A26D6；
- staged diff check：PASS，7 个精确暂存文件、0 个未暂存已跟踪文件、103 个历史无关未跟踪文件保持在外、0 个空白错误，SHA-256 B872C65927DAEA927073BB7698AA1E89DBDD0C3D2553C4ADE73C6A4520A6B6FD；
- final Editor：up to date / native 0 / 1.77 秒，SHA-256 3E1343B29B8CB84B316352AE75620114E9E365635C495D4D1CE976878F3C4D34；
- final Game：4 actions / native 0 / 31.81 秒，SHA-256 1AD7C9098102708AE8D5B3027471AEF7DFC6A8B1CC5E32D001666030856262CA。

产物：

- Binaries/Win64/UnrealEditor-demo_map.dll：16,758,272 bytes，SHA-256 DC5B0103590EC05BEF8D9FE91E3DB3BC7C0D2A305B77DED1DBD967EF4C7276B6；
- Binaries/Win64/demo_map.exe：357,956,608 bytes，SHA-256 D6EA2F25825384FCA3E9595A7560129995FDB94D6BFDCF6E117FDC0F683757E6。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：accepted session result 到 Show/Replace/Hide/NoOp 的纯投影、确定性 CommandId、Run/item/player scope、严格 revision、duplicate 与 hidden tombstone NoOp、reject-no-command、source/result/command 交叉校验、renderer mutation 分类，以及完整 0.0.10 与 changed-file 回归。

未验证：HUD/UI presentation、真实 renderer 或可见轨迹、命令消费确认、真实设备输入、World trace/collision/occlusion、真实地形落点、Editor UI/PIE/Standalone、产品可执行文件、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。Development 构建与无头自动化不能描述为可见产品验收。

建议 P20.37 建立 consumer-owned presentation command acknowledgement/cursor ledger：按 CommandId 去重，记录 renderer port 对 Show/Replace/Hide/NoOp 的 applied/rejected receipt，并保持 Run fence；仍不接真实 HUD、renderer 或 World。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-36-thrown-weapon-arc-preview-presentation-command>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-36-thrown-weapon-arc-preview-presentation-command/Docs/Report/Dev.D.UE.0.0.10.P20.36.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-36-thrown-weapon-arc-preview-presentation-command/Docs/Log/Dev.D.UE.0.0.10.P20.36.r0_log.md>
