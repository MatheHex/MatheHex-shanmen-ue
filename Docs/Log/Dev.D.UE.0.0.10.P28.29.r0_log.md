# Dev.D.UE.0.0.10.P28.29.r0 Development Log

状态：`P28_29_PASS`。M01 生成失败投影的原 owner 保持与续清理；不是整体冻结或 F 验收。发布基线由包含本文的 Git 提交追溯。

## 1. 基线与保护

2026-09-21T17:33:48.761Z heartbeat；分支 agent/0.0.10-p27-28-formation-scatter-gamemode-composition；HEAD `48b2a40f0a1e45b74f8baa4f80b56b66b60c1b8a`。读取 P28.28 Report/Log、P 基线、冻结清单与 Git 状态。17:33:59.0491639UTC P28.28 Published 核验通过：1617 输入、103 原文件、两份保护文档、九份原件 SHA、141 链接、暂存 0；未重复上一阶段验证。

保护文档 OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`、Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。沿用 P28.26 的 103 个原文件精确路径/哈希与 1617 项产品/验证输入；本轮生成独立 red-inputs.json / validation-inputs.json，只允许测试、随后 Manager 两个明确路径改变，不覆盖原锁。

## 2. 可达条件与修复

审阅 Manager 的 HandleM01EnemyDeath、PrepareGeneratedRewardSource、DestroyRuntimeContainers 与去激活端口，CorpseContainerActor 的 InitializeGeneratedCorpse / InitializeM01GeneratedCorpse，以及 Profile Session/Coordinator 的 GeneratedReward 提交条件。新 Flow 的 StartPreparedRunDirect 使用与夹具相同的 Shanmen RunLifecycleAdapter::StartPreparedRun；不调用旧协调器 BeginRun。M01 身份组件 ProjectCorpse 直接转发到本次验证的 Manager 端口。

扩展已有 ManagerDeactivationRetention：在其真实持久 Run、非零装备/库存和瞬态 World 上，以现有 M01 配置建立有效身份，再调用 HandleM01EnemyDeath。未启动 Enemy 行为或正式地图。规划实际成功且产生 8 个计划堆叠，旧 Profile 提交拒绝，容器未初始化；不是无效配置或缺失 Session 的人为早退。OnActorSpawned 中使用既有 SetRole 引擎条件令 Destroy 实际拒绝，无新增生产故障端口。两次去激活的保持断言和恢复后的一次释放断言在旧生产代码上产生五条最终失败。

最小修复只改该 M01 失败分支：有效对象非销毁中且 Destroy 拒绝时 Corpses.AddUnique，交还既有同 owner 清理链；不登记来源成功、不提交奖励、不改权威快照。生产 7 新增/1 删除，测试 70 新增，注册数不变。首次修复后专项通过，无二次源码修正；健康回滚对照没有独立 Red。

生成奖励的旧提交路由另列 FZ-1：新 Run 活动不等于旧 Profile 协调器 RunActive；本测试实际拒绝与静态调用链一致。清理修复不解决该权威衔接，不把拒绝改成成功，也不直接向 Runtime 赠送物品。其他尸体重载、所有终局组合与强制 EndPlay 未在本新增变体取证。

## 3. 首次 Red

17:37:36.7741209UTC 记录 Red 输入锁，只改测试、生产保持 48b2a40 基线。任务 Dev.D.UE.0.0.10.P28.29.r0.Red。

Red Editor：17:37:37.3787895–17:37:48.7253607UTC，测试编译成功、原生 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Red/BuildEditor/20260921T173737336Z-495156f6/stdout.log`，SHA `F1B3DD791D4C3EE3BAA5B9F96E9BBEAE8F9F0228CC9B507CD569186C4B66ED87`。

RedProof：17:37:49.2179733–17:38:09.5804179UTC，ManagerDeactivationRetention 0 成功/1 失败，五条最终 Expected 断言失败、精确队列 1、原生 0、Fatal/Ensure/Unhandled 指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Red/RedProof/20260921T173749203Z-8dda024f/UnrealEditor.log`，SHA `1C2632751BA174F3DB4D020FF4BA775E187C41DA384F6D6A3F5E96A34E3966B4`。exec29917 外层 0 表示“预期 Red 被捕获”，不是测试成功；用例末尾局部释放孤立对象仅清理夹具，不遮盖此前断言。

## 4. 固定修复输入后的验证

17:38:34.1417265UTC Final 输入锁只允许 Manager 与既有测试两个产品路径改变，其余 1615 项及原 103 文件一致。任务 Dev.D.UE.0.0.10.P28.29.r0.Final；通过既有构建入口与隐藏 UnrealEditor-Cmd 无头执行。

Editor：17:38:34.7030534–17:38:52.5033073UTC，实际编译 Manager/链接、原生 0，工具报告 17.55 秒。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Final/BuildEditor/20260921T173834660Z-6a6a9e3f/stdout.log`，SHA `5B17C011BDDF15CDCCA41F017526DD81757363C4A1C31C493662DC5454D5CE3C`。

