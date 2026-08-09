# Dev.D.UE.0.0.9B.P13.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P13——Code B 1—9 快捷栏引用生命周期
- 任务编号：Dev.D.UE.0.0.9B.P13.0.r0
- 任务性质：在 P7 已提供玩家基础 6 格与活动 Run 背包 Host、P8 已提供终局归还／没收、P10/P12 已提供普通容器／尸体到 P6 的真实拖拽转移后，建立 1—9 快捷栏的唯一引用真值、P5↔P6 生命周期、生产 UI 绑定／解绑与失效清理。本任务不启用按键使用、消费、属性或战斗效果。P 阶段只做功能开发、静态代码审查、Editor 代码编译与必要的最小编译修正；真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证均留给 0.0.9B.F。
- 执行文件：Dev.D.UE.0.0.9B.P13.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P13.0.r0_report.md
- 活动工程根：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B
- 活动工程：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject
- 引擎：C:\\Program Files\\Epic Games\\UE_5.8
- 工程级开发基线：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r1、P10.0.r0、P11.0.r0 与 P12.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P13 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 既有真值与所有权

1. Code A 继续拥有默认地图、Player Actor、输入原始派发、战斗、生命／伤害、正式 Run 生命周期、撤离／死亡分类、HUD、Run Save、旧库存、旧 Loot／搜索和所有尚未迁移的运行时。P13 不把快捷栏、物品使用或任何物品效果写入 Code A。
2. Code B 的 P1 Repository 是新物品体系唯一可变真值。P5 是局外 Profile 物品 snapshot；P6 是精确 `OwnerId + RunInstanceId` 的活动 Run 玩家携带 snapshot；P8 只在 Code A 已提交终局后处理 P6 的归还／没收与关闭。不存在 A／B 镜像、同步或双写。
3. P7 是 P6 玩家装备、基础 6 格、已装备空间戒指快捷空间和空间储物囊内部格的生产 Host。所有真实物品位置变化仍只允许由已挂载生产 Cell 的明确 Drop 提交。双击、右键、详情、I、Esc、Close、Cancel、空格、UI 外区域和无 payload Drop 均不得成为物品位置写入入口。
4. P9/P10 是普通地图容器唯一的 Run-local 真值；P11/P12 是唯一绑定 Low Skirmisher 身体容器的 Run-local 真值。它们的身份、内容、揭示、开启与跨图转移语义保持不变，不能保存、拥有或成为任何快捷栏引用的第二份真值。
5. 现有“基础 6 格”是实际 P1 物品容器；1—9 快捷栏不是容器、不是额外格子、不是数量副本，也不是装备栏。空间戒指的“快捷空间”仍只是已装备空间道具的内部普通储物，不能因此变成 1—9 快捷栏来源。

## 3. P13 产品裁决

P13 只完成“一个真实物品如何被稳定引用到 1—9 快捷栏，并在 Profile、活动 Run、物品移动与终局之间保持不悬挂、不复制”的功能纵向切片。

P13 的最终数据模型必须等价于九个固定槽位：

    SlotIndex [1..9] + Referenced ItemId | Empty

其中 `ItemId` 必须是 P1 中已存在的真实物品实例。快捷栏不得保存 `Quantity`、Definition 副本、ContainerId 副本、ChildContainerId 副本、Widget 指针、Actor 指针、UI 索引或 runtime GUID。物品的名称、图标、数量、可用性和位置一律从当前 P5／P6 的权威 projection 读取。

本轮只开放“绑定／解绑引用”这一非位置写入。物理 1—9 按键、消耗、物品数量扣减、冷却、属性、治疗、战斗调用和任何 Code A 效果适配均保持未实现／零写入，留给后续独立 P 阶段。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

实现 Code B 的生产快捷栏引用服务与 UI：符合资格的、当前实际位于玩家基础 6 格的物品可被明确绑定到 1—9 中一个槽位；绑定仅保存其唯一 `ItemId` 引用。任何使该引用失效的已接受 P5／P6 物品图变更，都必须在同一 owner durable commit 中自动清理无效引用。P5、P6 与 P8 必须只在对应 snapshot 的合法生命周期内保存／迁移该引用，绝不复制实体物品或产生第二份可编辑快捷栏状态。

