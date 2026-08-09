# Dev.D.UE.0.0.9B.P4.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P4——代码 B 局外仓库／人物配置的拖拽、合法目标反馈与快速操作
- 任务编号：`Dev.D.UE.0.0.9B.P4.0.r0`
- 性质：在已验收的 P3 默认关闭开发态 UI 上增加真实鼠标拖拽与确定性快速转移。
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 开发基线：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`

## 不变边界

P1 Repository 是代码 B 唯一可变物品权威；所有写入必须继续经过 `P4/P3 Controller → P2 Application Service → P1 Repository`。Widget、DragOperation、Tooltip、菜单、预览和测试工具只能保存稳定地址、ItemId、Expected Revision 与不可变展示快照，不能保存物品实例、容器或 Repository 的可写真值。

代码 A 仍维持默认地图、玩家、Profile、Run、旧背包、Loot、SaveGame 和正式 UI；P4 不得接管、同步、迁移或双写这些状态。未显式执行 `CodeB.P3.Open` 时不得创建 P2 Fixture、P3/P4 Page 或改变默认地图。

P4 只处理单格物品的局外仓库配置交互；不实现自动整理、世界丢弃／拾取、局内 Loot、搜索、快捷栏 1—9 绑定、多格／旋转、重量、Profile／SaveGame 或 P5。

## 要求的 P4 交互

- 从占用格开始真实 Unreal 拖拽；payload 至少包含来源稳定地址、ItemId、开始 Revision、会话标识及只读展示快照。
- Hover/Enter/Leave 显示合法 Move、Merge、Swap／Replacement、Equip／Unequip 或中文拒绝状态；它们不写入领域状态。
- Drop 时重新经 P2/P1 以 Expected Revision 裁决；成功只用返回 Projection 刷新，失败／取消／Stale 不乐观写入或自动重放。
- 支持空储物格 Move、完整兼容堆叠 Merge、普通储物格 Swap、装备和已占用装备栏 Replacement、明确目标 Unequip，以及仓库／基础 6 格／空间内部储物之间的操作。
- 已装载内容的空间道具整体移动必须保持 `LoadedSpatialItemMoveUnsupported`；不改变子容器或复制内容。
- 非法类型、满格／不可交换占用、同源、过期 Revision、禁用 1—9、空格和 UI 外部区域必须反馈且不写入。
- 取消、Esc、关闭、重开、Reset、Shutdown 与重复回调均清理临时样式、payload、输入和回调，不重复提交。

## 快速转移规则

双击占用格必须调用与右键／等价上下文菜单相同的 P4 应用层入口；菜单显示“快速转移”“查看详情”，菜单本身不写入。

- 仓库来源：按槽位索引，优先基础 6 格中第一个兼容且未满堆叠；否则第一个空格。
- 基础 6 格、空间内部储物或装备栏来源：按槽位索引，优先局外仓库中第一个兼容且未满堆叠；否则第一个空格。
- 已装载空间道具整体移动继续拒绝；无合法目标显示中文原因且 Revision、数量、ItemId 和 Projection 不变。

快速转移不得写入空间装备槽、饰品槽、兵器栏、道袍栏或 1—9 占位；不得把多个 Move 拼成伪原子的“整理”。

## 验证与交付

重新运行全部 `demo_map.CodeB.P1`、`P2`、`P3` 与新增 P4 自动化；验证真实 DragOperation/payload/预览/Drop 路由及 QuickMove。完成 Editor/Game Development 构建和默认地图 `/Game/M01/Maps/L_M01_Expedition?Name=Player` 的未启用 Host Smoke。

用开发 Host 保存初始页、合法拖拽高亮、成功 Projection、失败或 QuickMove 结果至少四张截图；在 1920×1080 与 1280×720 完成真实拖拽和 QuickMove Smoke。更新 `PROJECT_INFO_CARD.md`、`PROJECT.md`，生成同名 Report 后只向策划部回传该 Report，并使用：

```text
[CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P4.0.r0","file":"Dev.D.UE.0.0.9B.P4.0.r0_report.md"}
```
