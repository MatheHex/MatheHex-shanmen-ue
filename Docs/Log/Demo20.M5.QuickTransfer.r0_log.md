# Demo2.0 背包便捷领取 Development Log

## 1. 基线与交付范围

基线 `4c0025698e499aa06c151dcfa60d3d1cf58644e8`；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。状态 `CODED_NATIVE_PASSED_MANUAL_PENDING`，不是完整 M5 或 Demo 完成。提交身份由包含本文的 Git commit 给出，避免文档自引用 SHA。

产品增量：既有背包双击／便捷转移优先兼容未满堆，再找空位和替代方向；部分领取余量留来源。每次只发一笔原 Move／Merge，经同一持久权威确认刷新。原局修复只补此前候选发现的零字节读取边界，不扩展恢复框架。没有窗口操作、真实档写入、格式／数量权威变化或自动任务变化。仓库文档技能用于保留既有 Report／Log 与证据分级，不创建云端 Page。

## 2. 最终固定源码

13 路径，排序后的 `path<TAB>SHA256`，LF 连接无末尾 LF，UTF-8 SHA-256：
`52D79114975DE1D860802937D50152515DAD87F60FAE05571C32755C15632260`。

| Source/demo_map/Demo20/ 下的文件 | SHA-256 |
| --- | --- |
| ShanmenDemo20Expedition.cpp | E902CB4BC7D2EF1FE2E5DD2216510362517D8581F33F6E54D74C078DFAC1F666 |
| ShanmenDemo20ExpeditionTests.cpp | E0B113898AA19C1C0A8EB1340F658F29A5FB81A20AE1A55B8955197537261954 |
| ShanmenDemo20InventoryTransfer.cpp | 2E14E4C16B628712CC1A28CD3FEED5A3CD5D8A475FE31CF9E0A2C331EBC7A138 |
| ShanmenDemo20InventoryTransfer.h | 196F85B7FAD70699055DAA060911E350765F0711B83738E9B052BF7EA1C3BD95 |
| ShanmenDemo20InventoryWidget.cpp | 81247805A878F7BFCF8A02E0049AF192E2F2C08E08B60DEE16419F812E1BB549 |
| ShanmenDemo20InventoryWidget.h | 049A8B725CEA5B07FFCD8B69B2937028765E3FCBF4D58B41E5F88BE9AE940AE1 |
| ShanmenDemo20LootTests.cpp | 188BA49F0BB4924CF83EBA8B87DC62999BA4A4232F1BC46FCDBC07D2150AE3A4 |
| ShanmenDemo20MedicineTests.cpp | A8DF47DE66E5228EAF4B86C332427A474774C2056CE714BC01F3850681B0557D |
| ShanmenDemo20Widget.cpp | D3CFDB7E4220DD872A090755045F08CC129A034B12D36567EC8A5A823E8D751B |
| ShanmenDemo20World.h | BBD4B6A3D22704FB08E7F5127D622E029911915318FBA512501FA0EE0DC55B27 |
| ShanmenDemo20WorldCheckpoint.cpp | 178C94F313FC9E6AC4008BE00A0F456FD88C0D6A9383930D17A34AFCF803AFDD |
| ShanmenDemo20WorldCheckpoint.h | C98B2EDF38B0CCD99787882BB7D0CAD5192C174EFC5F0BBB7BF5FD504E678BE5 |
| ShanmenDemo20WorldRecoveryTests.cpp | 202A293BEDC9AE6CA03D0BB60D43292B061FC002A61B1B65C26A6D62A5FA85CA |

首次十三源码 manifest `EAE5181AE1BCB464B3D391BB1AB34694C12419D2A8B340EE8E7D943A5DCC4B32` 是失败前历史输入；只改 WorldCheckpoint.cpp／WorldRecoveryTests.cpp 后重算。五个背包源码 SHA 未变，不把不同候选结果混算。

## 3. 先前候选完整根及首次环境失败

这三条证据属于旧 4c00256，不证明本轮新代码；始终使用旧 Editor DLL SHA `47EF842723E05C5B725CC5076519825C49CE4D9A521E9DDE354B534548A121C4`，旧根退出后才编译新 Editor。

| 旧根 | Success / Fail | native / outer | 原始 UnrealEditor.log（Saved/FoundationRuns/ 下）及 SHA-256 |
| --- | --- | --- | --- |
| Shanmen | 1549 / 0 | 0 / 整体随后 1 | Demo20.M4.Settlement.FullRoots/Automation-Shanmen/20261005T160157647Z-4d2c6ff6/UnrealEditor.log；3D928061F9EF050AB93C91B55DBF012B722F5D2E8F52A342A8599BD428EE6487 |
| demo_map 首次 | 1329 / 1 | 0 / 1 | Demo20.M4.Settlement.FullRoots/Automation-demo_map/20261005T171630315Z-7ada3bbc/UnrealEditor.log；1EF154AB10F33FA7473681897B1FB4CBC31703AABCD1D6FC4857489E38B6994A |
| demo_map canonical 重试 | 1330 / 0 | 0 / 0 | Demo20.M4.Settlement.FullRootsCanonicalRetry/Automation-demo_map/20261005T172148778Z-86858cae/UnrealEditor.log；24359F5F39145684D0D7A283AA3B076414708B4CC271ED3E94853FFD5BF8DC63 |

