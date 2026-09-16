# Dev.D.UE.0.0.10.OverallReadiness.r0 审计 Log

## 1. 任务与边界

用户要求总体开发报告：系统/分级、信息交互分区、数据计算的内存占用、各系统完成度，为后续 UI 和实际 UE 开发准备。

本轮是文档与只读分析任务。没有修改产品代码、资产、数值、输入、存档或自动化；没有运行 Editor UI、PIE、Standalone、Cook、Package；没有把后续建议标记为完成。本轮计划仅提交 Report、本 Log、机器可读证据索引三份文件。按照既有“Report/Log 推 GitHub”要求交接，不上传其余用户文件。

## 2. 基线

- 时间：2026-09-14；静态汇总采样 2026-09-14T21:47:49.0428352Z。
- 代码提交：831c6b1efb4e154347f389d424d82fb3f4ba93af。
- 分支：agent/0.0.10-p27-28-formation-scatter-gamemode-composition。
- 远端：https://github.com/MatheHex/MatheHex-shanmen-ue.git。
- 检查开始时 tracked changes = 0，既有未跟踪文件 = 103。
- 引擎：项目声明 UE 5.8。
- 当前最后实现阶段 P27.28；P27.29 冻结审计仍是下一步建议，本报告不代为宣称执行成功。
- 人工版本规划：用户提供的《山门》UE 0.0.10 战斗系统规划讨论报告；以其五大体系和手感优先/数值后置为需求方向，不把原稿中的未来设想视为现有实现。

## 3. 检查方法与范围

1. 核对 Git 分支、HEAD、远端、已跟踪差异与未跟踪集合。
2. 检查 uproject、六个模块 Build.cs；确认纯战斗核/物品域的依赖边界与 demo_map 组合层。
3. 读取物品 Authority/Transactions/Migration ADR。
4. 读取 P0 至 P27 的代表性末期 Report，重点核对最新 P27.28 Report/Log、回归和构建原日志。
5. 对核心结算、物品 Repository/AuthorityService/Persistence、GameInstance 物品宿主、GameMode Run 入口、时间线、动作仲裁、剑气命令、熟练度 Adapter、HUD 和 Journal 进行定点源码核验。
6. 重新计算六模块规模、阶段报告数、全状态拷贝位置数，以及 12 份既有日志 SHA-256 和结果计数。
7. 完成成熟度矩阵、信息分区、UI命令/投影边界、内存风险与后续验收建议。没有用行数或用例数计算游戏完成百分比。
8. 对三个新文档进行 JSON 解析、相对链接存在性、统计一致性和 Git 差异卫生验证；提交时精确限定三份文档。

本报告是广度审计加关键路径深读，不是对全部 30 万非测试行的形式化证明。系统矩阵中一些产品能力引用其对应阶段证据，而不是声称本轮全部重新执行。

## 4. 静态统计口径

仅统计 Git 跟踪的六个模块内 .h/.cpp/.cs，排除 Source 根部 Target.cs；保留空行/注释。以文件名 Test/Tests.cpp 或 .h 分类测试，可能不等于最终编译时测试代码比例。

- 文件 1,084。
- 非测试行 300,974；测试行 160,007；总计 460,981。
- 测试文件 247，按上述行数口径约 34.7%。
- demo_map 非测试行 270,732，约占非测试行 90.0%。
- 基线已有 0.0.10 Report 394 份，未计本轮新增报告。
- Repository 中 `FState Candidate = State;` 静态出现 26 处，不是每命令拷贝次数。

逐模块数据和原日志计数在同目录 evidence.json，可与 source_commit 重算。

## 5. 重要发现与纠偏

### 已有设计证据

- CombatCore 已有有序防御、标签条件、Vitality/致死拦截、ImpactId 重算、伤害守恒和快照捕获；不再沿用早期 P0 的固定五字段评价。
- 物品在线真值由 ShanmenItems 持有；CodeA/CodeB 作为旧来源/兼容边界，不应让新 UI 双写。
- P27.28 提供 GameMode-owned formation scatter 发布和结束失败的恢复保留；不是新增玩家输入或正式地图接线。
- 熟练度权限、动画命令 ACK、World 发布证明，分别不等同于经验成长、资产实际播放、完整游戏性验收。

### 容量与性能风险

- 物品历史记录持续积累，审计路径未见运行时裁剪/容量策略。
- 全量映射复制、快照排序、命令前后对照、规范化/JSON序列化/读回及部分历史嵌套扫描会随数据规模增长。
- 锁内持久化为同步路径；实际是否卡 Game Thread 需要后续追踪调用线程和时长，不能冒称已测到卡顿。
- 无变更保存已有免写盘分支，但不免除所有读取与校验成本。
- 有些局部 Journal 已明确 16 条容量限制；不能概括为所有日志无限增长。满载合法处置仍需验收。
- 48 bytes 编码是 manifest 元数据，不是完整 checkpoint 或恢复内存。
- 628.29 MB 仅为 NullRHI/NoSound 进程启动样本；不是实际游戏内存、完整运行峰值或显存。

### 文档陈旧信息

早期 ItemAuthority ADR 仍称 schema 限定 1..5 未修；当前 ProfileRepository 的 IsSupportedLegacySchema 为 `>=1 && <CurrentSchemaVersion`，Migration ADR 也已记载相应修正。本轮仅指出这条陈旧表述，没有擅自改历史 ADR。

P27.28 早期 Reset 的历史措辞也应以实际差异为准：不要推导成旧拒绝块中曾逐字调用 FormationLifecycle.Reset。总体报告只描述恢复所需的 owner/Coordinator/检查点保留这一可验证行为。

## 6. 原始验证复核

没有重跑 UE 自动化，以下均为对既有文件的独立读取与哈希复核：

