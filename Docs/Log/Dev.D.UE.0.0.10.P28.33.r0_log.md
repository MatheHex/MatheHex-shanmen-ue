# Dev.D.UE.0.0.10.P28.33.r0 Development Log

## 1. 基线与边界

2026-09-21 UTC；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`；父提交 `dedff598ec7b14f14af647056af8f37c239686e3`。开始读取P28.32 Report/Log、Git状态和P阶段基线；上轮Published核验通过。工作区原有103未跟踪文件及2份用户改动，不覆盖或收编。实施结论见 [Report](../Report/Dev.D.UE.0.0.10.P28.33.r0_report.md)。

本轮12源码（11修改+1新增RepositoryGeneratedSource.cpp），加Report/Log/冻结索引共15路径。只接同一Repository/Service/Document的来源持久命令和查询，不改Manager/World产品入口、正式内容或玩法。辅助执行/核验脚本和原始运行输出仍在忽略的Saved中，不推送原始日志或将其伪称为GitHub附件。

## 2. 输入固定与用户文件

最终锁Recovery2含1623项产品/验证输入；前阶段1622项中11项变更、1611项保持，新增1源码。最终双构建、三组测试前后逐文件核对该锁；早期两份锁只追溯失败/修正过程，不要求其旧代码哈希等于最终工作区。

原件 `Saved/Automation/P28.33/validation-inputs-Durable.json`，SHA `4F5B04E8CF862C28FC78B8740B7A68155B5BEE44E07AB58862634B2E392AB814`。

原件 `Saved/Automation/P28.33/validation-inputs-Recovery1.json`，SHA `53EEA738A245994C5BD9274013EDA0762F427979E1BC8C12DF19F811CF859D90`。

原件 `Saved/Automation/P28.33/validation-inputs-Recovery2.json`，SHA `BFF2611A03C9E1456BDFC721185771A99D997199816871E7787209EB7B3EE16F`。

103原未跟踪文件逐文件比SHA保持。两份原有用户修改不编辑、不暂存：

- `Docs/Report/Dev.D.UE.0.0.10.OverallReadiness.r0_report.md`：`3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`。
- `Docs/Log/Dev.D.UE.0.0.10.OverallReadiness.r0_log.md`：`A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。

## 3. 首次失败、中间修正与最终双构建

通过既有 `Invoke-Shanmen.ps1 -Action BuildBoth` 执行Win64 Development。首次单并行，修复轮使用已有参数MaxParallelActions=4；没有修改仓库构建器。正常原生退出码与主动取消、输入一致性门拒绝分别记录，所有原件独立保留。

首次 Editor：编译日志记录测试块误入局部条件编译段的 C++ 错误。定位修正后主动取消该构建以重新验证；进程退出 -1 是取消，不是正常编译器结束码。没有执行Game或测试。

UTC 2026-09-21T23:34:33.1475538Z — 2026-09-21T23:36:37.2007399Z。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Durable/BuildEditor/20260921T233433098Z-6f1d6d36/stdout.log`，SHA `407CF6E4894D3D7D6C0891FEE78B6394D02651EAF8EF6A4A4FC89DF115C2C787`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Durable/BuildEditor/20260921T233433098Z-6f1d6d36/run-state.json`，SHA `EBD6BBD9A6B9B9430BC205CF5EFC55CAEFC9D6E3BABDAFF2789E775C1A4E40F1`。

中间 Editor：299 actions，原生0。

UTC 2026-09-21T23:36:38.600855Z — 2026-09-21T23:39:55.6077769Z。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery1/BuildEditor/20260921T233638564Z-d68c777f/stdout.log`，SHA `836002DD6BAB914D47E48EF13A48A56EB123D0DBB540A6BBA7E93EAA4DE6E2DC`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery1/BuildEditor/20260921T233638564Z-d68c777f/run-state.json`，SHA `550EBBBFB9E219FCD823225BCC7CE9A545365FB78DF7EFCF48387D3E4F1D8F53`。

