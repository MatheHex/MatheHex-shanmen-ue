# Dev.D.UE.0.0.10.P28.10.r0 Development Log

最终状态：`P28_10_COMPLETE`（测试证据闭合，非生产修复/整体冻结）。第1–5节保留原始执行与等待检查点，其中IN_PROGRESS、未提交及运行中计数均为当时事实；最终结果与本阶段交接范围以第6节为准。

## 1. 入场与有限范围

2026-09-18 22:27 heartbeat；分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `f1ce4c405c1783ba21c6030ed4312be516aa328c`。完整读取P28.9 Report/Log与共享P/F基线，入场无运行中的UE/UBT，暂存为空，仅两份OverallReadiness用户修改。

22:29:39.588UTC核验上一阶段锁定1617输入和103原未跟踪用户文件，哈希/路径一致，保护Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`、Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26` 未变，保存本地baseline.json。本轮不再执行已完成P28.9。

## 2. 源码审计与测试增量

沿Manager.RequestSettlementAndReload→Flow.CommitRuntimeSettlement/FinalizeShanmenSettlement及RetryPendingProfileSettlement/RetryPendingSettlement读取真实调用。持久失败可进入SettlementPending，保留Summary与StartedRunId；再次持久成功会清StartedRunId，因此Manager必须在调用Flow前捕获原RunId并交给P28.9完成端口。静态上已具备该路径，本轮先补实际失败—重试—World清理组合证明，不假称发现生产缺陷。

仅修改demo_mapShanmenPreparationAdapterTests.cpp：将同一个ManagerSettlementContinuation注册测试扩成两变体。原直接提交变体继续保留；新变体以既有EShanmenItemStoreFailureStage::WriteTemp使首次提交和一次重试失败，检查活动持久快照不变、Runtime已Settled而证据仍绑定原Run、Manager仍pending且无已提交前缀、原World对象未释放。撤销存储故障后，再一次持久重试成功；World仍拒绝，进入原Run的已提交前缀，之后沿既有绑定错配/启动门/解除World故障/恰好一次清理断言继续。最终持久submit=1、retry=2；后续World重试不得继续增加它们。两个变体分别预期2/3次真实World拒绝日志。

存盘失败使用既有隔离自动化故障入口，World拒绝仍由引擎Role触发实际Destroy；未添加产品故障注入API或篡改成功返回值。测试没有Controller、正式地图BeginPlay或物理输入，仍不是F验收。终局前置Runtime容器清理和强制EndPlay只初步读到相关代码，不在本阶段作完成结论。

## 3. 锁定验证

22:30:41.570UTC固定1617输入+103用户哈希，相对入场恰好一个测试源文件变化；git diff --check通过。Saved/Automation/P28.10/run.ps1将依次执行Editor、WorldLifecycle7/ProductFlow5、Game、完整旧根1330/新根1430，各段前后校验输入锁。保持单实例，不在运行时改输入或重复构建；未完成前保留IN_PROGRESS，后续还需文件驱动映射、自检、文档与受保护文件核验及精确发布。

## 4. 已完成的专项与双目标

原exec71660 / pwsh21252运行串行验证，独立exec5285执行既有映射自检；没有运行Editor UI或产品exe。新增延迟持久成功变体在已有P28.9产品逻辑上首次运行即通过，因此本阶段只有测试增量，不包装成生产修复，也没有捏造Red失败。

- Editor于22:32:26.374UTC完成，4 actions / 10.56秒，SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.10.r0/BuildEditor/20260918T223215484Z-92ea7c4a/stdout.log`，SHA `853EF75A8599B8CA03BC86FB72F4E4A236FF43570F6C8DD17BCACE6011EFFD4A`。
- WorldLifecycle于22:32:47.566UTC完成，7/0、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.10.r0/WorldLifecycleFocused/20260918T223227168Z-b50d416c/UnrealEditor.log`，SHA `953A7E0C357377F5C32A2C235E0F2AB6A97B5958C53D3D88742B83D0B8B1EC64`。新增组合变体记录真实WriteTemp拒绝及3次World销毁拒绝，直接变体记录2次；同一ManagerSettlementContinuation注册测试成功，不把两变体计为两个注册测试。
- ProductFlow于22:33:08.038UTC完成，5/0、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.10.r0/ProductFlowFocused/20260918T223247636Z-f3493734/UnrealEditor.log`，SHA `3ECF75EE26B78485F2C6AD06CE5E634710FD55485A18E0F9B6126A9C1B92D7F0`。
- Game于22:33:23.960UTC完成，3 actions / 15.15秒，SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.10.r0/BuildGame/20260918T223308579Z-6043b9fa/stdout.log`，SHA `FF0574593CF72163300B4C438DB59FBAEF81FF1F2E5759EC1DFE9BC1142D0C83`。
- 映射自检exec5285退出0、529/529；原件 `Saved/Automation/P28.10/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。独立执行，输出与此前相同，不是复用旧进程。

22:33:56UTC再次核验1617输入+103用户哈希和两份保护文档一致，未跟踪105=103用户+本阶段Report/Log，暂存为空。22:34:18UTC旧根20260918T223324438Z-a853b892仍运行，快照897 Success / 0 Fail、崩溃指标0，无最终队列；随后原运行器将执行新根。原日志含未使用平台SDK提示，不称全平台构建通过。运行中日志不写最终SHA。

静态推导本测试路径命中ItemProductAdapters，必跑Shanmen.0_0_10.Items、demo_map.ItemUseAndArmor和demo_map.P4.Hotbar；新增WorldLifecycle变体另有本阶段专项及计划完整新根覆盖。结束后需用完整根实际日志执行精确四路径覆盖门（测试源码+索引+Report/Log），更新有限索引并发布。现在不提交、不推送、不开展下一缺口。

## 5. 本轮交还检查点

原exec71660确认旧根1330/0、精确队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.10.r0/LegacyFullRoot/20260918T223324438Z-a853b892/UnrealEditor.log`，SHA `AF4A976AAA64450BFBC60FC04FFF88A1572F5D9BDB7682FA5B467457B0D8996B`。随后自动启动ShanmenFullRoot/20260918T223436354Z-398bbc22，22:36:02UTC快照480 Success / 0 Fail、崩溃指标0，无最终队列；同一运行器继续，未另起实例。已完成证据哈希与3个文档链接校验通过，源码差异检查通过；本阶段尚未提交/推送，索引仍指向P28.9。

