# Dev.D.UE.0.0.9B.P33.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P33——未装备标准非空间装备 root 的 BaseQuick 主动落地与明确拾回。
- 任务编号：Dev.D.UE.0.0.9B.P33.0.r0。
- 前置：已接受 0.0.9B.P1—P32 与 0.0.9BFix.P1—P4。Fix 仅用于已确认、已验收功能的缺陷修复；P33 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P33.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P33.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动真实运行验证，也不得自动开始 P34、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程及已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、WorldDrop Actor、地图放置适配层、数量草稿和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行世界背包、Actor-first 写入或 A/B 双写。

P4、0.0.9BFix.P4 与 P23 已确立共享 Inventory Workspace Kernel：稳定 Slot 地址、共享 Cell/Drag payload、统一 modifier router、动态空间容量、稳定排列、独立滚动、Ctrl + 左键 QuickTransfer 与 Shift + 1—9 Bind。所有 Reveal 后的 root item 都使用同一拖拽资格与同一 P4 → P3 → P2 → P1 写入链；Definition、slot semantic、完整图、容量与 revision 只裁决落点是否合法，不能产生第二种按物品类别分叉的拖拽入口。

P14 建立 P6 内的 WorldDrop 根记录，P19 建立空间 parent 的完整图落地/拾回，P26—P28 建立 simple stack 的整堆、精确数量与部分拾回，P29/P30 建立已打开 WorldDrop 的受限 QuickTransfer，P31 建立 Owner/Run scoped 的多 record Registry。每一条 Registry record 永远只包含一个 root、一个 derived world container、一个 WorldDropId、一个 Ordinal 与一个 Actor projection；所有打开、拾回、恢复、Actor diff 和 P8 终局排除均按 exact record identity 处理。

P32 已使当前 P6 正式 Weapon、ArmorRobe/道袍/护甲和普通 Accessory 装备位中的已装备标准非空间 root 可以正常落地，并可从 exact opened WorldDrop 拖回明确空 BaseQuick 或明确空兼容装备位。P32 有意排除了位于 P6 BaseQuick 普通格、但尚未装备的同类 standard root。本 P33 只补这一处来源缺口。

### 2. P33 产品裁决

P33 允许当前 active P6 的正式 BaseQuick 普通格中、已经 Revealed 的标准非空间装备 root，通过既有 GroundDropZone 的 normal Drag 主动落地。该 root 必须是当前 Catalog/Repository 所证明的 Weapon、ArmorRobe/道袍/护甲或普通 Accessory 标准装备实例，且不可堆叠、Quantity = 1、无 ChildContainer、无 spatial semantic。

P33 的世界 root 仍只能先由玩家主动打开其 exact Actor；随后只能 normal Drag 到用户明确选择的空 BaseQuick 格，或明确选择的空、Definition-compatible 正式装备位。P33 不提供自动装备、自动选格、快捷丢弃、Ctrl + 左键装备拾回、右键 Take 或 Actor direct pickup。

| 明确动作 | 唯一允许来源 | 唯一允许目标 | 权威结果 |
| --- | --- | --- | --- |
| 主动落地 | active P6 中正式 BaseQuick 的 standard non-spatial equipment root | 既有 GroundDropZone | 一个 P1 whole-root Move，直接进入新的 P31 derived world container slot 0。 |
| 明确拾回到普通格 | 当前已打开、身份有效的 P32/P33 standard WorldDrop root | 用户明确 Drop 的空 BaseQuick 格 | 一个 P1 whole-root Move；同一 accepted Owner save 只删除该 exact record、其空 world container 与 Actor。 |
| 明确拾回到装备位 | 当前已打开、身份有效的 P32/P33 standard WorldDrop root | 用户明确 Drop 的空、正式、Definition-compatible P6 equipment slot | 一个 P1 formal Equip/root relocation；不搜索候选槽、不自动装备、不替换、不 Swap。 |

P33 是已有 root relocation 的一个来源扩展，不是第二库存、普通容器泛化或新的输入命令。P26—P28 simple stack、P19 complete spatial graph 与 P32 equipped standard root 的既有语义必须保持。

### 3. 严格范围与持续排除

P33 只接受同时满足全部条件的 root：

