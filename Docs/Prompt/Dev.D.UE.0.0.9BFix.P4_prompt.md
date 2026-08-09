# Dev.D.UE.0.0.9BFix.P4

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`（同一既有工程；不得新建项目）
- 修复线：`0.0.9BFix`
- 阶段：P4——局内统一物品工作台、搜索／移动／稳定布局与快捷输入修复
- 任务编号：`Dev.D.UE.0.0.9BFix.P4`
- 前置：已接受 `0.0.9B.P1—P21`、`0.0.9BFix.P1—P3`；`0.0.9BFix.F1/F1.r1` 已提供真实运行缺陷证据，其中已接受的框架修复继续保留
- 执行文件：`Dev.D.UE.0.0.9BFix.P4_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9BFix.P4_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 任务性质：同一 0.0.9B 工程内的插入式代码修复；不是新项目、不是 I／IPF、不是旧版本恢复，也不是 F 阶段验证
- 后续治理：本任务完成并回传 Report 后停止延长当前 Fix 链，由策划重新规划 `0.0.9BFix2`；不得自行开始 `0.0.9BFix.P5`、`0.0.9BFix2` 或 F 阶段任务

---

## 上半部分：只读项目裁决、现状与边界

### 1. 唯一有效治理依据

仅以最新 `0.0.9B` 大纲、当前活动工程、已接受的 `0.0.9B.P1—P21`、`0.0.9BFix.P1—P3`，以及本任务记录的真实截图症状为依据。

任何 `0.2`、V2、V3、旧 I／IPF、早期页面说明、早期命名、旧策划文档或旧规则均无架构、范围、验收和命名效力，不得读取、恢复、引用或据此决策。当前源码中带有历史名称的活动类只可作为需要审计的现行实现读取，不能因此恢复其历史规则、旧页面或旧物品权威。

### 2. 必须保留的已接受基线

1. Code B P1 Repository 继续是物品图唯一可变真值。P5 是局外持久物品图；P6 是精确 `OwnerId + RunInstanceId` 的活动 Run 携行图；P8 只在 Code A 提交真实终局后结算。
2. P7 已建立活动 P6 的局内个人背包与正式 `NativeOnDrop → P4 → P3 → P2 → P1` 位置写入链。
3. P9/P10 已建立现有普通容器的开启、逐件搜索／揭示和 P6↔P9 原子转移；P11/P12 已建立现有尸体、逐件搜索／揭示和 P6↔P11 原子转移。
4. P13 的 1—9 快捷栏只是稳定 `ItemId` 引用；只有同一 P5/P6 snapshot 中位于 `BaseQuick`、数量为正且 `bQuickUsable` 的物品可绑定。绑定不移动物品，不复制数量。
5. P15 已让普通 `1—9` gameplay Pressed 输入使用合法绑定的 RestoreHealth 消耗品；UI 打开、Shift 组合、重复键、无有效 session 或不合格物品不得触发使用或扣除。
6. P17 已建立空间戒指／纳物戒和空间储物囊／吞天袋的真实 `ChildContainer` 图。实际容量只能来自正式 Definition 与权威 ChildContainer：快捷戒指容量按品级变化，既有定义示例包括 4／6／8／10／12；储物囊容量同样按正式品级／`TotalCapacity` 变化。任何 UI 均不得把 6 或 36 当作永恒固定值。
7. P20/P21 已让未来现有尸体可以含空间 parent graph 和装备位物品，并通过现有 P12 事务取得。空间 parent 必须连同完整 child graph 原子移动；装备、普通物品和空间物品都保持稳定 ItemId、ContainerId、唯一 parent 和合法 slot。
8. `0.0.9BFix.P1—P3` 的新框架、Coordinator、M01 Adapter、P5 仓库／战备同图和出战边界继续有效。本任务只处理已进入真实活动 P6 后的局内物品 UI／输入／事务接入，不重做启动状态机。

### 3. 已由真实运行确认的当前缺陷

本任务必须直接修复以下一组相互关联的问题，不得只改文字或遮盖症状：