中间 Game：328 actions，原生0。构建期间对Repository复制前条目上限、测试夹具自引用/前置失败保护、旧schema诊断文字作最后修正，三个文件不同于该轮输入锁；验证门在启动测试前拒绝继续。因此这轮不作为最终输入一致的验收。

UTC 2026-09-21T23:39:55.6256036Z — 2026-09-21T23:43:45.9833792Z。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery1/BuildGame/20260921T233955622Z-0d4d8596/stdout.log`，SHA `C8D6A62D42A43C1E0D25A3A17AB490F392227298DF7D7285E869D0280555E4F5`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery1/BuildGame/20260921T233955622Z-0d4d8596/run-state.json`，SHA `435407DF11906AA204833972BAB25F75F0DC6CEF7FA4722B707EBC03074CB57A`。

最终 Editor：6 actions，编译/链接实际执行，原生0。

UTC 2026-09-21T23:43:51.5830393Z — 2026-09-21T23:43:58.7150439Z。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/BuildEditor/20260921T234351546Z-a08e5a8d/stdout.log`，SHA `75FDF1F711A9CDEACA6E82C4E55970135114A9F6DFC3905406B41B44081DA3E9`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/BuildEditor/20260921T234351546Z-a08e5a8d/run-state.json`，SHA `81581B25A8C8928D49DD6B4470FB0D8DA6F8ACADC08ADF666D452497440F3AE0`。

最终 Game：4 actions，编译/链接实际执行，原生0。

UTC 2026-09-21T23:43:58.7318824Z — 2026-09-21T23:44:11.670644Z。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/BuildGame/20260921T234358729Z-32325779/stdout.log`，SHA `F44D4AF1A2F0E3DA618D8C47DC789589904F818BED909CDEC538AD43A7AB9020`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/BuildGame/20260921T234358729Z-32325779/run-state.json`，SHA `2F3AA7429CBD91187EAA29DAE7732DE86435F4DDC0093B69F6775FDE94F5CBBB`。

## 4. 无头自动化

最终统一使用UnrealEditor-Cmd，`-Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile`，指定RunTests组及 `-TestExit=Automation Test Queue Empty`。三个进程均SUCCEEDED/native0；成功总数、唯一名称数、唯一结束队列数精确相等，无Result=Fail/Fatal/Ensure/Unhandled/Assertion。

最终来源专项：Shanmen.0_0_10.Items.GeneratedSource，13 Success / 0 Fail / queue13，原生0。

UTC 2026-09-21T23:44:12.1200416Z — 2026-09-21T23:44:37.4895734Z。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/GeneratedSourceFocused/20260921T234412098Z-644a6264/UnrealEditor.log`，SHA `F2FBF1CDAC9E5326BEF6D0EF2A027409D2F6414F621C15D783A0DB70F8DAD3B1`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/GeneratedSourceFocused/20260921T234412098Z-644a6264/run-state.json`，SHA `B147F802DFCC9B76156C11C626F54362EECCEBC32A751865C8A3371366EECE6F`。

最终物品整组：Shanmen.0_0_10.Items，100 Success / 0 Fail / queue100，原生0。

UTC 2026-09-21T23:44:37.9398982Z — 2026-09-21T23:45:08.2694225Z。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/ItemsRegression/20260921T234437937Z-d63adb5b/UnrealEditor.log`，SHA `E334C3B3F9BE013901274BAFF173CB12A8C2A69D2CD06718DCE43B09981DEDF0`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/ItemsRegression/20260921T234437937Z-d63adb5b/run-state.json`，SHA `48EE978A36462296AF8E17A8C7F0EE7988E50EC9AF048E4280AD1523D45CAF77`。

最终旧系统全根：demo_map，1330 Success / 0 Fail / queue1330，原生0。

