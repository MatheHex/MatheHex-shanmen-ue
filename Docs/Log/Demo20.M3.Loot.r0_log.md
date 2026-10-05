# 山门 Demo2.0 搜索领取与局内背包 Development Log

日期 2026-10-05；进入基线 `dd6d0255434839190a76c1e40f91ff4de8ea10e2`，分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。范围及限制见 [Report](../Report/Demo20.M3.Loot.r0_report.md)。本轮是 M3 的领取增量，不是完整探索 Goal 或最终可玩 Demo 验收。

## 1. 开始检查与权限

读取完整 Goal、最新 Sources Report／Log、当前 Git／保护、进程、Catalog／Loadout／Medicine／世界记录、生成来源服务、Repository／Grid 和既有 InventoryWidget。普通丹药的人工成功保留为前一阶段证据，不重新调参，也不扩大为新拾取路径通过。实时操作交人工，独立开发不等待画面反馈；旧自动化保持暂停。

本轮没有启动 Editor UI、PIE、Standalone、产品 exe，没有接管窗口或新增截图。无头 UnrealEditor-Cmd 和 Editor／Game 编译均使用既有脚本、隐藏进程及隔离 native 夹具。四份用户 tracked 修改、104 项旧 untracked 及已有存档保持，113 项保护哈希核验通过。

## 2. 本阶段实现

21 个 Source：Items 的 AuthorityService h／cpp、Repository h／cpp、GeneratedSource cpp、Grid cpp、Types h／cpp、Grid h，新增 RunGrid cpp；Demo20 的 Expedition、InventoryWidget、Medicine、SourceWorld、World h／cpp、WorldCheckpoint h／cpp，现有 ItemAuthoritySubsystem h／cpp，新增 LootTests cpp。另交付 README、ExpeditionStatus、ManualLootReview、Report 和本 Log 五份文档，共 26 文件。无 Config、Content、地图、旧 writer、真实存档、脚本或自动任务修改。

已接纳随机计划一次落实到原物品／来源世界格图；Run 格子命令复用既有几何、CAS、部分堆叠、回执重放和原子持久化。原携带余额只读占格，实际权威数量仍在原消费账本；显示投影不送载入／保存，拖动预检用真实快照。局内新物和安全格可整理，原携带物与装备暂只读。

新领取药使用同一 Reserve／Commit 和世界意图，确认扣药后才治疗；最后一颗恢复匹配精确已提交预留。终局清理未领取世界物；撤离保留已领取新物，死亡按确认后的普通／安全格位置损失或保留。新版 operation enum 尾部追加，World Origin 新增 2，Schema 6 不增字段；旧 DLL 不能理解新操作，切换版本先退出旧进程。

## 3. 双目标构建

命令 `./Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M3.Loot -AttemptId attempt-NNN`，原始 stdout／stderr／result.json 保留在 `Saved/Automation/Demo20.M3.Loot/Build`。Editor 成功才执行 Game，不把编译产物当作实际游戏验收。

| 尝试与目标 | 原生码 | 外层 | 秒 | stdout SHA256 |
| --- | --- | --- | --- | --- |
| 001 Editor | 0 | 0 | 538.674 | D7235FADE5AA72C3561D26FBE9A9984B474BF07FCCC25ABA14A63E51518E3725 |
| 001 Game | 0 | 0 | 518.008 | 24F1A8DF522EA1215A7E954409400B373A8B469FD769F6B3FFB614E4D98BB9C9 |
| 002 Editor | 0 | 0 | 354.117 | 17973E8CEF49FB6981642FFE8F0841C2B14C9BE83C2B311EA43747D5E98002F6 |
| 002 Game | 0 | 0 | 511.624 | DD6A4BB8BD238D090B452783F584442D6052C444CC5E17A0CCFEEC8AEBE776E4 |
| 003 Editor | 0 | 0 | 510.595 | 9110C2F18BA845ABB2CA27E4A84EBBD8EEF890C716C4F8D35E9F646CA43C6DFB |
| 003 Game | 0 | 0 | 19.589 | 9C96692A0A4D781DA0755BC4D38F91F07E67C300DCE0D422BA33035232D54E1A |

