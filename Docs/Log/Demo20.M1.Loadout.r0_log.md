# 山门 Demo2.0 M1 携带与原子出发 Development Log

日期：2026-10-04。基础提交：`dbd4d1554092d7dfc9013b8a9a9fd305ab6a869d`。分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。本轮为 M1 携带增量；玩家正式探索入口尚未接通，完整 Goal 保持活动。结论与范围见 [Report](../Report/Demo20.M1.Loadout.r0_report.md)。

## 1. 开工和实现

读取完整用户 Goal、最新 Resupply Report／Log、分支／Git 状态、Demo20 正常入口、网格与原生持久服务、来源接纳及 Run 端口。113 项保护清单核验不变；四份用户 tracked 文档与 104 项原有 untracked 历史材料不提交。旧自动任务保持暂停，不新建或恢复自动任务。

新增 LoadoutStart 冻结批次，经既有 Subsystem／AuthorityService／Repository 在一个候选中 Reserve 多行并 StartPreparedRun，最后一次持久保存。复用 Reserve／Start 回执及 schema 6，无新库存、种子文件或事务枚举。精确重试核对每行指纹；Owner、内容、版本、数量、用途和预留冲突失败关闭。

产品 Build 只选择已装备普通装备与 Carry；安全装备／内容、Stash、Wallet 不进入普通清单。InspectActive 从已有回执重建 RunSeed、未消耗余额并拒绝未完成 Intent。EditGrid 增加活跃 Run 的准备锁，防 Stored 安全物品绕过。整备摘要在刷新时缓存；新增九项隔离测试与携带边界、状态索引和运行说明。

## 2. 失败原件和修复

首次回归原件前缀 `Saved/FoundationRuns/Demo20.M1.Loadout.FirstRegression/Automation-Shanmen.Demo20/20261004T193045994Z-aee69527/`。4 Success 后 TArray 自引用断言崩溃，UE 原生 3、验证外层 1，无已完成测试队列。测试 `Lines.Add(Lines[0])` 改为先复制临时行。`UnrealEditor.log` SHA-256 `DBC4ADFD2EEC622FD0C5C4004FC5D5FF45FCBAF764A5CD35573E0AD6B39BB633`；run-state.json `F02F5CA03D77D993E354B1517B06A0EF6204BB23AE31CF4A19D3AA18FFA690F1`。stdout／stderr 与崩溃输出保留。

第二次前缀 `Saved/FoundationRuns/Demo20.M1.Loadout.FinalRegression/Automation-Shanmen.Demo20/20261004T193800983Z-eb1e2001/`。21 Success／3 Fail，Queue Empty 24，UE 原生 0、验证外层 1；其他映射组因 fail-fast 未运行。`UnrealEditor.log` SHA-256 `D891AB1BE21DA8533F904B6FCFF46AFFC2411CE79B274D23DC91E9965600ECBD`；run-state.json `F9DEFD7D3EAE159A2DA711D8BB11F380730B0739DF040EE4446F03938FEC1C4F`。

三项构造问题分别修正：CAS 用例目标 (8,4) 与 2×2 残卷重叠，换 (10,4) 真空位；Amount 篡改改为合法非零 Quantity，另断言 DeploymentLock=2 为 InvalidRequest；Subsystem 使用 GI Init／GetSubsystem／Shutdown，而非未初始化 NewObject。保留状态不变、拒绝类别和正常重试断言，未放宽生产规则。run-state 的 SUCCEEDED 仅代表原生退出 0，不证明测试通过。

第一次实际 UI 在 720p 发现摘要长句与装备栏标题重叠。截图 `Saved/Automation/Demo20.M1.Loadout/UI/overlap-first-1280x720.png` SHA-256 `F97804327C116B919B9AB8E4A6DB24585A90D6F6338EFF98D7364D93F3CCC850`。修为右侧背包下方两行摘要，重新 attempt-004 构建、双分辨率及最终映射回归。没有用无头通过替代界面证据。