1. 当前位于 active P6 Layout 的 exact BaseQuick/Basic 普通根容器及其一个正式 stable SlotIndex；实际 ContainerId 必须从 active Layout/P1 真值读取，不得写死别名、数字、fixture 或猜测字符串。
2. Catalog 与 Repository 共同证明其为已 Revealed 的正式 standard equipment Definition：Weapon、ArmorRobe/当前正式道袍/护甲，或当前正式普通 Accessory；实例不可堆叠、Quantity = 1、MaxStack = 1、没有 ChildContainerId、没有 spatial semantic。
3. ItemId、DefinitionId、source stable address、OwnerId、RunInstanceId、P6 revision、Definition slot semantic 与 source placement 均可在 Preview 与 durable Commit 前从 P1/P6 真值重新验证。

以下对象或来源持续不属于 P33：

1. 已装备 Weapon、ArmorRobe/道袍/护甲和普通 Accessory root；它们继续只走 P32 equipment-source 路径，P33 不改写 P32。
2. WindTalisman、BackpackLevel1、SpatialRing、任何带 ChildContainer 的 parent、child item、嵌套图或其他 complex graph；它们继续仅走 P19/P30。
3. simple stack、部分数量、Merge、Split、空/已满 stack 以及 P26—P29 stack 路径。
4. P17 child、P5 warehouse、P9 normal container、P11 corpse、尸体装备位、Hidden/Searching 外部 item、Hotbar 引用、另一 WorldDrop、Code A 旧库存或 UI 外对象。
5. Weapon/ArmorRobe/Accessory 的数值、攻击、护甲、视觉模型、战斗、死亡、撤离、搜索、敌人、地图世界生成、经济、制作、网络或多人。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不创建第二物品真值、不改变 P31 multi-record Registry、不拆分任何空间图且不增加自动行为的前提下，将 active P6 BaseQuick 中的未装备 standard non-spatial equipment root 接入现有 GroundDrop / WorldDropTarget 工作流：

1. BaseQuick standard root normal Drag → GroundDropZone 创建新的独立 P31 WorldDrop record；
2. exact opened P32/P33 standard WorldDrop root normal Drag → 明确空 BaseQuick 或明确空兼容装备位；
3. 每条 accepted path 只建立一个 P1 candidate、一次 Owner durable replacement，并只影响对应 ItemId、source/target、exact record 和必要 Actor projection。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P1/P4/P7 的 active P6 BaseQuick placement、stable SlotIndex、whole-root Move、formal Equip、replacement/swap rejection、P6 revision、single candidate 与 BeforeSnapshot rollback 调用图。
2. P14/P26/P31 的 GroundDropZone request、floor placement、Registry insertion、WorldDropId/Ordinal、derived world container、Actor diff、exact target open、whole-root pickup、record cleanup、recovery 和 P8 player-only filtering 调用图。
3. 当前 Catalog 中 Weapon、ArmorRobe/当前正式道袍槽和普通 Accessory 的 canonical Definition/slot semantic 判断方式；确认它只依据正式 Definition/graph/provenance，而不是显示名、图标、Cell class、Actor tag、fixture 或旧库存。
4. P32 equipped source 与 P33 BaseQuick source 的现有分流位置；明确两者都继续使用共享 Drag payload 与同一 P1/P31 durable chain，且 P33 不扩大 P32 的 source eligibility。
5. P13 reconcile、P29/P30 QuickTransfer、P17/P19 graph guard、P21 corpse equipment projection、P5/P6 bridge 与 P8 terminal 的当前边界；明确本任务如何保持而不重定向它们。
6. 一切可能由 Widget 直接 Move、由 Actor 直接领取、先写临时容器、根据 first/last Registry record 或当前 selected Cell 猜测目标、预建 WorldDropId/ItemId，或在 SaveRecord 前销毁 Actor/record 的旧/旁路路径。它们不得成为 P33 写入路径。

#### 5.2 单一通用 Drag 与 BaseQuick source gate

