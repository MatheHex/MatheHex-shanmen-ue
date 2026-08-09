# Dev.D.UE.0.0.9B.P19.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P19——空间道具完整图的局内地面丢弃／拾回
- 任务编号：Dev.D.UE.0.0.9B.P19.0.r0
- 任务性质：在 P14 已建立“简单 BaseQuick 物品 → 单一 P6 WorldDrop → 空 BaseQuick 拾回”的真实纵向路径，P17 已建立空间戒指／储物囊的正式 ChildContainer graph，且 P18 已让 `WindTalisman` 与 `BackpackLevel1` 能通过 BasicCache 真实进入 P6 后，最小扩展 P14，使这两种已在 P6 的空间道具及其内部内容可作为一个不可拆分的完整图主动丢到地面并以真实拖拽完整拾回。P19 只扩展既有 P14 world-record、P1 图事务、P7/P3/P4 的真实 Drag／Drop 与既有投影；不创造新来源、随机地面 Loot、子物品地面操作、地图拾取、装备效果或任何 F 阶段验证。
- 执行文件：Dev.D.UE.0.0.9B.P19.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P19.0.r0_report.md
- 活动工程根：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B
- 活动工程：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject
- 引擎：C:\\Program Files\\Epic Games\\UE_5.8
- 工程级开发基线：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r1、P10.0.r0、P11.0.r0、P12.0.r0、P13.0.r0、P14.0.r0、P15.0.r0、P16.0.r0、P17.0.r0 与 P18.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P19 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、进程检查、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 持续有效的真值、来源与边界

1. Code A 继续拥有默认地图、地图 Actor 的即时表现与生命周期、交互距离、原始输入派发、Player Actor、战斗、生命、HUD、正式 Run 生命周期、终局分类、Run Save、旧库存、旧 Loot／搜索及所有尚未迁移运行时。P19 对 Code A 的允许改动仅限 P14 已有 `WorldDropActor`／placement／投影／交互转发的最小 graph-aware 适配；Code A 不得保存、物质化、推断、复制、拆分、装备、拾取或结算 Code B 物品图。
2. Code B P1 Repository 是新物品体系唯一可变真值。P5 是局外 Profile snapshot；P6 是精确 `OwnerId + RunInstanceId` 的活动 Run 玩家与 WorldDrop snapshot；P9 BasicCache 与 P11 BasicCorpse 是独立 Run-local 残余。P8 只在 Code A 已提交终局后结算 P6 玩家图并丢弃 P9／P11／P14 残余。不存在 A／B 镜像、同步或双写。
3. P14 的 P6 schema `4`、确定性 `WorldDropId`、派生 world-container、一个 root `ItemId`、地图落点和 `Available` state 已是唯一地面记录模型。它当前简单物品路径必须保持原样可读、可写、可拒绝和可结算。P19 不另建空间道具世界库存、第二个 `WorldDrops` 集合、child-item 列表、平行 graph save、Widget inventory 或 Code A Save；P14 record 的 root `ItemId` 继续是完整图唯一入口。
4. P17 已确定两种正式空间父项：`Prototype.Item.Accessory.WindTalisman`（`Fdemo_mapItemIds::WindTalisman`，`Prototype.Slot.SpatialRing`，`QuickRing` child layout）与 `Prototype.Item.Backpack.Level1`（`Fdemo_mapItemIds::BackpackLevel1`，`Prototype.Slot.Backpack`，`StoragePouch` child layout）。稳定 `SpatialChildGuid(ItemId)`、child type、容量、slot legality、definition provenance 和“一层空间道具”限制继续只能由 P17 的正式 Definition Catalog／canonical resolver 导出；不得由显示名、Actor、Widget、fixture、地图坐标或 P16 seed 猜测。
5. P18 已使未来未物质化 BasicCache 的空间候选以 r2 确定性 Profile 完整进入 P9，并且只经 P10 的既有 P9→P6 原子事务进入玩家图。P19 不改写 `BasicCache.r1/r2`、BasicCorpse、P9/P10 物质化／揭示／UI／target identity、P11/P12、任何 receipt、权重、recipe 或来源；它只接受已经真实位于精确 P6 的两种空间父项。
6. P7 是 P6 玩家背包、装备槽、空间容器投影和实际 item-cell 的生产 Host；P14 已有 `GroundDropZone` 与 `WorldDropTarget` 是真实 `NativeOnDrop` 链的一部分。所有 P19 的位置写入仍必须由已挂载生产 Cell 的明确 Drop 提交。双击、右键、详情、I、Esc、Close、Cancel、空格、UI 外区域、Actor 交互、无 payload Drop、选择状态与 1—9 都不得写物品位置。
7. P13 的 1—9 仍只保存 `SlotIndex + ItemId | Empty` 引用；P15 仍只允许位于 P6 BaseQuick 的 simple `RestoreHealth` 使用。P19 不为空间 parent、其 child 或 child 内物品增加绑定、使用、扣减、快捷、装备效果或 Code A 效果。任何因完整图离开玩家可达 P6 位置而失效的引用只可由既有同 commit reconcile 清理，拾回后不得自动恢复。

