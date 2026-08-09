# Dev.D.UE.0.0.9BFix.P2

## 任务身份

- 项目：Dev.D.UE.0.0.9B（同一既有工程；不得新建项目）
- 修复线：0.0.9BFix
- 阶段：P2——宗门仓库／战备正式接入
- 任务编号：Dev.D.UE.0.0.9BFix.P2
- 前置：已接受 `0.0.9B.P1—P21` 与 `0.0.9BFix.P1`
- 执行文件：Dev.D.UE.0.0.9BFix.P2_prompt.md
- 报告文件：Dev.D.UE.0.0.9BFix.P2_report.md
- 活动工程根：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B
- 活动工程：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject
- 任务性质：在 `0.0.9BFix.P1` 已建立的 0.0.9B 顶层框架内，完成宗门仓库与战备的真实 Code B 接入；不是新项目、不是 I／IPF，也不是 F 阶段验证。

---

## 上半部分：只读项目裁决、现状与边界

### 1. 唯一有效治理依据

仅以最新 `0.0.9B` 大纲、活动工程、已接受的 `0.0.9B.P1—P21`，以及已接受的 `0.0.9BFix.P1` 为依据。

任何早期版本文档、名称、旧页壳、旧日志或源码残留都不得用于架构、范围、验收或命名决策；它们只能作为需要被隔离的历史实现。不得读取或恢复旧版本规则。

### 2. 已接受框架基线

`0.0.9BFix.P1` 已将以下内容设为正式基线：

1. `demo_map0909BFramework`、`demo_map0909BRunStartCoordinator`、`demo_map0909BM01RuntimeAdapter`、`demo_map0909BCodeBItemBridge`、`demo_map0909BEditorSupport` 与 `demo_map0909BSectWidget` 是新的 0.0.9B 顶层入口。
2. 唯一正式顶层状态机为：

       AtSect → PreparingStart → ActivatingWorld → InRun → ResolvingTerminal → AtSect
                              ↘ TechnicalStartFailure → AtSect

3. 只有已经确认真实 M01 世界、稳定 `OwnerId + RunInstanceId` 与输入恢复后，Coordinator 才能进入 `InRun` 并通知 P6。技术启动失败不得留下活动 P6、P8 终局、物品扣除、仓库锁或伪活动 Run。
4. Code B P1 Repository 是物品唯一可变真值：P5 为局外持久物品图，P6 为精确 Run 的携行图，P8 只在 Code A 已提交真实终局后结算。Code A 只保留 M01、Actor、战斗和必要生命周期的窄适配职责。
5. P7—P21 的背包、装备、普通容器、尸体、地面物品、快捷引用、消耗、确定性 Loot 与空间道具图均已接受，必须保留其既有所有权、稳定 ItemId、ContainerId、生命周期与终局语义。

### 3. 本任务的产品裁决：塔科夫式仓库与战备

本任务只把局外的“能整理的仓库”与“准备出战的携行配置”接入新宗门框架。塔科夫只作为结构参考，不复制其武器、战斗、经济、地图或商业设计。

固定产品规则如下：

1. **局外仓库与战备同属一份 P5 持久物品图。** 仓库、已配置装备位、携行区和空间物品内部内容均只能是同一个 P5 graph 的不同位置／投影，不能成为第二库存、Widget-local array、starter fixture 或平行 Profile。
2. **战备不是新的 Run。** 在 `AtSect` 整理、装备、卸下或把物品放入携行配置，只能进行 P5 内部的原子位置变更；不得创建 P6、RunId、P8 receipt、活动锁或 Code A 物品副本。
3. **出战选择来自当前 P5 战备图。** 只有用户明确提交出战、且 Coordinator 以后确认 M01 世界与真实 Run 后，既有 P5 → P6 正式桥接才能按精确 ItemId／完整 graph 交接携行物品。浏览页面、选择远征、进入 `PreparingStart`、地图解析、激活等待或技术失败都不得先移动、锁定、复制或扣除 P5 物品。
4. **技术失败不改变局外物品。** 它不是死亡、撤离或放弃，不进入 P8；恢复 `AtSect` 后同一份 P5 仓库和战备必须仍可整理，不能出现“结束当前 Run 后再整理”。
5. **真实 `InRun` 才禁止仓库写入。** `OPEN WAREHOUSE` 只能以 Coordinator 已确认的 `InRun` 状态禁用；旧布尔值、旧 RunId、旧页壳、地图名、Widget 存在或临时 StartAttempt 均不得把它伪锁。`PreparingStart`／`ActivatingWorld` 中可暂时拒绝新的 P5 写事务以避免并发，但必须显示为“出战尝试处理中”，不能伪称已有活动 Run 或终局。

### 4. P／F 阶段分界

