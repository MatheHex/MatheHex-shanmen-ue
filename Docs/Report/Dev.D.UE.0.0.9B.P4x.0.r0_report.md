# Dev.D.UE.0.0.9B.P4x.0.r0 Report

## 1. 任务身份与结论

- 项目：`Dev.D.UE.0.0.9B`
- 任务：`Dev.D.UE.0.0.9B.P4x.0.r0`
- Prompt：`Docs\Prompt\Dev.D.UE.0.0.9B.P4x.0.r0_prompt.md`，已与用户原件 SHA-256 `ECF21D11B78965F74B013DEA42CB1BC4EBFFB0E9EB5FA81AAC5497175F52E91B` 一致归档。
- 前置：`Dev.D.UE.0.0.9B.P4.0.r1` 已由策划验收为 `READY_FOR_CODE_B_OUT_OF_RAID_ORGANIZATION`；其真实 Slate/UMG 输入 trace 已核对。
- 最终状态：`READY_FOR_CODE_B_OUT_OF_RAID_ORGANIZATION_WITH_NONBLOCKING_FINDINGS`

P4x 已移除正式 Start Run 对旧备战布局、备战装备、空间引用及 Widget 的运行时门槛；收紧 Code B 为真实拖拽唯一的位置写入入口；区分空间戒指快捷空间与空间储物囊非快捷空间。Code B 仍是默认关闭的开发态切片，未接管 Code A 的 Profile、Run、SaveGame、Loot、地图、玩家 Actor 或默认 UI。

唯一非阻断项见第 11 节：现存 P4 截图是 P4x 前资产，未作为本次证据；用户要求优先代码审查，因此本次以最终构建后的真实双分辨率 Slate/UMG trace 与静态入口审计为主证据。

## 2. 启动链审计与去备战化

实际产品链为：

`Ademo_mapV3ProgressionManager::StartPreparedProfileRun` → `Fdemo_mapProfilePreparationFlow::StartPreparedRunDirect` → `Udemo_mapProfileSessionSubsystem::StartRunWithoutPreparation` → 既有 Session/Runtime/地图激活。

- `demo_mapV3ProgressionManager.cpp` 保留产品地图与玩家可用性检查，但隐藏遗留备战 UI 后直接走 Flow 的 Direct 方法；产品入口没有 `StartPreparedRunThroughWidget` 调用。
- `demo_mapProfilePreparationFlow.cpp` 的 Direct 方法不接收 Widget，也不读取/提交 `PreparationLayout`。
- `demo_mapProfileSessionSubsystem.cpp` 构造 `bRequireCommittedPreparationLayout=false` 的请求并记录 `P4X_START_RUN_DIRECT ... preparation_layout_ignored=1`。
- `demo_mapProfileBeginRunTransaction.cpp` 在 Direct 请求中从空的请求拥有布局构造本次运行计划，不读 Profile 的遗留布局；该布局仅用于本次不可变 Runtime plan，且 Direct 路径不回写/重置遗留字段。
- `demo_mapProfileRepository.cpp` 将 `PreparationLayout` 视为历史工具保留的透明元数据。其失效 ID、过期空间戒指或非标准旧 Hotbar 长度不再令一个其他字段合法的 Profile 无法加载或阻断 Start Run。显式历史 Prepared Run/布局提交事务仍自行验证明确选择。

因此未伪造装备、未清空 Profile、未初始化 Code B，也未吞掉地图或运行时错误。

## 3. Start Run 证据

新增 `demo_map.ProfileNormalStartup.17.P4xDirectStartIgnoresPreparationLayout`：

1. 写入含失效旧武器 ID、失效旧空间戒指 ID、长度为 2 的旧 Hotbar 的受控 Profile；
2. 不构造 Preparation Widget、不选择装备，初始化现有 Flow；
3. 直接 Start Run 成功，Runtime 部署物品为空；
4. 遗留布局仍按字节语义保留且未被 Direct 路径重置；
5. Extraction 后再次 Direct Start Run 成功，RunId 更新且没有重新引入备战门槛。

命令：

```text
UnrealEditor-Cmd.exe demo_map.uproject /Game/M01/Maps/L_M01_Expedition?Name=Player -ExecCmds="Automation RunTests demo_map.ProfileNormalStartup.17; Quit" -unattended -nop4 -nosplash -NullRHI -log
```

退出码 `0`，`1/1` 成功，两个 `P4X_START_RUN_DIRECT` 标记。日志：`Saved\Logs\Dev.D.UE.0.0.9B.P4x.0.r0_direct_start.log`。

默认地图最终 Smoke：

