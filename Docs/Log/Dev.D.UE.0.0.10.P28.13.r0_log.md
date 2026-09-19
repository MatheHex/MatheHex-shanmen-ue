# Dev.D.UE.0.0.10.P28.13.r0 Development Log

状态：`P28_13_COMPLETE`；完整阶段验证通过。第1—7节保留当时的执行与等待记录，最终结果见第8节；阶段完成不等于整体冻结。

## 1. 入场与保护

2026-09-19 14:59 heartbeat，分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `94a383d5b6b1f0df6f93edf79edc9bea3a3386a4`。读取P28.12 Report/Log、P/F基线、有限索引和已发布续接记录，无运行中的UE-Cmd/UBT。前阶段1617输入与103原用户路径/哈希一致，暂存为空，仅两份OverallReadiness有原用户修改。不重做已发布P28.12。

保护Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`；两次输入锁已验证一致。

## 2. 取证与首次失败

Manager.DeactivateProfileWorld在战斗/容器完成后调用Runtime.TeardownWorld，但后者为void，先RemoveWorldBinding再忽略Actor.Destroy拒绝，随后DestroyWorld与清ActiveWorld。WorldItem.EndPlay会回调同一个Runtime清绑定/归属；因此需要按身份快照迭代，在引擎接受销毁后才移除仍存在的绑定并完成权威墓碑，拒绝时不动。

扩展既有ManagerDeactivationRetention，不新增注册用例或友元：真实Profile cutover/Runtime/瞬态World/GameMode、非零固定时间线、两单位SpiritDust。先沿原用例拒绝两次飞剑销毁，再恢复飞剑并令散落物ROLE_SimulatedProxy，两次调用同一个Manager端口；要求原身份/数量/绑定/Manager owner保持、战斗前缀不重做、持久快照不变，恢复后一次销毁。

15:02:47.909UTC Red锁1617输入、103用户文件，只有测试源码变化。RedBuild于15:03:08.388UTC完成4 actions / 19.06秒，SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.13.r0.RedBuild/BuildEditor/20260919T150248929Z-a66c2ade/stdout.log`，SHA `1EEC58BA7D2410ACC81784E28032138720D42E786CBB63E1A606F0C6584B9762`。

RedProof于15:03:55.173UTC完成0 Success / 1 Fail、精确队列1、原生0、崩溃指标0；5条最终断言失败：两次拒绝不能确认Manager完成、两次原身份/owner保留、故障解除后一次销毁。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.13.r0/RedProof/20260919T150308887Z-8cbc4360/UnrealEditor.log`，SHA `B427B368FD978D71343EA18A75B71559EF4A65E58729309817F41D586B796923`。引擎Condition failed诊断亦保留；不把SUCCEEDED进程或原生0当断言通过。

## 3. 最小改动与锁定

Runtime.TeardownWorld/BeginWorld返回bool；拒绝不清原绑定、World物品身份和ActiveWorld；Destroy的EndPlay回调可先完成绑定移除，随后只处理仍存在的World身份。空间包追踪仅全部释放后清除。Manager把拒绝往上返回，InitialWorldItems/活动状态保持。原Preparation与测试重置不再无条件清权威，两个Runtime新World写入口和三个非Smoke Manager入口接收BeginWorld拒绝。没有schema/第二权威/新增恢复记录，也不修改引擎权限。

最终测试还检查错误World去激活、新World绑定、跨WorldCreateWorldItem在投影前拒绝，原World绑定与不变量仍成立；临时OtherWorld不启动或初始化场景。上述追加检查没有独立Red，不宣称每条都先失败；核心持有/恢复断言已有首次Red。成功项一次释放，非整批原子撤销。

15:05:24.554UTC Final锁1617输入与103用户文件，恰好ItemSubsystem.cpp/.h、PreparationAdapterTests.cpp、V3ProgressionManager.cpp四处源码变化，75新增/22删除。git diff --check通过。锁定记录Saved/Automation/P28.13/validation-inputs.json；之后不修改产品输入。

## 4. 验证执行

原exec60467已结束（Red）；新exec73046串行执行Editor→WorldLifecycle7/ProductFlow5→Game→旧根1330→新根1430，每段前后核验同一输入锁。仅UE-Cmd Unattended/NullRHI，无Editor UI或产品执行。未完成的构建/根日志不计算最终通过，也不重启正常前进的测试。

静态七路径映射命中ProductRunItemUse与ItemProductAdapters，必跑7组为Shanmen.0_0_10、Shanmen.0_0_10.Items、demo_map.Profile、demo_map.CodeB、demo_map.V2RangedCompatibility、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar。实际覆盖门脚本Saved/Automation/P28.13/coverage.ps1等待两根最终SUCCEEDED/原生0后才能运行；这不是已通过证明。

映射自检exec80168独立完成529/529、原生0；原件 `Saved/Automation/P28.13/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。输出确定而与前阶段同SHA，本阶段确已运行。

