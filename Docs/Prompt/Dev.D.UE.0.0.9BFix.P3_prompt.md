# Dev.D.UE.0.0.9BFix.P3

## 任务身份

- 项目：Dev.D.UE.0.0.9B（同一既有工程；不得新建项目）
- 修复线：0.0.9BFix
- 阶段：P3——M01 出战激活与技术失败回退链收敛
- 任务编号：Dev.D.UE.0.0.9BFix.P3
- 前置：已接受 0.0.9B.P1—P21、0.0.9BFix.P1、0.0.9BFix.P2
- 执行文件：Dev.D.UE.0.0.9BFix.P3_prompt.md
- 报告文件：Dev.D.UE.0.0.9BFix.P3_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 任务性质：同一 0.0.9B 工程内的插入式框架修复；不是新项目、不是 I/IPF、不是常规新功能线，也不是 F 阶段验证。

---

## 上半部分：只读项目裁决、现状与边界

### 1. 唯一有效治理依据

仅以最新 0.0.9B 大纲、活动工程、已接受的 0.0.9B.P1—P21，以及已接受的 0.0.9BFix.P1/P2 为依据。

任何早期版本资料、旧名称、旧页壳、旧日志、旧启动路径或源码残留均无架构、范围、验收和命名效力。它们只能作为必须隔离的历史实现；不得读取、恢复、继承或据此作决策。

### 2. 已接受的 0.0.9B 修复基线

1. 0.0.9BFix.P1 已建立新的正式顶层边界：

       AtSect → PreparingStart → ActivatingWorld → InRun → ResolvingTerminal → AtSect
                              ↘ TechnicalStartFailure → AtSect

   demo_map0909BFramework、demo_map0909BRunStartCoordinator、demo_map0909BM01RuntimeAdapter、demo_map0909BCodeBItemBridge、demo_map0909BEditorSupport 与 demo_map0909BSectWidget 是 0.0.9B 的正式入口与职责边界。

2. Code B P1 Repository 是唯一可变物品真值。P5 是局外持久物品图；P6 只属于精确 OwnerId + RunInstanceId 的真实局内携行图；P8 只在 Code A 已提交真实终局后进行结算。Code A 继续只承担 M01 世界、Actor、Player、战斗和必要生命周期的窄运行时适配。

3. 0.0.9BFix.P2 已将宗门仓库、战备装备位、基础携行区和空间物品 child graph 接入同一 P5 graph。它们是同一 owner、同一 persistent revision/digest 的不同 placement／投影，不存在第二库存、P6 preview、Widget-local inventory 或 Code A mirror。

4. P2 已实现只读 LoadoutSelection：它含稳定排序的根 ItemId、ContainerId、slot semantic、必要的完整空间闭包、P5 revision、graph revision 与 digest。只有世界已确认、输入已恢复且 Coordinator 即将进入 InRun 后，既有 P5→P6 bridge 才可在重新核验成功后执行。

5. P1—P21 中已接受的 P5/P6/P8、P7、P9—P15、P17—P21 的所有权、稳定 ItemId、ContainerId、空间闭包、容器、尸体、地面、快捷引用、消耗、确定性 Loot、装备来源与终局语义必须全部保留。

### 3. 当前需要补齐的实现缺口

此前截图暴露的实际问题是：用户提交 START M01 RUN 后，旧激活页壳能失败并遗留“当前 Run”语义，使仓库错误显示为不可整理。

P1 已将旧壳从正式默认路径隔离，P2 已将仓库／战备与 P5→P6 边界接入新框架；但当前仍需把新 UI Host 发出的单次出战请求，与真实 M01 地图解析、OpenLevel／世界生命周期、PlayerController、Pawn 和输入恢复的异步事实统一到同一个正式 M01 Runtime Adapter 契约中。

本任务不要求也不允许实际启动产品验证。目标是在源码、配置、事件回调和静态诊断层完成这条链的唯一化，使后续 F 阶段能验证真实成功和真实技术失败，而不会再次由旧壳、stale callback 或临时状态伪造活动 Run。

### 4. 战备与仓库的固定产品规则

塔科夫只作为战备／仓库结构参考，不复制其武器、战斗、经济、地图或商业设计。固定规则如下：

