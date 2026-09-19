# Dev.D.UE.0.0.10.P28.14.r0 Development Log

状态：`P28_14_COMPLETE`。完整阶段验证通过；第1—9节保留当时的失败、修复与续接历史，最终结果见第10节。阶段完成不等于底层整体冻结。

## 1. 入场与保护

2026-09-19 17:33 heartbeat，分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `478bd1670180fc8933066b7a79c5adfc44b19f9e`。读取P28.13 Report/Log、P/F基线、有限冻结索引和已发布续接记录；无运行中的UE-Cmd/UBT，暂存为空，仅两份OverallReadiness有原用户修改。前阶段1617输入和103原未跟踪路径/哈希一致，不重做P28.13。

保护Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`，Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。原103文件不纳入本阶段。

## 2. 调用链与首次失败

Manager.RequestSettlementAndReload先执行Runtime.RequestSettlement，再提交Flow的持久终局。Runtime的SettleRunItems把World物品变为Destroyed并生成损失行，随后先删WorldActors绑定、忽略Destroy返回值、CompactDestroyedRun删记录；ValidateWorldBindings因两边都被删而错误通过。Flow.PrepareForPersistentRun之后也看不见仍存活的Actor，Manager现有World-only续接无法处理它。

在既有ManagerSettlementContinuation注册用例中保留直接/延迟持久成功两变体，并各增加实际两单位SpiritDust的散落World投影。沿真实Manager端口，先保留飞剑拒绝，再保留宝箱拒绝，最后两次仅剩散落物拒绝；要求精确原Run损失行/Destroyed身份/Actor绑定保持，已接受前缀与持久提交不重做，解除故障后仅一次销毁。瞬态Floor只用于既有安全投影检查，不是正式地图或物理输入。未加注册测试、友元或故障注入端口。

17:38:34.930UTC Red锁1617输入、103用户文件，只有测试源码变化。RedBuild完成4 actions / 11.80秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0.RedBuild/BuildEditor/20260919T173836014Z-a96b6317/stdout.log`，SHA `92B72976EE0D8E1CCC94F95B4112C8325A3A86FC49B983612D57AC554303794D`。