最终Editor于15:08:25.171UTC完成48 actions / 179.28秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.13.r0/BuildEditor/20260919T150525544Z-5338b5ff/stdout.log`，SHA `0384CFCC211B3293905E63D828574A0D13613BAFB3CA545F7B2F831B58310169`。同一exec73046/pwsh46964继续后续专项和Game/双根，不另启验证。

WorldLifecycle于15:08:46.073UTC完成7/0、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.13.r0/WorldLifecycleFocused/20260919T150825636Z-61c0fd2a/UnrealEditor.log`，SHA `3D12D01CCF611A9842750F165EBF055B815D2CC3F17AA89195F2CA2DBFDD248B`。首次Red中的核心保持/恢复断言及追加跨World拒绝检查均通过。

ProductFlow于15:09:06.535UTC完成5/0、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.13.r0/ProductFlowFocused/20260919T150846159Z-6b6580a1/UnrealEditor.log`，SHA `7E1AA9EEF2E2732CC7ECBA55685C12E42FB665FE33774BBDBB9D22D62CE7FD3E`。同一父进程继续Game（15:10UTC快照15/47 actions），之后才执行双根；已结束证据共6项，不提前给未完成验证记SHA或通过。

15:10:28.203UTC复核1617锁定输入、103用户路径/哈希和两份保护文档一致，未跟踪精确105（原103+本Report/Log），暂存为空、git diff --check通过。只写进度/续接，不改输入、不提交推送、不运行尚缺完整根的覆盖门；保持IN_PROGRESS。

## 5. 未闭合范围

本阶段直接Manager去激活中Run仍活动；RequestSettlement先改变归属和压缩墓碑后清理World的路径并未由它证明。空间包部分释放、其他容器/灵石/敌人调用点、强制EndPlay及FZ-1入口仍需按现有可达条件核对。Preparation、自动化Reset及三个非Smoke Manager激活入口只做静态返回值路由审查；不冒充各入口的真实故障专项或正式M01激活验收。

103原未跟踪文件与两份用户总体文档不动；未跟踪新增仅本Report/Log。完整验证后才更新有限索引和精确暂存七路径，提交并核对远端。原始Saved证据不上传，公开Log只列准确路径/SHA。

- [Report](../Report/Dev.D.UE.0.0.10.P28.13.r0_report.md)

## 6. 15:43 heartbeat续接检查点

当前分支/HEAD仍为94a383d，读取本阶段Report/Log、P/F基线和原续接记录。原exec73046/pwsh46964继续执行，无重复验证实例。Game于15:12:16.538UTC完成47 actions / 189.31秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.13.r0/BuildGame/20260919T150906947Z-7ec65da9/stdout.log`，SHA `E08852564B07780C36F06FAE9DB30B2840B3B45CC66C05F46DE21F848ED9ACF3`。

