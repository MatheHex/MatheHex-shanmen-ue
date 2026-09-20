# Dev.D.UE.0.0.10.P28.20.r0 Development Log

状态：`P28_20_COMPLETE`（本阶段验证完成，不是整体冻结）。Manager独有遭遇对象释放拒绝已复现并实施最小修复；双构建、专项、完整双根2760/0及实际改动覆盖通过。第1—9节是执行时记录，不代表当前仍在运行；最终证据与精确交接范围见第10节。首次中断原因未证实、原件保留，FZ-1/2继续开放。

## 1. 入场及用户文件保护

2026-09-20T08:39:37.399Z heartbeat。分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `a8c8cea3eb6ffd09cd199ee3f3973a65b7f6b95d`。读取最新P28.19 Report/Log、续接、Git状态及P/F共享基线，核对有限索引未完成项。08:39:57.5090135UTC检查上一阶段锁定1617产品输入、103原用户文件精确路径/哈希、暂存0、10项原件SHA、98相对链接通过；没有UE-Cmd运行，不重跑已完成阶段。

两份用户OverallReadiness修改保护哈希：Report `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。本阶段不编辑、不暂存它们或103原未跟踪文件。

## 2. 可达性与选择

检查GameMode与Manager剩余清理调用点后，选择Manager拥有但GameMode未绑定的遭遇对象：InitializeWorldContent的非M01路径调用InitializeEnemyEncounterContent，循环按FullMapDistribution创建14个对象并保存在EnemyActors，只在最后将三个Primary对象传给Mode.BindV3EnemyProjections。GameMode释放三个槽之后，其他对象仍由Manager持有。

DeactivateProfileWorld在GameMode完成后调用DestroyRuntimeContainers，后者调用DestroyEnemyEncounterContent再清奖励上下文；仅当其返回成功才继续Items.TeardownWorld。旧敌人清理为void、先清原数组和导航标记、忽略Destroy返回，直接构成拒绝后原引用丢失且下游提前释放的真实端口缺口。

初始化内的其他六个共用清理调用点均用于失败退出；本阶段只静态核对它们，不运行正式地图、NavMesh或完整激活。EndPlay、其他容器调用点、空间包及非M01完整再进入仍开放，不把本次局部证明扩大成全入口关闭。

## 3. 原始Red与最小修复

只扩展既有ManagerDeactivationRetention，不增加注册测试或友元：完成原战斗/散落物/灵石变体后，在相同实际AuthGameMode的瞬态World追加两个Manager独有遭遇对象和两单位SpiritDust。导航标记预置两个不同非空值，一个遭遇对象设ROLE_SimulatedProxy真实拒绝两次；另一个应只销毁一次。断言原遭遇owner和标记、下游物品身份/数量/绑定、Manager活动上下文、非零持久快照及原Run保持，解除拒绝后所有对象恰好释放一次，重放成功。

08:42:08.3658050UTC Red锁1617输入/103用户文件，只允许测试源码改变。Red Editor于08:42:31.0220481UTC完成，4 actions/21.48秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0.RedBuild/BuildEditor/20260920T084209215Z-5d2706dc/stdout.log`，SHA `54D59CD97B342742F39E513EF98440976A44BCDA2E5E92B5767AA420AE77FA5C`。

RedProof于08:42:51.9500065UTC完成，0成功/1失败、精确队列1、原生0、崩溃指标0，最终5条Expected失败为两次错误完成确认、两次原owner/标记/物品保持、恢复后恰好一次释放；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0/RedProof/20260920T084231528Z-76be681d/UnrealEditor.log`，SHA `4448E3DDB6F156616E5E7455FEEAFE96B9338F8E029E4C9EE99266BE9808A6E3`。原exec84895退出0仅说明预期反例流程结束，不能把测试失败说成通过；原件保留。

修复只将私有DestroyEnemyEncounterContent改为bool，快照遍历、保留实际拒绝项、全部完成后才清NavigableEnemySpawnMarkerIds；DestroyRuntimeContainers先取得敌人释放确认，再清其余上下文并允许下游World释放。成功对象不在续接时重做，不增加第二份owner或恢复结构。生产cpp 11新增/6删除，私有声明1新增/1删除，测试57新增，总69新增/7删除；注册数不变。

## 4. 最终验证执行

08:43:29.9328043UTC Final锁1617输入/103用户文件，Saved/Automation/P28.20/validation-inputs.json。原exec48438，pwsh25844/父11128；按Editor→WorldLifecycle7→ProductFlow5→Game→LegacyFullRoot1330→ShanmenFullRoot1430执行，每段前后校验输入。原运行器健康时不重复启动，不更改锁定产品输入。尚未取得最终双构建/回归结果，不提前暂存提交。

独立映射自检exec98545正常退出0，529/529；原件 `Saved/Automation/P28.20/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。相同哈希来自本阶段实际执行的确定性输出，不是复用旧日志。

