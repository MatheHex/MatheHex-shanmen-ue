Dev.D.UE.0.0.9B.P4.0.r1_prompt.md
Dev.D.UE.0.0.9B.P4.0.r1
任务身份

项目：Dev.D.UE.0.0.9B

阶段：P4——代码 B 局外仓库／人物配置的真实拖拽、合法目标反馈与快速操作

任务编号：Dev.D.UE.0.0.9B.P4.0.r1

任务性质：P4.0.r0 的同方案输入路径返工；不进入 P5

执行文件：Dev.D.UE.0.0.9B.P4.0.r1_prompt.md

报告文件：Dev.D.UE.0.0.9B.P4.0.r1_report.md

活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B

活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject

工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1

预期引擎：Unreal Engine 5.8；以活动工程真实配置与可用构建环境为准

上一任务：Dev.D.UE.0.0.9B.P4.0.r0

上半部分：只读决策、现状与边界
1. 本返工的策划结论

P4.0.r0 的最终状态是 NEEDS_P4_REWORK，不能以 Controller 或 Repository 自动化通过替代真实 UMG 交互验收，也不能进入 P5。

P4.0.r0 已经提供了以下可保留成果：

Code B 的稳定拖拽 payload、只读 Drop Preview、P4 Interaction Controller 与 QuickMove 规划；

P4 → P3 Controller → P2 Application Service → P1 Repository 的唯一写入链；

Move、Merge、Swap、Equip、Replacement、Unequip、Loaded Spatial 拒绝、Stale Revision 与 QuickMove 的 Controller／事务验证；

P1/P2/P3/P4 合计 57 项 Code B 自动化通过，Editor/Game Development 构建和未启用 Host 的默认地图 Smoke 通过。

但 P4.0.r0 的现场验证同时确认：UCodeBP3CellButton 的真实鼠标点击和双击路径回归。现有 P4 测试直接驱动 FCodeBP4InteractionController，未证明真实 UMG 的命中、按下、拖拽检测、Enter、Leave、Drop、右键菜单或双击能够抵达应有的 UI 回调。

因此本任务只修复并证明该输入路径。不得以新的 Controller 测试、直接调用 NativeOn...、直接调用 P3/P4 Controller、静态截图或测试专用旁路冒充真实鼠标输入。

2. 0.0.9B 不变的系统原则

0.0.9B 的最终目标仍是：

局外仓库配置人物 → 进入地图 → 搜索容器与尸体 → 转移、装备与管理物品 → 撤离 → 同一批真实物品进入局外仓库。

本任务只修复该链条中局外开发态页面的输入表现层，不扩展版本范围。每件真实物品仍只能有一个稳定实例、一个真实父位置与一个统一修改入口；Widget、DragOperation、Hover Preview、菜单、截图和自动化工具都不拥有可写物品真值。

当前仍使用 1×1 物品。多格、旋转、不规则形状、重量、高级嵌套、自动整理与世界物品继续留给后续独立任务。

3. 代码 A／代码 B 的权威边界

0.0.9-XFix1 仍是合法的整体工程开发基线。代码 A 继续负责当前地图、怪物、战斗、默认 UI、玩家、Profile、Run、旧背包、旧 Loot 与结算；代码 B 是 0.0.9B 新物品体系的未来权威，但目前仍是默认关闭的开发态纵向切片。

本任务的唯一可见写入链必须继续是：

UMG／Slate Cell 输入 → P4 Interaction Controller → P3 UI Controller → P2 Application Service → P1 Repository

不得让 Widget、DragOperation、菜单或测试直接写 P1；不得让 P1/P2 依赖 UMG、Slate、Widget、DragOperation、P3/P4 UI 类型或代码 A UI；不得让代码 A 与代码 B 同步、镜像或双写。

本任务不接管 Profile、Run、SaveGame、Loot、地图、玩家 Actor、默认 UI 或正式运行时。

4. 已记录的活动工程非 Code B 变化

