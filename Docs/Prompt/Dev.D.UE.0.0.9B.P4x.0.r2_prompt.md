# Dev.D.UE.0.0.9B.P4x.0.r2

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P4x——去备战启动门槛、拖拽优先物品规则与双空间容器修正
- 任务编号：Dev.D.UE.0.0.9B.P4x.0.r2
- 任务性质：P4x 同方案的验收证据补齐与版本交接修正；不是 P5，也不重新设计已实现的功能。
- 执行文件：Dev.D.UE.0.0.9B.P4x.0.r2_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P4x.0.r2_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：Dev.D.UE.0.0.9B.P4.0.r1

---

# 上半部分：只读决策、现状与边界

## 1. 当前证据状态与本任务目的

`Dev.D.UE.0.0.9B.P4x.0.r0_report.md` 记录了一组有价值的候选实现与测试结果：去备战 Start Run、拖拽唯一写入入口、条件式装备／卸下、空间戒指快捷空间、空间储物囊非快捷空间、双分辨率真实 Slate/UMG trace、构建和回归。

但 r0 不能作为 P4x 的正式放行记录，原因是：

1. 在 P4.0.r1 验收后，P4x.0.r1 已将 P4x 的最终验收口径明确为“双击／右键真实命中但零写入”，而 r0 Report 仍使用 r0 身份、r0 Prompt 和 r0 日志命名；不得把它事后改名为 r1 或假称 r0 未执行。
2. r0 Prompt 与后续明确的验收口径均要求当前 P4x 版本的可见 Smoke／截图；r0 Report 明确表示现有截图早于 P4x，并且本次未取得新的可见 UI 证据。旧截图不能代表当前页面。
3. r0 Report 的单一 Direct Start 测试对若干无效旧数据给出组合证据，但未将“正常、无装备、空／缺失旧备战、失效旧空间引用、返回后再次 Start Run”逐例以实际产品 Start Run 入口展开，也没有 P4x 专用的零写入 trace 命令与完整字段。

因此：

- r0 的源码、构建和日志只能作为只读候选证据；不得回滚、删除、重命名或改写它们；
- r0 不是已接受任务，也不是失败后应盲目推倒重做的工作；
- 本 r2 是唯一活动的 P4x 纠正任务。若当前实现已符合本 Prompt，工作应以验证、补证与最小修正为主；
- 不得自动开始 P5 或任何后续阶段。

## 2. 不变的产品裁决

### 2.1 去备战化的正式开局

- 局外仓库／人物整理仍可作为独立物品管理页面或 Code B 开发态页面存在；
- 它不再是 Start Run 的阶段、门槛、初始化器、校验来源或有效性判定来源；
- 正常 Start Run、无装备、空／缺失旧备战布局、失效旧武器／空间戒指引用均必须沿既有产品地图／Run 链启动；
- Start Run 不得为物品系统创建、恢复、重置、校验或依赖旧备战数据，也不得创建 Code B Host、Fixture、Repository、页面或拖拽对象；
- 不得通过吞掉错误、伪造装备、清空 Profile、跳过地图加载或改由 Code B 接管正式 Profile／Run／SaveGame 来掩盖问题。

### 2.2 拖拽是位置改变的唯一用户入口

- 仓库、基础 6 格、已装备空间戒指的快捷空间内部格、空间储物囊的非快捷内部格都是普通储物位置；它们不因物品类别互斥；
- 普通位置之间的 Move、Swap、兼容堆叠 Merge、Replacement，以及从装备栏移出的 Unequip，均只能在真实 DragOperation 的明确 Drop 目标上提交；
- 仅装备栏位保留类别与单槽互斥：兵器、道袍／护甲、饰品 I／II、空间戒指；兼容 Replacement／Swap 必须是一次 P2/P1 原子事务；
- 不得存在可达的 Move、Merge、Swap、QuickMove、放回、双击、右键、详情按钮、菜单项、热键或隐藏测试命令的非拖拽位置写入入口；
- Split 如保留，只能是明确数量拆分；拆分后的地点仍由真实 Drop 决定。

### 2.3 条件式装备／卸下

