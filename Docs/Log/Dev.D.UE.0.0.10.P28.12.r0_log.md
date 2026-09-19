# Dev.D.UE.0.0.10.P28.12.r0 Development Log

状态：`P28_12_COMPLETE`。完整阶段验证已结束；第1—7节保留执行时序与中间检查点，第8节为最终结果，不以旧快照覆盖最终证据。不是整体冻结或F阶段验收。

## 1. 入场与保护

2026-09-19 02:58 heartbeat，分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `e01eaea99329e232d05124d9c1a4db8585445338`。完整读取P28.11 Report/Log、P/F基线与有限冻结索引；未发现运行中的UE/UBT。核验前阶段1617锁定输入和103原未跟踪用户路径/哈希全部一致。已跟踪差异仅两份OverallReadiness，暂存为空；不重做P28.11。

保护Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`，Red/Final锁定时核验一致。后续仅精确暂存阶段六路径，不包含上述保护文档或103用户文件。

## 2. 取证与首次失败

源码调用链：RequestSettlementAndReload→已接受持久终局→CompleteDurableProfileSettlementWorld→DeactivateProfileWorld→DestroyRuntimeContainers。P28.11只阻止战斗拒绝时过早进入容器清理；容器函数仍先Reset归属/奖励Run，忽略实际Destroy拒绝，返回整备成功后原pending消失。

读取UE5.8本机LevelActor.cpp的DestroyActor分支：Game World中的非Authority角色且无bNetForce/bNetTemporary会返回false。读取SearchContainerActor.EndPlay→Manager.NotifySearchContainerEndPlay，确认销毁回调会修改原数组，因此续接必须按快照遍历。没有修改引擎或系统权限。

沿既有真实Profile/cutover/Runtime/瞬态World/AuthGameMode测试，保留非零时间线、实际物品和原Run；直接/延迟持久成功两变体各增加两次实际宝箱Destroy拒绝，再恢复权限重试。没有新增注册测试、故障接口或friend。03:01:21.961UTC Red锁1617输入，仅测试文件变化。

RedBuild Editor于03:01:46.491UTC完成，4 actions / 23.64秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.12.r0.RedBuild/BuildEditor/20260919T030122506Z-8225ad86/stdout.log`，SHA `7D274BB68717324A464BA6E0EFC975172D0851A18E8000E88E2DD67508DE1232`。

RedProof于03:02:07.399UTC结束，0 Success / 1 Fail、精确队列1、原生0、崩溃指标0。两变体共8条断言失败：两次“Container destroy refusal retains original terminal owner after combat release”、恢复后终局完成、容器恰好一次销毁，各变体4条。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.12.r0/RedProof/20260919T030146944Z-16946bdd/UnrealEditor.log`，SHA `622321BFDE30C8D4E3B8556E0ED4A8AE7E47E6EF109C5FF9FDF6FBB8D966D1E5`。进程原生0不代表断言通过；首次失败未删改。

## 3. 最小修改与输入锁

Manager私有容器清理返回bool，以快照处理EndPlay回调，只留下拒绝的原弱引用；三类已有owner数组都处理完再决定是否完成，非短路丢弃其他可完成项。有拒绝则不清奖励Run/会话/相关上下文；全部接受才重置。DeactivateProfileWorld在局部灵石/World清理前检查结果，继续用原终局/技术回滚记录传递false。M01奖励重绑定检查同一结果；不扩大为通用恢复系统。

初始差异为测试27新增、Manager.cpp 29新增/26删除、私有头2新增/1删除。03:02:45.951UTC Final锁1617输入、103用户文件，恰好上述三处源码改变；git diff --check通过。锁定记录Saved/Automation/P28.12/validation-inputs.json。最终验证期间不再改变输入。

## 4. 验证过程（历史检查点）

exec57131串行运行Editor→WorldLifecycle7/ProductFlow5→Game→完整旧根1330→完整新根1430；每个阶段前后校验相同输入锁。不并发另启UE/UBT，不把未结束日志当最终证据。

映射自检exec56528实际完成529/529，原生0；原件 `Saved/Automation/P28.12/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。哈希与前阶段相同系输出确定，本阶段独立执行。

