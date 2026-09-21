# Dev.D.UE.0.0.10.P28.31.r0 Development Log

状态：`P28_31_DOMAIN_CONTRACT_COMPLETE`，仅领域候选/精确重放契约通过，持久接收与产品路由未接入。日期2026-09-21 UTC。

## 1. 起点与范围

21:24:51.866UTC heartbeat；分支 agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `8b00869fec92f1c78029406faa8c33a8b65067e3`。21:25:11.6097345UTC P28.30 Published 核验通过；读取最新Report/Log、P阶段基线、有限冻结索引、Items/旧来源类型和实际调用契约，无进行中的验证。

P28.30之后只新增 ShanmenItemGeneratedSource.h/.cpp 和对应测试，共372行生产声明/实现、367行测试；保留既有1617输入，最终锁增加为1620。初始锁 Saved/Automation/P28.31/validation-inputs.json 保留首次测试内容；修正夹具后另写 validation-inputs-Final.json，最终清理末尾空行后写 validation-inputs-Final2.json，不覆盖旧证据。Saved运行/核验辅助脚本不提交。

保护文件：103个原未跟踪文件按P28.29锁逐个校验。两份OverallReadiness用户修改不暂存：Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`；Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。

## 2. 实施与首次失败

计划完整保留旧生成来源已解析事实，并新增Owner和序号前提；定义/奖励元数据复用原Items类型。新增纯计算，不接Service/Repository，不新增schema、存储或World所有者。候选回执的“结构有效”和“持久已接受”刻意分开；来源自然键不含seed，冲突无法靠改变seed获得新来源身份。相等检查覆盖完整元数据，5个注册专项之一逐项变更53处载荷验证拒绝与原值保持。

首次任务 Dev.D.UE.0.0.10.P28.31.r0.Contract：
- Editor 21:35:37.3700595–21:35:48.0526711UTC，7 actions、原生0。
- Game 21:35:48.0716573–21:36:03.0304316UTC，4 actions、原生0。
- 专项 21:36:03.6046962–21:36:24.9174968UTC，原生3、1条Success、0条Fail、无结束队列。BoundedPlanValidation构造“同定义漂移”负例时使用 P.Entries.Add(P.Entries[0])，TArray自引用保护断言终止进程。外层验证拒绝该运行。不是完整1/0通过、不是已存在产品bug的RedProof，也不是环境超时。
- 修正仅测试夹具：先复制独立Duplicate，再更改slot/definition并MoveTemp追加。生产文件未变；重新构建/重新运行全部本轮组。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Contract/BuildEditor/20260921T213537340Z-5ddc4f02/stdout.log`，SHA `097EAF4E7AFD7585E9F92CB27EB30D3028D4F27A28A1B220CBBD38C1D64F3CA9`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Contract/BuildGame/20260921T213548068Z-69ac1f02/stdout.log`，SHA `42F49B37305C839A9F3542BA1A0547176216E73659737DE4F2FB1663B62C83DC`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Contract/GeneratedSourceFocused/20260921T213603589Z-8176eef5/UnrealEditor.log`，SHA `3A12C2815FB522021AA735681C9C02995069B49ED8175C87D378260AA442F565`。

## 3. 修正夹具后的完整运行

任务 Dev.D.UE.0.0.10.P28.31.r0.Final，统一执行会话47115正常完成0。每段前后验证1620输入和103原文件；没有并行重复UE测试。BuildBoth通过既有Scripts入口，无头组经既有Foundation工具以隐藏UnrealEditor-Cmd执行，带NullRHI/Unattended/TestExit和独立日志目录。

Editor 21:37:22.8595586–21:37:27.8760906UTC，4 actions，测试重编译及Items链接，原生0。
原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final/BuildEditor/20260921T213722830Z-487f850d/stdout.log`，SHA `E747D922B5E9E7CAAAB56D825F6B19A6009BBD36747962D28A01537F62D3B30C`。

Game 21:37:27.8944787–21:37:40.2019432UTC，3 actions，测试编译/目标链接，原生0；没有运行demo_map.exe。
原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final/BuildGame/20260921T213727891Z-98f82193/stdout.log`，SHA `D826B4BB2AAA54DF25E5C618EE97CC7C259D77B1461BB77769FB6CC2D3D2310D`。

GeneratedSourceFocused 21:37:40.7592920–21:38:01.1091449UTC，5 Success/0 Fail，队列5、原生0。
原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final/GeneratedSourceFocused/20260921T213740744Z-63ec9482/UnrealEditor.log`，SHA `CBCAD07D93A24CFBF998AE979460DA3CB97ACA8F05DCDF03C798275045405AE9`。

ItemsRegression 21:38:01.5767469–21:38:31.9051219UTC，90/0，队列90、原生0（原85项+本轮5项）。
原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final/ItemsRegression/20260921T213801574Z-98339946/UnrealEditor.log`，SHA `6C478532D31DD33681F855B2FFF4317B523563296D9A5DA1A7D3B3ADE83AB67D`。