1. 普通容器／尸体中的未揭示物品显示为“未搜索 x0”；左键点击无反应，搜索无法从 Hidden 进入 Searching。需要查明是否由旧物品 Cell、数量为零判空、HitTest、旧搜索路由或新 Code B projection 相互冲突造成。
2. 尸体普通装备可以开始拖拽，而空间戒指和吞天袋不能拖拽，只能通过当前右键 Take／领取路径移动。物品是否能开始拖拽不得由物品类别决定；类别只可参与目标兼容、堆叠、容量和空间 graph 合法性验证。
3. 玩家装备／卸下任意装备时，纳物戒快捷格、吞天袋内部格和其他随身物品会重新排列。除非用户主动整理或物品本身被移动，任何装备变化、绑定变化、搜索进度、页面刷新或 snapshot revision 都不得压缩、重排、重新编号或重建其他物品的位置。
4. 吞天袋虽显示容量摘要，但内部真实槽位没有完整显示；当前页面以一个占位框或空白区域代替全部 ChildContainer cells。
5. `Tab` 局内背包与普通容器／尸体搜索页使用不同的玩家左栏结构、格子布局和交互路径。尤其尸体页左侧是缩减／另写的一套玩家背包，导致容量、排列、拖拽和刷新行为不一致。
6. 玩家侧和目标容器／尸体侧内容均可能超过可视高度；两边都必须有独立、可见、可用鼠标拖动 thumb 的纵向滚动条。
7. 当前右键 Take 形成了与拖拽不同的物品移动入口。用户现裁决：快速获取改为 `Ctrl + 左键`；右键不再承担物品位置写入。
8. 用户现裁决：统一背包 UI 内以 `Shift + 1—9` 把当前明确指向的合格物品绑定至对应快捷栏槽；普通 `1—9` 的局内使用语义继续归 P15，二者不得串线。

### 4. 固定产品裁决：统一但不抹平合法差异

1. `Tab`、普通容器和尸体搜索必须共享同一个玩家物品面板、同一种 Cell／ItemTile、同一种 Drag payload、同一个 selection／hover 模型、同一种空间容量投影和同一稳定布局规则。目标搜索页只是在右侧增加一个外部目标面板，而不是另造一套玩家背包。
2. 所有已揭示的真实 root item 均通过同一通用移动意图和同一 Code B 候选事务处理。物品类别不能决定是否产生 Drag 或使用哪条 UI→事务链；类别、Definition、slot semantic、stack rule、graph closure 和容量只决定目标是否合法。
3. Hidden／Searching 是存在但尚未揭示的搜索 Cell，不是数量为零的空 Cell。未揭示状态不显示数量、不泄露 ItemId／Definition／品级，也不能拖拽；但必须能够接收明确搜索点击。
4. P1 graph 中的 `ContainerId + SlotIndex + ItemId` 是位置身份。UI 必须按权威 slot 原样渲染，不得在刷新时按名称、类型、ItemId、装备状态或数组顺序重新紧凑排列。
5. 空间戒指和吞天袋的 UI 容量由当前 parent 的正式 Definition／ChildContainer record 决定。品级变化必须自然产生不同的 cell 数量，不允许 Cell 数量写死在 Widget、Presenter 或显示文本中。
6. 塔科夫只作为双栏整理和快捷输入的结构参考，不复制其武器、战斗、经济、地图、商业或其他规则。

### 5. P／F 阶段分界

本任务仍属于 P 阶段，只允许代码／资产实现、静态审查和最终编译。

禁止启动产品、PIE、Standalone、点击 Tab／尸体、真实搜索、真实拖拽、真实 Ctrl+左键、真实 Shift+数字键、截图、Smoke、自动化、回归、试玩、Cook、Package 或最终验收。当前截图只作为缺陷输入；所有修复后的真实交互验证留给后续 `0.0.9BFix2` 规划决定。

---

## 下半部分：授权执行内容

### 6. 单一授权目标

在现有 P6、P9/P10、P11/P12、P13、P15、P17、P20/P21 基础上建立一个正式的 **In-Run Unified Inventory Workspace**：

