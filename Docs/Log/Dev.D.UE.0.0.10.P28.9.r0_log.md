# Dev.D.UE.0.0.10.P28.9.r0 Development Log

最终状态：`P28_9_COMPLETE`，本阶段产品验证及文件驱动覆盖门通过。下文第4—7节是执行期间的历史检查点，最终结果见第8节；不是全框架冻结或F验收。

## 1. 入场

2026-09-18 19:45 heartbeat；分支agent/0.0.10-p27-28-formation-scatter-gamemode-composition，HEAD `ac435b420b4174260dec36dd28eb5c3de6c2d736`。完整读取P28.8 Report/Log与共享P/F基线；入场无运行中的UE/UBT，暂存区为空，仅两份既有OverallReadiness用户修改。

19:47:43UTC基线检查通过：1617产品/构建输入与103原未跟踪用户文件匹配P28.8最终锁，未跟踪路径一致。保护Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`、Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。捕获Saved/Automation/P28.9/baseline.json。

## 2. 调用审计与Red设计

核对Manager的终局请求、持久重试、整备/激活入口、DeactivateProfileWorld、EndPlay；核对ProfilePreparationFlow的提交/重试和成功后的Runtime准备、RunId失效；核对Runtime结算和GameMode的逻辑终止前缀/实际World销毁。候选问题是持久成功不等于World释放完成，现有调用方却无条件解除pending，而持久Flow已回Preparation，不能再次提交它来替代World确认。

新增测试ManagerSettlementContinuation采用既有真实隔离Profile/cutover及瞬态World夹具，实际启动持久Run和飞剑动作，将时间线推进至非零。仅以引擎Role拒绝真实飞剑Destroy，不伪造持久成功。使用公开RequestSettlementAndReload与RetryPendingProfileSettlement，检查释放拒绝、恢复、错误Runtime绑定、下层提交次数及快照不变。Manager/GameMode仅新增此测试的友元访问。未生成Controller、未调用正式地图BeginPlay或物理输入。

原exec88071启动Saved/Automation/P28.9/red.ps1：先Editor编译，再单条新增测试；生产逻辑保持P28.8。必须保留首次失败日志，以实际Red决定最小修复，不能把build/进程0当成测试通过。

## 3. 实际Red与修复

Red Editor于19:53:51.449UTC完成，51 actions / 263.22秒 / SUCCEEDED、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0.RedProof/BuildEditor/20260918T194927578Z-8c33e222/stdout.log`，SHA `1DD06CF228BAC793AEA9962BD2B447B9A4009EA57E1BA728CE5DDF029B5939F8`。

RedProof于19:54:42.838UTC完成，0/1、精确队列1、原生0、Fatal/Ensure/Unhandled指标0，原exec88071退出0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0/RedProof/20260918T195351529Z-3f634c3c/UnrealEditor.log`，SHA `4CAF35E2CEA795E0C46467E8BBC71F9159D8270344C67DC4FD2EE3325FDB3FC2`。五条核心失败断言是durable成功后的pending保持、World重试保留原回执、故障移除后完成，以及weapon/enemy销毁次数各为0而非1。持久快照和单次提交不变量未失败。Red里门已错误解除时不再强行启动新Run，避免夹具制造无关后果；修复后才继续覆盖两个启动/激活门。

实现单个Manager-owned瞬态PendingProfileWorldSettlement，保存已接受回执和原Flow/Runtime/Session/World/AuthGameMode/存储路径/Owner/Run/提交计数及提交后的StartedRunId。成功状态只表示持久接受，原World释放仍可能pending；绑定不匹配时返回SessionStateRejected但不覆盖保留回执。CompleteDurableProfileSettlementWorld验证原绑定与Preparation/RuntimeInactive状态，复用已有DeactivateProfileWorld；false则保留，true才清记录与pending并调用既有整备呈现。两个准备入口和两个激活入口增加此待确认门。未改持久实现、GameMode生命周期逻辑或正式内容；GameMode.h只有一个测试友元声明。

RetryPendingProfileSettlement发现已有回执时不再调用Flow.RetryPendingSettlement；若是原持久pending，则原重试真正成功后也转入同一完成端口。初次失败分支仅在DeactivateProfileWorld返回true后显示整备。没有为此恢复旧CSEMI、增加聊天审批或新持久重试系统。当前新增真实变体只覆盖直接持久成功后World拒绝，不把未测试的持久失败重试与前置容器清理、EndPlay或正式UI重试入口称为完成。

## 4. 锁定验证进行中

19:56:22.878UTC固定1617输入和103用户文件，较入场恰好四源码路径变化：Manager.cpp/h、GameMode.h、PreparationAdapterTests.cpp。原exec32772 / pwsh27296执行Saved/Automation/P28.9/finish-validation.ps1，串行修复Editor、ProductFlow5/WorldLifecycle7、Game、旧根1330/新根1430，各段前后核验输入锁。独立exec29210执行既有映射脚本自检。验证未完成前不改锁定输入、不重复启动UE或构建、不提交/推送。

## 5. 修复后专项结果与交还检查点

Fixed Editor于19:58:55.080UTC完成，27 actions / 118.73秒 / SUCCEEDED、原生0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0.Fixed/BuildEditor/20260918T195655970Z-97b504d4/stdout.log`，SHA `AA56D6854987E00A4DE588E9B2E69539995D2CF7FEA056C16A0B81D0276E164D`。

