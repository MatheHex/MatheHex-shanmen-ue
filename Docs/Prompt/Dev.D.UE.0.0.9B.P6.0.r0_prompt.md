# Dev.D.UE.0.0.9B.P6.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P6——Code B 局外物品到活动 Run 库存会话的桥接
- 任务编号：`Dev.D.UE.0.0.9B.P6.0.r0`
- 任务性质：在已实施的 P5 真实局外 Profile 之上，建立不阻塞 Start Run 的 Code B 活动 Run 库存会话；不制作局内背包 UI、Loot、搜索、世界物品或结算。
- 执行文件：`Dev.D.UE.0.0.9B.P6.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P6.0.r0_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 工程级开发基线：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`
- 已接受前置：`Dev.D.UE.0.0.9B.P4x.0.r2`
- 已接收实施交接：`Dev.D.UE.0.0.9B.P5.0.r0`；P5 的完整验证矩阵已登记给 `0.0.9B.F`，不得把它重写为已完整验收。

## 上半部分：只读决策、现状与边界

### 1. 当前有效状态

- `0.0.9-XFix1` 仍是完整工程基线。Code A 继续拥有当前地图、怪物、战斗、默认玩家、正式 Run、旧 Loot、搜索、结算、Run Save 和旧运行时。
- Code B 的 P1 Repository 仍是新物品系统的唯一可变真值。真实页面的位置写入链仍唯一且不可回退：`真实 UMG／Slate Drop → P4 Interaction Controller → P3 UI Controller → P2 Application Service → P1 Repository`。
- P4x 的规则已经生效：没有备战启动门槛；普通格不做类别互斥；仅装备栏做类别／单槽互斥；移动、交换、合并、装备、卸下只由真实 Drop 提交；不存在 QuickMove、放回、双击、右键、详情或隐藏命令的可达写入入口。
- 空间戒指只有在装备后才提供快捷空间；空间储物囊始终是非快捷独立容器；两者均有稳定 `ChildContainerId`，带内部内容时整体移动必须原子拒绝。
- P5 已实现每个正式 `OwnerId` 的版本化 Code B 局外 sidecar、迁移 receipt、原子持久化、真实“仓库／人物配置”入口和 Profile Host。它不是 fixture，也不是 Code A／Code B 双写。
- 当前加速执行策略只把 P 阶段所需的窄范围证据留在本阶段；P1–P6 全量自动化、跨分辨率完整输入、全产品回归、最终构建、截图巡检和最终来源审计统一在 `0.0.9B.F` 执行。此策略不允许把未实现的功能或边界风险伪装成已验证。

### 2. P6 的产品裁决

P6 只建立正式闭环的第二个数据落点：已经存在且已提交的 Code B 局外 Profile，在 Code A 成功开始一次 Run 后，可以把当前“随身布局”以同一批 `ItemId` 转入一个持久、可恢复、只读待用的 Code B 活动 Run 库存会话。

“随身布局”是 P5 Profile 中的：已装备兵器、道袍、饰品、空间戒指、基础 6 格，以及已装备空间戒指和已装备空间储物囊内部的合法内容。仓库中未选入该布局的物品留在局外 Profile。它不是新的备战阶段，也不是 Start Run 的前置检查。

P6 对 P5 的 Start Run 限制作唯一且精确的阶段性扩展：Code A 已成功激活 Run 并提供稳定 `RunInstanceId` 后，允许一个后置、非阻塞、只读接收 Code A Run 身份的 Code B bridge 尝试创建或恢复活动 Run 会话。它无权启动、拒绝、重试、回滚或修改 Code A Run。

## 下半部分：授权执行内容

### 3. 单一授权目标

实现 `CodeBRunInventorySession`（工程内名称可调整）及其 bridge，使其能够：

- 只以已提交的 P5 `OwnerId` sidecar 和 Code A 已成功生成的稳定 `RunInstanceId` 为输入；
- 原子地将同一批“随身布局”`ItemId` 从局外 Code B snapshot 移入活动 Run Code B snapshot，而非复制、重建或重编号；
- 在重复、进程中断或持久化恢复后保持唯一 `ItemId`、唯一父位置、稳定 `ContainerId`／`ChildContainerId` 与可审计 receipt；
- 让 Code A 的 Start Run 对无 P5 Profile、空布局、bridge 拒绝或 bridge 恢复失败始终保持非阻塞；
- 为后续 P7 局内背包 UI 提供只读的活动 Run Projection 和会话状态，但本任务不把它接进玩家 Actor 或任何局内 UI。

