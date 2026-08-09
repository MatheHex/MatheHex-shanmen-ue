# Dev.D.UE.0.0.9BFix.P1 Report

## 结论

`READY_FOR_0_0_9BFIX_P2_OR_F_PLANNING`

本次在既有 `Dev.D.UE.0.0.9B` 工程中完成插入式顶层框架修复。0.0.9B 宗门宿主、唯一启动协调器、M01 运行时适配、Code B 窄桥接、UI Host 与只读 Editor Support 已成为正式入口；历史运行实现被收束在 `Ademo_mapGameMode` 的单一 Code A 兼容缝内，不再暴露给新产品框架。

本任务未启动产品、未点击 CTA、未截图、未跑自动化/回归/Smoke、未 Cook 或 Package；真实验证债务保留给 `0.0.9B.F` 或策划后续明确下发的任务。

## 安全快照

- 快照：`C:\AIDev\shanmen-ue\Snapshots\Dev.D.UE.0.0.9B_FixP1_20260808_1406`
- 覆盖：`Source`（330 files）、`Config`（5）、`Content`（485）、`Scripts`（33）、`Docs`（64）、`demo_map.uproject`、`PROJECT.md`、`PROJECT_INFO_CARD.md`。
- 逐项文件数/文件长度已与活动工程比对一致；未通过删除、覆盖或改名活动工程来建立恢复点。

## 框架与文件清单

### 默认框架及其职责

- `demo_map0909BFrameworkTypes.h`：唯一顶层状态词汇和包含 `StartAttemptId`、`OwnerId`、`RunInstanceId`、前后状态、失败分类及序列的结构化诊断。
- `demo_map0909BFramework.{h,cpp}`：宗门宿主与 UI Host，装配可见宗门入口并将仓库与远征意图路由给协调器；不持有物品权威。
- `demo_map0909BRunStartCoordinator.{h,cpp}`：唯一 `AtSect → PreparingStart → ActivatingWorld → InRun` 调度者，以及 `TechnicalStartFailure → AtSect` 清理路径。
- `demo_map0909BM01RuntimeAdapter.{h,cpp}`：仅检查已配置 M01 世界、PlayerStart、Pawn 与 GameMode，并调用窄 Code A M01 激活契约；不写 P5/P6/P8。
- `demo_map0909BCodeBItemBridge.{h,cpp}`：仅在 Coordinator 已确认真实世界和输入恢复后通知 P6；Bridge 拒绝是审计事件，不能撤销真实 Code A Run。
- `demo_map0909BEditorSupport.{h,cpp}`：只读检查默认地图、默认 GameMode、M01 包存在性、运行时适配准备状态、已退役默认 Widget 和最近一次协调器诊断。
- `demo_map0909BSectWidget.{h,cpp}`：新的宗门 UI。远征和仓库按钮只在 Coordinator 的真实 `AtSect` 状态可用，Widget 不保存库存、Run 或事务状态。

### 修改的兼容与入口文件

- `demo_mapGameMode.{h,cpp}`：新增 `Prepare0909BRun`、`Activate0909BM01World`、`Rollback0909BPreparedRun`、`Observe0909BConfirmedRun`、P5 打开与返回回调等窄适配契约。历史 M01 实现只保留在该 Code A 边界内；新框架不再直接引用其类型。
- `demo_mapPlayerController.cpp`：保留并使用受控的新 Run 输入恢复：清除宗门 UI 遗留的 Move/Look ignore 栈后才建立 GameOnly 输入面，避免“世界已激活但输入仍被旧 UI 锁住”的假技术失败。
- `Config/DefaultEngine.ini`：静态核对为 `GameDefaultMap` 与 `EditorStartupMap` 均指向 `/Game/M01/Maps/L_M01_Expedition`，`GlobalDefaultGameMode` 指向 `demo_mapGameMode`。

### 保留但停止默认入口的内容

P1—P21 的 Code B Repository、P5 Profile、P6 精确 Owner/Run session、P7—P21 背包/装备/容器/尸体/地面物品/快捷栏/消耗/Loot/空间图全部保留，未重置、删除或双写。

历史运行实现、地图、敌人、战斗与 Player 仍作为 M01 Adapter 的 Code A 运行时对象保留，但历史宗门页壳已不再是默认启动、M01 CTA 或错误返回入口。没有新建项目、分支工程、第二库存、starter fixture 或第二个可写 session 状态机。

