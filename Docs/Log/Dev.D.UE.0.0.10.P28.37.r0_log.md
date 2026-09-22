# Dev.D.UE.0.0.10.P28.37.r0 Development Log

## 1. 基线与范围

- UTC 2026-09-22；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`，父提交 `f865ff863e6a5ec34b6b572bf1631ba0029df00e`。
- 起始读取P28.36 Report/Log、P阶段基线、冻结索引、Git状态；前阶段Published检查通过，无残留UE验证。空暂存；103个既有用户未跟踪文件、两份OverallReadiness修改，没有其他源码改动。
- 继续P28.30既有来源闭合链中的目录子项；不接Manager，不启动游戏流程。成果见 [Report](../Report/Dev.D.UE.0.0.10.P28.37.r0_report.md)。

## 2. 实施

先在既有AuthorityServiceTests与GeneratedSourceAdapterTests加入3项注册用例，并把原八线程幂等用例扩展为新目录条目。实际稀疏目录+M01/P8原规划器候选在旧接收端被拒绝。随后仅修改RepositoryGeneratedSource.cpp的原Candidate提交：计划内缺失定义加入、已有定义完全相等复用；不增加另一事务或存储。

源码214新增/3删除：生产13新增；测试201新增/3删除。没有头文件/持久schema/codec/请求指纹/回归映射/配置/内容资产修改。Plan.IsValid及全状态验证保留，加载时缺目录或漂移仍拒绝；保存失败不发布目录或来源，重开只认原同文档完整结果。新增定义不产生库存实例、不授权产品来源，不放宽现有获物/消费/终局约束。

## 3. Red、构建与执行过程

Red与Initial分别冻结1628输入（Source1097、Content485、Scripts40、Config5、uproject1）及原103用户文件。两输入锁之间只允许RepositoryGeneratedSource.cpp不同，测试源码逐字节相同；后续Recovery1沿用Initial，不修改源码或重构建。

构建经 `Scripts/Invoke-Shanmen.ps1`，Development Win64、MaxParallelActions=4。Red Editor UTC14:51:21.424—14:51:47.641，7actions/原生0；最终Initial Editor14:54:14.874—14:54:23.300，4actions/原生0，Game14:54:23.325—14:54:50.419，5actions/原生0。均实际Succeeded编译，不是up-to-date，未运行游戏程序。

Red专项UTC14:51:48.072—14:52:09.183，0 Success/3 Fail、正常队列3、10条最终Expected失败；原生进程退出0。外层临时脚本错误地要求Red原生必须非0，因此退出1；检查实际日志后更正该脚本的假设，保留原生run-state与测试失败原件，不重写为非0、不重跑覆盖Red。结果门仍按注册结果+唯一队列+进程状态判断，不能将SUCCEEDED进程状态当测试通过。

Initial专项3/0完整通过。Initial Items在UTC14:55:12.206—14:55:47.895原生0，但只记录110 Success、0 Fail；尾部停在TriggeredChargeCommit开始，缺该项结果及结束队列。外层证据门退出1，后续两武器/旧根尚未启动。此日志不计完整通过，保留原件，缺尾段具体原因未确定。

本机UE5.8 `Engine/Source/Runtime/Core/Private/Misc/OutputDeviceFile.cpp:569-580`确认 `-FORCELOGFLUSH` 每行调用AsyncWriter->Flush。Recovery1仅对测试启动参数加入此选项，仍用原构建与Initial输入锁；这是一项日志刷新措施，不声称已经定位或修复引擎缺尾段原因。

所有调用使用UnrealEditor-Cmd、`-Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile`、单个Automation RunTests组和 `-TestExit=Automation Test Queue Empty`；Recovery1额外带上述逐行刷新标志。

| 验收证据 / 组 | 唯一Success / Fail / 结束队列 | 原生 |
|---|---|---|
| Initial / Shanmen.0_0_10.Items.GeneratedSource.Catalog | 3 / 0 / 3 | 0 |
| Recovery1 / Shanmen.0_0_10.Items | 111 / 0 / 111 | 0 |
| Recovery1 / Shanmen.0_0_10.Product.ThrownWeaponContent | 1 / 0 / 1 | 0 |
| Recovery1 / Shanmen.0_0_10.Product.ControlledWeaponContent | 1 / 0 / 1 | 0 |
| Recovery1 / demo_map | 1330 / 0 / 1330 | 0 |

Recovery1从UTC14:57:32.482开始，旧根15:00:36.898—15:05:17.600执行，15:05:18.083外层回传完成/退出0。独立成功1443，专项3项为Items子集；Red及Initial不完整Items不计通过、不拼接。全部UE进程结束。七份原始测试日志各有13条既存启动Condition failed Error；除Red的注册Fail/Expected外，无其他Error、Fatal、Ensure、Unhandled Exception、Assertion failed或HTTP request timed out，不清洗原日志成零Error。

## 4. 原始证据 SHA-256

以下原件保存在本地Saved，GitHub交付报告及路径/哈希索引，不声称上传原始日志。最终检查逐项验证。

- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Red/BuildEditor/20260922T145121390Z-c59a1f3e/stdout.log`，SHA `5D0A44C31FF580DAE2C8E0C4C52B5C19D0DD8C50AF998BE9E84E913238C77197`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Red/BuildEditor/20260922T145121390Z-c59a1f3e/run-state.json`，SHA `41B43B60E010F2E549EC1B2F8F2077221A3994A8FD367DB631BA9DAAA4E9A863`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Red/CatalogFocused/20260922T145148060Z-38ed7785/UnrealEditor.log`，SHA `F61D24CB3898DC080BA76F5A4D840861832C3C79A52026E848291074F6D50B9F`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Red/CatalogFocused/20260922T145148060Z-38ed7785/run-state.json`，SHA `A22F9CC668852B41BA16044BE400E6B6061B3C29E82D16822ABB232BB8C95CF3`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Initial/BuildEditor/20260922T145414841Z-e3ebe484/stdout.log`，SHA `F603833D339E0669AF6BE2A9E02A1E01772F89D86F9D5291EA69CA31EDF8D9CE`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Initial/BuildEditor/20260922T145414841Z-e3ebe484/run-state.json`，SHA `27CBD7638F6ECFF7D7F4C5CCFF1EE115648F734A1E0314E6A68286A8E198E30B`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Initial/BuildGame/20260922T145423322Z-8d1a315b/stdout.log`，SHA `FA4B3F832793C20116BE509B29F8684328E9AC2CA66EBCE1328B346042FBF523`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Initial/BuildGame/20260922T145423322Z-8d1a315b/run-state.json`，SHA `5C1099D78B27A6B9171544974F00B81447E601CE40FB89CF12ADF520C10AEE10`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Initial/CatalogFocused/20260922T145450780Z-70434125/UnrealEditor.log`，SHA `06640E40F98D97CAE62D8E20D2A8855FD533942E39070C6E44913D496AD4B3A0`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Initial/CatalogFocused/20260922T145450780Z-70434125/run-state.json`，SHA `AC3B447985EEDF6CDED5E10F220C948CCE0255C2BDA96AF08B9E79F0D57BF535`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Initial/ItemsRegression/20260922T145512203Z-ac4c5cc5/UnrealEditor.log`，SHA `C712CA7419B3C11151740B2DE0ACEBED6F617D228614A724F2749085EFEB1C71`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Initial/ItemsRegression/20260922T145512203Z-ac4c5cc5/run-state.json`，SHA `3EB0C91AF6B9E32569CC1B957B5D67D3FAF269B832E2BA45B5DDEB9DEC5DDBA5`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Recovery1/ControlledDefinition/20260922T145951172Z-06176cd0/UnrealEditor.log`，SHA `A39A5105C2CCD917EAF46C9DB45777EA58ACBD966206BC19CB9E857880853ACB`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Recovery1/ControlledDefinition/20260922T145951172Z-06176cd0/run-state.json`，SHA `373078C6E4A67572869335DBB4E6598EC41451DAD530B29B2200D6E2E3250B86`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Recovery1/ItemsRegression/20260922T145732439Z-8ea35082/UnrealEditor.log`，SHA `F63A58C10DC1C6FFFD5EB4DDA00405E4E52B68446D3846FE3B3EFB1002FCFC75`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Recovery1/ItemsRegression/20260922T145732439Z-8ea35082/run-state.json`，SHA `57B6FE6A442A4AAAA0487429E735FD26933672AD9EBC922F81B3E4EBE38055AE`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Recovery1/LegacyFullRoot/20260922T150036895Z-11b40dc9/UnrealEditor.log`，SHA `D576854DA3B53AD00688AC5B4B66DF5E413005438BDC8C30B4AD49B8FBA97908`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Recovery1/LegacyFullRoot/20260922T150036895Z-11b40dc9/run-state.json`，SHA `7C9581F8C56DDA3631C819B47E0CAEBFCCBCAA4FECDCDE5DAAC4F40D08E6E5EC`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Recovery1/ThrownDefinition/20260922T145859101Z-573e1676/UnrealEditor.log`，SHA `06D6F93BF436D3A91E0006086A6C6402DA5305DDEA2B6834F67029E68F6D160E`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.37.r0.Recovery1/ThrownDefinition/20260922T145859101Z-573e1676/run-state.json`，SHA `6D17A04F1AB8ED8C902AC8D320860235E168EE71C8DF776558846EE273CE82E1`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.37/validation-inputs-Red.json`，SHA `241EE69731EF0B1B3C1B731F5C870F7E0AE649AA7BC291CD56BF29B337D54588`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.37/validation-inputs-Initial.json`，SHA `3F5C8BB887B25C93CEFCFAE5EC1CD6B58DFEABADE5A23C2E89157EE8C421777D`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.37/coverage-Initial.log`，SHA `F3987ABBD95BE565CB009301CF84CB52C294692DBFAD0C4AAB705A4DCF2DD0C8`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.37/regression-selftest-Initial.log`，SHA `27C66F1B346956EB9E16A6CC798E763AEA6EEE484FA7986FDE75814BFB50B6C7`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.37/boundary-Initial.log`，SHA `C25760FAE0FADBF588356D039C36A6E7D684B6EA30C0AD2A3AEFC672018B09B6`。