RedProof于17:39:09.133UTC完成0 Success / 1 Fail、精确队列1、原生0、崩溃指标0，12条最终Expected断言失败，分别为两新变体的损失身份保持、终局Runtime保持、两次续清理及恢复完成/一次销毁；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0/RedProof/20260919T173848686Z-3724b0c6/UnrealEditor.log`，SHA `8E772A1CFD1303635E1F8F624054F933E763C314148934294E9673A8F1625A2F`。引擎Condition failed诊断保留；exec52734进程结束0不是测试通过。

## 3. 最小修复与输入锁

Runtime不再移除拒绝销毁的终局投影，存在残留绑定时不压缩Destroyed身份。ValidateWorldBindings只接受既有Settled状态中Summary的World→Destroyed行与原身份/定义/数量匹配的额外绑定；普通World仍用原不变量。Teardown遍历普通World身份与实际绑定的并集，拒绝继续保留，全部成功才压缩该Run墓碑/清空间追踪。物品已逻辑损失，不重新授权拾取。

Flow.FinalizeShanmenSettlement只对原Summary匹配且Prepare返回InvalidWorldBinding的拒绝保留真实durable成功，不改写成FatalProfileError；其他清理错误仍失败关闭。直接Flow Start在残留终局投影时先拒绝，避免提前提交新durable Run。Manager在已有PendingProfileWorldSettlement中校验同一Flow/Runtime/Session/World/Run/终局原因，允许原Settled Runtime；World释放完成后才Prepare并清pending，不再提交持久终局。

测试新增竞争终局不能改写Summary和直接Flow Start不能提交新Run的断言，这两条追加检查未独立Red；原两种无散落物变体继续执行。四处源码为ItemSubsystem.cpp、ProfilePreparationFlow.cpp、V3ProgressionManager.cpp和PreparationAdapterTests.cpp，147新增/17删除；无头文件/API/schema/恢复记录新增。

17:40:55.157UTC Final锁1617输入和103原用户路径/哈希，Saved/Automation/P28.14/validation-inputs.json；git diff --check通过。之后不改产品输入。

## 4. 验证执行与下一步

原exec29056 / pwsh17168（父49224）串行执行Editor→WorldLifecycle7/ProductFlow5→Game→旧根1330→新根1430，每段前后核对同一输入锁。只有UE-Cmd Unattended/NullRHI与双目标编译；不重复启动正常前进的验证实例。映射自检另由exec35302执行，最终结果待读取。

本阶段七路径静态匹配ProductRunItemUse、ProfileAuthority、ProfilePreparationProductFlow、ItemProductAdapters四条规则；九组必跑为demo_map.CodeB、ItemEconomySchema、ItemUseAndArmor、P4.Hotbar、Profile、V2RangedCompatibility，以及Shanmen.0_0_10、Shanmen.0_0_10.Items、Shanmen.0_0_10.Items.ProductFlow。Saved/Automation/P28.14/coverage.ps1只在双根最终SUCCEEDED/原生0后运行，当前不是实际覆盖通过证明。

完整验证后更新有限索引、Report/Log，核验原始日志SHA和输入/用户文件不变，再精确暂存四源码与三文档七路径，提交并核对远端。本轮Saved原件仅本地保留，不上传；不提前标记COMPLETE或给不存在的GitHub阶段链接。

## 5. 尚未证明的边界

本阶段针对Shanmen正常Manager终局的直接/延迟持久成功路径；不将其推广为旧兼容终局、技术激活回滚后Runtime清理、空间包所有部分释放、其他容器/灵石/敌人调用点或强制EndPlay的证明。直接Flow Start围栏并非全部外部Adapter入口审计。未进入F阶段，未进行实际UI/地图/玩法或性能验收；FZ-1/2仍开放。

- [Report](../Report/Dev.D.UE.0.0.10.P28.14.r0_report.md)

## 6. 首次修复验证失败与有界修正

exec29056在首次WorldLifecycle专项失败后退出1，没有执行后续ProductFlow/Game/完整根。该轮Editor于17:41:30.977UTC通过7 actions / 34.39秒、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0/BuildEditor/20260919T174056227Z-bd7a08f8/stdout.log`，SHA `8D98E6C7E891C3917766927B4E5D2C56E7505357343A896DC4FDC390BBDDC868`。

WorldLifecycle于17:41:52.417UTC结束6 Success / 1 Fail、队列7、原生0、崩溃指标0，ManagerSettlementContinuation的延迟成功+散落物变体出现24条最终Expected失败；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0/WorldLifecycleFocused/20260919T174131467Z-705a8c89/UnrealEditor.log`，SHA `3001A5FBD391D39AEB3C7DBAB26292DFF4990A57065B715F863A1E02E7970D93`。这是本阶段首次修复缺陷，不归咎于环境、不与最初Red混为一轮，也不继续用该输入跑完整根。

根因：RetryPendingSettlement将PendingShanmenSettlement.GetValue()以const引用传给FinalizeShanmenSettlement；新增bWorldReleasePending判定放在PendingShanmenSettlement.Reset()之后，延迟变体失去原Summary匹配。移到Reset之前判定，同一引用生命周期内使用，随后才清原暂存；不新增状态、不更换原Run、不放宽测试。原17:40输入锁保存为Saved/Automation/P28.14/validation-attempt1-inputs.json。

17:44:49.253UTC重新锁定1617输入/103用户文件，源码149新增/17删除。新exec71947重跑Editor→专项→Game→双根，原失败exec29056已停止，不是并行重复运行；旧成功/失败证据独立保留。实际覆盖门仍等待最终健康双根，索引继续保持P28.13为最新完成阶段。

映射自检exec35302独立完成529/529、原生0；原件 `Saved/Automation/P28.14/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。输出与前阶段同SHA是确定性结果，本轮确已独立执行。

