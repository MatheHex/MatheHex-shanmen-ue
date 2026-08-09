# Dev.D.UE.0.0.9BFix.P1

## 任务身份

项目：Dev.D.UE.0.0.9B（同一既有工程；不得新建项目）

修复线：0.0.9BFix

阶段：P1——插入式框架大修复

任务编号：Dev.D.UE.0.0.9BFix.P1

任务性质：等价于原 0.0.9B 的 P22 插入式大改；不是 I、IPF 或新项目

执行文件：Dev.D.UE.0.0.9BFix.P1_prompt.md

报告文件：Dev.D.UE.0.0.9BFix.P1_report.md

活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B

活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject

已接受功能链：0.0.9B.P1—P21

## 上半部分：只读项目裁决、现状与边界

### 1. 唯一有效治理依据

仅以最新的 0.0.9B 大纲、当前活动工程，以及已接受的 0.0.9B.P1—P21 任务链为依据。

0.2 final report、V2、V3 和其他早期版本文档或源码残留，不得读取、引用、继承或用于决定本任务的架构、范围、验收或命名。源码中残留的旧类名、日志、资源、UI 文案只可作为待隔离历史实现处理。

### 2. 本任务在版本链中的位置

0.0.9B.P1—P21 保留为已完成的功能链，不撤销、不重编号、不迁移至其他项目。

Dev.D.UE.0.0.9BFix.P1 是插入 P21 后的框架大修复，产品意义上等价于 P22，但使用独立修复线名以清楚区分“既有功能开发”与“为使其成为可运行产品而进行的框架重建”。

不建立空白 UE 项目，不复制 Content 后伪造迁移，不另建 0.0.9B 分支项目，不使用 I、IPF 或任何新项目编号。

后续正常功能任务在本修复线闭合后再由策划决定；本任务结束不自动开启下一项任务或 F。

### 3. 当前已报告缺陷

用户在实际界面确认：点击 START M01 RUN 后仍进入旧传送页壳并显示 V3 activation failure；返回宗门后仓库被“结束当前 Run 后再整理”错误锁定。

这说明旧启动壳仍在控制世界激活、活动 Run 与仓库锁定。修复必须替换其正式控制权，不能只改文本、隐藏错误、无条件返回成功或要求重启工程。

### 4. 不变的物品与产品边界

Code B P1 Repository 是唯一可变物品真值；P5 为局外持久 Profile，P6 为精确 OwnerId + RunInstanceId 的活动携行图，P8 仅在 Code A 已提交真实终局后处理归还或没收。

P7—P21 的背包、装备、普通容器、尸体、地面物品、快捷引用、消耗品、确定性 Loot 与空间道具图必须保留，禁止回退为旧库存或 A/B 双写。

战备与仓库以塔科夫式闭环作结构参考：局外仓库是可整理的持久真值；战备是同一真值的选择与装备视图；只有明确出战提交才交接真实携行物品；技术启动失败不等于死亡、撤离或放弃。

Code A 仅保留 M01 地图、Actor、战斗和必要生命周期的运行时适配职责；它不得拥有仓库可用性、物品事务、Code B session 或技术失败语义。

## 下半部分：授权执行内容

### 5. 单一授权目标

在既有 Dev.D.UE.0.0.9B 工程内重建 0.0.9B 顶层游戏／Editor 框架：以新的宗门宿主、启动协调器、UI Host、M01 Runtime Adapter 和 Editor Support 取代旧传送页壳对正式流程的控制。

目标是从代码结构上消除“技术 world activation 失败后残留假活动 Run、P6 锁、仓库锁或旧 V3 文案”的根因，同时不丢弃 P1—P21 的 Code B 成果。

### 6. 保护与审计

改动前读取当前工程的 PROJECT.md、PROJECT_INFO_CARD.md、P1—P21 报告及 Source／Config／Content 结构，建立本次重构清单。

