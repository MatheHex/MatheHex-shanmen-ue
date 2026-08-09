# Dev.D.UE.0.0.9B.P21.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P21——单一 BasicCorpse 的正式敌方装备布局与确定性装备取得
- 任务编号：Dev.D.UE.0.0.9B.P21.0.r0
- 任务性质：在 P11 已建立唯一 Low Skirmisher 的尸体真值、P12 已建立其搜索、揭示与真实拖拽、P16/P20 已建立 BasicCorpse 的一次性确定性内容来源、P7 已建立 P6 个人背包与装备拖拽之后，为同一具既有 BasicCorpse 增加正式的兵器、道袍和普通饰品装备位。P21 只让未来未物质化尸体在其自身装备位中最多出现一件现有的、可装备的非空间物品，并允许该已揭示物品经既有 P12 真实拖拽先进入空 P6 BaseQuick，再由既有 P7 装备。不得实现装备数值、战斗效果、第二尸体、第二敌人、地图掉落来源、物品使用或 F 阶段验证。
- 执行文件：Dev.D.UE.0.0.9B.P21.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P21.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 引擎：C:\Program Files\Epic Games\UE_5.8
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r1、P10.0.r0、P11.0.r0、P12.0.r0、P13.0.r0、P14.0.r0、P15.0.r0、P16.0.r0、P17.0.r0、P18.0.r0、P19.0.r0 与 P20.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P21 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、进程检查、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 持续有效的真值、来源与边界

1. Code A 继续拥有地图、敌人 Actor 的生成／死亡／可见表现、尸体交互距离、原始输入派发、Player Actor、战斗、生命、HUD、正式 Run 生命周期、终局分类、Run Save、旧库存、旧 Loot／搜索及所有尚未迁移运行时。Code A 不得决定、物质化、保存、镜像、复制、拆分、装备或结算 Code B 的尸体装备图；P21 预期不需要 Code A 功能改动。
2. Code B P1 Repository 是新物品体系唯一可变真值。P5 是局外 Profile snapshot；P6 是精确 OwnerId + RunInstanceId 的活动 Run 玩家图；P9 BasicCache、P11 BasicCorpse 和 P14 WorldDrop 是独立 Run-local 残余。P8 只在 Code A 已提交终局后结算 P6 玩家图并丢弃 P9、P11、P14 残余。不存在 A／B 镜像、同步或双写。
3. P11 只定义一个正式生产身体来源：CodeB.BodyContainer.BasicCorpse，绑定 M01.Encounter.LOW.Skirmisher.01、M01.Spawn.LOW.Skirmisher.01、ordinal 0 与 M01.BodyTarget.LOW.Skirmisher.01.Ordinal.0。其 BodyTargetId 与 DeathReceiptId 均由既有稳定路线导出。P21 不增加第二尸体、第二敌人、随机 Encounter 或全量敌人迁移。
4. P12 已将上述已提交死亡回执后的 P11 record 接入既有生产尸体交互、读条、Hidden → Searching → Revealed 状态机、页面生命周期和真实 NativeOnDrop 写入链。P21 只能在该既有 BodyContainerTarget 中增加 Code B 的尸体装备位投影；不得重做尸体 Actor、交互距离、读条、输入、状态机或普通物品的既有路径。
5. BasicCorpse.r1 与 BasicCorpse.r2 都是历史 Profile。任何已经 materialized 的 P11 record 的 ItemId、ContainerId、数量、visibility、action state、DeathReceipt、ProfileId、ProfileVersion、AlgorithmVersion、digest 与完整图均不得重掷、补料、迁移或改写。只有同一死亡回执尚未 materialized 的未来 BasicCorpse 才可使用本轮 r3。
6. P20 的 BasicCorpse.r2 保留 Guaranteed.Main：IronShard weight 3、quantity 1–2；SpiritDust weight 2、quantity 1；选择次数为 1。它还保留 Optional.SpatialUtility：NoDrop:9 / Spawn:1；命中后在 WindTalisman weight 1 与 BackpackLevel1 weight 3 中恰选一个空的正式空间 parent graph。P21 不改写 r2 或既有空间图；r3 只在未来尸体上原样继承这两组，再增加受限的尸体装备位组。
7. P1/P4x/P7 已有玩家兵器、道袍、饰品和空间戒指装备槽，以及真实 Drop 唯一位置写入规则。装备本体不可堆叠；普通储物位置不按类型互斥；仅明确装备栏执行类型与单槽互斥。P21 不改变玩家装备语义、P7 个人背包结构或 P4x 现有 Move、Swap、Merge、Split、Equip、Unequip 事务。
8. P17/P18/P19/P20 的空间 parent／ChildContainer 图、一层限制、P13 绑定、P14/P19 地面路径、P15 RestoreHealth、P5↔P6 bridge、P8 terminal settlement 与 P9/P10 普通容器均已存在并保持不变。尸体普通装备不是空间 parent，不得拥有 ChildContainerId，也不得借本任务取得任何空间、快捷、使用或战斗语义。