1. AtSect 中的仓库与战备都是同一 P5 物品图的正式位置；整理不创建 Run、P6、P8 receipt、锁定副本或临时库存。
2. Submit Expedition 仅创建不可变的 StartAttempt 与只读 LoadoutSelection 引用；在真实世界确认前，不移动、扣除、reserve、复制或锁定 P5 物品。
3. 只有确认的 M01 世界、稳定 OwnerId + RunInstanceId、有效 PlayerController／Pawn、输入恢复和同一 StartAttempt 的成功回执共同满足时，才允许进入 InRun 并尝试既有 P5→P6 bridge。
4. 世界确认前的任何技术失败都回到 AtSect；P5 与战备等价、无 P6 session、无 P8 receipt、无活动 Run、无“结束当前 Run 后再整理”的仓库假锁。
5. 世界确认后 bridge 拒绝不是 world activation failure，也不能回滚或清除真实 Code A Run；它必须留下可审计诊断，供后续专项处理。

### 5. P／F 阶段分界

本任务仍属于 P 阶段。只允许实现、静态代码／配置审查与最终编译。

禁止启动产品、点击 CTA、运行地图、PIE、Standalone、真实 M01 激活、截图、自动化、Smoke、回归、试玩、Cook、Package 或最终验收。所有成功／失败路径的真实验证继续保留给 0.0.9B.F。

---

## 下半部分：授权执行内容

### 6. 单一授权目标

在 0.0.9BFix.P1/P2 的新 0.0.9B 框架内，完成唯一的 M01 出战激活适配链：

宗门 UI Host 的出战意图 → Run Start Coordinator 的单次 StartAttempt → M01 Runtime Adapter 的地图／世界就绪事实 → Coordinator 的成功或技术失败裁决 → P2 的 LoadoutSelection bridge 边界。

该实现必须使每个 attempt 都可按 StartAttemptId、OwnerId、必要时 RunInstanceId 追踪、去重和审计；不得回接任何旧激活页壳、旧 CTA、旧 Run manager 或旧仓库锁判断。

### 7. 正式 StartAttempt 契约

#### 7.1 唯一创建与输入

1. 只有新的 0.0.9B UI Host 可向 Run Start Coordinator 提交出战意图。Warehouse、Loadout Widget、Editor Support、M01 Actor、旧页面、快捷调试或 Widget-local handler 都不得自行创建 StartAttempt。
2. Coordinator 只在 AtSect 接受请求。它必须先读取 P2 已生成的只读 LoadoutSelection，记录不可变 StartAttemptId、OwnerId、P5 revision、graph revision、selection digest、M01 map descriptor／软引用、请求时间和序号，再进入 PreparingStart。
3. PreparingStart 只完成本 attempt 的配置解析、对象预检和异步激活准备。它不得写 P5、创建 P6、创建 P8 receipt、生成 ItemId、替换战备、调用旧库存或设置活动 Run。
4. 已有 pending attempt、非 AtSect 状态、失效／空选择、Owner 不一致、map descriptor 缺失、配置不合法或 revision 输入不完整时，必须返回结构化拒绝原因；不得 silent success、复用旧 attempt、覆盖 pending attempt 或以旧布尔值“已开始”处理。
5. 对同一用户意图的重复点击必须由 Coordinator 去重：只允许保留一个仍有效的 StartAttemptId，UI 仅显示其状态，不得创建第二条世界加载或第二次 P5→P6 bridge。

#### 7.2 M01 Adapter 的真实职责

1. M01 Runtime Adapter 必须是 Coordinator 与既有 M01 Runtime 的唯一双向窄接口。它可以复用当前正式 M01 地图、GameMode、World Settings、PlayerController、Pawn 和输入系统，但不得让这些对象拥有 P5/P6/P8、仓库可写性或技术失败的物品语义。
2. Adapter 必须将以下事实以带 StartAttemptId 的结构化异步回执报告给 Coordinator；不得以 UI 文案、地图显示名、Widget 存在、静态 flag 或残留 RunId 推断：

   - M01 descriptor 已解析／解析失败；
   - OpenLevel 或等价正式世界切换请求已接受／被拒绝；
   - 目标 World 已创建且与本 attempt 的正式 M01 descriptor 匹配；
   - 正确的 GameMode／World Settings 已就绪；
   - 匹配 Owner 的 PlayerController 与 Pawn 已就绪；
   - 该 Controller 的输入已恢复到可玩状态；
   - 目标世界在就绪前已卸载、发生错误、产生不匹配回调或超出正式等待边界。

