# Dev.D.UE.0.0.10.P28.15.r0 Development Log

状态：`P28_15_COMPLETE`。技术激活回滚的散落物拒绝续接修复及完整阶段验证通过；第1—7节保留当时的失败/修复/进行中历史，最终验收见第8节。不宣布整体冻结。

## 1. 入场与保护

2026-09-19 20:13 heartbeat，分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `37136226805de31e2fe7c4c47f6c52f97867baec`。读取P28.14 Report/Log、P/F共享基线、有限冻结索引及续接记录，核验上阶段输入锁和原用户文件。P28.14已完成，不重做正常终局修复。

原103未跟踪路径/哈希独立保存在本地输入锁users中。两份保护文档：OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`；Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。不纳入本阶段。

## 2. 实际调用链与首次失败

普通Manager启动回滚或框架原保留attempt进入Manager.RollbackPreparedProfileRunFor0909B，再调用Flow.CancelActiveRunForActivationFailure、Runtime.RequestSettlement(ActivationFailure)与PrepareForPersistentRun。Runtime现在保留拒绝销毁的Destroyed散落投影，但Flow仍把这类InvalidWorldBinding记为FatalProfileError/RecoveryRequired，并清掉materialized标志；Manager未能取得原pending前缀，后续重试被阶段校验拦住。故障移除也无法结束原attempt/清理。

扩展原注册测试Shanmen.0_0_10.Product.ControlledWeaponWorldLifecycle.ManagerRollbackContinuation：普通/框架原两变体保持，各增真实两单位SpiritDust散落投影。使用ROLE_SimulatedProxy触发引擎实际Destroy拒绝；瞬态碰撞Floor只满足既有安全投影检查。继续使用原保留activation-attempt夹具/既有友元，不新建故障端口、注册用例或公共接口，不声称复现完整M01 BeginActivation。

20:16:15.5609074UTC Red锁1617输入/103用户文件，仅测试源码相对基线变化。RedBuild完成4 actions / 11.54秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.15.r0.RedBuild/BuildEditor/20260919T201617023Z-aa89ccca/stdout.log`，SHA `83954FE1E18391E13AB2704EA69B9E96061DEFBFD48F5325080E45CA87BD9E91`。

RedProof于20:16:49.7414175UTC结束0 Success / 1 Fail、精确队列1、原生0、崩溃指标0，39条最终Expected断言失败，涵盖原pending/上下层身份保持、重试门与最终一次性释放。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.15.r0/RedProof/20260919T201629342Z-1f39f386/UnrealEditor.log`，SHA `6341B0007578AAED55427AED99C1EAA4690FC15D66D9B880703991C0DBAD164E`。39指最终Expected断言，不是日志所有Error行总数；预期错误计数差异及引擎诊断原样保留。exec79878退出0并不表示Red测试通过。

## 3. 最小修复

Flow只接受匹配原StartedRunId、有效ActivationFailure Summary、Settled Runtime及RuntimeSnapshot的InvalidWorldBinding作为原Runtime前缀已接受/World尚未完成。维持原RuntimeRollbackReady状态交给Manager既有续接，持久ActiveRun未改写；其他准备错误仍FatalProfileError，不恢复新Run权限。

Manager在原pending绑定校验中允许同Run、同ActivationFailure原因的Settled Runtime，其他Flow、Runtime对象、Session、Owner、World、GameMode、Run、提交计数及阶段约束保持。DeactivateProfileWorld全部接受后，才Prepare原Runtime并清原pending；Prepare仍拒绝则保留原前缀，不重复提交终局。

测试要求两次飞剑拒绝、错误Runtime绑定和两种新启动入口仍失败关闭；新增两变体在飞剑故障移除后再两次仅散落物拒绝，确认飞剑/敌人各已销毁一次但原散落物身份/Actor/数量/损失回执不丢，持久快照逐字段保持、SettlementSubmitCount仍1。框架保留原attemptId、sequence与correlation，普通入口不能代接框架归属；最终故障移除只完成原回滚，不顺便进入新玩法。

源码三路径：ProfilePreparationFlow.cpp（15新增/3删除）、V3ProgressionManager.cpp（13/2）、ShanmenPreparationAdapterTests.cpp（82/8），共110/13。无新schema/API/恢复记录/第二权威，未改P28.14正常终局实现。

## 4. 锁定验证与已完成结果

20:17:38.361316UTC最终锁1617输入/103原用户路径与哈希，Saved/Automation/P28.15/validation-inputs.json；Red输入独立保存red-inputs.json。原exec90855/pwsh6320（父34540）依次Editor→WorldLifecycle7→ProductFlow5→Game→旧根1330→新根1430；每段前后核验同一输入锁。没有重启正常验证或在锁定后改产品输入。

Editor于20:17:58.8026295UTC通过5 actions / 18.77秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.15.r0/BuildEditor/20260919T201739737Z-4370e700/stdout.log`，SHA `7DF744D86C03E33D7C6F3F53C84244541A13B2786F3BDDB4A59E90733EB30385`。

