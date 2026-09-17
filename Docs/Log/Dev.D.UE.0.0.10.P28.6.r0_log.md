# Dev.D.UE.0.0.10.P28.6.r0 Development Log

最终状态：`P28_6_VERIFIED`，22:41 heartbeat完成整轮证据核验。以下第1至6节是保留的执行时间线，其中IN_PROGRESS只描述当时状态；最终结果见第7节。

## 1. 入场

`P28_6_IN_PROGRESS`。2026-09-17 19:57 heartbeat。入场 HEAD `83f1ec142b627832fa016236907c101312b9fb2f`，分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。已读取 P28.5 Report/Log、共享 P 阶段基线、冻结索引和 Git 状态；未发现仍运行的 UE/构建。P28.5 已交付，不重复其验证。

入场 index 为空；仅两份 OverallReadiness 用户跟踪修改和103个原未跟踪用户文件。复核 P28.5 validation-inputs 中1617个产品/脚本输入和103个用户文件 SHA 全部一致，生成本阶段 `Saved/Automation/P28.6/baseline.json`，更新 HEAD 与实际 UTC。

保护 OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`；Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`，不修改/暂存它们。

## 2. 审计和测试设计

核对 Manager.RollbackPreparedProfileRunFor0909B、两类 Start/Activate、DeactivateProfileWorld，Flow.CancelActiveRunForActivationFailure/Initialize/Start/Presentation，Runtime.RequestSettlement/PrepareForPersistentRun，GameMode.Rollback0909BPreparedRun 和 M01RuntimeAdapter.CancelAttempt。

当前顺序：Flow 成功清 Runtime、Phase=Preparation → Manager.DeactivateProfileWorld 可能保留失败世界 → Manager 仍返回 true 并清 bSettlementPending → GameMode/Adapter 据 true 清上层身份。后续 Flow.Cancel 因非 RunActive 拒绝。不能通过先销毁世界再调用 Flow 解决，因为 P28.4 已证明下层拒绝时须保留世界。

新增 ManagerRollbackContinuation 使用真实隔离 Flow 和持久 Run，而非手工伪造已接受结果。现有 preparation fixture 增加可选 Flow 初始化：仅该测试使用 Flow 已允许的隔离 automation 根，其他测试根保持不变；不放宽生产根规则。两份产品头只加测试友元，尚无行为修复。测试不要求已成功清理的 Runtime 物品继续存在。

## 3. 当前执行