静态六路径（三源码+索引+Report/Log）映射为ProductRunItemUse、ItemProductAdapters两规则，共7组：demo_map.CodeB、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、demo_map.Profile、demo_map.V2RangedCompatibility、Shanmen.0_0_10、Shanmen.0_0_10.Items。最终实际覆盖门待两根完整日志结束后运行Saved/Automation/P28.12/coverage.ps1，不提前宣称通过。

03:06UTC同一运行器检查点：

- Editor于03:05:00.632UTC完成27 actions / 133.82秒，SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.12.r0/BuildEditor/20260919T030246427Z-e2cd8c2b/stdout.log`，SHA `B7ED7A012F33C011C32ACD236B91A2ED856A22ED9AC0FC65A8001F2A460CFF29`。
- WorldLifecycle于03:05:21.507UTC完成7/0、队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.12.r0/WorldLifecycleFocused/20260919T030501086Z-6e86a1ff/UnrealEditor.log`，SHA `68E658AD01B38AC0CBEDF36B7A6FF7116A7B68FA10AB14DF0CFDED747BD60938`。
- ProductFlow于03:05:41.974UTC完成5/0、队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.12.r0/ProductFlowFocused/20260919T030521589Z-8fb11357/UnrealEditor.log`，SHA `4B0B5F302DB2C1151F5090E803AC4F1EC4B12506F15CD00654646949C5E0FB04`。

原exec57131/pwsh39968继续Game编译，其后自动执行两完整根，未另启UE/UBT。Game及根组没有最终结果前不计算最终SHA、不称为通过。注册测试数没有增加，两种终局变体仍在同一注册用例中。当前共6份已结束原件哈希，后续验证沿同一实例续接。

03:08更新：Game于03:08:09.374UTC完成26 actions / 146.68秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.12.r0/BuildGame/20260919T030542388Z-030e2b76/stdout.log`，SHA `EE09F6667E2FCE7BE43CEF28D009C7E19A971C40E03C9D38352B8F6A0A81FCC7`。同一父进程39968已启动UE-Cmd48172，LegacyFullRoot/20260919T030809723Z-aad31d62，参数Unattended/NullRHI。此前6项原件加Game为7项，两完整根及覆盖门仍待结束。

03:08:20.918UTC复核1617锁定输入、103原用户路径/哈希、两份保护文档及此前6项原件哈希全部一致；未跟踪精确105、暂存为空、3处相对文档链接有效，git diff --check通过。HEAD保持e01eaea，尚未更新索引或提交推送；等待同一实例完成，不改输入或启动下一缺口。

## 5. 边界与后续

实际故障取证覆盖LootChest和原终局两种持久成功路径；其他容器类型的共用释放代码不冒充各入口均专项验证，奖励重绑定返回false只做静态核对。散落World物品、灵石/其他敌人释放、强制EndPlay及FZ-1剩余权限入口未因这项局部修复而关闭。等待验证期间索引仍以P28.11为最新完成阶段；最终更新见第8节。

未触及物理输入、正式地图/内容、玩法/数值/UI；未启动Editor UI、PIE、Standalone、产品exe、Smoke、Cook或Package。所有验证仅Editor/Game编译和UE-Cmd Unattended/NullRHI。原始证据本地保留，GitHub只交接完成后的精确阶段代码、Report和本Log。

- [Report](../Report/Dev.D.UE.0.0.10.P28.12.r0_report.md)

## 6. 03:41 heartbeat续接检查点