1. BaseQuick 中符合资格的 standard root 必须继续使用 P4 现有共享 InventoryDragOperation 或等价 stable payload。不得为 P33 新建专用 Widget、button、right-click、double-click、hotkey、Actor click 或 UI direct write handler。
2. GroundDropZone 收到该 payload 时，Store 必须在 Preview 和 Commit 前复核 exact OwnerId、RunInstanceId、P6 revision、ItemId、DefinitionId、source ContainerId、source SlotIndex、formal BaseQuick semantic、quantity、stackability、ChildContainerId、spatial semantic、active session、terminal/Prepared gate 与 floor placement。
3. 只有满足第 3 节全部条件的 active P6 BaseQuick standard root 才可继续。已装备 source、普通 storage、P17 child、空间 parent/child、stack、未揭示对象、错误 Owner/Run、stale payload、已终局 session、slot mismatch、definition mismatch 或任何 graph closure 必须零写入。
4. 同一 Drag eligibility 不表示任何目标都可接收。P33 只扩展 GroundDropZone 对符合条件的 BaseQuick source 的正式接受；目标兼容性仍由 P1/P7/P14/P31 裁决。
5. 右键仍只读详情；普通左键仍只选择；Ctrl + 左键继续既有 P29/P30 路由而不获得 P33 意义；Shift + 1—9 继续仅走 P13 Bind；双击、Tab、I、Esc、Close、Cancel、滚动、失焦、空白区和无 payload Drop 均不得写入 P33。

#### 5.3 BaseQuick root → 新的 exact WorldDrop record

1. GroundDropZone::NativeOnDrop 或现有等价正式入口仍是 player → world 位置写入的唯一入口。先由既有 Code A floor-placement adapter 解析合法、不可变的 route/floor placement；该 adapter 不得创建 ItemId、WorldDropId、record、数量或 P6 graph。
2. 接受后只建立一个 Owner/Run scoped P1 candidate。它必须使用 whole-root Move 将同一个 BaseQuick ItemId 直接置入新 record 的 derived world container slot 0；不得先写临时 Container、不得 two-step relocation、不得产生短暂 visible placement、第二 revision 或第二保存。
3. root 的 Definition、Quantity = 1、ItemId、provenance 与 non-spatial single-root 资格必须原样保持。不得 Split、Merge、Swap、clone、new ItemId、new player container、改变 equipment Definition、创建 ChildContainer，或把 source 当成 P19 graph。
4. 仅在 P1 candidate、P31 Registry insertion、NextWorldDropOrdinal、P13 reconcile、full candidate validation 与 SaveRecord(Candidate) 全部接受后，才建立新的 deterministic WorldDropId、derived world container、record 与 Actor projection。已有 records 的 root/container/placement/ordinal/Actor/open state 绝不因创建本 record 而变动。
5. Candidate、slot validation、floor placement、P13 reconcile、Registry validation、Actor projection 前检查或 SaveRecord 任一失败时，完整恢复 BeforeSnapshot：BaseQuick source 仍在原 stable SlotIndex，Registry/ordinal/record/Actor 均不变，且没有 phantom world container 或短暂丢失状态。

#### 5.4 exact opened WorldDrop → 明确玩家目的地

1. 世界 Actor 只能提交“打开这个 exact record”的窄生命周期信号。Workspace WorldDropTarget 与 Drag payload 必须带并在 Preview/Commit 前复核 exact OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived container、root ItemId、route、focus、target-open generation 与 P6 revision。
2. 一个 Target 仍只渲染其 exact record 的一个 root Cell。不存在 Registry 列表、多 root 面板、child list、Take All、自动打开其他 Actor、cross-record Merge/Swap 或从显示顺序推断目标。
3. normal Drag 的唯一目的地为用户明确 Drop 的空 BaseQuick 正式普通储物格，或用户明确 Drop 的空、当前 P1/P7 Catalog 证明 Definition-compatible 的正式 equipment slot。
4. Drop 到 BaseQuick 时，复用 P1 whole-root Move。Drop 到空兼容 equipment slot 时，复用 P1 formal Equip/root relocation。两者都必须保留同一 ItemId/Definition/Quantity/provenance，且不自动绑定、使用、触发战斗效果或恢复历史 binding。
5. P33 必须保留 P32 已有 WorldDrop → BaseQuick/equipment 语义。标准 world root 的合法性必须只依 canonical definition、exact record/owner/run/revision 与 no-child/non-stackable 条件裁决；若需区分 accepted create family，只可接受既有 P32 provenance 与本任务 P33 provenance，不能把 P32 record 视为非法，也不能改写 P31 schema、migration 或 record identity。
6. 只有 accepted proof 证明该 root 已离开这个 exact derived world container，才可在同一个 Owner candidate 中移除对应 record、空 world container 与其 Actor projection。不得先删除 record/Actor 后试图 Move/Equip，也不得把另一个 Registry member 当作 fallback。
7. destination occupied、不兼容、stale、close/focus loss、Actor EndPlay、record/root/container/ordinal mismatch、错误 Owner/Run、terminal/Prepared、SaveRecord failure 或 Host 无效时都必须零写入。world root、record、Actor、BaseQuick 与其他装备均保持原状。

