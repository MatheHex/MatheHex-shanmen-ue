# Dev.D.UE.0.0.10.P28.23.r0 Development Log

状态：`P28_23_PASS`。实际拾取释放拒绝反例已最小修复；双构建、专项、完整新旧根及实际改动覆盖全部通过。下文保留当时运行中的检查点，最终结果见第5节；不是整体架构冻结或F验收。

## 1. 入场与保护

2026-09-20T21:29:26.048Z heartbeat。分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `fb14a536f9f0581eede1cb47d8ab7b4115ee4872`。读取P28.22 Report/Log、分支/状态、有限索引和P/F基线。21:29:41.4986668UTC上一阶段Published核验通过：1617产品输入、103原用户精确集合/哈希、两份保护文档、14项原件SHA、110个相对链接、2760独立完整根用例、暂存0，无UE-Cmd。不重复开发或重跑P28.22。

保护哈希：OverallReadiness Report `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。103原用户文件路径/哈希沿用P28.22锁，不编辑或暂存。

## 2. 静态路由与实际Red

沿WorldItem.RequestInteract→ItemSubsystem.PickupWorldItem检查普通非空间包路径；Authority.PickupWorld/TagAffectedForActiveRun可能改变堆叠、所有权、来源及修订号，删除World绑定后忽略Destroy结果。EndPlay只对仍匹配的绑定退休World身份，故成功销毁仍必须在去绑定之后。检查本机UE5.8 LevelActor.cpp:839起DestroyActor：GameWorld内非Authority且非强制/NetTemporary的network actor返回false；用既有SetRole构造该拒绝，不改引擎。

只扩展既有PreparedWorldPickupIdentity，无新增注册/友元。21:35:34.4200633UTC Red锁1617输入/103原用户，其中只有测试文件变化。exec23344按Editor→单项Red运行，外层退出0仅说明预期失败执行链正常结束，不等于测试成功。

Red Editor于21:35:54.9475429UTC完成，原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Red/BuildEditor/20260920T213534953Z-372d1f9c/stdout.log`，SHA `CFFB3A752B236CE491F35A058A84F67827E685E498C9D4BE9BEC556C0C390738`。

Red于21:36:15.7783234UTC完成，0成功/1失败、队列1、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Red/RedProof/20260920T213555385Z-b561bd23/UnrealEditor.log`，SHA `C32902AB6C540ECF2BE1AF7BBC59C482D011BED7C490CB63A8FF97324FE6ADCE`。七条最终Expected失败：两次拒绝各三项（正确失败结果、原World身份/绑定/数量/来源、背包/修订/持久Run保持），恢复role后原真实拾取隔离断言再失败。该实现首次错误成功后原物品已入背包，第二次变成AlreadyClaimed。后续合并变体未在Red中执行，不冒称它也单独复现。

## 3. 首次修复与输入锁

PickupWorldItem检查Destroy返回值，拒绝则恢复既有Authority快照、原Actor绑定并返回InvalidWorldBinding；成功仍先去绑定再Destroy。既有Invariant错误分支改用Result.RelatedDefinitionId，避免RestoreState之后访问旧Instance指针。

内存Fdemo_mapItemAuthorityState补AuthorityRevision，CaptureState/RestoreState同进同出。当前Revision原来只在独立值字段中，既有快照回滚无法恢复它；此次不是改持久序列化或添加权威。其他手动回滚/已有RevisionBefore写法保持原样，不顺手重构。生产三文件12新增/3删除，测试61新增，共四个源码路径。

21:36:41.6141988UTC Final锁保存Saved/Automation/P28.23/validation-inputs.json，1617产品输入/103原用户。exec49915执行Editor→拾取1→WorldLifecycle7→ProductFlow5→Game→完整旧根1330→完整新根1430，各段前后核验输入；同一链完成前不改源码或重复启动。Red与Final目录分离，首次失败原件保留。

映射自检exec19860退出0、529/529通过；原件 `Saved/Automation/P28.23/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。这不是产品验证，也不是本轮实际日志覆盖。

## 4. 完整回归过程检查点

