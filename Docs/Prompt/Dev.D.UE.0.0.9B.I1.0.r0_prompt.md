# Dev.D.UE.0.0.9B.I1.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：I1——0.0.9B 代码与 UE Editor／游戏框架重建
- 任务编号：Dev.D.UE.0.0.9B.I1.0.r0
- 任务性质：框架重建与启动缺陷修复，不是对旧传送页做文字补丁
- 执行文件：Dev.D.UE.0.0.9B.I1.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.I1.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 引擎：C:\Program Files\Epic Games\UE_5.8
- 工程级开发基线：Dev.D.UE.0.0.9-XFix1
- 已存在的 0.0.9B 物品域：Code B P1—P21

---

# 上半部分：只读项目裁决、现状与边界

## 1. 当前唯一治理依据

后续设计、实现、验收和任务拆分只以最新 Dev.D.UE.0.0.9B 大纲、当前活动工程和已经接受的 0.0.9B 任务链为依据。

0.2 final report、V2、V3 及其他早期版本文档不具备任何规划、范围、验收或架构决策效力，不得读取、引用、继承或据此决定本任务。工程源码中若仍有旧类名、日志、资源或 UI 文案，它们只可被识别为待隔离的历史实现，不是有效规则。

## 2. 已报告的真实缺陷

用户在当前活动工程的实际界面中确认了同一启动链的两项缺陷：

1. 从宗门页点击 START M01 RUN 后，画面仍进入旧传送页壳，并显示：

       V3 world activation failed and was rolled back as a technical ActivationFailure, not a player Abandon.

   这不是允许保留的降级体验。需要找出真实的 M01 远征世界激活失败点，并以 0.0.9B 的启动框架修复它；不得只替换显示文本。

2. 随后的宗门页显示“结束当前 Run 后再整理”，使“前往仓库整理 / OPEN WAREHOUSE”被错误锁定。技术启动失败被错误残留为活动 Run；玩家没有成功进入远征，也没有主动放弃，因此此锁定和任何可能的物品占用都不合法。

这两项缺陷共同证明：当前 UI、启动状态、世界激活、活动 Run 与 Code B P5/P6 的边界仍被旧运行壳主导，不能继续以局部补丁维持。

## 3. 本轮的总体裁决

在不丢弃已完成 Code B P1—P21 物品域成果的前提下，重新建立一个由 0.0.9B 自己拥有的：

- 游戏启动与宗门状态框架；
- M01 远征选择、发起、激活、失败回退与返回流程；
- 局外仓库／战备入口和局内锁定边界；
- UE Editor 启动、地图／UI 配置、可视化调试和框架验证入口。

旧地图、敌人、战斗、Player Actor、M01 内容和稳定资产可以作为运行时适配对象保留；它们不得继续拥有 0.0.9B 的页面状态、启动结果、Run 锁定或物品权威。

本任务中的“Editor 框架”指 UE 工程内的 Editor 启动与验证框架、0.0.9B 内容入口、配置和开发调试能力；不要求制作给玩家使用的关卡编辑器或外部工具。

## 4. 不变的 0.0.9B 物品权威

1. Code B P1 Repository 仍是新物品体系唯一可变真值。
2. P5 是局外持久 Profile；P6 是精确 OwnerId + RunInstanceId 的活动携行图；P8 只在 Code A 已提交真实终局后处理 P6 归还或没收；P9、P11、P14 是独立的 Run-local 残余。
3. 已实现的 P7—P21 个人背包、装备、普通容器、尸体、地面物品、快捷引用、消耗品、确定性 Loot 和空间道具图不得被删除、重置、回退为旧库存或改成 A/B 双写。
4. 代码 A 可以继续作为 M01 地图／战斗／Actor／输入／旧内容的窄运行时适配层；它不得在 0.0.9B 正式路径中决定仓库可用性、Code B session、物品移动、远征 UI 或启动后的技术失败语义。

## 5. 战备与仓库的产品参考

只要涉及战备和仓库，使用“塔科夫式物品闭环”作为辅助思考，而不是照搬美术、名称、武器、战斗、货币或商业设计：

