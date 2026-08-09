# Dev.D.UE.0.0.9B.P6.0.r1

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P6——Code B 局外 Profile 到活动 Run 库存会话的桥接，证据返工
- 任务编号：`Dev.D.UE.0.0.9B.P6.0.r1`
- 任务性质：保留 `P6.0.r0` 已实现的候选 bridge，只补齐其真实产品生命周期与非阻塞语义的窄范围证据；不开始 P7、P8 或 `0.0.9B.F`。
- 执行文件：`Dev.D.UE.0.0.9B.P6.0.r1_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P6.0.r1_report.md`
- 活动工程根：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B`
- 活动工程：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject`
- 工程级开发基线：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9-XFix1`
- 已接收前置：`Dev.D.UE.0.0.9B.P5.0.r0`（实施交接，完整验证仍属于 `0.0.9B.F`）
- 候选输入：`Dev.D.UE.0.0.9B.P6.0.r0`

---

# 上半部分：只读裁决、现状与边界

## 1. 本次返工的唯一原因

`P6.0.r0` 报告已经给出有价值的实现证据：P5 sidecar、P6 durable Run session、同一记录内的 receipt 恢复、随身布局的同 ItemId 抽取、空 session、幂等、冲突拒绝及局外页面锁定均可保留为候选实现。

但它的唯一自动化入口直接测试 `NotifySuccessfulRun(...)`；它只以源码审阅说明该调用位于 `ActivatePreparedProfileWorld()` 成功之后。这个证据不足以证明**正常产品 Start Run 的成功生命周期确实会投递 bridge**，也不足以实测“bridge 自身失败后 Code A Start Run 仍成功”。因此 r0 不得作为 P6 正式放行依据。

本任务不是推翻 r0，不要求执行此前已留给 `0.0.9B.F` 的 P1–P6 全量回归、双分辨率 UI trace、截图巡检、Game 构建或最终 SHA-256 总审计。

## 2. 持续有效的权威边界

1. Code A 继续拥有默认地图、玩家 Actor、战斗、正式 Run 生命周期、旧库存、Loot、搜索、世界物品、结算、Run Save 和旧运行时。
2. Code B P1 Repository 仍是新物品系统唯一可变真值；P5 是局外 Code B snapshot，P6 是活动 Run Code B snapshot。不存在 Code A / Code B 镜像或双写。
3. P6 只能在 Code A 已成功完成正式 Run 激活后收到只读的 `OwnerId` 与稳定 `RunInstanceId`。工程实际字段若名为 `ProfileId` / `ActiveRunId`，必须在资料、trace 和 Report 中明确其与这两个契约身份的一一对应关系。
4. bridge 无权启动、阻止、重试、回滚、退出或改变 Code A Run。无 P5 sidecar、空布局、receipt 恢复失败、storage 错误和活动 session 冲突都不得重新成为 Start Run 门槛。
5. P4x 规则保持不变：没有备战门槛；普通物品移动、交换、合并、装备和卸下只由真实 Drop 提交；仅装备栏有类别／单槽互斥；空间戒指为装备后的快捷空间，空间储物囊为非快捷独立容器。

---

# 下半部分：授权执行内容

## 3. 单一授权目标

在不改变 P6 领域语义的前提下，建立并运行一个**产品生命周期级**的定向验证入口，例如：

    demo_map.CodeB.P6.ProductStartBridge

该入口必须从与正常产品相同的 Code A Start Run 成功路径观察 bridge，而不是把 `NotifySuccessfulRun`、P1/P2/P3/P4 Controller、Widget `NativeOn...` 或存档写接口当作测试起点直接调用。

允许为可观察性增加极小的只读 trace / test seam；仅当需要使该生产生命周期可重复验证时，才可改动成功激活后的最小通知适配层。不得重构 Start Run、地图加载、Player Actor、旧库存、Loot、搜索、结算或 Run Save。

## 4. 必须证明的真实生命周期

### 4.1 真实起点与事件顺序

每个目标场景都必须经由正常 Code A Start Run 的成功生命周期；测试可以使用隔离 Profile/存储根和受控的成功世界激活替身，但不得绕过正式的成功回调链。Trace 至少逐次记录：

1. Code A Start Run 请求进入正式生命周期；
2. `ActivatePreparedProfileWorld()` / 实际等价的世界激活成功；
3. 稳定 `OwnerId` / `RunInstanceId`（或实际字段的一一对应）在成功后首次交给 P6 observer；
4. P6 bridge 最终状态、session / receipt 状态与 Code A Start Run 返回结果。

桥接通知不得在 CTA 点击、Start Run 请求、旧 Preparation 读取、地图加载前、激活失败或回滚路径出现。Trace 要能证明顺序，而不是只说明源码位置。

### 4.2 定向场景矩阵

在同一产品生命周期入口中，至少覆盖并断言下列场景：

| 场景 | 强制断言 |
| --- | --- |
| 已接入、完整随身布局 | 正常 Start Run 成功后才创建 P6 session；武器/道袍/饰品、基础 6 格、已装备空间戒指及其内容、已装备空间储物囊及其内容均以原 ItemId、原位置和原 `ChildContainerId` 抽取；未选仓库物品不变。 |
| 已接入、空布局 | Code A Start Run 成功；创建持久真实空 session；不恢复任何备战或装备门槛。 |
| 未接入 Profile | Code A Start Run 成功；P6 返回 `NotEnrolled` / 等价状态；不创建 Profile、fixture、迁移、Repository 或 session。 |
| bridge / storage 故障 | 在世界激活成功之后、P6 提交之前故意让 bridge 失败或返回可审计故障；Code A Start Run 仍以成功结果完成，不重试、不回滚、不退出、不改玩家／旧库存；Code B 不得留下半扣除或不明状态。 |
| 幂等与 receipt 恢复 | 同一成功 Run 的重复成功通知不重复抽取；在 verified `Prepared` receipt 后中断，重启后通过同一真实成功通知恢复到确定的完整提交或完整旧状态，不出现重复 ItemId、半扣除或 Revision 回退。 |
| 未关闭活动 session 的新 RunId | 保留旧 session，P6 返回 `ActiveSessionConflict` / 等价状态，局外与旧 Run snapshot 不变；对应 Code A Start Run 仍成功且不读取该拒绝作为门槛。 |
| 局外入口锁定 | 活动 session 存在时，从正常“仓库／人物配置”入口只能得到只读提示“当前 Run 中，返回后再整理”或等价文本；不产生局外 Code B 写入、镜像或释放 Run 物品。 |

受控中断只可触及 P6 receipt seam；恢复阶段必须重新经由实际成功生命周期 observer，不得直接调用底层 Repository、Profile store 或 `NotifySuccessfulRun` 来伪造产品桥接。

## 5. 实施范围和禁止项

允许：

- Code B P6 定向测试、trace、断言和必要的临时故障注入 seam；
- 成功 Start Run 后的最小只读 observer / identity adapter；
- 资料与本次 Report 的更新。

禁止：

- 新建 P7 局内背包 UI、I 键入口、玩家 Actor 应用、Loot、尸体/容器搜索、世界掉落、拾取、丢弃、1—9、消耗品；
- 撤离、死亡、结算、物品返还、关闭 Run session、Run Save 接管；
- P5 迁移重写、正常局外页面改版、备战系统恢复或任意 Inventory 双写；
- 用一次直调 bridge / storage 的单元测试取代第 4 节的产品生命周期证据；
- 扩大为 `0.0.9B.F` 的全量构建、全量回归、截图、跨分辨率 UI 或来源审计。

## 6. 最小验证与退出要求

只运行本任务的产品生命周期定向套件及必要的单次 Editor 目标编译。每条报告中的命令必须给出：实际入口、隔离存储根、测试数量、结果、进程退出结果和关键 trace 文件。

此前 r0 出现“测试通过后外层等待超时”的情况。本轮不能把超时或遗留 UnrealEditor-Cmd 进程当作成功退出：需要以 `-TestExit` / 等价受控方式让测试进程在可记录的成功状态退出；若环境确有关闭延迟，必须同时给出明确的最终退出状态与无残留进程证据。不要因为处理退出问题重跑宽范围套件。

## 7. 资料、报告与状态

更新 `PROJECT.md`、`PROJECT_INFO_CARD.md` 或等价资料，明确：

- P6 r0 为候选实现，r1 补齐真实产品 Start Run 生命周期证据；
- `OwnerId` / `RunInstanceId` 与实际工程字段的对应；
- P5、P6、未来 P7、未来 P8 的所有权边界；
- P1–P6 全量验证、双分辨率 UI、Game 构建、最终来源审计仍属于 `0.0.9B.F`。

生成 `Dev.D.UE.0.0.9B.P6.0.r1_report.md`，存入：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须逐项列出：

1. r0 保留的实现与 r1 的精确改动文件；
2. 真正的产品 Start Run 入口、成功回调、P6 observer 和完整顺序 trace；
3. 第 4.2 节每个场景的可追溯入口、身份、bridge 状态、Code A Start Run 结果、P5/P6 Revision 与断言；
4. bridge 失败不影响 Start Run 的实测证据；
5. P5 只读锁和两类空间容器的明确证据；
6. 编译与目标测试的真实退出状态；
7. 仍明确留给 `0.0.9B.F` 的验证债务；
8. 所有 Code A 改动的逐文件理由，证明没有改变其 Run/Player/Loot/搜索/结算/Run Save/旧库存权威。

只有全部实现、七个定向场景、产品生命周期顺序、受控退出和边界审阅都完成时，最终状态才可使用：

    READY_FOR_CODE_B_IN_RAID_UI_WITH_F_DEBT

否则只能使用：

    NEEDS_P6_REWORK
    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P7、P8、`0.0.9B.F` 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P6.0.r1","file":"Dev.D.UE.0.0.9B.P6.0.r1_report.md"}

若项目、任务编号、Prompt、Report、活动工程、P5/P6 状态、Code A/Code B 权威边界或目标 Chat 无法对应，停止受影响工作并生成 Error001 Report；不得自行猜测、改号、切换项目或直接开始后续阶段。