预定六路径静态映射命中ProductRunItemUse与ItemProductAdapters，必跑7组：Shanmen.0_0_10、Shanmen.0_0_10.Items、demo_map.Profile、demo_map.CodeB、demo_map.V2RangedCompatibility、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar。实际覆盖脚本Saved/Automation/P28.20/coverage.ps1仅在本阶段两个健康完整根完成后执行，不用静态映射或自检替代实际覆盖。

## 5. 发布条件与边界

最终核验双构建原生退出、专项/完整根计数与队列、原件SHA、输入锁、用户精确集合/保护哈希、文档链接及diff check；只精确提交Manager cpp/h、既有PreparationAdapterTests、有限冻结索引、本Report/Log六路径，再推送并核对远端HEAD。原日志Saved仅本地保留。

有限索引暂留P28.19，不提前声明本阶段通过。FZ-1/2仍开放；本次不涉及敌人行为或玩法数值，没有Editor UI、PIE、Standalone、产品exe、正式地图/内容编辑、物理输入、UI开发、Smoke/Cook/Package，不暂停自动化或跨越P/F边界。

- [Report](../Report/Dev.D.UE.0.0.10.P28.20.r0_report.md)

## 6. 08:47UTC验证检查点

原exec48438继续运行；已完成三项最终证据：