- `Tab` 模式显示共享玩家物品面板及现有详情／局内辅助区；
- 普通容器／尸体模式复用完全相同的玩家物品面板，并在右侧挂载当前外部目标面板；
- 搜索、Drag Drop、`Ctrl + 左键` Quick Transfer、`Shift + 1—9` Bind、空间容量渲染、稳定槽位和双栏滚动由共享组件与共享意图路由处理；
- 所有写入最终仍由现有 Code B 权威 Store／Repository 在合法事务中完成，Widget 不取得 P6/P9/P11/P13 真值。

### 7. 先做活动调用链与根因审计

实现前必须静态追踪并在 Report 中记录当前活动调用链：

1. `Tab` 输入如何打开 P7 active-P6 Host，玩家面板由哪些 Presenter／Widget／Cell 构建，装备、BaseQuick、空间 child 与 Hotbar 如何投影。
2. 普通容器与尸体交互如何挂载 P10/P12 target，左侧玩家面板和右侧目标面板各由哪些 Presenter／Widget／Cell 构建。
3. Hidden Cell 的点击、HitTest、quantity／occupied 判定、search locator、action timer 和状态回执为何导致 `x0` 与无响应。
4. 普通装备、普通 root、尸体装备、空间 parent 各自从 Cell 到 Drag/Drop 或 Take 的实际分支；定位空间类别被排除或走右键旁路的具体条件。
5. 装备／卸下后哪些 refresh／rebuild／sort／compact／enumeration 行为使权威 SlotIndex 被丢弃或映射到新视觉位置。
6. Tab 与搜索页之间所有重复的玩家 UI builder、item view model、输入 handler 和 quick-action handler。

若活动路径仍调用带历史名称的类，可在不采用旧规则的前提下审计并改造其当前代码；不得为“兼容”保留两条同时可写的 active route。修复必须收敛正式路径，不接受仅在最外层屏蔽 `x0`、为某两种 Definition 加特殊按钮、每次刷新后重新排序或新增第三套 Widget。

### 8. 共享玩家物品面板与统一页面壳

1. 从当前 P7/P12 正式 Host 中抽取或收敛一个共享 `PlayerInventoryPane`／等价组件。它必须由同一 P6 projection 渲染：
   - 固定顺序的玩家装备位；
   - 固定 BaseQuick 容器；
   - 当前可访问的每一个正式空间戒指 ChildContainer 区；
   - 当前可访问的每一个正式吞天袋／空间储物囊 ChildContainer 区；
   - P13 1—9 Hotbar 只读／绑定反馈；
   - 统一的 item hover、selection、detail 与 drag source 状态。
2. `Tab` 页面和普通容器／尸体页面必须挂载同一个玩家面板类／同一 Presenter 输出；禁止复制其 Cells、容量计算、排序、Drag handler 或 Hotbar handler到另一个 Widget。
3. 页面模式只决定右侧内容：
   - `InventoryOnly`：现有物品详情与局内世界丢弃／辅助区域；
   - `ExternalTarget`：当前 P9 普通容器或 P11 尸体的目标面板、搜索状态和合法目标 Cells。
4. 关闭、重开、Tab 切换、target 打开／关闭、snapshot 更新或 Host 重建时，只重新读取当前精确 P6/P9/P11/P13 projection；不得从 Widget array 反向重建物品图，也不得使两种页面产生不同的玩家结果。
5. 同一时刻只能有一个 active workspace input context。不得让旧 Tab 页面、旧尸体页或旧右键 Take handler 在后台继续接收位置写入。

### 9. 通用 Cell 状态与搜索修复

1. 建立一个共享、显式的 Cell presentation state，至少区分：
   - `Empty`：真实空槽，无 ItemId、无 Search intent、无数量；
   - `Hidden`：目标 record 中存在未揭示条目，仅持有不泄密的 search locator 与 revision；显示“未搜索”，不显示 `x0`；
   - `Searching`：显示同一 search action 的进度／状态，不显示数量、Definition、ItemId 或 Drag payload；
   - `Revealed`：从权威 graph 显示真实 ItemId、Definition、数量、slot 与合法交互。
