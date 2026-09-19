# Dev.D.UE.0.0.10.P28.11.r0 Development Log

状态：`P28_11_COMPLETE`。完整回归与实际覆盖门已通过，最终结果见第9节。第5—8节保留当时的待完成状态与续接检查点，不代表当前仍在运行。

## 1. 入场

2026-09-19 00:23 heartbeat；分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `9ce0e8126eb3469681db3670f9dc0fb00f25861e`。读取最新P28.10 Report/Log、P阶段基线和有限冻结索引，未发现进行中的UE/UBT。最初进程查询无匹配返回非零，并非构建失败。

00:25:45.571UTC完整核验前阶段1617输入+103原未跟踪用户文件路径/哈希，暂存为空。保护Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`、Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26` 均未变，本地baseline.json留存。不重做P28.10已发布工作。

## 2. 可达路径与Red证据

RequestSettlementAndReload在Runtime终局成功后无条件DestroyRuntimeContainers，之后才进入Profile持久提交和DeactivateProfileWorld；前者重置容器、遭遇及Reward上下文，后者对真实战斗World拒绝虽返回false，但无法恢复已销毁的原宝箱。选择该具体上游次序问题，不同时改散落物解绑、容器自身Destroy确认或强制EndPlay。

扩展同一个ManagerSettlementContinuation测试，不新增注册测试或friend：Spawn实际LootChest并调用既有InitializeChest创建非空Runtime容器，记录有效ContainerId、原RunId、Manager.Chests与非零M01RewardRunId。保留P28.10直接/延迟持久提交、真实飞剑ROLE_SimulatedProxy销毁拒绝及非零时钟。新增断言验证终局调用、持久重试失败和World-only重试期间保持容器owner，最终恰好一次释放。

00:27:08.800UTC Red锁仅一个测试文件变化。RedBuild Editor构建4 actions / 10.50秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.11.r0.RedBuild/BuildEditor/20260919T002717004Z-a461aaa5/stdout.log`，SHA `66AD6DB3D2C2F3737F0B35281E4B2243BEA4BEA19A2933B78EFFC8931FBFA5A6`。

RedProof于00:27:48.982UTC退出，0 Success / 1 Fail、队列1、原生0、崩溃指标0。两变体共5条新容器保持断言失败，精确原句含“Terminal caller preserves original container until combat World release acknowledges”“Persistent retry failure preserves the pending container owner”“World-only retry preserves the pending container owner”。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.11.r0/RedProof/20260919T002728551Z-0ad26343/UnrealEditor.log`，SHA `98DB2601B110FEE3070D6BEFA40157F9764D098FDCB637D60C7083175C2CE60E`。原生0与run-state SUCCEEDED只表示进程结束，不能覆盖自动化Fail；没有删除首次失败日志。

## 3. 修复与锁定

Manager.cpp仅11新增/2删除：按原StartupMode/Flow条件计算局部bUsesProfileSettlement，Profile路由不提前DestroyRuntimeContainers，统一留给既有去激活完成端口；旧兼容路径不变。没有新增持久状态或释放接口。测试32新增/2删除，注册总数不变。00:28:27.425UTC锁定1617输入、103用户文件，恰好两个源码差异；git diff --check通过。

对容器保留的边界另作静态核验：TransferContainerItemToInventory要求Active Run；终局已Settled/Inactive时不能从保留投影重新取物。本轮不证明真实UI/输入流程，也不把容器显示内容等同于仍可消费的权威。下层Runtime的物品终局数学、清理内容和上层CloseSearchContainer不改。

## 4. 已结束的验证原件

同一exec53493串行运行Editor→专项7/5→Game→完整旧根1330→完整新根1430；各段前后核验锁定输入，不并发启动第二实例。

