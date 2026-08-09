# Dev.D.UE.0.0.9B.P23.0.r0 Report

## 结论

`READY_FOR_P24_PLANNING`

P23 已把正式宗门 Warehouse／Loadout 接入 P4 共享物品工作台。功能范围经静态审查闭合，Editor 与 Game 的指定 Development 构建均以原生退出码 `0`、UBT `Succeeded` 完成。按 P 阶段规则未启动产品，也未执行任何 F 阶段真实验证。

## 实际改动与停用入口

- `demo_map0909BFramework`：正式 `RequestOpenWarehouseFromUI` 改为调用 `Ademo_mapGameMode::Open0909BOutOfRaidInventory`，不再创建或挂载独立 `Udemo_map0909BSectWarehouseWidget`。
- `demo_mapV3ProgressionManager`：真实 P5 Repository／Store 打开共享 `UCodeBP3InventoryWidget` 时注入 `OutOfRaidP5` Workspace Context，以及由正式 Coordinator 状态和 P5 persistent revision 动态解析的只读 gate。
- `demo_mapCodeBP3/CodeBP3UI`：新增共享 Workspace identity/transient context；统一地址、drag、Ctrl+左键、Shift+1—9、hover／selection、active destination 与双栏 scroll policy。
- `demo_map0909BSectWarehouseService`：从可写的第二仓库 service 收敛为 StartAttempt 前从当前 P5 durable graph 重建的只读 LoadoutSelection adapter。
- 旧 `RequestWarehouseDragDrop` 仅为源码／ABI 兼容保留，固定关闭旧页并提示重开共享页面，绝不创建 intent 或写 P1/P5；`ApplyDragIntent`、`Fdemo_map0909BWarehouseIntent`、`ShowWarehouse` 已从活动源码移除。
- 未新建项目、I/IPF、Fix2、第二 P5/P6、第二 Warehouse authority、fixture、Widget inventory 或 Code A mirror。

## P5 改造前后 Call Graph

改造前的正式宗门路径：

```text
SectWidget::ClickOpenWarehouse
  -> Framework::RequestOpenWarehouseFromUI
  -> SectWarehouseService::OpenForSect
  -> Framework::ShowWarehouse
  -> Udemo_map0909BSectWarehouseWidget / WarehouseSlotWidget
  -> custom drag operation
  -> Framework::RequestWarehouseDragDrop
  -> SectWarehouseService::ApplyDragIntent
  -> P1 Repository
  -> P5 Store::CommitAcceptedSnapshot
```

P23 后的唯一正式路径：

```text
SectWidget::ClickOpenWarehouse
  -> Framework::RequestOpenWarehouseFromUI
  -> GameMode::Open0909BOutOfRaidInventory
  -> ProgressionManager::OpenCodeBOutOfRaidInventory
  -> P5 Store::OpenOrMigrate + authoritative Repository
  -> CodeBP3UIHost::OpenProfilePage(OutOfRaidP5)
  -> UCodeBP3InventoryWidget / UCodeBP3CellButton
  -> P4 Drag payload / modifier router / preview+commit
  -> P3 controller -> P2 application -> P1 Repository
  -> P5 Store::CommitAcceptedSnapshot
```

StartAttempt 的只读选择链：

```text
Framework::RequestStartM01FromUI
  -> SectWarehouseService::OpenForSect(current durable P5 snapshot)
  -> CaptureLoadoutSelection
  -> RunStartCoordinator
```

因此可见整理页和 StartAttempt 不再读取旧 Widget 内存图；第二个 P5 写 handler 不在产品路径中。

## Shared Workspace Context 与组件复用

- `FCodeBP3InventoryWorkspaceContext` 仅保存 `Scope`、`OwnerId`、可选 `RunInstanceId`、session revision、Coordinator write gate、玩家／目标 pane identity、active destination、hover、selection 和两侧 scroll offset；不保存 Item、Container 或 placement 副本。
- P5 与 P6/P9/P11 继续共用 `UCodeBP3CellButton`、`FCodeBP3SlotAddress`、`FCodeBP4DragPayload`、同一 pointer modifier router、P2 projection builder、动态 grid、详情／右键只读逻辑和两侧 `ScrollBox`。
- P5 地址明确区分 `OutOfRaidPlayer` 与 `OutOfRaidWarehouse`；局内为 `InRunPlayer`／`ExternalTarget`。payload 同时携带 Owner、可选 Run、scope、ItemId、ContainerId、SlotIndex 和 P1 expected revision。
- 同时只有 Host 内一个 Workspace Context。页面关闭或 Host 重建会清除 active destination、hover 和 selection；snapshot／revision 变化通过现有 projection refresh 和 stale payload 拒绝回到权威图。
- P5 没有搜索遮蔽；沿用 P5 projection 的 `Empty`／`Revealed`，未创建 Hidden／Searching 假状态或假 ItemId。