映射自测初次包装误把 PowerShell 脚本的空 LASTEXITCODE 当失败，工具外层 1，实际 557 项均 PASS；保留 `Saved/Automation/Demo20.M1.Loadout/RegressionMappingSelfTest.log`。改用 `$?` 判断后外层 0，最终原件 `RegressionMappingSelfTest.final.log`，557/557，SHA-256 `D7FD012D6300D759C1E67EC3096F14C5C6F467897CF4533C59EBA9DC84AE1F7D`。只读检查中的不存在路径／rg 路径错误不是源码构建失败，也没有计为验证成功。

## 3. 双目标构建

执行 `Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M1.Loadout -AttemptId attempt-001/002/003/004`。各次 Editor／Game 原生 0、构建包装外层 0。001 期间源码继续完善，002／003 尚未包括最终 UI 修正，因此仅 004 为最终候选。

路径相对于工程根 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`。下表前缀 `Saved/Automation/Demo20.M1.Loadout/Build/`；SHA 对应 stdout.log，同目录 stderr 与 result.json 保留。

| 尝试／目标 | 原生／秒 | stdout 路径 | SHA-256 |
| --- | --- | --- | --- |
| 001 Editor | 0／355.486 | `attempt-001/demo_mapEditor-Win64-Development.stdout.log` | `373CF176C47D75B30868929DB74AE8B4C9DAAA39877CF98D04BCC07E9F947080` |
| 001 Game | 0／361.680 | `attempt-001/demo_map-Win64-Development.stdout.log` | `06B9BE38FC4E8ED7D5732898381E77CE14E35284DA44D89C2EF7B469FF4F5849` |
| 002 Editor | 0／8.911 | `attempt-002/demo_mapEditor-Win64-Development.stdout.log` | `CB34C3F549E46B5830BB119B5049B256AD001AB400259B0C38D1219B01250CDF` |
| 002 Game | 0／13.054 | `attempt-002/demo_map-Win64-Development.stdout.log` | `18526B3D2383680079E35F3CDA8CB33D838EDF2C2F0C8D10C6D70FD16924E34E` |
| 003 Editor | 0／6.497 | `attempt-003/demo_mapEditor-Win64-Development.stdout.log` | `5BBBCCBFE79A8F042EE7B4CFCDCC39A248F2798AE8D290AE26EC6B581DA08D5C` |
| 003 Game | 0／12.338 | `attempt-003/demo_map-Win64-Development.stdout.log` | `A340F34F57900579CA7A847271A2B296E10316D6CCDBF1262EC6F3DFE62DF180` |
| 004 最终 Editor | 0／8.030 | `attempt-004/demo_mapEditor-Win64-Development.stdout.log` | `02BC370944BDB326F58C7B231519354E2C7272BF3F3FD68FBDAFA6830D00E3B7` |
| 004 最终 Game | 0／13.328 | `attempt-004/demo_map-Win64-Development.stdout.log` | `4F8089F1159369A1418407420B6A7A33AC7049694F1B2C36DDDB35DB603D6D18` |

004 Editor：19:53:07.743Z → 19:53:15.773Z；Game：19:53:15.790Z → 19:53:29.118Z。最终 result.json SHA-256：Editor `23C7A0F8009B67974F79A91299135E352798A71975560E4D9823DA54AF1CA22E`，Game `E27790D707EA9609CDCCFA0ABAE6EF43D086F30B3953E87A48A7C37BC777F4BF`，stderr 均空。不把构建耗时当游戏性能。

## 4. 改动驱动回归

最终命令 `Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M1.Loadout.FinalRegressionR2`，代码包括真实 UI 重叠修正。218 Success／0 Fail，八组原生 0、验证外层 0，各组 Test Queue Empty 数量和结果一致。覆盖门 `PASS Changed=13 Rules=3 Required=8 Logs=8`。逐一复核原件、run-state.json、结束记录与 SHA-256，不以启动器 SUCCEEDED 代替断言通过。

路径前缀 `Saved/FoundationRuns/Demo20.M1.Loadout.FinalRegressionR2/`，下表为 UnrealEditor.log；同目录保存 stdout／stderr 与 run-state.json。

| 最终组／Success-Fail／原生 | 日志路径 | SHA-256 |
| --- | --- | --- |
| Demo20／24-0／0 | `Automation-Shanmen.Demo20/20261004T195733205Z-fc8b2c45/UnrealEditor.log` | `540A7840ECD816ECD8AEAEF92ED500F20F1C84C13F3C8009BCE8FBFEC2950171` |
| Items／119-0／0 | `Automation-Shanmen.0_0_10.Items/20261004T195809793Z-40888355/UnrealEditor.log` | `AE89F7C70CF3B6F56D914A43FDAB7E3913478048B61F8F7C57A5EC266E0FD05F` |
| ItemUseAndArmor／46-0／0 | `Automation-demo_map.ItemUseAndArmor/20261004T195905302Z-52f3eee1/UnrealEditor.log` | `B37A5421E6B67C77F2C72A3AE0F86A5719F5B990FDA2CB28ACD69913F622DF63` |
| Hotbar／7-0／0 | `Automation-demo_map.P4.Hotbar/20261004T195940729Z-3b455d01/UnrealEditor.log` | `CB7F04A0A7E014827C833C1D6572C59908C3302552F7FBB897DAC11867246692` |
| CombatCore／9-0／0 | `Automation-Shanmen.0_0_10.CombatCore/20261004T200016188Z-41f42a73/UnrealEditor.log` | `2A907532F7CD882A03CEEEA6E8D1CAB97C1870E6963AB67AA5947FAC480F67D6` |
| BasicSword／4-0／0 | `Automation-Shanmen.0_0_10.CombatRuntime.BasicSword/20261004T200048508Z-ecf06668/UnrealEditor.log` | `A43D9F2D4C0FD43EDB41CF54E9F6934F591DA5265C327150F8771A2479F3B0F9` |
| VitalityAuthority／5-0／0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityAuthority/20261004T200119929Z-ccebb50a/UnrealEditor.log` | `537AEA41EA39D9DC8DDA894CCA587AAB28B40C773080D4B090C00D2FF74F6456` |
| VitalityLedger／4-0／0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityLedger/20261004T200151308Z-3008ab52/UnrealEditor.log` | `A2FEE0BF24027D1BC7213CAFBBA4A6EF14E1E570E0A18B5C3C90A0247B7A5A04` |

此前 FinalRegressionR1 在 UI 修正前为 218 Success／0 Fail、八组原生 0、外层 0，覆盖门 `PASS Changed=13 Rules=3 Required=8 Logs=8`，原件保留在 `Saved/FoundationRuns/Demo20.M1.Loadout.FinalRegressionR1/`。不能替代最终 UI 候选。

新增九项测试：LoadoutSelection、AtomicLoadoutStart、LoadoutSaveRollback、LoadoutCandidateReject、LoadoutReplayAndTamper、LoadoutBalancesAndSeedRecovery、LoadoutDeathSecureAndStash、LoadoutThroughExistingSubsystem、LoadoutPendingIntentFailsClosed。使用真实 Reserve／Start／Consume／Intent／Finalize 和原生存盘，没有伪造成功回执。仍须 M5 完整新旧测试根与真实玩家验收。

## 5. Standalone 界面和隔离档

正常命令 `Scripts/Start-Demo20.ps1 -Action Play -ProfileName LoadoutAcceptance_20261004 -Width 1280 -Height 720`；修正后同参数重启，随后同档 Width 1920／Height 1080。没有运行控制台命令或手工改档。使用 Computer Use 观察后逐次实际操作，截图未合成、裁切或修改。

下表日志前缀 `Saved/FoundationRuns/Demo20.S01/Play/`，每目录同时有启动 run-state.json。

| 目录后缀 | 观测 | UnrealEditor.log SHA-256 |
| --- | --- | --- |
| `20261004T195114680Z-d681f71d` | 首次 720p，Tab 打开，观察摘要重叠并保存失败截图；无库存操作 | `E0BE7AC9E8B9942244F19A6AB4FAA55F1057AF1D50412A6CE184CDB34E3220BC` |
| `20261004T195358562Z-9cdd13dd` | 修正 720p，摘要不重叠；卸下护具明确缺装，重装恢复；Tab 返回与重开正常 | `1BCD2CC103613C371F3F31597FBBEEF419D4A301F27F82702319DAD2C792A394` |
| `20261004T195609404Z-5eb53c06` | 1080p 同档重启，装备／摘要保持，Esc 返回正常 | `9116BE25D97D4F6C9FDC5AB05B53D0BE173E65247700174D0C0069342D74DB8A` |

三次 Alt+F4 正常关闭，均有 LogExit: Exiting。启动器非阻塞，run-state.json 仍 STARTED、最终 exit_code 为 null，不宣称 UI 原生码 0。视口分别 Pixels=1280x720 WidgetScale=0.666／Pixels=1920x1080 WidgetScale=1.000；原生桌面截图分别 1922×1128／2882×1670。

物品档 `Saved/Demo20/User/Saved/Demo20/LoadoutAcceptance_20261004/ShanmenItems/Authority/165C5C9C689E2502A733AFF4489C62CC.json`。初始第 1 代、revision 0、测试灵石 1,000,000，SHA-256 `B361DC4A0E41EE92AA35E2B3BC61CD9381D6DD464811C4D50341BB731F553A1B`。

真实卸下／重装各保存一次，随后第 3 代、revision 2；道袍原 ItemId `6D7A93C24AE67965075B06304B8AE438` 回到 Armor 槽、quantity 1、item revision 2。720p 面板重开后、1080p 同档重启并返回后 SHA-256 均为 `76C5EA889705618D03A6396D3223CDA131F5587001C9509A8E413F27E4D5FDAD`，仍第 3 代、测试灵石 1,000,000，没有重新发物或额外代数。

最终截图目录 Docs/Demo20/Evidence/M1.Loadout：

| 原生截图 | SHA-256 |
| --- | --- |
| [ready-1280x720.png](../Demo20/Evidence/M1.Loadout/ready-1280x720.png) | `5288E98D4DF9DEADB2F830324561017EFBE5B6DD3E0551C2A268BEF6212F884E` |
| [missing-armor-1280x720.png](../Demo20/Evidence/M1.Loadout/missing-armor-1280x720.png) | `3476E00D3CA4465BFCBC06462CE3160C50DCB3E0BA3E0EFCFFB4FA863A7F9B58` |
| [ready-restart-1920x1080.png](../Demo20/Evidence/M1.Loadout/ready-restart-1920x1080.png) | `0EF7EA6CBDEBC2166E6D320AF7F3BBF5111A501488B99DA6BA87A2194060347A` |

## 6. 边界和交付检查

新增 Items Loadout header／cpp 未扫描到 demo_map、UWorld、AActor、ApplyDamage、FRandomStream 或 FMath::Rand；ShanmenItems.Build.cs 仍无 Engine。复用 schema 6 与既有回执，UI 不扣库存、不计算伤害。准备锁不会代替未来 Run 专用二维操作端口。

当前增量不修改地图／资产／旧库存，不 Cook／Package；没有 PIE、正式探索或性能验收。未确认的世界生命、位置、敌人、搜索及来源状态仍须后续组合，不能称为完整恢复。仅缓存预览不能消除候选复制和历史扫描成本。

精确交付名单为 13 个 Source 文件、携带边界／README／阶段状态、Report／Log 和三张截图。提交前核验 113 项保护、diff --check、链接、哈希、最终日志和暂存名单；Saved 原始证据留本地，不用 git add -A。完整 Goal 不标完成，旧自动任务仍暂停。

下一增量：新的三分区探索地图、正式玩家出发／返回和可继续的世界持久状态；之后治疗、稳定来源随机搜集、Run 二维转移、安全格使用、终局与完整 M5 验收。
