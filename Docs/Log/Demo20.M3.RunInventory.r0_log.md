# 山门 Demo2.0 原携带物统一背包 Development Log

日期 2026-10-05；进入基线 `9956f391d99099b6420ea081d6e40abe598f8857`，分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。范围见 [Report](../Report/Demo20.M3.RunInventory.r0_report.md)。本轮是原携带物整理增量，完整探索 Goal 仍活动，旧自动任务不恢复。

## 1. 开始检查与权限

读取完整 Goal、当前 Git、最新 Loot Report／Log、状态与保护、Run 身份／出发、物品权威与格子、治疗／世界恢复、UI。普通丹药“正常使用”是已核对的旧基线人工主路径，不扩大为新版整理后的治疗或所有安全格恢复通过。

实时操作交人工，本轮没有启动 Editor UI、PIE、Standalone、产品 exe，没有代理输入或新增截图。编译与 UnrealEditor-Cmd 只用于代码验证；不改真实存档。四份 tracked 用户修改、104 项历史 untracked 及原有存档保持，113 项保护哈希核验通过；提交前再次核对。

## 2. 本阶段实现与输入

十五个 Source：ShanmenItems 的 AuthorityService h／cpp、Repository h／cpp、Types h／cpp、Grid h、RunGrid cpp；Demo20 的 Expedition cpp、InventoryWidget cpp、Loadout cpp、LootTests cpp、World h；既有 ItemAuthoritySubsystem h／cpp。五份文档为 README、ExpeditionStatus、ManualRunInventoryReview、Report、本 Log，共二十文件。无 Config、Content、地图、脚本、旧 writer、自动任务或正式玩家档修改。

新增单向 MaterializeRunInventory，不接收 caller 数量，按原账本未消费余额恢复原 ID；零余额不补回，背包未装备武器护具转为 Stored。接管回执按 Owner／Scope／Run 绑定，重复不重做。载入验证接管身份、历史总数、预留集合、Run 时序和旧 pending intent 关闭；全额预留普通数量不允许无接管回执再变可用。

新出发、原局恢复、原检查点重试先恢复治疗再接管。无法读权威／治疗状态也暂停明确提示，未确认不继续探索。旧数量消费端口对接管局拒绝新命令，已确认请求可精确重放。整理、Stored 丹药使用与终局沿用原格子、Reserve／Commit、世界意图和唯一持久权威。

终局只关闭已接管原预留，不从旧账本重复返还；死亡依据实际普通／安全归属。调用方提交接管物资的返还量拒绝。Schema 6 与世界记录字段不变，operation 尾部追加，治疗继续用前一轮 Origin 2；旧 DLL 不支持新增操作，先退出旧进程再切版本。

## 3. 首次构建停滞与双目标构建

命令 `./Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M3.RunInventory -AttemptId attempt-NNN`，原件位于 `Saved/Automation/Demo20.M3.RunInventory/Build`；每个目标保留 stdout、stderr 与 result.json。Editor 成功才进入 Game，编译不代表实际游戏通过。

001 于 12:31:56.011Z 启动 Editor，743.267 秒没有进入 UBT 编译。stdout 只有 bundled .NET 和启动命令，stderr 为空，UBT Log.txt 仍是前一阶段 12:05 的内容。只读进程核对 cmd 21780 → dotnet 44908 → 子进程 15668，主进程 CPU 0.15625 秒、线程等待，无新的 Application 错误事件；不能据此断言源码错误或 .NET 崩溃。

获批环境运行已确认真实路径的 bundled dotnet `--info` 0.526 秒返回 0，版本 SDK 10.0.203／Host 10.0.7。首次探测误用了不存在的 10.0.100 路径，PowerShell 返回 1；随后按 rg 发现的实际 10.0 路径纠正，没有安装运行时或修改系统。该探测错误不算编译错误。

12:44 核对 PID／Parent／完整构建命令后，只结束本轮 dotnet 44908，未结束游戏或其他应用。Build.bat 观察码 -1，外层 1，12:44:19.278Z 完成；这是代理终止的环境启动停滞，不是编译器自然失败码。Game 未执行，001 不计成功。原件保留，不把观察等待当作成功或原生超时码。

002 在获批执行环境顺利进入 UBT；Editor 原生 0、487.412 秒，12:53:53.167Z 完成，Game 原生 0、472.255 秒，13:01:45.444Z 完成。其启动后补上 GameMode preflight 失败关闭分支，因此 002 是中间候选，不代替固定最终源码双构建。