## 5. 回归范围与检查

本轮6路径：3 Source + Report/Log/架构索引。依据既有Items、GeneratedSourceProductBoundary和ItemProductAdapters三条映射，必跑7组：完整Items、两武器内容、旧RewardSourceProjection、RewardFullMapDistribution、ItemUseAndArmor、P4.Hotbar；旧根完整覆盖四个旧子组。

以四份Recovery1日志检查，结果 `REGRESSION_COVERAGE: PASS Changed=6 Rules=3 Required=7 Logs=4`；独立映射自检549/549已重新通过。自检首次误向Coverage入口传入不存在的SelfTest参数，属于执行调用错误、没有运行产品；随后读取脚本入口，调用独立CoverageSelfTest脚本通过，未修改共享脚本或规则。

模块边界扫描实际零命中demo_map include、UWorld/AActor、ApplyDamage、直接RNG；核心Build.cs无Engine/demo_map依赖。产品测试调用既有规划器，不声称产品整体无RNG。首来源夹具记录entries4、definitions2→4、serializedBytes9619，不等于内存峰值。最终核对两锁差异、1628输入集合/内容、用户文件、全部证据SHA、文档链接、diff --check及精确阶段暂存范围。

## 6. 文件保护与发布

不变的103个用户未跟踪文件及两份原有用户修改均不暂存：OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`；Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。

仅显式暂存本阶段6路径，普通非强制推送，发布后核对远端与本地HEAD。提交身份由Git提供，不在本文自引用SHA。临时执行/检查脚本、首次Red和不完整证据保留本地Saved。

## 7. 下一边界

目录一致性接纳已补齐，首次来源内容/产品入口授权尚未完成；下一步先以当前canonical登记内容约束Subsystem端口，再考虑Manager路由及原计划物化恢复，不恢复旧writer。获物/消费/遗失与终局组合、FZ-2剩余调用点仍待闭合；不能用本轮内存Repository产品夹具代替全产品持久链。

未执行当前完整Shanmen根；P28.34.r1完整双根2778仅历史。FZ-3最终完整双根/双构建待有限清单关闭后执行，见 [冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)。保持 [P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：无物理输入/玩法/UI/正式地图内容修改，无Editor UI、PIE、Standalone、游戏程序、截图、Smoke、Cook、Package；不暂停为已完成。