## 3. P19 产品裁决

P19 的单一纵向切片是：一个已在精确活动 P6 的 `WindTalisman` 或 `BackpackLevel1` parent，可以连同其唯一 P17 ChildContainer 与其中全部合法普通物品作为一个图单元，经过既有 P14 `GroundDropZone` 进入同一 P6 的一个既有 WorldDrop root；随后玩家只能从相同 `WorldDropId` 打开的既有 `WorldDropTarget`，将该同一个完整图真实拖回一个兼容且为空的 P6 玩家位置。

地面上的空间 parent 不是可打开的新背包，不是多物品地面容器，也不是 child-item 拾取来源。`WorldDropTarget` 在本轮仅表示完整图的 root（可读取必要的只读结构摘要），不展示、转移或修改其内部 child cells。无论 parent 当前 child 为空还是包含合法普通物品，丢弃与拾回都必须保持完整、原子和无拆分。

| 项目 | 本轮固定裁决 |
| --- | --- |
| 合法来源 | 仅已在当前 P6 的 `WindTalisman` 或 `BackpackLevel1` 完整图；不得从 P5、P9、P11、旧库存、Code A、地图或新 recipe 取得。 |
| 合法丢弃位置 | 当前 P6 BaseQuick，或该 parent 的正式兼容装备位置：`WindTalisman` 的 `Prototype.Slot.SpatialRing`、`BackpackLevel1` 的 `Prototype.Slot.Backpack`。来源必须是 P7 已呈现的真实 item cell。 |
| 地面真值 | 继续只用 P14 的一个 WorldDrop record、一个 world root、同一 parent `ItemId` 与其现存 child closure；不建立 child 清单或第二套容器。 |
| 拾回位置 | 仅真实拖到一个空的 P6 BaseQuick cell，或该 parent 自身正式兼容且为空的装备位置；不自动装备、Swap、Merge、Split、QuickMove 或按钮拾取。 |
| 地面操作 | 只能整体移回 parent 与完整 child closure；不得打开、单独拾回、单独丢弃、转移或编辑 child 内物品。 |
| 终局 | P8 Extracted、Dead、RecoveredAbandon 都完整丢弃仍属于 P14 world root 的 parent、child 与后代普通物品；不得有任何一部分回流 P5。 |

---

# 下半部分：授权执行内容

## 4. 单一授权目标

在不改变 P14 简单物品路径和 P17/P18 真值的前提下，扩展 P14／P1 的候选事务和 P7 真实拖拽入口，使两个已定义的空间 parent 可作为完整、不可拆分的 P1 graph 在 P6 玩家位置与既有单 root WorldDrop 之间原子往返；既有 Code A actor 只继续表现同一 `WorldDropId`，不拥有该图。

