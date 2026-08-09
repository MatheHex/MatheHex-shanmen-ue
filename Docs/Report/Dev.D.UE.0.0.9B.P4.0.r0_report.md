# Dev.D.UE.0.0.9B.P4.0.r0 Report

## 最终结论

`NEEDS_P4_REWORK`

P4 已具备独立的 Code B 拖拽规划、Drop 预览、P2 → P1 提交链和 QuickMove 代码；构建及 57 项 Code B 自动化均通过。但本轮代码审查结合实际 UI 输入验证发现：`UCodeBP3CellButton` 的真实鼠标点击/双击路径发生回归，实际点击未到达对应操作。现有 P4 自动化直接驱动 Controller，并没有证明原生 UMG 的按下、拖拽、Enter/Leave/Drop 与点击路由可用。

因此不得使用 `READY` 状态，也不得把截图或 Controller 测试替代真实交互验收。本报告是按 P4 停止条件提交的真实进度/返工报告，不自动开始下一 P 任务。

## 任务身份与实际范围

- 项目：`Dev.D.UE.0.0.9B`
- 任务：`Dev.D.UE.0.0.9B.P4.0.r0`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：`C:\Program Files\Epic Games\UE_5.8`
- Prompt：`Docs\Prompt\Dev.D.UE.0.0.9B.P4.0.r0_prompt.md`
- Report：`Docs\Report\Dev.D.UE.0.0.9B.P4.0.r0_report.md`

本任务只扩展默认关闭的 Code B 开发态仓库/人物配置页面；没有把 Code B 接入正式 Profile、Run、Loot、SaveGame、地图、玩家 Actor 或代码 A 默认 UI，也没有建立 A/B 双写。

## P4 实际实现与所有权

### 文件清单

