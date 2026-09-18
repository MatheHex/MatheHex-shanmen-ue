# Dev.D.UE.0.0.10.P28.8.r0 Development Log

## 1. 入场与范围

2026-09-18 02:18 heartbeat，分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `1dacc52c1fdc528b4a3c702e95caa7ea6451d87a`。读取最新P28.7 Report/Log、共享基线和有限冻结索引；无正在运行的UE或构建，只有两份既有OverallReadiness用户修改，暂存区为空。

02:20:20UTC核验1617输入与103用户文件和P28.7最终锁完全一致、未跟踪路径集一致，记录Saved/Automation/P28.8/baseline.json。保护OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`、Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`，不修改/暂存。

## 2. 调用审计与反例

只读检查Manager Start/FromSect、Rollback、Activate、Initialize、ShowSectNavigation，SectNavigation公开转发入口，GameMode Prepare/Rollback及既有ManagerRollbackContinuation测试。普通Start在原World回滚pending时仅拒绝，框架Coordinator有续接；Initialize已有bUse0909BFrameworkHost路由标志且重复绑定不接受不同值。普通恢复不能跳过框架Coordinator/Adapter的原尝试确认。

测试先扩展为两种路由，保留原实际飞剑销毁拒绝、非零时钟、错误绑定拒绝、恰好一次清理、持久快照不变和下层提交次数1的断言。框架夹具设置正确初始化路由；普通变体不生成Controller。框架变体在解除故障后额外检查普通Start仍不能消耗其上层原尝试。普通变体使用底层回滚形成真实pending后进入公开Start续接，不冒充正式M01或UI端到端验收。

Red运行器exec7983使用既有跟踪构建/进程启动器。Red Editor于02:23:53UTC完成，4 actions、43.74秒、SUCCEEDED/native0；随后串行执行单条ManagerRollbackContinuation。当前Report仍IN_PROGRESS；首次失败原件必须保留，确认实际反例后才修复产品。

## 3. 实际Red与最小修复

Red Editor原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.8.r0.RedProof/BuildEditor/20260918T022309549Z-60a9b7ea/stdout.log`，SHA `086E2AA098D13849AB09DAD55DF84FEE18A4A1AC096792B9700FE355FC102A61`。

RedProof于02:24:19UTC原生退出0，实际0/1、精确队列1、Fatal/Ensure/Unhandled指标0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.8.r0/RedProof/20260918T022353762Z-20f3abef/UnrealEditor.log`，SHA `6AA08DCB89E8C2735E8F053D459DD731B26538AD8167D8F5A0A7B95201DCBB31`。五条实际失败断言是普通入口在故障解除后未完成清理、未给完成诊断、飞剑/敌人销毁次数各为0而非1、Manager/GameMode上下文未清。框架原尝试、非零状态保持、下层提交次数和持久快照断言未失败。native0仅是进程结果，不是测试通过。

Red运行器完成后，在StartPreparedProfileRun的既有Pending分支增加12行：普通路由调用已有RollbackPreparedProfileRunFor0909B，用真实诊断/快照返回，仅完整成功调用原ShowSectNavigation；无论是否完成，本次都返回SessionNotReady，后续请求才可能进入新的Run。bUse0909BFrameworkHost为true时维持拒绝，不窃取上层Coordinator尝试。未动其余正常启动流程、回滚实现、接口或schema，也未削弱产品断言。

02:25:47UTC捕获validation-inputs.json：1617最终输入+103原用户文件，相对入场仅Manager.cpp与ShanmenPreparationAdapterTests.cpp变化。固定后串行exec27970执行修复Editor及ProductFlow5/WorldLifecycle6专项；独立exec47355执行映射自检。不在验证过程中修改源码或重复启动UE。

## 4. 修复后专项与串行完整验证

exec27970已退出0。Fixed Editor 4 actions / 22.39秒 / SUCCEEDED/native0，02:25:48.416至02:26:11.160UTC；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.8.r0.Fixed/BuildEditor/20260918T022548381Z-ec2feecd/stdout.log`，SHA `6376BEA1A27CF22C72C9863AACEA24C39A15AFE63F3A0ABFFF6F0A4E3A287AC9`。

- ProductFlow专项5/0、精确队列5/native0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.8.r0/ProductFlowFocused/20260918T022611223Z-988f2df6/UnrealEditor.log`，SHA `34152C32CABDDF7852281162743AD113FF4E75162AA361F6D8D5FCC6E789F28F`。
- WorldLifecycle专项6/0、精确队列6/native0、崩溃指标0，两种Manager续接变体都通过；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.8.r0/WorldLifecycleFocused/20260918T022632150Z-caaf8103/UnrealEditor.log`，SHA `85B488A58AE804C6CBC79F67067BDDF89A947692142CA68674F8EBC6B60E9E7C`。
- 本阶段独立重跑映射自检529/529，exec47355退出0；原件 `Saved/Automation/P28.8/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。未改脚本，确定性输出与前阶段同哈希，不是复制日志替代新执行。