3. 回执不得伪造“世界成功”。只有 M01 world、GameMode／World Settings、Controller、Pawn、Owner、输入和 attempt 关联均验证成功，Adapter 才可发出 RuntimeReady。
4. 可为配置／回调安全建立有限的等待边界或 watchdog，但必须使用本 attempt 的 monotonic token／StartAttemptId，且只产生明确分类的技术失败。禁止无期限轮询、无限重试、跨 attempt 定时器、静态全局 pending flag 或以重启工程恢复。
5. 旧世界、旧 PlayerController、stale delegate、已取消 attempt、map unload 后 callback、重复 RuntimeReady 或来自不匹配 World 的事件必须被 Adapter／Coordinator 明确忽略并记录诊断；不得改变当前 attempt、创建 P6 或改变仓库可写性。

#### 7.3 成功提交的严格顺序

1. Coordinator 在收到同一 StartAttemptId 的 RuntimeReady 后，必须再次验证：当前状态为 ActivatingWorld、attempt 未取消、Owner 一致、正式 M01 map identity 一致、稳定 RunInstanceId 已分配、Controller／Pawn／输入仍有效，且 P2 LoadoutSelection 的 Owner、revision、graph revision 与 digest 仍可用于复核。
2. 通过以上验证后，Coordinator 才可原子将状态置为 InRun，并将稳定 OwnerId + RunInstanceId 与原 StartAttemptId 传递给 P2 selection-aware Code B bridge。
3. 既有 bridge 必须再次从 P5 durable snapshot 复核 exact ItemId、完整空间 closure、revision 与 digest；成功时才建立精确 P6 session。不得产生 P5 mirror、clone、fallback P6 或第二 parent。
4. P5→P6 bridge 的拒绝、冲突或存储失败发生于真实 M01 成功之后时，必须保持“Code A Run 已确认、Bridge 拒绝为审计事件”的既定边界：不得把 Coordinator 退回 TechnicalStartFailure、不得清除真实 Code A Run、不得偷偷从旧系统补一份库存。
5. 成功进入 InRun 后，Warehouse 的不可写性只由该 Coordinator 的结构化 InRun + 同一稳定 RunInstanceId 决定。禁止以 pending attempt、旧 RunId、页面可见性、旧 flag、地图名或任何历史文本锁定。

### 8. 技术失败、清理与返回宗门

1. 在 RuntimeReady 之前发生的 descriptor、配置、OpenLevel、World、GameMode、World Settings、delegate、Controller、Pawn、Owner、输入、超时或 attempt 关联失败，统一由 Coordinator 分类为 TechnicalStartFailure。
2. TechnicalStartFailure 必须只处理同一个 StartAttemptId 的 transient 状态：

   - 停止或解绑该 attempt 的 delegate、timer、watchdog 与 UI loading token；
   - 丢弃未提交的 Runtime readiness、临时 Run identity 和 bridge 意图；
   - 确认未调用或未提交 P5→P6 bridge；若仅建立了未持久化本地候选，必须丢弃候选而非写回；
   - 不创建 P8 receipt，不触发 Death／Extracted／Abandon，不写 Code A terminal；
   - 恢复 AtSect，使 P5 warehouse 与 SectLoadout 立即按当前持久 snapshot 可重新整理。

3. 返回 AtSect 的 UI 不得显示历史版本号、旧 activation 文案、Teleport／旧页壳标题或“结束当前 Run 后再整理”。它只能显示结构化、当前 0.0.9B 的诊断摘要，例如“出战准备未完成，仓库未发生变更”，并允许用户重新操作。
4. 在失败后到达 AtSect 前，P5 写请求可被明确诊断为 StartAttemptPending；一旦原 attempt 清理完成，下一次 P5 读取／写入必须只受 AtSect 的真实结构化状态判定。禁止要求重启 Editor／游戏才能解锁。
5. 技术失败清理必须幂等。重复失败回执、晚到 callback 或 UI 重建不得再次改变 P5/P6/P8、覆盖新的 attempt 或导致仓库锁。

### 9. UI Host、页面路由与可观察诊断