### 4.1 快捷栏持久化模型与资格

1. 在 Code B 同一 owner durable record 内为 P5 Profile 和 P6 active session 增加明确、版本化、可恢复的 `HotbarBindings` 或等价字段。它必须含恰好九个逻辑槽位，槽位索引稳定为 1—9；空槽显式表示为空，不得以 `ItemId=0`、显示文本或 UI 状态隐式猜测。
2. 每个非空引用只能指向当前同一 owner、同一 snapshot 内一个唯一的 P1 `ItemId`。同一个 `ItemId` 同时最多绑定一个 1—9 槽位；重新绑定到另一槽位必须在一项原子提交中移动引用，而不是复制出第二个引用。
3. 绑定资格固定为：物品存在、数量大于零、拥有 `Item.Property.QuickUsable` 或现有等价的明确 Code B 标记、且其当前父位置正是该 snapshot 的 `Container.Player.BaseQuick`。物品位于装备栏、空间戒指内部、储物囊内部、局外仓库、P9、P11、旧库存或任意世界／Actor 位置时均不得绑定。
4. P13 不新增背包格、不改变 P1 物品图、不搬动物品，也不为快捷栏创建新的 `ContainerId`、`ChildContainerId`、ItemId、库存数组、SaveGame、随机 seed 或 UI 真值。Bind／Unbind 只可改变这九个引用字段及其所属 snapshot revision。
5. 若现有 Code B 定义中没有任何带 `Item.Property.QuickUsable` 的物品，本任务仍完成通用引用服务、验证和生产 UI；不得为凑演示而添加 starter item、修改 P9/P11 固定 recipe、改写 P5 存档内容或加入消费效果。Report 必须如实列出该数据可达性状态，不得把它伪装成 F 阶段测试结论。

### 4.2 P5、P6 与 P8 的引用生命周期

1. P5 的局外 Profile 与 P6 的已 committed active session 不得同时持有同一真实物品的可编辑绑定。P5 已锁定时只读／不可操作；Active Run 的唯一可编辑快捷栏真值为精确匹配 `OwnerId + RunInstanceId` 的 P6 snapshot。
2. 在既有 P6 bridge 把某个已绑定的 P5 基础 6 格物品带入匹配 active session 时，随同一既有 owner durable transaction 将该物品的合法 binding 转入 P6，并从 P5 移除同一引用。未出战且仍留在 P5 的物品及其合法 binding 必须保持不变。不得改变 P6 Prepared receipt、rebind、Start Run 条件、携带物品选择、Profile lock 或 recovery 语义。
3. 在 P8 的 `Extracted` 原子 P6→P5 图合并中，只迁移那些仍指向已返回 P5、数量大于零、且当前位于 P5 基础 6 格的 P6 binding；其余 P6 binding 必须在同一终局 commit 中清除。`Dead` 与 `RecoveredAbandon` 只丢弃 P6 active snapshot 的 binding，绝不把其 ItemId 或 binding 回流 P5。
4. P8 的 terminal receipt、幂等、锁、分类、Code A 后置观察与 P5/P6 物品归还／没收语义均不可被 P13 重写。P13 仅使快捷栏引用作为 P5/P6 snapshot 的附属 metadata 随既有原子事务一起迁移或丢弃。
5. P6 的既有 Prepared 或 `RecoveredAbandon` rebind 一律保持原有规则。P13 不单独创建、恢复、迁移、重绑或补造快捷栏；若该路径已合法保留同一 P6 snapshot，则它只能随该 snapshot 原样携带并由共享的有效性清理规则处理。

### 4.3 引用失效清理与跨图移动