## 3. P21 产品裁决

P21 的单一纵向切片是：未来第一次因同一已提交 BasicCorpse 死亡回执而 materialize 的 P11 record，除保留 r2 的主材料和可选空间来源外，拥有固定的尸体兵器、道袍、普通饰品三个装备位。一次确定性 Optional.EquippedLoadout roll 至多填充其中一个装备位；该物品始终是当前正式 Catalog 中已经存在、可装备、非空间、无 ChildContainerId 的真实 Definition。它在被 P12 正常揭示前不可见、不可写；揭示后只能从该尸体装备位通过既有 NativeOnDrop 真实拖到空 P6 BaseQuick，再由玩家沿 P7 已有拖拽自行装备。

| 项目 | 本轮固定裁决 |
| --- | --- |
| 影响范围 | 仅未来未物质化的 CodeB.BodyContainer.BasicCorpse；不增加第二尸体、第二敌人、BasicCache 或新的地图来源。 |
| 新 Profile | CodeB.LootProfile.BasicCorpse.r3；AlgorithmVersion 固定为 CodeB.DeterministicWeightedLoot.Crc32.r3。r1/r2 与所有 materialized P11 history 保持原样。 |
| 尸体装备位 | Weapon、ArmorRobe、Accessory0 各一个固定、0–1 的 P11-owned equipment container；ArmorRobe 只是“当前正式 Armor/道袍槽”的产品标签，实际 enum／slot semantic 必须复用现有 Catalog。稳定 ContainerId 只能从 BodyTargetId、source DefinitionId 与 slot semantic 导出。 |
| Optional.EquippedLoadout | NoDrop:4 / Spawn:1 gate；命中后恰选一个候选，Weapon、ArmorRobe、Accessory0 的权重各为 1。候选排序由稳定 DefinitionId 的字节序确定。 |
| 候选要求 | 仅使用当前正式 Code B Definition Catalog 已存在的、不可堆叠、quantity=1、无 ChildContainerId、分别兼容玩家 Weapon、当前正式 Armor/道袍、Accessory 槽的三个 canonical DefinitionId。Accessory 候选必须排除 SpatialRing、WindTalisman 与所有带空间语义的定义。 |
| 取得 | 仅复用 P12 已揭示尸体装备 cell → 真实 NativeOnDrop → 空 P6 BaseQuick。不得从尸体直接装备到 P6 装备栏，不得按钮领取、双击、右键、QuickMove、自动装备、Actor direct pickup 或 Widget direct write。 |
| 反向移动 | P21 不允许任何玩家装备或普通物品从 P6 回写到 BasicCorpse 的任一尸体装备位。P12 既有 P6→P11 simple root 路径的产品语义不改写。 |

若当前正式 Catalog 无法为 Weapon、ArmorRobe、Accessory 三类各解析一个同时满足上述要求的 canonical DefinitionId，或者已有 P1 slot semantic 无法表达其固定尸体装备位，停止受影响部分并使用 NEEDS_PLANNER_DECISION / BLOCKED。不得新增 starter、临时定义、别名、fixture、假 ItemId、UI 显示名映射或 Code A 旧库存作为替代。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

在 Code B 中把未来首次 materialize 的唯一 BasicCorpse 升级为 r3：建立同一 P11 P1 graph 中固定的尸体装备位和一次性确定性的最多一件装备来源，并最小接入 P12 现有尸体投影及 P11 → P6 Drop 候选事务。实现不得改写 P12 既有尸体交互，不得让 Code A 取得库存权威，也不得让装备物品直接跨越 P6 BaseQuick 进入玩家装备栏。

### 4.1 r3 Profile、正式 Definition 解析与尸体装备图