旧根于15:13:32.340UTC完成1330 Success / 0 Fail、精确队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.13.r0/LegacyFullRoot/20260919T151216857Z-5f23a985/UnrealEditor.log`，SHA `A06C2EE5E2E6CD9C899127431DD063F5CDA4710DF6945D4C533E421CDF8AB157`。已结束原件共8项，均属本阶段独立运行。

当前UE-Cmd32124是pwsh46964启动的原有子进程，运行ShanmenFullRoot/20260919T151332631Z-0747dbc8，参数Unattended/NullRHI。15:44:50.438UTC快照946 Success / 0 Fail、崩溃指标0，没有最终run-state或队列结束；当前GameModeReleaseRecovery在15:44:37UTC有RunRetirementCompleted/RunReleased进展。HTTP generate_204超时警告保留，不把过程中的预期拒绝日志当最终失败。

复核1617锁定输入、103原未跟踪路径/哈希、两份OverallReadiness保护哈希及原6项完成原件均一致；未跟踪精确105、暂存为空、git diff --check通过。保持源码/构建输入不动，仅记录续接，不执行缺少最终新根的覆盖门，不提交推送或宣布冻结；索引仍以P28.12为最新完成阶段。

## 7. 16:18 heartbeat续接检查点

读取当前分支/HEAD、Git状态、最新Report/Log、P/F基线及有限索引；HEAD仍为94a383d，P28.13保持IN_PROGRESS。原exec73046复查仍运行，pwsh46964/UE-Cmd32124未替换，没有另起测试或修改锁定源码。

16:19:14.483UTC，新根同一日志快照1191 Success / 0 Fail、崩溃指标0，无结束队列或最终run-state。16:19:04.782UTC的Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.AdoptionAuthorityFences完成Success，下一AuthorityFailureFences于16:19:04.783UTC开始；相较上次946有实际进展。16:19:07UTC的HTTP generate_204超时警告保留，不把它判为测试失败，不计算运行中文件的最终SHA。

1617锁定输入、103原未跟踪用户路径/哈希、两份OverallReadiness保护哈希以及本Log现有8项完成原件SHA全部复核一致。未跟踪仍精确105，暂存为空，git diff --check通过。继续保留原验证实例；实际改动驱动覆盖门、阶段提交/推送及索引完成标记仍等待新根最终结束。没有进入F阶段或宣布全局冻结。

## 8. 16:54 heartbeat最终验收与交接

本轮读取当前分支/HEAD、Git状态、最新Report/Log、P/F基线、有限索引和续接记录。原exec73046已经结束、原生退出0，运行器在双根完成后核对同一输入锁并输出validation complete；没有重新启动UE、修改生产输入或复用前阶段执行结果。

最终新根于16:21:15.906UTC完成1430 Success / 0 Fail、精确队列1430、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.13.r0/ShanmenFullRoot/20260919T151332631Z-0747dbc8/UnrealEditor.log`，SHA `DAF4D9F41556ED58BBB2B74284B1747DDFB5BC687C5FF135BA50C722C6157818`。本阶段完整旧根1330与新根1430合计2760成功、0失败，专项7+5为另行执行但不重复增加完整根计数。HTTP超时等原始警告保留，不据无崩溃指标声称没有警告或内存/性能达标。

本轮在两根最终状态均为SUCCEEDED/原生0后执行实际覆盖门，原生退出0：PASS Changed=7 Rules=2 Required=7 Logs=2，命中ProductRunItemUse与ItemProductAdapters；第4节七个必跑组均由这两份健康完整根日志覆盖。原件 `Saved/Automation/P28.13/regression-coverage.log`，SHA `7B591D08FA8380DAD931874DA7139637E5F42668992D77105B8172E495C8CC8C`。映射529/529自检和实际覆盖门是不同检查，未将静态匹配当成日志覆盖通过。

16:56:28.995UTC复核1617锁定输入、103原用户路径/哈希、两份OverallReadiness保护哈希和此前8项完成原件全部一致；专项与双根的最终计数/队列/进程状态再次独立核对通过。所有10项完成原件原地保留，包括首次失败，不拼接、删警告或上传Saved目录。输入自15:05锁定后未变；最终只编辑索引及本Report/Log，Git空暂存起步，原未跟踪集合为103+本阶段两文档。

精确交接七路径：ItemSubsystem.cpp/.h、PreparationAdapterTests.cpp、V3ProgressionManager.cpp四处源码，加FoundationClosure_Index与本Report/Log；不暂存103份原用户文件和两份OverallReadiness修改。包含这些文件的提交是本阶段基线；提交后的远端一致性核验记录在本地续接记录与对用户的交接中，不在本文预写未知提交哈希。

本轮未进入F：没有正式地图/内容资产、物理输入、玩法数值或UI开发，没有Editor UI/PIE/Standalone/产品exe/Smoke/Cook/Package。FZ-1/2的其余可达路径仍需有限审计，特别是Runtime.RequestSettlement先改归属再清World的不同路径，不能以本次TeardownWorld拒绝保持替代其证明。