2. Hidden／Searching Cell 不得通过 `Quantity == 0`、`bOccupied == false` 或无 ItemId 被当作 Empty。Cell 的搜索 HitTest 和明确左键搜索意图必须独立于 item quantity／drag eligibility。
3. 文本、图标、进度层和边框不得吞掉 Cell 的搜索点击；合理设置 HitTest visibility／pointer routing，使一次普通左键在 Hidden Cell 上只提交一次现有搜索请求。
4. 搜索 locator 必须使用当前 P9/P11 已持久化的 target identity、container/slot visibility identity、action/revision 与精确 Run 身份；不得把未揭示 ItemId、显示文本、Widget 地址、数组索引或 `x0` 当作请求身份。
5. 现有 Hidden → Searching → Revealed、搜索时长、中断、重开和 stale-action 规则保持。修复必须覆盖当前 P9 普通容器和 P11 尸体的所有既有可搜索 root／equipment／body/storage entries；不得按 Weapon、Accessory、Spatial、Material 等类型复制 handler。
6. Revealed 后才生成通用真实 Item drag payload。搜索拒绝、距离中断、页面关闭、target 销毁、Run terminal、revision conflict 或保存失败必须保持原状态或走既有中断，不得 reroll、materialize 第二份 Loot 或把未知条目变成 `x0`。

### 10. 通用 Drag／Drop 移动系统

1. 所有 Revealed root item——普通物品、尸体装备、空间戒指、吞天袋及以后兼容的现有 Definition——使用同一个共享 `InventoryDragOperation`／等价 payload。Payload 至少携带稳定 source scope、OwnerId、RunInstanceId、ItemId、SourceContainerId、SourceSlotIndex、source revision 与必要 graph root identity；不得只携带 Widget pointer、显示名、类型分支或数量文本。
2. Drag 开始资格只由“当前 Cell 为 Revealed、存在真实 root ItemId、页面／session 可写”决定。物品类别不得让空间 parent 无法起拖；空间类别只在候选事务中要求完整 child closure、one-level rule、unique parent、无环和合法 destination。
3. 所有 Drop 均收敛到一个通用 Move intent builder，再委托现有 P4/P3/P2/P1 与 P10/P12 same-owner composite durable commit。不得为普通装备、空间 parent、Take、尸体 root 各建一个 Widget mutation。
4. 目标合法性仍由权威数据决定：slot semantic、Definition compatibility、capacity、stackability、source/destination policy、visibility、revision、Run gate 和完整 graph。通用不等于任何物品可进入任何格。
5. 对空间 parent 的任何合法 P11/P9→P6 或 P6 内移动，必须把 parent 与完整 ChildContainer graph 作为不可拆分闭包原子移动；不得 parent-only、flatten、复制 child、改变 ChildContainerId 或允许空间容器嵌套。
6. 成功只提交一次 owner durable replacement，并保持 ItemId、ContainerId、ChildContainerId、未移动 cells 与 Hotbar reconciliation 正确。失败、冲突或保存失败必须零写入，不能发生视觉先移动再回滚成新顺序。

### 11. `Ctrl + 左键` Quick Transfer 取代右键 Take

1. 右键不再触发 Take、领取、QuickMove、自动装备或任何位置写入。右键可保留纯详情／上下文显示，但不得调用 Store mutation。
2. 在统一 workspace 内，`Ctrl + 左键` 作用于一个 Revealed、可写的真实 item tile 时，创建 `QuickTransferIntent`／等价通用意图；它必须调用与 Drag/Drop 相同的 target resolver、P1 candidate validation 和 durable transaction，不得直接改 Widget array。
3. 目标解析必须以当前明确打开的窗口为优先，且可审计、稳定：
   - 当前存在 P9/P11 外部目标面板时，玩家侧 item 的优先目的地是该目标的合法普通储物区；外部目标 item 的优先目的地是玩家当前明确打开／激活的合法储物区；
   - 无外部目标面板、但用户明确进入／激活某个玩家空间 ChildContainer 时，来自其他玩家容器的 item 优先进入该 ChildContainer；
   - 没有明确 opened destination 时不得猜测、自动装备或任意搬动；返回结构化 `NoActiveQuickTransferTarget`／等价拒绝。