1. 保留 BasicCorpse.r1/r2 的兼容读取、历史验证和所有 materialized P11 record。只有尚未 materialized、且已经通过既有 P11 death-receipt gate 的 BasicCorpse 才选择 CodeB.LootProfile.BasicCorpse.r3。BasicCache.r1/r2、BasicCorpse.r1/r2 及其历史 P9/P11 record 一律不改写。
2. r3 必须逐字节保持 r2 的 Guaranteed.Main 与 Optional.SpatialUtility 定义、候选、权重、quantity、group order、gate 语义和既有稳定结果。新增 Optional.EquippedLoadout 时，ProfileId、ProfileVersion、AlgorithmVersion、组顺序、NoDrop/Spawn weights、canonical DefinitionIds、candidate weights 与 ProfileDigest 都必须是明确、可审计的内容定义。
3. 首先从当前正式 Code B Definition Catalog 解析并记录三个 canonical DefinitionId：一个 Weapon、一个 ArmorRobe、一个普通 Accessory。每个候选必须已有正式 DefinitionId、非 stackable、MaxStack=1、quantity=1、无空间 semantic、无 ChildContainerId、能由 P1/P4x slot compatibility 验证放入其对应玩家装备槽。不得从显示名、图标、Actor、Widget、fixture、旧 Code A ItemId 或随机结果猜测候选。
4. r3 的完整 deterministic identity 至少包含 OwnerId、RunInstanceId、BodyTargetId、BasicCorpse source DefinitionId、DeathReceiptId、LootProfileId、ProfileVersion、ProfileDigest、AlgorithmVersion、所有三条 canonical DefinitionId 与其 candidate-set digest。相同精确 identity 必须复建相同的主材料、空间 group、equipment gate、候选、尸体 slot、stable ItemId、ContainerId 与 result digest；不得读取时钟、帧号、Actor 指针、世界坐标、UI、临时 GUID、旧 Code A Loot、网络状态或全局可变 RNG。
5. 每个 r3 candidate graph 必须先建立固定尸体装备位：Body.Weapon、Body.ArmorRobe、Body.Accessory0，各自容量为 1，且都属于同一 P11 record。Optional.EquippedLoadout 未命中时三者保持空位；命中时只将一个新建的 simple equipment item 放入对应 slot。该 item 必须无 ChildContainerId、无子图、无空间 item、quantity=1、唯一 parent、合法 Definition 和合法 slot。
6. P16/P17/P1 与 P11 在保存前联合验证：r2 继承 groups、r3 group order、候选 Definition、weight、quantity、固定排序、尸体装备 slot provenance、根容器容量、unique owner、无自引用、无环、无 duplicate child owner、无 nested spatial item、无非法 slot、无 duplicate item/container identity。任何候选不合法、装备位不能完整建立、Definition 缺失、optional 配置错误、容量不足、receipt 冲突或 save failure，均必须拒绝整次首次 materialization，零写入 P11/P6/P5/P8，并且不得降级为 root material、simple substitute、starter、fallback recipe 或直接 P6 grant。
7. 成功 materialization 必须在一个 Owner durable replacement 中共同保存：完整 P11 P1 graph、原有 root items、三个尸体装备位、所有初始 item 的 Hidden state、materialization/death receipt、r3 ProfileId/Version、AlgorithmVersion、ProfileDigest、candidate-set digest 与 result digest。不得先保存空装备位、seed、gate、receipt 或部分图，再补写物品。
8. 同一 corpse 的 Open/Close、搜索中断、Actor 销毁、query、P6 recovery/rebind、保存冲突或重复 death receipt 只能读取已保存 r3 结果或零写入拒绝；不得第二次 gate、reroll、追加装备、替换 slot item、重写稳定 ID 或创建第二个尸体 root。

### 4.2 P12 尸体装备投影与完整取得链

