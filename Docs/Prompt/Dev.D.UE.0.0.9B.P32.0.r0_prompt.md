# Dev.D.UE.0.0.9B.P32.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P32——已装备的非空间标准装备 root 的地面主动丢弃与明确拾回。
- 任务编号：`Dev.D.UE.0.0.9B.P32.0.r0`。
- 前置：已接受 `0.0.9B.P1—P31` 与 `0.0.9BFix.P1—P4`。`Fix` 仅用于已确认、已验收功能的缺陷修复；P32 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P32.0.r0_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9B.P32.0.r0_report.md`。
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动真实运行验证，也不得自动开始 P33、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 `0.0.9B`、活动工程及已接受任务链。所有 `0.2`、`V2`、`V3`、`I`、`IPF`、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 `ItemId`、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、WorldDrop Actor、地图放置适配层、数量草稿和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行世界背包、Actor-first 写入或 A/B 双写。

P4、`0.0.9BFix.P4` 与 P23 已确立共享 Inventory Workspace Kernel：稳定 Slot 地址、共享 Cell/Drag payload、统一 modifier router、动态空间容量、稳定排列、独立滚动、`Ctrl + 左键` QuickTransfer 与 `Shift + 1—9` Bind。所有 Reveal 后的 root item 都使用同一拖拽资格与同一 P4 → P3 → P2 → P1 写入链；Definition、slot semantic、完整图、容量与 revision 只裁决落点是否合法，不能产生第二种按物品类别分叉的拖拽入口。

P14 建立了 P6 内的 WorldDrop 根记录，P19 建立 `WindTalisman` 与 `BackpackLevel1` 的完整空间图落地/拾回，P26—P28 建立 simple stack 的整堆、精确数量与部分拾回，P29/P30 建立已打开 WorldDrop 的受限 QuickTransfer。P31 已将活动地面状态收敛为 Owner/Run scoped 的多 record Registry：每一条 record 永远只包含一个 root、一个 derived world container、一个 `WorldDropId`、一个 Ordinal 与一个 Actor projection；所有打开、拾回、恢复、Actor diff 和 P8 终局排除均按 exact record identity 处理。

P7/P4x/P21 已定义玩家的标准装备语义：兵器、当前正式道袍/护甲槽以及当前正式普通饰品槽是实际 P6 装备位置；空间戒指与空间储物囊属于带 ChildContainer 的空间 parent，继续由 P19/P17 管理，绝不落入本任务。当前 Catalog 中也已有可装备、非空间、不可堆叠、`Quantity = 1`、无 ChildContainer 的标准装备 Definition。P32 只填补这类**已装备 standard root**不能走同一 GroundDrop 工作流的缺口。

### 2. P32 产品裁决

P32 让当前已装备的非空间标准兵器、道袍/护甲和普通饰品，可以从其真实 P6 装备 Cell 通过既有 `GroundDropZone` 的真实 Drag/Drop 主动丢到地面；之后玩家只能主动打开对应 Actor，并把该 exact WorldDrop root 拖回一个明确的空 `BaseQuick` 普通格，或一个明确、空且 Definition-compatible 的正式装备位。

这是已有 root relocation 的一个**来源/目标语义分支**，不是第二库存、自动卸下、自动装备、Actor 直接拾取或新的输入命令。其语义如下：

| 明确动作 | 唯一允许来源 | 唯一允许目标 | 权威结果 |
| --- | --- | --- | --- |
| 主动落地 | 当前 active P6 中已装备的 standard non-spatial root | 既有 `GroundDropZone` | 一个 P1 root relocation / existing equipment-source normalization，在同一 P31 Registry candidate 中创建一个新的 single-root WorldDrop record。 |
| 明确拾回到普通格 | 当前已打开、身份有效的 exact WorldDrop root | 用户明确 Drop 的空 `Container.Player.BaseQuick` 格 | P1 whole-root `Move`；同一 accepted Owner save 只删除该 exact record、其空 world container 与 Actor。 |
| 明确拾回到装备位 | 当前已打开、身份有效的 exact WorldDrop root | 用户明确 Drop 的空、正式、Definition-compatible P6 equipment slot | P1 formal Equip/root relocation；不搜索候选槽、不自动装备、不替换、不 Swap。 |

本任务不扩展 `Ctrl + 左键`。P29 simple-stack 与 P30 complete-graph 的既有 QuickTransfer 语义保持；P32 world equipment root 的 `Ctrl + 左键`不得获得自动拾回、自动装备、自动丢弃或选槽含义。标准的 normal Drag/Drop 是本任务唯一的位置写入手势。

### 3. 严格范围与持续排除

P32 仅接受满足全部下列条件的 root：

1. 当前位于精确 active P6 的一个正式装备位：Weapon、当前正式 ArmorRobe/道袍/护甲槽，或当前正式普通 Accessory 槽。实际 enum、ContainerId 和 slot semantic 必须复用活动 Catalog/P1/P7，不得建立别名、fixture 或猜测固定字符串。
2. Catalog/Repository 证明其为正式、已 Revealed、非空间、无 `ChildContainerId`、无 spatial semantic、不可堆叠、`Quantity = 1`、`MaxStack = 1` 的单 root standard equipment instance。
3. ItemId、DefinitionId、source stable address、OwnerId、RunInstanceId、P6 revision、slot semantic 与 source placement 均可在 Preview 和 Commit 前从 P1/P6 真值重新验证。

以下对象或来源持续不属于 P32：

1. `WindTalisman`、`BackpackLevel1`、SpatialRing、任何带 ChildContainer 的 parent、child item、任何嵌套图或其他 complex graph；它们继续仅走 P19/P30 已有规则。
2. simple stack、部分数量、Merge、Split、空/已满 stack、P26—P29 stack 路径；P32 不改写数量语义。
3. P6 `BaseQuick` 或 P17 child 中的普通 root、P5 warehouse、P9 normal container、P11 corpse、Hidden/Searching 外部 item、尸体装备位、Hotbar 引用、另一 WorldDrop root、Code A 旧库存或 UI 外对象。
4. Weapon/ArmorRobe/Accessory 的数值、攻击、护甲、角色属性、视觉模型、战斗、死亡、撤离、搜索、敌人、地图世界生成、经济、制作、网络或多人。

P32 的目的是让**已经穿戴的标准装备**在玩家明确拖到地面时不再因为源位置而被排除；它不把全部 P6 root、全部容器或全部物品类型一次性泛化进 WorldDrop。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不创建第二物品真值、不改变 P31 multi-record registry、不拆分任何空间图且不增加自动行为的前提下，将 P7 已装备的 standard non-spatial root 接入现有 GroundDrop / WorldDropTarget 工作流：

1. equipped root normal Drag → GroundDropZone 创建新的独立 P31 WorldDrop record；
2. exact opened WorldDrop root normal Drag → 明确空 BaseQuick 或明确空兼容装备位；
3. 每条 accepted path 只建立一个 P1 candidate、一次 Owner durable replacement，并只影响对应 ItemId、source/target、exact record 和必要 Actor projection。

### 5. 实现要求

#### 5.1 先完成活动调用链与 eligibility 审计

改动前必须审阅并在 Report 中列出：

1. P1/P4x/P7 的 standard equipment source placement、formal slot compatibility、Move/Equip/Unequip/root relocation、replacement/swap rejection、P6 revision、single candidate 与 BeforeSnapshot rollback 调用图。
2. P14/P19/P26 与 P31 的 GroundDropZone request、floor placement、Registry insert、`WorldDropId`/Ordinal、derived world container、Actor diff、exact target open、whole-root pickup、record cleanup、recovery 和 P8 player-only filtering 调用图。
3. 当前 Catalog 中 Weapon、ArmorRobe/当前正式道袍槽、普通 Accessory 的 canonical Definition/slot semantic 判断方式；确认它只依据正式 Definition/graph/provenance，而不是显示名、图标、Cell class、Actor tag、fixture 或旧库存。
4. P13 reconcile、P29/P30 QuickTransfer、P17/P19 graph guard、P21 corpse equipment projection、P5/P6 bridge 与 P8 terminal 的当前边界；明确本任务如何保持而不重定向它们。
5. 一切可能把 equipment source 先写入临时 BaseQuick、由 Widget 直接 `Unequip`、由 Actor 直接领取、根据 first/last Registry record 选择目标、按当前选中 Cell 推断 record、预建 `WorldDropId`/ItemId，或在 SaveRecord 前销毁 Actor/record 的旧/旁路路径。它们不得成为 P32 写入路径。

#### 5.2 单一通用 Drag 与 standard equipment source gate

1. 已装备 Weapon、ArmorRobe/当前正式道袍槽和普通 Accessory Cell 必须继续使用 P4 现有共享 `InventoryDragOperation`/等价 stable payload。不得为装备另建一个拖拽 Widget、button、right-click、double-click、hotkey、Actor click 或 UI direct write handler。
2. GroundDropZone 收到该 payload 时，Store 必须在 Preview 和 Commit 前复核 exact OwnerId、RunInstanceId、P6 revision、ItemId、DefinitionId、source ContainerId、source SlotIndex、formal source semantic、quantity、stackability、ChildContainerId、spatial semantic、active session、terminal/Prepared gate 与 floor placement。
3. 只有满足第 3 节全部 standard non-spatial 条件的已装备 root 才可继续。source 是普通储物、空间 parent/child、stack、未揭示对象、错误 Owner/Run、stale payload、已终局 session、slot mismatch、definition mismatch 或任何 graph closure 时，必须零写入。
4. “同一 Drag eligibility”不表示任何目标都可接收。P32 只扩展 GroundDropZone 对符合条件的 equipment source 的正式接受；目标兼容性仍由 P1/P7/P14/P31 裁决。
5. 右键仍只读详情；普通左键仍只选择；`Ctrl + 左键`继续既有路由而不获得 P32 意义；`Shift + 1—9`继续仅走 P13 Bind；双击、Tab、I、Esc、Close、Cancel、滚动、失焦、空白区和无 payload Drop 均不得写入 P32。

#### 5.3 已装备 root → 新的 exact WorldDrop record

1. `GroundDropZone::NativeOnDrop`/现有等价正式入口仍是 player → world 位置写入的唯一入口。先由既有 Code A floor-placement adapter 解析合法、不可变的 route/floor placement；该 adapter 不得创建 ItemId、WorldDropId、record、数量或 P6 graph。
2. 接受后只建立一个 Owner/Run scoped P1 candidate。它必须使用已有 equipment-source canonical root relocation/Move/Unequip 语义，将**同一个** equipped ItemId 直接置入新 record 的 derived world container slot 0；不得执行“先 Unequip 到临时 BaseQuick，再另一次 Move 到世界”的两步写入，也不得产生临时 visible placement、第二 revision 或第二保存。
3. 该 root 的 Definition、Quantity=1、ItemId、provenance 与非空间单-root 资格必须原样保持。不得 Split、Merge、Swap、clone、new ItemId、new ContainerId、改变 Equipment Definition、创建 ChildContainer，或把 source 当成 P19 graph。
4. 仅在 P1 candidate、P31 Registry insertion、`NextWorldDropOrdinal`、P13 reconcile、full candidate validation 与 `SaveRecord(Candidate)` 全部接受后，才建立新的 deterministic `WorldDropId`、derived world container、record 与 Actor projection。已有 records 的 root/container/placement/ordinal/Actor/open state 绝不因创建本 record 而变动。
5. Candidate、slot validation、floor placement、P13 reconcile、Registry validation、Actor projection 前检查或 SaveRecord 任一失败时，完整恢复 BeforeSnapshot：equipment source 仍在原装备位，Registry/ordinal/record/Actor 均不变，且没有 phantom world container 或短暂脱装备状态。

#### 5.4 exact opened WorldDrop → 明确玩家目的地

1. 世界 Actor 只能提交“打开这个 exact record”的窄生命周期信号。Workspace `WorldDropTarget` 与 Drag payload 必须带并在 Preview/Commit 前复核 exact OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived container、root ItemId、route、focus、target-open generation 与 P6 revision。
2. 一个 Target 仍只渲染其 exact record 的一个 root Cell。不存在 Registry 列表、多 root 面板、child list、Take All、自动打开其他 Actor、cross-record Merge/Swap 或从显示顺序推断目标。
3. normal Drag 的唯一目的地为：
   - 用户明确 Drop 的空 `Container.Player.BaseQuick` 正式普通储物格；或
   - 用户明确 Drop 的空、当前 P1/P7 Catalog 证明 Definition-compatible 的正式 equipment slot。

   不得自动选择“最佳”装备位、当前/最近空位、Accessory I/II、SpatialRing、P17 child、P5/P9/P11、Hotbar、另一 WorldDrop 或 UI 外区域。
4. Drop 到 BaseQuick 时，复用 P1 whole-root Move。Drop 到空兼容 equipment slot 时，复用 P1 formal Equip/root relocation。两者都必须保留同一 ItemId/Definition/Quantity/provenance，且不自动绑定、使用、触发战斗效果或恢复历史 binding。
5. 只有 accepted proof 证明该 root 已离开**这个 exact** derived world container，才可在同一个 Owner candidate 中移除对应 record、空 world container 与其 Actor projection。不得先删除 record/Actor 后试图装备，也不得把另一个 Registry member 当作 fallback。
6. destination occupied、不兼容、stale、close/focus loss、Actor EndPlay、record/root/container/ordinal mismatch、错误 Owner/Run、terminal/Prepared、SaveRecord failure 或 Host 无效时都必须零写入。world root、record、Actor 与已经穿戴的其他装备均保持原状。

#### 5.5 单一事务、Registry 生命周期、终局与回滚

1. P32 的 player → world 与 world → player 各自只能形成一个 P1 candidate、一次 P31/P6 Owner replacement 和一次 `SaveRecord`。不得使用逐步 Unequip、逐 record 持久化、UI array mutation、Actor-first write、第二 Repository transaction 或 Code A inventory mutation。
2. Registry 的 canonical order 仅用于序列化、只读投影与静态比较；所有 P32 create/pickup 都只能按 exact WorldDropId/Ordinal/record revision 操作。P32 不修改 P31 migration、旧 schema 无损升级、other-record isolation 或 `NextWorldDropOrdinal` 的 accepted-create-only 规则。
3. P8 `BuildP14PlayerOnlySession`/等价 finalization 继续枚举全 Registry，并将 P32 world root、derived world container 与其他 P19 closure 一并从 player-only final graph 排除。P32 不重写 terminal 分类、P5 receipt、recovery 或 run replacement，只补齐对 standard root 的同一 generic ground-root 不泄漏证明。
4. repeated Actor refresh、map reload、active P6 recovery、Actor EndPlay、close、lost focus 和 duplicate terminal observer 都只处理 matching projection/transient context，不能改变 durable item graph、删除 record 或把 root 回填玩家装备位。
5. 成功后只刷新 source/target、对应 WorldDropTarget 与匹配 Actor；不得 Sort、Compact、重排无关 SlotIndex、重建空间区域、清空无关选择、重置无关滚动、自动 Bind/Use/Equip，或改变 P29/P30/P26—P28 的语义。

### 6. 允许范围

允许以最小方式修改：

- Code B P1/P2/P3/P4/P7 的既有 root relocation、equipment-source validation、Drag payload、preview、commit、rollback 与 projection，仅用于 P32 已装备 standard root 的 GroundDrop/explicit pickup；
- P14/P31 所在的 P6 durable Store、WorldDrop record validation、accepted create/pickup proof、P13 reconcile、P8 player-only generic-root filtering 与 Actor projection callback，仅用于表达 P32 的 single-root accepted transaction；
- P7 active P6 equipment projection、GroundDropZone、WorldDropTarget 与最小 Code A floor-placement/Actor presentation adapter；Code A 继续只做边缘投影/生命周期转发；
- 必要 include、声明、Build.cs、项目资料、本任务 Prompt 归档和本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 P5/P6、第二 Warehouse、WorldDrop inventory、Widget inventory、fixture、假 ItemId、clone、Code A mirror、双写、存档重置或历史数据改写。
- 不扩大到 spatial parent/child、其他 complex graph、nested container、空间品级、child 地面操作、graph Split/Merge、stack quantity、world multi-item container、record-to-record transfer、multi-slot/rotation、地面自动堆叠或跨 record merge。
- 不实现 auto equip、quick-drop、`Ctrl + 左键`装备拾回、actor direct pickup、交互键领取、Take All、right-click Take、双击、自动空位/目标选择、Swap、Replacement、Sort、Compact、绑定、使用、装备数值、战斗效果或 HUD 接管。
- 不改变 P5/P6 bridge、M01、P8 receipt 产品分类、P9/P10、P11/P12/P21 corpse source、P13/P15、P16、P17/P19、P20、P26—P31、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济、制作、网络或多人。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P33、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 standard equipment 的唯一合法 source 为 active P6 当前已装备、non-spatial、no-child、non-stackable、quantity=1 的 Weapon/ArmorRobe/普通 Accessory；确认 source 与 target 全部由 Catalog/P1 truth 判断，而非 UI 或名字。
2. 审查 player → world 只由既有 GroundDropZone normal Drag 进入，使用单一 P1 root relocation / equipment-source normalization、单一 P31 registry candidate、一次 Owner save；确认无临时 BaseQuick、无 two-step Unequip、无新 ItemId/ContainerId/ChildContainer、无 actor-first write。
3. 审查 world → player 只作用于 exact opened record，并且只允许明确空 BaseQuick 或明确空兼容 formal equipment slot；确认无 auto equip、target guess、Swap、Replacement、P17 child/P5/P9/P11/Hotbar fallback 或 current-record ambiguity。
4. 审查 P31 multi-record identity、other-record isolation、`NextWorldDropOrdinal`、Registry migration、Actor diff、open/close/stale/EndPlay/recovery/terminal、P8 full registry exclusion、BeforeSnapshot rollback 与 P13 reconcile 均保持；成功删除只删除 exact record。
5. 审查 P19/P30 space graph、P26—P29 simple-stack/quantity、P7 normal equipment drag、P21 corpse equipment source、P5/P6 bridge、P8 receipt、P13/P15/P17 与 Code A authority 均无产品语义回归。
6. 审查 `Ctrl + 左键`、`Shift + 1—9`、right-click、double-click、normal player inventory drag、Actor interaction与 scrolling 的语义：P32 只新增 normal equipment Drag 到/自 GroundDrop，不新增任何隐式位置写入。
7. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

8. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 equipment-source validation、GroundDrop transaction、WorldDrop exact pickup、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P32.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改/未修改文件及职责；
2. P1/P4x/P7 equipment source 与 P14/P31 GroundDrop Registry 的活动调用图；
3. 实际解析到的 Weapon、ArmorRobe/当前正式道袍和普通 Accessory eligibility、source/target slot semantic，以及 space/stack/外部来源的拒绝证据；
4. equipped root → GroundDrop 的单一 root relocation、ItemId/Definition/Quantity/provenance 保持、无临时 BaseQuick、single candidate、single Owner save、ordinal 与 Actor accepted-only 创建证据；
5. exact WorldDrop → explicit BaseQuick / explicit equipment slot 的两个 pickup path、无 auto equip/target guess/Swap/Replacement、exact record cleanup 与 other-record isolation 证据；
6. Owner/Run/revision/WorldDropId/Ordinal/record revision/target-open generation/route/focus/close/stale lifecycle 与零写入结论；
7. P31 Registry migration、Actor diff、recovery、P8 full-registry player-only exclusion、P13 reconcile、rollback 和无 second truth 结论；
8. P19/P30、P26—P29、P7、P21、P5/P6/P8/P13/P15/P17 与 Code A authority 的保持结论；
9. 稳定 SlotIndex、动态空间容量、scroll、selection、right-click、double-click、`Ctrl + 左键`、`Shift + 1—9`与无 Sort/Compact/auto behavior 的结论；
10. 两个编译命令、目标、原生 exit code 与关键结果；
11. 所有未执行的 F 阶段真实验证，至少包括：装备 root 从 Weapon/ArmorRobe/多个 Accessory slot 落地；多 record 同时存在；分别拖回 BaseQuick/正确装备位；占用/不兼容槽零写入；stale/close/EndPlay/recovery/terminal；Actor projection；真实鼠标键盘、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P32 standard equipment GroundDrop/explicit pickup 静态闭合、P31 multi-record 与 P19/P26—P30 边界保持、P8 全量排除无泄漏，且 Editor 与 Game 均以 native exit code `0` 完成时，使用：

    READY_FOR_P33_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P32_REWORK

若 P1/P7 的现有装备 source 语义无法在不建立临时 BaseQuick、第二次保存、第二物品真值或改写空间图边界的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P33、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P32.0.r0","file":"Dev.D.UE.0.0.9B.P32.0.r0_report.md"}
