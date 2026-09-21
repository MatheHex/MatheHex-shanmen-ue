# Dev.D.UE.0.0.10.P28.24.r0 Development Log

状态：`P28_24_PASS`。实际Red、首次修复、Editor/Game双构建、专项及完整双根、实际改动驱动回归检查完成；下文按发生时间保留运行中检查点，最终结论见第6节。不是整体框架冻结或F验收，提交/远端状态由本阶段Git交接另行核验。

## 1. 入场与保护

2026-09-21T00:10:58.183Z heartbeat。当前分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `81464d6780b833e7333c72a35a115464c542e1d2`。完整读取P28.23 Report/Log及P阶段基线，核对Git状态。00:11:16.0505307UTC上一阶段Published验证通过：1617输入、103原用户精确集合/哈希、两份保护文档、11原件SHA、114相对链接、2760独立完整根、暂存0，无重复UE-Cmd。

保护：OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。103原用户路径和哈希沿用P28.23锁，不修改或暂存。

## 2. 路由与Red

静态读取普通拾取、空间包丢弃/回收、Authority.DiscardSpatialBundle/RecoverSpatialBundle、WorldItem.RequestInteract/CanInteract/EndPlay、TeardownWorld、结算及World不变量。普通回收通过RequestInteract直接可达；三对象销毁不可用P28.23单对象快照回滚冒充原子恢复。

仅扩展既有PreparedWorldPickupIdentity；00:14:56.7005852UTC Red锁1617输入/103原用户，只改测试。exec63013执行Editor→单项Red，外层正常0是执行链结束，不等于测试成功。

Red Editor于00:15:11.0064408UTC完成、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Red/BuildEditor/20260921T001457255Z-a112b3a4/stdout.log`，SHA `A11823E8E505982EA64E464BDECE3A0BAC9372B7B5E4BA19CA77355F594438E1`。

Red于00:15:31.9312039UTC完成，0成功/1失败、精确队列1、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Red/RedProof/20260921T001511509Z-a08d23f7/UnrealEditor.log`，SHA `C0CE49B72BE657007D44FB6FE25379699631FD1C609881B78BB05A589416845A`。八条最终Expected失败：两次拒绝各两条（整包不报成功、原拒绝绑定/可续清理保持），竞争丢弃门、故障解除后恰好一次续清理、完成重放，以及后续持久撤离。这里的撤离失败来自旧实现允许竞争丢弃后丢失原整备背包，不将它冒充后来终局变体的独立Red。

## 3. 首次修复及新增检查

原Runtime的PendingSpatialRecoveries只存原BundleId接受回执，不存另一份数量/库存。逻辑成功和修饰同步完成才登记；重试直接使用回执，不再次执行Authority.RecoverSpatialBundle。成功释放原对象只去其绑定，拒绝对象保留原映射；RemoveWorldBinding在最后一个对象释放时同步清包/回执，ClearSpatialBundleTracking也清该表。World不变量允许由该已接受包精确关联的非权威待释放投影，不把它重新算作World物品。

普通Pickup入口与CanInteract只读资格识别该待释放状态，仍要求原绑定与距离；空间回收检查同World的Pawn，禁止旧回收未释放时新空间包丢弃。未修改提示、资产、物理输入或玩法规则，没有新的生产故障端口。公开新增一个只读资格查询，不是新的物品写端口；有界瞬态接受回执不是跨进程恢复协议。

测试保留原普通单件拒绝/合并/持久化断言，扩展真实三对象包：两次拒绝、两个成功前缀、原持久Run不变、修订不重做、竞争包拒绝、解除后恰好一次完成及完成重放拒绝。Red之后追加再次丢弃/部分回收→真实Runtime结算及durable finalize→Teardown仍拒绝→原对象恢复后释放的组合，此变体没有独立Red。

四源码变化中生产三文件83新增/16删除，既有测试94新增/2删除，无新增测试注册。回归映射加入WorldItemProjection；脚本自检新增cpp/h分别验证正向以及三个缺组失败，共8项。

