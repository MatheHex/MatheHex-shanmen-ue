# Dev.D.UE.0.0.9B.P34.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P34——已打开标准非空间装备 `WorldDrop` 的受限 `Ctrl + 左键`快速拾回。
- 任务编号：`Dev.D.UE.0.0.9B.P34.0.r0`。
- 前置：已接受 `0.0.9B.P1—P33` 与 `0.0.9BFix.P1—P4`。Fix 仅用于已确认、已验收功能的缺陷修复；P34 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P34.0.r0_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9B.P34.0.r0_report.md`。
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动真实运行验证，也不得自动开始 P35、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 `0.0.9B`、活动工程及已接受任务链。所有 `0.2`、`V2`、`V3`、`I`、`IPF`、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 `ItemId`、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、QuickTransfer intent、WorldDrop Actor、地图放置适配层、数量草稿和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 `ItemId`、平行世界背包、Actor-first 写入或 A/B 双写。

P4、`0.0.9BFix.P4` 与 P23 已确立共享 Inventory Workspace Kernel：稳定 Slot 地址、共享 Cell/Drag payload、统一 modifier router、动态空间容量、稳定排列、独立滚动、`Ctrl + 左键` QuickTransfer 与 `Shift + 1—9` Bind。输入只产生瞬时 intent；所有位置写入继续经 P3 → P2 → P1 → Owner durable callback 的单一链完成。

P14 建立 P6 内的 single-root `WorldDrop` 记录；P19 建立空间 parent 的完整图往返；P26—P28 建立 simple stack 的整堆、精确数量与部分拾回；P29 建立已打开 simple-stack WorldDrop 的受限双向快速转移；P30 建立已打开 P19 complete graph root 到首个空 `BaseQuick` 格的快速拾回；P31 建立 Owner/Run scoped 的多 record Registry。每条 Registry record 永远只包含一个 root、一个 derived world container、一个 `WorldDropId`、一个 Ordinal 与一个 Actor projection；所有打开、拾回、恢复、Actor diff 和 P8 终局排除均按 exact record identity 处理。

P32 已允许 active P6 已装备的 standard non-spatial Weapon、ArmorRobe/道袍/护甲和普通 Accessory root，经 normal Drag 主动落地，并经 normal Drag 明确回到空 `BaseQuick` 或空兼容装备位。P33 将同一 normal Drag 闭环补至 active P6 `BaseQuick` 普通格中尚未装备的同类 root。两项均有意不赋予 `Ctrl + 左键`自动装备、自动丢弃或目标猜测语义。

### 2. P34 产品裁决

P34 补齐同一共享工作台对 P32/P33 已有标准装备地面记录的快捷拾回：当玩家已经主动打开并保持一个身份有效的 P32 或 P33 standard non-spatial equipment `WorldDropTarget` 时，对该 root Cell 按下 `Ctrl + 左键`，可将该同一 root 一次性移入当前 active P6 `BaseQuick` 中按真实 `SlotIndex` 升序的第一个正式、可写、空普通储物格。

这是一条已有 `Ctrl + 左键` QuickTransfer intent 的 topology/source-family 分支，不是新的按钮、Actor 直接领取、第二个库存、自动装备或快捷丢弃。它对应“已打开外部物品窗口时，快速转移该已选根物品”的统一交互；标准装备没有 merge 或部分数量语义，因此只允许 whole-root Move 到安全、确定的 BaseQuick 空格。

| `Ctrl + 左键`来源 | 唯一允许的快速目的地解析 | 权威事务 |
| --- | --- | --- |
| 当前已打开、身份匹配的 P32/P33 standard non-spatial equipment WorldDrop root | active P6 `BaseQuick` 内按真实 `SlotIndex` 升序的第一个合法空普通储物格 | 一次 P1 whole-root `Move`；同一 P31/P6 Owner candidate 删除该 exact WorldDrop record。 |
| 玩家侧 P32/P33 standard root | 本任务不提供目的地 | 零写入拒绝；仍以 normal Drag → `GroundDropZone` 为唯一主动落地路径。 |
| 任意标准装备 WorldDrop → equipment slot | 本任务不自动解析 | 继续仅允许 normal Drag 到用户明确 Drop 的空、Definition-compatible 装备位。 |

`BaseQuick` 是本项唯一自动解析目的地：它不猜测装备意图，也不会改写已确认的 normal Drag 装备路径。没有合法空 `BaseQuick` 格时必须拒绝且零写入；不得回退到自动装备、空间 child、仓库、尸体、另一个 WorldDrop、Swap、挤位、Sort 或 Compact。

### 3. 严格范围与持续排除

P34 只接受同时满足全部条件的 source：

1. source 是当前 Workspace 中已主动打开、已 Revealed、identity-valid 的 exact P31 WorldDropTarget root；其 OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived world container、root ItemId、route、focus、target-open generation 与 P6 revision 都可在 Preview 和 durable Commit 前从 P1/P6/P31 真值重新验证；
2. root 的 canonical Catalog/Repository 定义共同证明其来自已接受的 P32 或 P33 standard record family，且为 Weapon/Weapon、Armor/Armor 或普通 Accessory/Accessory；不得以显示名、图标、Cell class、Actor tag、fixture、当前选择或 Registry 顺序判断；
3. 实例必须是 non-spatial、无 `ChildContainerId`、无 spatial semantic、不可堆叠、`Quantity = 1`、`MaxStack = 1` 的 single root，且 exact derived world container 的 slot 0 反向指向同一 `ItemId`；
4. 自动解析的目的地必须是当前 active P6 Layout 真正的 `BaseQuick`/Basic 普通根容器中，按 P1 实际 stable `SlotIndex` 升序找到的第一个正式、可写、空格。实际 `ContainerId`、容量、SlotIndex 与空格状态均从活动 P1/P6 真值读取，不得写死、别名化或猜测。

下列对象或动作持续不属于 P34：

1. P29 的 simple stack、Merge、Split、部分数量、active child 优先路径、player → world merge；P30/P19 的空间 parent、完整图、child、图闭包、空间装备位与全部 graph 语义；
2. P32/P33 normal Drag 的 player → world、explicit WorldDrop → BaseQuick、explicit WorldDrop → compatible equipment slot；这些路径保持，不得被 Ctrl 重定向或收窄；
3. player-side standard equipment 的 Ctrl + 左键、quick-drop、自动回收、Actor click/interaction direct pickup、right-click Take、double-click、Take All、交互键领取、自动选槽、自动装备、自动 Bind/Use、Swap、Replacement、Sort、Compact、stacking 或跨 record transfer；
4. P5 warehouse、P9 normal container、P11 corpse、尸体装备、P17 child、Hotbar 引用、Hidden/Searching object、另一 WorldDrop、Code A 旧库存、P8 receipt、P13 Bind、P15 Use、P21 corpse source、搜索、敌人、地图、战斗、死亡、撤离、经济、网络或多人；
5. 实机运行、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P31 multi-record Registry、不拆分空间图且不增加自动装备的前提下，将当前已打开的 P32/P33 standard non-spatial equipment WorldDrop root 接入既有 `Ctrl + 左键` QuickTransfer resolver：

1. source 只能是 current opened exact record 的 root Cell；
2. target 只能是 active P6 第一个合法空 `BaseQuick` 普通储物格；
3. accepted path 只能形成一个 P1 whole-root `Move`、一次 P31/P6 Owner durable replacement 与一次 `SaveRecord`；
4. 只有同一 accepted proof 确认 root 已离开该 exact derived world container 后，才删除该 exact record、空 world container 和其 Actor projection；
5. 任一 identity、capacity、session、record、target、close/stale 或保存检查失败时完整零写入/BeforeSnapshot rollback。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P4/P23/P29/P30 的共享 `Ctrl + 左键` pointer consumption、QuickTransfer transient intent、payload、preview、commit、取消与 stale 生命周期；确认 P34 如何在同一 resolver 中依据 canonical topology/source family 进入分支，而不是新建 input/UI/Actor 写入入口；
2. P32/P33 的 standard root canonical classification、P31 exact record identity、WorldDropTarget root-only projection、normal Drag pickup proof 与 existing P32/P33 provenance acceptance；
3. P1/P2/P3/P7 的 active P6 `BaseQuick` semantic、stable SlotIndex、whole-root `Move`、target capacity/empty validation、single candidate、BeforeSnapshot rollback 与 P13 reconcile 调用图；
4. P14/P31 的 target-open、registry record cleanup、derived world container、Actor diff、other-record isolation、recovery、terminal 与 P8 full-registry player-only filtering 调用图；
5. P29 simple-stack 分支与 P30 P19 complete-graph 分支；明确 P34 不改变它们的 source、target、quantity、graph、record lifecycle 或 player-side Ctrl 语义；
6. 一切 Widget direct Move、Actor direct pickup、right-click Take、double-click、预建 ItemId/WorldDropId、按 first/last record 或 current selection 猜测目标、UI/Actor 可写缓存、先删 record/Actor 再 Move、第二 Repository transaction 或 Code A 库存写入旁路。它们不得成为 P34 写入路径。

#### 5.2 共享输入与精确 transient intent

1. `UCodeBP3CellButton::NativeOnMouseButtonDown` 或当前等价共享 pointer router 仍是 `Ctrl + 左键`唯一消费点。命中后只能创建一次既有 QuickTransfer transient intent 并返回 `Handled`；不得继续普通选择、drag threshold、P15 Use、right-click detail 或 Actor interaction。
2. 不得为 P34 新建 Button、hotkey、Actor click、专用 Widget、第二个 pointer handler 或 UI direct write。resolver 必须仅根据重新验证的 canonical root definition、P31 exact record identity、source placement 与 P32/P33 accepted provenance 在 P29、P30、P34 分支之间分流；显示文字、图标、Cell class、Actor tag、fixture 或当前 Registry index 都不是分支依据。
3. intent 在 Preview 与 Commit 前必须复核 OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived world container、root ItemId、route、focus、target-open generation、P6 revision、active session、Prepared/terminal gate 与 Host 有效性。
4. close、focus loss、重新打开其他 record、Actor EndPlay、map reload、recovery、terminal/Prepared、record/root/container mismatch、revision stale、payload cancel、invalidation 或 Host 无效都必须立即使 intent 失效并零写入；不得删除 durable record，不得把 root 自动回填玩家侧。
5. ordinary left click 继续只选择；right-click 继续只读详情；double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、`Shift + 1—9` 与 player-side standard root 的 `Ctrl + 左键`均不得取得 P34 位置写入语义。

#### 5.3 canonical standard-root gate 与唯一自动目的地

1. Preview 只接受位于 current exact P14/P31 derived world container slot 0 的 root。Store 必须从 canonical Catalog 和 Repository snapshot 重建并全字段核对 standard definition / slot semantic，证明为 P32/P33 accepted standard non-spatial Weapon、ArmorRobe/道袍/护甲或普通 Accessory root。
2. 资格必须同时满足：已 Revealed、`bStackable=false`、`MaxStack=1`、`Quantity=1`、no-child、spatial semantic none、derived container slot 0 与 exact `ItemId` 一致、P31 record available、P32/P33 provenance accepted。任何 stack、partial draft、space parent/child、P17 child、P19 graph、P5/P9/P11/corpse/Hotbar/other WorldDrop、unknown family 或 UI-only object 一律拒绝且零写入。
3. 唯一 target 是 active P6 `BaseQuick` 中按真实 stable `SlotIndex` 升序扫描得到的第一个正式、可写、空普通储物格。每个候选必须由 P1 真值验证 ContainerId、Owner/Run、capacity、slot semantic、slot availability 与 P6 revision；UI 显示为空不构成空格证据。
4. 不得把 Weapon、Armor、Accessory、SpatialRing、Backpack 或任何装备位当成 P34 自动目的地；不得搜索 child、Hotbar、P5/P9/P11、世界容器、另一个 record、最近空位或 fixture。没有合法 `BaseQuick` 空格时，拒绝且不得交换、挤位、替换、重排或自动落地。
5. 本项只补快捷拾回，不改变 P32/P33 normal Drag 的 explicit BaseQuick / explicit compatible equipment target 规则。用户仍可通过 normal Drag 把 exact root 放入明确空兼容装备位；P34 不得自动使用这一条路径。

#### 5.4 单一 whole-root 事务、exact cleanup 与回滚

1. Preview 成功后只能建立一个 Owner/Run scoped Candidate，并只调用 P1 existing whole-root `Move`：同一个 ItemId 从 exact derived world container slot 0 直接移至唯一解析出的 empty `BaseQuick` target。不得调用 Equip、Unequip、Merge、Split、Quantity=0、RequestedMergeQuantity、WorldPickupDraft、PlayerSplitDraft 或 P19 graph Move。
2. Commit 必须沿既有 P3 → P2 → P1 → P14/P31/P6 durable callback。Store 在任何 durable write 前重新验证 command intent、source/target stable address、record/root/ordinal、Owner、Run、revision、P32/P33 canonical eligibility、target empty state、registry closure、active session 与 lifecycle gate。
3. accepted candidate 中 DefinitionId、ItemId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、provenance 与 no-child/non-spatial资格必须完全保持；唯一合法变化是 root 的 parent/slot 从该 exact derived world container 变为该 exact `BaseQuick` slot。不得生成新 ItemId、ContainerId、record、ordinal、Actor、child、binding 或第二 revision。
4. 只有 accepted snapshot/replay proof 明确证明 root 已离开这个 exact derived world container，才可在同一 Owner candidate 中删除匹配 `WorldDropId`/Ordinal/revision record、其空 world container 与该 Actor projection。不得先删 record/Actor 后尝试 Move；不得删除相邻、first/last、当前选中或任何 other Registry record。
5. `NextWorldDropOrdinal` 不得写入或递增；不得创建新 WorldDrop。P13 只按已有 accepted commit reconcile，不得自动 Bind、Use、Equip 或恢复历史 binding。
6. candidate、canonical gate、target scan、P1 Move、P13 reconcile、registry validation、record cleanup、Actor projection 前检查或 `SaveRecord`任一失败时，必须完整恢复 BeforeSnapshot：world root、record、container、ordinal、Actor、BaseQuick slot、selection 与无关 record 均保持未变，且不得遗留 phantom empty slot、phantom pickup 或 transient item copy。
7. 成功后只刷新该 exact WorldDropTarget、解析得到的 BaseQuick target 和匹配 Actor projection；不得 Sort、Compact、重排无关 SlotIndex、重建空间区域、清空无关选择、改变无关 scroll offset 或改写 P29/P30/P32/P33 的语义。

#### 5.5 非回归、终局与权威边界

1. P29 simple-stack WorldDrop 的 `Ctrl + 左键`继续保留 active P17 child 优先、BaseQuick 回退、merge-first/empty-second 与 player → world compatible `Merge(Quantity=0)`语义；P34 不得让 standard equipment 进入该分支。
2. P30/P19 complete-graph WorldDrop 的 `Ctrl + 左键`继续只将合法完整图移入首个空 BaseQuick，且 player-side complete graph Ctrl 继续拒绝；P34 不得让 standard equipment 走 P19 graph closure，也不得改写 graph 的 normal Drag/Drop。
3. P32/P33 的 normal Drag player → world 与 exact opened WorldDrop → explicit BaseQuick/equipment pickup 保持。P34 只添加 world → first empty BaseQuick 的快捷分支，不提供 standard root quick-drop、quick-equip 或更广泛的自动转移。
4. P31 Registry 的 canonical order 仅用于序列化、只读投影和静态比较；所有 P34 写入只按 exact identity 处理。P34 不修改 migration、旧 schema 无损升级、other-record isolation、ordinal accepted-create-only 规则、record-to-record 行为或 Actor open semantics。
5. P8 `BuildP14PlayerOnlySession`/等价 finalization 必须继续遍历全 Registry，并将 remaining P14/P19/P32/P33 ground root、derived world container 与全部 graph closure 从 player-only final graph 排除。P34 不重写 terminal 分类、P5 receipt、recovery 或 run replacement。
6. Code A 继续只做 floor/Actor/交互边缘投影和生命周期转发；不得拥有或复制 Item、Container、Quantity、Run、Player、Loot、搜索、终局或 WorldDrop 持久化权威，也不得自行清理/重建 Actor 以影响 Code B 真值。

### 6. 允许范围

允许以最小方式修改：

- Code B P1/P2/P3/P4/P23/P29/P30 的既有 QuickTransfer resolver、transient command identity、standard-root validation、BaseQuick deterministic target scan、preview、commit、rollback 与 projection，仅用于 P34 accepted standard WorldDrop quick pickup；
- P14/P31 所在的 P6 durable Store、WorldDrop record validation、whole-root accepted proof、exact record cleanup、P13 reconcile、P8 player-only filtering 与 Actor projection callback，仅用于表达 P34 的 single-root accepted transaction；
- P7/P3 UI 的 current opened WorldDrop identity 注入和最小 Code A 边缘转发；Code A 仍只做投影与交互转发；
- 必要 include、声明、Build.cs、项目资料、本任务 Prompt 归档和本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 P5/P6、第二 Warehouse、WorldDrop inventory、Widget inventory、fixture、假 ItemId、clone、Code A mirror、双写、存档重置或历史数据改写。
- 不扩大到 player-side standard root Ctrl、quick-drop、auto equip、equipment target scan、space parent/child、other complex graph、nested container、child 地面操作、graph Merge/Split、stack quantity、multi-item WorldDrop、cross-record transfer、multi-slot/rotation、地面自动堆叠或跨 record merge。
- 不实现 Actor direct pickup、距离自动拾取、交互键领取、Take All、right-click Take、double-click、自动空位/目标选择（除本项唯一按 SlotIndex 的 BaseQuick 空格扫描外）、Swap、Replacement、Sort、Compact、绑定、使用、装备数值、战斗效果或 HUD 接管。
- 不改变 P5/P6 bridge、M01、P8 receipt 产品分类、P9/P10、P11/P12/P21 corpse source、P13/P15、P16、P17/P19、P20、P26—P33、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济、制作、网络或多人。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P35、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P34 复用唯一 `Ctrl + 左键` pointer router 与现有 QuickTransfer intent；确认没有专用 input/UI/Actor 写入旁路，且分支只依据 canonical P32/P33 standard record truth 与 exact record identity。
2. 审查 source 只接受 current opened exact P32/P33 standard non-spatial root，并逐项复核 Owner/Run/revision/WorldDropId/Ordinal/record revision/derived container/root/focus/target-open generation；stack、space、child、external source、stale/close/terminal 均零写入。
3. 审查 target 只为 active P6 `BaseQuick` 内按真实 SlotIndex 升序第一个合法空普通格；确认没有 auto equip、P17 child/P5/P9/P11/Hotbar/other-record fallback、Swap、Replacement、Sort 或 Compact。
4. 审查 accepted candidate 只使用一个 P1 whole-root Move、一个 Owner durable replacement 和一次 `SaveRecord`；确认 ItemId/Definition/Quantity/provenance 保持，无 new ItemId/Container/record/ordinal/Actor/child/second truth。
5. 审查 exact record cleanup 只发生在 accepted Move proof 后；确认 other-record isolation、Registry migration、`NextWorldDropOrdinal`、Actor diff、open/close/stale/EndPlay/recovery/terminal、P8 full-registry exclusion、BeforeSnapshot rollback 与 P13 reconcile 均保持。
6. 审查 P29 simple-stack、P30/P19 complete graph、P32/P33 normal Drag、P7 normal equipment drag、P21 corpse equipment、P5/P6 bridge、P8 receipt、P13/P15/P17 与 Code A authority 均无产品语义回归。
7. 审查 right-click、double-click、ordinary player drag、Actor interaction、`Shift + 1—9`和 player-side `Ctrl + 左键`均未获得 P34 隐式位置写入语义。
8. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

9. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 QuickTransfer routing、standard-root validation、BaseQuick target scan、world-store transaction、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P34.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改/未修改文件及职责；
2. P4/P23/P29/P30 shared QuickTransfer 到 P34 standard root 的活动调用图，以及没有新增 pointer/UI/Actor 写入路径的证据；
3. P32/P33 canonical eligibility、P31 exact record identity、source placement 与 P29/P30 branch rejection 证据；
4. current opened target、Owner/Run/revision/record/root/ordinal、focus/close/stale/recovery/terminal 生命周期与零写入结论；
5. active P6 BaseQuick 唯一 target 的真实 SlotIndex 次序、capacity/empty 验证，以及不存在 auto equip/child/Swap/implicit fallback 的证明；
6. P1 whole-root Move、single candidate、single Owner save、ItemId/Definition/Quantity/provenance 保持、accepted-only exact P31 cleanup、Actor refresh、P13 reconcile、rollback 与 ordinal 不变的证据；
7. P29 simple stack、P30/P19 complete graph、P32/P33 normal Drag、P7、P21、P5/P6/P8/P13/P15/P17 与 Code A authority 的保持结论；
8. stable SlotIndex、动态空间容量、scroll、selection、right-click、double-click、ordinary Drag、`Shift + 1—9`和 player-side Ctrl 的无新增写入结论；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证，至少包括：P32 equipped 与 P33 BaseQuick source建立的多个 standard records；分别 Ctrl 拾回至首个空 BaseQuick；无空 BaseQuick 时零写入；normal Drag 回 BaseQuick/正确装备位保持；stack/space/P17 child/warehouse/corpse/another record 拒绝；stale/close/EndPlay/recovery/terminal/save failure；Actor projection、P8 full-registry exclusion、真实鼠标键盘、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P34 standard-equipment QuickTransfer 静态闭合、P29/P30/P32/P33 与 P31 multi-record 边界保持、P8 全量排除无泄漏，且 Editor 与 Game 均以 native exit code `0`完成时，使用：

    READY_FOR_P35_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P34_REWORK

若 P4/P29/P30 的现有 QuickTransfer resolver 无法在不创建第二输入路径、第二保存、第二物品真值、自动装备或改写 P19/P32/P33 边界的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P35、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P34.0.r0","file":"Dev.D.UE.0.0.9B.P34.0.r0_report.md"}
