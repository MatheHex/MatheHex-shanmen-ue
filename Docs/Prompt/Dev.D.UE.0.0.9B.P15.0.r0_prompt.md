# Dev.D.UE.0.0.9B.P15.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P15——单一快捷消耗品使用与生命恢复回执纵向切片
- 任务编号：Dev.D.UE.0.0.9B.P15.0.r0
- 任务性质：在 P13 已完成 1—9 的唯一 ItemId 引用、P14 已完成玩家主动地面丢弃／拾回之后，使一个已绑定、位于活动 P6 基础 6 格的简单 QuickUsable 消耗品可以由实际 1—9 按键请求使用。Code B 负责唯一物品数量、使用资格、一次性扣除和效果回执；Code A 继续只负责玩家生命、存活状态、输入原始派发、HUD 与效果实际落地。本任务只做一个数据驱动的 RestoreHealth 效果类别，不实现武器使用、技能、装备效果、空间物品使用或通用动作系统。P 阶段仅进行功能开发、静态代码审查、Editor 代码编译及必要的最小编译修正；真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证均归 0.0.9B.F。
- 执行文件：Dev.D.UE.0.0.9B.P15.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P15.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 引擎：C:\Program Files\Epic Games\UE_5.8
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r1、P10.0.r0、P11.0.r0、P12.0.r0、P13.0.r0 与 P14.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P15 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 持续有效的真值与边界

1. Code A 继续拥有默认地图、Player Actor、原始输入派发、战斗、生命／伤害、玩家存活状态、HUD、正式 Run 生命周期、撤离／死亡分类、Run Save、旧库存、旧 Loot／搜索及所有尚未迁移的运行时。P15 不得把 P1/P5/P6 物品图、数量、快捷栏引用或效果回执镜像到 Code A。Code A 对 P15 的唯一新增职责可以是：接受一次已提交的、不可变的 RestoreHealth 回执，按自己的现有生命规则幂等地恢复生命，并回传是否已经处理该回执。
2. Code B P1 Repository 仍是新物品体系的唯一可变真值。P5 是局外 Profile snapshot；P6 是精确 OwnerId + RunInstanceId 的活动 Run 玩家携带 snapshot；P8 只在 Code A 已提交终局分类后处理 P6 的归还／没收与关闭。P15 的物品使用只写入同一 P6 Owner durable record，绝不写入 Code A 旧库存、Actor、Widget、P9、P11、世界 Actor 或第二份 SaveGame。
3. P13 的快捷栏继续只有九个固定 SlotIndex + ItemId | Empty 引用。它不保存数量、Definition、ContainerId、ChildContainerId、Widget、Actor 或 runtime GUID。P15 必须每次从当前 P6 权威 projection 重新解析该引用和 ItemId；不得把按键缓存、UI 选择状态或显示内容当成使用真值。
4. P7 的玩家背包、P10 的普通容器、P12 的尸体容器和 P14 的地面投影继续遵循既有位置写入规则。P15 允许的唯一非拖拽物品图写入是：成功的 QuickUse request 在同一 P6 transaction 内将一个合法消耗品数量减一，或数量归零时移除同一 ItemId；它不是 Split、Merge、Swap、自动整理、QuickMove、世界拾取或任意跨图移动。
5. P8 的 Code A 后置观察、终局分类、receipt、幂等、锁、Prepared、RecoveredAbandon rebind、P5↔P6 bridge 与 Profile lock 均保持既有语义。P15 的 pending／acknowledged 效果回执只随当前 P6 session 存在；它不得成为 P5 长期效果、撤离后 Buff、死亡后补偿、重连奖励或 Code A 终局依据。

## 3. P15 产品裁决

P15 只完成一条明确路径：

    玩家在活动 P6 基础 6 格中拥有一个 QuickUsable、无 child-container graph 的简单消耗品
    → 该 ItemId 已由 P13 显式绑定到一个 1—9 槽
    → 游戏正常输入状态下按下对应数字键一次
    → Code B 重新验证后原子扣除一份并提交确定性效果回执
    → Code A 仅在同一有效 Run 中按既有生命上限规则幂等处理 RestoreHealth
    → 回执得到确认，物品与生命均不存在第二份可写真值

