# Dev.D.UE.0.0.10.P28.22.r0 Development Log

状态：`P28_22_PASS`。普通容器部分生成回滚反例及同World恢复崩溃已取证并最小修复；最终专项、完整新旧根、双构建与实际覆盖全部通过。下文保留当时运行中的检查点，最终结果见第6节；不是整体冻结或F验收。

## 1. 入场与保护

2026-09-20T19:27:54.133Z heartbeat。分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `7523758c24124ae18344dfa6cda7fb6f581a6aaa`。读取P28.21 Report/Log、分支/状态、有限索引和P/F基线；19:28:11.5528374UTC上一阶段Published核验通过：1617产品输入、103原用户文件精确集合/哈希、两份保护文档、10项原件SHA、106个相对链接、2760完整根独立用例、暂存0，无UE-Cmd。未重跑已发布阶段。

保护哈希：OverallReadiness Report `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。103原用户文件路径和哈希沿用P28.21锁，不编辑或暂存。

## 2. 反例构造与首次夹具错误

P28.21的入口旧批次清理已保持拒绝owner，但函数内部安全落点失败和生成/身份失败回滚仍忽略Destroy结果并Reset。只扩展既有ManagerDeactivationRetention，保留前阶段用例后加入瞬态M01Marker和现有测试地板；实际第一容器生成回调记录原指针、设ROLE_SimulatedProxy并关闭地板，使第二容器落点失败。恢复地板不能绕过原对象的释放拒绝；恢复原role后应即时重建健康对象并可重放。

最初瞬态锚点在原点，既有容器偏移使首个落点超出地板，根本未生成第一项，故不构成本轮产品Red。保留该次失败，随后只将锚点设为(200,1600,0)，两目标经原偏移落到地板范围，不改正式资产或生产落点逻辑。

首次夹具Editor于19:31:43.9278399UTC完成，4 actions/26.28秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.RedBuild/BuildEditor/20260920T193117323Z-00a8ff48/stdout.log`，SHA `DBDADC72F45C6E8151908D965A103A59E548703E45C4110C3919D25CCC67713D`。

首次夹具测试于19:32:04.730152UTC完成，0成功/1失败、队列1、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0/RedProof/20260920T193144380Z-a22de32f/UnrealEditor.log`，SHA `3571930D578621D142210D5329E3801699642344A97A1692DC9062BD123BC7FC`。exec74512退出0仅表示预期失败执行链结束，不能把夹具错误当产品缺陷。

## 3. 正确夹具的实际Red与首次修复崩溃

校正夹具Editor于19:32:45.0023833UTC完成，4 actions/9.79秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.RedFixtureCorrected/BuildEditor/20260920T193234930Z-a1142fb8/stdout.log`，SHA `60F8B2739E22735F43BEBC620E447F63BF98F898A01D622B5F591DBB56D470D8`。

实际Red于19:33:05.7947998UTC完成，0成功/1失败、队列1、原生0、崩溃指标0，四条最终Expected失败；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.RedFixtureCorrected/RedProof/20260920T193245444Z-bbe1fabd/UnrealEditor.log`，SHA `EFDBA14AD572B99481FD4BE3BAD1C1DEDC000273F343367B6804B73E0104F656`。日志直接显示.01首建RuntimeAdapter、.02落点失败，随后.01被识别为MapAuthored、.02新建并错误初始化成功。exec82964退出0；此为产品反例，不把原生0视作测试成功。

首次修复将入口释放提为局部ReleaseSpawnedTargets并用于两处失败回滚；有拒绝保留原owner，不清登记或继续初始化。生成/身份失败时先将实际对象加入创建数组。此分支没有独立故障注入，本轮实际取证的是第二项落点失败。

首修Editor于19:34:19.3887503UTC完成，4 actions/18.91秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0/BuildEditor/20260920T193400166Z-001abcca/stdout.log`，SHA `7FCAD0951F9ED403334E711039F75A66EB072663EB59B1259FB6973527097E14`。