- 局外仓库是唯一可整理的、持久的真实物品库；
- 战备是从同一仓库中选择并装备实际物品的视图，不创建第二份“备战库存”；
- 点击地图或查看远征信息不移动、锁定或复制物品；
- 只有一次明确、可审计的出战提交，才把该次携行物品交给活动 Run；
- 技术启动失败不是撤离、死亡或放弃：玩家局外图必须完整可用，仓库立即可进入；
- 成功进入远征后，仓库才因真实活动 Run 锁定；撤离、死亡或已确认放弃后，再按既有 P8 唯一终局恢复可整理状态。

---

# 下半部分：授权执行内容

## 6. 单一授权目标

以当前 0.0.9B 工程为基础，建立新的 0.0.9B 游戏／Editor 顶层框架，替代旧传送页和旧启动壳对正式流程的控制。修复 M01 的真实世界激活路径与技术失败回滚，使默认 Editor/游戏入口直接进入 0.0.9B 宗门框架，并保证技术激活失败后不会留下活动 Run、P6 锁、物品扣除、错误仓库锁定或任何面向用户的旧版本文案。

这是一次有保护的框架重建：保留 Code B P1—P21 的领域成果和 M01 内容；重新组织它们的顶层宿主、运行时协调、UI 入口和 Editor 验证，不把旧壳继续包装成 0.0.9B。

## 7. 先保护当前工程

1. 在改动前读取当前活动工程的 PROJECT.md、PROJECT_INFO_CARD.md、已接受 P1—P21 报告和当前 Source／Config／Content 结构，建立可审计清单。
2. 在同一磁盘上创建一个带时间戳的只读可回退快照或等价安全副本，至少覆盖 Source、Config、Content、Scripts、.uproject、PROJECT.md、PROJECT_INFO_CARD.md 和 Docs。不得删除、覆盖或改名当前活动工程来代替快照。
3. 若磁盘空间、工程完整性或现有并行写入使安全快照不能建立，停止后续重建并报告 BLOCKED；不得在无可恢复点时做破坏性重构。
4. 旧工程目录、旧报告和旧实现保留为审计材料，但不得作为新的正式框架入口或规则来源。

## 8. 建立 0.0.9B 顶层代码框架

### 8.1 代码边界

在现有工程风格下建立清晰的 0.0.9B 顶层边界。可采用同一 Runtime module 内的明确目录／命名空间，也可建立最小的 Runtime 与 Editor-only module；具体文件名由真实工程决定。最低限度应有以下职责边界：

| 边界 | 职责 |
| --- | --- |
| 0.0.9B Game Framework | 宗门、远征、局内、终局后的顶层状态与页面路由。 |
| 0.0.9B Run Start Coordinator | 单一的 Start Attempt、地图解析、世界激活、成功提交、技术失败回退。 |
| Code B Item Bridge | 只以正式 P5/P6/P8 契约读取或调用 Code B；不复制或重建物品权威。 |
| M01 Runtime Adapter | 只连接既有 M01 地图、Player、Actor、战斗和必要的真实世界激活回调。 |
| 0.0.9B UI Host | 宗门、仓库／战备、远征选择、局内、提示的页面组装与输入路由；UI 只呈现状态和提交意图。 |
| 0.0.9B Editor Support | 默认启动配置、地图／资产引用验证、Editor 可视化诊断、框架验证入口；只在 Editor 环境可用。 |

不得要求全工程立即重命名历史类。若旧命名的运行类仍需被 M01 adapter 调用，必须由新 coordinator 包装并限制在 adapter 内；不得使其 UI、状态机或错误文本直接暴露给 0.0.9B 正式页面。

### 8.2 明确的顶层状态机

建立一个单一且可审计的 0.0.9B 会话状态机，至少表达以下状态或精确等价状态：

    AtSect → PreparingStart → ActivatingWorld → InRun → ResolvingTerminal → AtSect
                           ↘ TechnicalStartFailure → AtSect

规则：

