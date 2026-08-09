# Dev.D.UE.0.0.9B.P11.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P11——Code B 尸体／身体容器的死亡回执、一次性物质化与 Run-local 持久化
- 任务编号：Dev.D.UE.0.0.9B.P11.0.r0
- 任务性质：在 P10 已完成首个普通地图容器的完整纵向切片后，为一个真实生产敌对单位建立独立的尸体／身体容器真值。P11 只处理 Code A 已提交死亡后的单向死亡回执、稳定身份、一次性 Code B 物品图物质化、残余物清理和静态只读 projection；不制作搜尸交互、尸体页面、揭示、拖拽或转移。P 阶段只做功能开发、静态代码审查、Editor 代码编译和必要的最小编译修正；真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验收统一留给 0.0.9B.F。
- 执行文件：Dev.D.UE.0.0.9B.P11.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P11.0.r0_report.md
- 活动工程根：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B
- 活动工程：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject
- 引擎：C:\\Program Files\\Epic Games\\UE_5.8
- 工程级开发基线：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r0/r1 与 P10.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P11 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 已有所有权与数据边界

1. Code A 继续拥有默认地图、敌人 Actor 的生成／存在／死亡表现、Player Actor、输入动作原始派发、战斗、伤害、死亡判定、正式 Run 生命周期、撤离／死亡分类、HUD、Run Save、旧库存和所有尚未迁移目标的旧 Loot／搜索运行时。
2. Code B 的 P1 Repository 是新物品体系唯一可变真值。P5 是局外 Code B Profile snapshot；P6 是精确 OwnerId + RunInstanceId 的活动 Run 玩家携带 snapshot；P8 是 Code A 已完成终局后才执行的 Code B session 归还／没收与关闭。不存在 A／B 镜像、同步或双写。
3. P7 是 P6 玩家自身装备、基础 6 格、已装备空间戒指快捷空间和储物囊内部格的生产 Host。P7 的玩家内布局仍只允许由已挂载生产 Cell 的明确 Drop 写入；双击、右键、详情、I、Esc、关闭、空格和禁用 1—9 均保持零写入。
4. P9 是普通容器唯一的 Run-local 真值。P10 已把一个 `CodeB.NormalContainer.BasicCache` 目标接到真实地图、开启、单件揭示和 P6/P9 原子跨图转移。P9/P10 的记录、状态、recipe、target identity 和语义不得被 P11 改写、泛化或作为尸体容器的隐式别名。
5. P11 必须建立独立的 BodyContainer Run-local record。其唯一身份为 OwnerId + RunInstanceId + BodyTargetId + DefinitionId；其物品图、ContainerId、ItemId、ChildContainer graph、death receipt、definition digest 和可见状态只由 Code B 持久化。Code A 不得成为该图、物品状态或容器内容的第二份真值。
6. Code A 对 P11 的唯一新增职责是：在已确认且不可逆的死亡结果之后，向 Code B 单向转发一个受静态生产身份约束的死亡回执。它不创建 Code B 物品、不选择 Code B contents、不保存 Code B record、不驱动 P6/P5 写入，也不从 Code B 反向取得战斗、死亡、AI、Actor 或 Run 结论。

## 3. 持续有效的物品、输入与终局规则

1. 每个真实物品保持一个 ItemId、一个父位置、稳定 ContainerId／ChildContainerId 和一次权威提交。不得复制、重新编号、部分移动、重建或临时镜像真实物品。
2. 普通储物格没有类别互斥；只有装备栏实施类别与单槽互斥。空间戒指只有在装备后才开放快捷空间；空间储物囊始终是非快捷内部容器。带内部内容的空间物品整体 Move、Swap、Equip 或 Unequip 继续由现有 P1 原子拒绝。
3. P11 不增加任何玩家物品位置写入入口。不得新增 QuickMove、自动拿取、自动放回、自动整理、双击、右键菜单、详情按钮、热键、搜尸按钮、自动掉落或隐藏命令。
4. P8 仍只在 Code A 已完成终局分类之后执行。它只结算已存在于 P6 玩家携带图中的物品；P9 与 P11 的未取得剩余内容都必须随活动 session 关闭而丢弃，绝不能回流 P5、P6、旧库存、Code A Loot 或世界物品。
5. 本任务不建立尸体地图交互、搜尸 UI、物品揭示、真实 Drag Drop、尸体 Actor 附着、掉落物、地面物品、灵石领取、快捷栏、消耗品、装备效果、战斗属性或敌人行为。上述功能属于后续独立 P 阶段。