不得开始 P7、局内背包页面、I 键入口、Loot、容器／尸体搜索、世界掉落、拾取、丢弃、1—9 使用、消耗品、撤离／死亡结算、从 Run 返回局外、Run Save、战斗、地图或怪物工作。

### 4. 活动 Run 会话模型

#### 4.1 合法创建时机

- Code A 的正常 Start Run 路径必须先按原有规则完成成功激活。P6 不得在 CTA 点击、开始请求、旧备战检查、地图加载前或失败回滚路径创建 Code B Repository、Profile、fixture、迁移或 Run session。
- 只有成功激活后的正式生命周期通知才可把 `OwnerId` 和不可变 `RunInstanceId` 交给 bridge。Code B 只读取这两个身份和“Run 已成功”的事实；不得读取、解释、镜像或修改 Code A 物品、装备、Hotbar、世界状态或结算数据。
- 若该 `OwnerId` 没有已提交 P5 sidecar，bridge 必须记录只读 `NotEnrolled`／等价状态后退出：不创建 P5 Profile、不迁移旧数据、不创建 fixture、不写入 Code B，也不影响该次 Run。
- 已有 P5 sidecar但随身布局为空时，可建立真实的空活动会话；它同样不得阻止 Run。
- 同一 `OwnerId + RunInstanceId` 的重复通知必须返回同一已提交或可恢复 session，不复制 ItemId、不重复扣出局外物品、不递增错误 Revision。一个不同 `RunInstanceId` 在旧活动 session 未由后续结算阶段关闭时，只能拒绝 Code B bridge 并保留原记录；Code A Run 仍继续。

#### 4.2 数据与单一权威

- 每个活动会话至少记录：`OwnerId`、`RunInstanceId`、会话版本、来源局外 Revision、Run Repository snapshot／Revision、创建和最后提交时间、桥接状态、prepare／commit receipt、恢复状态及异常原因。
- 从局外布局抽取时，原 `ItemId` 直接进入 Run snapshot；不得为同一物品生成第二个活跃实例，也不得让其同时保留在局外位置和 Run 位置。
- 仓库内未被抽取的物品必须保持在原 P5 snapshot。已装备空间容器本体和其有效内部内容必须作为同一逻辑布局处理，保持原 `ChildContainerId`；不得展开成第二套容器或把内容静默移到仓库。
- bridge 需要跨局外 snapshot 和 Run session 持久化时，必须有可恢复的 prepare／commit receipt。故障、中断或重启后只能得到：旧局外状态且无 Run session，或已完整抽取的局外状态加完整 Run session，或可确定恢复到其中之一的 pending receipt；不得出现半扣除、重复 ItemId、丢失父位置、Revision 回退或不明状态。
- 活动 session 存在时，该 `OwnerId` 的 P5 正常“仓库／人物配置”不得成为第二个可编辑来源。可以显示清楚的只读提示“当前 Run 中，返回后再整理”，但不得触发旧局外数据回写、镜像或暗中释放 Run 物品。

#### 4.3 非阻塞与明确不做的事

- Code B bridge 的任何拒绝、恢复失败、数据异常或 storage 错误只能在 Code B 侧记录并提供可审计状态；不得使 Code A 的正常 Start Run 失败、卡住、重试、自动退出或改变其玩家／装备。
- P6 不把 Run snapshot 应用到 Code A Player Actor，不生成世界物品，不让 Code B 参与伤害、碰撞、显示、AI、掉落、消耗、快捷栏或任何 gameplay 权威。
- P6 不实现 Run 结束后的回收、撤离转移、死亡丢失、结算或自动恢复。Run session 必须保持为后续 P7／P8 的唯一待处理输入；不得为了“方便继续玩”而把物品复制回 P5 Profile。
- P6 不恢复旧备战、装备校验或无装备阻断。空 Profile、空布局、未接入 Profile 与 Code B bridge 错误都不能重新成为进入游戏的门槛。