- ProductFlow专项5/0、精确队列5、原生0、崩溃指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0/ProductFlowFocused/20260918T195855761Z-b8055e4b/UnrealEditor.log`，SHA `D6870DA449BAEBBDA3763002EE6608EEA90809AAC2C30E48A2637975B4CC00FA`。
- WorldLifecycle专项7/0、精确队列7、原生0、崩溃指标0；新ManagerSettlementContinuation通过，原回滚两种变体继续通过；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0/WorldLifecycleFocused/20260918T195916292Z-353f417c/UnrealEditor.log`，SHA `756CE854F1B51CD4730CE7BF1E4A1ABF38F10C6BFD1C4B67A7B3D855EB14166F`。
- 独立映射自检exec29210退出0、529/529；原件 `Saved/Automation/P28.9/regression-selftest.log`，SHA `362F4979D0F2DDDAB139967B121993490AB4696ED7AC43D34FE6F6EA7A72A678`。脚本未改，输出相同不代表复用旧执行。

20:01UTC原exec32772仍运行Game构建（50 actions，尚未完成），目录Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0.Validation/BuildGame/20260918T195937196Z-894e4266；随后才会执行旧根1330、新根1430。再次核验1617锁定输入和103用户文件SHA全匹配，两份OverallReadiness文档哈希不变，未跟踪精确105=103用户+本Report/Log，暂存为空。当前不更新冻结索引、不提交/推送；下一轮接续同一运行器的原始证据，不重复验证。完成后还需七路径映射（四源码+索引+Report/Log）、文档检查与精确发布。

## 6. 20:34 heartbeat续接：双目标与旧根完成，新根运行中

本轮只续接原exec32772 / pwsh27296，没有重新编译或重复启动测试。读取现有结果并核对实际进程：新根UnrealEditor-Cmd PID39220、父进程27296，参数仍为Shanmen.0_0_10、Unattended/NullRHI/NoSound/NoCompile，无Editor UI或产品运行。

- Game于20:03:34.147UTC完成，50 actions / 236.66秒 / SUCCEEDED、原生0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0.Validation/BuildGame/20260918T195937196Z-894e4266/stdout.log`，SHA `52B69A2297196367A9DABD534A877155AFC0CA2FCD5D8EB15C5AF3B88C1C6B7A`。与本阶段Fixed Editor合计双目标通过，但不等同完整自动化通过。
- LegacyFullRoot于20:04:50.178UTC完成，1330 Success / 0 Fail、精确队列1330、原生0、Fatal/Ensure/Unhandled指标0；原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0/LegacyFullRoot/20260918T200334653Z-e0607fda/UnrealEditor.log`，SHA `4220B7626D89BD2229DF204AA718DBA5961B073BEB30269ED37A3EA422C7DA91`。
- ShanmenFullRoot原件仍写入Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0/ShanmenFullRoot/20260918T200450457Z-6edab816/UnrealEditor.log。20:37:41UTC快照为945 Success / 0 Fail、崩溃指标0，最新成功是20:37:36UTC的SwordRhythmEffectCuePresentationRunController.FifoBackpressure；无最终队列与run-state。可见HTTP generate_204超时和8—10秒大Tick间隔警告，不能称无警告；有持续用例进展，保留原运行器等待，不杀进程或修改锁定源码。