02:27:54UTC启动原exec77410 / pwsh runner51224按Game→完整旧根1330→完整新根1429串行执行，未重复启动Editor或专项。02:28UTC重算1617最终输入及103用户文件全部匹配。

Game于02:28:40.822UTC完成，4 actions / 45.55秒 / SUCCEEDED/native0，只构建未启动产品exe。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.8.r0.Validation/BuildGame/20260918T022754903Z-ca195763/stdout.log`，SHA `1A7D407DED1AAE697ED0BAC51A85D4BD467932625DC1D88077965E0FED826C9D`。

完整旧根于02:30:07.101UTC结束，1330/0、精确队列1330/native0、崩溃指标0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.8.r0/LegacyFullRoot/20260918T022841567Z-28c481e8/UnrealEditor.log`，SHA `955D93F2B152B26B81B7903E3C76A41190E2C50CCC2347EE4E9B5114168B42BE`。原串行运行器随后启动完整新根，UE PID15740命令行匹配 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.8.r0/ShanmenFullRoot/20260918T023007400Z-97fc4494`。无最终队列/退出证据前保持IN_PROGRESS；冻结索引尚未更新，不提交/推送，不开展其他生产变更。

## 5. 03:03 heartbeat续查

03:06UTC原exec77410仍运行，pwsh51224和UnrealEditor-Cmd15740命令行与既有串行验证匹配。新根日志已记录906 Success / 0 Fail、Fatal/Ensure/Unhandled指标0，尚无精确最终队列、run-state.json或原生退出结果；最后完成项为RecordOrderChangesEnvelopeIdentity，随后RestoredJournalAcceptsOneFreshCommand已启动。HTTP generate_204超时和较大Tick间隔警告原样保留，不因等待而重复启动验证、终止进程或修改锁定源码。

03:07:01UTC重新核验1617输入与103用户文件全部一致，两份OverallReadiness文档保持入场SHA；未跟踪路径集精确为103用户文件及两份阶段草稿，暂存区为空，git diff --check通过（仅换行转换提示）。本次只更新阶段进度文档及忽略的恢复记录；最终映射、索引收口与提交推送仍待本轮完整新根结束。

## 6. 完整回归结束与19:07 heartbeat证据恢复

原新根已于2026-09-18 04:12:02.5824935UTC结束，1429 Success / 0 Fail、精确队列1429、Fatal/Ensure/Unhandled指标0；同目录run-state.json为SUCCEEDED、exit_code=0，启动参数与锁定的新根一致。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.8.r0/ShanmenFullRoot/20260918T023007400Z-97fc4494/UnrealEditor.log`，SHA `CEAB2B581C9F906441CF7A98A0B2392813E3590209D405C5391432947E216A1F`。新旧完整根合计2759成功，专项不重复计入。

19:07UTC恢复时，原pwsh/UE进程已不存在；外层exec77410查询返回Unknown process id，不能补写该外层退出码。改为读取原生run-state与完整原始日志，重新独立核验既有8份日志哈希、各构建/测试原生退出结果及输入锁，不重跑已完成验证。HTTP generate_204超时、大Tick间隔及非目标平台SDK探测信息保留；Win64构建成功不代表其他平台SDK齐备，也不是性能验收。

19:08:43UTC核验1617锁定输入、103用户文件和两份OverallReadiness文档SHA均不变；105个未跟踪路径恰为103用户文件+本阶段Report/Log，暂存区为空。精确五路径覆盖门通过：Changed=5、Rules=2、Required=7、Logs=2。必跑组是demo_map.CodeB、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、demo_map.Profile、demo_map.V2RangedCompatibility、Shanmen.0_0_10、Shanmen.0_0_10.Items，由本轮完整两根实际日志覆盖。原件 `Saved/Automation/P28.8/regression-coverage.log`，SHA `B7D2118D3EC903E86B8FDBED3986F0387E5FF566E459F3598AFFB23F763000DF`。

## 7. 交付范围与后续

阶段交付精确五文件：Manager.cpp、ShanmenPreparationAdapterTests.cpp、本Report/Log、FoundationClosure_Index。索引只关闭普通Start外层续接，FZ-1及FZ-2终局/强制EndPlay仍待审计；当前不是FZ-3最终冻结。提交前检查相对链接、git diff --check、精确暂存集合和用户保护哈希，提交后核验同分支远端提交一致；提交ID由本Log所属Git提交追溯，不把用户修改或Saved原件上传。

19:11:36UTC交付前复核完成：三份阶段文档共61个相对链接有效，Log中10份原件SHA逐一匹配，1617输入锁与103用户文件、两份保护文档不变，未跟踪精确105项，暂存区为空，git diff --check通过。两源码实际差异为Manager +12行，既有测试+197/-148（两变体循环的缩进包含在其中，无新增注册项）；索引只更新证据与有限剩余项。

- [Report](../Report/Dev.D.UE.0.0.10.P28.8.r0_report.md)