001 为实现中间输入，002 编入十一项 Loot 用例但随后发现一项错误补给预期；两者均不替代最终输入。修正该测试和投影注释后，003 双目标原生／外层 0，stderr 均为空，最终回归前核对 21 个 Source 文件哈希未变。003 Editor 于 12:05:15.682Z 完成，Game 于 12:05:35.291Z 完成；本轮未出现编译失败。

## 4. 首次自动化失败与修正

命令 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M3.Loot.Regression001`。首个 Demo20 根 54 次执行，53 Success／1 Fail，Queue Empty 54、UE 原生退出码 0、外层因失败记录返回 1；没有继续剩余组。run-state 的 SUCCEEDED 只代表进程码，不代表测试全部通过。

首次原件 `Saved/FoundationRuns/Demo20.M3.Loot.Regression001/Automation-Shanmen.Demo20/20261005T114404556Z-87678ea9/UnrealEditor.log`，SHA256 `7C326538E2CC33AAAC70F7153BC303AFBCE010EB9E328E6CFC49E620687C2925`。开始 11:44:04.598Z，结束 11:52:45.937Z。启动停在 TurnkeySupport；只读进程链确认其子 Build.bat 正在 ValidatePlatforms，Game 构建结束后 SDK 校验和自动化正常继续，未终止进程或启动 UI。

失败 `DeathUsesConfirmedPlacementAndAllowsResupply` 误认必须发补给并立即出发。实际初始仓库有重剑／皮甲／大行囊，死亡后安全格还保留新药；既有补给规则正确拒绝全库存仍有的物品，装备槽却尚未穿戴。产品没有因测试修改补给规则。

改为 `DeathUsesConfirmedPlacementAndAllowsNextRun`：精确断言 BasicSupplyNotNeeded 和非零库存不变，再走整备 EditGrid 正常穿戴三件原仓库备用装备，按正常携带端口开始下一局。保留原死亡位置／安全格三颗／移出损失／未领取销毁／仓库与钱包逐项不变／持久重开／旧世界不能下一局领取的断言，不删失败证据。

## 5. 最终改动驱动回归

最终输入双目标构建后执行 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M3.Loot.FinalRegression`，从 21 个实际 Source 路径推导 Demo20StandaloneSlice、Items、ItemProductAdapters 三条映射，合并八组。Demo20 根包含十一项 Loot，不另跑子组制造重复计数。

最终 249 Success／0 Fail、249 个去重路径，八组原生码及外层均 0，Queue Empty 数量与成功结果一致，run-state 有完成时间。覆盖门禁 `REGRESSION_COVERAGE: PASS Changed=21 Rules=3 Required=8 Logs=8`。最后一组于 12:10:57.964Z 完成；进程退出后重新核对八份日志哈希。此处不是 M5 全部新旧测试根的最终候选验收；不拿上一轮 398 或本轮首次 53／1 替代最终结果。

原件根 `Saved/FoundationRuns/Demo20.M3.Loot.FinalRegression`；每行日志为 `<根>/Automation-<测试组>/<目录>/UnrealEditor.log`，同目录保存 run-state.json。

