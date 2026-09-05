# Dev.D.UE.0.0.10.P20.38.r0 Log

## 阶段

- 任务：P20.38 renderer-neutral thrown-weapon Arc preview presentation port 与 bounded delivery coordinator；
- 基线：46269e88031edc989aaf08790083a26270caced9；
- 分支：agent/0.0.10-p20-38-thrown-weapon-arc-preview-delivery-coordinator；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/真实 renderer/可见轨迹/截图/Smoke/Cook/Package。

## 实现记录

1. 新增 immutable renderer-neutral port response。
2. response 绑定 exact CommandId、Applied/Rejected 与 stable outcome code。
3. ResponseId 使用固定命名空间和 canonical parts 确定性派生。
4. response IsValid 重新派生 identity。
5. 新增窄 Idemo_map...PresentationPort 接口。
6. port 只暴露 consumer identity 与 Apply(command)。
7. 新增 stateless bounded delivery coordinator。
8. invalid command 在任何 port 查询/调用前拒绝。
9. invalid/inactive ledger 在 port 调用前拒绝。
10. foreign Run 在读取 port consumer 前拒绝。
11. missing/foreign consumer 在 Apply 前拒绝。
12. future/out-of-order cursor 在 Apply 前拒绝。
13. 所有 preflight rejection 的 PortCallCount 均为 0。
14. 已 Applied CommandId 只重放 existing receipt。
15. 已 Rejected CommandId 只重放 existing receipt，不自动重试。
16. external recovery 后优先重放 Applied receipt。
17. 无历史且有序 command 恰好调用一次 Apply。
18. Applied response 封存 Applied receipt 并推进 cursor。
19. Rejected response 封存 Rejected receipt且不推进。
20. invalid/stale response 封存固定 InvalidPortResponse rejection。
21. response outcome code 原样进入正常 receipt。
22. port 调用前复制 candidate ledger，成功验证后整体提交。
23. ledger commit 异常路径尝试封存固定 rejection。
24. 无法形成终止证据时返回显式 InvariantViolation。
25. result 自校验 status、port call count、response、receipt 与 ledger result。
26. Show、Replace、Hide、NoOp 使用同一 delivery contract。
27. NoOp 不要求绘制 mutation，但仍让 consumer 观察一次 revision。
28. 新增 8 项 fake-port 自动化；完整 0.0.10 总数 1053→1061。
29. changed-file map 新增 delivery 规则与正/反 fixture；self-test 369→371。
30. 不新增产品 mutation、库存、World、Actor、UI、renderer 或 input 权威。
31. 历史无关未跟踪文件保持未暂存、未修改、未删除。

## 修正记录

候选 Editor build 与专属 coordinator 自动化首轮成功。静态复核补充了 InvariantViolation，使理论上的 receipt/ledger 内部失配不会伪装成正常 rejection；同时把两个文件级 FName 常量改为按需构造，避免模块初始化顺序依赖。最终 Editor build 与专属 8/0 覆盖修正后的代码。

正式证据保持 3 日志集合：delivery coordinator 专属、ItemUseAndArmor legacy、完整 0.0.10。完整套件覆盖 24-group 映射中的其余上游组，不重复运行 command/ledger/session 等子组。

完整套件在既有 SwordRhythm retry/journal/checkpoint 大型快照区段出现长 tick；UE 明确记录为不惩罚 unresponsive test，进程持续运行并原生完成 1061/0。没有外层 timeout、强制终止或人工推断成功。

## 最终自动化

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| arc_preview_delivery_coordinator_final.log | 8/0 | F178A40079D0F6EC4B76BE9208ADA825752D587B63DD5C6795591045B35CC7B5 |
| item_use_and_armor_final.log | 46/0 | 34C2900EF73F762A435EB88CD26006751397EA7DC7CDE42372BFEC31BE743E60 |
| full_0_0_10_final.log | 1061/0 | 4D32D50347D05323AB60F4A75B276F59C2F7ED7580C233AE20427BFB9AAA4689 |