1. 新 Sect UI Host 只呈现 Coordinator 的状态与拒绝原因，并将 START M01 RUN／等价出战确认意图路由到 Coordinator。它不保存 RunId 真值、激活结果、P5 snapshot、活动锁或可写 inventory。
2. Warehouse 与 Loadout 页面必须清楚区分：

   - AtSect：可根据 P5 正式事务整理；
   - PreparingStart／ActivatingWorld：显示“出战尝试处理中”，拒绝新的 P5 写操作但不得称为活动 Run；
   - InRun／ResolvingTerminal：显示真实局内状态，禁止 P5 写操作；
   - TechnicalStartFailure 回到 AtSect 后：显示仓库未改变且恢复可整理。

3. UI 重建、关闭／重开页面、应用焦点变化、世界切换或 stale presentation event 只重新读取 Coordinator 与 P5 snapshot；不得创建 attempt、重试激活、复活旧 loading token 或回写物品。
4. Editor Support 可增加只读诊断，至少能静态检查：

   - 默认入口及正式 START M01 RUN 路由仅指向新 UI Host／Coordinator／M01 Adapter；
   - M01 map descriptor、GameMode、World Settings、必要 UI class 和 Adapter callback binding 可解析；
   - 每个可观察的 StartAttempt 记录其状态、map identity、Owner、必要时 RunInstanceId、selection digest、时间顺序与最终分类；
   - 没有旧壳、旧 activation CTA、旧仓库锁 flag 或旧库存入口处于正式默认路径；
   - Editor Support 自身没有写 P5/P6/P8、创建 attempt、强制成功或绕过失败清理的入口。

### 10. Code A、Code B 与既有系统边界

1. Code B 继续独占 P5/P6/P8 的物品图、仓库／战备整理、LoadoutSelection、P5→P6 bridge、P8 receipt 与仓库可写性判断。不得引入 A/B 双写、A inventory mirror、Run preview、fixture、starter kit 或 item clone。
2. Code A 只允许为 M01 Runtime Adapter 提供必要的只读／窄生命周期信号：地图／World、GameMode、World Settings、Controller、Pawn、输入、稳定运行身份及真实终局回调。不得让 Code A 决定 P5 写入、P6 创建、P8、物品移动、仓库锁或技术失败终局。
3. 不修改地图美术、敌人、战斗、生命、死亡、终局逻辑、装备数值、消耗效果、Loot 概率、经济、商店、制作、自动拾取、多人、网络同步或其他新内容。
4. 不删除、重置、重掷、迁移或重建任何已 materialized P5/P6/P8、用户物品、历史 WorldDrop、尸体、空间图、快捷绑定或 P1—P21 已接受功能。

### 11. 允许范围

允许在同一工程内最小新增／修改：

- demo_map0909BRunStartCoordinator、demo_map0909BM01RuntimeAdapter、demo_map0909BFramework、demo_map0909BSectWidget 与新 UI Host 的窄启动／状态接口；
- 0.0.9B M01 map descriptor、正式启动配置、World／GameMode／World Settings／Controller／Pawn／输入就绪检查、delegate 生命周期与结构化诊断；
- CodeBItemBridge 与 P2 LoadoutSelection 的只读接入、attempt correlation 及已存在 P5→P6 契约的窄调用边界；
- demo_map0909BEditorSupport 的只读启动链审计；
- 为编译兼容所必需的最小 Code A 生命周期信号声明；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档和本任务 Report。

### 12. 明确禁止

- 禁止新建工程、Blank Project、平行 Profile、第二库存、第二 Start Run 状态机、P6 preview、Widget authority、fixture、starter kit、假 ItemId、clone、Code A mirror 或双写；
- 禁止恢复、重新接入、引用或借用任何早期版本的规则、页壳、CTA、导航、传送／激活链或状态判断；
- 禁止为了静态通过而无条件宣布世界成功、吞掉错误、硬编码 fake RunInstanceId、无限重试、无界 timer、以重启作为恢复机制，或把技术失败变为 P8／Abandon／Death／Extracted；
- 禁止在世界确认前移动、锁定、reserve、扣除或复制 P5 物品，或创建／提交 P6、P8 receipt；
- 禁止改写 Code A 地图、Actor、Player、输入、HUD、战斗、生命、Run Save、结算、旧库存或旧 Loot 权威；若最小 lifecycle signal 不可避免，必须在 Report 中逐文件证明它不改变这些权威；
- 禁止启动产品、运行任何真实流程、截图、Smoke、自动化、回归、试玩、Cook、Package 或最终验收；
- 禁止未经后续策划授权自动开始 0.0.9BFix.P4、常规 P、0.0.9B.F 或其他任务。