首修WorldLifecycle专项于19:34:40.7317491UTC退出3，3成功/0失败、无队列结束、崩溃指标2；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0/WorldLifecycleFocused/20260920T193419872Z-481ee291/UnrealEditor.log`，SHA `95A1C9A63A78447C4EE5A55DEE2E37389B620507E1C2245B96D50AE2E1FC5ED6`。外层exec95798退出1，没有继续ProductFlow/Game/完整根。Fatal原文：Cannot generate unique name for 'M01_CodeBNormalContainer_BasicCache_01' in level 'Level /Engine/Transient.World_3:PersistentLevel'. 堆栈为Manager初始化SpawnActor、测试即时恢复调用，明确是源码恢复路径缺口，不标为环境超时。

## 4. 最终最小修复与锁定执行

读取本机UE5.8源码：World.cpp:607将默认NameMode设为Required_Fatal；LevelActor.cpp:573起若StaticFindObjectFast发现占名即按模式处理，Requested使用MakeUniqueActorName，Required_Fatal于586报告致命错误。原Destroy完成不等于立即清除UObject名称，当前实现不应依赖GC时序才能重试。

只在该容器SpawnParameters设置NameMode=Requested，保留可读名称前缀及原MapTargetIdentity。未修改引擎或其它Actor创建点。测试另断言新内部对象名不同于原名、两个业务ID分别仍为BasicCache.01/.02，健康重放保留原owner、原对象只释放一次、原Run及非零durable快照不变。生产共20行新增/24行删除，测试62行新增，无新注册、友元、API、schema或恢复记录。

19:38:51.5702029UTC Final锁1617产品输入和103原用户文件，保存于Saved/Automation/P28.22/validation-inputs.json。此前19:33:59.6762713UTC首修输入锁及两次Red锁均保留独立副本。最终exec38983使用Recovery1目录，按Editor→WorldLifecycle7→ProductFlow5→Game→旧根1330→新根1430串行执行，各段前后比对锁定输入，没有覆盖首次失败原件。

Recovery1 Editor于19:39:22.413516UTC完成，4 actions/20.57秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.Recovery1/BuildEditor/20260920T193901546Z-6497eb18/stdout.log`，SHA `D2E55FDFB994F40DEC85AE38BBA548FE910EE95A12EAA16F2D06C0C10AE691C3`。

Recovery1 WorldLifecycle专项于19:39:43.2771901UTC完成，7成功/0失败、队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.Recovery1/WorldLifecycleFocused/20260920T193922899Z-61b38400/UnrealEditor.log`，SHA `2A5E7D3A013F4C15E0290BD8FB24BF03754D449FB5D0530B3497557F39660758`。

Recovery1 ProductFlow专项于19:40:03.7192398UTC完成，5成功/0失败、队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.Recovery1/ProductFlowFocused/20260920T193943371Z-01d90775/UnrealEditor.log`，SHA `73C659B5430AED340BD69FA4148146CFED4FEB1B2E71CC78BCD3C369E745E2E5`。

映射自检exec84311原生退出0，529/529通过；原件 `Saved/Automation/P28.22/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。自检实际运行，不以确定性相同输出推定复用，也不冒充本轮改动覆盖。

## 5. 完整回归过程检查点

Recovery1 Game于19:40:40.3688376UTC完成，4 actions/35.98秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.Recovery1/BuildGame/20260920T194004140Z-e39a56f3/stdout.log`，SHA `ED0143ECD234063487DBB065D31CA470BE2CF8229097D98B5CEE0DE9CA92748A`。

