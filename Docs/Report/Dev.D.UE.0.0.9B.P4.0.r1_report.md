# Dev.D.UE.0.0.9B.P4.0.r1 Report

## 任务身份与结论

- Project: `Dev.D.UE.0.0.9B`
- Task: `Dev.D.UE.0.0.9B.P4.0.r1`
- Prompt: `Docs\Prompt\Dev.D.UE.0.0.9B.P4.0.r1_prompt.md`
- Baseline: `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`
- Engine: Unreal Engine 5.8
- Final status: `READY_FOR_CODE_B_OUT_OF_RAID_ORGANIZATION`

P4.0.r0 的 57 项 Controller／事务测试不曾证明真实 UMG Cell 鼠标路由；其真实点击与双击回归已在 r1 修复并以实际 viewport Slate hit-test 验证。r1 未进入 P5 或 P4x，也未接管 Code A 的正式 Runtime、Profile、Run、Loot、SaveGame、地图或玩家权威。

## 输入所有权与一次手势一次命令

- `UCodeBP3CellButton` 是每个生产 Cell 唯一的可命中 pointer owner，显式为 `Visible`。
- 内部 `UButton` 只保留背景与内容，设为 `HitTestInvisible`，不再竞争按下、抬起、双击、拖拽或 Drop。
- Cell 左键按下仅登记 `DetectDrag`；未越过阈值的 MouseUp 走 P3 选择。超过阈值才由 Cell 创建稳定 `UCodeBP4DragOperation`。
- Drag Enter／Leave 只更新 P4 的只读 Preview；Drop 再经 `P4 Interaction Controller -> P3 UI Controller -> P2 Application Service -> P1 Repository` 裁决并刷新权威 Projection。
- 双击与其第二次 MouseUp 使用页面级稳定 ItemId/时间窗去重；QuickMove、Drop、上下文菜单每条物理手势最多一次命令。
- 右键只打开菜单；菜单中的真实 `UButton` 点击才可走同一 QuickMove 链。
- `UCodeBP4DragOperation::DragCancelled_Implementation` 是 Slate Escape 取消的可靠清理点。它只清理非权威的 Preview／菜单／临时状态，绝不写 P1。

`P4.UITrace` 格式包含 `Gesture`、Container／Slot、ItemId、实际 UI 事件、`Commands`、Revision 与 Preview/commit 细节。Trace 文件证明 DragOperation、Enter、Leave、Drop、QuickMove、ContextMenu、DragCancelled 均来自挂载页面的真实命中回调。

## 真实 UI 输入 harness

入口：

```text
CodeB.P4.RunRealInputTrace [expected width] [expected height] [Quit]
```

Harness 显式打开正常生产 P3/P4 Host，使用其已挂载的生产 Cell hierarchy 与 `GetCachedGeometry()` 的屏幕坐标。它先以 `FSlateApplication::LocateWindowUnderMouse` 验证目标 Cell 在真实命中路径上，再送入 `ProcessMouseMoveEvent`、`ProcessMouseButtonDownEvent`、`ProcessMouseButtonUpEvent`、`ProcessMouseButtonDoubleClickEvent` 和 `ProcessKeyDownEvent`。它不调用任何 `NativeOn...`、P3/P4 command/controller 写入接口或 P2 Service；对权威 Projection 的读取仅用于断言结果。

两档独立的真实游戏 viewport 运行：

- 1280×720：`Saved\Logs\Dev.D.UE.0.0.9B.P4.0.r1_real_input_1280.log`
- 1920×1080：`Saved\Logs\Dev.D.UE.0.0.9B.P4.0.r1_real_input_1920.log`

两份日志均以 `P4.RealInputTrace PASS Resolution=...` 结束，且 Fatal/ensure/assert/trace fail 均为 0。

实际覆盖结果（两档分辨率均通过）：

| 路径 | 真实输入和权威断言 |
|---|---|
| 左键选择／详情 | 命中仓库占用格，P3 选择更新，Revision 不变 |
| Move | Warehouse.06 → Basic.00，DragOperation、Enter/Leave、Drop 真实发生，Revision 仅 +1 |
| Merge | 两个 Material.Dust 堆叠合并为 20，保留目标 ItemId，源格清空 |
| Swap | 两个仓库占用格交换稳定 ItemId，Revision 仅 +1 |
| Equip／Replacement／Unequip | Weapon A 装备、Weapon B 替换、再卸至 Basic.00，三条手势各一次事务 |
| 空间内部储物 | 装备 Spatial 后，Warehouse.06 真实拖入 SpatialInternal.00 |
| 拒绝 | Loaded Spatial、类型不符、同源目标均保持 Revision/位置，反馈为中文 |
| 双击 QuickMove | 首击选择、第二个真实 Slate DoubleClick 只提交一次 QuickMove |
| 右键菜单 QuickMove | 右键仅开菜单，实际点击菜单按钮提交一次 QuickMove |
| Escape／Close/Reopen | Escape 取消 DragOperation 并清理 Preview、无写入；真实 Close 后原 Host/repository 可重开 |