LegacyFullRoot 21:38:32.2443399–21:39:52.6382586UTC，1330/0，队列1330、原生0。
原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final/LegacyFullRoot/20260921T213832242Z-e00b1451/UnrealEditor.log`，SHA `DA9CC8292D45027F3021F0006DE93928B614444FB3B1A3118A0298A1BE194E87`。

本批三个测试日志均无Fatal/Ensure/Unhandled/Assertion；逐用例名称唯一，专项5项包含在Items90内，独立成功1420。未重跑整个Shanmen根，不复用旧1430条来宣称新增源码后的完整产品根已通过。本轮非最终冻结。

### 3.1 精确提交文件的最终复跑

21:45暂存门发现新头文件与Report末尾各多一个空行，git diff --cached --check失败，因此未提交。移除两处空行后重新固定Final2输入；相对Final源码只变头文件末尾空行，不改变语义。为维持精确输入证据，重新执行BuildBoth和同三组；会话46670正常完成0。前两批原件未删。最终仍为1420个独立成功用例，重复运行不累加为更多用例。

Final2 Editor 21:45:37.9078160–21:45:44.9207092UTC，5 actions、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final2/BuildEditor/20260921T214537881Z-26fdba5c/stdout.log`，SHA `B4402F14EB018AD6DF49E01D5FA23BB4D7AA2D9FB28E7F965C9E617B6B764F89`。

Final2 Game 21:45:44.9388890–21:45:58.6836644UTC，4 actions、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final2/BuildGame/20260921T214544936Z-2e39071f/stdout.log`，SHA `F132A05A33A6895C84BC5B7ED032226981C42774B415E2393E70EA10C1B26CC9`。

Final2 GeneratedSourceFocused 21:45:59.2389663–21:46:19.5869992UTC，5/0、队列5、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final2/GeneratedSourceFocused/20260921T214559215Z-817a1809/UnrealEditor.log`，SHA `1E3C4417B06DBC8A684F8F71A8350FB8988C9AB27B9790015E66B3C926F8EE5C`。

Final2 ItemsRegression 21:46:20.0393597–21:46:50.3725557UTC，90/0、队列90、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final2/ItemsRegression/20260921T214620037Z-728b2f78/UnrealEditor.log`，SHA `102DA2C93AABABB87E8ADE090A309085E3FC31AC87999C74A2F223B88C76D425`。

Final2 LegacyFullRoot 21:46:50.6887746–21:48:02.0019016UTC，1330/0、队列1330、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.31.r0.Final2/LegacyFullRoot/20260921T214650686Z-69a0664e/UnrealEditor.log`，SHA `005A1360370B524C3A96CB7B74C45525362F4975D7A7194F54DA5D7416C83C84`。

## 4. 改动覆盖、核验与交接

映射检查器现跑537/537；文件未改，因此输出内容SHA恰与前阶段相同，但本轮确实重新执行。
原件 `Saved/Automation/P28.31/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。

三源码全部属于ShanmenItems，必跑Items整组。六路径检查 `REGRESSION_COVERAGE: PASS Changed=6 Rules=1 Required=1 Logs=1`，采用实际90项完整日志，不只新专项。
原件 `Saved/Automation/P28.31/regression-coverage-final2.log`，SHA `6B6DBCCD20D42569DC73FABCE2C5653E8D2E466910542889760E8EE648320480`。

新增生产文件扫描无demo_map/UWorld/AActor/NewGuid/FMath::Rand/RandomStream引用，Items.Build.cs依赖未改，无Engine依赖。没有新增持久权威/运行状态或生产调用方；回执使用私有载荷/const访问，不加Blueprint读写口。

21:43:58.4015997UTC PreStage通过：1620输入/1617原输入保持、103原文件、两保护文档、当时十份原件SHA、152处相对链接、双实际构建与精确队列；暂存0/未跟踪108（仅多本阶段五个新文件），工作区diff检查通过。随后暂存新文件时发现第3.1节空行问题；不能用先前未覆盖新文件的工作区检查替代暂存检查。Final2完成后按最终锁重新执行Staged→Published检查，校验十五份原件SHA及6路径/父提交/分支。仅提交本轮三源码、Report、Log、冻结索引，不提交Saved原件，GitHub Log只提供原件路径/哈希，不声称原始日志已上传。

FZ-1/2仍开放。下一步是既有唯一权威内的锁下接收、同文档持久化和schema迁移，不是接玩家输入或重新启用旧奖励writer。最终仍需生成→持久接收→恢复/物化→获物/消耗→终局的组合证据。遵守P/F限制，不暂停自动化。

- [Report](../Report/Dev.D.UE.0.0.10.P28.31.r0_report.md)
- [冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