003 对固定最终源码成功：Editor 原生／外层 0，10.765 秒，13:02:07.184Z 完成；Game 原生／外层 0，1.221 秒，13:02:08.427Z 完成，Game 已包含该最终输入，UBT 确认 Up to date。十五个 Source 哈希在最终构建后核对不变。全部 stderr 为空，SHA 在进程退出后重算；构建输出中列出 UnrealEditor.exe 是 UBT 产物路径，不表示启动 Editor 窗口。

| 尝试与目标 | 观察码 | 外层 | 秒 | stdout SHA256 |
| --- | --- | --- | --- | --- |
| 001 Editor 代理终止 | -1 | 1 | 743.267 | 241D79AE6954131BE4FA887E6CF8C3302CEE8F7255F736F355F5718BE7D90883 |
| 002 Editor 中间输入 | 0 | 0 | 487.412 | ED473C7325C7C4D518872B475EAAE43B27241A7E50FEAA869B6BE8BABCD69388 |
| 002 Game 中间验证 | 0 | 0 | 472.255 | 396CD008BD7B022A4F73769F44BA81B5E596EFE7B98CE719D9BF5622BA3D8536 |
| 003 Editor 最终输入 | 0 | 0 | 10.765 | C731B590B379FD398027E7F47146BBBF68526F1F981CD05935C55996AEBF75A3 |
| 003 Game 最终输入 | 0 | 0 | 1.221 | 7974EF0C242EB89B879A2A07BCEAC029454D98611684A23C8A169606D50C97C2 |

001 result.json SHA256 为 `E4A88A9DB317D274B50ED64DE34DE256E6114438707AAF4DF3F2AD0366C7D2C8`；空 stderr SHA256 为 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。没有覆盖或删除首次停滞原件。

## 4. 改动驱动回归

十五个 Source 触发 Demo20StandaloneSlice、Items、ItemProductAdapters 三条映射，合并八组：Shanmen.Demo20、Shanmen.0_0_10.Items、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar、CombatCore、BasicSword、VitalityAuthority、VitalityLedger。不因“主题是背包”而漏跑旧物品／快捷栏，未修改 RegressionMap 或测试筛选脚本。