1. AtSect：仓库／战备可进入；远征可选择；不应存在活跃 P6 session 或活动 Run。
2. PreparingStart：只保存本次 StartAttemptId 与只读选择；点击地图、停留在远征页或关闭页面都不改变 P5/P6。
3. ActivatingWorld：只允许当前唯一 attempt 请求 M01 adapter 解析并激活世界。任何旧 RunId、旧 delegate、旧 UI callback 或过期 attempt 都不能写入新状态。
4. 只有真实的 M01 world activation 完成、稳定 OwnerId + RunInstanceId 已被确认、且新 coordinator 明确收到成功回调后，才可进入 InRun 并通知既有 P6 成功观察链。
5. 在 world activation 成功前的任何地图解析、配置、OpenLevel、World、Pawn、Controller、delegate、加载超时或准备失败，都只能进入 TechnicalStartFailure。它不是 Abandon、Dead、Extracted、RecoveredAbandon 或 P8 终局。
6. TechnicalStartFailure 必须以同一 attempt 原子清理：取消或清除 transient Run／UI state；确保不存在活动 RunId；确保 P6 没有 committed active session；通过既有 Code B 合法事务取消任何未提交 Prepared receipt 或恢复其来源；P5 revision、物品 GUID、数量、位置、装备、快捷引用均保持开始前等价状态；随后回到 AtSect。
7. 已完成真实 world activation 后发生的 Code B observer／bridge 失败，不得伪装为“世界激活失败”。此类情况必须保留 Code A 已成功启动的真实 Run，并以可审计的受控错误处理；不得因为 bridge 错误回写或清除 Code A Run。
8. InRun 才能锁定仓库。回到 AtSect 后仓库应立即恢复可用，且旧锁定 UI、P6 session 或 transient attempt 不能残留。

所有状态转换都需要 StartAttemptId、OwnerId、需要时的 RunInstanceId、前后状态、失败分类和时间顺序的结构化诊断记录。不可从显示文字、Widget 指针、地图名称或静态布尔值反推真实 Run 状态。

## 9. M01 世界激活的实际修复

1. 先追踪当前 START M01 RUN 的真实调用链，从 0.0.9B 宗门 UI 意图到地图定义、世界加载、成功／失败 delegate、RunId 设置、P6 observer 和返回 UI。列出当前使截图出现的直接原因。
2. 修复真实根因。可以修复 0.0.9B 启动配置、地图软引用、GameMode/World Settings、地图加载请求、生命周期 delegate、初始化顺序、旧状态泄漏或 Editor 配置；不得只隐藏错误文本、无条件返回成功、伪造 world activation、用 fixture 代替 M01，或把失败静默吞掉。
3. 将 M01 的有效世界定义、启动 GameMode／必要 Player 与启动入口明确注册到新的 0.0.9B Framework 配置中。引用丢失、地图不可加载或启动配置不匹配必须被 Editor Support 在运行前指出。
4. 新正式宗门页不得显示 V3、V2、0.2、Teleport Array 或任何旧版本状态／错误文案。技术失败的用户可见提示必须使用 0.0.9B 语义，例如“远征启动失败，已安全返回宗门；仓库与物品未改变。”同时给出可追踪的诊断编号。
5. 旧传送页可以作为历史内容保留，但从默认 Game/Editor 启动路径、0.0.9B 宗门导航、M01 Start CTA 和错误返回路径中退出。不得存在两个竞争的 Start Run 按钮或两套可写 session 状态机。

## 10. 0.0.9B 宗门、仓库与战备 Editor／UI 框架

### 10.1 新默认体验

默认 Editor Play 与游戏启动应进入 0.0.9B 宗门宿主，而不是旧传送页。最少提供以下产品层级：

- 宗门主页：进入仓库／战备、选择远征、查看当前远征状态；
- 仓库／战备页：同一 P5 局外物品图的仓库、人物装备和出战携行整理入口；
- 远征选择页：展示 M01 远征卡和启动意图，但不提前提交任何物品；
- 局内宿主：在真实 M01 activation 成功后承接既有 Code B P6/P7/P9—P21 UI；
- 统一状态／错误层：显示当前状态、明确故障和可返回路径。

页面的视觉细节可由执行端根据现有项目美术自由裁量，但仓库和战备必须满足第 5 节的单一持久物品图原则。不得创建新的“预备物品副本”“starter kit”“临时 Loadout fixture”或 Widget-local inventory。

### 10.2 仓库锁定契约