1. 不改动 P12 的目标身份、打开／读条／揭示状态机、输入、尸体 Actor、页面生命周期、普通 root-item Move/Merge/Swap 规则或 NativeOnDrop 入口。仅为 r3 BasicCorpse 的同一 BodyContainerTarget 增加三个只读尸体装备位投影：Weapon、ArmorRobe、Accessory0。
2. 在 Hidden 或 Searching 阶段，尸体装备位只能显示未揭示状态，不得携带 ItemId payload、详情、拖拽、选择或写入资格。进入既有 Revealed 阶段后，每个 slot 从权威 P11 graph 显示为空或一个真实 root equipment item；不得创建独立尸体 inventory、Widget array、显示名替代 ItemId 或第二个 materialization 来源。
3. P21 唯一合法取得路径是当前精确 OwnerId + RunInstanceId P6 的一个空 BaseQuick cell。P1/P11/P12 必须验证活动 session、BodyTargetId、DeathReceipt、source/destination revisions、Reveal/Open gate、source slot semantic、P6 BaseQuick 空位、item Definition、item simple shape 与 graph closure，再由现有生产 NativeOnDrop 发起请求。
4. 成功路径必须在同一 Owner durable replacement 中移动同一 ItemId：该 item 从对应 P11 尸体装备位移除，P6 出现同一 ItemId、Definition、quantity、parent placement，P6 revision/digest 与 P11 receipt/digest 仅在该提交中前进。不得 clone、new ItemId、flatten、partial commit、P12 Widget inventory、direct equip、auto-equip、Merge、Swap、Split、QuickMove 或 player-side direct write。
5. 任何 source、visibility、slot semantic、definition、target、revision、session、Prepared／terminal gate、冲突或保存失败必须在 durable replacement 前拒绝，并保持 P11、P6、P7、P13、P14、P15、P19 与 P8 不变。失败不得改变 Code A Actor，也不得创建或销毁 Code A inventory。
6. P12 的既有 P6 → P11 simple root-item 行为不改变；本轮必须明确拒绝把任何物品、尤其 Weapon、ArmorRobe、Accessory、Spatial parent 或其 child contents，从 P6 写入 P11 的 Body.Weapon、Body.ArmorRobe、Body.Accessory0。不得以“对称”为由将玩家装备注入尸体。
7. 成功后的 P7 只从新的权威 P6 projection 读取该 item。玩家若要装备它，只能沿 P7/P4x 已有的真实 P6 BaseQuick → 明确玩家装备栏 Drop 提交；P21 不新增装备按钮、自动装备、属性、战斗、动画、HUD 或 Code A loadout mirror。

### 4.3 生命周期、终局与既有系统保护

1. 进入 P6 的尸体装备只依既有 P5 ↔ P6 bridge、P6 recovery/rebind、P8 terminal settlement 与 P7/P4x 玩家装备事务处理。P21 不创建 P5 source、direct grant、特殊 save、Code A mirror、P14 world record 或新的装备持久化模型。
2. P8 的 Code A 后置 terminal authority、Extracted 的完整 P6 player graph 返回 P5、Dead/RecoveredAbandon 的没收、及 P11 残余丢弃时序均不变。仍在 BasicCorpse 尸体装备位中的 item 只能作为 P11 residual 被完整丢弃；已进入 P6 的 item 只按既有 P8 结算。
3. P13 不自动为尸体装备创建 1—9 binding；P15 继续只允许 P6 BaseQuick 的 simple RestoreHealth。P14 simple branch 继续拒绝装备，P19 只处理既有两种空间 parent，P17/P18/P20 的空间语义、P9/P10 的普通容器及 P11/P12 的其他来源／交互都保持不变。
4. Code A 继续只在既有已提交死亡与尸体交互边缘转发，不得读取／缓存 P11 inventory、equipment roll、P6 target、receipt 或终局结果。不得新增 Code A Save、旧 Loot 连接、地图 Actor 装备物品、掉落、拾取、玩家库存镜像或战斗属性。

## 5. 允许的改动范围

允许：

- 在 Code B 内最小扩展 P16 Loot Profile Catalog、deterministic roll/digest、P11 BasicCorpse 首次 graph materialization、固定尸体装备位和 r1/r2/r3 历史兼容；
- 在 Code B P1/P12 的既有候选事务中最小接入 P11尸体装备位 → P6 空 BaseQuick 的同一 Owner durable transaction、验证与 transient stale cleanup；
- 仅在 P12 Code B presenter/UMG projection 中增加尸体装备位的读取、Hidden/Searching/ Revealed 投影与既有 NativeOnDrop source adaptation；不得改变其页面结构、读条、地图 actor 或 Code A 权威；
- 为 schema、序列化、调用签名或编译兼容最小调整 P7/P8/P13/P14/P15/P17/P18/P19 的 Code B 声明或无效引用清理，但不得改变其产品语义；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档和本任务 Report。

## 6. 明确不在本任务内

不得实现、启动、重构或接管：