```text
UnrealEditor-Cmd.exe demo_map.uproject /Game/M01/Maps/L_M01_Expedition?Name=Player -game -unattended -nop4 -nosplash -NullRHI -ExecCmds="Quit" -abslog=...P4x.0.r0_default_map_smoke.log -log
```

退出码 `0`；地图生命周期标记 `2`；`P4.RealInputTrace`、`P4.UITrace`、`CodeB.P1`—`CodeB.P4` Host 标记均为 `0`；Fatal/ensure/assert 为 `0`。这证明默认启动未显式 Open Host，也未由 Start Run 初始化 Code B fixture/repository/page。

## 4. 拖拽优先规则与代码映射

`Source\demo_map\CodeB\demo_mapCodeBP3UI.cpp/.h`：

- 每个真实格位仍由 P4.0.r1 的 `UCodeBP3CellButton` 作为 pointer owner；唯一位置提交在 mounted `NativeOnDrop` 中经 `FCodeBP4InteractionController::CommitDrop` 完成。
- 普通 Click、DoubleClick、RightClick 只激活选择/详情或反馈；右键文本明确为“仅用于查看详情”。
- 已移除可达 Move/Merge/Swap/QuickMove/放回操作按钮、右键写入项、双击写入路由，以及旧截图用的 Console 成功写入捕获函数。
- 详情只按真实位置显示一个语义提示：未装备的兼容项显示“装备（拖到明确装备栏）”；已装备项显示“卸下（拖到明确储物目标）”。两个按钮只写反馈，不提交位置事务；多饰品/已占装备目标仍要求实际 Drop。
- 静态最终审计：旧 QuickMove Binding `0`、UI 内 `BeginOperation` 直调 `0`、`CommitDrop` 只有实际 `NativeOnDrop` 一处。

`Source\demo_map\CodeB\demo_mapCodeBP2.cpp/.h`、`demo_mapCodeBP4.cpp`：普通仓库、基础 6 格、快捷内部格与储物囊内部格接受全部物品类型；仅装备栏裁决 Weapon/Armor/Accessory/Spatial.Ring 类型和原子 Replacement。P1/P2 继续保持容量、占用、同源、Revision、唯一 ItemId、循环与 Loaded Spatial 安全规则。取消、Esc、Close/Reopen、Stale、无效 Drop 都不乐观写入。

## 5. 两类空间容器

受控 Fixture、Projection、UI 和测试均明确区分：

| 类型 | 定义／容器 | 规则 |
|---|---|---|
| 空间戒指 | `Spatial.Ring` → `SpatialItemId` → `QuickSpatial` | 仅空间戒指装备栏可接收；未装备不显示内部，装备后显示“快捷空间（空间戒指）”。 |
| 空间储物囊 | `Spatial.Pouch` → `SpatialPouchItemId` → `PouchInternal` | 普通空间物品，可在任意普通储物位置；页面明确标为“非快捷储物囊”，无 1—9/QuickMove/使用语义。 |

每个 Item 的 Child ContainerId 和内部 ItemId 都是 P1 Repository 中唯一实例。空容器可真实拖拽；两类容器已有内部内容时，Move/Swap/Equip/Unequip 均由 `LoadedSpatialItemMoveUnsupported` 原子拒绝，位置与 Revision 保持不变。

`demo_map.CodeB.P4.SpatialPouchPolicy` 覆盖空囊普通移动、囊拒绝进入空间戒指装备栏、物品拖入 PouchInternal，以及 Loaded Pouch 整体移动拒绝。

## 6. 真实输入链与一次手势一次提交

`CodeB.P4.RunRealInputTrace [width] [height] [Quit]` 打开正式 P3/P4 Host，以生产格位 `GetCachedGeometry()` 坐标通过 `FSlateApplication` 命中并注入 Pointer/Drag/Drop/DoubleClick/RightClick/Escape/Close。它没有直调 Widget NativeOn、P3/P4 Controller 或 P2/P1 写接口；投影只用于断言。

最终 trace 覆盖真实 Move、Merge、Swap、Equip、Replacement、Unequip、空间戒指内部、非快捷储物囊内部、Loaded Ring/Pouch、装备类型拒绝、同源拒绝、双击无写入、右键无写入、Esc、Close/Reopen。每个接受的物理拖拽只记录一次 P2/P1 事务和一次 Revision 递增。

| 分辨率 | 结果 | 日志 |
|---|---|---|
| 1280×720 | `P4.RealInputTrace PASS Resolution=1280x720`；Failure/Fatal/ensure/assert `0` | `Saved\Logs\Dev.D.UE.0.0.9B.P4x.0.r0_real_input_1280.log` |
| 1920×1080 | `P4.RealInputTrace PASS Resolution=1920x1080`；Failure/Fatal/ensure/assert `0` | `Saved\Logs\Dev.D.UE.0.0.9B.P4x.0.r0_real_input_1920.log` |

