# Dev.D.UE.0.0.10.P28.30.r0 Development Log

状态：`P28_30_AUDIT_COMPLETE`，仅完成生成来源接收/恢复/终局的契约审计，不是功能修复 PASS。日期2026-09-21 UTC。

## 1. 起点、范围和保护

20:42:21.532UTC heartbeat；分支 agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `2b52268df0dbe134d7ff4a713ecd39e0dade1813`。读取 P28.29 Report/Log、P阶段基线、冻结清单与Git状态。20:42:35.3616094UTC上一阶段Published核验通过：1617输入、103原未跟踪文件、2保护文档、十份原件SHA、141相对链接、暂存0；没有仍在运行的验证。

沿用 Saved/Automation/P28.29/validation-inputs.json，产品源码/测试/脚本/配置零改动。保护文档 OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。仅新建本阶段 Report/Log 并更新有限冻结索引；Saved 辅助脚本不提交。

## 2. 审计过程与证据边界

逐段读取 Manager 三个来源端口、LootChest 两类生成分支、Corpse/M01 包装和 SearchContainer 的已提交物化。继续核对旧 Coordinator 的 RunActive/RunId、ProfileRepository.SaveProfile 的写入隔离，以及 WriteFence.SameItemBearingRun 的 GeneratedRewardSources 比较。新文档存在时旧奖励写入被拒绝是正确隔离，不是应放宽的条件；Profile非物品进度另有允许边界。

检查 ShanmenItems Service/Repository 所有公开变更命令、Snapshot/ProcessedRequest、schema声明，以及 RunLifecycleAdapter.FinalizeSettlement 与 Repository.FinalizePreparedRun。已有 RewardMetadata 并不包含完整来源接收/恢复证据；现有 acquired 新物品若已在State.Items会拒绝，只有撤离成功候选才首次入库。这是本次新增的结构结论，避免后续把“接收先入图”直接接到“终局首次导入”而返工。Report第3节将接口、同文档迁移、产品读取物化、终局转换拆成有限实施顺序，未创建新生产端口/schema或更改经济规则。

搜索中有两类无副作用定位错误：不存在的 ProfileRunCoordinator/SearchContainerBase 文件名，以及 Windows 下向rg传字面通配路径；随后通过rg --files定位到 ProfileSessionCoordinator/SearchContainerActor并完整读取相关函数，没有据失败搜索宣称实现不存在。公开接口缺口结论来自实际头文件与调用体核对。

## 3. 本轮执行原件

任务 Dev.D.UE.0.0.10.P28.30.r0.Audit，由既有构建入口及隐藏 UnrealEditor-Cmd 顺序执行；每段后验证固定1617输入与103原文件，exec70580正常完成0。本轮无失败测试或中断，无新增Red；P28.29首次Red原件继续保留，由其Log追溯。

Editor：20:45:33.1872578–20:45:35.4065478UTC，up-to-date，0 action、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.30.r0.Audit/BuildEditor/20260921T204533150Z-6ce88220/stdout.log`，SHA `56677B39AACE6018D760E246AA55DB8E3D16A3466C7EDBE30BCDE26954E92CB5`。

Game：20:45:35.426396–20:45:36.5100985UTC，up-to-date，0 action、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.30.r0.Audit/BuildGame/20260921T204535423Z-b9465cc7/stdout.log`，SHA `F12D15CBF0A025EBFA109E6B3864961AB4C958FAFD5D76F2672802E070323D63`。只检查构建，不运行游戏程序。

CutoverFocused：20:45:37.0000448–20:45:57.3542483UTC，4/0、精确队列4、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.30.r0.Audit/CutoverFocused/20260921T204536988Z-cc0747c4/UnrealEditor.log`，SHA `D635B3054FF11A746CB635C61046574E4662F1379361DA5A16550041FF52B18D`。

RunLifecycleFocused：20:45:57.7485364–20:46:18.064111UTC，10/0、精确队列10、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.30.r0.Audit/RunLifecycleFocused/20260921T204557746Z-967b45b2/UnrealEditor.log`，SHA `FC06749B55CD0D7AA548EBB3CF522BA6DFAC97A5760462C8380C8D90FEEA9251`。

WorldLifecycleFocused：20:46:18.5074490–20:46:38.8303221UTC，7/0、精确队列7、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.30.r0.Audit/WorldLifecycleFocused/20260921T204618505Z-05617f1a/UnrealEditor.log`，SHA `98512976AC320F286949CBA7C6934B42C30A22FE8904AFE6B5C666DB0D4211E8`。三组共21个独立成功用例、Fail/Fatal/Ensure/Unhandled均0；不把M01拒绝保持测试解释为成功生成奖励。

## 4. 复用与精确交接

以下是P28.29已完成原件，不是本轮重新运行：新完整根1430/0、旧完整根1330/0，合计2760个独立用例；本轮21项为其子集，不另加到2760。复用前后1617产品/验证输入均一致。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Final/ShanmenFullRoot/20260921T174122243Z-17c2dbd9/UnrealEditor.log`，SHA `848B001954601A58D3969C01700C3FC6E1482FEE05A5E1F6EA75E7433F73D6A2`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.29.r0.Final/LegacyFullRoot/20260921T174006092Z-5d3a4264/UnrealEditor.log`，SHA `604AFC90EC00A13E9A2885E981BDC702251226EBBC6FF78622A1098C6D42E89D`。

映射检查器未改动，其537/537自检也明确复用P28.29。原件 `Saved/Automation/P28.29/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。

三文档映射分类通过：`REGRESSION_COVERAGE: PASS Changed=3 Rules=0 Required=0 Logs=0`；没有生产改动，因此无需对应UE组，此分类不等于产品回归。原件 `Saved/Automation/P28.30/regression-coverage.log`，SHA `92637BA9A2DD35B80B09B9D8046527FA74138760EFFB9D7069542C3A7D50A845`。

20:50:54.8736146UTC PreStage核验通过：1617输入、103原文件、2保护文档、九份原件SHA、156处相对链接一致；本轮21项、复用根2760项的精确队列/原生退出/子集关系符合记录，双目标确为up-to-date。暂存0、未跟踪105仅多本阶段Report/Log，git diff --check通过。随后只登记此核验并澄清索引最新审计/最近完整根的区别，重复核验后精确暂存三文档；发布后再检查父提交、路径集合和远端HEAD。

只精确暂存Report、Log、有限冻结索引三个文档。原始日志/辅助脚本留在本地，不宣称已上传原件。保持103原未跟踪文件和两份OverallReadiness用户修改；FZ-1/2仍开放，FZ-3等待，不暂停自动化，不进入F边界。

- [Report](../Report/Dev.D.UE.0.0.10.P28.30.r0_report.md)
- [冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