23:08 heartbeat续接：分支/HEAD未变，原exec71660仍运行；实际UE-Cmd PID6708、父进程21252，仍是同一新根/Unattended/NullRHI。23:09:15UTC读取1042 Success / 0 Fail、崩溃指标0，无最终队列或run-state；23:09:45UTC又记录LocalClearAndLifecycleFences成功。存在HTTP超时及大Tick间隔警告，但有持续进展，不重启或判为失败。23:09:50UTC逐项核验1617输入、103用户文件、两份保护哈希和6项已结束原始证据SHA全部匹配，未跟踪精确105、暂存为空，源码diff检查通过。只更新进度文档和续接记录；未更改锁定输入、重复验证、提交或推送。

## 6. 最终完整结果与发布范围

23:42 heartbeat续接原验证实例，未重启UE或重复构建。23:43:13.901UTC同一个新根完成1430 Success / 0 Fail，精确队列1430，原生退出0，崩溃指标0；原exec71660通过末次输入锁检查并退出0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.10.r0/ShanmenFullRoot/20260918T223436354Z-398bbc22/UnrealEditor.log`，SHA `27A3A544A2F6A0B738F838FF4733817F82B6376E7143644AF58290C144EAD731`。新旧完整根1430+1330=2760；专项7/5是其子集，不能相加为额外完整测试。

以本阶段四个精确路径和上述新旧根实际日志执行既有覆盖检查器，原生0，PASS Changed=4 Rules=1 Required=3 Logs=2。唯一源码路径命中ItemProductAdapters，必跑demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、Shanmen.0_0_10.Items均有健康日志；新增WorldLifecycle变体另由本阶段7项专项和完整新根取证。原件 `Saved/Automation/P28.10/regression-coverage.log`，SHA `1A1503519F779AA18C1AEE1A26CD27493F5C47EAD976C9219BBF6562A878321A`。

Editor/Game、专项、两完整根及自检均为本阶段实际运行；所有8项结束后的原件路径/哈希在本Log保留。Saved原始证据留在本地，不随GitHub报告上传。HTTP generate_204超时、大Tick间隔及未使用平台SDK警告不删除；通过的是指定测试和Win64双目标，不是性能、无警告或全平台验收。

最终源码差异仅PreparationAdapterTests.cpp（146新增/98删除，主要为保留直接变体并移入共享lambda）；产品实现没有变更。新增延迟变体首次即通过P28.9实现，无本阶段Red失败可报告。保留非零持久快照、原Run和时间线、两个持久失败、错误绑定拒绝、原World恰好一次释放；不改断言基准为恒真，不把两个变体计成两个注册测试。

精确发布范围仅测试源码、FoundationClosure_Index、P28.10 Report、P28.10 Log四文件。1617锁定输入、103用户文件、两份OverallReadiness保护哈希、原始证据SHA、文档链接及git diff --check作为提交前复核门；不暂存用户文件，不强推。基线为f1ce4c4，本阶段身份由包含本Log的提交给出，远端一致性在推送后单独核验，不在同一提交内虚构其未来SHA。

23:49:37.335UTC提交前实际复核：1617锁定输入、103原用户路径及哈希、两份保护文档均匹配；未跟踪精确105=103用户+Report/Log，暂存为空。8项原始日志SHA逐一匹配，三个阶段文档中的68个相对链接均存在，git diff --check退出0。Git换行规范化提示不当作构建错误，原用户文件不纳入提交。

有限索引更新“持久失败后成功重试的World确认变体”为已实测；仍留终局前置Runtime容器/World物品、强制EndPlay和FZ-1入口审计，保持FREEZE_AUDIT_IN_PROGRESS。没有实际玩法、物理输入、正式地图/资产、UI/PIE/Standalone/Smoke/Cook/Package；不宣布整体底层完成，不暂停本自动化。

- [Report](../Report/Dev.D.UE.0.0.10.P28.10.r0_report.md)
- [有限闭合索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