1. 为 P5／P6 的权威提交路径增加一个最小的、服务端／Repository 级别的 binding reconcile 步骤。它必须在任何接受的物品图变更完成验证后、同一 durable record 提交前，根据最终 graph 清除无效 `HotbarBindings`；Widget、Presenter、Actor、Input adapter 和 timer 不得直接修改 binding 或绕过此步骤。
2. Reconcile 至少覆盖：物品离开基础 6 格、数量变为零、ItemId 被合法 merge 吞并、物品被丢弃／没收、P6 session 关闭、P5↔P6 生命周期迁移，以及 P6 与 P9/P11 之间的已接受真实 Drop。物品在基础 6 格内部合法换位时，稳定 ItemId 的 binding 可以保留。
3. P9/P10 与 P11/P12 的跨图操作仍分别在 P6/P9 或 P6/P11 的既有原子事务中处理。P13 只能在该既有 transaction 内为最终 P6 graph 清理引用；不得把 binding 写到普通容器或尸体 record，不能反向把任何 item 从 P9/P11 拉回 P6，也不得更改它们的 reveal／target／recipe／终局残余策略。
4. Reconcile 只删除已失效的引用，绝不自动为新获得、移动到基础 6 格或从尸体／容器转入 P6 的物品绑定快捷栏。自动绑定、自动换槽、QuickMove、自动整理、自动使用和隐藏输入均不在本任务内。
5. 保存失败、revision conflict、身份失配、Prepared、terminal、closed 或不完整图校验时，物品图与快捷栏引用均必须保持旧状态；不得出现“物品位置已变而旧 binding 仍已提交”或“binding 已变而物品事务失败”的半提交。

### 4.4 生产 UI 与唯一绑定入口

1. 复用 P5／P7 已有的生产 Host、P3/P4 Cell 与 projection 架构，为当前 P5 Profile 页面和／或匹配 P6 active session 页面呈现九个只读引用槽。每个非空槽从权威 projection 显示现有物品的正常名称、图标和数量；空槽不伪造 ItemId 或库存。
2. 只增加一个命名清晰的显式动作链：选择符合资格的基础 6 格物品 → 进入 `BindHotbarSlot` 请求 → 选择 1—9 槽位。可提供一个显式 `UnbindHotbarSlot`／清空动作。二者都只经 Code B 的专用 binding application service 和同一 owner durable transaction 写入引用；不得借用 Item Drag Drop 把物品位置变更伪装成绑定。
3. 绑定／解绑是本 Prompt 明确授权的非位置写入例外；它绝不通过 Widget 直写 Repository，也不改变 P4/P7 的“物品位置只由真实 Drop 写入”规则。双击、右键、详情点击、I、Esc、Close、Cancel、空格、UI 外区域、无 payload Drop 及禁用的 1—9 按键不得触发 Bind、Unbind、物品移动或任何写入。
4. P13 不注册、重映射或激活物理 1—9 使用输入；已有 1—9 占位保持无物品位置写入、无 binding 写入、无 Code A 调用、无数量变化。快捷栏 UI 也不得在页面关闭、Host 失效、Run terminal 或当前 P6 session 缺失时构造可编辑 fallback。
5. P13 不实现消耗、技能、治疗、冷却、属性、动画、网络同步、装备效果、世界掉落或拾取。后续使用功能只能从最终 P6 权威 projection 解析 binding，重新验证 ItemId／BaseQuick／数量与 session，不能把本轮 UI 缓存当成真值。

## 5. 允许的改动范围

允许：

- 在 Code B P5/P6 durable record、projection 与 schema migration 中增加九槽快捷栏引用、生命周期迁移和可恢复有效性清理；
- 在 P1/P2 或其既有 owner-record transaction 边界增加最小 binding application service 与 post-graph reconcile，保持 P1 物品图语义不变；
- 在 P5/P7 的生产 Host、Presenter、Cell 或动作面板中加入 Hotbar 只读呈现和显式 Bind／Unbind 请求路径；
- 为 P8 的既有 P5↔P6 原子终局合并／没收补充引用随 graph 合并、清理或丢弃的最小适配；
- 仅为挂载／卸载生产 UI、传递已存在的精确 P6 身份或让 Host 在终局失效而修改最小 Code A UI 生命周期边缘；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档和本任务 Report。

## 6. 明确不在本任务内

不得实现、启动、重构或接管：

