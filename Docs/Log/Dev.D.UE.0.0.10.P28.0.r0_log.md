# Dev.D.UE.0.0.10.P28.0.r0 Development Log

## 1. 基线与范围

- 日期：2026-09-16 UTC；heartbeat codex-10 15:32:23.438Z。
- 入场 HEAD：`d62343b3c4ba48d3ba9dc5c97f9cc7569e39ca81`，最近产品实现 P27.31 / `0b6af9c`；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 入场 tracked/index 干净；103 个未跟踪用户文件保存原路径与原字节 SHA-256。另对 1577 个 tracked Source/Config/Content/Plugins/uproject 输入取原字节基线；该数量是本轮选定的产品路径范围，不是 P27.31 的 1131 个 Source/Config/Scripts 清单。
- 本轮只做文档、映射分类和脚本自检，不改产品代码。允许双目标编译，不执行实际游戏性开发。
- 2026-09-16 16:57:54.460Z heartbeat 续交接：HEAD 为 `8c47c6641e17ecd701241fb466b6d6e41e369320`。期间用户总体报告请求形成独立两文档提交，P28.0 十一文件原状保留；续交接只复核与完成发布，不重新构建或再次运行 510 项自检。

## 2. 读取与分析

1. 完整读取 P27.31 Report/Log，确认完成状态、失败和中断的保留方式、当前 P/F 边界；总体报告已经更新，不重复交付该报告。
2. 检查根项目入口、信息卡和旧 I 门禁的当前/历史冲突；读取三份物品 ADR，核对当前 Repository RecordProcessed、只读精确终局比较与服务持久结果类别。
3. 核对 ProfileRepository 的 legacy 判定和 ItemEconomySchema.22/.23：Schema 4/5/6 使用非零 TownLevel/资源；不把旧 ADR 当当前源码事实。
4. 深读 GameMode TryActivateCombatRun、Condition TryBegin/TryEnd、SwordRhythm Session 和 Presentation Controller 的候选创建，确认可疑早期 Reset 不能仅凭关键词判为已复现失败；留下有限可达性核对，不改源码。
5. 检查回归映射与自检入口。两个根文档确实未分类，生成原错误证据；只增加准确文件名匹配及近似路径反例。

## 3. 精确十一文件

1. `PROJECT.md`：当前入口及历史范围。
2. `PROJECT_INFO_CARD.md`：当前卡片及历史范围。
3. `Docs/Process/I_STAGE_FOUNDATION_GATE.md`：旧版本范围说明。
4. `Docs/Process/P_STAGE_BASELINE_0_0_10.md`：已有 P 范围与验证规则的共享基线。
5. `Docs/Architecture/Dev.D.UE.0.0.10_ItemAuthority_ADR.md`：schema 已修状态与路由解释。
6. `Docs/Architecture/Dev.D.UE.0.0.10_ItemTransactions_ADR.md`：拒绝写入与历史阶段边界。
7. `Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md`：契约/权威/生命周期/证据及有限冻结清单。
8. `Scripts/ShanmenRegressionMap.json`：精确根 Markdown 分类。
9. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`：两个正例、四个近似路径反例。
10. 本阶段 Report。
11. 本 Development Log。

## 4. 脚本证据

路径相对 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`，原文件留本地。

| 记录 | 结果 | 原字节 SHA-256 |
|---|---|---|
| `Saved/Automation/P28.0/root-docs-red-exception.log` | 原映射抛出两个 root docs unclassified；这是预期失败复现 | `E929B903636E66EA141B92C948A7AB1C854599014256CE95311DCAA2E4A3C8EA` |
| `Saved/Automation/P28.0/regression-selftest.log` | PASS 510/510（PowerShell 检查器自测，不是 UE 用例） | `00472985AEDDCB36E54C6E40B2239DC18C6CDD0C4941B055C03CAB48C04582B4` |
| `Saved/Automation/P28.0/regression-coverage.log` | PASS Changed=11 Rules=0 Required=0 Logs=0 | `B6257EF5C5620E76596FCA6EBEBE8AF02647CB25EA7BEDB961FBC287BE116080` |
| `Saved/Automation/P28.0/validation-wrapper-note.md` | 两次外层 LASTEXITCODE 错判的原状说明 | `E61EC9FC24EAAFAA6C9BEC3F3B11121C44A9D21A833AE54FFAE3722C9B2DBACF` |