4. 在一个明确 destination container 内，候选顺序必须稳定：先按 SlotIndex 升序寻找合法的现有同类非满堆叠（仅当现有 Merge 规则允许），再按 SlotIndex 升序寻找合法空槽。不得交换占用格、挤走其他物品、重新紧凑排列、自动 Split 或改变未参与物品的位置。
5. 外部→玩家时不得自动装备；玩家→尸体时不得写入尸体固定 equipment slots。Hidden／Searching、受保护、terminal、stale 或不可见条目不能 Quick Transfer。空间 parent 遇到空间 child 等非法目标时必须跳过非法候选；没有合法候选则零写入拒绝。
6. 普通左键保持搜索／选择语义；`Ctrl + 左键` 不得同时触发搜索、selection action、Drag start 或 P15 使用。输入 modifier 必须在统一路由入口一次判定并消费。

### 12. `Shift + 1—9` 快捷栏绑定

1. 仅在统一局内物品 workspace 已打开且拥有精确活动 P6 context 时处理 `Shift + 1—9`。目标物品按下列唯一优先级解析：
   - 当前鼠标悬停的玩家侧 Revealed item tile；
   - 若无悬停项，则当前明确选中且仍可见、仍属于同一 P6 snapshot 的玩家 item；
   - 两者均无合法对象时零写入拒绝，不得沿用页面重建前的 stale selection。
2. 该输入只调用 P13 正式 `BindHotbarSlot`／等价 binding service。Store 必须重新验证同一 Owner/Run、当前 revision、ItemId、数量、`bQuickUsable` 和 parent 恰为 P6 BaseQuick。
3. 绑定到目标槽只移动／替换一个 ItemId 引用；不得移动物品、复制数量、使用物品、创建 receipt 或改变 P1 graph。同一 ItemId 原来绑定在其他槽时继续使用 P13 的原子单引用规则。
4. `Shift + 1—9` 不得落入 P15 普通 1—9 Use request；统一 workspace 打开时普通 1—9 仍不得消费物品。workspace 关闭并回到合法 gameplay input 后，P15 现有普通 1—9 使用语义保持不变。
5. 尸体／普通容器侧 item、Hidden／Searching item、空间 child 中的 item、非 BaseQuick item、非 QuickUsable item、数量为零 item 均不得直接绑定。必须先通过合法移动进入玩家 P6 BaseQuick。
6. Shift、Ctrl、数字键与鼠标输入必须使用明确 Pressed／modifier state 和 UI focus gate；按住、key repeat、焦点丢失、页面关闭、terminal 或 stale Host 不得重复绑定。

### 13. 稳定 Slot 渲染与禁止隐式整理

1. 每个固定容器按权威 `Capacity` 建立 SlotIndex `0…Capacity-1` 的视觉 cells，并按 Item placement 的真实 SlotIndex 放置。禁止将 occupied items 收集后重新 `Sort`／`Compact`／顺序 AddChild。
2. 共享 Cell 的稳定 render key 至少包含 scope、OwnerId、RunInstanceId、ContainerId、SlotIndex；Revealed item 可再含 ItemId。snapshot revision 变化不得替代 SlotIndex 成为排序键。
3. Equip／Unequip、Hotbar Bind、搜索状态变化、quantity 变化、详情选择、外部 target 刷新或单一 Move 只更新受影响 cells／摘要。未参与事务的物品必须保持完全相同的 ContainerId、SlotIndex 和视觉位置。
4. 空间 parent 装备／卸下只改变该 parent 的合法 placement 和入口可访问性；其 ChildContainerId、内部每个 ItemId 与 SlotIndex 不得变化。若 parent 仍在当前 P6 且 ChildContainer 仍可访问，打开状态和滚动位置应保持；若不再可访问，只关闭 transient view，不改图。
5. UI 重建后必须从权威 graph 恢复同样排列，不能根据当前装备顺序、物品类型、Definition 名称或 ItemId 重新布置。只有用户明确 Drag/Drop、Ctrl+左键或未来独立“整理”事务才可改变位置；本任务不新增自动整理按钮。