在同一磁盘创建时间戳安全快照或等价可回退副本，至少覆盖 Source、Config、Content、Scripts、.uproject、项目文档和 Docs。不得用删除、覆盖或改名现工程代替快照。

若无法安全建立可恢复点，报告 BLOCKED，不得继续重构。

### 7. 新顶层边界与状态机

在当前工程风格下建立清晰职责边界；可在同一 Runtime module 内组织，或建立最小 Runtime／Editor-only module。至少包含：

| 边界 | 职责 |
| --- | --- |
| 0.0.9B Game Framework | 宗门、远征、局内、终局后的顶层状态与页面路由。 |
| Run Start Coordinator | 唯一 StartAttempt、地图解析、世界激活、成功提交与技术失败回退。 |
| Code B Item Bridge | 仅调用 P5/P6/P8 正式契约，不复制物品权威。 |
| M01 Runtime Adapter | 连接既有 M01 地图、Player、Actor、战斗及真实启动／终局回调。 |
| 0.0.9B UI Host | 组装宗门、仓库／战备、远征选择、局内和状态提示；只呈现状态与提交意图。 |
| Editor Support | 启动配置、引用校验、只读诊断和开发验证入口，仅 Editor 可用。 |

建立唯一可审计状态机：

```text
AtSect → PreparingStart → ActivatingWorld → InRun → ResolvingTerminal → AtSect
                       ↘ TechnicalStartFailure → AtSect
```

关键规则：

- AtSect 时仓库／战备可进入，且不应存在活动 P6 session 或活动 Run。
- PreparingStart 仅记录 StartAttemptId 和只读选择；浏览地图或页面不得移动、锁定或复制物品。
- 只有 M01 世界真实激活成功、稳定 OwnerId + RunInstanceId 已确认并由 Coordinator 收到成功回调，才可进入 InRun 并接入既有 P6 成功观察链。
- 激活成功前发生的解析、配置、OpenLevel、World、Pawn、Controller、delegate、加载或准备失败，都必须分类为 TechnicalStartFailure，不得进入 P8 或伪装为玩家 Abandon／Death／Extracted。
- 技术失败的同一 attempt 必须原子清除 transient Run／UI state 和未提交 receipt，恢复到 AtSect；P5 物品图保持等价，不能残留 ActiveRunId、committed P6 session、P8 terminal 或仓库锁。
- 世界已成功激活后发生的 bridge 失败不得回写或清除真实 Code A Run，也不得被伪称为世界激活失败。
- 所有转换记录 StartAttemptId、OwnerId、需要时的 RunInstanceId、前后状态、失败分类与时间顺序；禁止从 Widget 指针、文本、地图名或静态布尔值推断真实 Run。

### 8. M01、宗门、仓库与 Editor 框架重接

追踪旧 START M01 RUN 从宗门 UI 到地图定义、世界加载、delegate、RunId、P6 observer 和返回 UI 的真实调用链，定位导致截图故障的直接根因。

修复真实根因，可修改必要的启动配置、地图软引用、GameMode／World Settings、生命周期 delegate、初始化顺序或旧状态泄漏；不得只替换文本、静默吞错、伪造成功或以 fixture 代替 M01。

默认 Editor Play 与游戏启动应进入新的 0.0.9B 宗门宿主；正式宗门页面不得可达 V3、V2、0.2、Teleport Array 或旧 activation 文案。

宗门主页提供仓库／战备、远征选择与当前状态入口；仓库与战备必须是同一 P5 物品图的视图，不得创建第二份备战库存、starter kit、临时 Loadout fixture 或 Widget-local inventory。

OPEN WAREHOUSE 只由 Coordinator 的真实 InRun 状态禁用。旧 bRunRequested、残留 RunId、旧 callback、旧 manager flag、Widget 存在与地图名均不得作为禁用依据。