根文档异常由调用者捕获并验证预期错误，原 red 输出文件为空，实际异常记录于上表单独文件。没有把预期失败改写为通过，也未制造 UE 失败日志。

## 5. 新双目标构建

统一入口：`Scripts/Invoke-Shanmen.ps1 -Action BuildBoth -TaskId Dev.D.UE.0.0.10.P28.0.r0.Validation`；串行 MaxParallelActions=1，未启用 UseUba。

| 目标 | 证据目录 | 结果 |
|---|---|---|
| Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.0.r0.Validation/BuildEditor/20260916T153925298Z-acf3597f/` | SUCCEEDED，native 0，5 个链接/元数据 action；UBT 12.09s |
| Game | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.0.r0.Validation/BuildGame/20260916T153938046Z-6c8c5e42/` | SUCCEEDED，native 0，up to date，0 action；UBT 1.46s |

- 两目录原 `run-state.json` 均读回核验，原生 0/0。
- Editor stdout SHA-256：`46899DC8FF264B7C385BDB9CA03B1BA30C948C2D582016C8766883EF9D7F095B`。
- Game stdout SHA-256：`BCDF7D0DD1B6BEC7EAC4F836832F447D4CA253A3BF3A828AB719071CD7B117AA`。
- 聚合输出：`Saved/Automation/P28.0/build-both.log`。Editor 日志说明工作集变化导致 makefile 失效和重新链接；没有编译新的产品源码，不把链接动作当作源码变化。

## 6. 外层判定错误与恢复

初次脚本执行后，外层使用 `if($LASTEXITCODE -ne 0)` 检查 PowerShell 脚本，分别抛出 `Self-test failed` / `BuildBoth failed`，两个外层命令退出 1。它们不承诺设置调用者的 LASTEXITCODE，不能这样推断结果。

未删改或覆盖原日志；后续只读取 self-test 最终 510/510 标记、构建 stdout/原生 run-state 并验证，独立核验命令正常退出 0。原生构建真实成功与外层调用真实失败分开记录；没有重复编译或将异常归咎于源码/环境。

## 7. 产品回归与冻结边界

本轮没有生产路径变动，所以映射不要求 UE 组；它不意味着免验证脚本，自检 510 项均执行。最近完整 UE 证据仍为 [P27.31 Log](Dev.D.UE.0.0.10.P27.31.r0_log.md) 的新根 1419/0、旧根 1330/0，本轮未重跑、不累计、不冒充最终冻结。

只形成 FZ-1 权威路由、FZ-2 激活/释放、FZ-3 最终验证三项有限清单。没有证据的可疑分支不称为故障，不因缺少真实 UI/游戏表现而扩张当前 P 实现范围。

## 8. 发布检查与下一步

续交接检查已完成：

- 完整读取本阶段 Report/Log、共享 P 基线并审核七个 tracked 差异；精确范围仍为第 3 节十一文件，未纳入总体 Report/Log。
- 原错误、自检、映射、外层调用说明共四份原件 SHA-256 重新计算，均匹配第 4 节；两个构建原 run-state 均为 SUCCEEDED/native 0，stdout SHA-256 均匹配第 5 节。当前映射及自检脚本原字节与验证后的保存基线一致。
- 九份阶段 Markdown 的 60 个本地链接目标全部存在，映射 JSON 可解析；`git diff --check` 通过。两个根入口的原首标题之外历史正文，经标准化换行后逐字尾部比较保持；没有删除旧文档或旧材料。
- 1,577 个产品输入原字节和 103 个原用户文件的路径集合、SHA-256 与本阶段初始基线一致。原用户文件不暂存；本地 Saved 证据不上传。
- 续交接前本地及远端 HEAD 均为 `8c47c6641e17ecd701241fb466b6d6e41e369320`，index 为空。只精确暂存本阶段十一文件并普通推送，不强推；换行策略提示不作为验证失败。

精确交接第 3 节十一文件；Saved 证据不上传。下一轮先针对 FZ-1/2 取证，能证明已有约束足够就关闭；只有真实缺口才修复。两项关闭后执行最终冻结验证并暂停，而非新增玩法。

未进行物理输入、Editor UI、PIE、Standalone、游戏程序、正式地图/资产、玩法/数值/敌人/UI、截图、Smoke、Cook 或 Package。

- [Report](../Report/Dev.D.UE.0.0.10.P28.0.r0_report.md)
- [闭合索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