### 4.1 完整空间图资格与不可拆分不变量

1. 只接受 exact active `OwnerId + RunInstanceId` 中已提交的 root parent，DefinitionId 必须精确为 `Fdemo_mapItemIds::WindTalisman` 或 `Fdemo_mapItemIds::BackpackLevel1`。不得把 P17 的其他 tier、任意饰品、任意 backpack、任意复杂 item 或 child item 以“通用空间物品”名义一并放开。
2. 合法 parent 当前位置只能是当前 P6 `Container.Player.BaseQuick`，或其定义对应的当前正式装备位置：`WindTalisman` 的 exact spatial-ring position、`BackpackLevel1` 的 exact backpack position。实现必须通过已有 P1 slot compatibility／P17 canonical semantic 验证实际 container，而不是硬编码 Widget index、显示名或 Actor 标签。任何 P5、P9、P11、P14、old inventory、P6 child container、错误 owner、stale run、Prepared、terminal 或 closed source 均零写入拒绝。
3. 写入前必须从 parent 得到完整图 closure，并验证：唯一 parent；唯一稳定 `SpatialChildGuid(parent ItemId)`；正式 child type／capacity／layout；所有 child items 当前均在该 child graph 内；无自引用、环、duplicate child owner、orphan、容量越界、非法 DefinitionId、非法 slot 或超过一层的空间道具嵌套。child 可以包含合法普通物品，且不能因为非空被清空、flatten、拒绝或复制。
4. P19 的空间 graph 是不可分割原子。不得只移动 parent、只移动 ChildContainer、把内部普通物品先移到 world root、为 child 创建第二 WorldDrop、重建 child、重新生成 ItemId／ContainerId、改变 child contents 顺序／数量、或将 child cell 暴露为另一个 source。任何 graph validation、落点、revision、冲突或保存失败均必须保留原 P6 图、P14 record、P13 state 和所有投影，且不得占用 P14 ordinal 或生成／销毁 actor。
5. P14 的既有 simple BaseQuick 资格与完整 simple stack 行为仍保持不变。P19 只在同一服务的候选图验证中增加上述两种 formal spatial parent 的 whole-graph branch；不得放宽 simple 分支以接受其他 child graph、装备、空间 parent 或部分数量。

### 4.2 Player → WorldDrop 的完整图原子丢弃

1. 复用 P14 当前 `WorldDrops`、`NextWorldDropOrdinal`、安全落点解析、world-container identity、`Available` state 和同一 Owner durable replacement。P19 不要求 schema 4 改号；除非现有反序列化无法仅凭稳定 root `ItemId` 与 P1 graph 无歧义地识别完整 closure，才允许增加一个最小、版本化、只读兼容字段。该字段不得复制 child IDs、数量、Definition、layout、inventory 或 UI state。
2. `GroundDropZone` 继续是唯一 player→world 写入口。只在 P7 已呈现的合法 parent item cell 拖入该 zone，且 `NativeOnDrop` 携带 exact session、stable parent ItemId 与既有 P4/P3/P2/P1 请求链时，才可请求 Store-owned whole-graph drop transaction。child cell、空间容器内部任意物品、快捷栏引用、Actor interaction、键盘／点击和 synthetic payload 都不能形成该请求。
3. Store-owned transaction 必须先验证 P6 revision、exact source placement、完整 graph closure、world container empty／capacity、P14 active state、terminal／Prepared gate 与 Code A 成功返回的只读安全落点；随后在一次 Owner durable replacement 内：保持 parent／child／所有内部 ItemId 身份不变，将 graph root placement 从 P6 玩家位置移到派生 P14 world container，写入同一 WorldDrop record，并对最终 P6 graph 执行既有 P13 reconcile。不得先保存 WorldDrop、空 root、child snapshot 或 actor hint，再补写图。
4. 成功保存后，P7 必须关闭／丢弃涉及该 parent 与其 child 的所有 transient projection、drag state 和 stale pointer；`WindTalisman` 离开 exact spatial-ring position 时失去已有 quick entry，`BackpackLevel1` 离开其 P6 player position 时失去已有选择入口。P19 不删除或修改 child 内容。既有 Code A placement／projection manager 仅用同一已提交 WorldDrop projection refresh actor；actor 创建、刷新或销毁失败不得回滚或改写 P6。
5. Code A 的现有 WorldDropActor 可以按同一 `OwnerId + RunInstanceId + WorldDropId` 表现该 root，并仅转发既有范围／焦点交互。它不得读取或缓存 child inventory、生成地面子物品、改变数量、隐藏地面 content、直接拾取、自动拾取或把 parent／child 写入 Code A Save。P19 不新增第二个地面 Actor 类型、地图预放置物或随机世界来源。

