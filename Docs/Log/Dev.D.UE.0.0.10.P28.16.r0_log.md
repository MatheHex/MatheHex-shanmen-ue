# Dev.D.UE.0.0.10.P28.16.r0 Development Log

状态：`P28_16_COMPLETE`。Manager直接去激活的灵石投影拒绝保持修复及完整阶段验证通过；第1—7节保留当时失败、修复和进行中记录，最终核验见第8节。不宣布底层整体冻结。

## 1. 入场、有限审计与保护

2026-09-19 22:39 heartbeat。分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `d8d0410bb21f592ce06aba52d90d42ba55801bcb`。读取P28.15完成Report/Log、共享P/F基线、有限索引及续接记录；无运行中的UE-Cmd/UBT，不重做P28.15。1617前阶段产品输入和103原未跟踪路径/哈希一致，暂存为空。

两份OverallReadiness原用户修改保护：Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`；Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。本轮不改、不暂存。

先检查剩余外部Adapter Start疑点：当前非测试源码只有ProfilePreparationFlow调用Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun，Flow已有Settled且WorldActorCount非零的先行拒绝。仅凭Adapter内部先持久化后Prepare的顺序不足以证明产品入口可达新故障，本轮没有修改该Adapter或新增公开恢复接口。随后沿既有Manager.DeactivateProfileWorld查到灵石释放的实际遗漏，选择它作为最小修复范围。

## 2. 反例与首次Red

Manager先完成GameMode释放、容器释放、Items.TeardownWorld；然后固定灵石与M01灵石数组直接Destroy并清引用，不检查返回值，最后清来源去重集合和ProfileWorld活动标志并返回true。与物品投影不同，灵石是纯货币Actor，没有物品实例可代替其World归属，因此不能靠物品Teardown保住这组引用。

扩展原注册测试Shanmen.0_0_10.Product.ControlledWeaponWorldLifecycle.ManagerDeactivationRetention。在原实际AuthGameMode/瞬态World、飞剑拒绝与两单位SpiritDust拒绝之后，放入经InitializeRuntimePickup绑定原Run的固定20灵石、数组拒绝30灵石及数组正常40灵石。前两项以ROLE_SimulatedProxy制造真实Destroy拒绝；连续两次调用同一Manager端口，要求保留固定/数组原owner、身份/值、两条来源去重标记和活动/pending标志，同时已成功灵石/敌人/散落物只释放一次、持久快照不改。恢复角色后各原Actor恰好释放一次，才清来源标记。不调用拾取、物理输入或货币提交；未新增友元/注册用例/故障端口。

22:42:38.6429916UTC Red锁1617输入、103原用户文件，只有测试源码变化。RedBuild 4 actions / 10.45秒，22:42:50.4045059UTC结束、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.16.r0.RedBuild/BuildEditor/20260919T224239631Z-080d7ec4/stdout.log`，SHA `6D65E661CC54C6EE650908366F8160CBB5ED92D7C9491F98D24120657D610205`。

RedProof于22:43:11.3275664UTC结束0 Success / 1 Fail、精确队列1、原生0、崩溃指标0，5条最终Expected断言失败（两次错误确认、两次归属丢失、恢复未一次释放）；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.16.r0/RedProof/20260919T224250909Z-d0fa9941/UnrealEditor.log`，SHA `D99FEEC9D7B5D09157094482691F6ABDCFA8571EB042B65F5543E7E3491D5433`。exec78875退出0不是测试通过，失败原件独立保留。

## 3. 修复与锁定

DeactivateProfileWorld对固定投影保留Destroy拒绝的原弱引用，对数组按快照释放、仅保留拒绝项；两个集合均处理完再判是否全部接受，不让固定项拒绝掩盖数组项的独立释放。拒绝时保留来源去重标记、InitialWorldItems及ProfileWorld未完成上下文并返回false；全部释放才清去重与活动标志。不重复已接受Actor的释放，不结算货币/物品，不新增持久状态或通用清理框架。

两源码：ShanmenPreparationAdapterTests.cpp 57新增/1删除，V3ProgressionManager.cpp 16新增/6删除，共73/7。新增include只为使用既有灵石Actor类型。22:43:42.3814615UTC Final锁1617输入/103用户文件，Saved/Automation/P28.16/validation-inputs.json；原Red输入单独保存在red-inputs.json。此后不改锁定输入。

## 4. 验证证据与续接

原exec19401/pwsh12388（父22016）串行Editor→WorldLifecycle7→ProductFlow5→Game→旧根1330→新根1430，各段前后检查输入锁；无Editor UI/PIE/Standalone/产品运行，只有UE-Cmd Unattended/NullRHI和编译。

Editor于22:43:58.5598181UTC通过4 actions / 14.93秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.16.r0/BuildEditor/20260919T224343328Z-173c9c46/stdout.log`，SHA `04BC86442CC34DA686A3289A9270ABE0C5D6AC86DA9D873BECA70663B903FF03`。