## 7. 修正后的专项验证与续接

新exec71947 / pwsh17092（父46116）在同一17:44锁定输入下运行。Editor于17:44:58.678UTC通过4 actions / 8.05秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0/BuildEditor/20260919T174450255Z-38e70730/stdout.log`，SHA `D59AAFFA866501A87E83956C1C19D3D6DD6B0FC17313E4BEAE1AA6997B7AA7CB`。

WorldLifecycle专项于17:45:19.637UTC完成7 Success / 0 Fail、精确队列7、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0/WorldLifecycleFocused/20260919T174459188Z-a1c81ded/UnrealEditor.log`，SHA `14A7B579AC6F46CD357885C254B78DB63DEC8BEF277CD0524D335AD59C25B7CC`。ManagerSettlementContinuation的四种组合（直接/延迟持久成功×原/散落物拒绝）及新追加竞争终局、直接Flow Start围栏均通过；两次失败原件仍独立保留。

ProductFlow专项于17:45:40.143UTC完成5/0、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0/ProductFlowFocused/20260919T174519734Z-abca9af4/UnrealEditor.log`，SHA `C6BB47943862736F8DE5F232CDBDD2A6C534DB86B2B997553318291EB80851B1`。同一运行器继续Game（目录BuildGame/20260919T174540586Z-21c3e6f1；当时快照3/6 actions），随后为双完整根，未重复启动或提前记最终SHA。

17:46:54.262UTC复核1617输入、103原用户路径/哈希、两份保护文档及此前5项完成原件一致；未跟踪105（103原文件+本Report/Log），暂存为空，HEAD仍478bd16。当前记录8项完成原件，索引保持P28.13，实际覆盖门、提交与推送仍等待完整根。仅记录续接，不修改锁定输入，不进入F。

## 8. 18:22 heartbeat续接检查点

读取当前分支/HEAD、Git状态、P28.13完成Report/Log、P28.14进行中Report/Log、P/F基线和有限索引。HEAD仍478bd1670180fc8933066b7a79c5adfc44b19f9e；原exec71947仍在执行，pwsh17092及其原UE-Cmd4416保持，没有启动重复验证实例或改动锁定产品输入。

Game于17:46:24.887UTC完成6 actions / 44.02秒、SUCCEEDED/原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0/BuildGame/20260919T174540586Z-21c3e6f1/stdout.log`，SHA `FE178FC0756052CFF8C1B5CBEA24534CBEF1F5516EA0AB40FC81AB492CC3B45E`。只编译Game目标，没有运行产品程序。

