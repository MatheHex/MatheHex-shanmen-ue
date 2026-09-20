# Dev.D.UE.0.0.10.P28.19.r0 Development Log

状态：`P28_19_COMPLETE`。非M01任务对象释放拒绝的原owner保持已复现并修复；完整双根2760/0、双构建、两专项、实际覆盖与映射自检通过。第1—8节保留当时的时间线状态，最终结论见第9节；发布版本以包含本Log/Report的Git提交为准。

## 1. 入场与保护

2026-09-20T06:06:35.411Z heartbeat；分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `880d010dfe996eedba38abc47f57e9ccb5d7b749`。读取已发布P28.18 Report/Log、续接、P/F共享基线与有限索引剩余项；没有UE-Cmd在运行，不重跑已完成阶段。06:08:09.1167199UTC核验1617产品输入、103原未跟踪文件精确集合及哈希、暂存0。

两份OverallReadiness修改保持：Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。捕获脚本在Red与Final分别核验，不编辑、不暂存这些用户文件。

## 2. 可达性与反例

先检查ActivateV3MissionContentForRun的非M01活动分支：其就绪条件不检查撤离foundation，但要走完整TryActivateCombatRun及上层Manager条件，不能仅凭表面缺项宣布完整入口缺陷，本轮不据此改激活代码。

选取已明确的直接释放入口：非M01 SpawnMissionActorsFromEncounterMarkers实际持有三个目标、出口与友军；旧非V3创建路径还创建三类敌人，V3 Manager.InitializeEnemyEncounterContent则经BindV3EnemyProjections提供三类引用。DeactivateProfileWorld先调用GameMode去激活，false应阻止后面的容器/物品清理。该GameMode端口前面已保护战斗、M01敌人与撤离区域，尾部却忽略非M01对象Destroy的实际结果，清全部引用/CountedTargetActors并返回true。

沿既有PreparationDeactivationRetention追加有界夹具，不新增注册用例或友元：保留原飞剑、敌人/Boss和撤离拒绝变体；完成其清理后在同一瞬态World添加三个目标与出口/近战/远程/重型/友军五对象。六个对象设ROLE_SimulatedProxy实际拒绝两次，两个目标成功；计数去重集合预置一个既有成功目标身份，避免空集合恒真。检查原owner、未完成任务上下文、持久非零快照与Run保持；成功项不重复；恢复角色后八个对象各销毁一次，完整清理重放成功。

此夹具直接调用既有去激活端口，GameMode是瞬态对象，未运行真实任务创建/激活和玩家流程；目标死亡回调及正式地图行为不在此专项的证明范围。

06:09:43.6721182UTC Red锁1617输入/103用户文件，仅测试源码改变。Red Editor于06:09:56.382494UTC结束，4 actions/11.63秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.19.r0.RedBuild/BuildEditor/20260920T060944443Z-4b7630e3/stdout.log`，SHA `E9C0B728096A3AEF00A33021D5D5FD5C4F31D0CE06EA77F02166A76AE198964C`。

RedProof于06:10:17.2761005UTC结束：0成功/1失败、精确队列1、原生0、崩溃指标0。最终5条Expected失败为两次错误完成确认、两次原owner/去重记录保持、恢复后的恰好一次释放；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.19.r0/RedProof/20260920T060956883Z-0937844f/UnrealEditor.log`，SHA `F4ABA0E6BB4BD164ED7BF5E507B626E786676E0FA2F4BA6F91CFC83F0A1C9918`。原exec28975退出0表示预期反例执行完成，不是测试成功；失败原件不覆盖。

## 3. 有界修复

GameMode去激活尾部局部lambda使用原类型弱引用，真实拒绝返回false且不Reset。目标先拷贝数组，只保留拒绝项；出口、近战、远程、重型和友军均独立尝试，任一拒绝不能短路其他槽。全部原对象释放后才清CountedTargetActors和bV3MissionContentActive。未引入第二份权威、通用恢复记录或新接口；没有更改其余初始化、终局、EndPlay或Smoke调用方。