Final Editor于21:41:03.2600673UTC完成，84 actions / 260.77秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Final/BuildEditor/20260920T213642147Z-5d4b6945/stdout.log`，SHA `BD1CEEF3227B7C8D4D1C1EA681B2A5543BD96903A73F480874AC38C9473F8E02`。

Final拾取专项于21:41:24.0284335UTC完成，1成功/0失败、队列1、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Final/PickupFocused/20260920T214103683Z-1360d6af/UnrealEditor.log`，SHA `B3F240C7C1284C0E8B382CDDCB7C0C33A4113FFC2A6332D85DC4676C6236C1DF`。原反例与后来合并/重放/持久结算断言均完成；合并变体没有独立Red，明确区分。

Final WorldLifecycle专项于21:41:44.4805915UTC完成，7成功/0失败、队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Final/WorldLifecycleFocused/20260920T214124124Z-5b604a13/UnrealEditor.log`，SHA `CFF55F329C220536523EB33F3E7DCF89DAC2EA9EAA4D1B59D4DDA54B45CB1A6F`。

Final ProductFlow专项于21:42:04.8516161UTC完成，5成功/0失败、队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Final/ProductFlowFocused/20260920T214144512Z-301dc8e8/UnrealEditor.log`，SHA `587A9DD79A7A186E97D69EBAEFAF455554A6E9EC9FE8C043DA094BC70B4A2BB2`。

此前检查点中Game编译仍在执行，83个构建动作；完整旧根/新根将在同一exec49915中依序运行，当时尚未计为通过，后续结果见下文。13项专项属完整根子集，不重复计数。实际映射需要本阶段健康日志；四源码命中ItemProductAdapters一规则、三必跑组（ItemUseAndArmor、P4.Hotbar、Shanmen.0_0_10.Items），计划精确七路径。

21:40:18.2829863UTC检查点核验1617产品输入、103原用户精确集合/哈希、两份保护文档、当时3项已完成原件SHA和4个本轮文档链接通过；HEAD未变、暂存0、未跟踪105仅新增本Report/Log，有限索引未改，diff check通过。运行中日志不算最终SHA。该检查不替代产品测试通过。

21:43:38.5974541UTC再次核验1617输入、103原用户精确集合/哈希、两份保护文档、7项已完成原件SHA和4个相对链接通过；暂存0、未跟踪105、HEAD及有限索引不变，diff check通过。七份本轮本地执行/检查脚本的PowerShell语法解析通过。此时Game进行至23/83动作，exec49915保持运行；下一次heartbeat承接同一执行链，不重新构建或启动重复完整根。

2026-09-20T22:17:26.768Z heartbeat读取分支、当前Report/Log、续接记录与P/F基线，承接原exec49915，不启动重复执行。Final Game已于21:46:27.9937793UTC完成，83 actions / 262.45秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Final/BuildGame/20260920T214205287Z-03bab6e3/stdout.log`，SHA `8A8D00CA215D62D2612355FCFB03086579753A935EB93961583DDEB33AA86E93`。

Final Legacy完整根于21:47:40.33851UTC完成，1330成功/0失败、队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Final/LegacyFullRoot/20260920T214628343Z-0987012d/UnrealEditor.log`，SHA `A26866511398471B1CC23846AF0C8CD95E53A69E5288FE3FA30AB1C1ED7028C6`。

22:18:06.8285340UTC检查完整新根：原目录Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Final/ShanmenFullRoot/20260920T214740646Z-78d71cf6，UE-Cmd28404仍运行，912成功/0失败、崩溃指标0，尚无队列结束或最终run-state。没有计算运行中原件的最终SHA，没有提前记为1430或阶段通过；原exec49915持续执行。

22:18:06.7425869UTC复核1617锁定输入、103原用户精确集合/哈希、两份保护文档、此前7项原件SHA和4个相对链接通过；HEAD及有限索引不变，暂存0、未跟踪105、diff check通过。保持原源码和进程，等待本阶段新根完整结果再运行实际覆盖并交接；常规健康等待不属于失败或完成。

22:19:07.9675807UTC更新本轮已完成证据后再次核验通过：1617输入、103原用户精确集合/哈希、两份保护文档、9项原件SHA、4个相对链接、暂存0、未跟踪105及diff check；有限索引仍未改，不把中间检查点视为最终冻结。