### 13. P 阶段静态审查与编译

完成实现后，只执行下列静态审查与最终编译：

1. 审查唯一 StartAttempt 创建者、Coordinator 的状态转换、M01 Adapter 的 callback correlation、stale callback／重复 callback 处理、成功顺序与 TechnicalStartFailure 幂等清理；确认不存在旧壳、静态 flag 或 UI 文案决定活动 Run。
2. 审查 P5/P6/P8 边界：世界确认前 P5 不被移动／锁定、P6/P8 零写入；世界确认后 bridge 拒绝不回滚真实 Code A Run；技术失败不会留下 P6、P8、活动 Run 或仓库假锁。
3. 审查 P2 LoadoutSelection 的 Owner、revision、graph revision、digest、ItemId／ContainerId 与完整空间 closure 只被读取和复核；不存在第二库存、Run preview、clone、mirror 或 Widget direct write。
4. 审查 UI／Editor Support 与默认配置：正式入口、M01 配置和状态提示均只走新 0.0.9B 链；早期壳、CTA、库存和仓库锁来源不可达；Editor Support 为只读。
5. 审查 Code A diff。除最小只读生命周期信号／调用签名／配置接入外，不得有 Code A 功能修改。每项例外均须逐文件说明不涉及地图、Actor、输入、HUD、战斗、生命、Run、Run Save、结算、库存或 Loot 权威。
6. 编译：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

7. 若编译失败，只修复本任务新增的 attempt、Adapter、delegate、配置、状态、UI／Editor 只读投影、LoadoutSelection 窄接口、include 或调用签名问题。不得为取得通过而删除、降级、绕开 P1—P21、Fix.P1 或 Fix.P2 的已接受成果。

### 14. Report 与完成信号

生成 Dev.D.UE.0.0.9BFix.P3_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改、停止默认入口但未删除的每个文件／资产与职责；
2. 唯一 StartAttempt 的创建点、完整字段、状态迁移、去重、Owner／RunInstanceId／selection digest 关联；
3. M01 Adapter 的正式 map descriptor、World、GameMode／World Settings、Controller、Pawn、输入就绪契约，以及 callback／delegate／timer 的关联和 stale-event 隔离；
4. RuntimeReady 到 InRun、P2 LoadoutSelection 再验证、既有 P5→P6 bridge 的严格调用顺序；
5. 技术失败的分类、幂等清理、AtSect 恢复、P5 等价、P6/P8 零副作用与仓库解锁静态证据；
6. 世界确认后 bridge 拒绝为何不被伪装为 activation failure，如何保持真实 Code A Run 与审计诊断；
7. 新 UI Host、Warehouse／Loadout 状态提示、默认入口、M01 CTA 路由与 Editor Support 的只读审计结果；
8. Code A、P5/P6/P8、P7、P9—P15、P17—P21 的静态边界结论；
9. 两个编译目标、实际命令、最终 native exit code 与关键结果；
10. 未执行的 F 阶段债务：真实宗门启动、仓库打开与拖拽、装备／卸下、空间图移动、空 P5、重复点击、M01 成功、每类技术失败、P5→P6、bridge 拒绝、P8 三种终局、recovery、截图、自动化、回归、Smoke、Game 试玩、Cook、Package 与最终验收；
11. 确认未读取或采用早期版本规则、未新建工程、未丢弃 P1—P21、未启动 F 或任何未授权任务。

仅当唯一 M01 出战激活链、attempt correlation、TechnicalStartFailure 清理、P2 selection 边界和旧壳隔离均通过静态审查，且两个编译目标均以 native exit code 0 完成时，使用：

    READY_FOR_0_0_9BFIX_P4_OR_F_PLANNING

若当前范围内存在可修复的编译、Adapter、配置、状态、callback、边界或静态审查问题，使用：

    NEEDS_0_0_9BFIX_P3_REWORK

若只能通过新建平行工程／库存、重置历史、恢复旧框架、改写 Code A 权威或超出范围的功能才能完成，使用：

    BLOCKED

完成后不得自动开始任何后续任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9BFix.P3","file":"Dev.D.UE.0.0.9BFix.P3_report.md"}