| 测试组 | Success／Fail | 原生码／Queue Empty | 目录 | SHA256 |
| --- | --- | --- | --- | --- |
| Shanmen.Demo20 | 54／0 | 0／54 | 20261005T120552201Z-962b6150 | BF273BBA897C7890BEC9F9E785AF275B8A97B8DC228091F4602AD28E2B56B31B |
| Shanmen.0_0_10.Items | 119／0 | 0／119 | 20261005T120652944Z-e441e1df | 1A36D977012C1D13AAAD2E2A8F8748BB6BEB33DEB36CE39F32F4260C159E20F6 |
| demo_map.ItemUseAndArmor | 46／0 | 0／46 | 20261005T120743735Z-71d4b1d5 | F163CBDC30D50D131CC3C4ADE23AD1D506C6644CF2B076350ACD26AF938D2888 |
| demo_map.P4.Hotbar | 7／0 | 0／7 | 20261005T120819396Z-2a2cbc75 | 1ED0012EDABABC33C98246CC28986477F4D58B7575FB8A570D1297D4BA56E4FA |
| Shanmen.0_0_10.CombatCore | 9／0 | 0／9 | 20261005T120851977Z-74c646c1 | D40D1504A6242F45C4B3E81201239F71DE44F384F740F9E2EA28EBF19BFE6B94 |
| Shanmen.0_0_10.CombatRuntime.BasicSword | 4／0 | 0／4 | 20261005T120923638Z-06d30f09 | 365C66132D0C1D768F98214F6BF0A9B7BAB3B1CA9EED7A2EDA6CF0107557032A |
| Shanmen.0_0_10.CombatRuntime.VitalityAuthority | 6／0 | 0／6 | 20261005T120955034Z-f60229f7 | 758F6E641AD28C88592423725F60EC75D282DA6AC12BD2179B1F2469C959D138 |
| Shanmen.0_0_10.CombatRuntime.VitalityLedger | 4／0 | 0／4 | 20261005T121026515Z-a7cdabbe | E5B63C855A5BBBB1F8FA6FBD01C653A88DE8941C9943026EAEF792CE303A5F60 |

## 6. 兼容、人工和剩余范围

新版本可读旧 World Origin 0／1，新增 Origin 2 走真实普通 Stored 药。世界未用药仍 333 字节、待确认用药仍 366 字节；这仅是序列化格式，不是运行内存。来源落实与 Run Grid 回执在现有 JSON codec 持久化，恢复校验来源计划与 Run 绑定，没有另建来源数量后端。

新用例覆盖六角色真实随机计划、非零携带占格、十二颗策划夹具的满十余二／拆分、安全规则和旋转、撤离／死亡／下一局、原生失败回滚／精确重读、待确认消费失败关闭、新药／最后一颗恢复、篡改绑定与真实 GameInstance。策划夹具不是 RNG 精确输出；六角色实际随机计划单独核验。

本轮没有实际 UI、输入／失焦、720／1080、PIE／Standalone、帧耗时、运行内存或多轮对象释放证据。人工步骤见 [ManualLootReview](../Demo20/ManualLootReview.md)，需安全处搜索／整理，世界敌人不停；非满生命新药不要求人为站在攻击圈耗尽原药。待人工不暂停独立开发。

原携带物自由整理、跨来源同类可追溯合并、可见地面丢弃拾回、随机敌人组合、目标内容量、详细结算摘要、损坏档玩家修复以及完整实际循环／性能仍未完成。来源退回不是地面丢弃。历史来源、回执和 tombstone 增长、刷新与拖动的快照成本列为实测事项，不声称无泄漏。

## 7. 收尾核对

提交前已复核最终八组日志计数、Queue Empty、原生码与 SHA，首次失败原件 SHA 未变；48 个文档相对链接、`git diff --check` 和 113 项保护核验通过。Items Build.cs 仍只依赖 Core／CoreUObject／GameplayTags／ShanmenCore、Json／JsonUtilities 与 bundled OpenSSL，未加入 Engine 或 demo_map 模块；新增 RunGrid 没有 UWorld、AActor 或 RNG。整包文字扫描会命中旧 Types 注释中的 demo_map，不误报为新增模块依赖。

仅精确暂存本阶段 26 文件，不包含四份用户文档、历史 untracked、存档、首次失败原件、自动化与构建输出。提交／推送和远程 SHA 核对完成才给 GitHub 链接；完整 Goal 保持活动，旧自动化不恢复。
