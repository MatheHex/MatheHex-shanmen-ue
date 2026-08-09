# Dev.D.UE.0.0.9B.P19.0.r0 Report

状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

## 本轮变更

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp`：将 P14 的唯一 ground-drop writer 扩展为带精确 P7 source container 的 `DropMatchedActiveRunWorldDropItem`。P14 仍以 schema 4 的单一 `WorldDropId`、派生 world container 和单一 root `ItemId` 保存地面真值；没有 child 列表、第二 world inventory 或平行 graph save。
- 同一 Store 新增 P19 closure validation：仅 `Fdemo_mapItemIds::WindTalisman` 与 `Fdemo_mapItemIds::BackpackLevel1` 可携带 child graph；验证稳定 `SpatialChildGuid(parent)`、P17 formal child type/capacity、合法普通 child placement 和一层限制，允许非空 child。simple item 无 child 的 P14 branch 保持原 BaseQuick→world→BaseQuick 行为和旧 record 兼容。
- Store 的 player→world transaction 保留 parent/child/contents 的 stable identity，只移动 root placement 并在同一个 Owner durable replacement 内写同一 record、reconcile P13。world→player 只接受 P1 candidate equality 的 root move：空 BaseQuick 使用 `Move`，空间 parent 可到其匹配且原本为空的 formal equipment slot 使用 `Equip`；不接受 Swap、Merge、Split、QuickMove、clone、flatten 或 child 单独转移。
- P8 `BuildP14PlayerOnlySession` 通过同一 closure validator 移除仍在 world root 下的 parent、child container 和全部 child contents，因此 Extracted、Dead、RecoveredAbandon 均不会把其中任何部分泄漏回 P5。
- `Source/demo_map/CodeB/demo_mapCodeBP3.h/.cpp`、`demo_mapCodeBP3UI.cpp`、`demo_mapCodeBP4.cpp`：保留既有 `GroundDropZone NativeOnDrop` / `WorldDropTarget` 真实拖拽链。地面丢弃成功清除失效选择／pending drag；WorldDropTarget 只挂 root cell、显示完整图根节点状态，且根节点只可拖到空 BaseQuick 或两种 parent 的匹配空装备栏。加载 child 的一般 P7 移动规则未放宽，唯一例外是 WorldDropTarget root 的返回 preview。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：Code A adapter 只复核实际 P7 source cell、转发同一 source container 与安全落点给 Store，并按已提交 projection 刷新现有 Actor；未增加 Code A inventory、graph save、child cache、拾取或终局权威。
- `PROJECT.md`、`PROJECT_INFO_CARD.md`：更新 P19 入口与范围记录；`Docs/Prompt/Dev.D.UE.0.0.9B.P19.0.r0_prompt.md`：归档本次 Prompt。

## 静态边界审查

- P14 simple branch 仍只允许无 child 的 BaseQuick item，并继续使用原 schema 4 root record；P19 仅为两种 exact definition 的完整 closure 增设验证和 matching-equipment route。
- child closure 不会出现在地面 UI、Actor、P14 record 或 P5；root 的 stable ItemId / stable child container / child contents 均不重新生成。失败于 graph、来源、revision、落点、冲突、candidate 或保存时发生在 durable replacement 前，不占 ordinal、不清 P13、不生成新的 authoritative graph。
- player→world 的唯一写入入口仍是已挂载 P7 cell 到 `GroundDropZone::NativeOnDrop`；world→player 的唯一写入入口仍是 `WorldDropTarget` root 的真实 P4/P3 drop。点击、键盘、Actor direct pickup、child cell、无 payload 和目标栏写入均未增加路径。
- WorldDropTarget 是 root-only transient projection；child 仅在 parent 回到玩家 P6 graph 后由下一次权威 projection 按既有 P17 规则显示。P13 只同 commit reconcile，P15 不获得空间 parent 或 child 的新使用资格。
- P5→P6 bridge、rebind/recovery、P9/P10、P11/P12、P16/P17/P18 的来源、receipt、reveal、child creation 和 identity 未改；Code A 仍只管理世界表现／范围／交互转发与既有 P8 后置通知。

## 指定编译

执行命令：

```powershell
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex
```

目标：`demo_mapEditor Win64 Development`  
最终 native exit code：`0`  
关键结果：`Result: Succeeded`（31 actions）。

## 留给 0.0.9B.F 的验证

未启动产品，未执行真实空／非空空间图丢弃、Actor projection、重开、完整拾回、P7 失效／恢复、rebind、P8 三种终局、多分辨率输入、CTA、wrapper、自动化、回归、截图、Smoke、Game Build、Cook、Package 或最终验证。

尚未启动：其他 tier space item、空间道具新来源、child 地面操作、嵌套袋、随机世界 Loot、自动／直接拾取、地面多物品、装备效果、武器／道袍／饰品效果、其他 consumable 与后续 P 阶段功能。
