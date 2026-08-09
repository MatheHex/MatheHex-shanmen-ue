# Dev.D.UE.0.0.9B.P6.0.r4

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P6——Code B 局外 Profile 到活动 Run 库存会话 bridge 的功能开发收尾
- 任务编号：`Dev.D.UE.0.0.9B.P6.0.r4`
- 任务性质：P6 r3 的功能实现与静态代码审查已接受。本任务只补齐该实现最后一次源码修改后的代码编译；不改变功能范围，也不执行任何真实运行验证。
- 执行文件：`Dev.D.UE.0.0.9B.P6.0.r4_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P6.0.r4_report.md`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：`C:\Program Files\Epic Games\UE_5.8`
- 工程级开发基线：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`

---

# 上半部分：只读项目裁决、现状与边界

## 1. 当前阶段规则（覆盖 r3 原真实测试条款）

本项目的工作拆分现正式采用以下分界：

| 阶段 | 负责内容 |
| --- | --- |
| `P` | 功能开发、必要的静态代码审查、代码编译与最小编译修正。 |
| `F` | 真实运行测试、CTA／wrapper、自动化与回归、截图或可见验收、Smoke、Game Build、Cook、Package 及最终验证。 |

因此，`P6.0.r3` 原 Prompt 中的七条真实 CTA、外层 wrapper、negative-control、截图、真实 trace、回归和最终运行审计，全部转为 `0.0.9B.F` 的验证债务；它们不是本任务的验收项，也不得在本任务执行。

P 阶段不是不检查代码：每个完成实现的 P 任务仍须至少通过与其修改范围对应的一次目标代码编译。这里的编译不是产品运行测试，也不替代 F 的验证。

## 2. 已接受的 P6 r3 功能状态

P6 r3 已完成以下功能实现与代码审查，作为本任务只读候选基线：

1. Code A 在其正式生命周期中已把当前绑定旧 Run 标记为 `RecoveredAbandon` 后，Code B 可把同一张已验证 `Prepared` receipt 重绑到第二次真实 CTA 产生的新 RunId；
2. `ReceiptId`、`OriginRunId`、原始 carry payload、ItemId、ContainerId、ChildContainerId、来源 P5 revision 与 payload digest 保持不变；
3. 追加式 `RecoveryRebindHistory` 记录旧／新 RunId、`RecoveredAbandon` 原因、序号与时间；旧 RunId 不被复活、改写或伪装；
4. rebind 仅使用 receipt 内已验证的 carry payload，随后沿原单次提交链完成；不得重新从 P5 snapshot 选取物品或生成第二份 session；
5. Code A 仍拥有默认地图、玩家、正式 Run 生命周期、旧库存、Loot、搜索、世界物品、结算和 Run Save；P6 只接收 Code A 成功激活 Run 后的只读身份与恢复上下文。

P5 的局外 Profile 持久化、P4x 的拖拽／装备规则及 Code A 的正式权威均为既有边界，本任务不重新设计、不迁移、不接管。

## 3. r3 留下的唯一 P 阶段缺口

`P6.0.r3_report.md` 明确说明，最终一次 `restart sidecar read path` 源码修改后未重新执行 Editor 编译。这个缺口不要求重跑 r3 的真实产品测试，但在 P 阶段规则下必须以一次目标编译收尾。

---

# 下半部分：授权执行内容

## 4. 单一目标

在保持 P6 r3 已实现功能与所有权边界不变的前提下，编译 `demo_mapEditor Win64 Development`，并仅在必要时修正由 P6 r3 最终源码状态引起的编译错误。

推荐的编译命令：

    "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

可按工程实际命令行作等价调整，但必须保持目标为 Editor 代码编译，且不得借此启动产品运行或测试 harness。

## 5. 允许的最小处理

允许：

- 读取 P6 r3 修改文件和编译日志；
- 对 P6 r3 引入的编译错误作最小、局部修正；
- 修正相应头文件声明、include、类型、序列化字段或调用签名，使既有 r3 语义可以编译；
- 在修正后重复同一 Editor 目标编译，直至通过；
- 对本轮涉及的 P6 代码做简短静态审查，确认没有把 Code A 的 `RecoveredAbandon` 生命周期决定、Run、Player、Loot、搜索、结算、Run Save 或旧库存权威转移给 Code B；
- 更新 `PROJECT.md`、`PROJECT_INFO_CARD.md` 和本任务 Report，仅记录 P6 r4 的编译收尾与 F 债务。

## 6. 不在本任务内

不得执行或新增：

- `UnrealEditor-Cmd.exe` 产品场景、真实 Sect Teleport CTA、P6 product trace、wrapper、negative-control 或 observer 运行验证；
- 自动化测试、回归、Smoke、截图、可见 UI 巡检、进程残留检查或大规模 SHA-256 审计；
- `demo_map` Game target 编译、BuildCookRun、Cook、Package 或独立程序验证；
- P7／P8／F 的功能、局内背包／I 键、玩家 Actor 物品应用、Loot、尸体／容器搜索、世界物品、拾取／丢弃、1—9、消耗品、撤离、死亡、结算、关闭 Run session 或返回 Profile；
- 改动 P5 正常页面／迁移、P4x UMG 交互语义、Code A Start Run 成功条件、地图加载、Player Actor、旧库存、Loot、搜索、世界物品、结算或 Run Save。

## 7. 编译失败时的处理边界

若编译失败：

1. 先定位第一处与 P6 r3 改动相关的错误；
2. 只修正使现有 r3 设计可编译的最小问题；
3. 不借机改变 Prepared rebind 的产品语义、扩展恢复规则或重构无关模块；
4. 重新执行同一 Editor 目标编译；
5. 若错误证明必须越过本任务边界才能修复，停止受影响部分并在 Report 中说明，不得自行扩展到 P7 或 F。

## 8. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P6.0.r4_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本 Prompt 的 SHA-256 与 P6 r3／r4 的关系；
2. 实际 Editor 编译命令、目标、最终 exit code 与关键编译结果；
3. 本轮新增／修改／未修改的文件，以及每项最小修正的理由；若无源码修正，明确写明；
4. 对 P6 rebind 与 Code A／Code B 权威边界的静态审查结论；
5. 明确说明本轮未运行产品、CTA、wrapper、测试、回归、截图、Smoke、Game Build、Cook 或 Package；
6. 仍留给 `0.0.9B.F` 的真实运行与最终验证债务。

仅当 Editor 代码编译通过且没有越界功能改动时，Report 可使用：

    READY_FOR_P7_FUNCTIONAL_WITH_F_DEBT

若仅存在可在当前范围内继续处理的编译问题，使用：

    NEEDS_P6_COMPILE_REWORK

若受阻且需要新的策划裁决，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P7、P8、F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P6.0.r4","file":"Dev.D.UE.0.0.9B.P6.0.r4_report.md"}