映射自检exec61264正常0、537/537通过；原件 `Saved/Automation/P28.24/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。仅证明映射脚本，不冒充产品测试。

## 4. 锁定完整验证与续接

00:18:24.2967041UTC Final锁1617输入及103原用户：修改四源码和两验证脚本，除此无漂移。exec21699运行Saved/Automation/P28.24/run.ps1 -Mode Full，顺序Editor→PickupFocused1→WorldLifecycle7→ProductFlow5→Game→LegacyFullRoot1330→ShanmenFullRoot1430，每段前后检查锁。最终输入保持不变，首次Red与Final证据分开，不覆盖原失败。

初始检查点Editor仍在构建48动作，后续完成结果如下。只续接原进程，待健康完整根后再运行实际coverage。九路径覆盖两规则，必跑五组为demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、demo_map.V3.WorldInteraction、demo_map.M01Extraction、Shanmen.0_0_10.Items；期望完整两根共2760唯一用例，13专项是子集，必须以实际最终队列/原生码/失败/崩溃标记联合核验。

Final Editor于00:21:30.6728061UTC完成，48动作/185.44秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Final/BuildEditor/20260921T001824844Z-29d9ac98/stdout.log`，SHA `59A29025276ADF4B7AB90AA1301C13775961E6E22AA60B05B47E9854AB9EAA7B`。

Final拾取专项于00:21:51.5398497UTC完成，1成功/0失败、精确队列1、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Final/PickupFocused/20260921T002131124Z-353af031/UnrealEditor.log`，SHA `3358DD988C67D07DCAF10630F8F45BD5115FE5313B5C470B7E56958362C8A8F8`。原拒绝反例、竞争丢弃门、原对象续清理及新终局变体断言全部执行成功；不把新增终局变体说成另有Red。

00:21:51.3061191UTC检查点通过1617锁定输入、103原用户精确集合/哈希、两份保护文档、当时3项已完成原件SHA、4个相对链接、暂存0、未跟踪105和diff check；有限索引仍未改。该检查不替代剩余最终产品验证。

当时尚未更新有限冻结索引、暂存、提交或推送。保存首次失败与完成原件路径/SHA，运行中原件不计最终SHA。FZ-1/2开放，不进入F；Saved日志本地保留，不称已上传GitHub。

Final WorldLifecycle于00:22:11.9783485UTC完成，7成功/0失败、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Final/WorldLifecycleFocused/20260921T002151630Z-1f0b8347/UnrealEditor.log`，SHA `7CAFD0A385F910C2B069250F7AC281B73F3E17BEE108B91C893CC53D916BA036`。

Final ProductFlow于00:22:32.4610022UTC完成，5成功/0失败、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Final/ProductFlowFocused/20260921T002212082Z-cc547fef/UnrealEditor.log`，SHA `65CF164D9E80176FDBBA56598F0D195E1E1D6F2008DACBA79DB18F533E2B810D`。

00:23:18.6215839UTC保护核验再次通过1617输入、103原用户精确集合/哈希、两份保护文档、当时5项原件SHA、4相对链接、暂存0、未跟踪105、diff check，有限索引未改；七份本轮本地执行/检查脚本解析无错误。Game仍在同一exec21699编译，后续完整根尚不计为成功，下一次heartbeat续接原执行链。

- [Report](../Report/Dev.D.UE.0.0.10.P28.24.r0_report.md)

00:24:49.7593503UTC补齐专项证据后复核通过：1617输入、103原用户精确集合/哈希、两份保护文档、7项原件SHA、4相对链接、暂存0、未跟踪105、diff check，有限索引未变。Game此时35/47动作，原exec21699继续运行；不是阶段完成或发布。

## 5. 2026-09-21T00:57:28.898Z heartbeat续接

先读取当前分支/HEAD、Git状态、本Report/Log、续接记录与P阶段基线，再续接原exec21699，没有重新构建或重复启动测试。01:00:25.7966383UTC检查点通过1617锁定输入、103原用户精确集合/哈希、两份保护文档、当时7项原件SHA、4相对链接、暂存0、未跟踪105、diff check；有限索引仍未改。

Final Game于00:25:41.8144517UTC完成，47动作/188.66秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Final/BuildGame/20260921T002232863Z-de3318ea/stdout.log`，SHA `58708E275FE1E7E1E8A5368877AAAE114CC7C1819AB7064FCF268D1682A2DDB8`。只进行Game目标编译，没有启动产品可执行文件。

