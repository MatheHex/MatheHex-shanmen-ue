# Dev.D.UE.0.0.10.P28.17.r0 Development Log

状态：`P28_17_COMPLETE`。M01敌人/Boss投影拒绝保持已复现并修复，最终串行验证与实际覆盖门均通过。下文保留中途检查点，其进行中状态不覆盖第6节最终结果；不宣布整体冻结。

## 1. 入场与保护

2026-09-20 01:05 heartbeat；分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `73ec9836182eeb70d805c6f3c48dab94423b6830`。读取P28.16最终Report/Log、本地续接与P/F基线；原阶段已发布、不重跑，无UE-Cmd/UBT验证在执行。核验原1617输入与103未跟踪路径/哈希一致，暂存为空。

两份OverallReadiness用户修改保护：Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。不编辑、不暂存这些文件。

## 2. 实际入口与反例

沿P28.16之后既有剩余清理调用图检查：Manager.DeactivateProfileWorld已经接收GameMode返回结果，但GameMode.DeactivateV3MissionContentForPreparation中的DestroyM01EnemyContent仍先清数组/Boss归属再请求销毁。战斗前缀成功后，真实敌人Destroy拒绝被吞掉，调用方继续清撤离与任务上下文并确认成功；下一次无原引用可重试。

扩展原注册用例Shanmen.0_0_10.Product.ControlledWeaponWorldLifecycle.PreparationDeactivationRetention。原瞬态World/飞剑/注册敌人夹具保留，增加普通成功敌人和使用既有Fdemo_mapM01EnemyConfig初始化的Boss；账本绑定原Run并有三条非零遭遇记录。先按原测试连续两次拒绝飞剑销毁，然后恢复飞剑角色、将原敌人和Boss设为ROLE_SimulatedProxy，真实触发引擎Destroy拒绝两次。观察原数组/Boss/Enemy引用、Boss非零生命、任务/撤离上下文保持，已释放飞剑与成功敌人不重做，账本逻辑终止但原Run/去重记录仍在且不能提交新Boss死亡；物品持久快照不变。恢复后每个原Actor恰好释放一次。没有模拟玩家输入或运行正式敌人行为，不新增故障端口/友元/注册测试。

01:09:17.4721646UTC Red锁1617输入/103用户文件，只有测试源码修改。RedBuild完成4 actions / 11.39秒，于01:09:30.1392953UTC结束、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.17.r0.RedBuild/BuildEditor/20260920T010918436Z-479de09e/stdout.log`，SHA `3805F7E286D7B5553776AFDCF640132B269A3E7874BD6A7B2D132849CFB70211`。

RedProof于01:09:51.0259628UTC结束0 Success / 1 Fail、精确队列1、原生0、崩溃指标0；5条最终Expected失败为两次错误确认、两次原owner/上下文丢失及最后一次释放断言。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.17.r0/RedProof/20260920T010930646Z-74001e79/UnrealEditor.log`，SHA `333C70EA5F626EBA79E9C543D3FCB2336EA0D99D7A8B75084B212FC945C89C81`。exec58968退出0是运行器成功退出，不是Red测试通过；原始引擎诊断一并保留。

## 3. 修复与范围

私有DestroyM01EnemyContent由void改bool；保留既有MarkTerminal、bM01EnemyContentActive=false和Suppress调用，逻辑不复活。按原数组快照请求销毁，仅将真实拒绝项保存回原数组；Boss弱引用仅失效/正在销毁时清掉。返回数组是否为空，GameMode去激活在false时不继续清撤离或任务标志。InitializeM01EnemyContent在旧清理拒绝时先返回false，发生在配置验证、ResetForNewRun与新生成之前。

GameMode.cpp 16新增/6删除，GameMode.h 1/1，ShanmenPreparationAdapterTests.cpp 58/4；三路径共75/11。没有新公共API、schema、恢复记录或第二权威。初始生成失败的未入数组Actor、非M01类型投影、撤离区域释放和强制EndPlay不由本次测试替代；初始化检查仅静态确认，不伪称完整M01入口故障已实测。

## 4. 输入锁与最终验证执行

01:10:37.1732454UTC Final锁1617输入/103原用户文件，Saved/Automation/P28.17/validation-inputs.json；red-inputs.json独立保留。原exec9346，pwsh21072/父21648，依次Editor→WorldLifecycle7→ProductFlow5→Game→LegacyFullRoot1330→ShanmenFullRoot1430，各段前后核验相同输入锁。只编译与运行Unattended/NullRHI UE-Cmd，未启动Editor UI/产品程序，不改锁定输入或重复启动健康验证。