| 日志 | Success | Fail | 备注 |
|---|---:|---:|---|
| P27.28_shanmen_full.log | 1415 | 0 | 队列清空，完整新根 |
| P27.28_formation_run_lifecycle.log | 13 | 0 | 与全根重叠 |
| P27.28_gamemode_scatter_focused.log | 3 | 0 | 与全根重叠 |
| P27.28_gamemode_scatter_focused_first_failure.log | 2 | 1 | 保留原始失败 |
| P27.28_legacy_v3_attributes.log | 4 | 0 | 旧系统兼容 |
| P27.28_legacy_enemy_skill.log | 44 | 0 | 旧系统兼容 |
| P27.28_legacy_v2_ranged.log | 22 | 0 | 旧系统兼容 |
| P27.28_legacy_item_armor.log | 46 | 0 | 旧系统兼容 |

- 新根 SHA-256：`75DEE689E683210ACE250292D8086DFC24D8663F6E148BB3513AE4421E42D930`。
- 覆盖检查：PASS Changed=6 Rules=7 Required=83 Logs=7。
- 映射自检：PASS 504/504。
- 最终 Editor：up to date、Succeeded、NATIVE_EXIT_CODE=0、1.77s。
- 最终 Game：34 actions、Succeeded、NATIVE_EXIT_CODE=0、45.56s。
- 12 份日志的完整 SHA-256、字节数、结果计数均在证据 JSON。
- JSON 中 success/fail 对构建/脚本日志只是 UE Result 记录匹配数，不代表“运行零个测试便通过”；实际脚本/构建结论放在 other_log_results。
- 本轮未复跑完整旧 demo_map 根，不复用历史成功冒充本轮测试。
- 最新根组耗时约 1h25m25s；不能套用早期纯值测试几秒的成本论据。

## 7. 交接内容

- [总体 Report](../Report/Dev.D.UE.0.0.10.OverallReadiness.r0_report.md)
- [证据 JSON](Dev.D.UE.0.0.10.OverallReadiness.r0_evidence.json)
- 本 Log。
- 没有复制用户私有草稿、旧报告、PDF、存档、二进制或整份本地日志目录进入本次提交。
- 本轮结论是“可开始 UI 契约和灰盒准备，但真实可玩与性能待验收”；未触发自动开发下一玩法。

## 8. 核验源码入口

下表链接读取对应仓库文件；行号为本报告代码基线。精确基线与各入口也保存在 JSON。