### 4.3 WorldDrop → Player 的完整图真实拾回

1. 空间 graph 只能由 exact active WorldDrop projection 打开的既有 `WorldDropTarget` 作为 root source。该瞬态栏可以显示 parent 的合法只读 metadata 及“完整图”状态，但不得进入 ChildContainer、渲染／选择 child item cell、改变内部 contents 或提供子物品拖拽。
2. 唯一 WorldDrop→player 写入口是：从该 `WorldDropTarget` 的 root parent 进行真实 `NativeOnDrop`，拖到当前 P6 一个空的兼容 target：空 BaseQuick cell，或同一 parent 的 empty formal equipment position。target 身份、兼容性和空位必须由 P1／Definition Catalog 验证；不得凭 Widget slot index、点击、自动装备或 UI cache 判定。
3. 事务在同一 Owner durable replacement 中完整验证 `OwnerId`、`RunInstanceId`、`WorldDropId`、WorldDrop record、root parent `ItemId`、source world container、destination revision／capacity／slot、完整 child closure、P14 state 和 active session。成功时，同一稳定 parent／child／contents 图完整移入 P6 destination，并且只在该保存成功时删除 P14 world root 与相应 WorldDrop record。不得 clone、flatten、部分拾回、Merge、Swap、Split、QuickMove、自动整理、按钮拾取、直接 Actor pickup 或创建新的 ItemId。
4. 拾回到 BaseQuick 时，parent 只是重新位于该真实 BaseQuick cell；拾回到对应 formal equipment position 时，P7 可按已有 P17 规则重新提供该 parent 的正常入口。无论何种 destination，child view 都只能在 P7 自下一次权威 P6 projection 刷新后重新进入；不得从 `WorldDropTarget`／actor 暂存指针恢复。P13 不得自动绑定 parent 或 child 内 item，P15 也不获得任何新资格。
5. 关闭目标栏、失去距离、actor 暂时销毁、P7 Host 失效、session rebind、输入取消、错误 target、冲突或保存失败都只清理瞬态显示／请求，且保持 world graph 与 record 不变。重新打开只能从相同 P6 WorldDrop 真值重建 projection，不能第二次 materialize、重掷或补造 child。

### 4.4 生命周期、恢复与终局丢弃