Final完整旧根于00:27:07.6885627UTC完成，1330成功/0失败、精确队列1330、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Final/LegacyFullRoot/20260921T002542200Z-a735bf2e/UnrealEditor.log`，SHA `E45C6DB65EE1A476EEC5E1FF046721A81FB317F17C3E91D2002D956869C1CC58`。

完整新根继续写入Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Final/ShanmenFullRoot/20260921T002708010Z-1ce5ddea/UnrealEditor.log。01:00:50.3127296UTC读取到1018成功、0失败、0崩溃指标、尚无最终队列；原PID41996自00:27:08UTC持续运行，exec21699未结束。运行中原件不记最终SHA，不把尚缺的退出码/队列或预计1430项宣称通过。实际覆盖检查与发布继续等待该原执行完成；输入未改变，不启动新一轮验证或另一个开发阶段。

2026-09-21T01:34:29.551Z heartbeat再次按分支/HEAD、Git状态、Report/Log和P/F基线恢复原exec21699。01:35:03.8221376UTC保护检查通过1617锁定输入、103原用户集合/哈希、两份保护文档、9项完成原件SHA、4相对链接、暂存0、未跟踪105、diff check，索引未改。01:35:03.9146765UTC完整新根已1189成功、0失败、0崩溃指标，尚无最终队列/退出码；日志持续推进、原PID41996存活。中间日志可见HTTP探测超时警告，但相关测试已成功；不将警告伪称失败，也不修改环境或重启测试。完整根与实际覆盖尚未结束，本轮继续保留原验证，无新的源码变更、暂存或发布。

## 6. 完整验证结束及精确交接

2026-09-21T02:10:30.102Z heartbeat先读取分支、HEAD、Git状态、Report/Log及P阶段基线；续接原exec21699得到全链正常结束、外层0。02:11:02.3613149UTC检查点通过1617输入、103原用户精确集合/哈希、两份保护文档、当时9项原件SHA、4相对链接、暂存0、未跟踪105、diff check。

Final完整新根实际于01:38:04.9990257UTC完成，1430成功/0失败、精确队列1430、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.24.r0.Final/ShanmenFullRoot/20260921T002708010Z-1ce5ddea/UnrealEditor.log`，SHA `1A43CA24808BF2D2F8975F3FC91602F505D268EA781763D24D6D246A09F3FCCE`。新根约71分钟，未中断或重启。双根共2760独立成功用例，13项专项只作为针对性证据，不重复相加。

本轮两份健康完整根实际覆盖检查通过 `Changed=9 Rules=2 Required=5 Logs=2`；原件 `Saved/Automation/P28.24/regression-coverage.log`，SHA `5D1C8D21491E92285EA904AE1A27DDB462B735194DA8A5F3387C5736A6A828FE`。五必跑组均有成功证据：demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、demo_map.V3.WorldInteraction、demo_map.M01Extraction、Shanmen.0_0_10.Items。

仅补写完成证据和有限索引，不更改锁定产品/验证输入。交接精确九路径：ItemSubsystem.cpp/h、WorldItem.cpp、ShanmenPreparationAdapterTests.cpp、ShanmenRegressionMap.json、Test-ShanmenRegressionCoverageSelfTest.ps1、FoundationClosure_Index、本Report/Log。原始11项SHA与首次Red保留本地；GitHub交接报告及日志索引，不声称原始Saved日志已上传。FZ-1/2保持开放，后续限于既有入口可达性/空间包生成回滚等剩余证据；不进入F，也不因本阶段通过宣称全框架闭合。

02:14:48.1385825UTC最终PreStage检查通过：1617产品/验证输入、103原用户文件精确集合/哈希、两份保护文档、11原件SHA、118相对链接、7次健康Final执行、完整双根2760独立用例、实际覆盖及537自检摘要、无残留UE-Cmd、diff check；暂存0、未跟踪105（103原用户加本Report/Log）。接下来只暂存上述九文件，核验暂存范围和差异后提交并正常推送，不强推。提交前后继续核对用户文件保持；本日志不提前虚构提交哈希或远端已更新。
