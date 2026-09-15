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