1. P5→P6 bridge 继续只复制合法 player graph，P5 不增加 WorldDrop 或任何空间地面状态。P19 不创建 starter、direct grant、P5 fallback 或 P6→P5 中途回填。当前 P6 rebind／recovery 若既有规则保留同一活动 snapshot，则只能原样保留其 WorldDrop root closure，并由既有 projection refresh 重挂；不得从 Code A／Widget 推测或重建任何 graph。
2. P8 的 Code A 后置 terminal authority、分类、receipt、锁、幂等与时序均不变。P8 形成 Extracted 的 player-only P6 graph 时，必须按 P14 world root 的完整 reachability 丢弃 root parent、其 ChildContainer 和所有后代普通 item；不得仅删除 parent 记录而将 child 或 child items误判为玩家图后回流 P5。Dead 与 RecoveredAbandon 同样完整丢弃该 closure。P9／P11 残余策略保持不变。
3. P13 reconcile 必须在完整图 player→world commit 的同一 replacement 中，对最终 P6 player graph 清理所有不再有效的引用；world→player 成功后不自动恢复旧 binding。P15 继续只允许 simple、currently BaseQuick 的 RestoreHealth；空间 parent 和 child 内 item 均不得因 P19 获得按键使用、receipt 或 Code A 生命调用。
4. P9/P10、P11/P12、P16、P17、P18 的 identity、receipt、reveal、profile、UI、拖拽入口、source routing、空间 child creation 和终局残余规则均保持不变。P19 不提供 P9/P11↔P14 直接通路，也不让 WorldDropTarget 成为 P10/P12 的替代双栏搜索 UI。

## 5. 允许的改动范围

允许：

- 在 Code B P1／P6／P14 既有事务和 validation 中最小扩展两种精确正式空间 parent 的 whole-graph closure move、world record projection 与 terminal discard；
- 在 P7/P3/P4 既有 `GroundDropZone`、`WorldDropTarget` 与 transient projection 中最小接入真实 parent root 的 Drag／Drop，且只服务于完整图；
- 在既有 Code A `WorldDropActor`、placement／projection manager 与 Host 生命周期边缘增加最小 graph-aware 表现和 stale cleanup；
- 为 P8 的既有 player-only extraction／terminal close 增加 P14 world root closure 的完整排除与丢弃；
- 为 schema、序列化、调用签名或编译兼容最小调整 P13／P15／P17 的 Code B 无效引用清理或 projection declaration，但不得改变其产品语义；
- 更新 `PROJECT.md`、`PROJECT_INFO_CARD.md`、本任务 Prompt 归档和本任务 Report。

## 6. 明确不在本任务内

不得实现、启动、重构或接管：

- 除 `WindTalisman` 与 `BackpackLevel1` 外的任意复杂 parent、其他 tier ring／bag、空间道具新来源、P5 starter、直接 P6 grant、BasicCache／BasicCorpse recipe、第二普通容器、第二尸体、随机 Loot、地图预放置 loot、敌人掉落、资源／灵石、自动拾取、直接 Actor pickup、交互键拾取、地面多物品容器、地面 child-item 操作、整包以外的部分丢弃／拾回；
- ChildContainer 打开、世界地面背包 UI、子物品转移、child 内物品单独 Drop／Pickup、嵌套袋、空间容量变化、空间装备效果、武器／道袍／饰品效果、物品使用、其他消耗品、技能、Buff／Debuff、动画、音效、战斗属性、网络同步或多人；
- P10/P12 的 UI、读条、揭示、状态机、target identity、普通／尸体转移；P13 新绑定语义、P15 新使用语义、P16/P18 Profile、P17 child creation／definition mapping 的产品改写；
- Code A 的地图、Player、原始输入派发、HUD、战斗、生命、死亡、Run、Run Save、终局分类、旧库存、旧 Loot／搜索或正式结算权威，以及任何 A／B 双写；
- 产品启动、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 P14 simple-item branch 和 schema 4 兼容性，确认没有回归、重写或放宽为任意 complex graph；P19 只接受精确 `WindTalisman` 与 `BackpackLevel1`，且仍只通过同一 WorldDrop root。
2. 审查 P17/P1 graph closure：parent／child／contents 的稳定 identity、formal child type／capacity、无环、一层限制、无 duplicate／orphan、非空 child 保留，以及所有失败路径的零写入。确认没有 flatten、clone、child-list mirror、child WorldDrop 或新 ItemId／ContainerId。
3. 审查 player→world 事务：只有 P7 已挂载真实 cell 的 `GroundDropZone NativeOnDrop` 可写；准确 source placement、full graph、revision、terminal／Prepared gate、world root 与安全落点先验证后同 commit 保存；拒绝路径不占 ordinal、不生成 projection、不清 P13。
4. 审查 world→player 事务：只有 `WorldDropTarget` root 的真实 Native Drop 到空兼容 P6 destination 可写；成功时完整图同 commit 回归并删同一 WorldDrop record；确认没有打开 child、单独拾取、自动装备、Swap、Merge、Split、QuickMove、按钮／Actor pickup 或 widget direct write。
5. 审查 P7/P13/P15/P17 行为：丢弃后 transient child view／drag pointers 清理；拾回只从新的 P6 projection 恢复；快捷栏只 reconcile 不自动绑定；P15 及 Code A health 不产生新路径。
6. 审查 P5/P6/rebind/P8：P5 无世界状态；rebind 不补造；Extracted、Dead、RecoveredAbandon 都完整丢弃 P14 world root closure，绝不让 child／contents漏入 P5；Code A 终局权威、P9/P11 残余规则不变。
7. 审查 Code A diff，确认仅限既有 P14 projection／placement／interaction 生命周期适配，不保存、决定、复制或结算物品图；没有地图、输入、HUD、战斗、生命、Run、Run Save、Loot 或 inventory authority 扩张。
8. 编译一次 Editor 目标：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex

9. 若编译失败，只修正 P19 引入的局部声明、include、P1 closure validation、P14 transaction／projection、P7 transient UI、P8 world-root exclusion、P13 reconcile、P15/P17 declaration 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要越过本 Prompt 边界，停止受影响工作并报告；不得自行扩展到新来源、child 地面操作、P20、0.0.9B.F 或 Code A 权威。

## 8. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P19.0.r0_report.md，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须简洁、可审计地列出：

1. P19 新增／修改／未修改的每个文件及职责；
2. P14 existing WorldDrop schema／identity 如何继续只用 root `ItemId` 表示完整空间图，以及 simple-item branch 如何保持不变；
3. 两种精确 DefinitionId、合法 P6 source／destination placement、P17 child semantic／capacity、允许非空 child 与一层限制结论；
4. player→world 与 world→player 的唯一真实 Native Drag／Drop 入口、whole-graph 原子 commit、stable identity、拒绝／冲突／保存失败零写入，以及没有 flatten／clone／child 地面操作的结论；
5. P7 transient projection、WorldDropTarget root-only 表现、Code A actor／placement adapter 的职责，以及 Code A 不拥有图／库存／终局权威的理由；
6. P5→P6、rebind／recovery、P8 Extracted／Dead／RecoveredAbandon 对 world root closure 的完整丢弃、P13 reconcile、P15 不变性与 P9/P10／P11/P12／P16/P17/P18 边界；
7. 实际 Editor 编译命令、目标、最终 native exit code 与关键结果；
8. 明确列出未执行的 F 阶段项目：真实空／非空空间图丢弃、Actor 投影、重开、完整拾回、P7 入口失效／恢复、rebind、P8 三种终局、多分辨率输入、CTA、wrapper、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验证仍由 `0.0.9B.F` 负责；
9. 明确列出尚未启动的功能：其他 tier space item、空间道具新来源、child 地面操作、嵌套袋、随机世界 Loot、自动／直接拾取、地面多物品、装备效果、武器／道袍／饰品效果、其他 consumable 与后续 P 阶段。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code `0` 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P19_COMPILE_REWORK

若现有 P14 single-root record、P1 graph 或 P7 真实 Drag／Drop 无法在不创建第二真值、拆分 child graph、改写 Code A 权威或扩展新来源的前提下承载完整空间图，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P20、`0.0.9B.F` 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P19.0.r0","file":"Dev.D.UE.0.0.9B.P19.0.r0_report.md"}