Game：17:38:53.0040689–17:39:24.2744377UTC，实际编译两个改动源文件/链接、原生 0，工具报告 31.04 秒。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Final/BuildGame/20260921T173853001Z-87fbcd6c/stdout.log`，SHA `F0AD4ADCD4CC7A6E56D05DB6F2D59BE86626E6B5B8FFC13E7E7BBB4126F76C92`。没有启动生成的游戏程序。

WorldLifecycleFocused：17:39:24.6593907–17:39:45.0033441UTC，7 成功/0 失败、精确队列 7、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Final/WorldLifecycleFocused/20260921T173924647Z-6420231d/UnrealEditor.log`，SHA `776184F98C10FB81D37DC880D3E0896422DAEBEADF29A36F0CBC6DAB77B07E33`。

ProductFlowFocused：17:39:45.3782346–17:40:05.6902822UTC，5 成功/0 失败、精确队列 5、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Final/ProductFlowFocused/20260921T173945375Z-07c39deb/UnrealEditor.log`，SHA `FFC26D7312DDEECAF16946AF59060E0A8A2F786755D4B4A3B2A154447E1E58B7`。

LegacyFullRoot：17:40:06.0947304–17:41:21.5057264UTC，1330 成功/0 失败、精确队列 1330、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Final/LegacyFullRoot/20260921T174006092Z-5d3a4264/UnrealEditor.log`，SHA `604AFC90EC00A13E9A2885E981BDC702251226EBBC6FF78622A1098C6D42E89D`。

ShanmenFullRoot：17:41:22.2456421–19:34:02.7531548UTC，1430 成功/0 失败、精确队列 1430、原生 0、Fatal/Ensure/Unhandled 指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Final/ShanmenFullRoot/20260921T174122243Z-17c2dbd9/UnrealEditor.log`，SHA `848B001954601A58D3969C01700C3FC6E1482FEE05A5E1F6EA75E7433F73D6A2`。该完整进程约 112 分 41 秒；有外部连通性 HTTP 超时警告，不声称零警告或由此证明运行时性能。多次心跳期间保持同一进程和源码锁，没有重启、拼接日志或使用中间计数提前通过；20:06UTC 取回已结束 exec15676，外层 0。两根独立成功用例合计 2760；专项 12 个为两根子集，不累计成 2772。

映射检查器自检：537/537，外层 0。原件 `Saved/Automation/P28.29/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。映射/检查器本身未修改。

改动路径覆盖：`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=7 Logs=2`，本轮两个源码与三个文档路径匹配 ProductRunItemUse / ItemProductAdapters；七组为 demo_map.CodeB、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、demo_map.Profile、demo_map.V2RangedCompatibility、Shanmen.0_0_10、Shanmen.0_0_10.Items。证据仅使用上述本阶段完整新旧根，未借用历史原件。原件 `Saved/Automation/P28.29/regression-coverage.log`，SHA `1963582797548518138F71F226A0870FF72E3CB646F50A71FF3F35C84D782563`。

## 5. 精确交接

只提交两个源码文件、本 Report/Log 与有限冻结索引五路径；Saved 中原件和辅助核验脚本只保留本地，GitHub Log 提供可追溯路径/哈希。发布前核验固定输入、103 原文件及两份保护文档、原件 SHA、链接、完整回归与队列、真实 Red、精确暂存范围及 git diff --check。

20:08:48.0558735UTC PreStage 核验通过：1617 项固定输入、103 个原未跟踪文件、两份保护文档均一致；十份原件 SHA、141 处相对链接、六次完整健康构建/自动化、2760 个完整根独立用例及专项子集关系、真实 Red 和覆盖证据全部匹配。暂存为空，未跟踪105项仅比原集合新增本阶段Report/Log；git diff --check 通过。此后只登记这条核验事实，再重复核验并精确暂存五路径；提交后以父提交和路径集合检查发布范围，不将用户文档或原文件一并提交。

FZ-1/2 仍开放；不将局部清理修复当成奖励权威路由或最终冻结闭合，不暂停自动化。未进入物理输入、正式地图/内容资产、玩法/UI、Editor UI、PIE、Standalone、游戏程序、Smoke/Cook/Package。

- [Report](../Report/Dev.D.UE.0.0.10.P28.29.r0_report.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