OPEN WAREHOUSE 只由新的 0.0.9B coordinator 根据真实 InRun 状态禁用。禁止以下替代判断：

- 旧 UI 的 bRunRequested、旧 ActiveRunId 残值、旧 map load callback、旧 V3 manager flag；
- P6 Prepared receipt、未完成 StartAttempt、界面正在切换、单纯存在旧存档字段；
- Widget 是否仍存在、前一次页面文本、地图名称或静态全局变量。

技术启动失败后的相同 Owner 必须立刻满足：AtSect、仓库可进入、P5 可读写、无活跃 P6 session、无待决 P8 结算、无旧 UI lock。

### 10.3 Editor Support

建立一个可被开发者直接使用的 0.0.9B Editor 验证入口，名称和形式可依据工程选择菜单、Utility、Commandlet、Automation spec 或 editor-only panel。它至少应：

1. 验证默认 Game/Editor 启动配置指向 0.0.9B 宗门框架；
2. 验证 M01 地图软引用、GameMode／World Settings、必要 UI 类和 0.0.9B startup config 均存在且可解析；
3. 显示本次 StartAttempt 的当前状态、诊断编号、Map reference 解析结果、world activation 结果、RunInstanceId 与 Code B bridge 状态；
4. 提供只读诊断与验证；不得提供可直接改写 P5/P6/P8、物品数量或终局的编辑器旁路；
5. 对缺失引用、旧默认 UI、旧 Start CTA 仍被默认路径引用、或旧错误文本仍可到达时给出明确失败。

## 11. Code B 与既有游戏内容的重接

1. 保留 P1—P21 的现有 Code B 数据、序列化、持久化、容器、装备、Loot、Drop、快捷栏和终局语义。重建只改变其顶层宿主和正式调用入口，不重置玩家 Profile，不批量迁移／重掷已 materialized 的 P9/P11/P14 history。
2. P5 只能在 AtSect 的仓库／战备页开放；P6 只在新 coordinator 的真实 activation success 后接入；P8 只在 Code A 已提交真实终局后运行。不得把技术启动失败送入 P8。
3. P7—P21 以已有的 0.0.9B 局内宿主接入。若某一功能暂未有实际 UI 入口，保持其现有数据能力和明确未接入状态，不用旧 UI、fixture 或假物品冒充 0.0.9B。
4. M01 adapter 对既有战斗、敌人、地图和 Actor 仅做最低必要的启动／成功／失败／终局转发。不得借框架重建改写战斗、数值、AI、地图内容、Loot 权重、物品定义、撤离规则或玩家属性。

## 12. 允许的改动范围

允许：

- 新建或最小重构 0.0.9B Runtime、UI Host、Run Start Coordinator、M01 adapter、Editor Support、启动配置、相关 UMG／地图入口和结构化诊断；
- 为正确处理 activation 成功、技术失败和终局转发而最小修改现有 Code A 生命周期／M01 启动适配文件；
- 为 P5/P6/P8 与新 coordinator 的精确契约增加 Code B adapter、状态查询、取消未提交 attempt 或 stale cleanup；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md、0.0.9B framework inventory、Prompt 归档和本 Report；
- 仅为本任务所报告启动缺陷新增有界的 Editor 验证与隔离 Profile 测试资产。

## 13. 明确不在本任务内

不得：

- 使用、读取、迁移或复活 0.2 final report、V2、V3 或其他早期版本规则；
- 删除 P1—P21、清空 P5/P6、重置用户物品、创建 blank project、以复制 Content 但不迁移代码／配置的方式伪造重建，或把旧传送页换名后当作新框架；
- 做全量 Inventory/Loot/战斗重写、第二地图、第二远征、第二尸体、资源／经济、装备数值、战斗效果、多人、Cook、Package 或新版本宣传；
- 创建塔科夫名称、界面、图标、资产、武器、货币、任务或商业内容的直接复制；
- 让 UI、Editor utility、Widget、fixture 或 debug command 成为 P5/P6/P8 或物品事务的可写权威；
- 把 TechnicalStartFailure 错归为玩家 Abandon/Death/Extracted、调用 P8、清空 P5、删除玩家物品、或用重新启动进程作为唯一“修复”；
- 自动开始下一项 P 任务、0.0.9B.F、全量回归、Game Build、Cook 或 Package。

