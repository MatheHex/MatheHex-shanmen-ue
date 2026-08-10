# Dev.D.UE.0.0.9BFix3.P1

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不得新建项目或分支工程。
- 修复线：`0.0.9BFix3`。
- 阶段：P1——`F0-001` M01 BasicCache 生产目标安全投影修复。
- 任务编号：`Dev.D.UE.0.0.9BFix3.P1`。
- 前置：已接受 `0.0.9B.P1—P50` 与 `0.0.9BFix.P1—P4`；`Dev.D.UE.0.0.9B.F0.0.r0` 已以两种真实运行面复现 `F0-001`。本任务不把未执行的历史 Fix2 草案作为前置、基线或授权。
- 执行文件：`Dev.D.UE.0.0.9BFix3.P1_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9BFix3.P1_report.md`。
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`。
- 引擎：`C:\Program Files\Epic Games\UE_5.8`。
- 任务性质：同一活动工程内、由真实 F0 缺陷证据触发的窄范围 P 阶段生产修复；不是新功能、不是重做 P10/P18/P49/P50、不是 F 阶段验证。
- 后续治理：本任务回传 Report 后立即停止；不得自行启动 `0.0.9BFix3.P2`、F0 重测、F1、常规 P、自动化或其他任务。

---

## 上半部分：只读项目裁决、现状与边界

### 1. 唯一有效依据

唯一有效依据是当前 `0.0.9B` 活动工程、已接受任务链、本任务书，以及 `Dev.D.UE.0.0.9B.F0.0.r0_report.md` 中的 `F0-001` 证据。

`0.2 final report.docx`、所有 `0.2`／V2／V3、旧 I／IPF、历史页面壳、旧 CTA、旧库存与旧物品规则均已废止。它们不得读取、引用、恢复或用于决定实现、测试、命名、验收或范围。源码中遗留的历史类名只可作为当前活动调用链的审计对象，不能因此恢复历史语义。

### 2. 已证实的缺陷：F0-001

`Dev.D.UE.0.0.9B.F0.0.r0` 在未修改工程、真实 UI 与正常宗门部署路径下取得了以下确定证据：

| 运行面 | 实际结果 | 证据 |
| --- | --- | --- |
| PIE | `DeploymentSucceeded` 前，同一 M01 初始化帧输出 `CODEB_P10_BASIC_CACHE: no safe projection for target=M01.CodeBNormalContainer.BasicCache.01 anchor=M01.Resource.TIER_1.Cluster.01`；Run 仍进入 InRun，但 BasicCache 不存在。 | `Saved/Logs/demo_map.log:2268, 2270, 2280` |
| Standalone | 对同一 Owner 的新 Run 再现相同 safe-projection 失败；其余 M01 内容仍正常。 | `Saved/Logs/demo_map_2.log:1807, 1809, 1819` |

因此，`M01.CodeBNormalContainer.BasicCache.01` 的生产 Actor 与 P10 正常入口在任何 P49/P50 source、P31 WorldDrop record 或相关 durable 写入之前即被放弃。该问题阻断 P10/P18、P49/P50 与 F0 的后续真实验证；它不是“未命中 P18 optional spatial roll”，也不是 P49/P50 的事务失败。

### 3. 必须保留的有效基线

1. Code B P1 Repository 与既有 durable Store 仍是唯一可变物品真值。P5 为局外图，P6 为精确 `OwnerId + RunInstanceId` 的 active carry 图，P9 为 BasicCache 的 Run-local target 图；Widget、Actor、地图适配层与 Code A 只可投影、显示或传递瞬时意图。
2. P10 的唯一生产普通容器身份固定为 `M01.CodeBNormalContainer.BasicCache.01`，定义为 `CodeB.NormalContainer.BasicCache`，静态 anchor 为 `M01.Resource.TIER_1.Cluster.01`。其 `SearchTargetId` 继续仅从稳定静态身份推导；不得由 Actor pointer、世界临时 GUID、Widget 地址、显示名、坐标或随机值充当 durable identity。
3. P10 的 `Closed → Opening → Open`、Hidden → Searching → Revealed、一次性 materialization、P9 receipt／digest、正常交互距离与 focus、以及 P9↔P6 的已接受事务均保持。仅在正常 P10 打开／搜索路径中才可首次 materialize P9；Actor 生成、修复代码或 map 加载不得预建 ItemId、P9 record、receipt、空间图或掉落。
4. P18 的 BasicCache r2 历史兼容与 deterministic optional spatial utility 保持。不得 reroll、补料、重置、迁移或改写任何已 materialized BasicCache 的 Profile／receipt／digest／item graph。
5. P31 WorldDrop、P49/P50 的 BasicCache → WorldDrop 路径、P5/P6/P8 生命周期、P13/P15、P17 完整空间图、一层无嵌套规则和 Code A 对 Run／地图／输入／终局的既有权威均不改写。
6. P10 原有设计允许两种且仅两种物理投影方式：识别一个具备 exact static identity 的预置生产 BasicCache Actor，或从同一静态 anchor 安全投影一个 Actor。无论采用哪一种，当前 Run 中都只能存在一个该 identity 的生产 Actor／adapter；二者不得并行重复生效。

### 4. P／F 阶段分界

本任务属于 P 阶段，只允许根因审计、生产代码／必要地图或 Actor 配置的最小修复、静态审查与 Editor／Game 编译。

禁止启动产品、Editor Play、PIE、Standalone、真实鼠标键盘、部署、`[G]` 交互、截图、Smoke、自动化、回归、试玩、Cook、Package 或最终验收。不得以“修复后验证”为由伪造运行结果；F0 重测由后续独立 F 任务决定。

---

## 下半部分：授权执行内容

### 5. 单一授权目标

修复 `F0-001`：使当前 M01 的唯一正式 BasicCache 在正常部署时具有一个可解析、可生成、可交互的**唯一物理投影**，而不改变其 static/durable identity、物品真值、掉落内容或已有 P10—P50 交互语义。

完成后，活动调用链在静态层必须保证：`M01.Resource.TIER_1.Cluster.01` 的合法物理锚点／预置投影可导向恰好一个 `M01.CodeBNormalContainer.BasicCache.01` Actor；该 Actor 只把既有 P10 static target identity、范围与输入意图转交给当前服务，不持有或写入任何库存真值。

### 6. 先做根因与活动调用链审计

实现前必须静态追踪并在 Report 中写明：

1. 从 M01 Run 初始化到 `InitializeCodeBNormalContainerTarget`、pre-authored Actor 发现、`ResolveSafeWorldLocation`／等价 safe-projection、Actor spawn／registration、P10 交互注册与 teardown 的完整活动链。
2. `M01.Resource.TIER_1.Cluster.01` 当前如何解析，anchor transform／偏移、目标碰撞／地面检测通道、object query、trace、navigation／visibility 条件、候选过滤与 spawn collision policy 中哪一项导致了 `no safe projection`。
3. 现有代码是否同时尝试预置与动态投影，以及 duplicate identity／重复 adapter 的实际拒绝位置。不得把“第二次尝试”误当作回退成功。
4. 当前 BasicCache Actor 的身份、interact prompt、range surface、P10 focus/input、normal target registration 与 Code B/P9 边界；明确哪些成员是投影而非真值。
5. 该失败发生于 P9 materialization 之前的静态依据，并确认修复不会触发 P18 roll、P49/P50 WorldDrop、P31 record 或 durable Store 写入。

不得以日志文本、错误吞没、宽泛 `catch-all` 重试或无条件 spawn 掩盖根因。若审计发现 anchor 缺失、地图 collision、visibility／trace 设置或 Actor 配置确有问题，必须修复其实际生产原因，而不是添加测试 fixture、随机坐标、只在 Editor 生效的特例或一次性 debug bypass。

### 7. 生产修复要求

1. 在不扩展目标数量与产品语义的前提下，选择并收敛为一个稳定的生产物理投影方案：
   - 若正确方案是 anchor 安全投影，修复 `ResolveSafeWorldLocation`／其调用参数／anchor 偏移／必要的 map collision 或 visibility 设置，使它只在从既有静态 anchor 可验证得到合法位置时返回该位置；候选顺序和最终选择必须稳定、可审计，不能依赖随机数、帧序、Actor pointer 或 UI 状态。
   - 若正确方案是 P10 已存在的 pre-authored Actor 路径，可在 M01 以 exact static identity 配置一个正式 BasicCache Actor，并使动态投影在该 identity 已被正常识别时不再创建第二个；不得将预置 Actor 作为第二库存、测试 fixture 或旁路入口。
   - 只可采用一条有效生产生成路径。不得通过“预置一个再额外动态 spawn 一个”或“任何失败都原地强行 spawn”掩盖问题。
2. 保持精确 identity：Actor／adapter 的 static target identity 始终为 `M01.CodeBNormalContainer.BasicCache.01`，anchor provenance 始终为 `M01.Resource.TIER_1.Cluster.01`。不得以新 TargetId、随机 InstanceId、复制 target、第二 BasicCache、替换 DefinitionId 或直接 P6 发放规避失败。
3. 修复后的 Actor 仅在当前 M01 Run 生命周期内存在；Run teardown、target destruction 与重复 initialization 仍按既有 exact identity 清理，且不得留下 stale adapter、duplicate Actor、幽灵 prompt 或第二次 registration。
4. 仍无法解析合法物理位置时，保持明确结构化失败并零创建 target／P9／item／receipt／record；不得 silently relocate 到不相关资源、自动打开容器、假设玩家位置、允许超距离交互或回退至旧 Code A Loot/search。
5. 除解决实际 map/actor placement 根因所必需的窄修改外，不得变动 M01 地图布局、资源数量、敌人、reward sources、战斗、HUD、Run Save、输入权威、终局、旧库存或 Code A 规则。
6. 不得改写 P10 target-open／search／transfer 状态机、P18 profile、P31 WorldDrop、P49/P50、P5/P6/P8、P13/P15、P17 graph、任何存档 schema 或已 materialized history。若现有函数签名需要兼容性调整，只能是服务于这条物理投影链的最小声明／调用改动。

### 8. 允许范围

仅允许最小修改下列现行生产层，且每一项都必须在 Report 中说明：

- M01 BasicCache 的当前 `demo_mapV3ProgressionManager`／等价 M01 adapter、`ResolveSafeWorldLocation`／safe-projection helper、NormalContainer Actor 注册与 teardown 代码；
- 仅当根因审计证明必须如此时，M01 的 exact anchor、BasicCache Actor、相关可见性／collision 配置或既有 map asset；
- 为上述编译兼容所需的声明、include、Build.cs 或无产品语义变化的配置；
- `PROJECT.md`、`PROJECT_INFO_CARD.md`、本任务 Prompt 归档与本任务 Report。

### 9. 明确禁止

- 禁止新建项目、分支工程、第二 BasicCache、第二 P9/P6/P5、Widget inventory、Code A mirror、fixture、debug spawn、假 ItemId、假 record、直接 P6 grant、存档编辑、强制随机种子、console path 或 UI injection；
- 禁止添加第二 P10 target、第二 anchor、随机 fallback coordinate、actor-first durable write、自动开启／自动搜索／自动拾取、P18 强制命中、P49/P50 world drop、自动装备或任何交互捷径；
- 禁止修改 Loot Profile、r1/r2 receipt/history、搜索时长／reveal 语义、P9/P11 内容、P31 record、P5/P6/P8、P13/P15、P17、战斗、地图玩法、敌人、奖励数、经济、制作、网络或多人；
- 禁止读取、恢复、引用或采用 `0.2 final report.docx`、任何 `0.2`／V2／V3 或旧规则；
- 禁止启动产品、PIE、Standalone、真实部署、`[G]`、截图、自动化、回归、Smoke、Cook、Package、最终验证，或自动开始 F0 重测／F1／下一 Fix。

### 10. P 阶段静态审查与编译

完成实现后，仅进行以下检查：

1. 审查修复前后完整调用链，明确 F0 的 exact `no safe projection` 条件为何发生、现在由哪一个真实条件被修复，以及没有宽泛 bypass 或隐性失败吞没。
2. 审查 single-identity invariant：同一 Owner/Run/M01 初始化、重复初始化、pre-authored detection、dynamic projection、destroy/teardown 与 stale registration 的各分支中，最多只有一个 exact BasicCache Actor／adapter；没有 duplicate prompt、Actor 或 target registration。
3. 审查 identity／authority：`SearchTargetId` 仍从 P10 static identity 推导；Actor 不写 P1/P5/P6/P9/P31，不创建 item、receipt、roll 或 graph；所有 P10/P18/P49/P50 实际内容路径保持不变。
4. 审查 physical safety：最终位置的 anchor provenance、偏移、ground/collision／visibility 条件和 spawn policy 均可解释；失败路径零创建且不会重定向到其他资源、玩家位置或旧 Code A 路径。
5. 审查 Code A 与 map diff：任何 Code A／map 修改只服务于 exact BasicCache physical projection，不改变 Run、input、HUD、combat、enemy、reward、Run Save、terminal 或 item authority。
6. 执行 `git diff --check` 或等价文本完整性检查。
7. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

8. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

9. 若编译失败，只能修复本任务引入的 safe-projection、exact Actor registration、map/actor placement、声明、include、Build.cs 或调用签名问题。若需要改变 P10 的产品状态机、内容真值、P18/P49/P50 语义、创建额外目标或越过 F0-001 边界，停止并报告。

### 11. Report 与完成信号

生成 `Dev.D.UE.0.0.9BFix3.P1_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地包含：