- Editor于00:28:46.432UTC完成，4 actions / 16.86秒，SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.11.r0/BuildEditor/20260919T002829241Z-f29bcad0/stdout.log`，SHA `88339003ED654F38963B9D9F71A4753C47EB5E8EE76125BC1E57FA3F7D06ECA5`。
- WorldLifecycle于00:29:07.571UTC完成，7/0、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.11.r0/WorldLifecycleFocused/20260919T002847168Z-c7bc7267/UnrealEditor.log`，SHA `887248EEF90149B64ED20F9529CB052101DB8450E367DCE98B01133A8A793DC3`。新增保持断言由Red失败变为成功，两个结算变体仍为同一注册用例。
- ProductFlow于00:29:28.023UTC完成，5/0、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.11.r0/ProductFlowFocused/20260919T002907656Z-1cf14825/UnrealEditor.log`，SHA `FD84E8E211B5853D7CF1DA15420DE39612A70DE355094DC8ECADE646C43D7B7D`。
- Game于00:29:56.667UTC完成，4 actions / 27.81秒，SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.11.r0/BuildGame/20260919T002928588Z-f02bba48/stdout.log`，SHA `C612633CC215B5ED21EC83075BED33222808C38A2EFBE32442E2EAF9A5ADFEFE`。
- 映射自检exec83250实际完成529/529，原生0；原件 `Saved/Automation/P28.11/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。输出与前阶段相同，但本阶段独立执行。

首次自检启动使用嵌套PowerShell字符串，外层展开$?导致“Missing expression after unary operator '!'”，包装进程退出1，测试脚本未执行。随后改用pwsh直接shell执行成功；这是命令解析错误，不是产品、自检断言或源码错误。工具输出人工转录于 `Saved/Automation/P28.11/selftest-launch-error.txt`，SHA `25A4021E075A61149B920EDE47B57867BDCE46D7AADE987F3F67D9DD3868D0C8`，明确是转录而非原生引擎日志。

## 5. 历史检查点：尚待结束与发布

两完整根、精确五路径覆盖门和最终文档核验未结束。静态映射为ProductRunItemUse+ItemProductAdapters，必跑7组：Shanmen.0_0_10、demo_map.Profile、demo_map.CodeB、demo_map.V2RangedCompatibility、Shanmen.0_0_10.Items、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar。新旧完整根将用于实际覆盖门，不能用当前专项或历史日志代替。

最终通过后只精确暂存两个源码、FoundationClosure_Index及本阶段Report/Log；103原用户文件和两份OverallReadiness不动。Saved原始证据只保留本地。当前不提交、不推送、不宣布底层完成；未启动Editor UI/PIE/Standalone、产品exe、输入、正式地图/内容、玩法/UI、Smoke/Cook/Package。

- [Report](../Report/Dev.D.UE.0.0.10.P28.11.r0_report.md)

## 6. 00:33续接检查点

原exec53493确认旧根1330/0、精确队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.11.r0/LegacyFullRoot/20260919T002957162Z-0c153459/UnrealEditor.log`，SHA `1183B35E1EBAE00D1624D23B6AC4D61CB3E603474EEF80A5E9D05C6B4DD693A3`。

同一pwsh47312已启动新根UE-Cmd50456，路径ShanmenFullRoot/20260919T003112939Z-072f8e82，Unattended/NullRHI，无Editor UI。00:33:46UTC快照585 Success / 0 Fail、崩溃指标0，尚无结束队列/最终run-state；不计算未结束日志的最终SHA，不重复启动。00:33:46.411UTC核验1617输入、103原用户路径/哈希、两份保护哈希及此前7项已结束原件SHA全部匹配，未跟踪精确105、暂存为空，3个文档链接与diff检查通过。旧根原件作为第8项新增，待新根结束后继续实际五路径覆盖与最终发布门。有限索引仍保持最新已完成P28.10，不提前标记P28.11完成。

## 7. 01:07 heartbeat续接检查点

分支和HEAD仍为9ce0e81，读取最新Report/Log、P/F基线及原续接记录。原exec53493仍运行；UE-Cmd50456/父进程47312、同一新根日志与Unattended/NullRHI参数均吻合，未重复启动构建或测试。01:08:49.701UTC快照1073 Success / 0 Fail、崩溃指标0，无结束队列、无最终run-state；01:08:45UTC的LifecycleAndPreflight已成功，随后继续OrderedAppliedReplay，属正常进展而非阻塞。