P4.0.r0 Report 记录：在 P4 验证期间，活动工程已单独修复一个已授权的代码 A 生产可靠性问题。该修复不属于代码 B 输入返工，且不能在本任务中被静默回滚、重写或扩大。报告列出的现存非 Code B 源码差异是：

Source\demo_map\demo_mapProfileSettlementTransaction.cpp
死亡结算清理失效的 SpatialRingItemInstanceId。

Source\demo_map\demo_mapItemPresentation.cpp
占用格优先显示实际 DisplayName，不再以 WPN 类别占位。

Source\demo_map\demo_mapProfileSettlementTests.cpp
死亡结算清理空间道具布局的回归用例。

Source\demo_map\demo_mapProfileSessionTests.cpp
撤离后兵器装备布局跨完整 Profile Session 重建的回归断言。

由于这四项已记录变化，P4.0.r0 开始时的“非 Code B 源码与 XFix1 零差异”历史断言不再可直接沿用。本任务必须在开始时进行一次精确审计：

以活动工程与 0.0.9-XFix1 的 Source\demo_map 做逐文件 SHA256／等价内容比对；

排除 CodeB 后，确认差异仅为上述四个已记录文件及其确有必要的同一报告／项目资料更新；

确认本次 P4.0.r1 自身不会增加、删除或改写任何非 Code B 生产源码；

运行对应 ProfileSettlement 与 ProfileSession 回归，证明该已记录修复没有被破坏；

若差异清单不符合上述事实、存在来源不明的重叠改动，或修复需要再改代码 A 才能使 P4 可用，停止受影响工作并按本任务 Report 要求说明，不自行合并或清理。

这项审计是为了诚实记录活动基线，不是授权重新处理该生产问题。

5. P4.0.r1 的单一问题

需要解决的是 UI 指针所有权与事件路由，而不是重写 P4 的领域逻辑。

每个可交互格位在同一次原生指针手势中必须有且只有一个清晰的 pointer owner。外层 UUserWidget、内层 UButton、Overlay、Border、拖拽视觉或任何代理元素不得竞争、吞掉或重复处理同一次按下／抬起／双击。

最终实现可以由执行端选择 UMG、Slate 或合理混合，但必须同时满足：

原生左键单击仍进入 P3 的选择、详情和已有非拖拽操作链；

按住并越过实际拖拽阈值时，真实 DragOperation 从占用格创建；

拖拽期间真实命中目标格，并经真实 Enter／Leave 更新只读高亮；

实际 Drop 由 payload 的稳定地址、ItemId 与 Expected Revision 经过现有 P4 → P3 → P2 → P1 链提交；

真实右键／等价菜单和真实双击都能调用同一 QuickMove 链；

双击、拖拽、P3 点击选择与鼠标抬起之间不会把一条手势变成两次物品命令；

空格、禁用 1—9 占位、已关闭页面、UI 外部区域和失效格位不能创建有效 DragOperation 或写入；

Esc、取消、关闭和重新打开页面能清理鼠标捕获、DragOperation、Hover、高亮、菜单与回调。

6. 本任务不做的内容

本任务不要求或不授权：

新增任何 P5 或后续功能；

改写 P1/P2 领域模型、Repository、事务语义、Fixture、Projection、QuickMove 目标规则或 P3 既有功能，除非发现实际输入路径无法安全接入且需要策划新判断；

重写 Code A、撤销第 4 节已记录的生产修复，或以代码 A 结果帮助 Code B 输入通过；

接管正式 Profile、Run、SaveGame、Loot、尸体、普通容器、搜索、世界丢弃、世界拾取、撤离或结算；

新增自动整理、1—9 热键绑定、物品使用、多格、旋转、重量或高级嵌套；

通过测试专用页面、不可见替身 Cell、直接 Controller 调用、直接 NativeOn... 调用、手写本地数组结果或截图伪装真实 UI 输入；

自动开始下一份任务。

下半部分：授权执行内容
7. 本任务单一目标