## 4. P11 产品裁决

P11 只为一个真实生产敌对单位建立「死亡后才存在」的 Code B 身体容器基础。该单位仍由 Code A 决定生成、受伤、死亡、销毁、战斗奖励与 Run 流程；Code B 只在 Code A 提交死亡之后接收回执并在同一 Owner durable record 中生成一次可供后续任务读取的身体容器图。

    Code A 已提交的死亡结论 + 稳定 Spawn/Encounter 身份
    → 单向、可去重的 DeathReceipt
    → 精确 OwnerId + RunInstanceId + BodyTargetId + DefinitionId
    → Code B 一次性 BasicCorpse 图物质化（全部 Hidden）
    → P11 只读 BodyContainer projection
    → P8 终局时丢弃尚未取得的 P11 残余

P11 不是将所有敌人、尸体或旧敌人 Loot 系统全量迁移，也不是 P10 普通容器的重构。第一个已绑定生产敌对单位足以建立后续 P12 搜尸纵向切片所需的数据基础。

---

# 下半部分：授权执行内容

## 5. 单一授权目标

为一个真实生产敌对单位实现独立的 `CodeB.BodyContainer.BasicCorpse` Run-local 真值：Code A 在其自身死亡提交完成后只发送稳定、可审计的死亡回执；Code B 以精确身份去重并一次性物质化有效身体容器图。该图全程只读、所有物品初始 Hidden、未挂载 UI、未允许转移，并会由 P8 在 session 终局时丢弃残余内容。

所有写入必须落在同一 Owner durable Code B record 的单次提交中。P11 不得以敌人 Actor、尸体视觉、UI、临时 Cache、旧 ItemSubsystem、旧 Enemy Loot、第二份 SaveGame、Run Save、fixture 或随机 runtime GUID 作为尸体容器真值。

### 5.1 生产身体容器定义与稳定身份

1. 新增或显式声明一个最小的 Code B 定义 `CodeB.BodyContainer.BasicCorpse`。它必须有稳定 DefinitionId、版本／digest 与合法 P1 layout；只可使用已有的 Code B 物品定义，或为该最小原型增加一个命名清晰、无战斗效果、无消费效果、无快捷栏语义的固定内容 recipe。
2. 选择且只选择一个现有生产敌对单位／Encounter spawn 作为 P11 绑定源。为它提供地图作者和代码审计均可见的稳定 `BodyTargetId`。该 ID 必须从静态 Encounter／Spawn identity 与确定性的 spawn ordinal 派生；不得使用 Actor pointer、ObjectName、显示名、临时 controller、UI 索引或 runtime 随机 GUID。
3. 每次死亡回执还必须携带稳定的 `DeathReceiptId` 或等价、可持久验证的 death sequence。它必须能证明「同一 Owner、同一 committed Run、同一 BodyTargetId、同一死亡」；重放同一回执、同一 Actor 的重复销毁通知或恢复后的重复通知均不得生成第二个 ContainerId、ItemId、receipt 或内容图。
4. P11 record 使用完整 OwnerId + RunInstanceId + BodyTargetId + DefinitionId gate。缺少／Prepared／terminal／已关闭／失配 P6 session、非法 Definition、重复 BodyTargetId、失效 death receipt 或 stale revision 一律零写入、零 fallback、零 rebind。
5. 不得复用 P9 的 BasicCache record、SearchTargetId、容器状态或 recipe 作为尸体容器的别名。可复用 P1 图校验、durable transaction、可见状态枚举和只读 projection 的通用实现，但不得为了复用而修改 P9/P10 的既有语义。

### 5.2 Code A 死亡边缘与单向回执

