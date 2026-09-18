# Dev.D.UE.0.0.10.P28.7.r0 Development Log

## 1. 入场

2026-09-17 23:18 heartbeat；HEAD `bc4210071b73fb013e47d6a3893ee149b8ace299`，分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。读取P28.6 Report/Log、共享P阶段基线和有限冻结索引。无运行中的UE/构建；index为空。23:20:50UTC复核1617产品/脚本输入及103原未跟踪用户文件与P28.6最终哈希一致，形成 `Saved/Automation/P28.7/baseline.json`。

保护OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`、Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`，不修改或暂存。

## 2. 只读审计

检查Manager所有Activate/Deactivate调用、StartPreparedProfileRun和SectNavigation的调用关系、RequestSettlementAndReload/RetryPendingProfileSettlement/ReturnToSectAfterSettlement/EndPlay，以及Flow取消、终局提交和重试前提。另查GameMode激活与退役、Framework启动协调器入口。

普通激活的三个失败分支均忽略Flow回滚接受状态，随后独立调用DeactivateProfileWorld并显示整备；Start结果无条件写“was rolled back”。本阶段仅处理此既有调用方缺口，不把终局、强制退出等其余候选同时扩大为新系统。强制EndPlay不能凭销毁进程内对象就认定持久终局完成。

## 3. 反例准备

既有ManagerRollbackRetention从两种变体扩展为四种：直接回滚/普通激活失败 × 健康/实际持久故障。保留原两种测试的非零数据和生产存档保护。普通激活时World无AuthGameMode，真实触发首个失败分支；无控制器，不进入UI创建或物理输入。其余两个失败分支将通过同一处理路径的静态检查覆盖，不冒称实际输入恢复已验证。

仅修改测试后，使用既有受跟踪构建/进程启动器串行执行Red Editor和RedProof，exec41737；证据保留于P28.7独立目录，不覆盖P28.6或首次失败。当前状态为IN_PROGRESS，尚未提交。

## 4. 实际Red与修复

Red Editor于23:22:48UTC完成，4 actions / 38.61秒 / native0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.7.r0.RedProof/BuildEditor/20260917T232209577Z-bfb88a12/stdout.log`，SHA `6BFA082EEB75B2470850CECB61B5C23B64C87FDA84DD1A1E1B13F6D05CA49184`。

RedProof于23:23:36UTC测试结束，实际0/1、精确队列1/native0、Fatal/Ensure/Unhandled指标0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.7.r0/RedProof/20260917T232248691Z-13eaa447/UnrealEditor.log`，SHA `9D2914C36BF89ED54759174F9219FF866A0F9FC2390CA2B85FF80F2BA395EB60`。一条健康回滚复合断言失败，及两次Manager集合/pending和World owner/绑定保持断言失败；原直接回滚、Runtime Run/三颗丹/生命1及持久字节断言未失败。native0不等于测试成功。

随后将普通激活三个失败分支接到局部FailActivation处理：标记pending → 调用已有RollbackPreparedProfileRunFor0909B → 仅成功显示整备；不另调Deactivate，不新建恢复记录/权威/schema。Start诊断改为明确回滚和释放尚须完成，不能声称已完成。新测试普通激活初始pending由true加强为false，要求处理自己建立锁；原直接回滚保持原初始值。对应真实失败的TechnicalRollback错误计数增加，产品断言未删减。

修复Editor及两组专项串行运行于exec2524；完成前不改验证输入或启动重复验证。终局/EndPlay及普通启动未完成清理后的外层恢复触发仍保留在有限审计内，不以本次调用方修补宣称全部生命周期闭合。

## 5. 修复验证与输入锁