正式日志累计 1115/0，包含定向、legacy 与全量重叠；独立完整套件为 1061/0。

## 流程与静态证据

- automation audit：PASS；3 logs / 1115 success / 0 fail-or-not-run；SHA-256 ED326027D8A65A6A9115B2376A7196C96CF03CD8FAD1F0FBBD8EE7A4FD8043FE；
- regression self-test：371/371；SHA-256 A3216731630BCBC7EF635478B0558F04EA6122717F079A68B777861101F4E1C5；
- boundary scan：PASS；2 files / 737 lines / forbidden 0 / deterministic ID factories 1 / World-UI include 0 / placeholder 0 / port entrypoints 2 / map rule 1/24 groups；SHA-256 3FDA92178A2698A349547AAF903B72D20168FBFDA1A134B4142B6A9DDE48B4A2；
- changed-file gate：PASS Changed=5 Rules=2 Required=24 Logs=3；SHA-256 69AE1D57AF00927D1A4BC9D08BEC0A9F9D8D0994A57C9D5DF9FF1AFBCD9B27C5；
- git diff check：PASS；7 个本轮文件、0 个空白错误；SHA-256 13FE89CEEE4B9AB5F58D08E77DA87FC586A6303DC09FD2DF83BC91B6310A26D6；
- staged diff check：PASS；7 个精确暂存文件、0 个未暂存已跟踪文件、103 个历史无关未跟踪文件保持在外、0 个空白错误；SHA-256 B872C65927DAEA927073BB7698AA1E89DBDD0C3D2553C4ADE73C6A4520A6B6FD。

## 构建与产物

- candidate Editor：5 actions / native 0 / 8.17 秒；SHA-256 3F1BB2B37C9DB2AF910B4E2996FA279685EC9E64FFDBFCD36196C81AC1C51823；
- candidate focused coordinator：8/0；SHA-256 217A4E5A0BB402C24E851C3FC73387487DB5F842B30D8A482B73D1C97D64C909；
- final Editor：4 actions / native 0 / 7.52 秒；SHA-256 39D4695BB70C7EC87A9B4C05463A7D15939E975C8A71491E2DDE6F5E0143E1B8；
- final Game：4 actions / native 0 / 34.84 秒；SHA-256 AD16BC4A45750FD886EC3F66C0CA2D655CA156CB6FEB523F6951AEE3C3010C36；
- Editor artifact：16895488 bytes；SHA-256 9AACAA9A4BD70887D18BF735C09BF2A43FFBF3EFB348CB9F8D44E8EB16FA4D0C；
- Game artifact：358070272 bytes；SHA-256 B6F497C56C341F7200F93E375447C503291C9FC5DED00A910A741739F3A69748。

## P/F

PASS：port response identity、自校验、Run/consumer/cursor preflight、zero-call rejection、Applied/Rejected receipt、invalid/stale response fail-closed、CommandId 去重、external recovery replay、Show/Replace/Hide/NoOp 顺序交付与 candidate-ledger 原子提交。

未验证：真实 HUD/renderer/port adapter、widget/component/visible frame、跨线程或进程崩溃后的 exactly-once、World trace/collision/occlusion、真实设备 choice、可见轨迹、Editor UI/PIE/Standalone、产品启动、projectile/launch、inventory reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。

## 下一步

P20.39：建立 consumer-owned delivery session，由 Run-scoped owner 管理 ledger begin/deliver/end，并要求关闭前 cursor hidden 或 empty；继续 fake-port 自动化，不连接真实 HUD/World。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-38-thrown-weapon-arc-preview-delivery-coordinator>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-38-thrown-weapon-arc-preview-delivery-coordinator/Docs/Report/Dev.D.UE.0.0.10.P20.38.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-38-thrown-weapon-arc-preview-delivery-coordinator/Docs/Log/Dev.D.UE.0.0.10.P20.38.r0_log.md>
