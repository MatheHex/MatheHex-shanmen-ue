# Dev.D.UE.0.0.9B.P3.0.r0 Report

## 结论

`READY_FOR_CODE_B_DRAG_AND_QUICK_ACTIONS_WITH_NONBLOCKING_FINDINGS`

P3.0.r0 已完成独立、默认关闭的 Code B 仓库 / 人物配置 C++ UMG/Slate 页面。展示层仅消费 P2 的只读 Projection，所有写操作仍经 P2 Application Service/Command Adapter 到 P1 Repository；未接入 Code A 默认游戏、正式玩家、Profile、Run 或 SaveGame。

## 任务身份与边界

- 项目：`Dev.D.UE.0.0.9B`
- Task：`Dev.D.UE.0.0.9B.P3.0.r0`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：`C:\Program Files\Epic Games\UE_5.8`
- Prompt：`Docs\Prompt\Dev.D.UE.0.0.9B.P3.0.r0_prompt.md`
- 活动源码：`Source\demo_map\CodeB\demo_mapCodeBP3.*`、`demo_mapCodeBP3UI.*`、`demo_mapCodeBP3Tests.cpp`
- 结论状态：`READY_FOR_CODE_B_DRAG_AND_QUICK_ACTIONS_WITH_NONBLOCKING_FINDINGS`

P3 只增加 Code B 的开发态可见 UI Host 与页面/controller。Code A 仍是默认地图和正式运行时权威；P3 不读取或写入 Code A 物品数据，也不引入 Profile、Run、SaveGame、Loot、地图或正式 UI 接入，未启动 P4。

对 `Source\demo_map`（排除 `CodeB`）与 `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1\Source\demo_map` 做 SHA256 逐文件比对，差异为 `0`；P3 的源码改动仅在 Code B 与文档。

## 实现

### 默认关闭的 UI Host

- `UCodeBP3UIHostSubsystem` 在默认地图保持惰性、关闭状态；只有显式 `CodeB.P3.Open` 才创建 fixture、P2 Application Service 和一个根 Widget。
- 提供 `CodeB.P3.Open`、`CodeB.P3.Close`、`CodeB.P3.Reset`。重复 Open 不复制 root、delegate 或命令；Close 移除页面并保留 fixture/revision；Shutdown 释放全部 P3 对象并恢复此前输入模式/光标可见性。
- 页面以高层级 Viewport 覆盖展示，但独立于 Code A；默认 Smoke 日志中没有 `CodeB.P3` 记录，证明未自动创建 fixture/page。

### 页面与只读边界

- 真实 C++ UMG/Slate 页面包含“兵器、道袍、饰品 I/II、空间道具、基础物品、局外仓库、空间储物、物品详情”。
- 仓库固定 30 格；基础物品 6 格；空间道具未装备时显示门控说明；空间内部数据来自 P2 Projection。
- 提供 1–9 个禁用的“未接入”快捷栏占位，不接入运行时热键/消耗。
- 详情区展示 Definition、类型、数量、等级/品质、稳定 ItemId、容器/槽位和种子。`FCodeBP3SlotAddress` 只带 ContainerId、SlotId/Index、ItemId、occupied 标记；Widget/controller 不缓存可写物品或数量真相。
- 页面使用固定 1740×960 设计尺寸、`UScaleBox` 和垂直 `UScrollBox`；1920×1080 完整可读，1280×720 缩放/纵向滚动可用。1280 Smoke 中发现关闭标签换行后已修为单行居中，并完成重构建验证。

### 交互与反馈

- 点击槽位选择并刷新只读详情；操作按“源 → 操作 → 明确目标槽位”执行。
- 支持 Move、Equip、显式目标 Unequip、装备替换/Swap、空间内部移动、Split（输入、取消和数量）、Merge。
- 每个修改命令携带当前 expected revision 并由 P2 Service 执行。成功只接受 Service 回传 Projection；失败不做 UI 乐观写入。
- `StaleRevision` 只刷新/清空选择，不重放命令；`LoadedSpatialItemMoveUnsupported` 保留中文显式提示。满格、目标占用、类型不兼容、源不匹配等 P2 失败也映射为中文反馈。

## 自动化与可见 Smoke

全量自动化日志：`Saved\Logs\Dev.D.UE.0.0.9B.P3.0.r0_all_codeb_automation.log`

| 范围 | 数量 | 结果 |
|---|---:|---|
| P1 Repository 回归 | 4 | 通过 |
| P2 Fixture / Projection / Service 回归 | 19 | 通过 |
| P3 UI / Controller | 24 | 通过 |
| 总计 | 47 | `Success=47`、`Failure=0`、退出码 0 |

P3 覆盖：默认关闭、单 fixture、Projection 形状、稳定地址、热键占位、选择/详情、空槽边界、stash/basic 往返、装备/替换/显式卸下、空间往返、拆分取消/成功、合并、full/occupied/incompatible/loaded-spatial/stale、Close/Reopen、重复 Open、Escape 取消及 Shutdown 清理。

可见 Smoke：

- 1920×1080：实际点击 `Warehouse.00 Weapon.A`，选择详情后执行“装备到目标 → Weapon.00”，revision 由 28 到 29；随后 Escape Close 和 `CodeB.P3.Open` 重开，revision 29 与变更后的 Projection 保留。
- 1280×720：显式 Open 后缩放和垂直滚动布局可读；关闭标签单行居中修复后复测通过。
- UI 截图：
  - `Saved\P3Screenshots\P3_Initial.png`
  - `Saved\P3Screenshots\P3_SelectedDetail.png`
  - `Saved\P3Screenshots\P3_MoveSuccess.png`
  - `Saved\P3Screenshots\P3_OccupiedError.png`

## 构建与默认地图 Smoke

全部使用 UE5.8、Win64 Development：

- `demo_mapEditor Win64 Development`：成功。
- `demo_map Win64 Development`：成功。
- 默认地图 Smoke：`/Game/M01/Maps/L_M01_Expedition?Name=Player`，日志 `Saved\Logs\Dev.D.UE.0.0.9B.P3.0.r0_default_map_smoke.log`。
  - `LogNet: Browse` 命中默认地图。
  - `LogExit: Exiting.` 命中。
  - Fatal、ensure、assert、Automation Controller error 均为 0。
  - `CodeB.P3` 命中为 0，默认关闭边界成立。

## 非阻断发现

- UE 自动化启动期间在引擎自带 unified-error/self-test 初始化中打印 13 条 `LogAutomationTest: Error: Condition failed`；发生在 `Automation RunTests` 命令之前。实际选中 P1/P2/P3 测试全数成功、`LogAutomationController` 无 error、最终退出码为 0，未归因于 P3。
- 当前机器缺少 LinuxArm64/VisionOS 可选平台 SDK，UE 平台验证会输出诊断；Win64 Editor/Game 构建、默认 Smoke 和本任务测试不受影响。
- 未执行 cook/package；直接运行未烹饪的 `demo_map.exe` 缺少 shader code library 是既有包装前置条件，未超出本次范围。

## 交付与下一步

已更新 `PROJECT.md`、`PROJECT_INFO_CARD.md` 并归档本 Report。建议下一单任务为 Code B 拖拽与快速动作：仍以 P3 Controller → P2 Application Service → P1 Repository 为唯一写入链，保持 Host 默认关闭，不提前接入 Code A 或持久化。
