# Dev.D.UE.0.0.9B.P31.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P31——多个独立、单根 `WorldDrop` 记录的持久化与投影。
- 任务编号：`Dev.D.UE.0.0.9B.P31.0.r0`。
- 前置：已接受 `0.0.9B.P1—P30` 与 `0.0.9BFix.P1—P4`。Fix 仅用于已确认、已验收功能的缺陷修复；P31 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P31.0.r0_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9B.P31.0.r0_report.md`。
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动真实运行验证，也不得自动开始 P32、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 `0.0.9B`、活动工程及已接受任务链。所有 `0.2`、`V2`、`V3`、`I`、`IPF`、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与现有 durable Store 是唯一可变物品真值。每件物品只有一个真实实例、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、WorldDrop Actor、地图交互适配层和 Code A 都只能持有投影、选择、瞬时意图或窄生命周期信号；不得持有第二库存、可写数量副本、预建 ItemId、平行世界背包或 A/B 双写。

P14 已建立活动 P6 内的自愿地面丢弃／拾回：每一条正式地面记录始终是一个 `WorldDropId`、一个派生 world container、一个 root `ItemId` 与一个 Actor projection。P19 将 `WindTalisman` 与 `BackpackLevel1` 的 parent、正式 ChildContainer 和全部内容视为不可拆分完整图；P26—P28 已补齐 simple stack 的整堆／显式数量地面 Drag/Drop 与拾回；P29 已支持当前已打开 simple-stack WorldDrop 的 `Ctrl + 左键`快速转移；P30 已支持当前已打开 P19 complete-graph root 的受限 `Ctrl + 左键`快速拾回。

P30 已通过 P 阶段验收。它保留了 P14 当前“单个记录只含一个 root”的正确边界，但也暴露出下一个产品缺口：旧 P14 durable model 在一个活动 Run 中只容纳一个活动 WorldDrop record。若玩家已把一个物品放到地面，再明确地丢下另一件物品，二者不能作为独立、可分别打开和分别拾回的记录长期共存。这是 P14 record 容器的基数限制，不是物品类型、UI 文本或输入路由问题。

P4／P23 已确立共享 Inventory Workspace Kernel：稳定 Slot 地址、共享 Cell／Drag payload、统一 modifier router、动态空间容量、稳定排列、独立滚动、`Ctrl + 左键` QuickTransfer 与 `Shift + 1—9` Bind。P31 只能把该 Kernel 的现有 WorldDrop context 从“一个可打开 record”扩展为“当前明确打开的一个 record”；不得创建世界多物品背包、第二 WorldDrop inventory 或新的写入路径。

### 2. P31 产品裁决

P31 把 P14 的活动地面掉落状态从“一个全局活动 record”收敛为**一个属于精确 Owner／Run 的、可容纳多个独立 record 的 WorldDrop Registry**。每个 record 仍然严格只代表一个 root：

| Record 身份 | 固定含义 |
| --- | --- |
| `WorldDropId` | 该一次独立地面掉落的稳定身份；不得复用或由 UI、Actor 生成。 |
| `Ordinal` | 该 Run 内单调、只在 accepted durable commit 后前进的创建顺序；不得因拾回、关闭、失败或刷新改写。 |
| derived world container | 仅存放该 record 的一个正式 root；不是所有地面物品共享的容器。 |
| root `ItemId` | simple whole stack、P26 partial split 的新 root，或 P19 complete-graph parent；每个 record 恰好一个。 |
| Actor projection | 只表现同一已提交 record 的位置与 root 投影；没有物品权威或直接拾取权。 |

因此，多个 WorldDrop 可以同时存在，但它们不是一个可打开的“地面背包列表”，也不可以相互 Merge、Swap、移动 child、Take All 或自动整理。玩家只能通过现有地图 Actor 明确打开其中一个 record；共享工作台右侧的 `WorldDropTarget` 在任意时刻只投影这个精确 record 的一个 root。

P31 的唯一新增产品能力是：在一个 active P6 Run 内，玩家可以使用既有真实 `GroundDropZone` 的完整拖拽／精确数量拖拽，再创建第二个、第三个及后续独立 WorldDrop，而已存在的记录、root、Actor、打开状态和可拾回性均不被覆盖、合并、重掷或删除。P26—P30 的每次拾回只影响当前精确打开的 record；拾回其中一个后，其他 record 必须继续存在并保持原身份。

### 3. 严格范围与持续排除

P31 只扩展 P14/P19 的 record registry 基数与生命周期；每条 record 的 single-root 语义保持不变。

以下行为属于 P31：

1. 现有 simple whole-root、P26 explicit partial-root、P19 complete-graph 的明确 `GroundDropZone` Drop 在已有活动 WorldDrop 存在时仍可创建一个新的独立 record。
2. 已打开 exact record 的 P26—P30 Drag／`Ctrl + 左键`拾回、P29 current-root simple merge 与 P30 complete-graph pickup 只读取、校验、清理同一 `WorldDropId`，绝不选择 registry 中的“第一条”“最后一条”或任意其他 record。
3. 地图 Actor、打开／关闭、失焦、stale payload、Actor EndPlay、Run terminal、P6 recovery 与 P8 player-only finalization 全部按 record 身份逐条处理。
4. 旧 schema 中已有的单 record 必须无损迁移为 Registry 的唯一成员；保留原 `WorldDropId`、derived container、root、Ordinal、位置、状态、provenance、revision 与生命周期，不得重掷、重新编号或替换其 ItemId／ContainerId。

以下内容不属于 P31：

1. 将多个 root 放入同一个 world container、在一个 Actor 内展示多个物品、世界容器 grid、地面 child-item 操作、地面空间背包面板、嵌套袋、graph Split／Merge、跨 record Merge／Swap、Take All 或自动拾取。
2. 修改 P29 的 player simple-stack → **当前同一** world root `Merge(Quantity=0)` 语义；它不能利用 P31 自动选取、创建或合并到另一条 record。P30 的 player-side complete graph `Ctrl + 左键`继续零写入拒绝，仍只能用已接受的 `GroundDropZone` 真实 Drag/Drop 主动落地。
3. 自动装备、自动绑定、自动使用、自动排序、Compact、位置压缩、空位猜测、自动打开最近 Actor、Actor direct pickup、右键 Take、双击、世界点击后隐式移动或另一条 pointer／UI 写路径。
4. 改写 P1 物品图、P5/P6 bridge、P8 receipt 分类、P9/P10 普通容器、P11/P12 尸体、P13 Bind、P15 Use、P16 Loot、P17 capacity graph、P18/P20/P21 来源、地图、战斗、生命、死亡、撤离、商店、经济、制作、网络或多人。
5. 真实产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package、网络或多人验证。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不创建第二物品真值、不把地面改造成多物品背包且不改变现有 P26—P30 交互语义的前提下，建立 P14/P19 的**多 record、每 record 单 root** WorldDrop Registry。每次明确的现有地面 Drop 都能原子地创建自己的 record；每次拾回、关闭、Actor refresh、恢复和终局都精确作用于对应 record；其他 record 不受影响。

### 5. 实现要求

#### 5.1 先完成活动调用链与 singleton 审计

改动前必须审阅并在 Report 中列出：

1. P14 原 `WorldDrop` record、derived container、`WorldDropId`／Ordinal、P6 schema、snapshot、SaveRecord、BeforeSnapshot rollback、Actor projection、P8 player-only session build、recovery 与 terminal cleanup 的实际调用图；逐项找出仍将“唯一活动 record”当作数据事实的读取／写入点。
2. P19 complete-graph Drop／pickup closure，P26 partial-root Drop，P27/P28 exact pickup，P29 current simple-stack QuickTransfer，P30 complete-graph QuickTransfer 的 source／target identity、registry lookup、accepted proof 与 record cleanup 路径。
3. 当前 WorldDrop Actor 的 spawn、refresh、destroy、map route、floor placement、EndPlay、open interaction、Workspace Context／target-open generation 与 Actor-to-record identity 注入；确认 Actor 只投影一个已提交 record。
4. P1/P2/P3/P4 的 stable addresses、preview、candidate、P6 revision、single Owner durable replacement、P13 reconcile、P8 terminal receipt 与 P14/P19 world-root closure。
5. 所有可能依据列表位置、first／last active record、当前 UI cell、Actor pointer、地图坐标、显示名称、图标、文本或缓存 root 推断 WorldDrop 身份的路径。它们不得成为 P31 的写入或清理依据。

若旧字段、类名或 JSON 名称带有历史 singleton 含义，可在不采用旧规则的前提下无损迁移和收敛；不得保留两个同时可写的 registry／singleton 路径。

#### 5.2 Durable Registry、稳定身份与无损迁移

1. 在现有精确 Owner + active Run 的 P6 durable record 内，将 P14 活动 world-drop 状态表示为一个 canonical Registry／等价集合。该集合是 P6 Record 的一部分，不是 Widget、Actor、Code A、Map subsystem 或新 Repository。
2. Registry 必须由稳定 `WorldDropId` 查询，且使用 deterministic canonical order（Ordinal 升序，必要时再以 stable `WorldDropId` 打破并列）仅用于序列化、投影和静态比较；任何交互都必须以 exact `WorldDropId` 选择，不能把顺序当作目标身份。
3. 每条 record 必须独立持有并校验：OwnerId、RunInstanceId、WorldDropId、Ordinal、map route／floor placement、state、derived world ContainerId、root ItemId、record revision／provenance，以及 P14/P19 所需的 graph closure 元数据。一个 record 不能引用另一 record 的 root 或 container。
4. 对旧 schema 中的单 record，实施一次读兼容／写升级：若旧 record 有效，就将同一对象作为 Registry 的唯一成员；若旧 record 为空／已终局，迁移为空 Registry。不得生成新的 WorldDropId、Ordinal、container、root、Actor、ItemId 或 ChildContainer，不得重新 roll 任何内容。
5. 新的 record candidate 在 Preview／commit 中可使用 next ordinal 和派生 identity 做验证，但 `NextWorldDropOrdinal`、Registry insertion 和 source graph placement 只能在同一个 accepted owner candidate 内一次持久化。candidate、closure、SaveRecord、P13 reconcile、final validation 或 projection refresh 失败时，BeforeSnapshot 必须完整恢复，不得留下 durable / in-memory ordinal gap、孤儿 container、孤儿 record、phantom Actor 或空 world slot。
6. 禁止双持久化：不得同时写旧 singleton 和新 Registry，不能从 UI/Actor 反写 record，也不能将 Registry 镜像成 P5、Code A inventory 或第二 JSON 真值。

#### 5.3 明确 Drop 创建：每条 record 单 root、互不覆盖

1. 现有 `GroundDropZone::NativeOnDrop`／等价正式 Drop 入口仍是 player → world 位置写入的唯一入口。它必须沿用已有 Drag payload、P4 preview、P1 candidate 与 P14/P19 durable callback；不得为 P31 增加 Button、hotkey、Actor click、right-click、double-click、Widget direct write 或 quick-drop。
2. 当 source 是 simple whole root、P26 `Split(N)` 得到的 partial root 或 P19 complete-graph parent 时，接受的 Drop 必须创建一个新的独立 record，并只把该新 root 放到新 record 的 derived world container slot 0。已有 record 的 root、container、placement、quantity、child graph、Actor 或 opened target 绝不变动。
3. P19 complete graph 仍只允许正式 `WindTalisman` 与 `BackpackLevel1`，并须复用 canonical parent/child closure：stable `SpatialChildGuid`、formal child type/capacity、one-layer、no-cycle、no-orphan、no-duplicate、child contents 与 provenance 均完整保留。P31 只移动 root placement，不能 flatten、clone、拆 child 或创建第二图。
4. P26 partial Drop 继续以已接受的精确 `Split(N)` 语义创建新 root；P31 不能把该 root 合并进已有 ground record，不能静默改数量，也不能让原玩家堆叠或其他 record 受影响。
5. 新 record 的 Actor 只能在 owner candidate 被 durable 保存、P1/P14/P19 验证通过后由现有 projection refresh 创建。Actor spawn／refresh 失败不得回写物品图、删除其他 record 或把未提交的 Actor 当作真值。
6. P29 的 player simple-stack `Ctrl + 左键`方向继续只尝试当前已打开、exact compatible simple root 的现有 `Merge(Quantity=0)`；它不创建新 record，不能因 current target stale／不兼容／不存在而回退到 registry 中其他 root 或普通 GroundDrop。

#### 5.4 当前打开 record、拾回与交互隔离

1. Workspace 的 `WorldDropTarget` 必须携带并在 Preview／Commit 前复核 exact OwnerId、RunInstanceId、WorldDropId、Ordinal、derived container、root ItemId、record revision、target-open generation、route 与 focus。打开第二个 Actor、关闭页面、失焦、Actor EndPlay、record unavailable、root/container mismatch、Run terminal、revision stale 或 Host 失效都使旧 payload 立即失效并零写入。
2. 一个打开的 WorldDropTarget 只渲染其 exact record 的 root Cell；不得因为 Registry 中存在多个 record 而展示列表、第二 Cell、child list 或来自其他 record 的数量／信息。
3. P26 whole / partial pickup、P27/P28 exact-N pickup、P29 simple-stack `Ctrl + 左键`与 P30 complete-graph `Ctrl + 左键`必须只在 exact record 上构造 candidate。成功后只删除该 record、其空 derived container 与对应 Actor projection；所有其他 Registry entries、root、container、Actor、open state（若仍指向自身）和 ordinal 均保持。
4. P30 的完整图拾回仍只按 SlotIndex 升序选择第一个合法空 P6 `BaseQuick` 格，且不进入 active P17 child、装备位、P5/P9/P11、Hotbar 或其他 WorldDrop。它只能删除完成 accepted proof 的同一 record。
5. 任何 pickup、merge、close 或 record deletion 都不得让 UI 自动打开、自动选择、自动合并、自动删除或自动重排其他 record。其他 Actor 的刷新只反映其同一已提交 projection。

#### 5.5 Actor projection、恢复与 P8 终局

1. Actor projection 必须按 Registry diff 管理：对每条 `Available` record 仅存在一个与 exact Owner／Run／WorldDropId／Ordinal／root／container 相符的 Actor；删除／终局只销毁对应 Actor；重复 refresh、Map reload、Host rebuild 或 recovery 不能复制 Actor。
2. Actor open interaction只提交“打开这个 exact record”的窄 target lifecycle 信号。它不得读取／写入 item graph、创建 record、主动领取、决定 registry 顺序或充当当前 record 的替代真值。
3. active P6 recovery 必须从同一 durable Registry 恢复所有仍 `Available` 的 record 及其 Actor projections；每条 record 保留原 identity、ordinal、placement、root 与 closure。不得重新 materialize、重掷、合并、重新编号或将多个 root 合到一个 Actor。
4. P8 `BuildP14PlayerOnlySession`／等价 terminal finalization 必须枚举全部 active Registry records，并从 player-only final graph 排除每一条 world root、其 derived world container 以及任何 P19 parent 的合法 child closure。Extracted、Dead、RecoveredAbandon 与重复 terminal observer 都不得把任一地面 root 或 child 泄漏回 P5。
5. terminal／run replacement 后所有 Registry records 和 Actor projections 只按既有 P8/P6 durable lifecycle关闭；不得以 UI close、Actor EndPlay 或 map cleanup 单独决定物品归属。

#### 5.6 单一事务、回滚与非回归边界

1. 每项 P31 Drop／pickup 继续只建立一个 P1 candidate，沿既有 P3 → P2 → P1 → P14/P6 durable callback 提交一个 Owner replacement。不得出现逐 record 单独保存、二次 Repository transaction、Actor-first write 或 UI array mutation。
2. Store 在 durable write 前必须复核 active Owner／Run、Registry membership／absence、WorldDropId、Ordinal、derived container、root placement、source/target stable address、revision、map route、record state、P19 closure（若适用）与 `NextWorldDropOrdinal`。错误 Owner／Run、duplicate Id、ordinal mismatch、container/root collision、stale revision、route mismatch、record unavailable、SaveRecord failure 都必须零持久化并完全 rollback。
3. 持久化成功后只执行既有必要的 P13 reconcile、accepted record/Actor refresh 和当前 target refresh；不得自动 Bind、Use、Equip、Sort、Compact、更新无关 scroll、清空无关 selection 或重排未参与 ItemId 的 SlotIndex。
4. P1 唯一 parent、ContainerId/ItemId identity、stack、definition compatibility、capacity、P17 graph、P19 root-only world semantics、P26—P28 exact quantity 与 P29/P30 quick path 的既有 invariant 全部保持。

### 6. 允许范围

允许以最小方式修改：

- P14/P19 所在的 `demo_mapCodeBOutOfRaidProfile.{h,cpp}`：P6 durable schema、Registry、旧 record 无损迁移、record lookup、accepted candidate proof、P8 player-only filtering、recovery 与 single Owner commit；
- `demo_mapCodeBP3.{h,cpp}`、`demo_mapCodeBP3UI.{h,cpp}`、`demo_mapCodeBP4.{h,cpp}` 中既有 WorldDrop projection、exact target context、Drag／QuickTransfer transient identity 与 refresh；
- `demo_mapV3ProgressionManager.{h,cpp}`／现有 Map Actor adapter：只用于 Registry diff 的 Actor projection、exact record open lifecycle 与 terminal／recovery 窄转发；
- 必要的 P1/P2/P3 声明、include、Build.cs、读兼容 schema、项目资料、本任务 Prompt 归档和本任务 Report。

允许重构旧 singleton helper／字段，只要最终只有 Registry 一条可写 P6 durable 路径，并且旧单 record 可无损读取。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 P5/P6、第二 Warehouse、WorldDrop inventory、Widget inventory、fixture、假 ItemId、clone、Code A mirror、双写、存档重置或历史数据改写。
- 不实现 world multi-item container、record-to-record transfer、world child access、nested container、graph merge/split、Take All、auto pickup、quick-drop、actor direct pickup、auto equip、auto bind、auto use、Sort、Compact、空位压缩或玩家侧 graph `Ctrl + 左键`落地。
- 不改变 P5/P6 bridge、M01、P8 receipt 产品分类、P9/P10、P11/P12、P13/P15、P16、P17、P18、P20/P21、搜索、尸体、装备、地图、敌人、战斗、生命、死亡、撤离、商店、经济、制作、网络或多人。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P32、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P14 singleton 的所有活动读写均已收敛到 P6 durable Registry；确认旧单 record 可无损迁移，且无 singleton/Registry 双写、无重掷、无新 ItemId/ContainerId。
2. 审查 simple whole root、P26 `Split(N)` root 与 P19 complete graph 的明确 GroundDrop 都能建立新的独立 record；已有 record、root、Actor、ordinal 和 open context 不被覆盖或合并。
3. 审查每条 record 仍只有一个 root 和一个 derived world container；不存在地面 multi-item container、child list、跨 record merge/swap、first/last-record implicit target 或 Actor identity 猜测。
4. 审查 P26—P30 的 pickup／QuickTransfer 只作用于 exact opened record；删除一个 record 后另一个 record 的 durable data、Actor、root、container、ordinal、UI isolation 均保持；P29/P30 的现有限制不被放宽。
5. 审查 Preview／Commit／SaveRecord／actor refresh／stale／terminal／recovery／P8 player-only filtering 对多个 records 的 candidate、single Owner save、rollback、dedup 和所有 ground graph exclusion；确认 `NextWorldDropOrdinal` 只在 accepted create 中前进。
6. 审查 Code A、Widget、Actor 均未取得 Registry、Item、Container、数量、Owner/Run、Loot、搜索、终局或 world-drop 持久化权威。
7. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

8. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 Registry、migration、record lookup、world callback、projection、Actor lifecycle、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P31.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改／未修改文件及职责，以及旧 singleton 的全部活动读写收敛结果；
2. 旧单 record → Registry 的无损读兼容／写升级、canonical order、WorldDropId／Ordinal／container／root identity 保持证据；
3. simple whole、P26 partial 与 P19 graph Drop 各自如何创建独立 record，及已有 record 完全不变的证据；
4. 每 record single-root／single-derived-container／single-Actor 边界，以及没有 world multi-item inventory、child list、cross-record merge 或 implicit target 的证据；
5. P26—P30 exact opened-record pickup／QuickTransfer call graph，删除一个 record 后其他 record 保持的证据；
6. Actor projection diff、open／close／stale／EndPlay、Map reload、recovery、terminal 和 duplicate refresh 的 identity／零写入结论；
7. P8 player-only finalization 如何枚举并排除所有 active record root／child graph，及 P5 不泄漏的结论；
8. single candidate、single Owner save、`NextWorldDropOrdinal`、accepted-only Actor create/delete、BeforeSnapshot rollback、P13 reconcile 与无自动行为结论；
9. P29/P30、P19 normal Drag/Drop、P26—P28 exact quantity、P5/P6/P8/P9/P11/P13/P15/P17/P20/P21 与 Code A authority 的保持结论；
10. 两个编译命令、目标、原生 exit code 与关键结果；
11. 所有未执行的 F 阶段真实验证，至少包括多条同时落地、分别打开／关闭、分别拖拽／按数量拾回、P29/P30 QuickTransfer、一个 record pickup 后其余 record 保留、恢复／终局、Actor projection、stale 和 UI 截图巡检。

仅当 Registry 静态闭合、旧 record 无损迁移、P26—P30 exact-record 非回归、P8 全量排除与 Actor projection identity 均已闭合，且 Editor 与 Game 均以 native exit code `0` 完成时，使用：

    READY_FOR_P32_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P31_REWORK

若无法在不创建第二物品真值、world multi-item inventory、旧／新双写、重置历史或越过当前范围的前提下完成，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P32、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P31.0.r0","file":"Dev.D.UE.0.0.9B.P31.0.r0_report.md"}