- Red Editor 20:04:47至20:11:21UTC结束，51 actions / 393.85秒 / native0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.6.r0.RedProof/BuildEditor/20260917T200447269Z-f64ea2e4/stdout.log`，SHA `37C1F4ACB198DC9DCC63E588BDD87E7C7534BDADED4C153AE2685CBAF5DCCE88`。
- RedProof 实际0/1、队列1/native0、崩溃指标0；20:12:49UTC测试完成。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.6.r0/RedProof/20260917T201140037Z-3a513ed9/UnrealEditor.log`，SHA `EFC0B125F6F73D6AAF073BB9CC3FCD78320144D22E75167FB1D9D8811146022E`。首轮错误返回完成，两次待清理上下文断言失败；恢复条件后仍不能完成，两类Actor销毁次数均0，最终状态未清。Runtime提交次数1和持久身份断言未失败。启动阶段日志含既有Error/Warning，不宣称所有Error行数0。
- 原映射自检 `Saved/Automation/P28.6/regression-selftest.log`：517/517/native0，SHA `96B77D69044EEA0655A097D38D0629A20A894FF21CE4352562003F17ABCE6A5F`。
- 扩展映射自检 `Saved/Automation/P28.6/regression-selftest-final.log`：529/529/native0，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。新增四路径各验证仅旧根拒绝、仅新根拒绝、双根接受；不覆盖原自检日志。
- Fixed Editor 20:16:16至20:20:53UTC完成，33 actions / 277.01秒 / native0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.6.r0.Fixed/BuildEditor/20260917T201616232Z-e4690baa/stdout.log`，SHA `DCCFCEA20EDD1A7C117779F8967174E86CA521AF789657CC9F859E15FD020C17`。
- 最终 WorldLifecycle 专项6/0、精确队列6/native0、崩溃指标0，20:22:51UTC完成。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.6.r0/WorldLifecycleFocused/20260917T202141727Z-d4e45ab2/UnrealEditor.log`，SHA `B34C2D2956F3B0D7B24B46DFD54EFCE557FAF965883F462D55723371F81F80DA`。新增上层组合、旧直接世界去激活与GameMode保持用例均通过。
- 最终 ProductFlow 专项5/0、精确队列5/native0、崩溃指标0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.6.r0/ProductFlowFocused/20260917T202252902Z-5f265ee2/UnrealEditor.log`，SHA `1CFE66D8A78EBFCBD979BBD519DD4F64A9C09C31FA147D5FBD3ECC6BE5705909`。P28.4实际持久故障/健康回滚变体继续通过。
- 20:20:53UTC锁定1617个最终产品/脚本输入和103个原用户文件至validation-inputs.json；相对入场只有10个预期产品/脚本路径变化。两份OverallReadiness仍保持原哈希。
- exec59481串行运行finish-validation.ps1；runner为pwsh PID21976，外层PID26932。先专项、后Game、再旧根与新根；尚无完整阶段结果、提交或推送。不要因本段PID号仍存在认定仍运行，需结合进程命令行和各run-state。
- Game已开始，52个编译动作，当前仍运行；不要以进行中动作数当成功，也没有运行产品exe。20:24附近复核锁定1617输入、103原用户文件和两份OverallReadiness哈希全部未变；index为空，105未跟踪路径仍为103原文件+2阶段草稿。git diff --check和三条草稿相对链接检查通过；最终阶段提交门仍待完整验证。

## 4. 20:58 heartbeat续查

同一exec59481与runner21976/外层26932继续运行，无重复启动。Game于20:29:19.9419111UTC完成，52 actions / 358.07秒 / SUCCEEDED/native0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.6.r0.Validation/BuildGame/20260917T202321412Z-d71f9c89/stdout.log`，SHA `4F67D0FE0E584E1F3B2DFD8A3DAB4ECC7194F9C441F90011C3071C8E1F6B1791`。仅编译，不启动产品exe。

完整旧根已结束1330/0、精确队列1330/native0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.6.r0/LegacyFullRoot/20260917T202921406Z-602a794b/UnrealEditor.log`，SHA `87C270719D7F41560ED0B7B741B3F048CD36C080EF928ABC305E4EDD3CAE5750`。

完整新根自20:30:57UTC启动，UE PID18000，原件目录 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.6.r0/ShanmenFullRoot/20260917T203057611Z-3ac3f693`。本次读取部分901/0，日志仍更新至20:59:02UTC，尚无最终队列或run-state，Fatal/Ensure/Unhandled指标0。存在HTTP探测超时Warning和大delta提示，不将其声称为测试失败，也不称全日志无警告。进程名/命令行均匹配原验证实例。

20:59:22UTC复核1617个锁定输入、103个原用户文件和两份OverallReadiness哈希均未变；index为空，105未跟踪路径仍为103原文件+2草稿。保持产品输入不变，等待同一新根；不提前提交、推送或进入下一阶段。

## 5. 反例之后的修复

确认已有上层 Coordinator.StartM01Run 在 TechnicalStartFailure 会直接拒绝，再次进入没有恢复通道；Framework 现有 Start 请求仍会调用该方法。本阶段把同一条链的续接一起闭合，而不是新增恢复服务：技术失败分支只重试原 CancelAttempt/Run，成功后仅 AtSect，当前调用不会 BeginAttempt、恢复输入或启动玩法。

Manager 保存实际已接受的单个 Runtime rollback 前缀及其原绑定，只在调用下层成功后建立；重试不再调用 Flow.Cancel。World去激活返回false时保留前缀和bSettlementPending，完成才清。逐项比对Flow指针、弱Runtime/Session/World/GameMode、Profile/Run、存储根、提交次数和inactive/preparation状态；原Mode已失效也拒绝。各启动/激活入口在任何新提交和UI动作前检查pending，GameMode.Prepare在清旧correlation前拒绝。