P15 只开放数据驱动的 RestoreHealth 效果类别。恢复量必须来自明确的 Code B QuickUse 定义或命名配置，而不是 Widget magic number、按键分支、显示文本或 Code A 物品表。若现有生产定义已经能表达 QuickUsable 消耗品，则复用其稳定 DefinitionId；若该定义缺少最小效果描述，允许仅为这一现有消耗品类别增加显式的 RestoreHealth 描述与安全默认值。不得为演示目的新增 starter grant、修改 P5 初始库存、修改 P9/P10 或 P11/P12 固定 recipe、随机生成物品，或以 fixture 数据伪造产品可达性。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

建立由 P13 热键引用发起、P6 durable transaction 授权、Code A 生命系统幂等落地的单一消耗品使用链。成功一次使用只会减少同一 P1 ItemId 的数量一份，并创建一个可恢复、不可重复结算的 P6 QuickUse receipt；它不会移动任何其他物品、不会创建新 ItemId、不会改变容器结构，也不会让 Code A 拥有库存或扣除物品。

### 4.1 QuickUse 定义、P6 receipt 与迁移

1. 在 Code B 物品定义中引入或最小扩展一个明确的 QuickUse effect descriptor。P15 只支持：

       EffectKind = RestoreHealth
       Positive, data-defined RestoreAmount

   只有现有的 bQuickUsable 消耗品类别可以拥有该 descriptor。武器、道袍、饰品、空间戒指、储物囊、非 QuickUsable、无定义、未知 effect、零／负恢复量或拥有 ChildContainerId 的实例均不可使用。
2. P6 schema 从 P14 当前 schema 4 升级至 schema 5，并新增最小、版本化的 QuickUse receipt state。每张 receipt 至少能够稳定表达：由 OwnerId + RunInstanceId + 已提交单调 ordinal 导出的 ReceiptId、SlotIndex、SourceItemId、不可变 EffectKind、不可变 RestoreAmount，以及 Pending／Acknowledged 的最小 delivery state。receipt 是一次物品使用的审计与效果投递令牌，不是物品副本、数量副本、Definition 镜像、冷却表、Buff 列表或第二份库存。
3. 只有在 P5 durable data 实际需要序列化新增 QuickUse definition 字段时，才以最小兼容方式升级 P5 schema；旧定义必须安全读为无 QuickUse effect，或按 P13 已有的权威 Consumable category 得到明确的 RestoreHealth 映射。不要为了 P15 修改 P5 物品图、初始物品、Profile lock、bridge、配置来源或历史 item migration。
4. 旧 P6 schema 4 session 迁移时获得显式空 receipt 状态和安全的初始 ordinal。未完成迁移、重复 ReceiptId、外部 Owner／Run receipt、未知 effect、无效 amount、已 closed／terminal session 或无法通过 P1/P6 校验的 document 均必须拒绝使用请求，保持零物品写入、零生命调用。
5. effect receipt 的完成与清理只能遵循精确 P6 session。可以保留当前 pending receipt 与其必要的最小 acknowledged 去重信息，或使用等价的有限 receipt ledger；不得用 Actor pointer、UI 地址、玩家显示名、随机 runtime GUID、旧 Run Save key 或按键状态生成身份。

### 4.2 数字键请求、资格与单次原子消耗