将 M01 有效地图、GameMode／World Settings、必要 UI 类与 startup config 显式注册到新框架。Editor Support 必须提供只读校验，能指出缺失引用、旧默认 UI／CTA 仍被默认路径引用，以及最近一次 StartAttempt 的诊断状态；不得提供绕过 P5/P6/P8 的写入入口。

旧地图、敌人、战斗和 Player Actor 可以作为 Adapter 的运行时对象保留，但旧传送页壳必须退出默认启动、M01 CTA 与错误返回路径。不得存在两套可写 session 状态机或两个竞争的正式 Start Run CTA。

### 9. 允许范围

允许新建或最小重构 Runtime、UI Host、Run Start Coordinator、M01 adapter、Editor Support、启动配置、相关 UMG／地图入口和结构化诊断；为精确对接 P5/P6/P8 而增加窄 Bridge／状态查询／未提交 attempt cleanup；更新项目文档和本报告。

### 10. 明确禁止

禁止读取、使用或恢复 0.2 final report、V2、V3 的规则；

禁止删除、重置或重掷 P1—P21、P5／P6 历史或用户物品；

禁止新项目、blank project、伪迁移、全量库存／Loot／战斗重写、第二地图、经济、装备数值、多人、Cook、Package；

禁止让 UI、Editor utility、Widget、fixture 或 debug command 成为物品可写权威；

禁止把技术失败送入 P8，或以重启进程作为唯一恢复方案；

禁止自动启动下一项 P、0.0.9B.F、全量回归、Game Build、Cook 或 Package。

### 11. P 阶段审查与构建

本任务属于 P 阶段。只进行实现、静态代码／配置审查与最终编译，不执行真实运行、点击 CTA、截图、回归、Smoke、Cook、Package 或最终试玩；这些统一保留给 0.0.9B.F。

审查并记录旧启动壳已从默认 0.0.9B 路径隔离、新状态机与 Code B 边界已建立、技术失败不触发 P8 的静态证据。

编译 demo_mapEditor Win64 Development。若新 Runtime 路径影响 Game target，再编译 demo_map Win64 Development。

仅修复本任务引入的编译、配置、adapter 或契约接入问题；不得为得到通过而降级、删除或绕开既有 P1—P21 功能。

### 12. Report 与完成信号

生成 Dev.D.UE.0.0.9BFix.P1_report.md，保存至：

`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report`

Report 必须列出：

- 安全快照的位置与覆盖范围；
- 新增、修改、保留及停止默认入口但未删除的文件／资产；
- 新框架、UI、Editor、Code B 与 M01 adapter 的边界及调用顺序；
- 两项截图故障的直接根因与实际结构性修复；
- 状态机、StartAttemptId／RunInstanceId 与技术失败清理契约；
- P5、P6、P8、仓库锁及物品图被保护的静态证据；
- 塔科夫式战备／仓库原则的具体落点；
- 默认入口、M01 配置、地图引用、GameMode／World Settings 与 Editor Support 的静态校验结果；
- 每个编译目标、命令与最终 native exit code；
- 明确标注未执行的 F 阶段真实运行与验证债务；
- 确认未读取或引用旧 0.2/V2/V3，未丢弃 P1—P21，未新建项目。

仅当框架已成为默认代码／配置入口、旧壳已被正式路径隔离、Code B 与 P1—P21 未被丢弃、边界审查通过且规定编译成功时，使用：

```text
READY_FOR_0_0_9BFIX_P2_OR_F_PLANNING
```

若当前范围内存在可修复的编译或静态审查问题，使用：

```text
NEEDS_0_0_9BFIX_P1_REWORK
```

若无法建立安全快照，或修复必须删除／重置 P1—P21 或改写范围外核心权威，使用：

```text
BLOCKED
```

完成后不得自动开始 0.0.9BFix.P2、常规 P、F 或任何未授权任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

```text
[CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9BFix.P1","file":"Dev.D.UE.0.0.9BFix.P1_report.md"}
```