新增用例扩展为上层组合：先绑定真实Flow启动产生的authority correlation，测试友元构造“已进入激活”的瞬态尝试，不伪造成功回滚回执；首次真实回滚拒绝、再次Start请求续接仍拒绝、错误Runtime绑定拒绝、恢复条件后只进入AtSect，最后相同Adapter取消为空操作。仍不跑正式M01/BeginPlay/输入恢复。既有P28.4下层拒绝保护及P28.5直接世界保护均在专项/完整根范围。

新增精确 PreparedRunStartComposition 映射覆盖两个类的h/cpp，要求demo_map与Shanmen.0_0_10双根；对应12个自检证明任一根缺失都会拒绝。映射涉及真实改动文件，不是仅按本轮主题挑测试。

本地忽略目录 validate.ps1 使用既有受跟踪启动器运行 UnrealEditor-Cmd：精确计数/失败数/队列/native/crash 联合判定。finish-validation.ps1 将串行执行专项、Game、旧根、新根，各段核对固定输入，不重复启动。原始日志仅本地，GitHub Log 只提供路径和 SHA。

## 6. 21:32 heartbeat续查

21:36:02UTC检查同一新根实例：exec59481尚未结束，pwsh外层26932/runner21976及UnrealEditor-Cmd 18000的进程名、命令行均匹配原验证。新根部分1139 Success / 0 Fail，日志更新至21:35:52UTC；尚无最终测试队列完成行、run-state或原生退出码，Fatal/Ensure/Unhandled指标0。命令行中的TestExit参数不作为队列完成证据。HTTP探测超时与大delta提示仍存在，未将部分结果误报为完整通过。

21:36:19UTC重新计算1617个锁定输入、103个原用户文件哈希，全部一致；两份OverallReadiness仍保持入场哈希，index为空，105未跟踪路径仍为103原文件加2阶段草稿。git diff --check通过（仅行尾转换提示）。本轮仅更新进度记录，未修改验证输入、重复启动验证、暂存或推送，继续等待原新根实例。

## 7. 22:41 heartbeat完成核验

原exec59481正常退出0，未重复启动。完整新根于22:08:43.3987648UTC结束：1429 Success / 0 Fail，精确队列1429、run-state SUCCEEDED/native0、Fatal/Ensure/Unhandled指标0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.6.r0/ShanmenFullRoot/20260917T203057611Z-3ac3f693/UnrealEditor.log`，SHA `B866502C4612E2248DADADD64706B2BEFCA08DBEBE0E4DEE95795FF5E4437D09`。本轮连续运行约97分46秒；不拼接之前的901/1139计数，不把专项再次计入总数。

本轮重新读取最新Report/Log、实际共享基线P_STAGE_BASELINE_0_0_10.md、有限索引，复查全部10个源码/脚本差异。22:41:40UTC对1617个最终输入及103个原用户文件重算SHA，全部一致；两份OverallReadiness仍为第1节保护SHA。原9份已记录日志SHA重新计算全部一致，新根另行核验。索引更新为P28.6局部闭合证据，FZ-1/FZ-2尚未全闭合，未执行或宣称FZ-3最终冻结。

精确13阶段路径由8个C++文件、ShanmenRegressionMap.json、Test-ShanmenRegressionCoverageSelfTest.ps1及3份阶段文档组成。改动文件驱动检查使用本轮完整新旧根，结果 PASS Changed=13 / Rules=4 / Required=85 / Logs=2；原件 `Saved/Automation/P28.6/regression-coverage.log`，SHA `0106D18291FFEA483B3A2A70121662776067E2720BF0B4DB33654EC92F024615`。命中PreparedRunStartComposition、M01GameMode、ProductRunItemUse、ItemProductAdapters。脚本/文档分类不等于产品测试通过；产品通过依据前述本轮双根。

22:44:52UTC最终预检通过：54条相对文档链接、11份原始日志SHA、1617个锁定输入、103个用户文件哈希及完整未跟踪路径集合均一致；index仍为空，git diff --check通过（仅行尾转换提示）。双构建和两组专项run-state再次确认SUCCEEDED/native0；此前双根run-state也已独立核验。

最终提交只包含上述13路径；保留103原未跟踪文件和两份OverallReadiness修改，不上传原始Saved日志、不强推。发布结果以实际Git提交及远端分支核对为准，不把“已验证”提前等同于已推送。

- [Report](../Report/Dev.D.UE.0.0.10.P28.6.r0_report.md)
