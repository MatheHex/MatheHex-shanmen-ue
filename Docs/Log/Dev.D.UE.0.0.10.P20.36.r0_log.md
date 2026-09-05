# Dev.D.UE.0.0.10.P20.36.r0 Log

## 阶段

- 任务：P20.36 renderer-neutral thrown-weapon Arc preview presentation command；
- 基线：de59a9cf6ec040c1f2abe7bb6b1117ab23fe7978；
- 分支：agent/0.0.10-p20-36-thrown-weapon-arc-preview-presentation-command；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/真实 renderer/可见轨迹/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand。
2. command kind 固定为 Show、Replace、Hide、NoOp；Invalid 不可执行。
3. Show 表示 non-visible 到 visible。
4. Replace 表示同 scope visible state 的严格新 revision。
5. Hide 表示 visible 到 non-visible。
6. NoOp 覆盖 empty、exact duplicate 与 hidden tombstone advance。
7. RequiresRenderMutation 只接受 Show、Replace、Hide。
8. command 保留 RunId、previous state 与 current state。
9. CommandId 从 kind、Run、previous/current state identity 确定性派生。
10. IsValid 重新派生 CommandId，不能只信调用方填入的 identity。
11. previous/current state 必须为空或属于 command Run。
12. 双有效 state 必须保持 player 与 source item scope。
13. 非 duplicate transition 必须保持严格 revision 单调性。
14. IsValid 重新分类 transition 并核对声明 kind。
15. 新增纯 Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector。
16. projector 只消费 P20.35 session result，不读取产品 authority。
17. invalid session result 与 rejected update 不产生 command。
18. typed status 区分 SessionResultInvalid、SessionUpdateRejected、TransitionRejected、CommandRejected 与 Projected。
19. project result 交叉验证 source result、command、Run 与 previous/current states。
20. 相同 accepted result 重放产生相同 command identity。
21. 不同 Run 的相同视觉组合仍产生隔离 identity。
22. 新增 8 项自动化；完整 0.0.10 总数 1037→1045。
23. changed-file map 新增 command 规则与正/反 fixture；self-test 365→367。
24. 不新增产品 mutation、库存、World、Actor、UI、renderer 或 input 权威。
25. 历史无关未跟踪文件保持未暂存、未修改、未删除。

## 修正记录

候选 Editor build 首轮成功，5 actions / native 0 / 23.25 秒。专属 command 自动化候选首轮 8/0；未发生生产代码或 fixture 修正轮。

正式证据使用改动驱动的 5 日志集合：三个直接契约组、一个 legacy 组、一个完整套件。完整套件直接覆盖其余 22-group 映射需求，因此没有重复启动 product bridge、capture、composition 等已被完整日志覆盖的定向组。

完整套件在既有 SwordRhythm journal/checkpoint 大型快照区段出现多次约 100 秒的长 tick；UE 明确记录为不惩罚 unresponsive test，进程持续响应，随后自然完成 1045/0。没有以外层 timeout 或人工中止替代原生结果。

## 最终自动化

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| arc_preview_presentation_command_final.log | 8/0 | C176CB917EEB38B452A9AE8EB7CFCE6BD7394191BC164FDA2B2F9D67543A81F1 |
| arc_preview_presentation_session_final.log | 8/0 | AF1DFBE97DBA7592D1EE6F03DB5ED60E8F8DC578B32A41D85F7A2A97E14232B9 |
| arc_preview_update_coordinator_final.log | 8/0 | 156E7DC21C345F9A4E74BE7949E7D415EE603265BE3CDE10B58259099EBB8CF0 |
| item_use_and_armor_final.log | 46/0 | 5C703A703DDC406F3E889E6D5F191AFB853A7410E1B40F3B3BA5BD4B46EFB68B |
| full_0_0_10_final.log | 1045/0 | 2CC1D297E967E9D2952A147D2F31FD2AC37ED55C3C562ED3809EE917D1A86DB8 |

正式日志累计 1115/0，包含定向与全量重叠；独立完整套件为 1045/0。

## 流程与静态证据

- automation audit：PASS；5 logs / 1115 success / 0 fail-or-not-run；SHA-256 6E117704F00C8F4F2A912C3E10A948F04C3238F03B380B8906FE1C85824B87D5；
- regression self-test：367/367；SHA-256 3F5F2B02F5B82F4E8A26F5292C131CAA774E050F9741300F9D94AEC8244FFD82；
- boundary scan：PASS；2 files / 452 lines / forbidden 0 / deterministic ID factory 1 / World-UI include 0 / placeholder 0 / state fields 2 / map rule 1/22 groups；SHA-256 B0CB5385B5074FF2BE1E15F146DCA7DBE36E6F37CE5D1DF3E357D752AF211A74；
- changed-file gate：PASS Changed=5 Rules=2 Required=22 Logs=5；SHA-256 99E85903F338A80C71E1087E3C317F760EBE8467D880719FFAF775DE50300639；
- git diff check：PASS；7 个本轮文件、0 个空白错误；SHA-256 13FE89CEEE4B9AB5F58D08E77DA87FC586A6303DC09FD2DF83BC91B6310A26D6；
- staged diff check：PASS；7 个精确暂存文件、0 个未暂存已跟踪文件、103 个历史无关未跟踪文件保持在外、0 个空白错误；SHA-256 B872C65927DAEA927073BB7698AA1E89DBDD0C3D2553C4ADE73C6A4520A6B6FD。

## 构建与产物

- candidate Editor：5 actions / native 0 / 23.25 秒；SHA-256 B9B39768C2A3040BF227ABA3236AD2D3E5572AFBC3E3099E8BA720F96C435CFE；
- candidate focused command：8/0；SHA-256 D0ADDB16BA74C2B0B50E3C494FAA545AC0929A5B40BB5264DB73466870DC0A09；
- final Editor：up to date / native 0 / 1.77 秒；SHA-256 3E1343B29B8CB84B316352AE75620114E9E365635C495D4D1CE976878F3C4D34；
- final Game：4 actions / native 0 / 31.81 秒；SHA-256 1AD7C9098102708AE8D5B3027471AEF7DFC6A8B1CC5E32D001666030856262CA；
- Editor artifact：16,758,272 bytes；SHA-256 DC5B0103590EC05BEF8D9FE91E3DB3BC7C0D2A305B77DED1DBD967EF4C7276B6；
- Game artifact：357,956,608 bytes；SHA-256 D6EA2F25825384FCA3E9595A7560129995FDB94D6BFDCF6E117FDC0F683757E6。

## P/F

PASS：accepted session result 到 Show/Replace/Hide/NoOp 的纯投影、确定性 identity、Run/item/player scope、strict revision、duplicate/hidden NoOp、reject-no-command、result self-validation、renderer mutation 分类，以及完整 changed-file 回归。

未验证：HUD/UI、真实 renderer、命令消费 acknowledgement、World trace/collision/occlusion、真实地形落点、真实设备 choice、可见轨迹、Editor UI/PIE/Standalone、产品启动、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

## 下一步

P20.37：建立 consumer-owned command acknowledgement/cursor ledger，按 CommandId 去重并记录 renderer port applied/rejected receipt；保持 Run fence，仍不连接真实 HUD、renderer 或 World。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-36-thrown-weapon-arc-preview-presentation-command>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-36-thrown-weapon-arc-preview-presentation-command/Docs/Report/Dev.D.UE.0.0.10.P20.36.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-36-thrown-weapon-arc-preview-presentation-command/Docs/Log/Dev.D.UE.0.0.10.P20.36.r0_log.md>