WorldLifecycle于20:18:19.6939253UTC完成7 Success / 0 Fail、精确队列7、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.15.r0/WorldLifecycleFocused/20260919T201759309Z-b8347408/UnrealEditor.log`，SHA `9DF5EAD584FBC7AAAA4815808A23FCDC1A45144513DDFC76427AEC785A4143B8`。ManagerRollbackContinuation四变体通过，首次生产修复即转绿，原Red保留。

ProductFlow于20:18:40.1533013UTC完成5/0、精确队列5、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.15.r0/ProductFlowFocused/20260919T201819783Z-439d7b81/UnrealEditor.log`，SHA `EBA74CFBF9466644F4CA3CDD903E5775AEC7B13523B6AF71475ACA2068DE5D00`。

Game于20:19:09.2521741UTC通过5 actions / 28.43秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.15.r0/BuildGame/20260919T201840576Z-a5730bc6/stdout.log`，SHA `B2706B920DA029C547AEB7AA619C7B6079BFAB7D969F9BF6C132756478F241ED`。只编译，没有运行产品exe。

映射自检exec99798独立执行529/529、原生0；原件 `Saved/Automation/P28.15/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。相同SHA是确定性输出，本轮不是复用此前执行。

旧完整根于20:20:20.0128473UTC完成1330 Success / 0 Fail、精确队列1330、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.15.r0/LegacyFullRoot/20260919T201909560Z-33dadaf3/UnrealEditor.log`，SHA `B8EFD74E1850B70778451799EE1D0CDA4997CFBBDACE53FA0B5B11B49092D448`。专项不重复计入该完整根。

20:22UTC读取原exec90855仍运行，已进入新根ShanmenFullRoot/20260919T202020306Z-25634a34；尚无最终run-state/精确结束队列，不给运行中日志最终SHA或声称双根完整通过。当前共八项完成原件，全部Saved本地保留，不上传原件。

## 5. 改动驱动映射与交接待办

预定六路径（三源码、有限索引、本Report/Log）静态命中ProductRunItemUse、ProfileAuthority、ProfilePreparationProductFlow、ItemProductAdapters四规则。九组必跑为demo_map.CodeB、demo_map.ItemEconomySchema、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、demo_map.Profile、demo_map.V2RangedCompatibility、Shanmen.0_0_10、Shanmen.0_0_10.Items、Shanmen.0_0_10.Items.ProductFlow。

实际覆盖门Saved/Automation/P28.15/coverage.ps1必须等双根唯一原运行最终SUCCEEDED/原生0后独立执行；静态匹配和529项自检均不是日志覆盖通过。待新根1430/0、精确队列、原生退出/崩溃指标与输入保持核验完成，才更新索引和最终Report/Log、精确暂存六路径、提交推送并核对远端HEAD。当前暂存为空、HEAD仍3713622，索引仍P28.14；没有新阶段GitHub链接。

## 6. 边界与续接

只处理当前实际技术激活回滚/保留attempt续接的散落物拒绝，不新增通用恢复层。其他外部Adapter Start、旧兼容终局、空间包所有部分释放、其他容器/灵石/敌人调用点和强制EndPlay仍属于有限审计范围，FZ-1/2不关闭。

遵守P/F边界：只Editor/Game编译与UE-Cmd无头契约测试，未启动Editor UI、PIE、Standalone或产品程序，未接物理输入、改正式地图/资产、玩法/UI，未做Smoke/Cook/Package。回执或无头测试不代表F阶段表现、性能或内存验收。

本地续接Saved/Automation/P28.15/resume.md记录原运行器与下一检查点；后续先续读原exec90855，禁止因heartbeat重复启动健康验证。当前只保存进行中文档，不暂存产品输入，不暂停自动化。

20:25:43.2905555UTC复核1617锁定输入、103原用户路径/哈希、两份保护文档及八项完成原件SHA全部一致；未跟踪精确105（103原文件+本Report/Log），文档相对链接有效，暂存为空、HEAD仍3713622、git diff --check通过。没有对运行中的新根取最终SHA，也没有提前执行实际覆盖门或发布阶段完成标记。

- [Report](../Report/Dev.D.UE.0.0.10.P28.15.r0_report.md)

## 7. 20:58 heartbeat续接检查点

读取当前分支/HEAD、Git状态、最新Report/Log、P/F基线与本地续接记录。HEAD仍37136226805de31e2fe7c4c47f6c52f97867baec，原exec90855/pwsh6320/UE-Cmd7544仍在同一串行验证链；没有启动重复验证或修改锁定源码。

20:58:58.2487213UTC的新完整根快照1093 Success / 0 Fail、崩溃指标0，尚无结束队列或最终run-state。20:58:44.691UTC的ThrownWeaponArcPreviewPresentationDeliveryHost.Lifecycle完成Success，随后OrderedAppliedReplay开始，确认原验证正常推进。HTTP generate_204超时警告保留，不据警告重启健康进程，也不把过程计数当最终完成结果；历史测试名中的Presentation不表示启动了实际UI。

20:59:26.0632528UTC复核1617锁定输入、103原用户路径/哈希、两份OverallReadiness保护哈希及八项已完成原件均一致；未跟踪精确105，暂存为空、git diff --check通过。只补续接记录，不对运行中新根取最终SHA，不执行缺少最终新根的覆盖门，不提前更新索引、提交或推送。FZ-1/2仍开放，未进入F阶段。

## 8. 22:01 heartbeat最终验收与交接

读取当前分支/HEAD、Git状态、最新Report/Log、P/F基线、有限冻结索引及续接记录。原exec90855已结束、原生退出0，输出locked validation complete。相同20:17:38输入锁贯穿Editor、专项、Game与双根；本轮没有重跑已成功产品验证或修改锁定源码。

新完整根于21:29:03.1396569UTC完成1430 Success / 0 Fail、精确队列1430、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.15.r0/ShanmenFullRoot/20260919T202020306Z-25634a34/UnrealEditor.log`，SHA `1EB3A74B00B2A39F79D09D0FB1B3C3ABB1E7D13AFADB8215B6AE203835659C5D`。与本阶段旧根1330/0合计2760成功、0失败；专项7+5独立执行但不重复计入完整根。历史Presentation/HUD命名的无头契约用例不是启动UI；HTTP超时和引擎警告原样保留，不据通过声称性能、内存或F阶段验收。