20:38:10UTC再次逐项核验1617输入+103用户文件SHA匹配，两份保护文档哈希不变，未跟踪路径精确105=103用户+本阶段Report/Log，暂存为空。git diff --check通过，仅既有LF/CRLF提示。本阶段继续IN_PROGRESS；索引仍指向已发布P28.8，尚未提交/推送P28.9。新根结束后还需1430/0、精确队列/原生退出/崩溃指标核验，完成七路径改动覆盖门、文档校验与精确发布；不提前进入下一缺口或最终冻结。

## 7. 21:12 heartbeat续接检查点

分支/HEAD仍为入场基线，原exec32772 / pwsh27296 / UnrealEditor-Cmd39220持续执行同一新根。21:12:41UTC实读1188 Success / 0 Fail、Fatal/Ensure/Unhandled指标0，未生成最终队列或run-state；21:13:08UTC又记录TrustedReplay成功，随后启动UnknownOutcomeClosure。有HTTP超时及37.12秒大Tick警告，但有实际前进，不把慢执行当完成或失败，不中断重启。

21:13:13UTC核验1617锁定输入、103原未跟踪用户文件、两份OverallReadiness保护哈希全部一致，未跟踪路径精确105，暂存为空。仅更新Report与本Log的进行中状态及本地续接记录；没有源码改动、重复构建/测试、提交或推送。双目标与旧根沿用本阶段已完成原件，新根及七路径覆盖门仍待结束后核验，不宣称整个P28.9完成。

## 8. 最终结果、覆盖与发布审计

21:48 heartbeat恢复后，确认原exec32772已退出0，最终输入锁检查通过；没有重启测试。新根原生run-state为SUCCEEDED/0，于21:16:42.764UTC结束；日志1430 Success / 0 Fail、精确队列1430、Fatal/Ensure/Unhandled指标0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.9.r0/ShanmenFullRoot/20260918T200450457Z-6edab816/UnrealEditor.log`，SHA `9DEC92494E0AEA47322523065F15061B89CBF9A2BE08788AFA71FF1D5F5F7396`。旧根1330/0与新根1430/0合计2760条根测试，专项不重复累计；双目标结果见第5—6节。保留已有HTTP超时、大Tick间隔警告，不将其隐藏为无警告或直接据耗时评估性能。

21:49:54UTC再次核对1617锁定产品/构建输入、103原未跟踪用户文件，哈希和路径全部匹配；两份保护OverallReadiness哈希不变，暂存为空，未跟踪精确105。核对最终源码差异恰为四路径：Manager.cpp/h、GameMode.h、PreparationAdapterTests.cpp；GameMode.h只加测试友元。下层持久实现、内容资产、映射与构建脚本未改。

本轮实际执行七路径文件驱动覆盖检查：四源码 + FoundationClosure_Index + Report/Log；Changed=7、Rules=3、Required=84、Logs=2，退出0。原件 `Saved/Automation/P28.9/regression-coverage.log`，SHA `3EF32939B7CB43551E4E47C9D355BCCEAD4F39AF6C7300177108502002FFDBEA`。所需组包括demo_map.Profile/CodeB/EnemySkillFramework/ItemUseAndArmor/P4.Hotbar/V2RangedCompatibility/V3.Attributes及Shanmen领域/产品入口，由本阶段健康完整根覆盖；没有因GameMode.h仅为友元而跳过映射规则。自检529/529证据见第5节。

有限冻结索引更新为P28.9局部完成，FZ-1/2剩余审计和FZ-3最终冻结仍未完成。Report明确单个瞬态回执不是第二持久权威，IsDurablySettled不等同World完成；实际测试不覆盖持久失败重试成功变体、前置Runtime容器清理、强制EndPlay或正式UI可达性。后续先取可达性证据，不添加通用恢复框架。

21:53:58UTC发布前复核通过：10项原始证据SHA、64个相对文档链接、1617输入和103用户文件及两份保护哈希全部匹配。精确暂存上述7路径，暂存差异检查通过；远端分支仍为父提交ac435b4，没有覆盖他人远端提交。发布到现分支并非强制推送origin，提交自身身份以Git记录及最终交接链接为准，不能写成父提交ac435b4。原始日志仅存在本地Saved，没有随文档上传GitHub。

- [Report](../Report/Dev.D.UE.0.0.10.P28.9.r0_report.md)