- Editor于08:45:40.9373898UTC完成，27 actions/129.78秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0/BuildEditor/20260920T084330803Z-a57954d0/stdout.log`，SHA `008252558F753CD19D76FDF7D1193B5366385D2FAC7CCB55F0B1DB69E3210E1F`。
- WorldLifecycle于08:46:01.8117001UTC完成，7成功/0失败、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0/WorldLifecycleFocused/20260920T084541408Z-804ba627/UnrealEditor.log`，SHA `A1367D44E0BAC55C608F7CD10042263D90171853F5CF0D45FA428527AF3A2720`。
- ProductFlow于08:46:22.2794021UTC完成，5成功/0失败、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0/ProductFlowFocused/20260920T084601899Z-6ac83d5e/UnrealEditor.log`，SHA `9F6B9BC83E00660ADA3F1BFDB246BCB21CEEA1F7ABAA7C77DCE25648913AB184`。

08:47:20.2163865UTC复核1617锁定产品输入、原103用户文件精确集合/哈希、两份保护文档、既有原件SHA、未跟踪105、暂存0与diff check通过。Game和完整双根继续沿同一运行器执行，不把专项通过提前视为全阶段验收，不执行实际覆盖门或暂存/推送；有限索引仍保持已发布P28.19。

## 7. 09:22UTC续接检查点

2026-09-20T09:21:37.955Z heartbeat先读取分支/HEAD、Git状态、最新Report/Log、续接和P/F基线，恢复原exec48438仍运行，未重复启动。新增两项已完成证据：

- Game于08:48:32.5370889UTC完成，26 actions/129.55秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0/BuildGame/20260920T084622684Z-3158cc88/stdout.log`，SHA `DC8366C47BE8ABA0D6426216CF21EC3A15EB55066A59C5F448C3DF840D8DC328`。
- Legacy完整根于08:49:48.3685214UTC完成，1330成功/0失败、精确队列1330、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0/LegacyFullRoot/20260920T084832875Z-d53e339f/UnrealEditor.log`，SHA `F73B1CFE891003FDCB6B1F5C1E3267E692E6C2B03A0743AAEB0A846121C96A53`。

Shanmen原目录20260920T084948679Z-698ae62e，09:22UTC观察1021成功/0失败、崩溃指标0；日志09:22:28UTC完成ThrownWeaponArcPreLaunchPreviewContext.CancelAndRunFences，随后开始LiveEditCancelAndConfirmation。原UE-Cmd42912/父25844存续、日志推进，无最终队列/run-state/原生退出，不给运行中原件最终SHA，不把部分结果当1430全根通过。

09:22:37.9421888UTC核验1617锁定输入、原103用户文件精确集合及哈希、两份OverallReadiness保护哈希、六项此前完成原件SHA、未跟踪105、暂存0、有限索引未改和diff check通过。保持同一产品输入等待原完整验证，完成后再做实际覆盖门和精确交接；不提前发布、不宣告冻结或进入F阶段。

## 8. 14:32UTC中断取证与一次有界恢复

2026-09-20T14:31:50.750Z heartbeat读取分支/HEAD、Git状态、最新Report/Log、续接和P/F基线。尝试读取原exec48438返回准确错误 `write_stdin failed: Unknown process id 48438`。随后独立检查：UE-Cmd进程数0，原42912/25844/11128均不存在；首次Shanmen目录没有run-state.json，日志最后写入09:50:39.5668948UTC，1181成功/0失败、崩溃文本指标0、无最终队列/原生退出。最后一个完成用例为ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.ReplayAtEveryBoundary，随后开始NextGenerationRotationSession.AdoptionTrustFences；尾部HTTP探测超时警告不是已证实的终止原因。

该次归类为执行中断、原因未证实。操作系统上次启动时间早于本轮，现有证据不支持直接归因于本机重启；同样没有证据将其视为源码失败或正常完成。原运行器最终退出码未知，不补写或伪造原run-state。中断原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0/ShanmenFullRoot/20260920T084948679Z-698ae62e/UnrealEditor.log`，SHA `142C9B49A1A92002714E10FA799C3E2748AB2EDFD60E3B5561BD4CF2A12FA164`；原目录全部保留、不覆盖、不拼接计数。

14:32:50.5562853UTC核验1617锁定输入、103原用户文件精确集合/哈希、两份OverallReadiness保护哈希、未跟踪105及暂存0均不变；独立复算双构建、两专项与旧完整根的原生退出/计数/队列及SHA，五个完成段仍健康。因此只重跑尚无完整结果的Shanmen根，不重复已经通过的五段，不改任何产品输入。