唯一失败 `demo_map.AutomationRootBoundary.26.RuntimeProductionContextClassifiedByCanonicalOrigin` 依赖 task-local `Dev.D.UE.0.0.7.F0.0.r0` UserDir。首次使用 Demo20/GridAutomationUser 不满足其 origin 前置条件；canonical 重试只把 UserDir 改为 `Saved/Automation/Dev.D.UE.0.0.7.F0.0.r0/Tests/User`，DLL／测试／生产保护未改。该 case 日志 task_local=1、protected=0；不把首次 native 0／队列清空冒充通过。成功的两个旧根合计 2879／0，仍不是最终 Goal 候选全根验收。

## 4. 本轮首次原生失败与修复

首次新 Editor 构建 native 0：`Demo20.M5.QuickTransfer/Build-Editor/20261005T172437088Z-556d9b08`，UBT 41.85 秒，stdout SHA `7DF01FA2C4BCA11D28D7791E05003A14E4E74F10B91382F2B712109B0205B519`。

首次映射运行 Demo20 90 Success／1 Fail、native 0、队列 91；driver／外层均 1，剩余五组与最终 Game 未执行。原件 `Saved/FoundationRuns/Demo20.M5.QuickTransfer.FinalRegression/Automation-Shanmen.Demo20/20261005T172520026Z-5a8669b9/UnrealEditor.log`，SHA `9D1C148DBA0AEE1BF13FDBD9028F4996851A6EBD9A021B03FCBD4A09CAA75BC0`。三项 QuickTransfer 已通过，但整组失败不计交付成功。

唯一失败 `Shanmen.Demo20.Expedition.WorldRepairLegacyReadOnlyAndBoundedDamage`：WindowsPlatformFile 读取器对零字节读返回 false。仅在已成功打开、尺寸受限且调用明确允许 Minimum=0 时跳过空读，写空诊断原件仍需打开／flush／读回／原子替换；正常 checkpoint 的 24 字节下限不变。测试增加空主档普通加载拒绝、真实空原件存在、精确最新体／head、代次不增与重复修复。没有删断言或采用旧 .bak。

## 5. 最终双目标构建

TaskId `Demo20.M5.QuickTransfer.r1`，同一最终 manifest；隐藏 `Invoke-ShanmenBuild`，Development Win64、WaitMutex／NoHotReload／MaxParallelActions=2／NoUBA。

| 目标 | UTC 起止（2026-10-05） | native / helper outer | 目录（同 TaskId 下） | stdout SHA-256 |
| --- | --- | --- | --- | --- |
| Editor | 17:33:00.948 → 17:33:07.857 | 0 / 0 | Build-Editor/20261005T173300924Z-7b6fac6a | 2F07B2FB2FF14021B95FD0A29E2008F5D3DC830BF7209B5CCAEF9633A1EC379C |
| Game | 17:37:10.048 → 17:37:26.052 | 0 / 0 | Build-Game-Final/20261005T173710045Z-010b4335 | 604E354B1C49B09AD8374A559AEFA6826AD552B4DE522E65FC477B6DE1B08089 |

UBT 分别 `Result: Succeeded`、6.64／15.76 秒。最终 Editor DLL SHA `4277A8F72AF9D05F2740A52F8A1D3E7869525688CFEA31F0290735F273D498F5`。Game 不含 WITH_DEV_AUTOMATION_TESTS，不靠该编译证明原生测试。

修复前 Game 曾 native／outer 0：`Demo20.M5.QuickTransfer/Build-Game/20261005T171019879Z-15ab3b2c`，30.74 秒，stdout SHA `704FA0B7039EA25E1D96D4A03836CB75D85CB9B7B36CD36D91D1C8027DCB4B51`，stderr 空 SHA `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`；保留为历史，不代替最终输入。

## 6. 最终改动文件映射回归

现有 `Scripts/Test-Demo20Grid.ps1` 从实际十三个 Source 路径导出六组；全部 NullRHI／NoSound／Entry，隔离 GridAutomationUser。每组 native 0、Fail 0，Queue Empty 数分别等于 Success。根目录 `Saved/FoundationRuns/Demo20.M5.QuickTransfer.r1.FinalRegression/`，下表各目录内为 `UnrealEditor.log`。