- 未处在任何玩家装备栏的兼容可装备物品：只显示“装备”，不显示“卸下”；
- 已处在玩家装备栏的物品：只显示“卸下”，不显示“装备”；
- 不可装备物品：两个动作都不显示；
- 多个候选饰品栏、已占用装备栏或目标不唯一时，界面只能引导用户拖到明确装备栏，不能猜测目标或自动替换；
- “卸下”不是自动放回。它只提示用户把物品拖至明确合法普通目标；无目标、取消或无效 Drop 都不得写入。

### 2.4 两类空间容器

| 类型 | 位置与可见性 | 容器语义 |
|---|---|---|
| 空间戒指 | 只可进入空间戒指装备栏；未装备时不显示内部 | 成功装备后显示其稳定 Child ContainerId 对应的“快捷空间（空间戒指）”；内部格只是普通储物格，不绑定 1—9、不实现使用或正式运行时接管 |
| 空间储物囊 | 可处在任意普通储物位置 | 打开自己的稳定 Child ContainerId，明确标为“非快捷储物囊”；不得带有快捷标签、1—9、QuickMove 或使用语义 |

每个实例及其内部物品必须继续只有一个 P1 ItemId、一个父位置与稳定 Child ContainerId。空容器本体可以拖动；带内容的戒指或储物囊整体 Move／Swap／Equip／Unequip 必须以 `LoadedSpatialItemMoveUnsupported` 原子拒绝，且 Revision、位置和内部物品不变。

## 3. A/B 权威和改动边界

- 0.0.9-XFix1 是合法工程基线；代码 A 仍负责当前地图、怪物、战斗、默认 UI、正式玩家、Profile、Run、旧背包、Loot、结算与持久化；
- Code B 仍仅位于 `Source\demo_map\CodeB`，默认关闭、显式打开的开发态纵向切片；它尚未接管正式 Profile、Run、SaveGame、Loot、地图、玩家 Actor 或默认 UI；
- Code B 唯一写入链保持：真实 UMG／Slate 输入 → P4 Interaction Controller → P3 UI Controller → P2 Application Service → P1 Repository；
- 允许保留 r0 所做、为去备战 Start Run 所必需的精确 Code A 修改；不得修改战斗、地图、奖励价值、Loot、SaveGame Schema 或无关正式 UI；
- 不得建立 A/B 同步、镜像、双写、迁移或由 Code B 初始化正式 Run。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

以 r0 当前工作树为候选实现，完成可审计的 P4x 放行闭环：

1. 对去备战的实际产品 Start Run 链补齐逐例、可复现的验证；
2. 以 P4x 命名的生产真实输入 harness 重新证明拖拽唯一写入和双击／右键零写入；
3. 生成当前构建版本的可见 UI Smoke 证据；
4. 仅在验证暴露真实不符合项时作最小修正；
5. 以准确的 `P4x.0.r2` 任务身份、来源审计和 Report 完成交接。

不得仅修改文件名、日志名或 Report 文字来宣称满足这些要求。

## 5. 执行要求

### 5.1 逐例产品 Start Run 验证

从用户实际触发的产品顶层 Start Run 入口开始，记录到地图 Browse／激活、玩家生成或既有会话正常结束的链路。允许测试为隔离 Profile 提供受控数据，但不得绕过顶层产品入口，直接调用 `StartRunWithoutPreparation` 或只测试低层 Transaction 代替产品链。

至少分别覆盖并以稳定测试名、日志段落和断言区分：

1. 正常 Profile 的 Start Run；
2. 无任何装备的 Profile 的 Start Run；
3. 空、缺失或从未构造的旧 `PreparationLayout` 的 Start Run；
4. 含失效旧武器 ID、失效旧空间戒指 ID 或非标准旧 Hotbar 数据的 Start Run；
5. 上述任一路径 Extraction／返回后再次 Start Run，证明不会重新引入备战初始化门槛。

每一例必须证明：不创建 Preparation Widget，不读取或提交旧布局来决定开局，不初始化 Code B，并且既有地图／Run 流正常完成。日志应将 Direct 路径与历史显式 Prepared Run／布局提交路径明确区分；不得删除历史显式路径来伪造结果。

### 5.2 P4x 专用真实输入 harness

新增或适配独立入口：

    CodeB.P4x.RunRealInputTrace [expected width] [expected height] [Quit]