本任务仍属于 P 阶段。只允许实现、静态代码／配置审查及最终编译；不得启动产品、点击 CTA、运行地图、采集截图、Smoke、自动化、回归、试玩、Cook、Package 或最终验收。所有真实 UI、拖拽、启动、技术失败、P5→P6、P8 与恢复验证继续留给 `0.0.9B.F`。

---

## 下半部分：授权执行内容

### 5. 单一授权目标

在 `0.0.9BFix.P1` 的新 0.0.9B UI Host 与 Coordinator 内，实现正式的 **Sect Warehouse / Loadout** 纵向功能层：

- 它读取并仅通过 Code B 正式事务修改同一 P5 Profile graph；
- 它把仓库和战备呈现为同图内的不同区域，允许在 `AtSect` 做受控拖拽整理；
- 它向 Run Start Coordinator 提供可审计、只读的当前携行选择快照；
- 它不创建第二库存、不提前开启 P6、不绕过 P5/P6/P8，也不重新接回任何旧启动页壳。

### 6. P5 仓库与战备图

#### 6.1 同图建模与兼容

1. 首先审计当前正式 P5 schema、P1/P4x/P7 的装备槽语义、P17 空间 parent／child graph 与 P5↔P6 bridge。复用其现有 canonical DefinitionId、slot semantic、容量、ItemId、ContainerId、revision、digest 和 durable replacement 契约。
2. 若当前 P5 已能表达局外仓库和正式战备位置，直接复用，禁止平行实现。若它尚缺一个局外战备 placement，只能在**同一 P5 profile graph 内**最小增加正式 `SectLoadout` container／slot placement；该 placement 与仓库 root 同属一个 P5 durable graph、一个 owner 和一个 revision/digest，不得保存第二份 item、copy、mirror 或 UI descriptor inventory。
3. `SectLoadout` 只可使用当前已有的正式装备／携行槽语义和容量。需要展示兵器、道袍／护甲、普通饰品、空间道具、基础携行格或其等价语义时，必须从现有 Catalog／P1/P4x/P7 解析，而不是从显示文本、图标、旧 Code A slot、fixture 或硬编码名称猜测。不得在本任务新增武器、道袍、饰品、空间 item、starter kit 或临时 Definition。
4. 空间 parent 进入或离开 P5 战备时，必须和其完整已存在的 child graph 一起作为一个原子闭包移动；不得 flatten、拆 child、复制内容、把 child 单独置入仓库，或更改 P17/P18/P19 的一层限制。
5. P13 的 1—9 运行时绑定、P15 消耗使用、P14/P19 地面路径和 P9/P10/P11 容器来源不是本任务的战备来源或写入目标，必须维持原语义。

#### 6.2 局外整理事务

1. 只在 Coordinator 真正处于 `AtSect` 时，允许 Warehouse ↔ SectLoadout、SectLoadout 装备位之间的正式 P5 Move／Equip／Unequip／Swap／Merge／Split（仅当现有 Item Definition 与原有事务本来就允许）事务。
2. 每次成功整理必须沿现有 P5 owner durable replacement 只移动同一 ItemId／完整合法 graph，原子推进 P5 revision 与 digest。不得产生 new ItemId、clone、第二 parent、部分提交、P6 mutation、Code A mirror 或 Widget direct write。
3. 事务开始前必须复核 P5 owner、profile revision、source／destination container、slot compatibility、capacity、item Definition、graph closure、空间深度、unique parent、循环、child owner、terminal／active session gate。任何失败、冲突或保存失败都必须在 durable replacement 前拒绝，保持 P5/P6/P7/P8/P13/P14/P15/P17/P18/P19 不变。
4. `PreparingStart` 或 `ActivatingWorld` 中，P5 的写请求必须以显式 `StartAttemptPending`／等价诊断拒绝，不能通过删除页面、伪锁仓库、创建 Run 或旧状态文本处理。Coordinator 回到 `AtSect` 后不需要重启工程即可重新整理。
5. `InRun`／`ResolvingTerminal` 中，宗门仓库页面可显示状态但不得发起 P5 写事务；它必须由 Coordinator 的结构化状态和精确 Run 身份判定，不得由 Widget 指针或旧 manager flag 判定。

### 7. UI Host、页面路由与只读投影

1. 由新的 `demo_map0909BFramework`／UI Host 路由宗门主页、Warehouse、Loadout、远征选择和状态提示。默认宗门入口只可达此新路由；不得把旧传送页壳、旧默认 Widget、旧 activation CTA 或旧错误返回重新接入。
2. 新的 Warehouse／Loadout presenter 与 UMG 只能保存最新 Code B snapshot、渲染 key 和用户意图；不得拥有 ItemId 真值、可写 inventory array、独立选中物品副本、Run 判定或 P5/P6/P8 transaction。
3. 页面至少应清楚投影：
   - P5 仓库内的真实容器／物品位置；
   - 同一 P5 graph 中当前 SectLoadout 的装备与携行位置；
   - 空间 parent 的完整、不可拆分归属；
   - 当前 Coordinator 顶层状态、是否可写、以及拒绝原因；
   - 当前可用于未来出战的只读 Loadout 摘要／digest。
   空 P5 Profile 必须显示为空仓库／空战备，而非生成 starter 或旧库存。