1. Code A 仍拥有 1—9 原始输入派发。仅在正常游戏输入状态、当前 Code A Run 有效、玩家存活、当前 Health 低于 Code A 自己的 MaxHealth、非 terminal、没有已挂载的 P3／P4 物品目标正在占用此输入、且数字键是一次离散 Pressed event 时，才可由 Progression Manager 向 Code B 请求 UseBoundQuickSlot。这个 Health preflight 只决定是否转发本次 intent，不把 Health 写入 P6，也不成为物品真值。按住、重复触发、UI 文本／详情／绑定操作、双击、右键、鼠标点击、I、Esc、Close、Cancel、空格、外部区域、满血、无当前 session 或任何 Actor interaction 都不得形成使用、数量写入或 Code A 效果调用。
2. Store-owned QuickUse service 每次都必须重新验证精确 OwnerId + committed RunInstanceId、P6 revision、P13 SlotIndex、唯一 binding ItemId、当前 P1 graph、父容器恰为 P6 Container.Player.BaseQuick、数量大于零、bQuickUsable、允许的 RestoreHealth descriptor、简单实例资格、P6 active gate 与非 Prepared／terminal／closed 状态。
3. 合法 request 必须以同一 Owner durable replacement 原子完成：

       重新验证当前 P6 + P1 graph
       → 同一 ItemId 数量减一，归零时仅移除该 ItemId
       → 执行既有 P13 binding reconcile
       → 消耗一个新的确定性 receipt ordinal 并加入 Pending QuickUse receipt
       → 验证最终 P1/P6 graph 与 receipt
       → 保存一次 Owner record

   若 ItemId 尚有正数量，原 hotbar binding 可继续指向同一 ItemId；若归零或其他既有图规则使引用无效，只由 P13 reconcile 在同一 commit 清除该 binding。P15 不自动换槽、重新绑定、移动物品、恢复旧 binding 或生成替代物品。
4. 任一身份失配、stale revision、无 binding、binding 已失效、物品不在 BaseQuick、数量不足、descriptor 无效、P1/P6 校验失败、冲突、保存失败、输入重复或 delivery 尚未可接受时，必须使 P1 图、P13 bindings、P6 receipt、ordinal 和 Code A 生命均保持旧状态。不得先扣数量再试图投递效果，或由 Code A 失败时直接改写、退款或重建 P1 物品。
5. P15 不加入 cooldown、连续使用、按住自动使用、队列多次消费、全局动作条、动画、音效、施法条、技能触发、网络同步、装备被动、Buff、Debuff、伤害、法力、治疗以外的属性或任意其他 effect。一次合法按键最多提交一张 P6 receipt。

### 4.3 Code A RestoreHealth delivery adapter

1. 在 Progression Manager 与现有 Code A 玩家生命服务之间建立最小单向 adapter。它只能读取当前 exact P6 Pending receipt 的不可变投递信息，并调用 Code A 自己已有或最小扩展的 RestoreHealth 入口。Code A 必须继续自行裁定当前 Player 是否存在、是否 alive、Health／MaxHealth 以及最终的 capped restore amount；Code B 不读写 Code A Health、不计算当前血量、不改 HUD，也不决定死亡或终局。
2. Code A delivery 必须按 ReceiptId 幂等。若同一 receipt 因 UI 重开、Actor 重建、Manager 刷新或 acknowledgement 重试被再次观察，Code A 不得重复恢复生命或重复修改 HUD。必要时，Code A 可在其自己已有的当前 Run health lifecycle 内保存最小的已处理 ReceiptId 集合；该集合只服务 Code A 生命效果去重，不保存 ItemId 位置、数量、容器、hotbar binding、Definition 或任何 Code B 库存图。
3. 只有 receipt 已由 Code B 保存成功后，Manager 才能请求 Code A delivery。Code A 成功处理或确认已处理同一 ReceiptId 后，才可调用 Code B 的 exact-session acknowledgement；acknowledgement 只能更新该 receipt delivery state，绝不能再次扣数量、生成第二张 receipt、移动物品或修改 P13 binding。
4. Code A 缺少 Player、Run 已无效、玩家死亡、终局已提交、receipt Owner／Run 不匹配或 restore entry 无法处理时，不得把失败解释为新的物品使用、不得让 Code A 假造 receipt、不得从旧库存／UI／Actor 创建恢复。该 receipt 保持 P6 pending，直至合法同 session delivery 或 P8 close；P8 close 只丢弃未投递 receipt，不退款、不把物品或效果带回 P5。
5. adapter 不得改写 Code A 的输入、战斗、伤害、死亡、撤离、HUD、Run Save 的物品部分、正式终局分类或 P8 观察顺序。它也不得触发 P9/P10、P11/P12、P14 actor、旧 Loot、地图容器、搜索或地面投影。