- `Source/demo_map/CodeB/demo_mapCodeBP4.h`：定义纯值 `FCodeBP4DragPayload`、只读 `FCodeBP4DropPreview`、Drop 分类和 `FCodeBP4InteractionController`。
- `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`：从 P2 Projection 规划 Move、Merge、Swap、Equip、Replacement、Unequip、Loaded Spatial 拒绝和 QuickMove；提交仍调用 P3 Controller。
- `Source/demo_map/CodeB/demo_mapCodeBP4Tests.cpp`：10 个 P4 Controller/事务验证。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h/.cpp`：P4 的 `UCodeBP4DragOperation`、格位 Enter/Leave/Drop、拖拽视觉、右键菜单、双击 QuickMove、Esc/关闭清理和状态颜色。

P1、P2 的 Repository、Projection 和 Application Service 源码未为 P4 改写。P3 UI 文件作为 P4 的可见输入层被扩展；其原有非拖拽流程不能视作已通过 P4 回归，因为当前格位指针路由需要返工。

### 调用链与非权威边界

`UMG Cell / UCodeBP4DragOperation → FCodeBP4InteractionController → FCodeBP3UIController → P2 Application Service → P1 Repository`

- payload 只包含稳定来源地址、`ItemId`、`ExpectedRevision`、会话 ID 与 Definition/数量/品质展示快照；明确不含 Widget 指针、Repository、Container 或可写 ItemInstance。
- Hover 只读取 Projection 生成 `FCodeBP4DropPreview`，不写状态；Drop 用 payload 的 revision 再经 P2/P1 原子裁决。
- 取消、Esc、Close、Shutdown 清除 payload/预览/菜单临时状态；Stale Revision 拒绝旧 payload，不自动重放。
- QuickMove 的稳定规则为：兼容且未满的堆叠按槽位升序优先，其次首个空槽；仓库与基础 6 格之间使用同一 Controller 入口，右键菜单复用该入口。

## 验证证据

### 已通过的代码级验证

日志：`Saved\Logs\Dev.D.UE.0.0.9B.P4.0.r0_all_codeb_automation.log`

| 范围 | 数量 | 结果 |
|---|---:|---|
| P1 Repository | 4 | 通过 |
| P2 Fixture / Projection / Service | 19 | 通过 |
| P3 UI / Controller | 24 | 通过 |
| P4 Controller | 10 | 通过 |
| 合计 | 57 | `Success=57`、`Failure=0` |

P4 十项覆盖 payload 稳定值、空来源拒绝、Move、完整堆叠 Merge、Swap、装备替换、Unequip/类型拒绝、取消与 Stale、QuickMove 目标顺序、Loaded Spatial 与关闭页面拒绝。日志中 `LogAutomationController` 的 P4 测试均为 `Result={Success}`，本任务 Error/Fatal/ensure/assert 检索计数为 0。

Editor/Game Win64 Development 构建此前均已成功；默认地图 Smoke 日志 `Saved\Logs\Dev.D.UE.0.0.9B.P4.0.r0_default_map_smoke.log` 命中 `/Game/M01/Maps/L_M01_Expedition?Name=Player` 和 `LogExit: Exiting.`，未显式打开 Host 时无 `CodeB.P3`/`CodeB.P4` 初始化记录。

### 未通过的验收门槛

`demo_mapCodeBP4Tests.cpp` 直接实例化 `FCodeBP4InteractionController`，不能覆盖实际 UMG 指针命中。对 `UCodeBP3CellButton` 的近期实现审查显示，左键处理同时涉及 `NativeOnPreviewMouseButtonDown`、`NativeOnMouseButtonUp`、`NativeOnMouseButtonDoubleClick` 以及可视 `UButton` 的命中设置；实际页面点击未执行对应操作。这使真实 DragOperation 创建、目标高亮、Drop 路由、双击 QuickMove 和 1280×720/1920×1080 交互 Smoke 均不具备可接受的通过证据。

已存在的 `CodeB.P4.CaptureInitial`、`CaptureHighlight`、`CaptureSuccess`、`CaptureError`、`CaptureQuickMove` 仅是捕获入口。它们不能证明真实鼠标输入路径，故不作为 P4 通过依据。

## 本轮并行修复（范围外，已透明记录）

用户在 P4 验证期间报告“战斗结束后装备武器显示为初始化/仓库空置”。经代码审查已单独修复：

- `Source/demo_map/demo_mapProfileSettlementTransaction.cpp`：死亡结算也清理失效的 `SpatialRingItemInstanceId`，避免准备布局保留已丢失的空间道具引用而被 Profile Session 校验拒绝。
- `Source/demo_map/demo_mapItemPresentation.cpp`：占用格优先显示实际 `DisplayName`，不再将装备武器显示为 `WPN` 类别占位。
- `Source/demo_map/demo_mapProfileSettlementTests.cpp`：增加死亡清理空间道具布局的回归用例。
- `Source/demo_map/demo_mapProfileSessionTests.cpp`：增加撤离后武器装备布局跨完整 Profile Session 重建的持久化断言。

此修复使用代码审查与自动化验证：`demo_map.ProfileSettlement` 19/19 通过、`demo_map.ProfileSession.13` 通过、Editor Development 构建通过。它是用户授权的生产可靠性修复，不是 P4 Code B 交互通过的替代证据；因此活动工程的非 CodeB 源不再满足 P4 开始时“与 XFix1 零差异”的历史断言，需由后续任务按该真实状态继续审计。

## 返工建议与下一单任务

建议策划部只下发一个 P4 输入返工任务，目标是恢复并可自动化验证真实 UMG 事件链：

1. 统一格位的 pointer owner，避免外层 `UUserWidget` 与内层 `UButton` 竞争/吞掉同一次输入；保留 P3 选择、P4 拖拽和双击 QuickMove 的互斥状态机。
2. 增加 Widget 级自动化或等价真实输入 harness，逐项证明按下、开始拖拽、Hover/Leave、Drop、右键菜单、双击、Esc、Close/Reopen 均真正经过 `UCodeBP4DragOperation` 和 Drop 路由，而非只调用 Controller。
3. 在真实 UI 路径跑仓库↔基础 6 格、Merge、Swap/Replacement、Equip/Unequip、Loaded Spatial 拒绝、Stale、取消、QuickMove，并在 1920×1080、1280×720 各完成一次交互 Smoke。
4. 返工完成后重新提交 P4 最终 Report；在此之前不要开始 P5，也不要把控制器成功视作 UI 验收成功。

## 最终状态

`NEEDS_P4_REWORK`

已提交的可靠事实是：P4 的权威数据边界和 Controller/事务测试通过；真实 UMG 输入验收未通过。后续必须先修复该输入回归并补齐真实 UI 交互测试。