生产GameMode.cpp 19新增/22删除，测试74新增，共93新增/22删除；头文件、schema、注册数、其他角色类、内容资产均未改。false沿Manager既有检查向上传递是源码调用次序证据，本轮测试不冒充Manager完整组合的新专项。

## 4. 最终验证执行

06:11:03.5749517UTC Final锁1617输入/103原用户文件；Saved/Automation/P28.19/validation-inputs.json。原exec98951，pwsh34880/父644；依次Editor→WorldLifecycle7→ProductFlow5→Game→LegacyFullRoot1330→ShanmenFullRoot1430，各段前后复核输入。原进程健康时不重复启动、不更改锁定产品输入。

- 最终Editor于06:11:16.9980174UTC完成：4 actions/12.35秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.19.r0/BuildEditor/20260920T061104339Z-40c26b7b/stdout.log`，SHA `752396FF6275940CA8AB0D93EB5D643BC738EA1005B8B9F069F47328F4BB6B52`。
- WorldLifecycle于06:11:37.9483143UTC完成：7成功/0失败、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.19.r0/WorldLifecycleFocused/20260920T061117561Z-90ed5919/UnrealEditor.log`，SHA `0353C047CE9C50E64E431FA95BC5646FA2564B3DCC1FC2673DA038266B58D46E`。
- ProductFlow于06:11:58.3827117UTC完成：5成功/0失败、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.19.r0/ProductFlowFocused/20260920T061138029Z-059cc307/UnrealEditor.log`，SHA `91C4B73D4EA8785C939175A16A0ADB74C7E2DCDA82F09F4FE0D039ED4B33E629`。
- 独立映射自检exec40320退出0，529/529；原件 `Saved/Automation/P28.19/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。相同SHA来自本阶段实际执行的确定性输出，不是复用旧运行。

Game及完整双根尚未完成最终验收，不给运行中日志最终SHA。五路径实际覆盖门Saved/Automation/P28.19/coverage.ps1只在本阶段两份健康完整根结束后执行，静态映射和自检均不能替代实际覆盖。

## 5. 交接边界（验证完成前记录）

只交接Source/demo_map/demo_mapGameMode.cpp、Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp、Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md与本Report/Log五路径。最终核验输入锁、原103用户文件、保护哈希、原件SHA、文档链接和diff check，再精确暂存/提交/推送及远端核对。有限索引暂留P28.18，FZ-1/2开放；未暂停监控、不进入F阶段。原日志Saved只本地保留。

- [Report](../Report/Dev.D.UE.0.0.10.P28.19.r0_report.md)

## 6. 06:15UTC检查点

原exec98951继续，新增两项已完成证据：

- Game于06:12:21.9478146UTC完成：4 actions/22.91秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.19.r0/BuildGame/20260920T061158786Z-0bcc5756/stdout.log`，SHA `999E250E77D57ACB5E7BBAC0875717039647782B69F9A150A6386C2151B8CA6F`。
- Legacy完整根于06:13:17.7082086UTC完成：1330成功/0失败、精确队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.19.r0/LegacyFullRoot/20260920T061222282Z-f7b5b8b2/UnrealEditor.log`，SHA `B80549EF5F38F0047D114A256695C2E3D81DE745158DDD29072D79B70E7D539F`。

Shanmen原运行目录20260920T061318001Z-1ea565e8，06:15UTC观察584成功/0失败、崩溃指标0，无最终队列/原生退出；不写最终SHA，不把部分结果视为通过。05路径静态映射命中M01GameMode和ItemProductAdapters，共81必跑组；实际覆盖仍待本阶段健康双根完成。

06:15:31.8612290UTC核验锁定1617输入、103原用户文件精确路径/哈希、未跟踪105=103+本Report/Log、暂存0、已记六项原件SHA、四个相对文档链接及diff check通过。有限索引仍留P28.18，产品输入不再修改，等待同一运行器，不提前提交/推送或宣告整体冻结。

## 7. 06:50UTC续接检查点

