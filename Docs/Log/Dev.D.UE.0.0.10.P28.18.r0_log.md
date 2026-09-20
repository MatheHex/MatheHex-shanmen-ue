# Dev.D.UE.0.0.10.P28.18.r0 Development Log

状态：`P28_18_COMPLETE`。动态撤离投影拒绝保持已复现并修复，锁定输入下双构建、两专项、完整双根及实际覆盖门均通过。第4—6节是当时的运行检查点，第7节为最终验证与交接记录；不宣布整体冻结。

## 1. 入场与保护

2026-09-20 03:37 heartbeat；当前分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `8432c354c700fb75063a75b45263ad4736a664aa`。读取P28.17已发布Report/Log、续接、P/F基线与有限索引剩余项；无UE-Cmd在运行，不重跑P28.17。03:38:28.7696295UTC核验1617既有输入与103原用户文件哈希一致、未跟踪103、暂存0。

两份OverallReadiness用户修改保护：Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`，不编辑、不暂存。

## 2. 实际调用图与反例

DeactivateV3MissionContentForPreparation在战斗/敌人释放后调用DestroyM01ExtractionFoundation。后者对authored区域调用SetProjectionActive(false)，对动态区域直接Destroy，但均忽略实际清理结果，最后无条件Reset数组；初始化也在无条件清理后ResetForNewRun。非M01兼容初始化会实际生成动态区域，因此该分支不是只为测试虚构。正式M01地图重用authored区域，仍保持仅停用、不销毁的所有权规则。

既有PreparationDeactivationRetention扩展，不新增注册用例/友元/故障接口：在原瞬态World添加RefusedExit(Regular)、AcceptedExit(Boss)、AuthoredExit(DiscardSpatial)，后者仅在瞬态Actor配置authored。原撤离权威已通过既有MainBoss身份解锁，避免以默认锁定状态作恒真基准；物品Run和持久快照沿用原非零夹具。原飞剑和敌人/Boss拒绝阶段保持；恢复这些角色后，将RefusedExit设为ROLE_SimulatedProxy真实触发引擎Destroy拒绝两次。检查数组只保留原动态owner/稳定ID，区域停用、authored原Actor仍存在，已成功飞剑/敌人/动态区域不重复，物品持久快照和Run保持。故障解除后动态区域恰好一次销毁，authored对象不销毁。

03:39:50.0708342UTC Red锁1617输入/103用户文件，仅测试源码修改。Red Editor四actions、16.44秒，03:40:08.2396479UTC结束SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.18.r0.RedBuild/BuildEditor/20260920T033951460Z-cfafe97a/stdout.log`，SHA `FA7FB04FD6B8D7B5E4D4C29A4E57ED7D3B6655BEE234E11E07DFBE57CF8C97AF`。

RedProof于03:40:44.1866400UTC结束0成功/1失败、精确队列1、原生0、崩溃指标0。5条最终Expected失败分别为两次错误确认、两次原owner/停用边界断言及最后一次释放断言；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.18.r0/RedProof/20260920T034008743Z-a69ad209/UnrealEditor.log`，SHA `B68E2CCCF587B50341EB986E9B2D334754EB8417AA22CF70C59140C20CF875D9`。原exec49649正常退出0代表反例运行器完成，不是Red测试通过；失败原件及red-inputs.json独立保留。

## 3. 修复与边界

私有DestroyM01ExtractionFoundation由void改bool。保留既有逻辑停用与清ActiveM01RiskId，按原数组快照处理；所有有效区域复用SetProjectionActive(false)，authored只停用，动态才请求Destroy。无效/正在销毁项无需重做，只有拒绝项回存原数组，返回数组是否为空。去激活调用方在false时停止后续任务/非M01投影清理；初始化在旧清理false时先返回，不到ResetForNewRun或生成步骤。

GameMode.cpp 19新增/8删除、GameMode.h 1/1、测试49新增，三路径69/9；没有新公共API、schema、恢复记录、故障端口或注册测试，不修改区域类、内容资产、正式地图、倒计时常量或UI设计。GetM01ExtractionSnapshot和RequestM01Extraction已有inactive先行拒绝，Tick也在inactive时不Advance；本次不另建第二份撤离权威。

初始化检查为静态调用次序证据；瞬态GameMode对象不冒充真实M01初始化/玩家流程。非M01活动内容再激活、局部生成失败、其他类型投影与强制EndPlay仍需独立核验，不能以该用例宣布全部生命周期闭合。Smoke调用点仅阅读，不启动、不修改其流程。

## 4. 验证执行与续接

03:41:45.5662205UTC Final锁1617输入/103原用户文件，Saved/Automation/P28.18/validation-inputs.json。原exec6611，pwsh40808/父9976，依次Editor→WorldLifecycle7→ProductFlow5→Game→LegacyFullRoot1330→ShanmenFullRoot1430，每段前后核验输入；当前最终Editor运行目录20260920T034147064Z-a1d5eed9，未完成全部验收，不对运行中日志写最终SHA。

映射自检独立exec47786完成529/529、原生0；原件 `Saved/Automation/P28.18/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。相同SHA是确定性输出，并非复用旧执行。最终双目标/两专项/双根继续原进程，不重复启动或修改锁定产品输入。

预定六路径：Source/demo_map/demo_mapGameMode.cpp、Source/demo_map/demo_mapGameMode.h、Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp、Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md与本Report/Log。静态映射为M01GameMode、ItemProductAdapters两规则、81必跑组；原双根全部健康后才执行Saved/Automation/P28.18/coverage.ps1，不能将静态匹配或自检当实际覆盖通过。