1. 先静态审阅当前绑定源的死亡链。仅在 Code A 已提交该敌人的死亡／不可继续战斗结论之后接入最窄的 production adapter；该 adapter 只能转发 OwnerId、committed RunInstanceId、BodyTargetId、DefinitionId、DeathReceiptId 和必要的静态 provenance。
2. Code A 继续拥有伤害、血量、死亡时序、AI、Actor 销毁／尸体表现、击杀归因、任务、经验、地图、Run、Run Save 与终局判定。P11 不得让 Code B 拒绝、保存失败、record 缺失或 projection 缺失改变 Code A 已完成或将来发生的死亡处理。
3. 若绑定敌人存在会对同一物理物品产生旧 Code A loot／drop 写入的窄出口，只可对该单一绑定源断开或旁路该出口，以杜绝 A/B 双写；不得修改其他敌人、全局掉落规则、战斗奖励或资源规则。若无法只切断同一物品真值而不改变更广泛的 Code A 产品行为，停止受影响部分并报告 `NEEDS_PLANNER_DECISION`，不得猜测替代方案。
4. Code A adapter 不得从 body record 读取数据来决定敌人是否死亡、何时销毁、是否重生、是否显示尸体、是否结算、是否给予旧奖励或是否开始／结束 Run。P11 不添加反向 callback、轮询同步或 A/B 镜像。
5. 敌人 Actor 在回执后被立即销毁、世界卸载或其视觉对象不存在，不得抹除已成功提交的 Code B body record；反之，Actor／死亡回执不存在时也不得由 Code B 自行制造尸体容器。

### 5.3 一次性物质化与身体容器状态

1. Code B 仅在首次接受一条完整、合法的死亡回执时物质化身体容器。一次逻辑事务中创建稳定 ContainerId、全部 ItemId／ChildContainer graph、definition digest、death receipt 和初始 visibility；任一 P1 校验、持久化或 revision 条件不成立时，整个 record 保持不存在或原样，不得半提交。
2. P11 的最小持久状态为：无 record（来源仍存活或未被接受）→ `BodyMaterialized`；重复相同 death receipt 仅返回既有 projection，不重新抽取、不换 recipe、不改 ItemId、不改 ContainerId。只有 P8 session terminal finalizer 可将其变为已丢弃／移除的残余状态。
3. 初始 body graph 的每个可取得物品均为 `Hidden`。P11 可提供后续 P12 所需的只读 slot metadata，但不得在 P11 提供 reveal、searching、item detail、icon、drag payload、容器页面、输入、Actor interaction 或任何玩家可见写入行为。
4. 内容 recipe 必须固定、命名、可审计且完全由 Code B 定义裁决；不得读取随机 Actor 状态、旧 Code A inventory、旧 Enemy Loot、UI、临时 Cache 或世界对象来临时决定内容。不得在本任务加入随机掉率、概率池、金币／灵石散落、掉落动画、地面物品或自动给入 P6。
5. P11 不得把身体容器图写入 P5、P6、P7、P9、Run Save、旧库存或 Code A Actor。其唯一合法落点是匹配 Owner record 中的 P11 BodyContainer Run-local section。

### 5.4 P8 残余清理与恢复边界

1. 以最小扩展让 P8 的既有 terminal finalizer 识别 P11 BodyContainer section：当且仅当 Code A 已完成终局分类且 P8 正在关闭精确 session 时，删除／丢弃该 session 的 P11 未取得残余。
2. 此扩展不得把任何 P11 物品移入 P6、P5、旧库存、Code A Loot 或世界；不得改写 P8 的撤离返还、死亡没收、RecoveredAbandon、idempotency、lock、顺序或 Code A terminal authority。
3. Process recovery、重复 P8 通知、已经清理的 section、无 P11 body record 或不匹配 Owner／Run 都必须无害且不生成／返还任何物品。P11 不得启动新的 rebind、session、migration 或补偿路径。
4. P11 可暴露给后续 P12 的只读 `BodyContainerProjection` 查询，但该查询必须完整 gate、不得构造空白 fallback 容器、不得在 read 时迁移／物质化／修复数据，也不得挂接生产 UI。

## 6. 允许的改动范围

允许：

- 新增 Code B P11 BodyContainer definition、Run-local record、固定 recipe、一次性物质化、死亡回执验证和只读 projection；
- 为单一绑定生产敌对单位接入最小 Code A 死亡回执 adapter，必要时仅对该源切断同一物品真值的旧 Code A 写入出口；
- 仅为 P11 残余丢弃而最小扩展 P8 finalizer；
- 复用但不改变 P1 图校验、P6 session gate、P8 的既定事务／恢复语义，以及不可写的 P7/P3/P4 production host；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档和本任务 Report。

## 7. 明确不在本任务内

不得实现、启动、重构或接管：

