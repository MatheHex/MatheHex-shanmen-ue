# Dev.D.UE.0.0.9BFix.P4 Report

## 结论

`READY_FOR_0_0_9BFIX2_REPLANNING`

P4 授权范围已完成：局内 Tab、普通容器和尸体搜索页收敛为同一物品 workspace；四态 Cell、搜索、通用 Drag、`Ctrl + 左键`、`Shift + 1—9`、稳定 Slot、动态空间容量与左右独立滚动均已接入。Editor／Game Win64 Development 最终均为 native exit code `0`、UBT `Succeeded`。本轮未执行任何真实产品或交互测试。

## 文件与职责

- `Source/demo_map/CodeB/demo_mapCodeBP3.{h,cpp}`：增加 `Empty/Hidden/Searching/Revealed`、稳定 scope/Owner/Run/Container/Slot 地址和不含隐藏 ItemId 的 search locator；权威地址按真实 SlotIndex 查找。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.{h,cpp}`：单一 workspace、共享 Cell builder、player/external panes、hover/selection、通用 transfer resolver、Ctrl-left、Shift-number、动态空间区、双 ScrollBox 与 offset 保留；移除可点击 Hotbar bind/unbind UI 路径。
- `Source/demo_map/CodeB/demo_mapCodeBP4.{h,cpp}`：统一 payload 增加 source scope 与精确 Owner/Run；取消 loaded spatial parent 的 UI/P4 特例拒绝，所有 Revealed root 进入同一 Preview/Commit。
- `Source/demo_map/CodeB/demo_mapCodeBP2.{h,cpp}`：停止 `LoadedSpatialItemMoveUnsupported` active gate；P2 继续只把通用请求交给 P1。
- `Source/demo_map/CodeB/demo_mapCodeBP2Tests.cpp`：旧拒绝枚举的编译定义改为 parent 移动后 parent/ChildContainer/child placement 身份不变的断言；未执行测试。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：P10/P12 search callback 改为验证持久化 exact Run target locator 后才在 Store projection 内解析 ItemId；容器/尸体页挂载同一活动 P13 Hotbar presentation。
- `PROJECT.md`、`PROJECT_INFO_CARD.md`：更新当前任务、边界、状态和 F 债务。
- 未修改资产、地图、Actor、Loot Profile、搜索时长、P5/P6/P8 durable schema、P15 Use、战斗、生命、经济、多人或 Code A 权威。

## 活动调用链与根因

1. 页面链：现有 Tab／P10／P12 均进入 `UCodeBP3UIHostSubsystem::OpenProfilePage → UCodeBP3InventoryWidget`；旧实现虽共用 Host，却在 `BuildPageContents` 内复制 normal/body builder，并用一个外层 ScrollBox 包住三列。
2. Hidden `x0`／无响应：normal/body builder 与 `Is*SlotProtected`、`Request*ItemSearch` 同时要求 P2 `bOccupied` 和隐藏 `ItemId`；被遮蔽的条目因此落入 Empty/quantity 文本或不能通过点击门槛。修复后 projection 以 exact OwnerId、RunInstanceId、TargetId、ParentContainerId、SlotIndex、target revision、ActiveActionId 表示 search locator，Cell 点击不依赖 quantity、occupied 或隐藏 ItemId；action timer、interrupt、completion 仍走既有 P10/P12 Store service。
3. 空间 parent 不可移动：UI 对 `WindTalisman/BackpackLevel1` 分支限制目标，P4 `IsLoadedSpatialItem` 和 P2 `IsLoadedSpatialItemMoveUnsupported` 又二次拒绝。三处 active 特例已停止；P1 candidate 与 P10/P12 composite owner replacement 继续原子验证完整 ChildContainer graph、唯一 parent、无嵌套及 stale revision。
4. 重排根因：旧 builder 按 `Container.Slots` 数组遍历序号 `Index` AddChild；刷新时 `ClearChildren` 后重建整列，视觉 key 没有 scope/Owner/Run。现按 Capacity 创建 `0..N-1`，每格按真实 `SlotIndex` 查 placement；稳定地址包含 scope、Owner、Run、ContainerId、SlotIndex，ItemId 仅在 Revealed 时附加。Equip/Unequip、Bind、Search、revision 不再改变未参与 item 的视觉槽位，scroll offset 也在重建前后保留并由 ScrollBox clamp。

## 统一交互

- 普通、尸体根／装备、玩家装备、BaseQuick、戒指 child、储物囊 child 与 WorldDrop root 使用同一 `UCodeBP3CellButton`、四态规则和 `FCodeBP4DragPayload`。Hidden/Searching 无详情、数量、定义、ItemId 或 drag payload；Revealed 才可拖动。
- Drag/Drop 共用 `PreviewInventoryTransfer → FCodeBP4InteractionController::PreviewDrop/CommitDrop → P3 → P2 → P1 → 当前 P6/P9/P11 composite commit`。外部 pane 内部整理、尸体固定装备位写入、Hidden/Searching target 和 WorldDrop target 写入均零提交拒绝；Definition 只通过 P1 equip/category eligibility 生效，没有两种空间 Definition 特例。
- `Ctrl + 左键` 在 pointer 入口一次消费，不再触发 selection、search、drag 或 P15。外部→玩家选择当前明确玩家 storage，否则 BaseQuick；玩家→打开的 normal/body root；纯玩家 workspace 使用 BaseQuick 与当前可访问空间区。单 destination 内先按 SlotIndex 升序尝试合法同类 Merge，再按 SlotIndex 升序尝试 Empty；不 Swap、不 Split、不挤位、不自动装备；没有候选则零写入。
- 右键仅打开只读 detail context；不存在 Take/QuickMove 位置写入。旧可点击 Hotbar bind/unbind 控件已从 mounted UI 删除。
- `Shift + 1—9` 只在 active P6 workspace、UI focus、非 repeat keydown 处理；目标顺序为 hovered Revealed address，其次 selected address。Host 再验证当前 P6 BaseQuick、ItemId、quantity > 0、`bQuickUsable` 和 P13 editable exact Run 后调用唯一 Bind callback。UI 打开时普通 1—9 被消费但不使用物品，未修改 P15 gameplay Pressed Use。

## 布局、容量与滚动

- 所有固定容器按权威 Capacity 建完整 Cell 数，按真实 SlotIndex 放置；无 Sort、Compact、occupied-only AddChild 或固定 6/36 cell 模板。
- 玩家 pane 同时渲染装备、BaseQuick、可访问的戒指 ChildContainer 与储物囊 ChildContainer。标题来自 parent 正式 DisplayName/Quality，并显示 used/capacity；Cell 数最终来自 ChildContainer Capacity，且与 `RingQuickCapacity/TotalCapacity` 交叉验证，不一致时显示结构化诊断并拒绝伪造布局。
- 玩家与目标各有独立、可见、可拖 thumb 的纵向 `UScrollBox`；各自保存 offset。滚轮/thumb 由各自 Slate subtree 处理，不进入 Cell pointer intent。

## 静态审查与编译

- `git diff --check`：通过。
- 旧 active 特例检索：`LoadedSpatialItemMoveUnsupported`、`WindTalisman`／`BackpackLevel1` UI 分支、`NotTwoColumnTransfer`、P20 body spatial 特例均为 0；仅保留历史 smoke 文案中的 “QuickMove” 字样，无 mounted write route。
- 统一调用器首次未启动 UBT：其 `cmd /c call` 对带空格 `Build.bat` 的引号转义失败；未计为源码编译结果。本任务未越权修改基础脚本，按 Prompt 明示命令直接调用，并将该项留给后续 I-stage foundation remediation。
- 首次实际 Editor 编译发现旧 P2 test 枚举引用与新 UMG 严格编译告警，已修正；未执行该测试。
- 最终 Editor：`Build.bat demo_mapEditor Win64 Development -Project=...\demo_map.uproject -WaitMutex -NoHotReload`；exit `0`，`Result: Succeeded`。
- 最终 Game：`Build.bat demo_map Win64 Development -Project=...\demo_map.uproject -WaitMutex -NoHotReload`；exit `0`，`Result: Succeeded`。

## 明确保留给后续 Fix2／F 的真实验证

未启动产品、Editor、PIE 或 Standalone；未点击 Tab／普通容器／尸体；未执行搜索、普通／装备／空间 Drag、Ctrl-left 双向转移、右键零写入、Shift+1—9、普通 1—9 隔离、装备／卸下稳定排列、不同品级容量、完整储物囊格、双滚动、重开／恢复／terminal；未截图、未运行自动化、回归、Smoke、试玩、Cook、Package 或最终验收。

P4 Report 回传后停止；不自动开始 P5、`0.0.9BFix2` 或 F。