逐项复核1617锁定输入、103原未跟踪用户文件路径/哈希、两份OverallReadiness保护哈希、8项已结束原始证据及独立命令错误转录哈希均一致。未跟踪精确105，暂存为空；已跟踪差异仍严格限于两个阶段源码和两份用户文档，git diff --check通过。本轮仅更新状态文档，不改锁定输入、不执行最终覆盖门、不提交推送或开展下一阶段。原始失败证据继续保留，IN_PROGRESS不变。

## 8. 01:41 heartbeat续接检查点

入场仍为原分支/9ce0e81，已读取最新Report/Log和P/F基线。原exec53493、pwsh47312和UE-Cmd50456仍运行同一ShanmenFullRoot/20260919T003112939Z-072f8e82。01:42:35.445UTC快照1190 Success / 0 Fail、崩溃指标0，无结束队列或最终run-state；01:42:29UTC的UnknownOutcomeClosure已成功，随后进入AdoptionAuthorityFences。期间HTTP generate_204超时、大Tick间隔69.90秒警告保留；有实际新成功记录，不把正常慢进展计为失败或重启验证。

01:43:14.317UTC再次核验1617锁定输入、103原用户文件路径/哈希、两份保护文档、8项已结束原始日志与独立错误转录哈希一致；未跟踪精确105，暂存为空，3个相对文档链接存在，源码diff检查通过。仅更新进度与续接文档；不改产品、不执行未具备完整新根证据的覆盖门、不提交推送或宣布底层闭合。下一轮继续同一验证实例。

## 9. 02:17 heartbeat最终验证与交接

本轮续接原exec53493取得最终原生退出0，未重复启动测试。新根原run-state为SUCCEEDED/原生0，开始00:31:12.941UTC、结束01:45:15.414UTC；实际1430 Success / 0 Fail、精确结束队列1430、崩溃指标0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.11.r0/ShanmenFullRoot/20260919T003112939Z-072f8e82/UnrealEditor.log`，SHA `A8D792D4AA169EF5B8064BDB2A871454CD46CE3B18C63C4B8B94B6C35FE7408A`。重新读取旧根也确认1330/0、队列1330、原生0、崩溃指标0；两根合计2760项，专项不重复累加。HTTP、大Tick及SDK警告原样保留，不把长时执行描述为性能通过。

用本阶段两份最终原始日志执行现有Test-ShanmenRegressionCoverage：PASS Changed=5 Rules=2 Required=7 Logs=2；七组正是第5节列出的实际映射，不使用前阶段或专项日志冒充完整根。原件 `Saved/Automation/P28.11/regression-coverage.log`，SHA `A62BF33439BE45B82CF87E4D371425221748E06D3CABC301D2F5147D0FFFAD7F`。本Log共列10项已结束原生输出的路径/哈希，另有一项明确标注的命令错误人工转录。

02:21:14.826UTC再次完整核验1617锁定输入、103原用户文件及两份OverallReadiness保护哈希不变；未跟踪精确105（103用户+本阶段Report/Log），暂存为空。分支/HEAD仍为入场基线9ce0e81，远端同分支也仍为该提交。只修正文档为最终结果并更新有限索引，不改变已验证源码；FZ-1/2剩余入口仍待审计，FZ-3尚未开始，不将本阶段写成整体冻结。

02:24:19.300UTC发布前核验：10项已结束原生输出哈希和独立错误转录均匹配；三份阶段文档共70处相对链接全部有效；1617锁定输入与103原用户文件仍一致，git diff --check通过。已跟踪差异严格为本阶段两个源码+索引，以及不提交的两份用户文档。Git的LF→CRLF提示保留，不为消除提示重写用户文件。

本阶段精确发布清单：

- Source/demo_map/demo_mapV3ProgressionManager.cpp
- Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp
- Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md
- Docs/Report/Dev.D.UE.0.0.10.P28.11.r0_report.md
- Docs/Log/Dev.D.UE.0.0.10.P28.11.r0_log.md

提交身份由包含本Log的Git提交确定，发布后的远端HEAD另行核验，不在文件中伪造自引用提交号。原始Saved证据不上传；两份OverallReadiness用户修改及103份原未跟踪文件不暂存。仅无头契约验证与Editor/Game编译，没有进入任何F阶段操作。
