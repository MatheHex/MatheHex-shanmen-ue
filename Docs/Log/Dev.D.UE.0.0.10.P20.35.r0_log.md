# Dev.D.UE.0.0.10.P20.35.r0 Log

## 阶段

- 任务：P20.35 consumer-owned thrown-weapon Arc preview presentation session；
- 基线：2e62553d23e20c1abab7e06e84755224b5ed7867；
- 分支：agent/0.0.10-p20-35-thrown-weapon-arc-preview-presentation-session；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/可见轨迹/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession。
2. 一个 consumer session 显式绑定且只绑定一个有效 Run。
3. 同 Run Begin 幂等；active session 拒绝 silent Run rotation。
4. session 在内存中独占保存一个 presentation state。
5. visible Arc path 预检 lifecycle/coordinator 存活、有效与精确 Run identity。
6. visible preflight 拒绝时 P20.34 调用计数为 0，已有 state 不变。
7. 每次 TryUpdate 最多调用一次 P20.34 bounded update coordinator。
8. 只有 completed update 且候选 state 属于 session Run 时才原子提交。
9. update rejection 与 defensive state rejection 都保留 previous state。
10. 非 preview/no-op/clear 路径不要求产品仍存活。
11. 产品 shutdown 后仍能 Clear stale preview，并对重复 clear 去重。
12. TryEnd 使用 expected Run fence；错误 Run 不得清理状态。
13. Reset 无条件丢弃 Run 与 state；后续 update 必须先重新 Begin。
14. 新 Run 不继承旧 geometry、state identity 或 hidden tombstone。
15. 返回结果记录 session Run、previous/current state、P20.34 result 与 0/1 调用预算。
16. typed status 区分 inactive、invalid、product unavailable、Run mismatch、update/state rejection、applied 与 no-change。
17. IsValid 交叉验证 status、scope、嵌套结果、调用预算与状态原子性。
18. lifecycle 与 Combat Run coordinator 参数均为 const reference。
19. 不新增产品 mutation、库存、World、Actor、UI、renderer 或 input 权威。
20. 新增 8 项自动化；完整 0.0.10 总数 1029→1037。
21. changed-file map 新增 session 规则与正/反 fixture；self-test 363→365。
22. 103 个历史无关未跟踪文件保持未暂存、未修改、未删除。

## 修正记录

候选 Editor build 首轮成功，5 actions / native 0 / 25.03 秒。专属 session 自动化候选首轮 8/0；未发生生产代码或 fixture 修正轮。

正式批量启动命令在创建 UE 进程前被本机命令策略拒绝，未生成任何正式日志，也未构成产品测试失败。随后改为逐组原生启动，10 份正式日志全部成功。

## 最终自动化

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| arc_preview_presentation_session_final.log | 8/0 | 7E9D563F9188BCF805E9C97C53768D69AB4562743C1728074D5CB3E38B781CB1 |
| arc_preview_update_coordinator_final.log | 8/0 | 199E69CE9979CECE3FC6A4C070321DC42185FDB9BCB9CF2BE3D76DFE05BA6E8C |
| arc_preview_presentation_final.log | 16/0 | 725D7E82B77A690584A942061FE8DD726994BE3A9185C44BBB9918873FE1FC0C |
| arc_preview_product_bridge_final.log | 7/0 | A8C265F34274875F13A9ACE81E5C3732F867536C841A7F719140B7A8A1E3695B |
| arc_preview_capture_final.log | 8/0 | 0DDDB0426D2B91BCA49B4B500FA1AF927DBD4FF6F80B3E3BF33F2CB0A72E597D |
| arc_preview_composition_final.log | 7/0 | 0B4670B8E24819FFEB120ACF76C012BF8947B4C011BF5FDF823583205124180A |
| thrown_product_lifecycle_final.log | 5/0 | F4128A6039B4D0D402B0D8857CE53778FEBC3914B326B824EC02DAEE63DC8978 |
| thrown_product_session_final.log | 6/0 | D7418A097BFC4B24F1C0538F0258B82B3CC1DED903B821C58A6CDC482FBA3A44 |
| item_use_and_armor_final.log | 46/0 | 52798154694CEF85AF7DE320717E704E885BA6CAC908F1064438968950232D38 |
| full_0_0_10_final.log | 1037/0 | 8C073C2BD79B9AA335DA51461A8CEB53BCF5453AEACF29F5335F77766A4A5E95 |