后续核验每组成功/失败数、精确队列、原生退出、run-state/崩溃指标、输入及用户文件保护、完成原件SHA、文档链接与diff check；全通过后更新索引、精确暂存六路径、提交推送及远端核对。索引暂留P28.17，FZ-1/2仍开放，不暂停自动化、不进入F，Saved原件只本地保留。

- [Report](../Report/Dev.D.UE.0.0.10.P28.18.r0_report.md)

## 5. 03:45UTC检查点

原exec6611继续，最终Editor与两专项已完成：

- Editor：37 actions / 142.39秒，03:44:09.7776937UTC结束SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.18.r0/BuildEditor/20260920T034147064Z-a1d5eed9/stdout.log`，SHA `85FA4ED73A2AACDF99026E04EDCC2307DD02CBCAF7722732DB8B8FC0C7C66B36`。
- WorldLifecycle：7成功/0失败、精确队列7、原生0、崩溃指标0，03:44:30.6724190UTC结束；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.18.r0/WorldLifecycleFocused/20260920T034410289Z-e843939d/UnrealEditor.log`，SHA `533A160C731E3A4B88D152B4730FB45A4F4221EF9836783651ACF3AA00A41750`。
- ProductFlow：5成功/0失败、精确队列5、原生0、崩溃指标0，03:44:51.1427619UTC结束；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.18.r0/ProductFlowFocused/20260920T034430769Z-74d5a015/UnrealEditor.log`，SHA `1FC41C51B63552C27106C0D65104D1823953A6FD1508B32430529FE03B7D769E`。

Game运行目录20260920T034451567Z-94690cb8，尚未完成，不报告最终SHA。03:45:12.5813893UTC复核锁定1617输入、103原用户文件、未跟踪精确105、暂存0、三项当时已记原件SHA、四个相对文档链接、静态2规则81组与diff check通过；随后两份OverallReadiness保护哈希仍一致。六项完成原件保留，等待同一运行器继续Game及完整双根，不提前提交或更新有限冻结索引。

## 6. 04:22UTC检查点

2026-09-20T04:19:03.966Z heartbeat恢复同一exec6611；pwsh40808仍运行，未启动第二份验证。新增核验两项完成证据：

- Game：36 actions / 140.09秒，03:47:11.9076966UTC结束SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.18.r0/BuildGame/20260920T034451567Z-94690cb8/stdout.log`，SHA `3A713E3C87CE8EE9CE43A5F27B90322B8E9A8B643CFAC04C81ECFEEFACF46A5C`。
- Legacy完整根：1330成功/0失败、精确队列1330、原生0、崩溃指标0，03:48:07.6749700UTC结束SUCCEEDED；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.18.r0/LegacyFullRoot/20260920T034712257Z-859d7dd0/UnrealEditor.log`，SHA `54AE8D1BEAE24BC1B2AE2824DAA8530DCDE4F1E4E63035BE66026D35EE92EDBA`。

Shanmen完整根原运行目录20260920T034807964Z-21e6c1c1，UE-Cmd15740/父40808；04:22UTC观察1042成功/0失败、崩溃指标0，日志推进至ThrownWeaponArcPreviewMainHUDRuntimeBinding.ExactUpdateReplay。尚无最终run-state、队列结束或原生退出，不将部分计数写成1430通过，不写最终SHA。继续等待原进程；实际覆盖门仍未运行。

04:22:59.2550699UTC核验1617锁定输入/103原用户文件哈希、两份OverallReadiness保护哈希、已记六项原件SHA一致；未跟踪105、暂存0、三源码仍69新增/9删除，分支/HEAD不变，diff check通过。有限索引仍留P28.17，不提前提交/推送或冻结，不进入F阶段。

## 7. 完整验证与交接

2026-09-20T05:28:34.875Z heartbeat恢复原exec6611并取得正常退出0；未重复启动。新根原UE-Cmd15740于04:56:09.1838933UTC结束SUCCEEDED/原生0：1430成功/0失败、精确队列1430、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.18.r0/ShanmenFullRoot/20260920T034807964Z-21e6c1c1/UnrealEditor.log`，SHA `B020BC5EEEB8C411CA821BA6D54D767A92C9B52A1A64A9230EAC934E0766F65D`。与本阶段原旧根合计2760成功/0失败，专项7+5包含重复覆盖，不增加唯一用例总数。

05:29:23.5845754UTC重新核验1617产品输入、103原用户文件及两份OverallReadiness保护哈希均未变，暂存0、未跟踪105。随后实际覆盖门对六路径和本阶段两份健康完整根执行，PASS Changed=6 Rules=2 Required=81 Logs=2，原生0；原件 `Saved/Automation/P28.18/regression-coverage.log`，SHA `0AC97506C2FE5EC42B9997095135F4D98C61E21036E3549F5088903A3BBD7BE1`。

交接范围为三源码、有限冻结索引、本Report/Log六路径，不含两份OverallReadiness修改、原103未跟踪用户文件及Saved原件。05:32:05.6706598UTC最终复核十项完成原件SHA、94个相对文档/源码链接、六组最终run-state、1617产品输入、原103用户文件的精确路径与哈希、两份保护文件及diff check均通过；未跟踪105=原103+本Report/Log，暂存为空。精确暂存后核对路径集合，再提交当前分支并正常推送，不强推。GitHub提交与远端同步结果由本次交接回复及本地resume记录，文档不嵌入自身提交哈希。

FZ-1/2仍开放。下一审计应先核对非M01活动内容再进入分支、其他投影/局部创建失败及强制EndPlay的实际调用条件；本阶段不把初始化静态检查扩大为所有重新启动路径已失败关闭。持续遵守P/F边界，不暂停为已完成整体收尾，不进入实际玩法。