在不改变已接受的 Code B 数据权威和 P4 交互语义的前提下，恢复并可重复证明默认关闭 Code B 仓库／人物配置页面的真实 UMG／Slate 鼠标输入链。

完成后，真实鼠标单击、按住拖拽、目标 Enter／Leave、Drop、右键菜单、双击 QuickMove、Esc、Close/Reopen 必须能在实际挂载到 viewport 的同一页面上工作；所有真实物品修改仍只可经 P4 → P3 → P2 → P1。

8. 建议的实现边界

执行端应先定位 P4.0.r0 中 UCodeBP3CellButton、外层 Cell／Tile、UButton、命中可见性、NativeOnPreviewMouseButtonDown、NativeOnMouseButtonDown、NativeOnDragDetected、NativeOnMouseButtonUp、NativeOnMouseButtonDoubleClick、NativeOnDragEnter、NativeOnDragLeave、NativeOnDrop 与 DragDropOperation 的实际职责。

可选择下列任一等价方式修复：

令外层 Cell 成为唯一输入 owner，内层视觉 Button 不再接收竞争性命中；

令内层 Button 成为唯一输入 owner，并把 P3/P4 所需事件可靠地转交给同一状态机；

采用明确的 Slate 子控件与命中策略，使每条手势只落到一个所有者。

无论采用哪一种，必须在项目资料和 Report 中说明：

每个事件由哪个对象实际拥有；

可视元素的 Hit Test／Visibility 策略；

单击、拖拽阈值、双击、右键、抬起、取消和 Drop 的互斥规则；

哪些回调只更新本地临时 UI，哪些回调可能提交一次 P2 命令；

为什么同一物理手势无法提交两次命令。

不要为使测试容易而使用与真实页面不同的输入 hierarchy，也不要只在测试构建里切换按钮命中策略。

9. 真实输入状态机要求

实现应至少表达下列行为；具体类名与回调组织由执行端根据真实工程决定。

9.1 单击、双击与拖拽

空占用格的普通左键单击：进入 P3 选择／详情，不写 Repository；

占用格的按下后若未越过拖拽阈值：作为一次单击处理；

占用格的按下后越过拖拽阈值：只创建一次携带稳定值的 DragOperation，不能先执行 P3 Move 或 QuickMove；

真实双击：至多提交一次 QuickMove；允许首击更新无写入的选择态，但不得再触发第二次写入、拖拽或重复菜单命令；

右键：只打开／关闭上下文菜单或由菜单项发起同一 QuickMove 链；右键本身不搬动物品；

空格、禁用 1—9、已销毁或关闭页面、没有 ItemId 的格位：不创建有效 payload，不提交命令。

9.2 Hover、Drop 与失败

只有拖拽已开始时，真实进入目标格才可请求 P4 的只读 Preview；

Leave、取消、Drop、Revision 变化、Close、Reset 与 Shutdown 均清理临时状态；

Drop 必须再次经 P2/P1 验证，成功只采用返回的权威 Projection，失败不得乐观改动 UI；

Loaded Spatial、同源目标、类型不符、满格／不可交换占用、UI 外部、Stale Revision 都必须不写入且显示可理解中文反馈；

Drop、双击或菜单回调的重复事件不得突破“一次手势最多一次命令”的限制。

9.3 生命周期

在 DragOperation 尚未结束时按 Esc、Close、重复 Open、Reset 或 Host Shutdown，必须释放输入与临时对象，不留鼠标捕获、Hover 样式、旧 payload、旧委托或跨页面写入回调。Close/Reopen 后继续使用既定 Host 内存 Repository／Revision 规则，不复制 Repository、Root 或事件绑定。

10. 实际 UI 输入验证

P4.0.r1 的关键证据必须来自真实页面的输入路由。新增或重写自动化时，测试必须：

真实创建并挂载 P3/P4 Host／Root 到 viewport；

使用与正常运行相同的 Cell hierarchy、Hit Test 和输入绑定；

通过 Unreal 可重复的实际 UI 输入方式，把屏幕坐标或等价真实指针事件送入 Slate／UMG 命中路径；