最终双目标完成后执行 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M3.RunInventory.FinalRegression`，八组原生码及外层均 0，257 Success／0 Fail、257 个去重路径。每组 Queue Empty 与 Success 一致，run-state 有完成时间；门禁 `REGRESSION_COVERAGE: PASS Changed=15 Rules=3 Required=8 Logs=8`。最后一组 13:08:23.730Z 完成，退出后重算八份 SHA 并核对最终十五个源码哈希不变。不用前一轮 249／0 替代本轮结果，不把进程 SUCCEEDED 等同于全部断言通过，也不声称已完成 M5 全部新旧根最终回归。

新增八项 RunInventory 全部通过，既有项目根未出现 Result Fail，没有为本轮目标失败删改断言或降低预期。原件根 `Saved/FoundationRuns/Demo20.M3.RunInventory.FinalRegression`；每行日志路径为 `<根>/Automation-<测试组>/<目录>/UnrealEditor.log`，同目录保存已完成 run-state.json。

| 测试组 | Success／Fail | 原生码／Queue Empty | 目录 | SHA256 |
| --- | --- | --- | --- | --- |
| Shanmen.Demo20 | 62／0 | 0／62 | 20261005T130314304Z-9d963dcf | 6899A3483E67A37059BE2373ADBD2C4867922DABE6D87A7C50A5F7A7C79AF25F |
| Shanmen.0_0_10.Items | 119／0 | 0／119 | 20261005T130415048Z-cd98624a | EA6DF82FBF3194AEFE174C6E8636BF53667E879E02BB2603D9F9DB0D6C8E7AF6 |
| demo_map.ItemUseAndArmor | 46／0 | 0／46 | 20261005T130506242Z-3af78099 | 81605087F92297747EFB00ABC922736CFD40C50A9CD8721AFC4408B072B2046F |
| demo_map.P4.Hotbar | 7／0 | 0／7 | 20261005T130541971Z-be2f9edc | BF0E09577DDD39317B3830610621267CCC8D6F9D2A219B696FEE0844F77C7685 |
| Shanmen.0_0_10.CombatCore | 9／0 | 0／9 | 20261005T130617409Z-acfb1a66 | 975B5ACF439D89D44FB3FF62016A1A4D70700FF079F48B0470886F8676D8DF53 |
| Shanmen.0_0_10.CombatRuntime.BasicSword | 4／0 | 0／4 | 20261005T130649009Z-98f004c0 | 35B3A41F00F93813D6D9AD781D8BF018CA40B848DDFA86979AAC1F99ABF7C5C6 |
| Shanmen.0_0_10.CombatRuntime.VitalityAuthority | 6／0 | 0／6 | 20261005T130720640Z-253d2d8d | 1A94FE64BD6DDBFE548E1F113208092CBC51197EF313080E5DC7394954657FEA |
| Shanmen.0_0_10.CombatRuntime.VitalityLedger | 4／0 | 0／4 | 20261005T130752152Z-91114bf5 | 9F7D5EAEE9DF94A4BB2ED268CC0039AD0CCE7A11055A828C8D1F8C9A58A190A8 |

Demo20 的目标结果 62／0，其中八项 RunInventory 全部 Success，Queue Empty 62、原生 0。启动之前另有十三条 `LogAutomationTest: Error: Condition failed`，在上一 Loot 最终日志也为十三条，均位于目标 RunTests 之前，旁边是 `UE::UnifiedErrorTest` 输出。读取本机 UE Core `Tests/Experimental/UnifiedError/UnifiedErrorTests.cpp`，其中有英文 ToString／格式断言，而当前输出已本地化为中文；本地化导致启动自测断言不符是推断，未逐条诊断，不把这些文本伪称不存在或故意失败。保留原件并列为既有 Engine 启动信号，不计入本轮项目根的 Result Fail，也不修改 Engine、语言设置或回归门禁来隐藏。

## 5. 新用例覆盖与人工边界

八项用例使用隔离 native 文件，不是仅值类型内存假成功：原八颗先确认用三颗再接管五颗；重复与原生重开；实际移动、拆三颗安全格后合一颗回普通、六／二非零基准；拒绝 caller 再返还、撤离原位置不补回与下一局；死亡保留两颗安全、移出一颗损失与仓库／钱包／安全装备逐项不变；pending 旧用药先终结；AtomicReplace 回滚与 ReadBackCommittedPrimary 精确重读；移动过的原药世界保存失败后只扣一颗、只治一次；外部 Owner／Scope／Run 与回执删除／历史量／fingerprint 篡改；零余额和未装备重剑旋转／带回。

既有真实 GameInstance 用例增加未绑定拒绝和正式 Subsystem 接管。既有 Loot 老只读路径测试保留历史 fallback，不改变所有未接管领域用例预期；普通装备部署和整备更换规则不放宽。

本轮没有实际 UI、战斗、失焦、720／1080、PIE／Standalone、帧耗时、运行内存或多轮对象释放。人工步骤见 [ManualRunInventoryReview](../Demo20/ManualRunInventoryReview.md)，按新隔离档检查原物整理、数量守恒、治疗、重启和终局；危险处背包不暂停敌人。待人工不暂停独立开发。

## 6. 未完成范围和成本

跨来源可追溯合并、可见地面主动丢弃／拾回、随机敌人组合、目标内容量、结算明细、损坏记录修复和完整实际循环／性能仍未完成；来源退回不是地面丢弃。不把本轮整理接线当作完整 Goal 收尾，也不自动恢复旧自动任务。

代码没有新增每 Tick 接管或数量缓存；一次接管保持实例总数不变，仅增加回执。历史回执／tombstone 增长、UI 快照复制与多局释放仍是实测事项。世界文件大小不变只是格式兼容信息，不是运行内存证据。

## 7. 收尾核对

最终十五个源码哈希、双构建、全部适用自动化计数／退出／Queue／SHA 已独立复核；57 个文档相对链接、15 个日志证据 SHA、git diff --check 与 113 项用户材料／存档保护检查均通过。Items Build.cs 保持 Core／CoreUObject／GameplayTags／ShanmenCore、Json／JsonUtilities／OpenSSL，无 Engine 或 demo_map 依赖；RunGrid 实现扫描无 demo_map／UWorld／AActor／ApplyDamage／FRandomStream／FMath::Rand。

只精确暂存二十个本阶段文件，不包含用户文档、历史未跟踪材料、存档、构建和自动化原件。提交推送与远程 SHA 一致后交付 GitHub 链接，完整 Goal 保持活动。