WorldLifecycle于22:44:19.4603738UTC完成7/0、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.16.r0/WorldLifecycleFocused/20260919T224359059Z-b0f8e30c/UnrealEditor.log`，SHA `935195C15ACC74B508B229A224E0114E07F55D4FCC5451957C3A9CEFE5DCE5EF`。首次生产修复即转绿，保留原Red，不删警告或错误重放证据。

ProductFlow于22:44:39.8898632UTC完成5/0、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.16.r0/ProductFlowFocused/20260919T224419542Z-3463ae1e/UnrealEditor.log`，SHA `8F025FEA126E7F173815DCD4B5FB54BF61CE0106D2B6537899CAF77526616805`。

Game于22:45:07.1999667UTC通过4 actions / 26.71秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.16.r0/BuildGame/20260919T224440242Z-b3499322/stdout.log`，SHA `3966F61CED5AA3CDEE7DF617AEA514E79F8013382DB91FB6FFBAB69E756D1016`。只编译，没有运行生成的产品exe。

映射自检exec20198独立完成529/529、原生0；原件 `Saved/Automation/P28.16/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。同SHA为确定性输出，并非复用旧执行。当前七项完成原件均只保留本地。

22:45UTC快照已进入LegacyFullRoot，之后同一原运行器执行ShanmenFullRoot；尚未核验双完整根最终计数/结束队列/原生退出/崩溃指标，不对运行中日志给最终SHA，不提前记阶段完成。