证明事件通过真实命中后的 handler、DragOperation、Enter／Leave、Drop 或菜单／双击路径，而非直接调用 Controller、Widget NativeOn...、Command Adapter 或 P2 Service；

记录可审计的 UI 输入 trace，至少包含手势编号、目标测试 ID／稳定槽位地址、实际触发的 UI 事件、是否创建 DragOperation、Preview 分类、提交次数、P2 结果和最终 Revision；

使用同一份真实 UI 输入 harness 验证 1920×1080 与 1280×720；在缩放或滚动后，目标稳定地址仍必须与视觉目标一致。

可使用 AutomationDriver、Slate 应用层的真实 hit-test 注入、功能测试或其他等价的 Unreal 原生方法。若当前工程没有可安全使用的真实输入 harness，执行端可在 Code B 测试／UI 区域最小新增；但它必须驱动真正挂载页面的命中与回调，不能变成 Controller 单元测试的别名。

截图只在真实 trace 和状态断言已经通过后作为辅助渲染证据，不单独构成验收。

11. 必须覆盖的验证矩阵
11.1 P4.0.r1 新增真实 UI 输入验证

至少逐项覆盖并在 Report 中映射：

默认地图未显式 Open 时不创建 Code B Host、Fixture、P3/P4 页面或 Code B 写入；

显式 Open 后真实左键点击占用格，能经过真实命中到达选择与详情；

空格、禁用 1—9、无 ItemId 格、关闭页面与 UI 外部区域不创建有效 payload；

从仓库占用格真实按住并越过阈值，恰好创建一次含稳定地址、ItemId、Expected Revision 与纯展示快照的 DragOperation；

移动到合法 Move、Merge、Swap／Replacement、Equip／Unequip 目标时，真实 Enter／Leave 产生正确 Preview 与高亮；

拖拽完成一次仓库 ↔ 基础 6 格 Move，并断言 ItemId、Revision 与权威 Projection；

拖拽完成一次兼容堆叠 Merge，验证数量、ItemId 生命周期、详情与选择正确；

拖拽完成一次普通格 Swap 或装备 Replacement，并验证只提交一次原子命令；

拖拽完成合法 Equip 与明确目标 Unequip；空间储物的已支持路径同样从真实 UI 通过；

Loaded Spatial、类型不符、同源、满格／不可交换占用和 UI 外部 Drop 都显示正确反馈且 Repository／Revision 不变；

取消、Esc、失焦或 Close 中的拖拽不写入且清理所有临时状态；

Drop 前制造 Stale Revision，确认旧 payload 不自动重放，刷新后需要重新操作；

真实双击仓库、基础区、空间储物和装备来源的 QuickMove 均只经既定链执行一次，目标顺序与 P4.0.r0 已记录规则一致；

真实右键菜单的 QuickMove 与双击复用同一写入链；打开／关闭菜单与查看详情不写入；

P3 的非拖拽 Move、Equip、Unequip、Replacement、Split、Merge、详情、Close/Reopen、Reset 与 1—9 禁用占位不回归；

同一 physical gesture 的 trace 显示最多一次提交；重复 MouseUp、DoubleClick、Drop 或菜单回调不得形成双写；

1920×1080 完成一次真实拖拽和一次真实 QuickMove，1280×720 也完成同样两类操作，并断言命中槽位与可见槽位一致。

11.2 回归与运行证据

至少完成：

全部 demo_map.CodeB.P1、demo_map.CodeB.P2、demo_map.CodeB.P3、demo_map.CodeB.P4 自动化，以及本任务新增的真实 UI 输入测试；

demo_map.ProfileSettlement 的全部现有回归和 demo_map.ProfileSession.13，以确认第 4 节已记录修复仍然正确；

demo_mapEditor Win64 Development 构建；

demo_map Win64 Development 构建；

未显式启用 Host 的默认地图 /Game/M01/Maps/L_M01_Expedition?Name=Player 启动并正常退出；

