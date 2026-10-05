# 山门 Demo2.0 随机来源和搜索预览 Development Log

日期 2026-10-05；进入基线 `3a36ec5b2e19c9de517e0f77798d6935237b94bf`，分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。结论和边界见 [Report](../Report/Demo20.M3.Sources.r0_report.md)。本轮是 M3 的来源／搜索增量，不是完整领取或最终游玩验收。

## 1. 开始检查与用户规则

读取完整 pasted Goal、最新 ManualReview Report／Log、当前 Git／保护、进程、Catalog／Loadout／Medicine／世界记录、来源服务和 Repository、旧来源 Adapter／RewardGenerator。进入时没有 UE／UE-Cmd／dotnet，四份受保护用户 tracked 文档修改保留，113 项保护验证通过。普通丹药人工成功已有生命和扣一颗回执，不重新调参，也不重做该验收。

实时操作交人工，独立开发不停；问题功能不叠加，不因待画面而降低权威或验收门槛。本轮不操作游戏窗口，不启动 Editor UI／PIE／Standalone，旧自动化保持暂停，完整 Goal 保持活动。

## 2. 已编码输入

14 个 Source：Catalog h／cpp、Expedition cpp、Widget cpp、World h／cpp、既有 ItemAuthoritySubsystem h／cpp、旧 RewardGenerator cpp；新增 Sources h／cpp、SourceTests cpp、SourceWorld cpp 和共享 DeterministicRewardRandom h。另改一个 ShanmenRegressionMap，交付 README、状态、人工步骤、Report 和本 Log 五份文档，共 20 文件；无资产或真实存档变化。

唯一物品服务包装检查 Ready／GameThread／Owner。六角色注册，Position／Loot 各自隔离随机流，已有结果不生成或写入；第一次搜索约一秒完成才持久接纳。共享旧 xorshift64* 原样提取，零种子与退化范围行为保持。Catalog 只抽出原定义／stamp 读取，13 项初始配置不变。

UI 用既有卡片显示中文名称和数量，明确只读不能领取，世界仍运行，玩家动作停止；输入表面用原清理方法。生成分值不进入钱包，来源计划不插入 Item／Container／Grid，也不增加普通或安全格药数。六标记复用，宝匣位置只在 Run 改变时计算。

## 3. 双目标编译

命令为 `./Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M3.Sources -AttemptId attempt-NNN`，使用 Hidden 构建进程和既有 UE 5.8／bundled dotnet，Editor 成功才运行 Game。原始 stdout、stderr、result.json 均独立保留在 `Saved/Automation/Demo20.M3.Sources/Build`。

| 尝试与目标 | 原生码 | 外层 | 秒 | stdout SHA256 |
| --- | --- | --- | --- | --- |
| 001 Editor | 6 | 1 | 154.672 | 2BBF98ED02E4831EB47DD13BFAE325C8ECAA61ED771CC9FB52D091C763F21B5E |
| 002 Editor | 0 | 0 | 21.498 | 5602A45F0C804A7C576F0213D8B3EEF91CDE3E14B3D04E9148FD0BD5C681465D |
| 002 Game | 0 | 0 | 155.152 | 4AA12FA812E44FD297EE5F70295B400FD156C97769B06656E7B38DAE68D5A855 |
| 003 Editor | 0 | 0 | 13.963 | 126129E965470A108BE120FA1121746455ABD8F88A00A9035713BF7547CDA3A8 |
| 003 Game | 0 | 0 | 15.472 | ECC04DFDC08766C424D9FF0CDBBFFBDE79F2308E66B022BEF328C5E860E98E1B |

001 编译发现 `const auto*` 无法从 Pawn 的 TObjectPtr 条件表达式推导，显式改 `const APawn*`；同时避免局部 SourceRole 遮蔽 AActor.Role。001 Game 未执行，不能计通过。002 双目标成功但随后的新测试有错误预期；修正测试／增加未解决分歧检查后，003 对最终源码双目标成功。最终回归开始后生产源码不再改变。

## 4. 首次失败自动化

命令 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M3.Sources.Regression001`。首个 Demo20 根 42 次执行，41 Success／1 Fail；结束记录为 Queue Empty 42，UE 原生 0，外层脚本因失败记录返回 1。run-state 的 SUCCEEDED 仅表示原生退出码，不是测试全部成功。

日志 `Saved/FoundationRuns/Demo20.M3.Sources.Regression001/Automation-Shanmen.Demo20/20261005T103920046Z-ccf2d50f/UnrealEditor.log`；SHA256 `5390D6D6FC6FFDDB8C00334C37FBB25D7AEFD58C5EA657FB91CDDE9162912A8E`。失败用例 SaveFailureAndUncertainRecovery 误认 ReadBackCommittedPrimary 必须返回未知；既有 Service 的 ReconcileFailedSaveLocked 会重开并验证精确后态，正确结果是 ResolvedAfterReopen。

改为分别证明替换前精确回滚、替换后精确重读确认；另建两个真实生命周期造成旧写入者分歧，证明不覆盖新原件、读取 Unavailable、不发布来源预览。没有删断言来迁就成功，没有修改服务的持久语义。首失败日志／原档保持。

## 5. 最终改动驱动回归

命令 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M3.Sources.FinalRegression`。14 个生产路径命中 SharedDeterministicRewardStream、Demo20StandaloneSlice、ItemProductAdapters 三条规则，共 11 组。共享随机流规则补旧生成器此前无路径映射的缺口；Demo20 根已含九项 Sources，不另跑子组制造重复计数。