Recovery1 Legacy完整根于19:41:56.1053146UTC完成，1330成功/0失败、队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.Recovery1/LegacyFullRoot/20260920T194040717Z-3eaa0d55/UnrealEditor.log`，SHA `F646FC2BD8887D39DEAC08FEDC08F099260D0C322972615BAEA587CB772A6E72`。

19:42:50.6009068UTC检查，新根原目录Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.Recovery1/ShanmenFullRoot/20260920T194156414Z-c26d0f95，UE-Cmd45900仍运行，410成功/0失败、崩溃指标0，无队列结束或最终run-state；运行中原件不计算最终SHA。exec38983继续执行，下一次heartbeat承接同一进程，不重复启动。

等待同一锁定执行链的新根和之后实际覆盖核验；不重启在跑的UE-Cmd或更改产品输入，不提前计入1430，不将首修崩溃前3个成功与最终日志拼接。

19:44:29.2434562UTC续接检查通过：HEAD未变、1617锁定产品输入、103原用户精确集合/哈希、两份保护文档、12项已完成原件SHA及4个本轮文档链接；暂存0，105未跟踪仅新增本Report/Log，有限索引仍为P28.21，diff check通过。运行中的新根不在这12项已完成原件中。

2026-09-20T20:17:24.896Z heartbeat继续原exec38983。20:17:49.6122969UTC新根UE-Cmd45900仍运行，1045成功/0失败、崩溃指标0，日志持续更新但尚无队列结束或最终run-state。20:18:27.1857512UTC再次核验1617输入、103原用户精确集合/哈希、两份保护文档、12项已完成原件SHA和4个本轮文档链接通过；HEAD/有限索引未变，暂存0、未跟踪105、diff check通过。保留原验证进程和锁定源码，不重跑、不提前提交，也不把常规等待视为失败或阶段完成。

## 6. 最终结果与精确交接

2026-09-20T20:51:25.391Z heartbeat读取分支、当前Report/Log、续接记录、Git状态和P/F基线，承接原exec38983。20:51:48.1007172UTC仍为1212成功/0失败，未重新启动。新根随后于20:52:14.1109461UTC正常完成，1430成功/0失败、精确队列1430、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.22.r0.Recovery1/ShanmenFullRoot/20260920T194156414Z-c26d0f95/UnrealEditor.log`，SHA `027F67E707EB417ECCED62CD5BC9315E8519456DBEEE1E573AE4EFFDF751F26A`。原exec38983退出0，尾部输入核验通过。新根总耗时约70分18秒，未以运行中计数代替最终结果；与旧根共2760个独立成功用例，专项7/5不重复累计。

20:52:20.5330481UTC复核1617产品输入、103原用户精确集合/哈希、两份保护文档、此前12项已完成原件SHA和4个本轮文档链接通过；HEAD/有限索引尚未更改，暂存0、未跟踪105、diff check通过。其后完成本轮实际覆盖，5路径/2规则/7必跑组/2日志通过；原件 `Saved/Automation/P28.22/regression-coverage.log`，SHA `2ECA0BE4A20AD83ACBD64D69346A3A7B531B7F02B08D66AE6E36F93C87E69FF3`。必跑组是demo_map.CodeB、ItemUseAndArmor、P4.Hotbar、Profile、V2RangedCompatibility、Shanmen.0_0_10及其Items组，全部使用Recovery1两份健康完整根日志，不借用P28.21产品日志。

最终精确范围为Manager cpp（20新增/24删除）、既有PreparationAdapter测试（62新增）、有限冻结索引、本Report/Log五路径。保留首个夹具错误、实际Red、首修原生3崩溃及所有对应构建原件；14项原件SHA一起检查。只暂存本阶段五路径，提交并推送当前分支，不强推。提交标识由Git历史记录，不在提交自身内伪造自引用哈希。

20:55:08.3205849UTC发布前独立核验通过：1617锁定产品输入、103原用户精确集合/哈希、两份保护文档、14项原件SHA、110个相对文档链接、6项最终健康执行、2760完整根独立用例、实际映射/自检和diff check。此时暂存0、未跟踪105；远端当前分支仍为原基线7523758c24124ae18344dfa6cda7fb6f581a6aaa，精确提交没有覆盖并行远端更新。

有限索引记录本轮局部闭合，但FZ-1/2仍开放。103原用户文件及两份OverallReadiness修改保持不变；其他容器/物品/空间包释放、强制EndPlay和完整入口权威路由不能因本轮通过而视作已闭合。未启动Editor UI/PIE/Standalone/产品exe，未改正式地图、内容、玩法或UI，不做Smoke/Cook/Package，也不暂停自动化或进入F。Saved原始日志仅保留本地，仓库Log提供可核验路径与哈希。

- [Report](../Report/Dev.D.UE.0.0.10.P28.22.r0_report.md)