1920×1080 与 1280×720 的真实 UI 输入 Smoke；

Fatal、crash、ensure、assert、Automation Controller error 与本任务新增 Error 的独立检索。

如 Automation RunTests 前存在已知 UE unified-error/self-test 初始化诊断，必须将其与选中测试的 LogAutomationController 结果、退出码、真实 UI trace 和本任务 Error 检索分开记录。任何本任务新增的 Error、Fatal、ensure、assert 或 UI 输入失败均不得被该既有诊断掩盖。

12. 可见 Smoke 与截图

在同一真实 UI 输入路径完成可见 Smoke：

打开 Host，真实点击一个物品并显示详情；

从仓库真实拖拽物品，捕获合法目标高亮；

完成一次 Move、Merge、Swap 或 Equipment Replacement；

触发并展示一次不合法目标或 Loaded Spatial 反馈；

用真实双击或右键菜单完成一次 QuickMove；

取消一次拖拽并确认无写入；

Close 后 Reopen，确认既定 Repository／Revision 状态与输入恢复；

正常退出。

至少保存以下可审计材料：

初始页面截图；

正在拖拽且存在真实合法高亮的截图；

成功后的权威 Projection 截图；

失败反馈或 QuickMove 成功截图；

对应的真实 UI 输入 trace／日志；

1280×720 的真实拖拽与 QuickMove Smoke 证据。

截图必须标注其实际分辨率、操作前后状态和对应 trace／日志路径。

13. 项目资料与基线记录

更新活动工程 PROJECT.md、PROJECT_INFO_CARD.md 或等价资料，至少记录：

P4.0.r0 的真实 UMG 输入验收失败，以及 P4.0.r1 是同方案返工；

最终的 pointer owner、Hit Test 策略、单击／拖拽／双击／右键互斥规则；

真实 UI 输入 harness 的入口、日志、trace、测试名称与验证限制；

P4 的 payload、Preview、Drop、QuickMove 仍只通过 P4 → P3 → P2 → P1；

P4 仍未接管 Profile、Run、Loot、SaveGame、地图、玩家 Actor、代码 A UI 或正式运行时；

第 4 节列出的四项已记录代码 A 生产修复及其精确基线审计结果；

P1/P2/P3/P4 自动化、构建、默认地图 Smoke 与截图／日志位置；

P4.0.r1 完成前不得进入 P5。

不得重写 P4.0.r0 或任何先前 Prompt／Report；本任务必须单独归档自己的 Prompt 和同名 Report。

14. 停止条件

遇到下列任一情形，停止受影响工作、生成同名 Report，不以旁路、直调、假页面、本地数组、代码 A 结果或截图掩盖：

真实 UMG／Slate 命中路径无法把单击、拖拽、Drop、双击或右键可靠路由到同一状态机；

修复必须使一条物理手势提交两次命令，或需要跳过 P2 Application Service；

实际 UI 验证只能直接调用 Controller、Widget handler、Command Adapter 或 P2 Service；

P1/P2/P3/P4 回归、构建、默认地图 Smoke 或已记录代码 A Profile 回归失败；

要求改写 P1/P2 领域逻辑、代码 A、正式 UI、Profile、Run、SaveGame、Loot、地图或玩家才能使 P4 可用；

非 Code B 基线审计出现第 4 节之外的来源不明差异；

Close/Reopen、Reset、Shutdown 或取消拖拽留下输入捕获、临时状态、委托、Root、Repository 或跨页面回调；

1920×1080 或 1280×720 无法通过真实 UI 输入完成拖拽和 QuickMove；

活动工程存在与本任务重叠、来源不明的修改。

15. 验收标准

满足以下全部条件，P4.0.r1 才可视为 P4 完成：

P4.0.r0 的真实输入回归已修复，不能再以 Controller 自动化代替真实 UI 验收；

每个 Cell 的 pointer owner、Hit Test 与手势互斥关系明确且在真实页面中生效；