## Scope、事务与完整图隔离

- `OutOfRaidP5` 仅在 `OwnerId` 有效、`RunInstanceId` 为空、Coordinator 为 `AtSect` 时允许写入。`PreparingStart`／`ActivatingWorld` 返回 `StartAttemptPending` 拒绝；`InRun`／`ResolvingTerminal` 只读。
- 每次 Preview／Commit 与起拖都复核当前 Workspace gate；payload 复核 Owner、Run、scope 与 repository revision。P5 intent 只通过其 P3/P2/P1 controller 和 P5 commit callback；P6/P9/P11 的既有 presentation／durable callback 未改写为 P5。
- 合法移动仍由 P1/P2 裁决 slot semantic、definition compatibility、capacity、stack、unique parent、无环、one-level 与 revision。空间 parent 沿用完整 graph snapshot／candidate commit，不由 UI clone、flatten、创建或拆分 ChildContainer。
- P5→P6 bridge、P8 terminal receipt、P13 binding lifecycle、P15 gameplay Use、P17 graph、P19 WorldDrop、P20/P21 来源均未改变。

## 输入行为

- Drag：所有 P5 Revealed root 使用与局内相同的 `UCodeBP3CellButton -> UCodeBP4DragOperation -> Preview/Commit`；没有按物品类别分流的第二起拖 handler。
- `Ctrl+左键`：warehouse root → 明确激活的合法玩家 `QuickSpatial`／`PouchInternal` child，否则 → P5 BaseQuick；玩家装备、BaseQuick 或 child → warehouse root。候选严格按 SlotIndex 升序先匹配非满同类堆叠，再找空槽；Swap、挤位、自动 Equip/Unequip、Split、Compact 和目标猜测均被排除。
- `Shift+1—9`：UI focus 内 hover 优先、selection 次之；仅 `AtSect`、P5 BaseQuick、quantity > 0、QuickUsable 的现有 ItemId 可调用既有 P13 Bind。P6 仍要求精确活动 Run。普通 `1—9` 始终由打开的工作台消费，不进入 P15 Use。
- 普通左键只作选择或激活玩家 child destination；右键、双击、详情、页面开关和滚动均没有位置写入。

## Stable Slot、动态容量与滚动

- 固定容器继续按正式 `Capacity` 创建全部 Cell，空格也保留原 `SlotIndex`；无 occupied-only AddChild、Sort、Compact 或重编号。
- 空间区由当前装备 parent 的 Definition 与真实 ChildContainer 共同解析；两者容量冲突会显示诊断并拒绝把错误容量当真值。现有正式定义覆盖 WindTalisman `4` 格、四档空间戒指 `6/8/10/12` 格与 BackpackLevel1 吞天袋 `36` 格，页面按完整容量渲染全部格。
- 玩家与仓库 pane 保留两个独立可见纵向 ScrollBox；offset 分别保存在 Workspace transient context，refresh 后各自恢复，不互相覆盖。

## 静态审查

- 正式宗门入口、P5/P6 scope、共享 Cell/Drag/modifier/dynamic grid/scroll、QuickTransfer、P13/P15、Coordinator gate 与旧写入口均逐文件复核。
- `Source` 中无活动 `Fix2`、`ApplyDragIntent`、`Fdemo_map0909BWarehouseIntent` 或 `ShowWarehouse` 引用。
- `git diff --check` 通过。

## 指定构建

Editor：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
Result: Succeeded
Native exit code: 0
```

Game：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
Result: Succeeded
Native exit code: 0
```

## 未执行的 F 阶段验证

本 P23 未启动产品、Editor UI、PIE 或 Standalone；未执行真实鼠标键盘输入、拖拽／Ctrl／Shift 热键交互、截图巡检、Smoke、自动化、回归、试玩、重开／恢复矩阵、Cook、Package 或最终验收。上述真实验证全部保留给 `0.0.9B.F`。