分支/HEAD仍为e01eaea，已读取本阶段Report/Log、P/F基线及原续接记录。原exec57131仍在运行：旧根于03:09:30.244UTC完成1330 Success / 0 Fail、精确队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.12.r0/LegacyFullRoot/20260919T030809723Z-aad31d62/UnrealEditor.log`，SHA `499949A741402E3D93F40C31A04C3AAFC79A36A70BA7129637A63A5036767F5A`。这是第8项已结束原件，不复用前阶段旧根。

同一pwsh39968下的UE-Cmd27820运行ShanmenFullRoot/20260919T030930559Z-b556cccf，Unattended/NullRHI。03:42:18.938UTC快照933 Success / 0 Fail、崩溃指标0，没有结束队列和最终run-state；03:42:18UTC的RetryReplayAndSuccess已成功，随后进入AudioExactBatch，有实际持续进展。

逐项核验1617锁定输入、103原用户文件路径/哈希、两份OverallReadiness及此前7份完成原件哈希一致；未跟踪精确105、暂存为空、git diff --check通过。已跟踪变化仅三份阶段源码和两份保护文档。本轮只更新续接事实，不改源码、不重启或另启验证、不执行缺少新根最终证据的覆盖门、不提交推送；索引和IN_PROGRESS状态不变。

## 7. 04:15 heartbeat续接检查点

已读取当前分支/HEAD、本阶段完整Report/Log及P/F基线。原exec57131/pwsh39968/UE-Cmd27820仍运行同一ShanmenFullRoot/20260919T030930559Z-b556cccf；04:16:18.590UTC快照1179 Success / 0 Fail、崩溃指标0，无结束队列或最终run-state。当前MaximumGenerationCapacity在04:16:17UTC仍输出World清理，前项HistoricalCheckpointReuseFence已结束；HTTP generate_204超时警告保留，不把有进展的长用例当失败或另启运行器。

再次逐项确认1617锁定输入、103原用户路径/哈希、两份保护文档及8项已结束原件SHA一致；未跟踪精确105，暂存为空，源码diff检查通过。仅更新本地报告/续接状态，不修改产品、不提前做覆盖门、提交或推送；IN_PROGRESS与有限索引状态不变，继续等待原验证实例。

## 8. 最终结果与发布范围

05:00 heartbeat续接原exec57131，已正常结束、外层退出0；没有重启UE/UBT或修改03:02:45.951UTC锁定源码。新根run-state为SUCCEEDED/原生0，结束于04:28:54.506UTC，1430 Success / 0 Fail、精确队列1430、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.12.r0/ShanmenFullRoot/20260919T030930559Z-b556cccf/UnrealEditor.log`，SHA `2F4CA6AEDAF0B28FB530BCA298D2664D1AB9F18B788943F75B3118C082F06C8D`。与本阶段独立旧根1330/0合计2760通过，不另计入已包含的专项7+5；注册数保持不变。

双根结束后实际执行coverage.ps1，PASS Changed=6 Rules=2 Required=7 Logs=2；两个完整原件均通过成功/失败/队列/退出/崩溃检查，7组均有对应证据。原件 `Saved/Automation/P28.12/regression-coverage.log`，SHA `89A0D919301D821C09A717CA1F4114B3D9476CE1A33CAD2C7FDC1D5BF3E9722B`。没有将静态选组当作实际测试，也没有借用前阶段日志。

05:04:43.765UTC再次核验1617输入、103原未跟踪路径/哈希及两份保护文档全部一致；独立重数旧根1330/0、新根1430/0，队列精确、原生0、崩溃指标0；覆盖原件哈希一致。暂存为空、未跟踪105（原103加本Report/Log）、git diff --check通过。远端分支仍为基线e01eaea99329e232d05124d9c1a4db8585445338。源码最终差异58新增/27删除，恰好三路径，内容与锁定验证相同。

最终文档将P28.12标记COMPLETE，索引只关闭本阶段已证明的终局容器拒绝保持路径，FZ-1/2不关闭。HTTP generate_204超时、SDK及大Tick等警告保留；不称无警告或性能测试。首次Red失败原件仍在，原生0不曾当作Red通过。

发布范围为三份源码、FoundationClosure_Index、本Report/Log共六路径；包含本Log的Git提交即阶段交接基线。只发布这六路径，不含两份OverallReadiness或103原用户文件；Saved原件留在本地。远端是否成功推送须以发布后ls-remote核验为准，不把提交操作等同于GitHub已更新。

05:09:02.253UTC最终文档核验：1617输入、103原用户文件、两份保护文档及本Log中10项原件SHA全部一致；索引/Report/Log共73处相对链接有效。已跟踪变化精确为三源码、索引和两份保护用户文档；未跟踪精确为原103及本Report/Log。Editor/Game最终run-state均SUCCEEDED/原生0，git diff --check通过。仅上述阶段六路径允许进入本次暂存和提交。