### 14. 品级驱动的空间容量与完整槽位显示

1. 玩家面板必须分别显示可访问的空间戒指区和吞天袋／空间储物囊区。每区标题至少来自当前真实 parent 的只读 projection：正式显示名／品级、已用格数、总容量；不得使用固定“纳物戒 6 格”或“吞天袋 36 格”模板覆盖实际数据。
2. Cell 数量的最终权威是当前 graph 中由 Definition provenance 验证的 ChildContainer `Capacity`。Presenter 可以交叉验证 Definition 的 `RingQuickCapacity`／`TotalCapacity`，但两者冲突时必须返回结构化诊断并拒绝伪造布局，不能静默选一个固定常量。
3. 对容量 N 的合法 ChildContainer，必须准确渲染 N 个可识别 cells，而不是一个横向占位框、摘要文字、只显示 occupied cells 或固定列数。网格列数可根据可用宽度自适应，但 SlotIndex 与总 cell 数不变。
4. 当前不存在空间 parent 时显示明确“未装备／未携带”，不创建 0 格假容器；存在合法空 ChildContainer 时仍显示全部空 cells。不同品级的戒指／袋在切换后必须自然呈现不同容量，不需要新 Widget 分支。
5. 外部目标中的 Hidden 空间 parent 不得因容量投影泄露 Definition／品级／ChildContainer；只有 parent 已 Revealed 且现有产品规则允许查看其 child 时，目标面板才可复用同一动态容量组件。不得借 UI 显示创建或填充 child graph。

### 15. 玩家侧与目标侧独立纵向滚动

1. 统一 workspace 的玩家面板与外部目标面板分别放入独立纵向 ScrollBox／等价容器。内容超过 viewport 时，两侧各自显示可见 scrollbar，支持鼠标滚轮和直接拖动 scrollbar thumb。
2. 滚轮只滚动指针所在面板；拖动一侧 scrollbar 不得滚动另一侧、开始 item drag、提交搜索或触发 Quick Transfer。
3. snapshot refresh、Equip／Unequip、搜索完成、Hotbar Bind 或单项移动后，分别按 workspace context + panel identity 保留并 clamp 各自 scroll offset。只有切换到不同 Run／不同外部 target 或用户显式返回时才可按明确规则重置。
4. 在 item drag 期间允许对当前 hover 面板进行受控 edge auto-scroll 或滚轮滚动，但不得丢失稳定 drag payload、改变 source identity 或产生额外 Drop。

### 16. 权威、事务与生命周期边界

1. P6 玩家图、P9 普通容器、P11 尸体、P13 Hotbar 与其 revisions 继续由现有同 Owner durable Store 管理。共享 UI 只持有 projection、稳定 render key、hover／selection、active destination 和 scroll offset。
2. 搜索状态写入继续走 P10/P12 既有 action service；物品位置写入继续走现有 P1 候选与 P10/P12 composite commit；绑定继续走 P13；P15 Use 不因本任务改变。
3. P5、P6 bridge、Prepared/rebind、P8 terminal、P14/P19 WorldDrop、P16 deterministic Loot、P17 graph migration、P20/P21 materialization 均不得被重置、重掷、复制或改写产品语义。
4. 本任务预期不需要新增 durable item schema。若为统一 search locator 或 UI projection 必须增加纯兼容字段，必须证明不改变任何已 materialized ItemId、ContainerId、SlotIndex、Visibility、receipt、digest 或 history；不得以 schema migration 修复视觉重排。
5. Code A 只可提供既有输入、target lifecycle、搜索 timer／距离和活动 Run 的窄适配信号。不得让 Code A Widget、Actor、旧 inventory、旧 Loot 或右键 handler取得 Code B item graph、搜索结果、binding 或移动权威。

### 17. 允许范围

允许在同一活动工程内最小新增／修改：