它必须打开已挂载的生产 P3/P4 Host，以生产 Cell `GetCachedGeometry()` 或等价真实坐标、Slate hit-test 和真实鼠标／键盘事件完成验证。不得直调 Widget `NativeOn...`、P3/P4 Controller、P2 Service 或 P1 Repository；只能读取权威 Projection 用于断言。

在 1280×720 与 1920×1080 各完整运行一次。两份 P4x trace 均必须逐行记录至少：Gesture、实际命中 Cell／Container／Slot、ItemId、DragOperation、Preview、P2 CommandCount、P2 CallCount、RevisionBefore／After、接受或拒绝结果。

每档分辨率均必须证明：

1. 仓库、基础 6 格、快捷空间与非快捷储物囊之间的真实 Drop Move；
2. 一次兼容堆叠 Merge，含数量、源／目标 ItemId、Revision、详情与 Projection 断言；
3. 兼容 Equip、已占用兼容装备栏的原子 Replacement／Swap，以及向明确普通目标的 Unequip；
4. 不兼容装备栏、Loaded Spatial、Stale Revision、同源、容量不足或 UI 外部 Drop 中至少一类无写入拒绝；
5. 空间戒指装备前没有快捷空间、装备后显示正确快捷空间；储物囊打开后显示非快捷内部格，并能和普通位置拖拽往返；
6. 空容器可以拖动，带内容的 Ring/Pouch 整体拖动被原子拒绝；
7. Esc、Cancel、Close/Reopen、禁用 1—9 占位和无 ItemId 格不写入且清理临时状态；
8. 真实 DoubleClick 与真实 RightClick 都抵达生产页面，但对每一种都断言 `P2 CommandCount=0`、`P2 CallCount=0`、`RevisionBefore=RevisionAfter`、ItemId 位置／父容器／数量不变；
9. 每个被接受的物理拖拽只提交一次 P2/P1 事务与一次 Revision 递增。

最终静态审计还必须给出命令和输出摘要，证明：

- 生产 UI 可达的 Move／Merge／Swap／QuickMove／放回写入口为 0；
- 生产 UI 内不存在 `BeginOperation` 或等价位置写入直调；
- `CommitDrop` 的生产位置提交只来自已挂载 Cell 的真实 `NativeOnDrop`；
- 右键、双击、详情按钮和菜单没有等价隐藏写入口；
- 普通储物格不按类型拒绝，只有装备栏实施类型／单槽互斥。

### 5.3 当前可见 Smoke 证据

必须在本任务开始后的最终构建上生成新的、带任务 ID 的可见证据。不得引用、复制、改时间戳或重新标注任何旧 `Saved\P4Screenshots` 资产；也不得使用直调 Controller／P2／P1 的截图命令制造结果。

在 `Saved\P4xScreenshots` 或同等明确目录保存带分辨率和任务 ID 的 PNG，并在 Report 中列出绝对或活动工程相对路径、生成时间、分辨率和画面说明。至少包括：

1. 无需备战即可通过实际产品 Start Run 的当前证据；
2. 初始 Code B 页面，同时可见空间戒指与非快捷储物囊；
3. 戒指装备后的“快捷空间（空间戒指）”；
4. 打开后的“非快捷储物囊”；
5. 真实 Drop 的普通 Merge，以及装备栏拖拽高亮或成功；
6. 不兼容装备栏或 Loaded Spatial 的中文拒绝；
7. 未装备／已装备／不可装备三种详情状态的装备／卸下条件显示；
8. DoubleClick／RightClick 事件到达但零写入的可追溯 trace 片段；
9. 1280×720 与 1920×1080 的真实拖拽页面与 trace 结果。

截图只证明可见层，不能替代第 5.1 和 5.2 节的真实输入与权威断言；反之，trace 也不能替代当前截图。

### 5.4 回归、构建、基线审计与资料

至少完成：

- `demo_map.CodeB.P1`、`P2`、`P3`、经 P4x 规则更新的 `P4`、全部新增 P4x 自动化；
- 第 5.1 节的产品 Start Run 测试；
- 受影响的 `ProfileSettlement`、`ProfileSession` 和 `ProfileNormalStartup` 回归；
- `demo_mapEditor Win64 Development` 与 `demo_map Win64 Development` 构建；
- `/Game/M01/Maps/L_M01_Expedition?Name=Player` 默认地图、未显式 Open Code B Host 的 Smoke；
- 两档 P4x 真实输入 trace；
- 每份最终日志独立检索 Fatal、crash、ensure、assert、Automation Controller error、`P4X_*Error` 与 trace failure。

