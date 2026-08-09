# Dev.D.UE.0.0.9B.P3.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P3——代码 B 局外仓库／人物配置可运行 UI 纵向切片
- 任务编号：`Dev.D.UE.0.0.9B.P3.0.r0`
- 执行文件：`Dev.D.UE.0.0.9B.P3.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P3.0.r0_report.md`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 开发基线：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`
- 引擎：Unreal Engine 5.8（以真实可用构建环境为准）
- 前置任务：`Dev.D.UE.0.0.9B.P2.0.r0` 已通过策划验收。

## 已接受前提与不变边界

P1 Repository 是代码 B 唯一可变物品权威；P2 提供确定性 fixture、只读 Projection 与单一 Application Service/Command Adapter。Move、Equip、Unequip、Replacement、Split 与 Merge 必须继续经 P2 到 P1 原子事务；成功递增 Revision，失败保持 Repository、Projection 与 Revision 不变。装有内容的空间道具整体移动继续明确返回 `LoadedSpatialItemMoveUnsupported`。

代码 A 继续支撑正式运行路径。本任务不删除或重写代码 A，不让旧 UI 读取代码 B，不接管玩家、Profile、Run、Loot、SaveGame 或默认地图物品状态，不做 A/B 同步、镜像或双写；原 XFix1 源不得修改。P3 UI 只能持有瞬时选择、焦点、滚动、输入与展示 Revision，绝不能持有可写物品实例、独立数量、父位置或容器真值。

## 单一目标

在 P1/P2 基础上，实现实际可启动、可见、可点击、可自动化验证的局外仓库／人物配置 UI 纵向切片。页面通过明确隔离、默认关闭的 Code B UI Host 使用 P2 Fixture、Application Service 和 Projection，完成布局展示、选择、详情与规定的非拖拽物品操作。交付真实 C++/UMG/Slate 或等价 Unreal UI、可重复启动入口、自动化和渲染证据；不得以线框、伪代码或静态截图代替。

## 页面与交互要求

页面需要有玩家配置区、局外仓库区、物品详情／操作区、状态反馈区。主要中文标签至少包括：兵器、道袍、饰品、空间道具、基础物品、空间储物、局外仓库、物品详情。

- 玩家区显示兵器、道袍、空间道具、至少两个饰品、固定基础 6 格与已装备空间道具内部容器。
- 仓库显示 P2 Fixture 的 30 个稳定 1×1 格，容量可扩展且可滚动。
- 1—9 快捷栏只显示九个清晰的“未接入”禁用占位；不得拥有 ItemId、储物或操作能力。
- `1920×1080` 全部主要区域可用；`1280×720` 仍可通过缩放或滚动完成核心操作。
- 格位只显示合法图标/占位、数量、等级或品质、状态；详情显示稳定 ItemId/DefinitionId、类型、数量、等级、品质、父容器/槽位和现有只读详情。未配置字段不得伪造。
- 单击物品格仅选择并刷新详情；普通模式点空格不写入；选中切换不隐式 Move/Swap；合并导致 ItemId 消失时清理选择。
- 提供统一且可取消的非拖拽操作模式，按稳定来源/目标地址与当前 Expected Revision 提交。

至少从实际 UI 覆盖：仓库↔基础 6 格 Move；合法 Equip；显式目标 Unequip；占用槽合法 Replacement/Swap；仓库/基础/空间内部容器之间 Move；Split 到明确空格；兼容堆叠 Merge；取消操作模式。禁止本阶段实现拖拽、Hover 合法目标、右键/双击快捷、自动整理/快速转移、世界丢弃、快捷栏绑定或使用。

成功后只按返回的新 Projection 刷新；失败不本地改位或手工补回。错误以中文反馈空间不足、目标占用、类型不符、来源已变化、状态已更新、已装物空间道具不支持整体移动。Stale Revision 不得自动重发；须刷新 Projection、清理无效选择并提示重新操作。

## UI Host 与生命周期

实现默认关闭、可脚本化启动的 Code B UI Host。未显式启用时不得创建 P2 Fixture 或页面，不修改默认地图状态。启用后 Host 只能创建一套 Repository、Application Service、Projection Controller 与 Root 页面；关闭再打开维持同一进程内 Repository/Revision；重复打开不得创建重叠页面、重复委托或重复提交。若提供 Reset，只能在隔离开发 Host 中由用户明确触发。退出时清理 Widget、委托、焦点和输入，不向下一进程残留状态，也不得读写代码 A Profile、SaveGame、Run、玩家背包或仓库。

P3 组件可用等价结构实现，但应清晰覆盖 Root、玩家 Panel、仓库/Container Grid、稳定 Cell、Item Tile、Detail Panel、操作/目标选择状态、Split 数量 Dialog 与成功/失败状态区。依赖方向必须是：

`P3 Widget/View Controller → P2 Projection and Application Service → P1 Repository`

P1/P2 不得反向依赖 UMG、Slate、Widget 或 P3 类型。格位地址必须使用稳定 ContainerId/SlotId/SlotIndex，不得依赖像素或临时 Widget 顺序。

## 自动化、Smoke 与资料

重新运行全部 `demo_map.CodeB.P1` 与 `demo_map.CodeB.P2` 自动化。新增不少于 20 项 P3 UI/Controller 验证，覆盖：Host 单例与默认关闭、30 格/装备/饰品/基础/空间展示、禁用快捷栏、重复 Revision 渲染、选择与详情、Move 往返、Equip/Replacement/Unequip、空间内部往返、Split 的输入/取消/成功、Merge 生命周期、错误反馈、Loaded Spatial、Stale Revision、关闭重开/重复打开、Esc、销毁清理和输入恢复。测试必须验证真实 UI 映射与操作链，不能仅重复调用 P2 Service。

使用 P3 Host 做可见 Smoke：打开、选择并显示详情、仓库→基础操作、装备或 Replacement、触发一个合法性错误、关闭重开验证同一内存状态、正常退出。保存至少三张 `1920×1080` 可审计 UI 截图（初始完整页面、选中详情、成功或错误状态），并完成 `1280×720` 布局 Smoke。

完成 Editor/Game Win64 Development 构建、P1/P2/P3 自动化、P3 UI Host Smoke、未启用 Host 的默认地图 Smoke、Fatal/Crash/新增阻断 Error 检查、XFix1 未修改与代码 A 关键 Authority 核对。不要求 Cook、Package、Latest_Demo 或人工试玩。

更新 `PROJECT.md` 与 `PROJECT_INFO_CARD.md`，记录 P3 阶段、P1/P2 验收、Host 启动及默认关闭、组件/权威边界、支持操作、错误和 Revision 规则、P1/P2/P3 入口、Prompt/Report/截图/日志位置和未授权的拖拽/快捷范围。保留历史 I0/P1/P2 资料，不改写旧 Report。

## 停止条件、报告与交接

若 P1/P2 回归失败，UI 需要第二份可写真值、Widget 必须直写 Repository、需要改 XFix1、需要 A/B 双写、无法隔离默认关闭 Host、需要提前接管 Profile/SaveGame/Run/Loot/正式 Authority、生命周期造成重复创建/提交、Stale 只能自动重放、需要伪造未有数据，或构建/自动化/可见 Smoke/默认地图存在无法安全解决的阻断失败，应停止受影响接入并生成同名 Report。

所有验收通过时状态为 `READY_FOR_CODE_B_DRAG_AND_QUICK_ACTIONS`；允许使用 `READY_FOR_CODE_B_DRAG_AND_QUICK_ACTIONS_WITH_NONBLOCKING_FINDINGS`、`NEEDS_P3_REWORK`、`NEEDS_PLANNER_DECISION` 或 `BLOCKED`。

Report 必须写入 `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report\Dev.D.UE.0.0.9B.P3.0.r0_report.md`，列明实际代码/资产、P2 依赖、Host/所有权/组件/映射/操作/反馈/生命周期、测试与 Smoke、截图、构建、A/B/XFix1 证据、资料更新、差异、问题与下一阶段建议。

完成后不得自动开始 P4。向策划 Chat 附带且只附带同名 Report，正文首行：

`[CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P3.0.r0","file":"Dev.D.UE.0.0.9B.P3.0.r0_report.md"}`