真实单击、拖拽、Enter、Leave、Drop、右键、双击、Esc、Close/Reopen 都经过可审计的 UI 输入路径；

真正的 DragOperation、payload、Preview 和 Drop 不持有第二份可写物品真值；

真实 UI 可完成 Move、Merge、Swap／Replacement、Equip、Unequip 和已支持的空间储物路径；

Loaded Spatial、类型不符、同源、满格、不可交换、Stale、取消和 UI 外部目标均不写入且反馈正确；

双击／菜单 QuickMove 使用既定单一写入链和稳定目标规则，每次手势至多一次提交；

P3 非拖拽操作、详情、Split、生命周期与 1—9 禁用占位不回归；

P1/P2/P3/P4 与 ProfileSettlement／ProfileSession 指定回归通过；

1920×1080、1280×720、Editor/Game 构建和默认地图 Smoke 全部通过；

真实 UI trace、截图、日志、测试映射与 Fatal/Error 检索证据完整；

代码 B 未接管任何正式运行时权威，A/B 不双写；

非 Code B 基线差异与第 4 节已记录的四项修复一致，且 P4.0.r1 未造成额外非 Code B 生产源码改动；

项目资料与同名 Report 已完成归档。

满足时最终 Report 可以使用：

READY_FOR_CODE_B_OUT_OF_RAID_ORGANIZATION

READY_FOR_CODE_B_OUT_OF_RAID_ORGANIZATION_WITH_NONBLOCKING_FINDINGS

若任一真实输入、唯一权威、一次手势一次命令、生命周期、回归、基线审计、构建或默认地图条件未满足，必须使用：

NEEDS_P4_REWORK

NEEDS_PLANNER_DECISION

BLOCKED

不得使用 READY 类状态。

16. Report 要求

生成：Dev.D.UE.0.0.9B.P4.0.r1_report.md

Report 至少包含：

任务身份、最终状态与 P4.0.r0 输入回归的处置结论；

实际新增、修改、保留文件的逐文件清单；

P4.0.r1 的 pointer owner、Hit Test、事件流与手势互斥状态机；

真实 UI 输入 harness 的实现、启动方式、为何不是直调旁路，以及 trace 格式；

单击、拖拽、Enter／Leave、Drop、右键、双击、取消、Esc、Close/Reopen、Reset、Shutdown 的实际结果；

每类真实 Drag／QuickMove 操作的 P4 → P3 → P2 → P1 调用链与一次提交证据；

P1/P2/P3/P4、ProfileSettlement、ProfileSession 的精确测试命令、总数、逐项映射、成功／失败、退出码和日志路径；

1920×1080 与 1280×720 Smoke 的操作链、截图路径、分辨率和对应 trace；

Editor/Game 构建、默认地图 Smoke、Fatal／ensure／assert／Automation Controller error 摘要；

已知引擎初始化诊断与本任务实际错误的区分证据；

第 4 节四项已记录代码 A 修复的逐文件基线审计、哈希／等价结果和回归结果；

代码 A／代码 B 不双写、代码 B 未接管正式运行时的核对；

项目资料更新位置；

与本 Prompt 不同的实现及其等价性；

已知问题、非阻断发现、需要策划判断事项与下一步建议；

最终结论。

Report 存入活动工程信息卡指定的 Task Report 目录；若尚未定义，暂存于：

C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

本 Prompt 原始文件归档到信息卡指定的 Prompt 目录；若尚未定义，暂存于：

C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Prompt

17. 完成与交接信号

完成后不要自动开始 P5 或任何下一份任务。

向策划 Chat 回传并附带且只附带本次同名 Report。正文首行使用：

[CSEMI] {"task_id":"Dev.D.UE.0.0.9B.P4.0.r1","file":"Dev.D.UE.0.0.9B.P4.0.r1_report.md"}

若 ProjectCode、任务编号、Prompt、Report 或目标 Chat 无法对应，停止受影响任务，按 IPF 协议生成 Error001 Report，不自行猜测、改号或切换项目。