- 第二尸体、第二敌人、全量尸体／地图容器迁移、BasicCache 改写、随机 Encounter、资源／灵石、P5 starter／初始库存、直接 P6 grant、地图 Actor 新来源、自动拾取、直接 Actor pickup、Code A 旧 Loot、地面多物品、child 地面操作、嵌套袋、空间道具预装内容或任何新 WorldDrop；
- P12 的读条、搜索状态机、普通 root-item 可见布局、普通物品 Move/Merge/Swap、Actor、target identity 或现有 P6 → P11 simple root 路径；尸体装备直接装备、按钮／快捷领取、自动装备、child 操作或玩家 item 回存尸体；
- P13 新快捷栏语义、P14 新地面来源、P15 之外的使用效果、空间装备效果、武器／道袍／饰品数值、攻击、护甲、生命、Buff/Debuff、技能、动画、音效、HUD、战斗属性、网络同步或多人；
- Code A 的地图、Actor、输入、HUD、Player Actor、生命、战斗、死亡、Run、Run Save、终局分类、旧库存、旧 Loot／搜索或正式结算权威；
- 产品启动、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 r3 只影响未来未 materialized BasicCorpse；BasicCorpse.r1/r2、BasicCache.r1/r2 与任何 materialized P9/P11 history 均保持原样。审查 r2 inherited groups、Optional.EquippedLoadout gate、canonical DefinitionId、weight、quantity、排序、ProfileVersion、AlgorithmVersion 与 digest。
2. 审查正式 Definition 解析与 deterministic identity：三个候选均来自当前 Catalog 的稳定 ID、对应真实装备 slot 且非空间；相同精确 death-receipt identity 绝不会因时钟、Actor、UI、临时 GUID、旧 Code A Loot 或全局 RNG 重掷、追加尸体装备或创建第二套 slot graph。
3. 审查 P11 首次图：Weapon、ArmorRobe、Accessory0 的固定 slot container 属于同一 corpse graph，容量与 slot provenance 正确，最多一件装备、无环、无 duplicate owner、无 child graph、无非法 slot、失败零写入均成立。
4. 审查 P12/P1 transaction：只有既有已揭示尸体装备 source 的真实 NativeOnDrop 到空 P6 BaseQuick 可以移动同一 ItemId；不存在 clone、new ItemId、direct P6 write、P12 Widget inventory、partial commit、direct equip、自动装备或复杂图回存尸体。
5. 审查 P5 → P6 → P8、P7 player equip、P13/P15 eligibility、P14/P19 world behavior、P9/P11 residual discard 与 Code A authority；确认本轮不会导致尸体装备逃逸、P5 starter、第二来源、地面路径、空间语义改变或 Code A inventory／combat authority。
6. 审查 Code A diff。除纯声明／编译兼容外，预期没有 Code A 功能改动；若存在，必须逐文件说明其不涉及地图、Actor、Loot、搜索、输入、HUD、战斗、生命、Run、Run Save、结算或库存权威。
7. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

8. 若编译失败，只修正 P21 引入的局部 Profile schema、deterministic roll、P11 corpse equipment graph、P1/P12 transaction、序列化、include、projection declaration 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要扩展到 P22、0.0.9B.F、Code A 权威或其他功能，停止受影响部分并报告。

## 8. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P21.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增／修改／未修改的每个文件及职责；
2. BasicCorpse.r3 与 r1/r2/BasicCache.r1/r2 的版本关系，r2 inherited groups、Optional.EquippedLoadout gate、三个 canonical DefinitionId、weight、quantity、排序、AlgorithmVersion、candidate-set digest 与 ProfileDigest；
3. r3 的完整 deterministic identity 输入，以及为何相同未 materialized corpse/death receipt 不会重掷、追加装备或创建第二尸体装备图；
4. P11 的三个尸体装备位、容量／slot provenance、最多一件装备、同一 Owner durable save、receipt/digest，以及 legacy materialized record 如何保持原样；
5. P12 的既有 NativeOnDrop 如何只允许已揭示尸体装备 item 先进入空 P6 BaseQuick，成功／失败／冲突／重复路径如何避免 clone、new ItemId、partial commit、direct equip、自动装备或玩家装备回存尸体；
6. P7、P5/P6/P8、P13/P14/P15/P17/P18/P19、P9/P10、P11/P12 与 Code A authority 的静态边界结论；
7. 实际 Editor 编译命令、目标、最终 native exit code 与关键结果；
8. 明确列出未执行的 F 阶段项目：真实 BasicCorpse r3 roll、命中／未命中、死亡／开尸／揭示、P12 装备转移、P7 装备／卸下、P8 三种终局、recovery、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证仍由 0.0.9B.F 负责；
9. 明确列出尚未启动的功能：第二来源／全量迁移、灵石／资源领取、空间道具预装内容、child 地面操作、嵌套袋、空间装备效果、武器／道袍／饰品数值与战斗效果、其他消耗品及后续 P 阶段。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code 0 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P21_COMPILE_REWORK

若缺失三个正式装备 Definition／兼容槽、既有 BasicCorpse/P11/P12 无法在不改写 materialized history、不扩展 Code A 权威、不创建第二来源或不改变 P12 尸体交互产品语义的前提下形成合法尸体装备图，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P22、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P21.0.r0","file":"Dev.D.UE.0.0.9B.P21.0.r0_report.md"}