4. 所有拖拽和按钮意图必须通过 UI Host → Code B 正式 command／transaction 入口，且由 Code B 回传新 snapshot。不得使用双击领取、QuickMove、自动装备、Actor pickup、Widget direct mutation 或 debug command 绕开验证。
5. 必须处理页面关闭、重新打开、Widget 重建、Coordinator 状态变化与 snapshot 过期：这些事件只重读当前 P5／Coordinator 状态，不得反向重建／覆盖／保留过期 widget inventory。

### 8. 出战选择与 P5 → P6 交接边界

1. 在 `AtSect` 由 UI Host 从权威 P5 战备图生成只读 `LoadoutSelection`：其中至少包括 P5 owner、profile revision、稳定排序的 root ItemId／ContainerId、必要的完整空间 graph closure、slot semantic、selection digest 和生成顺序。它不是物品库存、不是 P6，也不是可写 Widget 状态。
2. 用户提交远征时，Coordinator 记录 `StartAttemptId` 和该只读选择的 digest／revision。`PreparingStart` 只能保存此选择供后续复核，不能移动、reserve、lock、copy 或提前扣除 P5 物品。
3. 只有 P1 的 M01 Adapter 已确认真实世界、稳定 `OwnerId + RunInstanceId` 且 Coordinator 已收到成功回调后，Code B Item Bridge 才可调用既有正式 P5→P6 交接契约。它必须以当前 P5 revision、LoadoutSelection digest、精确 root ItemId 与完整 closure 再验证一次，并在一个 owner durable replacement 内完成既有的真实交接。
4. 交接成功时，P6 只获得同一批既有 ItemId／ContainerId／合法 graph；P5 不得保留镜像或第二 parent。P6 的准确 Run 身份、revision 与 digest 必须与 Coordinator 的确认身份一致。P8 仍然是唯一终局结算点。
5. 在世界确认前任何失败均只走 `TechnicalStartFailure → AtSect`，并保持 P5 与 SectLoadout 等价、无 P6 session、无 P8 receipt、无仓库假锁。不得把技术失败伪装为 Abandon／Death／Extracted。
6. 世界确认之后若 Code B bridge 因 stale selection、冲突或保存失败拒绝，只能按 P1 的既定“Code A Run 已确认、Bridge 拒绝为可审计事件”边界处理：不得清除／回滚真实 Code A Run，不得偷偷创建 fallback P6、clone P5 item 或让旧框架重获控制权。必须保留结构化诊断，供后续 F／专项修复处理。

### 9. Editor Support 与静态审计

扩展 `demo_map0909BEditorSupport` 的只读诊断能力，使其至少能核对：

1. 默认入口仍为新 0.0.9B Sect UI Host，旧默认 UI／旧 CTA 不在正式路径；
2. P5 warehouse 与 SectLoadout 使用同一 owner graph，而非第二 profile、widget inventory 或 Code A inventory；
3. 现有 Slot／Definition 解析、空间 graph closure、P5 transaction command 与 P5→P6 bridge 引用均完整；
4. Warehouse 的可写性只由 Coordinator 的结构化顶层状态决定；
5. 最近一次 StartAttempt 的 selection digest、P5 revision、状态和拒绝／bridge 诊断仅供只读排查，不提供绕过 P5/P6/P8 的写入口。

### 10. 允许范围

允许在同一工程内最小新增／修改：

- 0.0.9B UI Host、Sect Widget、Warehouse／Loadout presenter 与相关 UMG；
- Code B P5 profile schema 中同图的战备 placement、合法 P5 整理事务、snapshot／selection digest、序列化与静态诊断；
- Coordinator、CodeBItemBridge 和 Editor Support 的窄接口，以传递只读 LoadoutSelection 和结构化拒绝诊断；
- 为编译兼容而最小调整 P1/P4x/P5/P6/P7/P8/P13/P15/P17/P18/P19 的 Code B 声明；
- `PROJECT.md`、`PROJECT_INFO_CARD.md`、本任务 Prompt 归档和本任务 Report。

### 11. 明确禁止