## 14. 有界验收与构建

本任务是框架重建，并因用户已提交真实运行缺陷而特别授权以下两条有界的运行验收；它们不等同于启动完整 0.0.9B.F：

1. 编译 demo_mapEditor Win64 Development。若新的 Runtime 路径影响 Game target，也编译 demo_map Win64 Development。任何编译失败只可修正本次框架、配置、adapter、Editor Support 或契约接入引入的问题。
2. 使用新的 0.0.9B 默认 Editor／游戏入口和隔离 Profile，实际点击 M01 Start CTA：
   - 成功路径必须到达真正的 M01 世界／局内宿主，不出现旧传送页或任何 V3 文案；
   - 记录可用的 StartAttemptId、OwnerId、RunInstanceId、世界激活成功、P6 observer 结果和最终 UI 状态；
   - 不得用 direct function call、fixture 或只读 trace 代替真实 CTA。
3. 使用隔离 Profile 制造一个受控、明确标记的 M01 激活失败，例如缺失的测试地图引用或受控 adapter failure，必须从真实新 CTA 进入 TechnicalStartFailure → AtSect：
   - 用户可见反馈为 0.0.9B 技术故障提示；
   - 仓库立即可打开；
   - P5 前后快照、物品 ItemId／数量／位置／装备／快捷引用等价；
   - 不存在 ActiveRunId、P6 committed active session、P8 terminal、旧 UI lock 或残留 StartAttempt；
   - 不出现 V3/V2/0.2 文案。
4. 在真实用户截图所对应的正常 M01 配置上再次验证，确认它不再落入技术 activation failure；若真实 M01 仍不能激活，必须报告精确根因和受影响的配置／资产，不能将受控失败测试误报为修复成功。
5. 不执行无关 P1—P21 全量回归、全量 F、Cook、Package 或无边界人工试玩。

## 15. Report 与完成信号

生成 Dev.D.UE.0.0.9B.I1.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 创建的安全快照位置、覆盖范围和未删除的既有工程；
2. 本轮新增、修改、保留、停用默认入口但未删除的每个文件／资产及职责；
3. 新 0.0.9B Runtime／UI／Editor／Code B／M01 adapter 的边界图和调用顺序；
4. 截图两项缺陷的直接根因，及为何不是仅替换文案；
5. AtSect → PreparingStart → ActivatingWorld → InRun 与 TechnicalStartFailure → AtSect 的状态转换、写入边界、StartAttemptId／RunInstanceId 规则；
6. 技术失败下 P5、P6、P8、仓库锁、物品图和旧 UI 状态如何被保护和清理；
7. 塔科夫式战备／仓库原则在本次框架中的具体落点；
8. M01 真正配置、Map reference、GameMode／World Settings、默认 Editor／游戏入口和 Editor Support 的验证结果；
9. 实际编译命令、目标、最终 native exit code；
10. 两条有界运行验收的 CTA、隔离 Profile、关键状态、日志／截图路径和结果；
11. 确认没有读取或引用 0.2/V2/V3 规则，且没有丢弃 P1—P21 或开启完整 F；
12. 仍留待后续的功能与验证债务。

仅当新的 0.0.9B 框架成为默认入口、真实 M01 成功路径可用、受控技术失败不会留下假活动 Run／仓库锁、P1—P21 未被丢弃、边界审查通过且规定编译成功时，使用：

    READY_FOR_0_0_9B_P_REBASE

若框架重建完成但真实 M01 仍因缺失或损坏的必要地图／配置／资产无法成功激活，使用：

    NEEDS_PLANNER_DECISION

若只存在当前框架范围内的可修复编译或受控验收失败，使用：

    NEEDS_I1_REWORK

若无法建立安全快照、当前工程不可审计、或修复必须删除／重置 P1—P21 或改写需求外核心权威，使用：

    BLOCKED

完成后不得自动开始 P22、其他 P、0.0.9B.F 或任何未授权任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.I1.0.r0","file":"Dev.D.UE.0.0.9B.I1.0.r0_report.md"}