## 7. 自动化、构建与回归

| 验证 | 结果 | 退出码／日志 |
|---|---:|---|
| `Automation RunTests demo_map.CodeB` | 58/58 | 0；`...P4x.0.r0_codeb_automation.log` |
| `Automation RunTests demo_map.ProfileNormalStartup.17` | 1/1 | 0；`...P4x.0.r0_direct_start.log` |
| `Automation RunTests demo_map.ProfileSettlement` | 19/19 | 0；`...P4x.0.r0_profile_settlement.log` |
| `Automation RunTests demo_map.ProfileSession.13` | 1/1 | 0；`...P4x.0.r0_profile_session13.log` |
| `demo_mapEditor Win64 Development` | 成功 | 0 |
| `demo_map Win64 Development` | 成功 | 0，最终二进制 `Binaries\Win64\demo_map.exe` |
| 默认地图未 Open Host Smoke | 成功 | 0；`...P4x.0.r0_default_map_smoke.log` |
| 真实 Slate/UMG trace 1280×720 / 1920×1080 | 通过 / 通过 | 两份 `...real_input_*.log` |

构建命令：

```text
Build.bat demo_mapEditor Win64 Development demo_map.uproject -WaitMutex -NoHotReload
Build.bat demo_map Win64 Development demo_map.uproject -WaitMutex -NoHotReload
```

## 8. 错误检索

对第 7 节七份最终日志逐份检索：`Test Completed Result={Fail}`、`P4.RealInputTrace FAIL`、`Fatal error:`、`Assertion failed:`、`Ensure condition failed:`、`LogAutomationController: Error:`、`P4X_*Error` 均为 `0`。

四个 Automation Commandlet 日志在引擎启动时各有 UnifiedErrorTest 的 `LogAutomationTest: Error: Condition failed` 诊断；它们发生在测试发现/执行之前，最终 AutomationController failure 为 `0`，不是本任务错误。

## 9. 起止基线审计与 A/B 边界

- 起止均以 `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B` 为活动工程；该目录及上层没有 `.git`，因此无法作 Git diff。改以逐文件 `rg` 审计、构建、自动化和独立日志审计。
- Code B 改动限于 Fixture/Container role、P2/P4 规则、P3/P4 UMG 输入与 P1/P2/P3/P4 测试。
- 非 Code B 生产改动仅限 `demo_mapV3ProgressionManager.cpp`、`demo_mapProfilePreparationFlow.cpp`、`demo_mapProfileSessionSubsystem.cpp/.h`、`demo_mapProfileBeginRunTransaction.cpp`、`demo_mapProfileRepository.cpp`，目的仅为删除正式 Start Run 的旧备战依赖。相应 NormalStartup/ProfileSettlement/ProfileSession/FullSystem 断言调整为区分 Direct 路径和历史显式 Prepared Run 路径。
- 未修改战斗、怪物、地图规则、奖励价值、Loot、SaveGame Schema、正式默认 UI 或 Code A/Code B 双写边界。
- 项目资料已更新：`PROJECT.md`、`PROJECT_INFO_CARD.md`。

## 10. 交付物索引

- Prompt：`Docs\Prompt\Dev.D.UE.0.0.9B.P4x.0.r0_prompt.md`
- 本报告：`Docs\Report\Dev.D.UE.0.0.9B.P4x.0.r0_report.md`
- 最终 trace/回归日志：`Saved\Logs\Dev.D.UE.0.0.9B.P4x.0.r0_*.log`
- P4x UI 截图不引用旧 `Saved\P4Screenshots` 文件；它们早于本任务，不能代表当前页面。

## 11. 已知问题、非阻断发现与后续

非阻断发现：当前 `Saved\P4Screenshots` 的 P4 文件均早于 P4x；一次无写入初始截图请求在自动退出窗口内未完成，因此未将它们作为当前 UI 证据。为避免重新引入任何 Controller 直调的隐藏写入入口，本任务没有恢复旧 CaptureSuccess/CaptureError/QuickMove 截图命令。用户已要求优先代码审查而非截图测试；当前双分辨率真实输入 trace、自动化、构建和静态入口审计构成主证据。

后续只能由策划部另行下发。不得自动开始 P5 或任何后续 Prompt。

## 12. 最终结论

P4x 通过：正式 Start Run 已不受旧备战数据/装备/UI 阻断；Code B 普通储物与装备栏规则已按拖拽优先收紧；空间戒指快捷空间与空间储物囊非快捷储物已分离；真实输入、回归、构建和默认地图 Smoke 均通过。状态为：

`READY_FOR_CODE_B_OUT_OF_RAID_ORGANIZATION_WITH_NONBLOCKING_FINDINGS`