- 物理 1—9 按键、物品使用、消耗、数量扣减、治疗、属性、战斗、冷却、技能、装备效果或 Code A Player Actor 效果适配；
- 新的 Consumable／QuickUsable starter 定义、奖励 recipe 修改、P9/P11 内容修改、随机 loot、灵石领取、资源结算、世界掉落、世界拾取、地面 Actor、丢弃、自动拾取或空间道具整体移动；
- P6 bridge、Prepared receipt、RecoveredAbandon rebind、Start Run 成功条件、Profile lock 核心语义、P8 terminal receipt／分类／恢复，或 P5/P6 的现有物品图迁移策略；
- P7 玩家背包核心拖拽、P9/P10 普通容器、P11/P12 身体容器、Code A 地图、Player、输入原始派发、战斗、死亡、HUD、Run、Run Save、旧库存、旧 Loot 或正式结算权威；
- QuickMove、自动整理、自动绑定、双击、右键、详情写入、分堆、隐藏命令、任何 A/B 双写；
- 实际运行、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查快捷栏模型，确认它只有 `SlotIndex + ItemId` 引用，不保存数量、Definition、位置、ContainerId、ChildContainerId、Widget、Actor、临时 GUID 或第二份库存。
2. 审查绑定资格与服务入口，确认只有真实位于同一 snapshot `Container.Player.BaseQuick` 的 `QuickUsable` 物品可绑定，且 Bind／Unbind 只写引用、最多一 ItemId 对应一个槽位、不会改动物品图。
3. 审查 P5↔P6 与 P8 生命周期，确认 active session 不与 P5 并发编辑同一 binding；Extracted 只迁回合法 binding；Dead／RecoveredAbandon 只丢弃 P6 binding；Prepared／rebind、P8 终局权威和 Profile lock 均未被重写。
4. 审查 reconcile 与 P7/P10/P12 跨图事务，确认有效性清理在同一 owner durable transaction 内完成，不会产生悬挂引用、半提交、P9/P11 binding、自动绑定或非 Drop 位置写入。
5. 审查 UI 和 Code A 改动，确认只有显式 Bind／Unbind 可写绑定；1—9 物理按键、双击、右键、详情、I、Esc、Close、Cancel、空格、无 payload Drop 与失败路径均不写入，且没有 Code A 使用／属性／战斗接管。
6. 编译一次 Editor 目标：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex

7. 若编译失败，只修正 P13 引入的局部声明、include、类型、schema migration、projection、binding service、reconcile、UI 生命周期或调用签名问题，然后重新执行同一 Editor 目标。若修复需要越过本 Prompt 边界，停止受影响工作并报告；不得自行扩展到物品使用、世界物品、P14 或 0.0.9B.F。

## 8. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P13.0.r0_report.md，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须简洁、可审计地列出：

1. P13 新增／修改／未修改文件及职责；
2. 九槽快捷栏的持久化位置、schema migration、字段、唯一 `ItemId` 引用语义和 QuickUsable／BaseQuick 资格规则；
3. P5↔P6 bridge、P8 Extracted／Dead／RecoveredAbandon、rebind 与 Profile lock 的 binding 生命周期和不变性结论；
4. reconcile 的调用位置、它覆盖的 P7/P10/P12/P8 物品图变更，以及为什么它不会改变 P9/P11 或 P1 物品位置真值；
5. 生产 UI 的 Bind／Unbind 入口、所有明确保持零写入的输入，以及本轮没有启用 1—9 使用／消耗／Code A 效果；
6. 对 P4x、P5、P6 Prepared rebind、P7 Drop-only、P8 finalizer、P9/P10、P11/P12 与 Code A 权威的静态审查结论；
7. 实际 Editor 编译命令、目标、最终 native exit code 和关键结果；
8. 明确列出未执行的 F 阶段项目：真实绑定／重开／移动清理／终局清理／存档恢复／1—9 输入验证、CTA、wrapper、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验证仍由 0.0.9B.F 负责；
9. 明确列出尚未启动的后续功能：物品使用、消耗效果、世界掉落／拾取、空间道具整体移动、第二尸体／全敌人迁移、随机 loot 与其他地图容器。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code 0 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P13_COMPILE_REWORK

若现有 P5/P6/P8 结构无法在不改变其核心事务语义的前提下携带引用，或现有生产 Host 无法建立不与 P7 Drop 链冲突的显式 Bind／Unbind 入口，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P14、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P13.0.r0","file":"Dev.D.UE.0.0.9B.P13.0.r0_report.md"}