对活动工程和 `0.0.9-XFix1` 的 `Source\demo_map` 做逐文件 SHA-256 或同等内容清单审计。Report 必须列出：

- Code B 的新增／修改文件；
- r0 已存在且仍保留的 4 个已登记 Code A 修复；
- 本 P4x 去备战化所必需的精确 Code A 文件和每个文件的原因；
- 任何意外差异。出现来源不明差异时停止受影响工作，不得自行吸收、清理或回滚。

更新 `PROJECT.md`、`PROJECT_INFO_CARD.md` 或等价项目资料，真实记录：r0 是未接受的候选证据，r2 完成 P4x 的当前验证；QuickMove 用户入口已废止；去备战的实际入口；两类空间容器语义；测试／trace／截图／构建路径；以及 Code B 尚未接管正式运行时。不得写入“r0 未执行”或“r0 已验收”。

## 6. 停止条件

立即停止受影响工作并生成同名 r2 Report，且不得使用 READY 状态，若发生任一情况：

- Start Run 仍经过旧备战初始化、校验或数据门槛，或去除它必须让 Code B 接管正式 Profile／Run／SaveGame；
- 找到任何非拖拽可达的 Move／Merge／Swap／QuickMove／放回／自动装备或自动卸下写入口；
- 普通储物发生类型互斥，或装备栏无法正确拒绝不兼容类型；
- 装备／卸下显示不能由真实位置决定，或卸下猜测目标自动放回；
- 空间戒指／储物囊无法维持唯一实例、唯一父位置、Child ContainerId、Revision 或 Loaded Spatial 原子性；
- P4x harness、实际产品 Start Run、当前可见 Smoke、构建、默认地图 Smoke 或必需回归失败／缺失；
- 真实输入只能靠直调 Widget／Controller／P2／P1、假页面、本地数组或截图证明；
- 出现来源不明修改、Fatal、crash、ensure、assert、Automation Controller error 或本任务新增 Error。

## 7. 验收与交接

以下全部满足才可通过：

1. P4.0.r1 的真实拖拽基础不回归；
2. 五类实际产品 Start Run 场景逐例通过，且没有 Code B 初始化或旧备战门槛；
3. 普通储物只由真实拖拽改变位置，装备栏是唯一类型／单槽互斥位置；
4. 所有非拖拽位置写入口不存在，真实双击／右键事件抵达但 `P2 CommandCount=0`、`P2 CallCount=0`、Revision 和物品真值不变；
5. 条件式装备／卸下与明确目标 Unequip 正确；
6. 空间戒指快捷空间和空间储物囊非快捷储物正确、可区分、可拖拽且不复制内部物品；
7. Loaded Spatial、取消、失败、生命周期和一次手势一次提交正确；
8. 自动化、P4x trace、Profile 回归、构建、默认地图和 Start Run Smoke 全部通过；
9. 当前版本的所有可见 Smoke 证据完整且无旧截图冒充；
10. 源码审计、项目资料、日志、trace、截图和同名 Report 完整且身份真实。

通过时最终 Report 只能使用：

- READY_FOR_CODE_B_OUT_OF_RAID_ORGANIZATION
- READY_FOR_CODE_B_OUT_OF_RAID_ORGANIZATION_WITH_NONBLOCKING_FINDINGS

否则只能使用：

- NEEDS_P4_REWORK
- NEEDS_PLANNER_DECISION
- BLOCKED

生成 `Dev.D.UE.0.0.9B.P4x.0.r2_report.md`，存入：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

本 Prompt 归档到：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Prompt

完成后不要自动开始 P5 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P4x.0.r2","file":"Dev.D.UE.0.0.9B.P4x.0.r2_report.md"}

若 ProjectCode、任务编号、Prompt、Report、活动工程、前置 P4.0.r1 状态或目标 Chat 无法对应，停止受影响任务，按 IPF 协议生成 Error001 Report，不自行猜测、改号或切换项目。