正式日志累计 1148/0，包含定向与全量重叠。`ArcPreviewPresentation` 的 UE 前缀过滤同时命中 8 项原 presentation 与 8 项新 PresentationSession，因此该日志精确为 16/0；独立完整套件为 1037/0。

## 流程与静态证据

- automation audit：PASS；10 logs / 1148 success / 0 fail-or-not-run；SHA-256 3C21EE13471C114038922F661DC79ECACF6052A5A5C43115C47FFCEE69D1B6AD；
- regression self-test：365/365；SHA-256 A39BAE64E4C1B42AF6873EEC1D352CE598B2060593FEBD3F2C90114152646C35；
- boundary scan：PASS；2 files / 467 lines / forbidden mutation-or-World calls 0 / P20.34 calls 1 / mutable authority params 0 / World-UI includes 0 / placeholders 0 / consumer Run-state pair 1；SHA-256 E2AB26C4B9B19BC06BC10579CBE3EC0DF60FF8E5B25C5C66E8A9EFE5CC3049F8；
- changed-file gate：PASS Changed=5 Rules=2 Required=21 Logs=10；SHA-256 E0F090AE36ECF3DC632F5A4C37B30EBA9E97DB2078AEEAA00E2EB5D20AFF801F；
- git diff check：PASS；SHA-256 6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9；
- staged diff check：PASS；7 个精确暂存文件、103 个历史无关未跟踪文件保持在外、0 个空白错误；SHA-256 ECE56D3FABEC3E7EF165397F82E4306AA6D8D2615C57C038F229BFB396D17B77。

## 构建与产物

- candidate Editor：5 actions / native 0 / 25.03 秒；
- candidate focused session：8/0；
- final Editor：up to date / native 0 / 1.18 秒；SHA-256 0BE8C41AE8EC1843D0F3F36C15801CD6411F6272C17AF3DDA4CD532C8A3CD8B8；
- final Game：4 actions / native 0 / 28.37 秒；SHA-256 F1FC9219123C085C52852B003795D58DD2980A9586F9777467C9692A1D9B7F15；
- Editor artifact：16,717,824 bytes；SHA-256 5D1CA176E1689AF9270956BFE44D916D7099555CB7455EB150BB7068E8163170；
- Game artifact：357,921,792 bytes；SHA-256 AD4299F5A17118388C45941812935ECBC2393DF876BCDE8D9187F37E8BA0064F。

## P/F

PASS：consumer Run binding、bounded update、completed-only atomic commit、rejection state preservation、foreign/unavailable source fence、product-independent clear、shutdown teardown、duplicate/revision、exact End/Reset/new-Run isolation、产品 authority 不变，以及完整 changed-file 回归。

未验证：HUD/UI、真实 renderer、World trace/collision/occlusion、真实地形落点、真实设备 choice、可见轨迹、Editor UI/PIE/Standalone、产品启动、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

## 下一步

P20.36：在 session result 上建立 renderer-neutral Show/Replace/Hide/NoOp presentation delta/command，作为未来 HUD renderer 的明确消费端口；仍不连接真实 UI、World 或 renderer。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-35-thrown-weapon-arc-preview-presentation-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-35-thrown-weapon-arc-preview-presentation-session/Docs/Report/Dev.D.UE.0.0.10.P20.35.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-35-thrown-weapon-arc-preview-presentation-session/Docs/Log/Dev.D.UE.0.0.10.P20.35.r0_log.md>