UTC 2026-09-21T23:45:08.6994417Z — 2026-09-21T23:46:24.1038346Z。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/LegacyFullRoot/20260921T234508697Z-85c55285/UnrealEditor.log`，SHA `7FB950E6B7C5EC8A84AAAFAB6E4D2235D1243B3C0AD757958EBF94D3BA12C665`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.33.r0.Recovery2/LegacyFullRoot/20260921T234508697Z-85c55285/run-state.json`，SHA `2B97D962819FFDBCDB4BD1FB4EA22A0BEE881E6A9DEC99A4E897E56F1EC20D4A`。

独立成功100+1330=1430；来源专项13是Items子集，不重复相加。1430不是整个Shanmen根的计数，本轮未运行该完整根；P28.29只作历史证据。三份最终无头日志各有13条既存 `LogAutomationTest: Error: Condition failed` 启动输出，不称为整份日志零Error。最终测试首次运行即通过，没有覆盖失败测试日志。

## 5. 本轮六项新注册测试

- DurableRestartAndTerminalReplay：同库存文档接收两来源，pity 0→7→8、cursor 0→1；UINT64_MAX、INT64_MAX、2^53+1真实写盘后新服务实例逐字段恢复；旧来源重放无写入；Abandon后保留历史但拒绝新来源，错误owner/Closed读取清空输出。新实例在同一无头进程内，不冒充OS断电或整个World跨进程恢复。
- DurableFailureAndReconcile：WriteTemp、ReadBackTemp、PrepareBackup、AtomicReplace四处失败均保持旧文档字节/状态且来源不可见；ReadBackCommittedPrimary不确定结果由重开确认后只提交一次，重试重放。
- DurableGuardsAndSnapshotIntegrity：9个错误请求与6个来源/回执快照破坏变体；数量、保底和预算基准非零，失败不修改文档。
- DurableConcurrentAcceptance：8个异步同来源请求，1次Persisted、7次Replayed、仅一条历史/一次文档代数推进。
- Schema2NonzeroMigration：旧字段JSON独立SHA计算与旧摘要函数对照，修改非零数量而不改旧摘要必须拒绝；读操作只内存升级，显式打开仅规范化一次；奖励元数据和库存保持，不重导入。
- BoundedUnambiguousJson：根/嵌套重复键、65层嵌套、超过64MiB文件失败关闭；超大夹具通过文件seek生成，不分配同等大小测试缓冲。单来源codec边界测试及既有schema1迁移一并回归。

上述子案例、并发次数与旧9来源测试不冒充新增注册数。源码源历史还限制4096来源、65536条目；这不是峰值RAM或容量吞吐实测。

## 6. 改动驱动覆盖与提交校验

原件 `Saved/Automation/P28.33/regression-coverage.log`，SHA `BD7A09673F18860EB914BD44F74C11061BB7AC54436B10457F2E92E86C8552B1`。

原件 `Saved/Automation/P28.33/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。

覆盖结果 `Changed=15 Rules=1 Required=1 Logs=1`，所有12源码路径命中Items必跑组，并由本轮100成功日志证明；旧根不是必跑组的替代。检查器本轮重新自检537/537通过。新Repository来源实现/codec扫描无demo_map、UWorld、AActor、随机GUID或RNG调用。

阶段核验重算21份原件SHA、输入锁、相对文档链接、精确15路径暂存/提交范围，检查103用户文件与2保护文件；`git diff --check`与暂存检查通过。提交后核对父提交、远端分支SHA及剩余工作区；不强推、不夹带用户文件。

## 7. 未关闭项

来源manifest/策略解析授权、合法新定义接纳、Manager路由/容器物化和未拿取/获物/消耗/遗失/终局组合仍属FZ-1；既有库存奖励大整数反射格式仍须在接入获物链前处理。当前拒绝把来源ID提前塞入库存图，不绕过AcquiredItemMismatch，不恢复旧writer。容量/锁内全历史计算与最终完整根验证未验收，FZ-1/2不关闭，自动化不暂停或进入F。