映射自检exec23083独立完成529/529、原生0；原件 `Saved/Automation/P28.17/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。相同SHA是确定性输出，并非复用旧执行。

01:17UTC复核原运行器及原件，已完成四段：

- 最终Editor：37 actions / 149.61秒，01:13:08.0440878UTC结束，SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.17.r0/BuildEditor/20260920T011038120Z-f5c624c4/stdout.log`，SHA `B73FA962B3B8468AB18FBA7C2F56EA89C07C1E8210048A350056407C39AC714C`。
- WorldLifecycle：7成功/0失败、精确队列7、原生0、崩溃指标0，01:13:28.9304820UTC结束；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.17.r0/WorldLifecycleFocused/20260920T011308532Z-45d8f1a9/UnrealEditor.log`，SHA `502009248200DED2779DD49B738E7278F5E23B1C2E811B639F3A94F41974BD7A`。
- ProductFlow：5成功/0失败、精确队列5、原生0、崩溃指标0，01:13:49.3887372UTC结束；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.17.r0/ProductFlowFocused/20260920T011329027Z-c62be529/UnrealEditor.log`，SHA `A3335D34F9764EF2BBD97B25931B40BB72DC8A9D0F3ABFA6B6FD8B4E9FC723C4`。
- 最终Game：36 actions / 147.13秒，01:16:17.2029174UTC结束，SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.17.r0/BuildGame/20260920T011349804Z-ae0ff896/stdout.log`，SHA `7C2524E93C0E7D55E6BA9FBFC9C289FE9F775BEF595E1F624A506F8A0D840B2B`。

LegacyFullRoot于01:17:28.0224063UTC结束，1330成功/0失败、精确队列1330、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.17.r0/LegacyFullRoot/20260920T011617526Z-09cb286e/UnrealEditor.log`，SHA `8979EB6596856C172E3001530D39EA5F53BD271A47CBA37A34D2F896DEDE5F38`。

随后原运行器进入ShanmenFullRoot，运行目录20260920T011728311Z-0f2a57e6，UE-Cmd PID440/父21072；尚无最终run-state/队列清空，不对运行中日志给最终SHA或宣布完整回归通过。01:17:18.5618573UTC再次核验1617输入、103原用户文件及两份OverallReadiness哈希未变，未跟踪105=103原文件+本Report/Log，暂存0，git diff --check通过。已记录完成原件与相对文档链接均经本地核验。

## 5. 覆盖与续接

精确六路径：Source/demo_map/demo_mapGameMode.cpp、Source/demo_map/demo_mapGameMode.h、Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp、Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md和本Report/Log。静态映射命中M01GameMode、ItemProductAdapters两规则，共81必跑组；待唯一原双根最终健康后执行Saved/Automation/P28.17/coverage.ps1，静态结果不是实际日志覆盖通过。

最终核验须分别检查成功/失败数、精确队列、原生退出、run-state与崩溃指标，再核验输入/用户文件不变、所有完成原件SHA及文档链接。随后更新有限索引并精确暂存六路径，提交推送/远端核对；目前索引仍P28.16、暂存为空、HEAD73ec983，无本阶段GitHub提交。Saved原件仅本地保留。

FZ-1/2仍开放，不扩充通用恢复体系、不进入玩法/UI/正式内容/F，不暂停自动化。后续从原exec9346续接，不重复执行已完成Red或P28.16。

- [Report](../Report/Dev.D.UE.0.0.10.P28.17.r0_report.md)

## 6. 2026-09-20 02:59 heartbeat最终核验与交接

读取当前分支/HEAD、Git状态、最新Report/Log、P阶段基线及有限冻结索引，续接原exec9346。它已退出0并输出locked validation complete；本轮未重跑已完成验证、未修改锁定产品输入。中途01:52与02:26 heartbeat分别观察新根1042和1197成功、0失败，仅保留续接检查点，未把过程计数当最终通过。

ShanmenFullRoot于02:27:10.2280838UTC完成1430 Success / 0 Fail、精确队列1430、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.17.r0/ShanmenFullRoot/20260920T011728311Z-0f2a57e6/UnrealEditor.log`，SHA `18B46FDE3ECBB58BD822105C3C1AE2E00F1194023085B634D5E16C15CDF40634`。与旧根1330/0合计2760成功、0失败；专项7+5独立执行但不重复计入完整双根。首次Red是预期反例，未混入健康计数；原始HTTP超时等警告保留，不伪称无警告或性能合格。

03:00:34.9531578UTC独立核验1617锁定输入及103原用户路径/哈希一致；未跟踪105=103原文件+本Report/Log、暂存0、HEAD仍73ec983。分别复核双完整根成功/失败、精确队列、原生退出、最终状态和崩溃指标，未发现仍运行的UE-Cmd。

随后运行Saved/Automation/P28.17/coverage.ps1，原生退出0：PASS Changed=6 Rules=2 Required=81 Logs=2。六路径的M01GameMode/ItemProductAdapters必跑组均由本阶段健康双根提供实际证据，详细81组列表在原件中；原件 `Saved/Automation/P28.17/regression-coverage.log`，SHA `324FC9258D9D272AD36038F6CA68176E413F5169F735AA9F4CEAF92C344CC952`。不是静态映射或自检代替产品回归。

最终更新本Report/Log及有限索引，03:03:35.9947323UTC核验十项已完成原件SHA、三文档90个相对链接、git diff --check、1617输入锁、103用户文件与两份OverallReadiness保护哈希全部通过；未跟踪精确105，暂存0。只交接第5节六路径；提交后核对远端，在本地续接和用户交接中记录实际提交，不预写未知哈希、不强推。Saved原件仅本地保留。

本阶段只证明既有GameMode去激活端口的敌人/Boss销毁拒绝保持、终局账本不复活及成功前缀不重做。完整M01初始化、撤离区域、非M01投影、局部生成失败和强制EndPlay不由该夹具替代。FZ-1/2仍开放，不暂停自动化，不进入玩法、UI、正式内容或F。
