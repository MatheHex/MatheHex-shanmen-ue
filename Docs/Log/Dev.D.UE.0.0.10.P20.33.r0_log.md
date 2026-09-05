# Dev.D.UE.0.0.10.P20.33.r0 Log

## 阶段

- 任务：P20.33 thrown-weapon Arc preview renderer-agnostic presentation；
- 基线：804b776d84b86456440623bc3042d8e746d1d2fc；
- 分支：agent/0.0.10-p20-33-thrown-weapon-arc-preview-presentation；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/可见轨迹/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 renderer-neutral presentation segment、state、project result 与 reduce result。
2. Projector 只消费 captured P20.32 bridge 与 immutable source basis。
3. Projector 使用冻结 choice/config 调用既有 composition 恰好一次。
4. composition 后再次校验 config、choice、Run、player、item 与 activation identity。
5. 相邻 preview positions 投影为有序 line segments，不执行 draw。
6. SegmentId 使用独立 r1 namespace、source preview、index 与坐标 bit pattern。
7. PresentationStateId 绑定 mode、scope、choice revision 与完整可见来源/线段身份。
8. visible state 自验证每段 positions[i]→positions[i+1]、apex、landing、flight time 与 source action scope。
9. reducer 支持 first install、duplicate、strict newer revision replace。
10. stale、same-revision conflict 与 foreign Run/player/item scope 均 fail-closed。
11. clear 只接受严格较新的非 preview choice，生成 hidden tombstone。
12. tombstone 保留 scope/choice revision，清空全部旧 preview identity 与 geometry。
13. tombstone 可阻止清除前迟到的旧异步 preview，并允许更高修订 retarget。
14. presentation 不持有 mutable product/inventory/Run/World/UI/renderer authority。
15. 新增 8 项自动化；完整 0.0.10 总数 1013→1021。
16. changed-file map 新增 presentation 规则与正/反 fixture；self-test 359→361。
17. 103 个历史无关未跟踪文件保持未暂存、未修改、未删除。

## 修正记录

候选 Editor build 首轮成功，5 actions / native 0 / 26.09 秒。专属 presentation 自动化候选首轮 8/0；未发生生产代码或 fixture 修正轮。

## 最终自动化

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| arc_preview_presentation_final.log | 8/0 | 437FBDE5842C8E73611EE1199D8B0175A8BB6068E230BC837F58975E7D805E92 |
| arc_preview_product_bridge_final.log | 7/0 | D518BAEBB7104E44138BBD5F1852B82505DEC62E40499BAF25EFECA87326AB6F |
| arc_preview_capture_final.log | 8/0 | 31F12420E5DFF721EACA9E3F8BE3ABFB54976C404AAECD468D8F73933E2D0014 |
| arc_preview_composition_final.log | 7/0 | AA55BE7D315A4747CF6DE13E3B32D0A752F7E1F2EE7A169C7D641C71DA5FED03 |
| thrown_product_lifecycle_final.log | 5/0 | BAE98433FCF68C8D77A9C272FFE059D7D943742596D070AC39E7CB2A0AFA3E06 |
| thrown_product_session_final.log | 6/0 | 93F249FCEA515BFDC90C9B0378F50AF5CF65C065D4A25117196DE3080C3DA562 |
| item_use_and_armor_final.log | 46/0 | 5BC229A22905D469716E0DCD39230BC47A14212B48668340FA56F83569F637D7 |
| full_0_0_10_final.log | 1021/0 | 7D8AB6D4BE7CE49619F25A0A601BB0741CE6A8CF99D05E53CB61A5D41C3FE708 |

最终证据合计 1108/0。每份日志均有唯一 RunTests、零 fail/not-run、唯一成功终止标记与零 fatal/unhandled/ensure/no-match。

## 流程与静态证据

- automation audit：PASS；SHA-256 F5CD021171FD9379BA06E57159AC91CA329843954A045B9B7F85151FD5EE08A7；
- regression self-test：361/361；SHA-256 9F50B3821680D59169C35A47206FB4C75C87924E2D6FDF9C4FC432365607C249；
- boundary scan：PASS；presentation files 2、lines 815、forbidden mutation/World call 0、composition call 1、mutable authority parameter 0、World/UI include 0、placeholder 0；SHA-256 872D0308EC904FB3E63A2617A62A84DD07D9634EA531ACCDD3EB704DEDA53B95；
- changed-file gate：PASS Changed=5 Rules=2 Required=19 Logs=8；SHA-256 2DCE69448BE743B98442A82C592238172C3A0C5B5AF52705003C442EB75E3BFE；
- staged diff check：PASS；7 个精确暂存文件、103 个历史未跟踪文件保持在外、0 个空白错误；SHA-256 751C42BD50172C022C81076AD8BB1E43A8E83B41C7D38EBD9D924D1984DDD1EF。

## 构建与产物

- candidate Editor：5 actions / native 0 / 26.09 秒；
- final Editor：up to date / native 0 / 1.94 秒；SHA-256 328A988F7B10F1DD7AD6A0484AFF9A20DCD724D122AE325F6626301703714C88；
- final Game：4 actions / native 0 / 33.37 秒；SHA-256 C871E5E55F5471147A6E9B720803A7B833E9C8C5B303FE133EDFC3E2E58AC41C；
- Editor artifact：16,640,512 bytes；SHA-256 8DF1F7CE3A3580A65526ED802E524961823565DE6DB01D23C063A769B9739871；
- Game artifact：357,857,792 bytes；SHA-256 A659B1E8988AC2DEB5FF65CBE090A27541866FA0D81D0FBD6E47DEF0A19E210D。

## P/F

PASS：captured live product preview 到 renderer-neutral immutable snapshot、deterministic segment/state identity、source/scope cross-validation、revision replace/duplicate/clear tombstone/stale fencing、typed rejection、产品 authority 不变，以及完整 changed-file 回归。

未验证：HUD/UI、真实 renderer、World trace/collision/occlusion、真实地形落点、真实设备 choice、可见轨迹、Editor UI/PIE/Standalone、产品启动、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

## 下一步

P20.34：建立 renderer-independent Arc preview update coordinator，把 live capture/project/replace-or-clear 组合成一个 bounded transition；仍不接真实 HUD 或 World renderer。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-33-thrown-weapon-arc-preview-presentation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-33-thrown-weapon-arc-preview-presentation/Docs/Report/Dev.D.UE.0.0.10.P20.33.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-33-thrown-weapon-arc-preview-presentation/Docs/Log/Dev.D.UE.0.0.10.P20.33.r0_log.md>