22:02:41.5334389UTC独立复核1617锁定输入、103原用户路径/哈希、两份OverallReadiness保护哈希及此前八项完成原件全部一致；两组专项与双完整根的成功/失败数、精确队列、原生退出、崩溃指标及最终进程状态均通过，Editor/Game也均SUCCEEDED/原生0。未跟踪精确105，暂存为空，HEAD仍3713622。

两根最终健康状态确认后独立执行实际改动驱动覆盖门，原生退出0：PASS Changed=6 Rules=4 Required=9 Logs=2。命中ProductRunItemUse、ProfileAuthority、ProfilePreparationProductFlow、ItemProductAdapters，第5节九组均在两份本阶段健康完整根日志中取得证据；原件 `Saved/Automation/P28.15/regression-coverage.log`，SHA `58C38CA8BD5100131E117AA32A90AACE5A1AC469B30927E17D36441243111E9D`。不是用静态匹配或529项自检代替实际覆盖。当前共十项完成原件，首次Red/RedBuild及输入锁独立保留，Saved原件只保留本地。

最终更新有限索引与本Report/Log；精确交接六路径：Source/demo_map/demo_mapProfilePreparationFlow.cpp、Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp、Source/demo_map/demo_mapV3ProgressionManager.cpp、Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md和本Report/Log。不纳入103原用户文件或两份OverallReadiness修改，不强推；包含这六文件的提交是本阶段基线，提交后核验远端一致并记录本地续接/对用户交接，不在此预写未知哈希。

本阶段只闭合既有技术激活回滚/保留attempt路径的散落物拒绝保持和原World续清理；FZ-1/2仍开放，后续需按有限清单检查其余实际入口，不能由本阶段通过推导整体冻结。没有进入玩法、UI或F阶段，不暂停自动化。