| 路径 | 定位 |
|---|---|
| [CombatResolver](../../Source/ShanmenCombatCore/Private/ShanmenCombatResolver.cpp#L163) | 身份、守恒、有序防御 |
| [Repository Snapshot](../../Source/ShanmenItems/Private/ShanmenItemRepository.cpp#L1225) | 全量导出与排序 |
| [Repository History](../../Source/ShanmenItems/Private/ShanmenItemRepository.cpp#L1674) | 处理记录加入 |
| [AuthorityService](../../Source/ShanmenItems/Private/ShanmenItemAuthorityService.cpp#L402) | Before/After 与锁内持久化 |
| [Persistence](../../Source/ShanmenItems/Private/ShanmenItemPersistence.cpp#L628) | 临时文件、flush、读回、备份 |
| [Formation publication](../../Source/demo_map/demo_mapGameMode.cpp#L950) | 产品组合入口 |
| [Run release](../../Source/demo_map/demo_mapGameMode.cpp#L3957) | 生命周期释放 |
| [Fixed timeline](../../Source/demo_map/demo_mapShanmenCombatRunFixedTimeline.h#L26) | 30Hz逻辑时间含义 |
| [Formation mastery](../../Source/ShanmenCombatRuntime/Public/ShanmenFormationMastery.h#L7) | 操作等级而非经验曲线 |
| [Profile migration](../../Source/demo_map/demo_mapProfileRepository.cpp#L922) | 当前 legacy schema 条件 |

## 9. 最终文档检查

- 证据 JSON 解析通过；模块合计、测试行数和文件数与 Report 一致。
- Report 与 Log 的 37 个本地相对链接已验证目标存在。
- 代码 HEAD 与采样基线一致；无产品代码或资产差异。
- 精确暂存仅本次 Report、Log、evidence.json 三份文件；既有未跟踪文件仍为 103。
- 暂存差异卫生检查通过。Git 的 LF/CRLF 提示是仓库换行策略，不是构建/产品错误。
- 本轮 Report 的当前本地 LF 字节 SHA-256：`BC137AA63AAEBB7FE781A530956810A4FF98DA0CE145456187DEDE1DA366A40E`。
- 证据 JSON 的当前本地 LF 字节 SHA-256：`449299B5BADEBB6AF79B9FC888227AC0DCB4E25173EDA3055DB6F83602022A2F`。
- 上述文档哈希采用 LF；其他工作区若检出 CRLF，应先按 LF 规范化后比较。原始自动化日志哈希按其文件原始字节，不做换行变换。
- 实现代码的测试/构建结果仍只使用第 6 节的历史证据，不把文档检查表述为新一轮 UE 验收。

## 10. 最新状态增补审计（2026-09-15 UTC）

本节是后续只读复核记录，不改写前九节的历史基线与当时文档哈希。由于 Report 新增最新状态说明，第 9 节的 Report 哈希只适用于原始报告版本；原证据 JSON 未改动。

- 入场实现 HEAD 与远端分支均为 `58e696483119137cd4ed3b267b615af812cc7f39`，最新完成阶段 P27.29。工作区另有 5 个未提交实现/映射文件，103 个既有未跟踪文件；本次不纳入交接、不修改。
- 完整读取既有总体报告/证据、P27.29 Report/Log；复核六个模块 Build.cs，以及物品 Snapshot、RecordProcessed、ExecuteCommandLocked 和 CommitDocument 关键代码。
- 重新计算原证据 JSON 所列 12 份 P27.28 日志 SHA-256，全部匹配。旧启动采样 628.29 MB 仍只是 NullRHI 无头进程启动时的 Physical Memory，不是游戏运行峰值或显存。
- 重新读取 P27.29 全根原日志：1,416 Success、0 Fail，`Automation Test Queue Empty 1416 tests performed`；SHA-256 为 `87CB5E8499AFF448ACB38C296AFC54341A200311B21BC8623845A0CAE486B9F5`，同目录 run-state 原生退出 0。
- 独立读取 P27.29 四个旧组：Attributes 4、EnemySkillFramework 44、V2RangedCompatibility 22、ItemUseAndArmor 46，全部 Fail=0；四份 SHA-256 均与 P27.29 文档一致。
- P27.29 最终 Editor/Game run-state 均为 SUCCEEDED、原生退出 0。所有路径沿用 P27.29 Development Log 第 5/6 节，不声称本轮重跑这些任务。
- 本次将 Report 中“P27.29 尚未完成”的当前状态措辞更正为“局部修复完成，整体未冻结”，并添加已提交基线与未提交工作分界。其余系统矩阵、源码规模、内存估算和测量建议保留原始口径。
- 本次交接仅此 Log 与总体 Report；未开发 UI、实际游戏性、资产或产品代码，未启动新的 UE 构建、测试或实际产品。
- 文档检查通过：39 个本地相对链接目标存在，文档差异卫生检查通过；5 个既有修改文件的原字节 SHA-256 均保持一致，未跟踪文件仍为 103，检查期间实现 HEAD 未变化。提交精确限定这两份文档。

## 11. P27.30 总体报告更新审计（2026-09-15 UTC）

### 范围

- 本次请求是面向后续 UI 与实际 UE 开发的总体报告，不是继续新增玩法或执行最终框架冻结。
- 入场 HEAD：`225762c1c4cc23e17df0bddd6c03e8101119bec9`；分支不变，tracked 与 index 均干净，既有未跟踪文件 103 个。
- P27.28 至当前 HEAD 的源码差异仅涉及 GameMode、御器 World 生命周期及相关测试。复核 CombatCore/Items 依赖、物品全量快照/锁内保存/读回、30Hz Run 时间线与阵法分级；没有把历史未修项机械沿用为最新故障。
- 更新当前状态、生命周期矩阵、待办和来源链接；补充 UI 字段表与工作依赖顺序。原工程规模、原证据 JSON、第 7/8 节历史样本不伪装成新采样。

### 原始证据复核

重新计算以下原文件 SHA-256，与 P27.30 Report 对照；逐项读取 Result 和实际队列结束计数，而非只搜索启动参数中的 Queue Empty 字样：

| 原日志组 | Success / Fail | 原字节 SHA-256 |
|---|---|---|
| FullRoot | 1418 / 0 | `F402AF7DE96A5AFE92464D571E1426DBBD22C46E06FE667658E2E3E69F9C724C` |
| WorldRetirementFocused | 3 / 0 | `EAC0FE6A414E04E5F21521037BD431146CEE9A7EE4FD4FEF5F107E0181C5C926` |
| LegacyAttributes | 4 / 0 | `26DE3E38906FC2F508637B3055D28DF321B7E8AB179068FC05A8FBB7D794C028` |
| LegacyEnemySkill | 44 / 0 | `BE0949DC222E1164EFEE37F1A351EC26AE0787308533CC15B2A4F5A23C018480` |
| LegacyV2Ranged | 22 / 0 | `46404EBF6FE6A14E96B19882D54D994D9498621DA2BFE19A48C763120F0120D7` |
| LegacyItemArmor | 46 / 0 | `5E1196988919FB6E7BC75F9D750FB4B25C2A4A8AF52023F4A4868A0BF75BECE4` |
| LegacyHotbar | 7 / 0 | `DD34A53BBE298F8B4254CF866A79E7241F37DDAFE1D3BB6D774EFB8EC82FE858` |
| RedProof（保留失败，不计通过） | 0 / 2 | `7B5C24326F6794F00028E4144890EA9C10A48122AF2549FC1119B88010B7F175` |

各路径完整列于 [P27.30 Development Log 第 5 节](Dev.D.UE.0.0.10.P27.30.r0_log.md#5-原始日志索引)，Saved 原文件留在本地，未复制原日志目录到 GitHub。

- Editor/Game run-state 均为 SUCCEEDED、exit_code=0；stdout SHA 分别为 `5BC9382A911FC01ED745222F3A0173BD65CC28C7AB35A832E5E56A8B43B010B5` 与 `FAC9FD874ADD58ECACFBB8F7B2C5F924397AFBB13B50A328A161089CBA749C58`。
- 已有映射日志：`PASS Changed=7 Rules=3 Required=88 Logs=6`；已有映射自检 `PASS 504/504`。不是本次文档修改需要再跑的产品验证。
- 本轮没有重新启动 UE、运行构建/测试、采样 RAM/VRAM 或修改自动化。没有宣布完整旧根、实际输入、正式地图、UI 或性能已验收。
- 交接只包含总体 Report 与本 Log；不包含源码、资产、历史私有材料或未跟踪用户文件。
- 文档核验：41 个本地链接目标存在；原证据 JSON 可解析；差异卫生检查通过；修改路径恰为这两份文档，既有未跟踪文件仍为 103。发布前远端与本地实现基线均为 `225762c1c4cc23e17df0bddd6c03e8101119bec9`。

## 12. 总体报告当前工作区审计（2026-09-15 UTC）

### 范围与版本

- 本轮继续响应总体报告请求：系统与分级、信息交互分区、计算/内存风险、完成度，以及 UI 和实际 UE 开发的准备条件；不是 heartbeat，也不是继续产品修复。
- 入场 HEAD 与远端分支均为 `e9bcf4f849353aafaea3af1ebb587149af26932c`；最新已交付实现 P27.30。分支为 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 四个既有源码/测试文件已修改但未暂存，103 个既有未跟踪文件；本轮只更新总体 Report 与本 Log。保留原证据 JSON 的 P27.28 历史口径，不把旧规模/内存采样伪装为新测量。
- 未启动新的构建、UE 自动化、实际游戏、UI 或资产工作；入场时已有的 P27.31 编译不属于本轮报告验证，也不纳入已完成验收。

### 只读证据复核

- 重新读取六模块 Build.cs、uproject 的 UE 5.8 与模块声明、物品 AuthoritySubsystem 生命周期、Repository 全量快照/历史、锁内 BeforeDocument/Before/After/SaveAuthority、30 Hz Run 时间线，以及阵法/暗器分级定义。
- 抽查护心镜、御器、神识 HUD、护盾、物品详情、阵法组合阶段 Report 的完成声明和 P/F 限制。没有把局部表现代码等同于真实玩家交互验收。
- 对第 11 节所列八份 P27.30 原测试日志及两个最终构建状态重新读取并计算 SHA-256，全部与该节匹配。完整新根 1,418/0、五个旧组 123/0、专项 3/0；各实际队列终止数一致。失败复现 0/2 仍作为失败保留；构建原生 0/0。
- 既有规模统计是 1,084 个模块代码文件、300,974 非测试行和 160,007 测试行，不代表内存/覆盖率。物品快照复制、历史增长与同步保存的风险仍由当前源码支持；未得到真实 RAM/VRAM 峰值或帧耗时。

### P27.31 未交付项

只读检查现有差异与失败日志，确认终局重放原来仅按 Run 查成功记录；本地修复改为规范化请求后比较已有请求身份和持久指纹。此处描述修改意图，不提前宣布正确性或回归通过。

- 测试：`Shanmen.0_0_10.Items.RunLifecycle.TerminalReplayIdentity`。
- 原始日志：`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0/RedProof/20260915T041521007Z-9be2e054/UnrealEditor.log`。
- 原字节 SHA-256：`7356B1D24714C5328583B56B5ABEFF75D56AD4FFB443B2629186A9F0086C12AB`。
- 结果：0 Success / 1 Fail；`Automation Test Queue Empty 1 tests performed`。同目录 run-state 的原生退出为 0，但测试明确失败，不能写成通过。
- 原日志明确出现结局改变、剩余数量改变、获取奖励元数据改变却仍被当成重放的失败断言。本报告不将这些失败等同于已发生真实玩家损失。
- 修复后的专项、完整受影响回归与最终双目标结果尚未完成交接；因此仍列为冻结前待验证项，不列入 P27.30 的已验证能力。

本轮不修改、不暂存以下文件，原字节保留基线为：

| 既有文件 | SHA-256 |
|---|---|
| Source/ShanmenItems/Private/ShanmenItemRepository.cpp | `AC884CB4091A27A0A804534C9670940B1755F205FDA7EC12B608F36A49136EBD` |
| Source/ShanmenItems/Public/ShanmenItemRepository.h | `92C6160A5D6A1E4E613AD98B5AD1FC38A36C3B5023073AAB9278DC870895893A` |
| Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp | `2CDA373D90953F3CBE9C35C2B201FA6189D157CA198AB55005833ECB5FE5314A` |
| Source/demo_map/demo_mapShanmenRunLifecycleAdapter.cpp | `FCEA358C58EB7991F4CD3244365296C1F525A418737E35EE5C4455AFFD21FCC1` |

### 交付结论

总体报告新增“已交付基线与当前工作区”的明确分界，修正 Run 完成度/优先级，并补充首轮 UI 接入责任表。结论是可以准备 UI 信息架构和字段/命令契约；终局重放缺口仍待验证，整体框架、真实可玩和性能均不得提前宣称冻结或达标。仅这两份 Markdown 文档属于本次交接。

发布前文档检查通过：42 个本地相对链接目标存在，原证据 JSON 可解析，文档差异卫生检查通过；四个既有源码/测试文件的原字节哈希未变，既有未跟踪文件仍为 103，入场至核验时 HEAD 未变、index 为空。精确提交仅总体 Report 与本 Log，不包含正在验证的产品实现。

## 13. P27.31 总体报告更新审计（2026-09-16 UTC）

### 范围与版本

- 继续响应总体报告请求：系统与分级、信息交互分区、计算/内存占用、完成度，以及 UI 和实际 UE 开发准备；本轮不是 heartbeat，也不执行新的产品阶段或冻结审计。
- 入场本地 HEAD 与远端分支均为 `0b6af9c52efde41c1508b4b7a540bb8198a103a3`，最新已完成实现 P27.31。分支仍为 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 入场 tracked 工作区与 index 均干净；103 个既有未跟踪用户文件单独保存原路径/原字节 SHA-256 供结束核验，不纳入交接。
- 仅修改总体 Report 与本 Log。未改产品代码、资产、存档、配置、自动化或历史 ADR；未运行新的构建、UE 测试、实际游戏、UI 或性能采样。
- 原证据 JSON 保留 P27.28 历史基线，前十二节保留当时的判断。当前 Report 的“P27.31 尚未交付”更新为“已提交并完成阶段验证”，不把旧状态继续呈现为当前故障。

### 独立证据复核

完整读取 P27.31 Report/Development Log；重新读取下列原始日志，按实际 Result、精确队列结束及同目录 run-state 联合核验，并计算原字节 SHA-256：

| 原日志组 | Success / Fail | 队列结束数 | 原字节 SHA-256 |
|---|---|---:|---|
| RedProof（原缺陷失败复现） | 0 / 1 | 1 | `7356B1D24714C5328583B56B5ABEFF75D56AD4FFB443B2629186A9F0086C12AB` |
| RunLifecycleFocused | 6 / 0 | 6 | `042307C034766DEF1B8974ECE4BCD74B44799ED10EB9200A958123C1B46B70B0` |
| LegacyFullRoot：demo_map | 1330 / 0 | 1330 | `0966DDC34F0B225CDA740BB7CB8A2EC807F9E7CB211E9091817EDF9896CB7DFE` |
| FullRootResumed：Shanmen.0_0_10 | 1419 / 0 | 1419 | `90A03871AFF2B16FB8E6994D87C94311239BF414D62DF301ABB2E690727C9F6A` |

四组原生退出均为 0；RedProof 的实际 Fail 仍为失败。各日志中匹配到的 Fatal error、Ensure condition failed、Unhandled Exception 为 0，同时各有 13 条既有 Condition failed 文本，不将后者隐去或描述为日志零错误。专项六项包含在新根，不重复累计。

原路径沿用 [P27.31 Log 第 5 节](Dev.D.UE.0.0.10.P27.31.r0_log.md#5-测试日志索引)。完整新根来自一次完整重跑；本轮未把中断日志计数拼接进结果，也未重跑产品测试。

- 最终 Editor / Game run-state：SUCCEEDED / SUCCEEDED，原生 0 / 0。stdout SHA-256 分别为 `B9F35894FFBDFFA79CACDC603DB36D2462D459DF57D93C8C4F88D05DAE641392`、`518B8B4FA26268D986AE7EE04CD450E5577872455962C2ED3FF987D57F225BDE`，均重新计算匹配。
- 原映射六文件记录 SHA-256：`9A2BCB572508D2A8FABD3278F032BDCB032EBAD7B1F5CB50ED520F20196DB611`；原映射自检 504/504，SHA-256：`507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`。这些是实现阶段的验证，不是本次 Markdown 修改新增的测试执行。
- 更新 Run 系统完成度及优先级，明确测试重开物品服务不等于整个游戏的跨地图/进程重启；不将终局重放修复扩张为 World 原子事务或历史裁剪方案。

### 当前静态统计与接口核对

沿用原始统计方法，对当前六模块 Git 跟踪的 .h/.cpp/.cs 重新计数（包含空行和注释；文件名 Test/Tests.cpp 或 .h 归为测试）：

| 模块 | 文件 | 非测试行 | 测试文件 | 测试行 |
|---|---:|---:|---:|---:|
| demo_map | 952 | 270830 | 214 | 143916 |
| ShanmenCombatCore | 9 | 877 | 1 | 317 |
| ShanmenCombatRuntime | 90 | 19607 | 27 | 12059 |
| ShanmenCore | 6 | 142 | 0 | 0 |
| ShanmenItems | 16 | 8769 | 3 | 3606 |
| ShanmenWorldGameplay | 11 | 863 | 2 | 527 |
| 合计 | 1084 | 301088 | 247 | 160425 |

- 总计 461513 行，测试行约 34.8%，demo_map 占非测试行约 90.0%。Git 跟踪的 0.0.10 Report 为 398 份，包含总体报告。不是覆盖率、包体、运行内存或游戏完成百分比。
- 再次完整读取 uproject 与六个 Build.cs：声明 UE 5.8；CombatCore/Items 无 Engine 模块依赖，CombatRuntime 依赖 GameplayAbilities/GameplayTasks，demo_map 为产品组合层。
- 定点读取 Repository CaptureSnapshot、RecordProcessed、IsExactFinalizedRunReplay，AuthorityService ExecuteCommandLocked，Persistence CommitDocument；26 个全状态候选复制位置及五类映射导出/排序仍存在，锁内持久化与历史增长的静态风险不变。当前审计路径未发现 ProcessedRequests 运行时裁剪或容量上限，不据此断言已发生泄漏。
- 重新读取 GameInstance 物品 authority 生命周期、Run 30Hz 时间线、阵法/暗器分级定义和 P1.14 产品切换报告；核对 V3 observer 的 UsesShanmenItemLifecycle / PostActivationNoLegacyWrite / PostTerminalNoLegacyWrite 分支。报告明确“唯一权威”是领域约束与已有路由证据，不假称已证明全部历史入口不可达。
- 核对局部 Journal 16 条上限与 manifest 48 bytes 定义：不概括为所有队列无限，也不把 manifest 当完整 checkpoint。
- 重新读取 P27.28 启动采样原行：进程 Physical 628.29 MB、Virtual 648.35 MB，仍是 NullRHI/NoSound 的历史启动样本。本轮没有获得实际游戏 RAM/VRAM 峰值或线程耗时。
- 保持七类 UI 分区、五类数据所有权、操作等级与成熟度等级的区别；后续页面契约、灰盒接线、性能采样均是准备要求，不声称已实现或已经授权本轮执行。

### 交接范围

仅更新总体 Report 与本 Log，沿用现有 GitHub 文档交接，不发送技术信封、不上传 Saved 原日志目录、私有草稿、无关用户文件或产品代码。未开始 UI/游戏性实现，未改变监控，也未宣布底层整体冻结。

发布前检查：45 个本地相对链接目标全部存在，原证据 JSON 可解析，`git diff --check` 通过；差异精确为这两份 Markdown。103 个既有未跟踪用户文件的路径集合和原字节 SHA-256 全部保持；检查时 HEAD 与入场一致，index 为空，没有产品代码差异。换行策略的 LF/CRLF 提示不作为产品故障。

## 14. UI与UE开发准备总览复核（2026-09-16 UTC）

### 当前请求与隔离范围

- 本次响应用户要求的游戏开发总体报告，包含系统与分级、信息交互分区、计算/内存、完成度，以及后续 UI/真实 UE 开发准备。途中到达的 heartbeat 不改变本次文档交付范围；本轮不继续 P28.0 实现，也不开始实际游戏性开发。
- 入场 HEAD 与远端分支均为 `d62343b3c4ba48d3ba9dc5c97f9cc7569e39ca81`；最新产品基线仍为 P27.31 / `0b6af9c52efde41c1508b4b7a540bb8198a103a3`。
- 入场 index 为空；7 个既有 tracked 修改、107 个既有 untracked 文件（P28.0 新文档 4 个及原用户文件 103 个），共 114 个文件记录原路径/原字节 SHA-256，供结束核验。本次仅编辑、交接总体 Report 和本 Log。
- 本地 P28.0 索引为 FREEZE_AUDIT_IN_PROGRESS。准确记录 FZ-1 全入口路由、FZ-2 激活失败/最终释放核对、FZ-3 最终冻结证据；不把待审计当已证实故障，不把未提交文档列为已交付成果。

### 证据复核与报告改进

1. 读取既有总体报告、P27.31 Log 及 P28.0 本地索引；重新核对 uproject 与关键模块 Build.cs，分离纯结算、物品服务、GAS 编排与产品组合职责。
2. 直接读取 P27.31 两个最终根的原日志：旧根 1330 Success / 0 Fail，新根 1419 / 0；均有精确的 `Automation Test Queue Empty <count> tests performed`，同目录 run-state 均 SUCCEEDED、原生 0。日志哈希重新计算，与第 13 节相同。未重跑这些测试。
3. 重新读取 P27.31 最终 Editor/Game run-state 与 stdout 哈希，均匹配第 13 节的 0/0 和原字节证据；没有在本次报告工作中启动构建。
4. 核对 Repository 五类映射的 CaptureSnapshot 导出/排序、26 处候选状态复制、RecordProcessed 追加；核对 AuthorityService 的 BeforeDocument、Before/After 与同步持久化调用，以及临时文件/读回/备份/替换流程。风险表述仍是增长与瞬时峰值/耗时风险，不诊断未测得的泄漏或卡顿。
5. 核对阵法/暗器的操作等级定义与纯结算身份校验。保留 D0–D4 工程成熟度与玩家操作分级的区别；不以 Master 枚举宣称高级智能操作或成长经验闭环完成。
6. 新增总体快速决策表，并修正报告里已过时的工作区干净、索引尚不存在和 ADR 状态表述。既有五区数据权威、七区 UI、内存模型、系统矩阵与下一阶段准备清单保持；不新建业务真值或改产品代码。

本轮未运行内存/显存采样、Editor UI、PIE、Standalone、产品可执行文件、实际输入、Cook 或 Package。原机器可读证据仍是明确标注的 P27.28 历史样本，不改写为当前测量。

### 发布前检查

- 两份 Markdown 的 46 个本地相对链接目标全部存在；原 evidence.json 可解析；限定两文档的 `git diff --check` 通过。
- 114 个入场既有修改/未跟踪文件的路径与原字节 SHA-256 均保持；另外 1,577 个跟踪的 Source/Config/Content/Plugins/uproject 输入与先前保存基线相同。没有混入产品或 P28.0 文件。
- 重新读取历史无头日志中的 628.29 MB Physical、648.35 MB Virtual 启动采样及 NullRHI/NoSound 参数。报告继续明确这不是实际游戏峰值或显存。
- 检查时 HEAD 仍与入场一致；使用精确两文件暂存和普通非强制推送。未上传原始 Saved 日志、个人文档或私有路径清单；未改变监控状态，也未宣布框架最终冻结。

## 15. P28.0 已交付后的总体报告与接线准备（2026-09-16 UTC）

### 本轮范围

- 用户要求游戏开发总体报告：系统与分级、信息交互分区、数据计算/内存、各系统完成度，为 UI 与实际 UE 开发准备。本轮是文档交付，不执行 heartbeat 中的“继续开发”，不修改玩法或框架代码。
- 入场 HEAD 与远端分支均为 `867a33794bfbd966f952bc888a6d422eeb1d7ae4`；P28.0 已提交。最新产品实现仍为 P27.31 / `0b6af9c52efde41c1508b4b7a540bb8198a103a3`，两提交之间 Source/Config/Content/Plugins/uproject 差异为空。
- tracked 工作区与 index 入场干净，103 个既有未跟踪用户文件及 1,577 个产品输入保存原字节 SHA-256 基线。只编辑现有总体 Report/Log，不添加另一份同内容报告。
- 未改监控、资产、配置、存档或代码；未新跑 UE 测试、构建、内存采样、Editor UI、PIE、Standalone、游戏 exe、实际输入、Cook 或 Package。

### 本次核验

1. 读取总体报告、P27.31 Log、P28.0 Report/Log、共享 P 基线和冻结索引。纠正旧报告中 P28.0 仍为未提交草稿的状态，保持 FZ-1/2/3 未关闭的真实界限。
2. 读取 uproject 与六个模块 Build.cs；再次按模块内 Git 跟踪 .h/.cpp/.cs 统计，六模块仍为 1,084 文件、301,088 非测试行、160,425 测试行。Source 根目录的两个 Target.cs 不属于六模块表，不混入原口径。当前 0.0.10 Report 数为 399，包含 P28.0；不是完成度或覆盖率。
3. 重新读取 P27.31 两个最终根原日志及同目录 run-state，匹配实际 Result、精确队列结束数、原生状态并重新计算 SHA-256：新根 1,419 Success / 0 Fail、原生 0；旧根 1,330 / 0、原生 0。SHA 与第 13 节相同，分别为 `90A03871AFF2B16FB8E6994D87C94311239BF414D62DF301ABB2E690727C9F6A`、`0966DDC34F0B225CDA740BB7CB8A2EC807F9E7CB211E9091817EDF9896CB7DFE`。这是复核既有验证，不是本轮重跑；不累计重叠专项，不将日志里的命令行文字当队列完成。
4. 读取最近 P28.0 Editor/Game 原 run-state，均 SUCCEEDED/native 0。stdout SHA 分别为 `46899DC8FF264B7C385BDB9CA03B1BA30C948C2D582016C8766883EF9D7F095B`、`BCDF7D0DD1B6BEC7EAC4F836832F447D4CA253A3BF3A828AB719071CD7B117AA`，与 P28.0 Log 匹配；未再次编译。
5. 定点读取 Repository 的五类快照导出/排序、RecordProcessed 和 26 个 Candidate 复制位置；AuthorityService 的 BeforeDocument、Before/After 与锁内 SaveAuthority；Subsystem 的 Game Thread 前置要求及直接同步调用。将“如果在主线程”收紧为这条产品路径确实同步在 Game Thread 执行，但不声称已测得帧卡顿或泄漏。
6. 读取阵法 Mastery 与暗器 ArcPlanner 的等级/权限定义和 Run 固定时间线 30 Hz 声明。保持操作等级与 D0–D4 成熟度、逻辑 tick 与渲染 FPS 的区别。重新定位并读取 `Saved/Automation/P27.28/P27.28_shanmen_full.log` 的 628.29 MB / 648.35 MB 启动采样及 NullRHI/NoSound 参数；原证据 JSON 保留历史基线。
7. 静态发现待核验路由：PreparationFlow 的 UsesShanmenItemLifecycle 依赖绑定 Authority 为 Ready；V3 RequestUseInventoryItem 否则调用旧 Runtime 使用方法；durable Subsystem 可能进入 RecoveryRequired。本轮没有故障注入、没有 Manager 入口复现，不认定实际重复消费、坏档或玩家损失；在 Report 第 9.1 节归入既有 FZ-1，不实施修复。
8. 新增首轮接线验收卡，要求真实非零状态、同一物品/请求身份、权威副作用、页面重绑和故障状态分别可证。它是未来准备要求，不是本轮已执行或新玩法授权。

### 交付边界

继续用同一总体报告承载六模块分层、五区数据所有权、七类 UI 分区、系统成熟度矩阵、内存事实/假设和分步准入。只提交 Report/Development Log；原 Saved 日志与私人文件不上传。当前结论是可开始 UI 信息架构与接线设计，底层尚未最终冻结，真实游戏性/性能/发行验收未完成。

发布前检查：两份文档的 53 个本地相对链接目标存在，历史 evidence.json 可解析，`git diff --check` 通过。当前两文档映射为 `REGRESSION_COVERAGE: PASS Changed=2 Rules=0 Required=0 Logs=0`；0 个 UE 必跑组只表示此次为纯文档变更，不是新的产品验证结论。103 个既有未跟踪文件与 1,577 个产品输入的路径集合及 SHA-256 均与入场完全一致，HEAD 未变化；精确暂存这两份 Markdown，普通推送，不强推。

## 16. P28.1 总体报告现状复核（2026-09-16 UTC）

### 范围与当前基线

- 用户要求系统与分级、信息交互分区、计算/内存和完成度总体报告，为 UI 与实际 UE 开发做准备。本轮仅更新现有总体 Report/Log，不继续 heartbeat 开发，不执行新的产品修复、UI/玩法或最终冻结。
- 入场 HEAD 与远端分支均为 `6b4876710d92a8d894312cee2b538e1fc9481fa3`，即 P28.1；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- tracked/index 入场干净；保存 1,577 个 Source/Config/Content/Plugins/uproject 输入及 103 个既有未跟踪用户文件的原路径/原字节 SHA-256，供发布前保持核验。不提交 Saved 或私人文件。
- 未修改产品、资产、配置、存档、ADR 或监控；未运行新的 UE 构建/测试、内存采样、实际输入、Editor UI、PIE、Standalone、游戏 exe、Cook 或 Package。

### 原证据复核，不是重跑

读取 P28.1 四组原始 UnrealEditor.log 与各自 run-state；读取 P27.31 最终完整新根及状态。按实际 Test Completed、精确队列结束与原生退出联合判断，重新计算字节哈希：

| 阶段/组 | Success / Fail | 队列完成 | 原生码 | 原日志 SHA-256 |
|---|---|---:|---:|---|
| P28.1 RedProof | 0 / 1 | 1 | 0 | `BE570297C6BD2C61FCAFD4C5CBDC7DF18E716ABCAF9CCC67C85B7744DDF4D536` |
| P28.1 ProductFlowFocused | 4 / 0 | 4 | 0 | `5A64E0078015D607F7084794762280862A33B8326DC052CBF63837F97E4B3E67` |
| P28.1 ItemsFullRoot | 80 / 0 | 80 | 0 | `FA06D567A191576E59931A05B02BC57FEF7867653CD2096D90E08C29B0933559` |
| P28.1 LegacyFullRoot | 1330 / 0 | 1330 | 0 | `6C83F335D64673CA6B405EB61DDDC37F395F18ACC9A5211E6C3A048AC57D7A40` |
| P27.31 FullRootResumed | 1419 / 0 | 1419 | 0 | `90A03871AFF2B16FB8E6994D87C94311239BF414D62DF301ABB2E690727C9F6A` |

路径分别见 [P28.1 Log 第 4 节](Dev.D.UE.0.0.10.P28.1.r0_log.md#4-自动化原日志) 和 [P27.31 Log 第 5 节](Dev.D.UE.0.0.10.P27.31.r0_log.md#5-测试日志索引)。五组各有既有 13 条 Condition failed；Fatal error / Ensure condition failed / Unhandled Exception 匹配为 0。RedProof 虽 native 0 仍是失败，不改写原件。ProductFlow 四项包含在 Items 八十项；P27.31 新根不能当成 P28.1 修复后的完整新根。

最新 P28.1 Fixed Editor / Game 原 run-state 均 SUCCEEDED/native 0；重新计算 stdout SHA-256 分别为 `60C5238AB258D99E8395DFF1E74E93A4A81D1E902ABDF3945658E5329D48580A`、`E44CD3B9520BB98986D1DA99743B4FC02AD897D9A5EFE26EB91D7F0B22C5097E`，与阶段 Log 一致。本轮不把构建输出当产品运行或完整冻结验收。

### 代码与报告变化

1. 重新读取 uproject 与六个 Build.cs：UE 5.8，CombatCore/Items 无 Engine 模块依赖，GAS 在 CombatRuntime 编排层。重新统计六模块内跟踪 .h/.cpp/.cs：1,084 个文件、301,110 非测试行、247 个测试文件、160,580 测试行；其中 demo_map 非测试 270,852 行、测试 144,071 行。共 461,690 行，测试约 34.8%、demo_map 约占非测试行 90.0%；400 份 0.0.10 Report，不把行数/报告数当完成百分比。
2. 直接读取当前 Flow 的 UsesShanmenItemLifecycle 与 ready-only FindBoundShanmenAuthority；将总体报告第 9.1 节过时的“候选待复现”改为 P28.1 已复现并修复。更新最新验证表、系统矩阵与冻结边界，不修改产品实现。
3. 读取 Runtime 的 TransferContainerItemToInventory、PickupWorldItem、RequestSettlement，以及 RunLifecycleAdapter 的 SecuredOriginals/AcquiredItems 校验和规范化终局请求。报告增加“局内拾取成功不等于永久入库”的 UI 边界；不以静态方法存在断言玩家损失，也不擅改 ADR 将 Runtime 可变模型认定为全入口合法投影。FZ-1 仍需有限调用图审计。
4. 重新读取 Repository 五类快照导出/排序、RecordProcessed、26 处 Candidate 拷贝位置；AuthorityService 保留 BeforeDocument/Before/After 并保存，产品 Subsystem 要求 Game Thread 且同步调用。报告继续区分历史增长、临时副本与主线程等待风险，未测得泄漏、峰值或卡顿。
5. 重新读取 P27.28 原日志的 NullRHI/NoSound 参数及 628.29 MB Physical / 648.35 MB Virtual 启动采样；它们不代表真实地图、显存、长时或运行峰值。原 evidence.json 保持明确的 P27.28 历史口径。
6. 读取阵法分级、暗器弧线权限与 Run 固定时间线：保留操作权限等级与 D0–D4 成熟度、30 Hz 逻辑 tick 与渲染 FPS 的区别。保留六模块、五类数据所有权、七类 UI 分区和四份准备清单，不新增同内容平行文档或业务服务。

### 当前结论

可以准备 UI 信息架构、字段/命令表、UE 类与资产接线清单及性能采样方案；不能宣布所有物品入口已统一、框架整体冻结、真实可玩或性能达标。仅提交本总体 Report 与 Development Log；未开始实际游戏性开发。

发布前检查：两文档 56 个本地相对链接目标存在，历史 evidence.json 可解析；`git diff --check` 通过；改动映射为 `REGRESSION_COVERAGE: PASS Changed=2 Rules=0 Required=0 Logs=0`，这表示纯文档无 UE 必跑组，不表示新产品测试通过。1,577 个产品输入和 103 个既有未跟踪用户文件的路径/原字节 SHA-256 与入场一致，HEAD 未变化。精确暂存两份 Markdown 后提交及普通推送，不强推、不混入用户文件或 Saved 原始证据。

## 17. 总体报告交付核对与未提交开发隔离（2026-09-16 UTC）

### 范围

用户当前要求总体报告，涵盖系统与分级、信息分区、计算和内存、完成度，为 UI 与 UE 实装准备。本轮不按历史 heartbeat 继续开发；复用并校正既有总体 Report，避免另起重复报告或新增产品功能。

入场本地 HEAD 与远端分支均为 `f89605577b6e07e30c3e87fd949b89144297bf57`，最新已提交产品为 P28.1。Index 无暂存内容；工作区已有四个 P28.2 源码修改和一个冻结索引修改，另有 105 个未跟踪文件（103 个既有用户文件及两份 P28.2 阶段草稿）。记录 1,723 个产品/脚本输入、未跟踪文件及修改中索引的原字节哈希作为保持基准。仅允许本总体 Report/Log 进入这次文档提交。

### 本轮核验

- 完整读取总体 Report，读取最新冻结索引、P28.1/P28.2 阶段材料；重新读取 uproject 与五个新模块 Build.cs，确认六模块及 Engine/GAS 依赖分区未改变。
- 直接核对 Repository.CaptureSnapshot 五类全量数组导出与排序、RecordProcessed 历史追加、26 个 FState Candidate 拷贝位置；核对 ExecuteCommandLocked 的 BeforeDocument/Before/After 和同步 SaveAuthority，及 Subsystem 的 Game Thread 前置条件。维持“有增长/复制/同步等待风险，尚未测得实际泄漏或卡顿”的结论。
- 重新读取 P27.28 原日志，确认 628.29 MB Physical / 648.35 MB Virtual 为 NullRHI/NoSound 下的启动采样，不是实际游戏 RAM/VRAM 峰值。
- 读取 P28.1 Items/旧根的原始日志及 run-state，Success/Fail 为 80/0 与 1330/0，native 均为 0；原 SHA-256 与第 16 节相同。读取 P28.1 Editor/Game run-state，均为 SUCCEEDED/native 0。本轮不重跑 UE。
- 本地 P28.2 原始 Items/旧根分别为 84/0、1330/0，native 0；SHA-256 分别为 `898B2FDD2D2174BF0299E5695358DE571434FCAE48433B474953FBC74490C921`、`8A5A38D68075D55FF8F49E4898A14872D5B5201628BEA593A7B42F95596E9D8B`。路径为 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0/ItemsFullRoot/20260916T205309354Z-783a8928/UnrealEditor.log` 与 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0/LegacyFullRoot/20260916T205334895Z-03207bde/UnrealEditor.log`。
- P28.2 Game 的 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0.Validation/BuildGame/20260916T205430917Z-fbf9f625/run-state.json` 已记录 SUCCEEDED/native 0，完成于 20:58:42.0048041Z。但阶段 Report/Log 仍为待完善草稿，源码未提交；总体报告只列本地进展，不代替该阶段发布审核或最终冻结证据。

### 交付结论

总体 Report 保留六模块职责、五类信息所有权、七类 UI 分区、D0–D4 完成度、操作熟练度分级、内存事实与情景估算、UI/UE 准备清单。修正开头工作区快照，分清已提交 P28.1 和未提交 P28.2；不把四份不同基线的测试结果拼成当前全量通过。

本轮未改动产品代码、配置、资产、存档、监控或 P28.2 阶段文件；未启动新的构建/UE 测试、Editor UI、PIE、Standalone、Cook/Package。后续准入仍是先闭合有限框架审计，并准备字段/命令、页面状态和 UE 接线清单；未自动开始实际游戏性开发。

发布前检查：总体 Report/Log 的 55 个本地相对链接目标均存在，历史 evidence.json 可解析，`git diff --check` 通过。精确两文档映射结果为 `REGRESSION_COVERAGE: PASS Changed=2 Rules=0 Required=0 Logs=0`；这是纯文档检查，不是新 UE 回归证据。1,723 个受保护路径集合与原字节 SHA-256 均保持，入场 HEAD 未变、index 无旧暂存；仅暂存总体 Report/Log 并普通推送，未将 P28.2 源码或草稿一并发布。
