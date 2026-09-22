# Dev.D.UE.0.0.10.P28.34.r1 Development Log

## 1. 基线与范围

- UTC 2026-09-22，分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 父提交/产品验证基线：`d0b4c96870b7ae31ab748932c4a53ab7776148d8`。
- 开始时读取 r0 Report/Log、当前分支、Git 状态、P 阶段基线与冻结索引，执行 r0 Published 验证通过：1623 输入、103 未跟踪用户文件、2 份受保护修改、暂存为空、33 条历史原件 SHA 相符。
- 本次仅三文档交付；[Report](../Report/Dev.D.UE.0.0.10.P28.34.r1_report.md)。没有生产、测试、受追踪脚本或内容变更，没有新增 UE 执行。

## 2. 原后台运行完成

沿 r0 第8节找到唯一 `ShanmenFullRoot/20260922T004752811Z-20fbf70c`，未启动替代进程。run-state 的 executable、参数、证据路径与 PID 51080 一致：`UnrealEditor-Cmd`、`-Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile`、`Automation RunTests Shanmen`、队列为空退出。

原生状态 SUCCEEDED / 0；开始 `2026-09-22T00:47:52.8141731Z`，结束 `2026-09-22T02:20:56.6212991Z`，耗时 5583.807 秒。日志中 1448 条 Success 的完整 Path 也有 1448 个唯一值；0 Fail，精确且唯一的 `Automation Test Queue Empty 1448 tests performed.`；Fatal/Ensure/Unhandled/Assertion 为 0。

原外层会话 18881 已取回，退出码 0；最后输出 UTC 02:20:57.3907560，Success=1448、Fail=0、Queue=1448、Native=0、Crash=0、Inputs=1623、Users=103，与独立复核一致。没有运行中的该验证进程，无需继续轮询或重开长测。

原日志 13 条 Error 全为既存启动 Condition failed；308 条 HTTP request timed out 日志行包含事件转录，不将其当作独立请求计数或未经测量的耗时归因。既存日志不改写、不删除。

## 3. 相同输入的组合证据

复核 r0 Verified 的 Editor/Game SUCCEEDED、原生0/0及 stdout 构建成功，实际 actions 4/3；没有本次重构建。PersistenceFocused 12/0、ItemsRegression 103/0、LegacyFullRoot 1330/0 的唯一成功、队列与原生状态重新核对。完整新旧两根名称互不重叠，总数 2778；103 Items 完整包含于新根，12 专项完整包含于 Items。

Verified 输入锁 1623 项路径集合和全部 SHA 保持：1092 Source、485 Content、40 Scripts、5 Config、1 uproject。本次文档不改变该产品基线。失败历史、旧根/专项/双构建原件的位置与 SHA 沿用并重新核对 [r0 Log](Dev.D.UE.0.0.10.P28.34.r0_log.md)，不冒充新一轮执行。

## 4. 本次检查

实施覆盖重新按 `3a3536c2837ccb3763f9289f3c668666cf8a96c2..d0b4c96870b7ae31ab748932c4a53ab7776148d8` 的八个真实路径推导，读入完成后的 Shanmen 和 demo_map 全根日志：Changed=8、Rules=1、Required=1、Logs=2，必跑 Items 已覆盖。

本次三文档显式路径分类：Changed=3、Rules=0、Required=0、Logs=0，只证明文档分类，不冒充 UE 通过。映射自检重新执行 537/537。交付门另核对本次文档相对链接、原件 SHA、工作区/暂存 diff --check、三路径暂存和提交范围、所有用户文件保持。只采用普通非强制推送，发布后核对远端分支 SHA 与本地 HEAD。

## 5. 本次补齐的原件与 SHA-256

- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/ShanmenFullRoot/20260922T004752811Z-20fbf70c/UnrealEditor.log`，SHA `5AEED25907A5557B2048CFB46FC3440066921F3C14B636A3B2F41B90E2EBF398`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/ShanmenFullRoot/20260922T004752811Z-20fbf70c/run-state.json`，SHA `97430171C6C5B74E097C2E78B28BFC8AF4478E5A8132A08FA83EE297C7B262B3`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.34/validation-inputs-Verified.json`，SHA `A495AADEA4E512204119D3AD67407F574AC80D0FE51F157CD8C0D6F3E68B7156`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.34/FullRootAcceptance/implementation-coverage.log`，SHA `CAC03602FF1ED096089ADDAA4BA1007262AE28A1C572EFD57EE9FA2E84CD7BD4`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.34/FullRootAcceptance/document-coverage.log`，SHA `92637BA9A2DD35B80B09B9D8046527FA74138760EFFB9D7069542C3A7D50A845`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.34/FullRootAcceptance/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。

这些原件在本地 Saved 内，不纳入 Git；GitHub 发布本文中的可核验索引。最终提交身份由 Git 给出，不在自身文档嵌入自引用 SHA。

## 6. 用户文件与精确交付

仅新增 `Docs/Report/Dev.D.UE.0.0.10.P28.34.r1_report.md`、本文，修改 `Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md`。r0 Report/Log 保持原历史内容。原 103 个未跟踪文件的集合/逐项 SHA 保持，不暂存。

两份原用户修改保持并排除提交：

- `Docs/Report/Dev.D.UE.0.0.10.OverallReadiness.r0_report.md`：`3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`。
- `Docs/Log/Dev.D.UE.0.0.10.OverallReadiness.r0_log.md`：`A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。

## 7. 接续条件

P28.34 长测已验收，不再把“仍在后台”当作阻止后续源码工作的条件。下一轮按索引中 FZ-1/2 的最小真实缺口继续，不重复本轮证据整理。可信来源授权/目录接纳/物化、获物/消费/终局组合以及尚未取证的生命周期边界仍待闭合；当前不能宣布全框架冻结或暂停为已完成。

遵守 [P 阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：无物理输入、正式地图/内容、UI/玩法、Editor UI/PIE/Standalone、产品 exe、Smoke/Cook/Package。内存峰值、吞吐和整个 World 跨进程恢复无新增成功声明。