- 尸体地图 Actor、尸体可交互提示、输入、搜尸 UI、P7 双栏挂载、搜索读条、Hidden → Searching → Revealed、Drag Drop、Move、Merge、Swap、物品取得、尸体关闭／重开可见行为；
- 所有敌人、所有尸体、第二种身体容器 definition、P9 普通容器全量迁移、敌人死亡掉落、灵石领取、世界掉落／拾取、地面物品或资源奖励；
- Code A 的敌人生成、伤害、战斗、AI、死亡判定、死亡视觉、击杀奖励、地图、任务、HUD、Run、终局分类、Run Save、旧库存、正式结算或返回值权威；
- P5 的局外页面或迁移、P6 bridge／Prepared receipt／RecoveredAbandon rebind、P7 玩家背包 UI、P9/P10 普通容器语义、P8 核心 terminal 语义、Profile lock 或复原语义；
- 快捷栏 1—9、物品使用、消耗品、装备效果、自动整理、自动拾取、QuickMove、双击、右键、详情写入、分堆 UI、随机 loot table、掉落动画或任意 A/B 双写；
- 实际运行、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 8. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查绑定敌对单位、BodyTargetId、DefinitionId 与 DeathReceiptId，确认它们可持久、可去重，且不依赖 Actor pointer、随机 GUID、显示名、UI 或临时 runtime 状态。
2. 审查 Code A adapter，确认它只在 Code A 已提交死亡之后单向发送最小回执；未获得稳定回执、session 不匹配或 Code B 拒绝均不会改变 Code A 死亡、AI、地图、Run、奖励或终局。
3. 审查 P11 一次性物质化，确认固定 recipe、ItemId、ContainerId、ChildContainer graph、death receipt 和 Hidden visibility 只以完整 P11 identity gate 在同一 Owner durable transaction 中创建；重放、失效、冲突、保存失败和 stale revision 均无半提交或 duplicate。
4. 审查旧 Code A loot／drop 的边界。确认只在存在同一物品真值时、且仅限单一绑定源时才局部断开；不存在全局改写、全局奖励移除、A/B mirror、同步或 dual write。
5. 审查 P8 扩展，确认它只将 P11 残余纳入既有 session discard set，不返还／没收／补偿／生成 P11 内容，不改写 P6、P5、P9、P10、RecoveredAbandon 或 Code A terminal authority。
6. 审查 P7/P10 输入与 UI，确认本轮没有新增可见 UI、输入、QuickMove、非 Drop 位置写入口、body item detail／payload 或玩家／尸体转移路径。
7. 编译一次 Editor 目标：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex

8. 若编译失败，只修正 P11 引入的局部声明、include、类型、schema migration、持久化结构、death receipt、P8 residual discard 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要越过本 Prompt 边界，停止受影响工作并报告；不得自行扩展到 P12、0.0.9B.F 或其他任务。

## 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P11.0.r0_report.md，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须简洁、可审计地列出：

1. P11 新增／修改／未修改文件及职责；
2. 选定生产敌对单位、BodyTargetId、DefinitionId、DeathReceiptId 组成方式、Code A adapter 与其不改变 Code A 权威的理由；
3. P11 record 的 identity gate、schema／migration（如有）、状态、固定 recipe、一次性物质化、Hidden visibility、ItemId／ContainerId／ChildContainerId 与重复回执处理；
4. 是否存在同一物品真值的旧 Code A loot／drop，以及为单一绑定源采取的局部去重方式；若无安全窄切口，必须如实说明并使用阻塞状态；
5. P8 如何只丢弃 P11 未取得残余，而不改变 P6 settlement、P9/P10、RecoveredAbandon 或 Code A terminal authority；
6. 对 P4x、P5、P6 Prepared rebind、P7 Drop-only、P8 finalizer、P9/P10 普通容器与 Code A 权威的静态审查结论；
7. 实际 Editor 编译命令、目标、最终 native exit code 和关键结果；
8. 明确列出所有未执行的 F 阶段项目：产品运行、死亡／重复死亡／恢复／终局实际验证、CTA、wrapper、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证仍由 0.0.9B.F 负责；
9. 明确列出仍未启动的后续功能：P12 的尸体地图交互、搜尸 UI、揭示和真实 Drag Drop／转移。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code 0 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P11_COMPILE_REWORK

若不存在可安全复用的生产死亡挂点、稳定 spawn／body identity、death receipt 去重方案，或无法仅切断同一物品真值的局部旧写入出口而需要新的产品裁决时，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P12、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P11.0.r0","file":"Dev.D.UE.0.0.9B.P11.0.r0_report.md"}