2026-09-20T06:49:05.952Z heartbeat读取分支、HEAD、最新Report/Log、续接与P/F基线；原exec98951仍运行，未重启或另起验证。06:50:00.0552122UTC新根累计1074成功/0失败、崩溃指标0；日志06:49:58UTC完成ThrownWeaponArcPreviewPresentationCompositionOwner.OrderedAppliedReplay，接着开始ReentrantSurfaceBlocked。原UE-Cmd10756/父34880仍在运行，尚无最终队列及run-state，不将中间计数当作1430最终通过。

同次核验1617锁定产品输入、原103未跟踪文件精确集合与哈希、两份OverallReadiness保护哈希、八项完成原件SHA均一致；未跟踪105、暂存0、有限索引未改、分支/HEAD不变、diff check通过。仅更新本地进度文档，继续等待原验证；不执行实际覆盖门、不提前暂存/推送或宣告冻结。

## 8. 07:23UTC续接检查点

2026-09-20T07:23:06.446Z heartbeat恢复原exec98951，仍未退出。07:23:36.4826719UTC观察1188成功/0失败、崩溃指标0；日志在07:23UTC完成ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.RequestContractAndDeterminism，随后运行TrustedReplay。原UE-Cmd10756/父34880存续，日志有推进；没有最终队列、run-state或原生退出，不计算最终SHA，不提前执行实际覆盖门。日志同时有HTTP连接探测超时警告，当前已完成用例仍为Success，不将警告等同于失败，也不据此重启健康原进程。

复核1617输入与103原用户文件哈希、精确未跟踪105集合、两份OverallReadiness保护哈希、八项完成原件SHA、空暂存区与diff check均通过，有限索引仍未改。继续保持锁定产品输入，仅更新本地进度记录，待原回归完整结束后交接。

## 9. 最终验收与本阶段交接

2026-09-20T07:59:37.018Z heartbeat读取当前分支、Git状态、最新Report/Log与P阶段基线，恢复原exec98951得到退出0；未启动替代验证。Shanmen完整根的run-state为SUCCEEDED、原生0，于07:27:14.7232705UTC完成：1430成功/0失败、精确队列1430、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.19.r0/ShanmenFullRoot/20260920T061318001Z-1ea565e8/UnrealEditor.log`，SHA `0D0DA2706260A5BBD27FAA43256F41DB666838B2946950659984C094A7CC69E9`。

对最终五路径执行实际覆盖门：PASS，Changed=5、Rules=2（M01GameMode/ItemProductAdapters）、Required=81、Logs=2。本阶段两份健康完整根合计2760成功/0失败，不累加7/5专项重复用例；原件 `Saved/Automation/P28.19/regression-coverage.log`，SHA `548645517F1C01F87C0088DB9A18F1C219355C0CD5E898216F8E63B7C8779E9D`。独立映射自检仍为本阶段实际执行的529/529；首次Red原件保留。

08:03:12.4577765UTC复核1617锁定输入、103原用户文件精确集合/哈希、未跟踪105=103+本Report/Log、两份OverallReadiness保护哈希、空暂存区与diff check；四组自动化最终计数、队列、原生退出和崩溃指标独立复算一致。仅更新Report/Log和有限冻结索引，不再改产品输入。最终精确交接五路径，不包含两份用户修改、原103文件或Saved原件；发布后验证远端HEAD及剩余工作区边界。

FZ-1/2继续开放：本次仅证明直接GameMode去激活端口，非M01完整激活/再进入、其他清理调用点和强制EndPlay仍须先核对实际入口条件。无Editor UI、PIE、Standalone、产品exe、地图/内容资产编辑、物理输入、玩法/UI开发、Smoke/Cook/Package；不宣布整体冻结、不暂停自动化。

08:05:53.6553785UTC最终预暂存核验通过：1617输入、103原用户文件、两份保护文档、10项原件SHA、98个相对文档链接、六个最终运行SUCCEEDED/原生0、未跟踪105、暂存0、diff check均符合预期。报告/日志/索引中的最终证据已更新，旧检查点只保留历史事实。