### 4.4 生命周期、终局与 UI 不变性

1. P5→P6 bridge 只按 P13 已有规则迁移合法 hotbar references与真实物品。P15 不在 bridge 时预扣消耗品、创建 receipt、预施加效果、修改 Prepared receipt 或向 P5 写入 cooldown／pending effect。
2. P6 Prepared／RecoveredAbandon rebind 合法保留同一 active snapshot 时，只能连同该 snapshot 原样保留仍合法的 P15 receipt state；不得从 P5、Code A、Actor、UI 或按键缓存补造、重放或猜测物品使用。若 rebind 不能安全恢复 Code A 当前 health delivery context，应保持 pending 并由同一 session 的合法 delivery／P8 close 处理，不得自行扩展 recovery 产品规则。
3. P8 Extracted 仍只把最终仍在 P6 玩家图中的合法物品与 P13 references按既有规则归还 P5。已经消耗的数量永不回流；Pending／Acknowledged P15 receipt 均在 P8 terminal close 中丢弃，不能在局外页面、下一个 Run 或死亡／放弃后补发效果。Dead 与 RecoveredAbandon 同样不退款、不恢复 item、不保留 receipt。
4. P13 UI 继续只做明确 Bind／Unbind 和只读显示。P15 可以为热键可用性提供纯投影状态，但不得让 UI 按钮、详情、双击、右键或选择态触发 UseBoundQuickSlot。P7、P10、P12、P14 的真实 Drag／Drop 位置写入链和各自的 transient target 语义保持不变。
5. P15 不新增 item instance、starter grant、固定 recipe 变化、随机 Loot、普通容器／尸体／地面新来源、自动拾取、直接 Actor pickup、Split、Merge、Swap、装备／空间道具移动、地面多物品容器、物理 1—9 非消耗品效果或 Code A／Code B 双写。

## 5. 允许的改动范围

允许：

- 在 Code B item definition、P6 durable schema、migration、projection 与 owner-record transaction 中增加最小 RestoreHealth QuickUse descriptor、确定性 receipt、单次数量消耗、acknowledgement 与 P13 reconcile 适配；
- 在现有 Code A input → Progression Manager 边缘增加 1—9 的离散 Use request 转发，并在现有 Code A player health lifecycle 中增加 receipt-idempotent RestoreHealth adapter；
- 仅在 delivery 去重确有必要时，向 Code A 当前 Run health lifecycle 增加最小 receipt acknowledgement metadata，且不得包含任何 Code B 物品真值；
- 为 P8 精确 session close 加入 P15 receipt 丢弃的最小适配；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档和本任务 Report。

## 6. 明确不在本任务内

不得实现、启动、重构或接管：