旧完整根于17:47:45.743UTC完成1330 Success / 0 Fail、精确队列1330、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0/LegacyFullRoot/20260919T174625245Z-381ee419/UnrealEditor.log`，SHA `2F988FCA297A8CD801455979FE47B1242D90E463A5E25F685C3FAE4F88C29167`。不是复用前阶段旧根，也不把专项计入该1330。

新根原目录ShanmenFullRoot/20260919T174746056Z-40c6218d，参数Unattended/NullRHI；18:23UTC读取快照1046 Success / 0 Fail、崩溃指标0，无结束队列/最终run-state。18:23:44.874UTC的ThrownWeaponArcPreviewMainHUDRuntimeBinding.LocalClearAndLifecycleFences完成Success，随后VisibleHUDRecreationAndDetach开始，证实正常推进；运行中的日志不计算最终SHA，不把历史命名含HUD的无头契约回归当成启动UI或实际表现验收。

18:24:22.052UTC复核1617锁定输入、103原未跟踪路径/哈希、两份OverallReadiness保护哈希及此前8项完成原件均未变。未跟踪精确105（103原文件+本Report/Log），暂存为空、git diff --check通过。当前Log累计10项完成原件，保留首次Red及首次修复失败；只补续接文档，不运行缺少最终新根的实际覆盖门，不暂存/提交/推送，不更新索引完成阶段或宣布整体冻结。

## 9. 18:57 heartbeat续接检查点

读取当前分支/HEAD、Git状态、最新Report/Log、P/F基线及续接记录，HEAD仍478bd16，索引最近完成阶段仍P28.13。原exec71947/pwsh17092/UE-Cmd4416仍是同一执行链，未重复启动验证或改动锁定源码。

18:58:31.234UTC同一新根快照1188 Success / 0 Fail、崩溃指标0，无最终队列或run-state；相较上次1046有实际进展。18:58:29.916UTC的ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.RequestContractAndDeterminism完成Success，随后TrustedReplay开始。HTTP generate_204超时警告原样保留，既不把该警告当断言失败，也不据当前健康过程宣称完整根通过或性能达标。

18:59:00.016UTC复核1617锁定输入、103原用户路径/哈希、两份OverallReadiness保护哈希、10项完成原件SHA均一致；未跟踪仍精确105，暂存为空、git diff --check通过。未对运行中的新根取最终SHA；实际覆盖门、索引完成标记、七路径提交/推送继续等待最终测试和进程状态，不进入F阶段。

## 10. 19:34 heartbeat最终验收与交接

读取当前分支/HEAD、Git状态、最新Report/Log、P/F基线、有限索引及续接记录。原exec71947已结束、原生退出0，运行器输出locked validation complete；同一17:44:49输入锁贯穿最终Editor、专项、Game、双根，不重复运行健康测试或修改已验输入。

新完整根于19:02:43.338UTC完成1430 Success / 0 Fail、精确队列1430、SUCCEEDED/原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.14.r0/ShanmenFullRoot/20260919T174746056Z-40c6218d/UnrealEditor.log`，SHA `6E9FB1AD72934D9B0E25D03491F0147887BAA8D29228F49AE7BEB5B3B3856E66`。加上本阶段旧根1330/0，完整回归合计2760成功、0失败；专项7+5独立执行但不重复增加完整根数量。HTTP超时等原始警告保留，不据测试通过宣称内存/性能或真实表现验收。

双根最终进程状态均为SUCCEEDED/原生0后，独立执行本阶段实际覆盖门，原生退出0：PASS Changed=7 Rules=4 Required=9 Logs=2。命中ProductRunItemUse、ProfileAuthority、ProfilePreparationProductFlow、ItemProductAdapters，第4节九个必跑组均在两份健康完整根中取得证据；原件 `Saved/Automation/P28.14/regression-coverage.log`，SHA `C6909E3FEC5A2F2071F20DAF311467F1ADD9A9ACE8932AB721C85B317C7FD295`。没有把静态规则匹配或529项自检当作实际日志覆盖通过。

19:36:11.996UTC复核1617输入、103原未跟踪路径/哈希、两份OverallReadiness保护哈希及此前10项完成原件全部一致；两组专项/双根的成功数、失败数、精确队列、崩溃指标和最终进程状态再次独立核对通过，Editor/Game状态也均SUCCEEDED/原生0。现在共12项完成原件，首次Red、首次修复失败及原输入锁独立保留，不删除/拼接或上传Saved证据。

最终只更新本Report/Log与有限索引，精确交接七路径：Source/demo_map下ItemSubsystem.cpp、ProfilePreparationFlow.cpp、ShanmenPreparationAdapterTests.cpp、V3ProgressionManager.cpp四源码，以及Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md与本Report/Log。暂存从空起步，不纳入103份原用户文件或两份OverallReadiness修改；包含七路径的提交是本阶段基线，远端一致性在提交后核验并记入本地续接及对用户交接，不在此预写未知哈希。

本阶段仅闭合正常Shanmen Manager终局中的散落物拒绝保持和原续清理；FZ-1/2仍需按实际入口做有限审计，不将本轮成功推广到技术激活回滚、直接外部Adapter启动、全部兼容终局、空间包部分释放或强制EndPlay。未启动Editor UI/PIE/Standalone/产品exe，未修改地图、内容资产、物理输入、玩法或UI；无Smoke/Cook/Package，不宣布整体冻结或暂停监控。