默认页面未显式 Open 时不存在 Code B Host/fixture/page 的创建与写入；1—9 禁用占位、空格、已关闭页面与 UI 外部区域没有有效 payload 的生产路径，既有 P3/P4 回归也保持通过。

## 文件清单

修改的 Code B 源：

- `Source\demo_map\CodeB\demo_mapCodeBP3UI.h`
  - 暴露只读 mounted-cell/control 查询给同一生产页面的 Slate harness；为 DragOperation 增加取消回调所有者。
- `Source\demo_map\CodeB\demo_mapCodeBP3UI.cpp`
  - 修复单一 Cell pointer owner/Hit Test 策略。
  - 增加 P4 UI trace、手势去重、DragOperation 取消清理和生产 Slate real-input harness。
  - 上下文菜单 QuickMove 记录实际 commit 结果。
- `PROJECT.md`、`PROJECT_INFO_CARD.md`
  - 更新 r1 状态、输入边界、harness、日志和回归记录。
- `Docs\Report\Dev.D.UE.0.0.9B.P4.0.r1_report.md`
  - 本报告。

保留且未改写：P1/P2/P3/P4 领域规则、fixture、QuickMove 目标规则、Code A 正式 UI/Runtime、Profile/Run/SaveGame/Loot。P4 payload、Preview、DragOperation 与菜单均不保存可写物品真值。

## 基线审计与 Code A 已记录修复

以活动工程和 XFix1 的 `Source\demo_map` 进行逐文件 SHA256 内容比对：总差异 18 个，其中 Code B 为 14 个新增文件；排除 CodeB 后恰为下列 4 个已记录的 Code A 文件，没有其他生产源码差异：

1. `demo_mapItemPresentation.cpp`
2. `demo_mapProfileSessionTests.cpp`
3. `demo_mapProfileSettlementTests.cpp`
4. `demo_mapProfileSettlementTransaction.cpp`

这些是此前已授权的死亡结算 SpatialRingItemInstanceId 清理、真实 DisplayName 显示与对应回归；r1 未增加、删除或改写任何非 Code B 生产源码。

## 构建、回归与 Smoke

| 验证 | 结果 | 证据 |
|---|---:|---|
| `demo_mapEditor Win64 Development` | 成功 | UE Build.bat Exit 0 |
| `demo_map Win64 Development` | 成功 | UE Build.bat Exit 0，`Binaries\Win64\demo_map.exe` |
| `Automation RunTests demo_map.CodeB` | 57/57 | `Saved\Logs\Dev.D.UE.0.0.9B.P4.0.r1_all_codeb_automation.log`，Exit 0 |
| `Automation RunTests demo_map.ProfileSettlement` | 19/19 | `...P4.0.r1_profile_settlement.log`，Exit 0 |
| `Automation RunTests demo_map.ProfileSession.13` | 1/1 | `...P4.0.r1_profile_session13.log`，Exit 0 |
| 默认地图 `/Game/M01/Maps/L_M01_Expedition?Name=Player`，未 Open Host | 成功启动并正常退出 | `...P4.0.r1_default_map_smoke.log`，Exit 0 |
| 真实 Slate/UMG 输入 1280×720 | 通过 | `...P4.0.r1_real_input_1280.log` |
| 真实 Slate/UMG 输入 1920×1080 | 通过 | `...P4.0.r1_real_input_1920.log` |

对上述最终日志的检索结果：Fatal 0、ensure 0、assert 0、Automation failure 0、`P4.RealInputTrace FAIL` 0。

一次并行运行 ProfileSettlement 时，三个 UnrealEditor-Cmd 进程竞争 `Intermediate\CachedAssetRegistry` 临时文件，使 ProfileSettlement.18 因文件移动失败退出；其余测试均通过。这是同一工程并发测试的环境文件锁，而非产品断言失败。停止并发后以相同命令串行重跑，ProfileSettlement 19/19、Exit 0，最终表格和结论只采用该串行有效结果。引擎的非 Win64 SDK 初始化诊断（Mac/iOS/Android/Linux 等不可用）同样与所选 Win64 测试、build 和真实 trace 无关。

## A/B 边界与交接

Code A 继续作为默认地图的正式运行时；Code B P3/P4 Host 仍默认关闭、只在显式开发命令下构造隔离 fixture/service/root。不存在 A/B 镜像、同步或双写。P4 已完成其局外开发态输入验收，但不表示 Code B 已接管正式 Profile、Run、SaveGame、Loot、地图或玩家 Actor。

建议策划以本 Report 为 P4 完成证据后，独立签发下一任务；执行端不会自动启动 P4x、P5 或其他后续工作。