- `CodeB/demo_mapCodeBP3.{h,cpp}`、`CodeB/demo_mapCodeBP3UI.{h,cpp}` 及其当前正式 P4 interaction／cell／drag operation，用于共享 workspace、统一 projection、Cell 状态、Drag 和输入路由；
- `CodeB/demo_mapCodeBOutOfRaidProfile.{h,cpp}` 的既有 P9/P11 search request、P10/P12 composite transfer、P13 binding 与只读 projection 窄接口；
- 一个共享 PlayerInventoryPane、ExternalTargetPane、InventoryCell／ItemTile、动态 SpatialContainerGrid 或等价 UMG/C++ 组件；
- 当前活动的 Run inventory／target lifecycle adapter 的最小输入与挂载改动，用于 Tab、搜索点击、Ctrl+左键、Shift+1—9、scroll focus 和右键零写入；
- `demo_map0909BEditorSupport` 的只读审计，可检查共享组件、输入映射、动态容量与 active route 唯一性；
- 为 Game／Editor 编译兼容所必需的 include、声明、Build.cs／配置窄调整；
- `PROJECT.md`、`PROJECT_INFO_CARD.md`、本任务 Prompt 归档和本任务 Report。

允许重构当前重复的 UI builder 和 handler，但必须保持现有 P1—P21/Fix.P1—P3 数据、事务和生命周期；不得以“重构”为由重建库存。

### 18. 明确禁止

- 禁止新建项目、Blank Project、分支工程、I／IPF、第二 P6、第二玩家库存、第二尸体库存、Widget-local inventory、Code A mirror、fixture、starter、假 ItemId、物品 clone 或双写；
- 禁止读取、采用、恢复或重新接入 `0.2`、V2、V3、旧规则、旧页面、旧 CTA、旧仓库／背包真值或旧右键 Take 写路径；
- 禁止仅把 `x0` 隐藏而不修复搜索路由，禁止为 WindTalisman／BackpackLevel1 写两个类型特例，禁止刷新后强制排序，禁止把右键和 Ctrl+左键同时保留为两条写路径；
- 禁止自动装备、自动卸下、自动整理、自动拾取、自动 Split、占用格交换式 Quick Transfer、批量领取、Take All、将未搜索物品视为空槽或搜索时泄露 ItemId／Definition／品级；
- 禁止改变搜索时长、Loot Profile、roll、尸体装备概率、空间容量数据、装备数值、RestoreHealth 效果、战斗、生命、死亡、地图、敌人、Actor、Run Save、终局分类、经济、商店、制作、多人或网络同步；
- 禁止修改 P15 普通 1—9 使用为 Shift+1—9，禁止让绑定消耗物品、让 Quick Transfer 自动绑定、让右键移动物品或让 UI focus 期间数字键误用消耗品；
- 禁止启动产品、PIE、Standalone、执行真实输入、截图、Smoke、自动化、回归、试玩、Cook、Package 或最终验收；
- 禁止未经策划回收本 Report 后自动开始 `0.0.9BFix.P5`、`0.0.9BFix2`、F 阶段或其他任务。

### 19. P 阶段静态审查与编译

完成实现后只执行以下检查：

1. 审查 active call graph：Tab、P9/P10、P11/P12 是否挂载同一玩家 pane／Cell／Drag／input router；旧右键 Take、重复玩家 builder 和类型专用 Drag handler是否已从正式写路径移除或变为只读。
2. 审查 Search：Hidden／Searching／Empty／Revealed 分离；未搜索不显示 `x0`；搜索点击不依赖 quantity／occupied；所有既有可搜索条目使用同一 locator/action service；失败不 materialize、reroll 或泄密。
3. 审查 Drag 与 Ctrl+左键：所有 Revealed root 共享 payload 和 transaction builder；空间 graph 原子；目标 resolver 稳定；右键零写入；无自动装备、排序、swap、split 或未搜索获取。
4. 审查 Shift+1—9：hover／selected target 解析、P6/BaseQuick/QuickUsable gate、P13 单引用事务、Shift 与 P15 Use 隔离、repeat/focus/stale gate正确。
5. 审查布局：所有固定容器按 Capacity 和 SlotIndex 原样渲染；Equip/Unequip／Bind／Search／refresh 不改变未参与 ItemId 的 placement 或视觉 slot；没有 Sort／Compact／occupied-only AddChild 路径。
6. 审查空间容量：戒指／袋均从正式 ChildContainer／Definition 读取容量并渲染准确 cell 数；不存在固定 6／36、单占位框、0 格假容器、品级特例或 UI 创建 child graph。
7. 审查滚动：左右独立 scroll context、thumb drag、wheel routing、offset 保留和 item drag 隔离；scrollbar 不触发物品 intent。
8. 审查权威：P6/P9/P11/P13 同 Owner durable transaction、P5/P8/P15/P17/P20/P21 边界、Code A 窄适配、无 Widget truth、无第二 active route、无历史数据改写。
9. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