### 5. 允许的工程改动范围

允许新增或修改隔离的 Code B Run session／bridge／receipt／测试支持文件，并在必要时对下列 Code A 边界做最小适配：

- 已成功 Start Run 的生命周期通知或只读身份交接；
- Profile／session 协调层中传递稳定 `OwnerId`、`RunInstanceId` 和桥接状态的最小接口；
- 已接管 Profile 入口在活动 session 期间显示只读状态的最小路由。

任何 Code A 文件改动都必须逐文件说明为什么不改变 Run、玩家、地图、Loot、结算、SaveGame 或旧库存权威。不得以 P6 名义吸收无关改动。

### 6. 窄范围实施证据（非 `0.0.9B.F` 全量验收）

本任务不执行 P1–P5 全量回归、双分辨率 UI 全套 trace、截图巡检、Game 构建或最终 SHA-256 总审计；它们留在 `0.0.9B.F`。但为避免把 bridge 只写成未接线的代码，必须保留以下针对性、可追溯证据：

- **已接入 Profile**：经正常产品 Start Run 的成功生命周期创建 Run session；同一批随身 `ItemId`、装备位置、数量和空间 `ChildContainerId` 从 P5 snapshot 转入 session，仓库剩余物品未改变。
- **空 Profile**：无装备／空布局仍能完成 Code A Start Run，且若已有 P5 sidecar，Code B session 是真实空 session，不是 fixture。
- **未接入 Profile**：从未打开 P5 页面时，Code A Start Run 成功，Code B 不创建 Profile、fixture、迁移或 session。
- **幂等与恢复**：同一 Run 重复通知不重复抽取；至少覆盖一次 receipt prepare／commit 中断后的确定性恢复。
- **活动会话冲突**：在旧 session 未处理时收到不同 RunId，Code B 拒绝且局外／Run snapshot 不变，Code A Run 不受影响。
- **边界审阅**：明确证明 P6 没有运行时 Player Actor、Loot、搜索、世界物品、结算、Run Save 或 Code A 库存写入路径。

可提供窄范围 `CodeB.P6.RunBridgeTrace`／等价测试入口，但它必须从真实产品 Start Run 成功通知观察结果，不得把直接调用 P1/P2/P3/P4、Widget `NativeOn...` 或存档写接口伪装为产品桥接。一次面向改动目标的最终编译或最小目标测试可以使用；不要把它扩大为重复构建或全量套件。

### 7. 资料、报告与验证债务

更新 `PROJECT.md`、`PROJECT_INFO_CARD.md` 或等价资料，明确记录：

- P5 仅为已接收的实现交接，完整验证仍属于 `0.0.9B.F`；
- P6 的 post-success、非阻塞 Start Run bridge 语义；
- P5 Profile、P6 Run session、未来 P7 局内 UI 与未来 P8 结算之间的所有权边界；
- 活动 session 对局外页面的只读锁定及未实现的返还／结算；
- 本轮实际执行的窄范围证据和仍待 `0.0.9B.F` 的验证清单。

生成 `Dev.D.UE.0.0.9B.P6.0.r0_report.md`，存入：

```text
C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report
```

Report 必须逐项说明：实际改动文件、P5／P6 数据所有权、bridge 创建时机、抽取集合、receipt／恢复语义、每条窄范围证据的命令或可追溯入口与结果、任何 Code A 文件的逐项理由，以及完整验证债务。不得声称 P5 或 P6 已完成 `0.0.9B.F` 验证。

只有上述实现和六项窄范围证据均完成时，最终状态可使用：

```text
READY_FOR_CODE_B_IN_RAID_UI_WITH_F_DEBT
```

否则只能使用：

```text
NEEDS_P6_REWORK
NEEDS_PLANNER_DECISION
BLOCKED
```

完成后不得自动开始 P7、P8、`0.0.9B.F` 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

```text
[CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P6.0.r0","file":"Dev.D.UE.0.0.9B.P6.0.r0_report.md"}
```

若项目、任务编号、Prompt、Report、活动工程、P4x／P5 状态、Code A／Code B 权威边界或目标 Chat 无法对应，停止受影响工作并生成 Error001 Report；不得自行猜测、改号、切换项目或直接开始后续阶段。