| 测试组（Shanmen. 前缀） | Success | 目录 | SHA-256 |
| --- | --- | --- | --- |
| Demo20 | 91 | Automation-Shanmen.Demo20/20261005T173308556Z-8807ae52 | C78C59C88FB46BCC33AB91F0259110B2E58054A6F13AC02394A02D7BBD9D663D |
| 0_0_10.Items | 121 | Automation-Shanmen.0_0_10.Items/20261005T173409182Z-1821116f | 16FE238C843F2F22343C3A2802DE2ECE302C871AC14FACD94B18B4C629FD1755 |
| 0_0_10.CombatCore | 9 | Automation-Shanmen.0_0_10.CombatCore/20261005T173500060Z-708cad20 | 4BE5E2653991AAB11D544691B3B7206DB222977E7F26D7D83C444674F9F699E4 |
| 0_0_10.CombatRuntime.BasicSword | 4 | Automation-Shanmen.0_0_10.CombatRuntime.BasicSword/20261005T173535489Z-df2f98ee | 4A978305B9C756D2E9B3BCB3342EEDA358F15E017C840F24CDAE5EA238691EB2 |
| 0_0_10.CombatRuntime.VitalityAuthority | 6 | Automation-Shanmen.0_0_10.CombatRuntime.VitalityAuthority/20261005T173606928Z-b1098919 | 038225EFAF217AAE86C5BFF0F4D1E5FEF379BC06AFFC569BF086710B5BC6BA9B |
| 0_0_10.CombatRuntime.VitalityLedger | 4 | Automation-Shanmen.0_0_10.CombatRuntime.VitalityLedger/20261005T173638278Z-351079b6 | 6F9304B6F1974370A34C020911D2924A0A6BA7086BD1C4EE73959DB12CBCA48C |

总 235 Success／0 Fail。driver native 0、helper outer 0；`PASS Changed=13 Rules=1 Required=6 Logs=6`。driver stdout `Saved/FoundationRuns/Demo20.M5.QuickTransfer.r1/Regression-Driver/20261005T173308000Z-45efda56/stdout.log` SHA `3A10D4DA46F82B26EBDA58A3C7ABA6C1F14881EDF0C8CB4FF28782C4A43BCE42`。不把旧根 2879 或失败候选的 90 混入本数。

## 7. 新测试证明的范围

三项 `Shanmen.Demo20.Loot.QuickTransfer*`：实际持久 Run 部分合并 8+3→10+1、单笔幂等重送、第二次移动与原生重启、真实取出统计、满格仍合并、横向唯一空位旋转、外国 Run 拒绝、AtomicReplace 失败原图不变与精确重试。

五项 `Shanmen.Demo20.Expedition.WorldRepair*`、一项新增 Medicine 组合及已有终局损坏组合成功，具体协议边界见 [原局修复 Report](../Report/Demo20.M4.WorldRepair.r0_report.md)。这些是无头原生持久契约证明，不是玩家画面／键鼠／帧耗时证明。

## 8. 辅助失败与原件

旧八路径辅助 session 65972 exit 1：新增五源码后冻结路径门拒绝，非产品失败。十三路径辅助首轮 chunk 12fa75 exit 1：未生成 run-state 的运行中旧根被误当已存在；补缺失即等待，源码未改。session 92892 exit 1：旧 demo_map 失败；session 32556 exit 1：新 Demo20 空原件失败。均保留，不反复轮询已结束 handle。

修复前 WorldRepair 辅助首个 parser exit 1（条件换行误解析 -or），仅修辅助后 parser 0；原工具 e40e99 与从工具字段重建的 `Saved/Automation/Demo20/WorldRepairVerifierParserFirstFailure.log` 保留，后者不冒充 UE 原始日志。最终辅助 `Verify-QuickTransfer-r1.ps1` session 10067 exit 0，前后核验 HEAD／十三路径／源码 manifest／保护；不写 Git 或操作窗口。

## 9. 文件保护与提交

113 项保护前后通过：四份 tracked 用户文档、历史 untracked 与实际 SaveGames 不变。`git diff --check` 通过，仅 LF／CRLF 提示。精确提交本阶段十三源码、两对 Report／Log、两页人工说明与 README／ExpeditionStatus，共 21 路径；不使用 git add .。GitHub 远程身份在阶段交付时只读核验，不把未提交用户文档纳入。

## 10. 真实验收及剩余 Goal

未启动 Editor UI／PIE／Standalone、接管窗口或截图；当前“丹药正常使用”只证明旧普通主路径。双击、提示、拖拽捕获、720／1080、原局按钮与组合实际续局依 [人工背包检查](../Demo20/ManualQuickTransferReview.md) 和 [人工恢复检查](../Demo20/ManualWorldRecoveryReview.md) 待验收，不能要求破坏真实档造条件。

临时 Repository 校验可能复制历史与图，世界 flush 亦有同步成本；无真实时间／进程内存／对象释放数据，不能以小文件或测试快宣称无卡顿。三只守卫尚非 10–15 分钟内容；完整撤离／死亡／次局搜集、最终候选全部新旧根及真实性能仍未闭合。Goal 保持 active；codex-10 保持暂停。后续独立产品增量继续，不扩展有待实际验证的修复路径。