- 禁止新建项目、Blank Project、分支工程、伪迁移、第二 P5 Profile、第二库存、P6 preview inventory、Widget-local inventory、starter kit、fixture、假 ItemId、旧 Code A inventory 或双写；
- 禁止改写、重置、重掷或迁移已 materialized P5/P6/P8、P9/P11/P14/P19 历史、用户物品或已接受 P1—P21 功能；
- 禁止接管／改写 Code A 地图、Actor、Player、输入、HUD、战斗、生命、死亡、Run Save、终局、旧库存或旧 Loot 权威；
- 禁止新增战斗、武器／道袍／饰品数值、装备效果、资源经济、交易、商店、制作、自动拾取、地面新来源、第二地图、多人、网络同步、Cook、Package；
- 禁止启动产品、点击 CTA、运行真实拖拽／启动／技术失败／撤离流程、截图、Smoke、自动化、回归、Game 试玩或最终验证；
- 禁止在未经策划授权的情况下自动开始 `0.0.9BFix.P3`、常规 P、`0.0.9B.F` 或其他任务。

### 12. P 阶段静态审查与编译

完成实现后，只执行以下检查：

1. 审查 P5 仓库、SectLoadout、空间 parent／child 与 LoadoutSelection 均为同一 P5 图的不同位置／只读描述；不存在第二库存、clone、双写、Code A mirror、fixture 或 Widget authority。
2. 审查每一条 AtSect 整理事务和选择快照：ItemId／ContainerId 保持稳定，slot／容量／图闭包／revision／digest／冲突拒绝正确，P6/P8 在世界确认前零写入。
3. 审查 Coordinator 边界：只有确认为 `InRun` 才禁用 Warehouse；技术失败不会留下 P5 写锁、P6 session、P8 receipt 或旧文案；world-confirmed 后 bridge 拒绝不会伪回滚真实 Code A Run。
4. 审查 UI／Editor Support：默认路径不重新可达旧页壳；UMG 只读投影、意图路由与过期 snapshot 处理不持有物品真值；Editor Support 没有可写绕过入口。
5. 审查 Code A diff。除编译所必需的纯接口声明外，不得有 Code A 功能修改；任何例外须逐文件证明不涉及地图、Actor、输入、HUD、战斗、生命、Run、Run Save、结算、库存或 Loot 权威。
6. 编译：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

   因本轮修改 Runtime UI／Coordinator 路径，还必须编译：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_map Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

7. 若编译失败，只修复本任务引入的 P5 schema／事务、LoadoutSelection、UI projection、Coordinator／Bridge 窄接口、Editor Support、序列化、include 或调用签名问题；不得为取得通过而删除、降级、绕开 P1—P21 或重新接回旧框架。

### 13. Report 与完成信号

生成 `Dev.D.UE.0.0.9BFix.P2_report.md`，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改及停止默认入口的每个文件／资产与职责；
2. P5 仓库、SectLoadout、现有 P1/P4x/P7 slot semantic、空间 graph 与 P5→P6 的真实所有权关系；
3. 仓库／战备为何是同一 P5 graph，如何排除第二库存、P6 preview、Widget authority、starter 与 Code A mirror；
4. P5 整理事务的成功、冲突、拒绝和保存失败如何保持 ItemId、ContainerId、graph closure、revision、digest 与零副作用；
5. LoadoutSelection 的精确字段、稳定排序、digest、StartAttemptId 关系，以及世界确认前和确认后 P5→P6 的分界；
6. Coordinator 的 Warehouse gate、`PreparingStart` 写拒绝、技术失败恢复，以及世界确认后 Bridge 拒绝的静态处理边界；
7. 新 UI Host／Warehouse／Loadout／Editor Support 的路由、只读投影与旧路径隔离结果；
8. Code A、P5/P6/P8、P7、P13/P14/P15、P17/P18/P19、P9/P10/P11/P12 的静态边界结论；
9. 两个编译目标、实际命令、最终 native exit code 与关键结果；
10. 未执行的 F 阶段债务：真实宗门启动、仓库打开与拖拽、装备／卸下、空间图移动、空 P5、出战提交、M01 成功／技术失败、P5→P6、P8 三种终局、recovery、截图、自动化、回归、Smoke、Game 试玩、Cook、Package 和最终验收；
11. 确认未读取或采用早期版本规则、未新建项目、未丢弃 P1—P21、未启动 F 或任何未授权任务。

仅当同图 P5 仓库／战备正式接入新框架、P6 未被提前创建、旧页壳未重新接管、静态边界通过且两项编译均以 native exit code `0` 完成时，使用：

    READY_FOR_0_0_9BFIX_P3_OR_F_PLANNING

若当前范围内存在可修复的编译、schema、事务或静态边界问题，使用：

    NEEDS_0_0_9BFIX_P2_REWORK

若只能通过创建第二库存、迁移／重置历史、改写 Code A 权威或超出范围的功能才能完成，使用：

    BLOCKED

完成后不得自动开始任何后续任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9BFix.P2","file":"Dev.D.UE.0.0.9BFix.P2_report.md"}