1. 本轮修改、未修改与如有必要新增的每个文件／资产及职责；
2. F0-001 的 PIE／Standalone 复现事实、首次异常、实际 anchor/target identity 与本修复审计出的根因；
3. 修复前后 M01 初始化 → target discovery／safe projection → Actor registration → P10 input/focus → teardown 的活动调用链；
4. 选择 anchor projection 或 pre-authored Actor 的理由，以及未选择的路径为何不会并行产生 duplicate；
5. 最终 physical placement 的稳定来源、偏移／collision／visibility／spawn 条件、失败时零创建策略；
6. single-identity、duplicate/stale cleanup、Run teardown 与 normal target registration 的静态证据；
7. P1/P5/P6/P9/P18/P31/P49/P50、receipt/history、P13/P15/P17 与 Code A authority 未被改变的逐项结论；
8. `git diff --check` 结果、Editor／Game 实际编译命令、最终 native exit code 与关键结果；
9. 明确列出未执行的 F 阶段项目：PIE、Standalone、正常部署、BasicCache 可见性、`[G]`、P10 reveal、P49/P50、P31 isolation、保存/重开、拒绝路径、截图、回归、Smoke、Cook、Package；
10. 确认未读取或采用废止 `0.2 final report.docx`，未创建 fixture／假目标／第二库存，且未自行开始 F0 重测、F1 或后续 Fix。

仅当实际根因已在生产投影链中修复、静态单 identity／authority／物理安全审查通过、`git diff --check` 通过且 Editor 与 Game 均以 native exit code `0` 完成时，使用：

    READY_FOR_F0_RERUN

若只存在当前授权范围内的编译或 safe-projection 局部实现问题，使用：

    NEEDS_0_0_9BFIX3_P1_REWORK

若无法在不扩大目标、改写 P10/P18/P49/P50 产品语义、创建非生产 fixture 或改变 Code A 权威的前提下修复，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始任何后续任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9BFix3.P1","file":"Dev.D.UE.0.0.9BFix3.P1_report.md"}