Saved/Automation/P28.20/recover-shanmen.ps1在前后核验原输入锁/HEAD，启动前拒绝已有UE-Cmd或已存在Recovery1目录，仍用UE-Cmd/NullRHI/Unattended及同一根组。恢复exec80008，UE-Cmd20856/父34700，独立目录 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0.Recovery1/ShanmenFullRoot/20260920T143413411Z-3c392778`。14:34:48.9870187UTC观察日志正常加载模块，尚未得到测试或终止结论。实际覆盖脚本只接受原健康Legacy根与本Recovery1完整健康新根；首次1181条不算覆盖，不为运行中Recovery1填写最终SHA。

本次只修复本地验证续接，不修改生产源码、测试内容、正式资产或自动化范围。等待此唯一恢复进程；未达到阶段发布或总体冻结条件。

14:36:37.8568883UTC复核1617输入/103原用户文件、9项完成或中断原件SHA、4个相对链接、有限索引未改及diff check通过。Recovery1原exec80008仍运行，新日志累计502成功/0失败、崩溃指标0且持续推进；没有最终队列或原生退出，不把该部分计数作为通过证据。

## 9. 恢复进程续接检查

2026-09-20T15:09:21.313Z heartbeat读取当前分支/HEAD、最新Report/Log、续接和P/F边界后，确认原Recovery1 exec80008仍运行，没有重新启动任何验证。15:12:14.0723516UTC独立观察同一UE-Cmd20856/父34700存续，日志最后写入15:11:30.4311516UTC，累计1042成功/0失败、崩溃指标0；最后完成ThrownWeaponArcPreviewMainHUDRendererAdapter.InitializationAndPhysicalIdentity，随后开始ThrownWeaponArcPreviewMainHUDRuntimeBinding.ExactUpdateReplay。无最终队列、run-state或原生退出，属于正常验证等待，不能视为1430全根通过。

15:13:11.2580157UTC复核HEAD a8c8cea不变，1617锁定产品输入、103原用户文件精确集合/哈希、两份OverallReadiness保护哈希、9项已完成或中断原件SHA及4个相对链接全部匹配；未跟踪105、暂存0、有限索引仍为P28.19，diff check退出0。仅更新本阶段记录，不更改锁定源码、测试或用户文件；实际覆盖、阶段提交/推送及有限索引更新继续等待本次恢复完整结束，FZ-1/2仍开放。

2026-09-20T15:46:21.882Z heartbeat续接仍为原exec80008、UE-Cmd20856/父34700。15:46:55.7665163UTC读取同一日志，最后写入15:46:55.3877616UTC，1211成功/0失败、崩溃指标0，完成ThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.InvariantFailures后开始PreflightAndReentrant；无最终队列/run-state/原生退出。计数已超过首次中断位置，但不能据此推断首次终止原因或将恢复声明为完成。15:47:36.6153019UTC再次核验1617输入、103原用户文件精确集合/哈希、两份保护文档、9项原件SHA、4个相对链接、未跟踪105、暂存0、索引未改及diff check通过；本轮没有重启、产品改动、覆盖门执行或提交。

## 10. 16:21UTC完整恢复与最终交接

2026-09-20T16:20:22.333Z heartbeat读取分支、最新Report/Log、Git状态及P/F基线，原exec80008返回实际退出0。Recovery1 run-state记录SUCCEEDED、原生0、完成于15:47:25.7826086UTC；第9节最后测试观察时间为15:46:55UTC，不能将稍后文件保护核验时间误当成进程存续观察时间。

16:21:03.9323380UTC独立复算新根1430成功/0失败、精确队列1430、崩溃指标0，当前UE-Cmd进程0；恢复原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.20.r0.Recovery1/ShanmenFullRoot/20260920T143413411Z-3c392778/UnrealEditor.log`，SHA `2B4E6DC8CAB6283DE067BB211F2F5C39219F8CF9C240ED28CD5E2263F515BDB8`。原exec48438依然没有最终退出证据；本恢复不改变首次执行中断的归类。

实际覆盖使用原健康Legacy1330根和Recovery1健康Shanmen1430根，六路径命中ProductRunItemUse与ItemProductAdapters两规则，七必跑组全部具有健康证据；覆盖执行退出0，原件 `Saved/Automation/P28.20/regression-coverage.log`，SHA `F0B5D04A44272CAAF2CB41920802A23FE41CE32E38C966575F2F65FDBCFF9B2E`。完整根合计2760项，不额外累加专项7/5、Red或中断1181条。

最终核验按Saved/Automation/P28.20/verify-final.ps1分别检查暂存前、六路径精确暂存后及提交后状态：1617输入/103原用户文件哈希与精确集合、两份OverallReadiness保护哈希、11项完整或中断原件SHA、所有本阶段相对链接、六段健康run-state与测试计数/唯一名称/最终队列、diff check。只交接Manager cpp/h、既有PreparationAdapterTests、有限冻结索引、本Report/Log；原始Saved日志和用户文件不上传。

有限索引更新至P28.20局部证明，不关闭FZ-1/2，不暂停自动化，不进入F或实际玩法。本阶段提交自身标识由包含本文的Git提交提供；推送及远端一致性核验在交接结果和本地续接记录记录，不伪造自身提交哈希。

16:25:47.4933081UTC实际PreStage核验通过：1617锁定输入、103原用户文件、未跟踪105、两份保护文档、11项原件SHA、102个相对链接、六段健康最终运行、2760个唯一完整根测试名；暂存0，diff check通过。六阶段路径已逐项复审，无额外源码或用户文件混入。