最终结果：398 Success／0 Fail、398 个去重路径，11 组 native 0，每组 Queue Empty 数量与完成结果一致、run-state 有结束时间，外层退出码 0。覆盖门禁 `REGRESSION_COVERAGE: PASS Changed=14 Rules=3 Required=11 Logs=11`。不是 M5 最终全部新旧根；不使用早一轮或 Medicine 日志替代最终输入。

日志根为 `Saved/FoundationRuns/Demo20.M3.Sources.FinalRegression`。每行原件为 `<根>/Automation-<测试组>/<目录>/UnrealEditor.log`，同目录保存 run-state.json。SHA256 在进程退出后重新读取核对。

| 测试组 | Success／Fail | 原生码／Queue Empty | 目录 | SHA256 |
| --- | --- | --- | --- | --- |
| Shanmen.Demo20 | 43／0 | 0／43 | 20261005T104409133Z-21f3a4de | 6982F8EE8747C97E348AC0795C12360416D5EFB10B6F6F82D6E00A9AB1155A1E |
| Shanmen.0_0_10.Items | 119／0 | 0／119 | 20261005T104459734Z-a23adf7a | DB9BB5DAC14D313CA6673DE2327CA825EF995CF5A6423425B9FA6B70F730BB9C |
| demo_map.ItemUseAndArmor | 46／0 | 0／46 | 20261005T104600812Z-f1a532ab | B0A505026DF978599239EC8C4C8FBDEFB39B2DCDFED79D4D70E4D2DF21B93300 |
| demo_map.P4.Hotbar | 7／0 | 0／7 | 20261005T104646358Z-62c9058b | 45F2451311E4AFF064A30635750F6692CE58A5641C4D73D5A9FD1936D02D1779 |
| demo_map.RewardFullMapDistribution | 84／0 | 0／84 | 20261005T104726846Z-753bb8cf | 1A6181DC736A9001F6BB52149BED7F0887B25B96A369CC21C7F66D9CE94D64C7 |
| demo_map.RewardGeneration | 36／0 | 0／36 | 20261005T104812683Z-4d066940 | A1C008D1CA8CDE48099A18A8FF51B1FB314B057CFF98B25F97FA31B737EF9C6B |
| demo_map.RewardSourceProjection | 40／0 | 0／40 | 20261005T104853391Z-144425a8 | CC4FF1D3DA419D7E21DAADBEE515DEEE9032CD09FABAEEB7360D5DC0DF5C1C25 |
| Shanmen.0_0_10.CombatCore | 9／0 | 0／9 | 20261005T104934022Z-11ef2d1c | 36D531EEF2502C7E729394F89BBBE70F6D7A0DFD7126158E49AAB832B889702E |
| Shanmen.0_0_10.CombatRuntime.BasicSword | 4／0 | 0／4 | 20261005T105014521Z-1a6ff938 | 6FB1C4D5CA36F19249366C13DFFF30B82B8C206076F91B7131B2682C45299CF6 |
| Shanmen.0_0_10.CombatRuntime.VitalityAuthority | 6／0 | 0／6 | 20261005T105051121Z-6e193107 | 07978C46BEBF83DCD061F3CC5A8C564C0158D567AF5B9828F6F2F7FDECBC1470 |
| Shanmen.0_0_10.CombatRuntime.VitalityLedger | 4／0 | 0／4 | 20261005T105131613Z-c1f9303c | 49CDF38A377B7EDE7BB4AA1E166DCA1C6BEF499E7CF515380E959A7C9698EA59 |

## 6. 兼容与实际验收

没有新 item schema 或 World 格式。未完成计时不保存；已搜索由已接纳来源计划证明，恢复读取同结果。原携带终局仍用既有请求，测试包括“已搜索但未领取”后正常撤离，确认计划不当战利品返还。随机敌人组合、领取／部分堆叠／丢弃拾回、局内安全格变更与新药使用仍待下一增量。

本轮未测真实 UI、世界计时／失焦、720／1080、截图、PIE／Standalone、实际帧耗时、内存或多轮对象释放。六标记复用是代码事实，不是泄漏趋势实测；333／366 字节是格式兼容事实，不是运行内存。人工 [来源搜索审查](../Demo20/ManualSourceReview.md) 给隔离启动、搜索／取消／重开／重启、危机中断和预期，待反馈不暂停独立开发。

## 7. 收尾核对

提交前已重新核对最终 11 组原始日志计数、结束记录、native 码及 SHA，并重新运行映射覆盖门禁；40 个文档相对链接、`git diff --check` 和 113 项保护核验均通过。仅精确暂存本轮 20 文件，不包含四份总体文档、104 项旧 untracked、已有存档、失败原件、构建输出或自动化原日志。提交后核对远程 SHA，提交／推送完成才交付 GitHub 链接；不建立第二自动任务，不暂停或完成整个探索 Goal。