10. 因本任务修改 Runtime UI 与输入路由，还必须编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

11. 若编译失败，只修复本任务引入的共享 UI、projection、search locator、drag/quick-transfer、hotbar input、dynamic grid、scroll、include 或调用签名问题。不得为取得通过而恢复旧页面、删减 P1—P21/Fix.P1—P3、写死容量或绕过 Code B transaction。

### 20. Report 与完成信号

生成 `Dev.D.UE.0.0.9BFix.P4_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改和停止 active 写入口的每个文件／资产及职责；
2. Tab、普通容器、尸体页面修复前后的活动调用链，重复玩家 pane／Cell／handler 的收敛方式；
3. `x0` 与搜索无响应的精确根因、Hidden／Searching／Empty／Revealed 模型和统一 search locator；
4. 普通装备、普通 root、空间戒指、吞天袋修复前后的 Drag eligibility 与最终共享 transaction 路径；
5. 右键 Take 如何停止写入，Ctrl+左键的 active destination、稳定候选顺序、拒绝行为和同一 durable commit；
6. Shift+1—9 的 hover／selection 解析、P13 gate、单引用写入，以及与 P15 普通 1—9 Use 的隔离；
7. Equip／Unequip 导致重排的精确根因，SlotIndex render key、增量刷新和未参与物品位置不变的静态证据；
8. 实际解析到的空间戒指／吞天袋 Definition、ChildContainer capacity provenance、不同品级容量和准确 cell 渲染方式；
9. 左右独立 scroll、thumb／wheel 路由、offset 保留与 Drag 隔离；
10. P6/P9/P11/P13、P5/P8/P15/P17/P20/P21、Code A 与 Fix.P1—P3 的权威边界；
11. Editor／Game 两个编译目标、实际命令、最终 native exit code 与关键结果；
12. 未执行的真实验证清单：Tab／普通容器／尸体 UI 等价、所有搜索状态、普通／装备／空间 Drag、Ctrl+左键双向转移、右键零写入、Shift+1—9 绑定、普通 1—9 使用隔离、装备／卸下稳定排列、不同品级容量、吞天袋完整格、双栏 scroll、重开／恢复／terminal、截图、自动化、回归、Smoke、Cook、Package；
13. 确认未读取或采用早期规则、未新建项目、未重置任何 P1—P21/Fix.P1—P3 数据、未启动 `Fix2` 或 F 阶段。

仅当统一 workspace、搜索修复、通用 Drag、Ctrl+左键、Shift+1—9、稳定 Slot、动态空间容量、完整吞天袋 Cells、双栏滚动均已实现并通过静态审查，且 Editor／Game 两项目标均以 native exit code `0` 完成时，使用：

    READY_FOR_0_0_9BFIX2_REPLANNING

若当前范围内仍有可修复的统一 UI、搜索、输入、移动、布局、容量、滚动或编译问题，使用：

    NEEDS_0_0_9BFIX_P4_REWORK

若只能通过新建工程／库存、恢复旧规则、重置历史、改写 Code A 物品权威或越过当前授权才能完成，使用：

    BLOCKED

完成后不得自动开始任何后续任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9BFix.P4","file":"Dev.D.UE.0.0.9BFix.P4_report.md"}