LegacyFullRoot于22:46:18.4682943UTC完成1330成功/0失败、精确队列1330、原生0、崩溃指标0、SUCCEEDED；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.16.r0/LegacyFullRoot/20260919T224507520Z-c8d06089/UnrealEditor.log`，SHA `44A6F44EBB8814CF17310FB5497E24FA0BDF8D99858E95AFB1D511658C643C4E`。同一原运行器随后进入ShanmenFullRoot/20260919T224618774Z-e406aa36，UE-Cmd18784为pwsh12388子进程，未另开重复验证；新根运行中不记最终计数或SHA。

22:48:01.8499333UTC复核1617锁定输入、103原用户文件和两份OverallReadiness用户编辑均未改变；当时七项完成原件SHA核对一致，文档相对链接和git diff --check通过。未跟踪105项仅为103原用户文件加本Report/Log，暂存为空、HEAD仍d8d0410。补入已完成旧根后共有八项完成原件，新根仍待原运行器结束。

## 5. 覆盖与后续交接

五个阶段路径静态命中ProductRunItemUse、ItemProductAdapters两规则；七必跑组为demo_map.CodeB、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、demo_map.Profile、demo_map.V2RangedCompatibility、Shanmen.0_0_10、Shanmen.0_0_10.Items。Saved/Automation/P28.16/coverage.ps1必须等唯一原双根最终健康后执行，静态匹配不是实际覆盖通过。

完整验证后更新有限索引、Report/Log，精确暂存两源码及索引/Report/Log五路径，提交推送并核对远端。当前索引仍P28.15，暂存为空、HEAD仍d8d0410；不触碰103原未跟踪文件和两份OverallReadiness编辑。不伪造本轮GitHub链接，Saved原件只本地保留。

## 6. 未证明范围

本阶段证明Manager直接去激活中货币投影的拒绝保持和成功前缀不重做，不替代普通/框架完整激活入口、货币拾取/持久结算、其他敌人/容器调用点、旧兼容终局、空间包部分释放或强制EndPlay的逐项审计。两个激活函数也有固定灵石释放代码，但本轮没有在其正常入口证明相同拒绝条件，因此不作无证据批量改动。FZ-1/2保持开放，不将本地契约通过当作内存/性能、UI或F阶段验收。

- [Report](../Report/Dev.D.UE.0.0.10.P28.16.r0_report.md)

## 7. 23:23 heartbeat续接检查点

读取当前分支/HEAD、Git状态、最新Report/Log、上阶段完成记录及P/F基线。HEAD仍d8d0410bb21f592ce06aba52d90d42ba55801bcb，原exec19401/pwsh12388/UE-Cmd18784仍为同一验证链；未启动重复验证或修改锁定源码。

23:25:02.5061583UTC的新根过程快照为1093 Success / 0 Fail、崩溃指标0，尚无结束队列及最终run-state；最后已完成ThrownWeaponArcPreviewPresentationDeliveryHost.Lifecycle。日志持续写入，原进程仍正常推进。HTTP generate_204超时警告保留，不据此中止健康验证；历史Presentation命名的无头契约不表示启动真实UI。

同一检查点复核1617锁定输入、103原用户文件、两份OverallReadiness保护哈希和八项已完成证据SHA全部一致，暂存为空。当前仅更新续接记录，不将1093过程计数当最终通过、不对运行中日志记最终SHA、不提前执行覆盖门或提交阶段。FZ-1/2仍开放。

## 8. 2026-09-20 00:27 heartbeat最终核验与交接

读取当前分支/HEAD、Git状态、最新Report/Log、P/F基线、有限冻结索引和续接记录。原exec19401已正常结束、原生退出0，输出locked validation complete。相同22:43:42输入锁贯穿Editor、两专项、Game和双完整根，本轮未重复启动验证或修改锁定源码。

新完整根于2026-09-19T23:54:57.0657186Z完成1430 Success / 0 Fail、精确队列1430、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.16.r0/ShanmenFullRoot/20260919T224618774Z-e406aa36/UnrealEditor.log`，SHA `2A26B5374948335B591BF84722F4BE2F9331964893C5400018B80FD664EE6652`。与旧根1330/0合计2760成功、0失败；专项7+5单独执行但不重复计入完整根。

00:28:11.8646934UTC独立核验1617锁定输入、103原用户路径/哈希、两份OverallReadiness保护哈希及此前八项完成原件全部一致。两专项与双根的成功/失败数、唯一精确队列、原生退出、崩溃指标和最终进程状态分别通过；Editor/Game均SUCCEEDED/原生0。未跟踪精确105（原103加本Report/Log），暂存为空，HEAD仍d8d0410。

确认原双根最终健康后独立运行改动驱动覆盖门，原生退出0：PASS Changed=5 Rules=2 Required=7 Logs=2；第5节七组均在本阶段两份完整根中取得实际日志证据。原件 `Saved/Automation/P28.16/regression-coverage.log`，SHA `B97DAA3FA1327473F399112C829D1C0ED03A9983E8D7BBD768F93633B34BDA70`。不是静态匹配或529项自检替代回归覆盖；当前十项完成原件以及首次Red输入独立保留，Saved原件只本地保留。

最终更新本Report/Log与有限索引，精确交接五路径：Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp、Source/demo_map/demo_mapV3ProgressionManager.cpp、Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md和本Report/Log。只暂存这五项，不包含103原用户文件或两份OverallReadiness修改；不强推，提交后核对远端并在本地续接和对用户交接中记录实际提交，不预写未知哈希。

本阶段仅闭合Manager直接去激活中的固定/M01灵石投影拒绝保持与成功释放前缀不重做，没有完成所有货币入口、拾取/持久结算或完整M01激活审计。FZ-1/2仍开放，不宣布整体冻结、不暂停自动化；没有进入玩法、UI、内容或F阶段。