- 新的 starter item、P5 初始库存、P9/P10 或 P11/P12 recipe 修改、随机 loot、资源掉落、第二种容器、敌人掉落、世界拾取、自动拾取、直接 Actor pickup、地面多物品、装备／空间道具整体移动；
- Split、Merge、Swap、QuickMove、自动整理、自动绑定、自动换槽、热键绑定写入、按住连发、cooldown、动作条、技能、法术、施法、动画、音效、治疗以外属性、Buff、Debuff、武器使用、装备效果、网络同步或多人；
- Code A 的地图、Player、战斗、伤害、死亡、HUD、Run、Run Save 物品真值、旧库存、旧 Loot／搜索、正式结算或终局分类权威；
- P1 的一般图语义、P5/P6 bridge、Prepared receipt、RecoveredAbandon recovery、Start Run 条件、Profile lock、P8 terminal receipt／分类／恢复、P9/P10、P11/P12、P14 world truth 或任何 A/B 双写；
- 实际运行、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 QuickUse definition 与 schema migration，确认只有明确 QuickUsable + RestoreHealth descriptor 可进入本链，definition 默认／旧数据不会伪造可用效果，P6 receipt 只保存稳定审计和 delivery 所需信息而非第二份库存。
2. 审查 1—9 输入边缘与 Store-owned service，确认只有一次正常 gameplay Pressed event 可提出请求；UI、双击、右键、选中、按住、重复、关闭、取消、无 session 和非 alive／terminal 情形均不能消耗或触发 Code A。
3. 审查原子 transaction，确认它先验证 exact P6 / P13 binding / P1 graph，再在一次 Owner durable save 内减少同一 ItemId 一份、reconcile bindings、创建一次 receipt；失败、冲突或保存失败无数量、binding、receipt、ordinal 或生命变化。
4. 审查 Code A adapter，确认 Code A 仅用 ReceiptId 幂等地按自己的 health authority 落地 RestoreHealth；它不保存或推断 P1/P5/P6 库存、不创建 receipt、不退款、不裁定 Code B 使用资格，且 acknowledgement 不会二次扣除。
5. 审查 P5→P6、Prepared／rebind、P8 Extracted／Dead／RecoveredAbandon，确认未消耗物品与合法 binding 保持现有迁移，已消耗数量和 P15 receipts永不回流 P5，terminal close 只丢弃 receipts，未改写 Code A 后置终局权威。
6. 审查 P4x、P7、P9/P10、P11/P12、P13、P14 与 Code A 边界，确认本轮没有新增非 Drop 位置移动、容器／尸体／地面 truth、QuickMove、通用物品使用、非 RestoreHealth effect 或 A/B mirroring。
7. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

8. 若编译失败，只修正 P15 引入的局部声明、include、definition schema、receipt migration、P6 transaction、input forwarding、health delivery adapter、acknowledgement 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要扩展到其他 effect、Code A 物品真值、P5 starter／recipe、P16 或 0.0.9B.F，停止受影响工作并报告。

## 8. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P15.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. P15 新增／修改／未修改文件及职责；
2. QuickUse RestoreHealth descriptor 的稳定来源、旧数据 migration、实际生产数据可达性状态，以及本轮没有新增 starter／recipe／随机物品的结论；
3. P6 schema migration、ReceiptId 组成、receipt 最小字段与为何它不是第二份库存或 Code A effect truth；
4. 1—9 Pressed request、精确 use 资格、同一 ItemId 数量扣减、P13 reconcile、保存失败／冲突零写入与一次 Owner durable commit 的结论；
5. Code A RestoreHealth adapter、ReceiptId 幂等、acknowledgement、health authority 与 pending receipt 在 Actor／UI refresh 下的处理；
6. P5→P6、Prepared／rebind、P8 Extracted／Dead／RecoveredAbandon 对未消耗物品、已消耗数量和 P15 receipts 的生命周期结论；
7. 对 P4x、P5、P6、P7、P8、P9/P10、P11/P12、P13、P14 与 Code A 权威的静态审查结论；
8. 实际 Editor 编译命令、目标、最终 native exit code 和关键结果；
9. 明确列出未执行的 F 阶段项目：真实 1—9 输入、满血／死亡／终局、重复按键、UI 焦点、save/restart、receipt delivery、P13 binding 清理、P8 terminal、CTA、wrapper、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验证均由 0.0.9B.F 负责；
10. 明确列出尚未启动的后续功能：其他消耗品 effect、cooldown／连续使用、武器或装备使用、空间物品使用、随机 world loot、全敌人／全容器迁移与其他 P 阶段。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code 0 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P15_COMPILE_REWORK

若无法在不让 Code A 持有 P1/P5/P6 物品真值、或不破坏 P8／现有生命终局权威的前提下建立单一 receipt delivery 边界，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P16、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P15.0.r0","file":"Dev.D.UE.0.0.9B.P15.0.r0_report.md"}