## 截图缺陷的直接根因与结构性修复

### 根因

旧启动链把“建立活动 Run”与“世界/输入已确认”混在同一前置流程：世界或输入恢复随后失败时，活动身份可能已存在。与此同时，旧宗门输入锁可留下引擎级 Move/Look ignore 计数，使世界已准备完成后仍被错误归类为启动失败。结果是技术失败路径可遗留假活动 Run/P6 锁，从而让仓库显示“结束当前 Run 后再整理”。

### 修复

1. Coordinator 以 `StartAttemptId` 记录 `Preflight → Prepare → Activate M01 → RestoreInput → Observe P6`。只有前四步全部成功才进入 `InRun` 并通知 P6。
2. 任一世界确认前的失败均进入 `TechnicalStartFailure`；如已产生 transient Run，则仅走技术回滚，随后以 OwnerId 静态查询确认没有活动 P6 session，才回到 `AtSect`。
3. 技术失败路径没有 P8/终局调用，也不会伪装为 Abandon、Death 或 Extracted；P5 物品图不被该路径改写。
4. 输入恢复先清理旧 UI 遗留的 ignore 栈，再恢复 GameOnly，因此 UI 锁不能再制造“激活失败”。
5. P6 Bridge 被移动到世界确认后的单向观察位置。其结果不再能重写或清除已确认的 Code A Run。

## Code B、P5/P6/P8 与仓库锁保护

- Code B Item Bridge 只接收 Coordinator 传来的精确 `OwnerId + RunInstanceId`；它不复制 P5/P6 图，也不拥有物品事务。
- `Prepare0909BRun` 在 Code A 兼容缝内临时保存已确认身份；`Observe0909BConfirmedRun` 会复核两项 ID 后才发出 P6 观察，并不以 Bridge 成败决定部署。
- 技术回滚后，Coordinator 通过 P5 存储根调用 `HasActiveRunInventorySession` 核验没有 P6 session 才恢复 `AtSect`。
- 新宗门仓库按钮仅由 Coordinator 的真实 `AtSect` 使能；不以 Widget 存在、旧布尔值、残留 RunId 或地图名推断可用性。
- 新框架静态审查未发现终局/结算写入口；技术失败分支只包含回滚、P6 absence verification 和 UI 状态恢复。

## 战备/仓库产品原则

局外 P5 仓库是唯一可整理的持久物品图；战备只是该图的选择和装备视图。新宗门 UI 不创建第二库存或 widget-local inventory。只有由 Coordinator 确认的真实 M01 出战才交接精确 P6 携行图；技术启动失败既不是死亡、撤离，也不是放弃，因此不触发 P8。

## 静态审查结果

- 新 `demo_map0909B*` 框架源码不包含历史页壳标识、历史 CTA 标识或本次之前的 I 系列任务标识。
- 新框架源码没有 `RequestSettlement`、终局请求或 P8 写入口。
- 新框架通过 `Ademo_mapGameMode` 的窄适配接口访问 Code A；新 UI、Coordinator、Bridge、M01 Adapter 和 Editor Support 均不直接暴露历史运行管理器类型。
- 默认地图和 GameMode 配置已静态匹配，Editor Support 在运行时只读验证这些入口以及已退役默认 Widget。

## 编译

| 目标 | 命令 | Native exit code | 结果 |
| --- | --- | ---: | --- |
| Editor | `Build.bat demo_mapEditor Win64 Development demo_map.uproject -WaitMutex -NoHotReload` | 0 | `Succeeded` |
| Game（受新的 Runtime 入口影响） | `Build.bat demo_map Win64 Development demo_map.uproject -WaitMutex -NoHotReload` | 0 | `Succeeded` |

没有为取得通过而删除、降级或绕过 P1—P21 功能。

## F 阶段验证债务

未执行：真实产品启动、M01 CTA、技术失败注入、截图、自动化、回归、Smoke、Game 试玩、Cook、Package 和最终验收。后续仅在策划明确下发的 `0.0.9B.F` 或修复线后续任务中处理。

## 治理确认

本次只基于当前 0.0.9B 工程、PROJECT 文档和已接受 P1—P21 报告进行边界审计；未采用任何早期版本的规则、命名或验收依据；未新建项目，未丢弃 P1—P21。

`READY_FOR_0_0_9BFIX_P2_OR_F_PLANNING`
