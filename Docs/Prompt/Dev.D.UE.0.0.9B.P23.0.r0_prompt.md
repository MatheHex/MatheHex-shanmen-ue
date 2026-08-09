# Dev.D.UE.0.0.9B.P23.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P23——宗门仓库／战备接入统一物品工作台。
- 任务编号：`Dev.D.UE.0.0.9B.P23.0.r0`
- 前置：已接受 `0.0.9B.P1—P21` 与 `0.0.9BFix.P1—P4`。`Fix` 线仅用于已验收功能的缺陷修复；`0.0.9BFix2.P1` 的未执行规划被本任务取代，禁止执行或引用。
- 执行文件：`Dev.D.UE.0.0.9B.P23.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P23.0.r0_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 任务性质：新增主线功能，不是 BugFix、不进入 F 阶段，也不自动开始 P24。

## 只读治理与产品边界

1. 唯一有效依据是当前 `0.0.9B`、本工程、已接受的 P1—P21、`0.0.9BFix.P1—P4` 与本任务。禁止读取、采用、恢复或以 `0.2`、V2、V3、I／IPF、旧页面壳、旧 CTA 或旧物品规则作出任何实现决定。
2. P4 已形成局内共享物品交互基线：统一 Cell／ItemTile、稳定 Slot 地址、通用 Drag payload、`Ctrl + 左键`快速转移、`Shift + 1—9`绑定、动态空间容量、稳定排列与独立双栏滚动。P23 要将这套内核扩展到局外 P5，不能复制第二套物品 UI 或事务路径。
3. 塔科夫只作为“局外仓库与战备为同一持久物品图、双栏整理、明确携行”的结构参考；不引入武器改装、交易、保险、经济、地图、战斗或商业规则。
4. Code B P1 Repository 与既有 durable store 是唯一可变物品真值。P5、P6、P8、P9、P11、P13 的已有权威边界及物品历史不得重置、重掷、迁移或双写；Widget 仅持有 projection、hover、selection、active destination 与 scroll offset。

## 单一授权目标

将宗门仓库／战备的 P5 页面正式接入 P4 的共享 Inventory Workspace Kernel，使 `OutOfRaidP5` 成为与局内页面共用 Cell、Drag、输入路由、稳定布局、动态空间格与滚动组件的权威 scope。

本任务完成后，玩家战备 pane 与宗门仓库 pane 必须是同一 P5 durable graph 的不同投影；不创建第二库存、P6 预览库存、Widget inventory 或 Code A mirror。

## 授权实现

### 1. 先完成静态路径审计

在改动前静态追踪并在 Report 中列出：

1. 新 Sect UI Host 打开 Warehouse／Loadout 的实际 Host、Presenter、Widget、Cell 与写入 handler；
2. P5 Move、Equip、Unequip、Merge、Split、Bind／Unbind、刷新各自进入的 Code B service；
3. P4 共享 Cell、stable address、Drag payload、pointer modifier router、动态 grid、scroll 组件与 P5 UI 的可复用边界；
4. P5 warehouse root、装备位、BaseQuick、戒指 ChildContainer、吞天袋 ChildContainer 的真实 ContainerId、Capacity、SlotIndex、revision 与 Hotbar projection；
5. 旧 P5 代码中所有 Sort、Compact、occupied-only AddChild、固定容量、Definition 特例、Widget direct mutation、右键 Take、双击 QuickMove 或其他位置写入入口。

若已有部分共用组件，继续收敛，不得重写复制。

### 2. Shared Workspace Context 与局外页面

1. 建立或收敛只读 `InventoryWorkspaceContext`／等价对象，至少表达 scope、OwnerId、可选 RunInstanceId、profile/session revision、Coordinator state、玩家／仓库 pane identity、当前 active destination、hover、selection 与两侧 scroll state；不得存放物品副本。
2. P5 Warehouse／Loadout 与 P4 局内页必须使用同一 Cell／ItemTile、stable address、Drag operation、modifier router、dynamic grid、scroll 组件。scope policy 可禁用不适用能力，但不得复制 handler。
3. `OutOfRaidP5` 固定投影：玩家装备位、BaseQuick、当前可访问的空间戒指 child、当前可访问的吞天袋 child、P5 HotbarBindings 的只读反馈，以及 warehouse root／正式仓库 containers。
4. P5 没有搜索遮蔽；仅使用 `Empty`、`Revealed`。不得伪造 Hidden／Searching、search locator 或 `x0`。
5. 同时只能有一个 active workspace input context。关闭、Host 重建、Coordinator 切换或 snapshot stale 时清除 transient hover／selection，从权威 snapshot 重投影，不保留后台写入口。

### 3. 通用移动与输入

1. 所有 P5 Revealed root（普通物品、装备、空间戒指、吞天袋及现有兼容定义）用 P4 同一稳定 Drag payload：scope、OwnerId、ItemId、source ContainerId、SourceSlotIndex、revision 与必要 graph root identity。不得按类型选择不同起拖 handler。
2. 所有 Drop 先进入共享 Move intent builder，再由 `OutOfRaidP5` policy 调用既有 P5 candidate／validation／durable replacement。P5 intent 不得误写 P6/P9/P11；局内 intent 也不得写 P5。
3. 合法性仍由权威图裁决：slot semantic、Definition compatibility、capacity、stack、graph closure、one-level、unique parent、无环、revision 与 Coordinator gate。通用拖拽不代表任何物品可进入任何格。
4. 空间 parent 的合法移动必须连同完整 ChildContainer graph 原子完成，保持 parent／child 的全部 ItemId、ChildContainerId 和 SlotIndex；不得 parent-only、flatten、clone、嵌套或 UI 创建 child graph。
5. `Ctrl + 左键`复用 P4 同一入口、QuickTransferIntent、candidate validation 和 durable transaction；右键仅可只读详情，普通左键只作选择／详情。
6. 局外 Quick Transfer：warehouse item → 当前明确激活的玩家合法 child，否则 P5 BaseQuick；玩家装备／BaseQuick／child item → warehouse root 的合法普通储物区。不得自动装备、卸下、猜测目标、Swap、挤位、自动 Split 或 Compact。候选顺序为先 SlotIndex 升序同类非满堆叠，再 SlotIndex 升序空槽。
7. `Shift + 1—9`在 `AtSect` 且 P5 workspace 打开时，按 hover 优先、selection 次之解析，只能绑定仍属于 P5 BaseQuick、quantity > 0、QuickUsable 的 ItemId；只调用既有 P13 BindHotbarSlot，不移动、复制或使用物品。普通 `1—9` 不得在 P5 workspace 中使用物品。
8. Coordinator 仅在 `AtSect` 允许 P5 写入；`PreparingStart/ActivatingWorld` 返回 pending 拒绝，`InRun/ResolvingTerminal` 返回 P5 不可写。技术失败回到 `AtSect` 后无需重启即可恢复。

### 4. 稳定布局、空间容量与滚动

1. 每个固定 P5 container 依据权威 Capacity 渲染 `0…N-1` SlotIndex；stable render key 至少包含 scope、OwnerId、ContainerId、SlotIndex。禁止 occupied-only、Sort、Compact 或数组序号重映射。
2. Equip／Unequip、Bind、刷新、Coordinator 状态、详情开关不得改变未参与物品的 ContainerId、SlotIndex 或视觉位置。只有明确 Drag／Drop、Ctrl+左键或既有合法 P5 事务可改 placement。
3. 戒指与吞天袋的标题、品级、已用格与总容量都来自真实 parent Definition 与 ChildContainer record，且必须显示全部 N 格；禁止固定 6／36 格、仅显示 occupied items 或单个占位框。定义与 child capacity 冲突时，输出结构化诊断并拒绝伪造布局。
4. 玩家战备 pane 与 warehouse pane 使用独立纵向 ScrollBox、可拖 thumb、独立 wheel routing 和独立 offset。刷新后按 scope + pane identity 保留并 clamp；scroll 不得触发 Drag、Quick Transfer、Bind 或选择。

### 5. 生命周期与隔离

1. 本任务不改变已接受的 LoadoutSelection、StartAttempt、M01 RuntimeReady、P5→P6 bridge、P8 terminal receipt 或 P13/P15 产品语义。战备整理不会创建 Run、RunInstanceId、P8 receipt、世界激活或仓库锁。
2. StartAttempt 只读取权威 P5 graph 与 selection digest，绝不读取 workspace selection、hover、active destination 或 Widget array。
3. 进入 InRun 后 P5 workspace 只读或关闭；局内 workspace 只适用于精确 OwnerId + RunInstanceId。Code A 仅提供现有 UI 生命周期、Coordinator、M01 与输入焦点的窄信号，不取得物品图、Hotbar 或移动权威。

## 允许范围

允许最小修改当前 P4 共享 workspace／Cell／projection／drag／modifier／dynamic grid／scroll 组件，现行 Sect UI Host、Warehouse／Loadout presenter／widget，P5 既有 Move／Equip／Unequip／Merge／Split 与 P13 binding 的共享 intent 接口，Coordinator 的只读 gate／生命周期接入，Editor Support 只读审计，以及必要 include／声明／Build.cs／配置与项目文档。

允许重构重复 P5 builder／handler，但必须复用既有 P5 graph 与事务，不能重建库存。

## 明确禁止

- 禁止新建项目、I／IPF、任何 `Fix2` 任务、第二 P5/P6/Warehouse、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。
- 禁止改变搜索、Loot profile、尸体／容器物品来源、空间容量数据、装备数值／效果、RestoreHealth、战斗、生命、死亡、地图、敌人、世界掉落、终局、商店、经济、制作、网络或多人。
- 禁止改动 P5→P6 bridge、P8 receipt、P13 binding lifecycle、P15 gameplay Use、P17 graph、P19 WorldDrop 或 P20/P21 物品来源的既有产品语义。
- 禁止右键、双击、详情、普通左键、页面开关或滚动产生位置写入；禁止 Take All、自动整理、自动装备、自动卸下、自动拾取、自动 Split 或占用格交换式 Quick Transfer。
- 禁止启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、自动化、回归、试玩、Cook、Package 或最终验收。
- 禁止未经回收本 Report 自动启动 P24、任意 Fix 或 F 阶段。

## P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P5 Warehouse／Loadout 与 P4 局内页是否共用同一 Cell、stable address、Drag payload、pointer modifier router、dynamic grid 和 scroll；旧 P5 写入口是否停用或只读。
2. 审查 P5 scope isolation：P5 只写 P5 durable transaction；P6/P9/P11 不写 P5；Owner、revision、Coordinator gate、payload 与 graph closure 正确；无 Widget truth 或第二 inventory。
3. 审查拖拽、Ctrl+左键、Shift+1—9、右键零位置写入、稳定 Slot、动态容量、双栏 scroll、P5→P6/P8/P13/P15 边界及无 Fix2 引用。
4. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

5. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的共享 UI、P5 projection／transaction intent、输入、空间 grid、scroll、include 或签名问题。

## Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P23.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须列出：实际改动与停用写入口；P5 改造前后 call graph；共享组件复用；P5 scope／事务隔离；Drag、Ctrl+左键、Shift+1—9 与右键零写入；SlotIndex／无重排证据；不同品级空间容量与完整格；独立滚动；未改变的 P5→P6/P8/P13/P15 边界；两个编译命令与原生 exit code；以及所有未执行的 F 阶段真实验证。

仅当上述功能静态闭合，且 Editor／Game 编译均以 native exit code `0` 完成，使用：

    READY_FOR_P24_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P23_REWORK