#### 5.5 单一事务、Registry 生命周期、终局与回滚

1. P33 的 player → world 与 world → player 各自只能形成一个 P1 candidate、一次 P31/P6 Owner replacement 和一次 SaveRecord。不得使用逐步 Move、逐 record 持久化、UI array mutation、Actor-first write、第二 Repository transaction 或 Code A inventory mutation。
2. Registry 的 canonical order 仅用于序列化、只读投影与静态比较；所有 P33 create/pickup 都只能按 exact WorldDropId/Ordinal/record revision 操作。P33 不修改 P31 migration、旧 schema 无损升级、other-record isolation 或 NextWorldDropOrdinal 的 accepted-create-only 规则。
3. P8 BuildP14PlayerOnlySession 或等价 finalization 继续枚举全 Registry，并将 P33 world root、derived world container 与其他 P19/P32 closure 一并从 player-only final graph 排除。P33 不重写 terminal 分类、P5 receipt、recovery 或 run replacement，只补齐 standard root 的同一 generic ground-root 不泄漏证明。
4. repeated Actor refresh、map reload、active P6 recovery、Actor EndPlay、close、lost focus 和 duplicate terminal observer 都只处理 matching projection/transient context，不能改变 durable item graph、删除 record 或把 root 回填 BaseQuick/equipment。
5. 成功后只刷新 source/target、对应 WorldDropTarget 与匹配 Actor；不得 Sort、Compact、重排无关 SlotIndex、重建空间区域、清空无关选择、重置无关滚动、自动 Bind/Use/Equip，或改变 P26—P32 的语义。

### 6. 允许范围

允许以最小方式修改：

- Code B P1/P2/P3/P4/P7 的既有 root relocation、BaseQuick-source validation、Drag payload、preview、commit、rollback 与 projection，仅用于 P33 BaseQuick standard root 的 GroundDrop/explicit pickup；
- P14/P31 所在的 P6 durable Store、WorldDrop record validation、accepted create/pickup proof、P13 reconcile、P8 player-only generic-root filtering 与 Actor projection callback，仅用于表达 P33 的 single-root accepted transaction；
- P7 active P6 BaseQuick/equipment projection、GroundDropZone、WorldDropTarget 与最小 Code A floor-placement/Actor presentation adapter；Code A 继续只做边缘投影/生命周期转发；
- 必要 include、声明、Build.cs、项目资料、本任务 Prompt 归档和本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 P5/P6、第二 Warehouse、WorldDrop inventory、Widget inventory、fixture、假 ItemId、clone、Code A mirror、双写、存档重置或历史数据改写。
- 不扩大到已装备 P32 source、spatial parent/child、其他 complex graph、nested container、空间品级、child 地面操作、graph Split/Merge、stack quantity、world multi-item container、record-to-record transfer、multi-slot/rotation、地面自动堆叠或跨 record merge。
- 不实现 auto equip、quick-drop、Ctrl + 左键装备拾回、actor direct pickup、交互键领取、Take All、right-click Take、双击、自动空位/目标选择、Swap、Replacement、Sort、Compact、绑定、使用、装备数值、战斗效果或 HUD 接管。
- 不改变 P5/P6 bridge、M01、P8 receipt 产品分类、P9/P10、P11/P12/P21 corpse source、P13/P15、P16、P17/P19、P20、P26—P32、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济、制作、网络或多人。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P34、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P33 的唯一合法 source 为 active P6 当前 BaseQuick 中、non-spatial、no-child、non-stackable、Quantity = 1 的 Weapon/ArmorRobe/普通 Accessory root；确认 source 与 target 全部由 Catalog/P1 truth 判断，而非 UI 或名字。
2. 审查 player → world 只由既有 GroundDropZone normal Drag 进入，使用单一 P1 Move、单一 P31 Registry candidate、一次 Owner save；确认无临时容器、无 two-step relocation、无新 ItemId/second truth、无 actor-first write。
3. 审查 world → player 只作用于 exact opened P32/P33 standard record，并且只允许明确空 BaseQuick 或明确空兼容 formal equipment slot；确认无 auto equip、target guess、Swap、Replacement、P17 child/P5/P9/P11/Hotbar fallback 或 current-record ambiguity。
4. 审查 P32 equipment-source 路径、P31 multi-record identity、other-record isolation、NextWorldDropOrdinal、Registry migration、Actor diff、open/close/stale/EndPlay/recovery/terminal、P8 full-registry exclusion、BeforeSnapshot rollback 与 P13 reconcile 均保持；成功删除只删除 exact record。
5. 审查 P19/P30 space graph、P26—P29 simple-stack/quantity、P7 normal equipment drag、P21 corpse equipment source、P5/P6 bridge、P8 receipt、P13/P15/P17 与 Code A authority 均无产品语义回归。
6. 审查 Ctrl + 左键、Shift + 1—9、right-click、double-click、normal player inventory drag、Actor interaction与 scrolling 的语义：P33 只新增 normal BaseQuick standard-root Drag 到/自 GroundDrop，不新增任何隐式位置写入。
7. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

8. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 BaseQuick-source validation、GroundDrop transaction、WorldDrop exact pickup、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P33.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改/未修改文件及职责；
2. P1/P4/P7 BaseQuick source 与 P14/P31 GroundDrop Registry 的活动调用图；
3. 实际解析到的 Weapon、ArmorRobe/当前正式道袍和普通 Accessory eligibility、BaseQuick source/target slot semantic，以及 stack/space/装备/外部来源的拒绝证据；
4. BaseQuick root → GroundDrop 的单一 Move、ItemId/Definition/Quantity/provenance 保持、无临时容器、single candidate、single Owner save、ordinal 与 Actor accepted-only 创建证据；
5. exact P32/P33 WorldDrop → explicit BaseQuick / explicit equipment slot 的两个 pickup path、P32 保持、无 auto equip/target guess/Swap/Replacement、exact record cleanup 与 other-record isolation 证据；
6. Owner/Run/revision/WorldDropId/Ordinal/record revision/target-open generation/route/focus/close/stale lifecycle 与零写入结论；
7. P31 Registry migration、Actor diff、recovery、P8 full-registry player-only exclusion、P13 reconcile、rollback 和无 second truth 结论；
8. P19/P30、P26—P29、P32、P7、P21、P5/P6/P8/P13/P15/P17 与 Code A authority 的保持结论；
9. 稳定 SlotIndex、动态空间容量、scroll、selection、right-click、double-click、Ctrl + 左键、Shift + 1—9 与无 Sort/Compact/auto behavior 的结论；
10. 两个编译命令、目标、原生 exit code 与关键结果；
11. 所有未执行的 F 阶段真实验证，至少包括：不同 BaseQuick slot 的 Weapon/ArmorRobe/多个普通 Accessory root 落地；与 P32 equipped root、多 record 同时存在；分别拖回 BaseQuick/正确装备位；occupied/incompatible slot 零写入；stack/space/P17 child/warehouse/corpse 拒绝；stale/close/EndPlay/recovery/terminal；Actor projection；真实鼠标键盘、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P33 BaseQuick standard-equipment GroundDrop/explicit pickup 静态闭合、P32/P31 multi-record 与 P19/P26—P30 边界保持、P8 全量排除无泄漏，且 Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P34_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P33_REWORK

若 P1/P7 的现有 BaseQuick source 语义无法在不建立临时容器、第二次保存、第二物品真值或改写空间图边界的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P34、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P33.0.r0","file":"Dev.D.UE.0.0.9B.P33.0.r0_report.md"}