2026-09-20T22:51:57.299Z heartbeat读取分支、当前Report/Log、续接记录、Git状态与P/F基线。原exec49915及UE-Cmd28404仍在执行同一新根，没有重新启动。22:52:36.6091784UTC为1183成功/0失败、崩溃指标0、无队列结束或最终run-state；22:54:10.0568976UTC已推进到1186成功/0失败，日志继续更新，仍无最终队列。原进程继续等待，运行中日志不声明最终SHA，不把阶段标记通过。

22:52:36.5309496UTC保护核验通过：1617锁定输入、103原用户精确集合/哈希、两份保护文档、9项原件SHA、4个相对链接、暂存0、未跟踪105及diff check；HEAD和有限索引不变。本次仅续接验证与更新过程记录，没有新源码修改、重复构建、暂存或推送。

此前检查点均未提交/推送，有限索引当时保持P28.22已发布事实；最终完成后再更新索引并精确交接，见第5节。空间包多对象恢复、其他容器释放及全入口权威路由仍属后续待核验，不能借本轮单对象事务宣称全部闭合。

## 5. 最终结果与精确交接

2026-09-20T23:31:27.636Z heartbeat读取分支、当前Report/Log、续接记录、Git状态和P/F基线，承接原exec49915，确认它已正常退出0，尾部锁定输入检查通过，没有重跑。Final完整新根于22:58:58.7763874UTC完成，1430成功/0失败、精确队列1430、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.23.r0.Final/ShanmenFullRoot/20260920T214740646Z-78d71cf6/UnrealEditor.log`，SHA `8DECD32C8784ABA8DB770B6396A4446577FE0BA3556AE9B0F3445C994745A945`。新根从21:47:40.6488849UTC开始，约71分18秒，原进程完整结束；两根共2760个独立成功用例，13项专项不重复累计。

23:31:52.1366457UTC复核1617锁定输入、103原用户精确集合/哈希、两份保护文档、此前9项原件SHA、4个相对链接、暂存0、未跟踪105及diff check通过；HEAD和有限索引尚未改动，无UE-Cmd残留。随后执行实际覆盖：7路径、1规则、3必跑组、2份本阶段健康完整日志通过；原件 `Saved/Automation/P28.23/regression-coverage.log`，SHA `B2ED404C3D84B30D55961F56364743EF1E87EA94D4E7176783EFA8867E79013B`。必跑组为demo_map.ItemUseAndArmor、demo_map.P4.Hotbar与Shanmen.0_0_10.Items，不借用P28.22日志。

最终源码未再变更，生产三文件12新增/3删除，既有测试61新增。有限索引记录普通拾取失败回滚、原绑定保持及内存快照修订号恢复。精确交接七路径：ItemAuthority cpp/h、ItemSubsystem cpp、PreparationAdapterTests cpp、有限冻结索引、本Report/Log；只暂存这些文件并正常提交推送，不强推，不包含两份OverallReadiness修改或103原未跟踪用户文件。所有11项原件SHA一起核验，提交标识由Git历史记录，不在提交自身中伪造自引用哈希。

23:36:46.4653752UTC最终PreStage核验通过：1617锁定产品输入、103原用户精确集合/哈希、两份保护文档、11项原件SHA、114个相对链接、7项最终健康执行、2760个独立完整根用例、实际覆盖及映射自检均一致；暂存0、未跟踪105仅多出本Report/Log，diff check通过。有限索引已补P28.23，提交前后继续核对精确七路径及原保护集合。

FZ-1/2仍开放：空间包多对象恢复、其他物品/容器入口、强制EndPlay及完整权威路由不能以本轮结果代替逐项证据。未新增公共操作API、持久schema、生产故障端口、第二权威或恢复系统；未启动Editor UI/PIE/Standalone/产品exe，未改正式地图、内容、玩法或UI，不做Smoke/Cook/Package，不暂停自动化或进入F。Saved原始日志仅保留本地，GitHub Log提供准确路径与SHA。

- [Report](../Report/Dev.D.UE.0.0.10.P28.23.r0_report.md)