- Fixed Editor于23:25:42UTC完成，5 actions / 26.35秒 / SUCCEEDED/native0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.7.r0.Fixed/BuildEditor/20260917T232516261Z-26d82e10/stdout.log`，SHA `AB81FE54B1624F83EA9EA013D7182A8F176D499D42F6707802B0FEFB01497026`。
- ProductFlow专项5/0、精确队列5/native0、崩溃指标0；四变体测试通过，未增加测试注册数。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.7.r0/ProductFlowFocused/20260917T232543019Z-32314594/UnrealEditor.log`，SHA `D31107958670FB42F80EDFE015CB72B870076718E69550B1DA209C6ADBBF29A9`。
- WorldLifecycle专项6/0、精确队列6/native0、崩溃指标0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.7.r0/WorldLifecycleFocused/20260917T232603565Z-4152ecb6/UnrealEditor.log`，SHA `AE1E28A0ECB85F342C4D05EC2D274A18496D7D412AB36ABCA582D8B90E28C22C`。
- Game于23:27:48UTC完成，4 actions / 44.77秒 / SUCCEEDED/native0；没有运行产品exe。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.7.r0.Validation/BuildGame/20260917T232703153Z-953675ba/stdout.log`，SHA `3D228B198877A21688738108F574CC9EA5FE500EDAF5DC0D742263C4A3236476`。
- 本阶段重新执行映射自检529/529/native0；原件 `Saved/Automation/P28.7/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。脚本未改，因此输出哈希与P28.6相同，但这是独立新执行原件。

23:26:38UTC捕获1617个最终产品/脚本输入和103用户文件至validation-inputs.json；相对入场仅Manager.cpp和ProfilePreparationFlowTests.cpp两处变化。静态检查普通Activate方法含3个FailActivation调用、0个直接Cancel、0个独立Deactivate；全文件旧“was rolled back”无条件描述为0。23:28:03UTC重算锁定输入及用户哈希全部一致，index为空，105未跟踪=103原文件+2阶段草稿。

原专项exec2524已退出0。后续exec20226 / pwsh runner18972串行执行Game（已完成）、完整旧根1330和完整新根1429；23:28UTC旧根部分329/0，无最终队列。不要重复启动，也不把运行中的计数或先前阶段结果当完整通过。终局与EndPlay及普通入口的外层续接触发仍须继续有限审计。

23:30UTC续查：完整旧根已完成1330/0、精确队列1330/native0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.7.r0/LegacyFullRoot/20260917T232749484Z-50120e9e/UnrealEditor.log`，SHA `DDE503C935EAA4DAA6686FD15B142788CABA0DE3C44A00996246D2A92F6FF724`。新根23:29:15UTC开始，UE PID20356、runner18972命令行匹配同一exec20226，目录 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.7.r0/ShanmenFullRoot/20260917T232915428Z-af83ed44`，部分389/0，无最终队列/run-state，崩溃指标0。git diff --check和三条阶段文档相对链接检查通过。保持锁定源码，继续等待原实例，不提前提交/推送。

## 6. 2026-09-18 00:03 heartbeat续查

读取当前分支/HEAD、Git状态、本阶段草稿、共享P阶段基线和恢复记录。原exec20226仍运行且未返回结束输出；00:06UTC核对runner18972和UnrealEditor-Cmd PID20356的实际名称/完整命令行，与上述唯一新根目录一致。00:07:09UTC部分909 Success / 0 Fail、Fatal/Ensure/Unhandled指标0，无精确最终队列及run-state.json；不据此认定1429完整通过，也未启动第二个UE或构建实例。

新根仍有HTTP generate_204探测超时及AutomationController大Tick间隔警告；保留原日志，不隐藏警告、不把外部探测超时误作产品测试结果。00:06:52UTC重算1617个锁定输入与103个用户文件全部一致，两份OverallReadiness保护哈希不变；105个未跟踪文件仍恰为原103个和本阶段Report/Log，未增删用户路径，暂存区为空。git diff --check通过（仅行尾转换提示）。本轮只更新阶段进度及恢复记录，保留IN_PROGRESS，不新增产品修改、不提交/推送。原实例结束后再联合核对退出码、完整队列、失败和崩溃指标，随后做文件驱动映射与最终审计。

00:40 heartbeat续查：00:41:11UTC同一新根已推进至1160/0，崩溃指标0，最新日志写入00:41:09UTC；仍无最终队列及run-state。原exec20226未结束，runner18972/UE20356名称与命令行保持匹配，没有重启。00:41:13UTC再次核验1617输入、103用户文件和两份OverallReadiness保护哈希全部一致；暂存区空、105未跟踪路径集合不变。维持源码锁定及IN_PROGRESS，未将运行进度当作阶段完成。

## 7. 01:39 heartbeat最终回收

原exec20226已正常退出0，未启动替代实例。新根run-state为SUCCEEDED，原生退出0，开始2026-09-17 23:29:15.4309922UTC，结束2026-09-18 01:07:00.6406976UTC；实际1429 Success / 0 Fail、精确队列1429、Fatal/Ensure/Unhandled指标0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.7.r0/ShanmenFullRoot/20260917T232915428Z-af83ed44/UnrealEditor.log`，SHA `3AEC7675085028644E231AFEF04E647173DDFB6172CE7CBA5B699AD9DE87CAE3`。

再次独立读取旧根run-state和日志：1330/0、队列1330、native0、崩溃指标0；23:27:49.4977003至23:29:15.0801679UTC。重新核验此前记录的8份原件哈希均一致；新根原件另外核验。新根墙钟约97分45秒，保留HTTP探测超时和大Tick警告，不把本次耗时当游戏性能或RAM/VRAM测量。没有将专项重复加入总用例数。

01:39:46UTC所有1617锁定输入和103用户文件哈希再次一致。运行精确5路径的Test-ShanmenRegressionCoverage检查：2个C++、索引、Report、Log；结果Changed=5 / Rules=3 / Required=6 / Logs=2，通过。六组为demo_map.CodeB、demo_map.ItemEconomySchema、demo_map.Profile、demo_map.V2RangedCompatibility、Shanmen.0_0_10、Shanmen.0_0_10.Items.ProductFlow；由本阶段完整新旧根覆盖，不按本轮主题裁剪。原件 `Saved/Automation/P28.7/regression-coverage.log`，SHA `25B3DFC5E9C6BAF04381E61AC60D2C7A70126B546F30040E688B4D139798480D`。

Report改为P28_7_PASS，索引更新已验证的普通激活失败范围，仍保留FZ-1/2待审计及FZ-3最终冻结条件。提交范围严格限于上述5文件，不纳入两份OverallReadiness用户修改、103原未跟踪文件或Saved原始日志。本阶段未改内容资产/输入/UI/玩法，也未进行F阶段验收。

01:43:10UTC最终发布前核验：1617锁定输入、103用户文件、两份保护文档全部哈希一致；105未跟踪精确路径集=原103+本阶段2文档，暂存区为空，无额外受跟踪改动。三份阶段文档57条相对链接通过，Log记录的10份原件SHA全部匹配，git diff --check通过（仅行尾转换提示）。普通Activate方法仍为3个FailActivation调用、0个直接Flow.Cancel、0个独立Deactivate；没有修改验证后的产品输入。接下来仅精确暂存5文件、常规提交/推送并核对远端提交；发布后的提交号由Git历史和交付链接追溯，不在本提交正文中制造自引用哈希。

- [Report](../Report/Dev.D.UE.0.0.10.P28.7.r0_report.md)
