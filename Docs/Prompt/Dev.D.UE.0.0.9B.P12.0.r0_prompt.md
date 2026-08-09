# Dev.D.UE.0.0.9B.P12.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P12——单一生产尸体的地图交互、搜尸揭示与双栏拖拽转移
- 任务编号：Dev.D.UE.0.0.9B.P12.0.r0
- 任务性质：在 P11 已为一个真实生产敌对单位建立独立的 Code B 身体容器死亡回执、一次性物质化与 Run-local 真值后，完成该单一尸体的局内功能纵向切片：死亡后地图交互边缘、开启／中断、逐件搜尸揭示、生产双栏页面，以及玩家与尸体之间的真实拖拽转移。P 阶段只做功能开发、静态代码审查、Editor 代码编译和必要的最小编译修正；真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验收统一留给 0.0.9B.F。
- 执行文件：Dev.D.UE.0.0.9B.P12.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P12.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 引擎：C:\Program Files\Epic Games\UE_5.8
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r0/r1、P10.0.r0 与 P11.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P12 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 已有所有权与数据边界

1. Code A 继续拥有默认地图、敌人 Actor 的生成／存在／死亡表现、尸体视觉与可交互距离、Player Actor、原始输入派发、战斗、伤害、死亡判定、正式 Run 生命周期、撤离／死亡分类、HUD、Run Save、旧库存和所有尚未迁移目标的旧 Loot／搜索运行时。
2. Code B 的 P1 Repository 是新物品体系唯一可变真值。P5 是局外 Code B Profile snapshot；P6 是精确 OwnerId + RunInstanceId 的活动 Run 玩家携带 snapshot；P8 只在 Code A 已完成终局分类后执行既定的归还／没收与 session 关闭。不存在 A／B 镜像、同步或双写。
3. P7 是 P6 玩家自身装备、基础 6 格、已装备空间戒指快捷空间和储物囊内部格的生产 Host。P7 的玩家内布局仍只允许由已挂载生产 Cell 的明确 Drop 写入；双击、右键、详情、I、Esc、关闭、空格和禁用 1—9 均保持零写入。
4. P9/P10 继续独立拥有普通地图容器的 Run-local 真值与单一 BasicCache 纵向切片。P12 可复用已验证的通用 P1 图校验、P3/P4 production Host 和双栏呈现能力，但不得将 BodyContainer 伪装为 P9 NormalContainer、改写 P9/P10 的 target identity／状态／recipe／转移语义，或让一个目标跨用两套容器记录。
5. P11 已为且只为下列生产来源创建独立 BodyContainer 基础：CodeB.BodyContainer.BasicCorpse，静态生产来源 M01.Encounter.LOW.Skirmisher.01，静态 spawn M01.Spawn.LOW.Skirmisher.01，ordinal 0。其 BodyTargetId 由静态地图身份确定；DeathReceiptId 由完整 OwnerId、RunInstanceId、BodyTargetId 和 DefinitionId 确定。P11 的 BasicCorpse 固定内容、P1 图、death receipt、definition digest 和首次物质化保持不变，初始物品均为 Hidden。
6. Code A 只在自身已经提交 EnemyState::Dead 后单向向 P11 发送最小死亡回执。Code B 的拒绝、保存失败、页面失败或尸体不存在，均不得改变 Code A 的伤害、死亡时序、AI、Actor 销毁／尸体表现、地图、Run、奖励、Run Save 或终局权威。

## 3. 持续有效的物品、输入与终局规则

1. 每个真实物品保持一个 ItemId、一个父位置、稳定 ContainerId／ChildContainerId 和一次权威提交。不得复制、重新编号、部分移动、重建或临时镜像真实物品。
2. 普通储物格没有类别互斥；只有装备栏实施类别与单槽互斥。空间戒指只有在装备后才开放快捷空间；空间储物囊始终是非快捷内部容器。带内部内容的空间物品整体 Move、Swap、Equip 或 Unequip 继续由现有 P1 原子拒绝。
3. 尸体内容在 P11 BodyContainer section 中仍是 Run-local 真值。P8 只结算当时位于 P6 玩家携带图的物品；所有仍位于 P11 BodyContainer 的内容——包括玩家主动拖回尸体的物品——都随精确 session 关闭而丢弃，绝不能回流 P5、P6、旧库存、Code A Loot 或世界物品。
4. 除本任务明确授权的已挂载生产 Cell 的真实 Drop 外，不得新增 QuickMove、自动拿取、自动放回、自动整理、双击、右键菜单、详情按钮、热键、搜尸即拿取、自动掉落、世界落地、隐藏命令或其他物品位置写入入口。
5. 只在完整匹配的 committed P6 session、已接受的 P11 body record、合法 BodyTargetId、合法 DefinitionId、可用 Code A 尸体交互边缘和非 terminal Run 同时存在时，才允许进入尸体功能链。任一缺失、Prepared、已关闭、失配、重复、冲突或 stale revision 都必须零写入、零 fallback、零 rebind，且不得阻塞 Code A Run。

## 4. P12 产品裁决

P12 把 P11 已经存在的单一 BasicCorpse 物品图接到一个真实生产尸体的局内交互面；它不改变尸体是否死亡、何时显示、何时销毁或是否授予任何 Code A 奖励。打开尸体只读取既有 P11 record；搜尸只把该 record 内单件物品由 Hidden 推进至 Revealed；取得或放回只由双栏 production Cell 的真实 Drop 在同一 Owner durable record 内原子修改 P6 与 P11 图。

    Code A 已提交死亡并保留自己的尸体交互边缘
    → 精确 OwnerId + RunInstanceId + BodyTargetId + DefinitionId
    → 读取既有 P11 BodyContainerProjection（不得物质化）
    → Code B 尸体开启／中断与单件揭示
    → 同一生产 Host 的玩家栏 + 尸体栏
    → 真实 Drag Drop
    → P6 玩家携带图 + P11 尸体图的单次 durable commit
    → P8 终局只结算 P6，并丢弃 P11 残余

本任务只覆盖该一个 Low Skirmisher 尸体。它不是全敌人尸体迁移、普通容器重构、地图掉落系统或 Code A Loot 替换。

---

# 下半部分：授权执行内容

## 5. 单一授权目标

实现单一生产尸体的 P12 纵向切片：当 Code A 已提交绑定敌对单位死亡、且 P11 已对同一 committed session 成功物质化 BasicCorpse 时，玩家可通过 Code A 自己的尸体交互边缘打开 P7 production Host 中的尸体双栏页面，逐件揭示其中的 Hidden 物品，并只通过真实拖拽在 P6 玩家图和 P11 尸体图之间执行原子 Move、兼容 Merge 或双方合法时的 Swap。

P12 的所有物品与状态写入必须落在同一 Owner durable Code B record 的单次提交中。P12 不得用敌人 Actor、尸体视觉、Widget、临时 Cache、旧 Enemy Loot、Run Save、fixture、随机 runtime GUID 或第二份 SaveGame 作为尸体内容真值。

### 5.1 尸体地图交互与 Code A 最小边缘

1. 静态审阅绑定来源的现有死亡、尸体表现、可交互 prompt 与距离链。优先复用 Code A 已有的尸体视觉／交互外壳和现有 contextual-interact 入口；若不存在能够安全复用的外壳，只可为这一静态 spawn 增加最小 production adapter。该 adapter 只维护短生命周期内的 focus、距离、timer 与 Host 生命周期，绝不持久化 Actor pointer，也绝不成为 BodyTargetId 或物品真值。
2. 运行时边缘只能从已死亡的绑定源映射到 P11 已确定的静态 BodyTargetId 与 DefinitionId。Actor pointer、ObjectName、display name、Controller、UI address 或随机 GUID 可以仅用于瞬时 Code A 交互路由，不能写入 P11／P12 durable record、receipt、ContainerId 或 ItemId。
3. 交互开始前，adapter 必须先要求精确 OwnerId + committed RunInstanceId + BodyTargetId + DefinitionId 的既有 BodyContainerProjection。它不得调用 P11 物质化、补造 death receipt、创建 P6、创建 P11 record、重绑 recovery 或生成空白尸体。若 projection 不存在或不匹配，则安静拒绝／不打开页面。
4. Code A 保持尸体 Actor 的存在、显示、距离、目标有效性、输入原始派发、死亡视觉和销毁权威。尸体被销毁、世界卸载、离开交互距离、Run terminal 或 Host 关闭时，P12 只能取消当前 Code B action 并清理 transient UI；不得阻止销毁、重建尸体、恢复敌人、重放死亡、延迟终局或改变 Code A 奖励。
5. 对该绑定源，不得把旧 Code A corpse／reward／loot inventory 当作 P12 展示或转移来源。若它确实会对相同物理物品产生第二份可变真值，只可局部断开这一单一 source 的重复写入出口；若不存在不影响更广泛 Code A 行为的安全窄切口，停止受影响部分并报告 NEEDS_PLANNER_DECISION。独立的 Code A 战斗奖励不得因 P12 被全局移除或重新裁决。
6. 不新增与左键、Q、E、F、I、Esc 或既有战斗动作冲突的输入。若工程没有可安全复用的 contextual-interact 边缘，才可增加一个最小、命名清晰、仅转发已验证尸体身份的生产交互绑定；该输入不能直接写 Repository、P5、P6 或 P11。

### 5.2 尸体开启、取消与可恢复状态

1. 在不修改 P11 的死亡 receipt、固定 recipe、首次物质化或 BodyMaterialized 含义的前提下，为 P11 BodyContainer section 增加最小、持久化的尸体 action state 与当前 action identity。合法路径为：

       Closed → Opening → Open
       Opening → Interrupted
       Interrupted → Opening
       Open → Open（重开只恢复既有页面，绝不重新物质化、重抽或改写既有物品）

2. 只有 Code B 可以提交上述状态；Code A／UI timer 只提出 request 与通知 completion。Opening 使用命名的 production 配置，而不是 Widget magic number 或测试延时。可复用 P10 的通用搜索节奏配置；若不能直接复用，则只增加一个 BodyContainer 明确命名的最小配置，并在 Report 中记录名称和默认值。
3. 关闭页面、主动取消、离开 Code A 交互距离、尸体 Actor 销毁、world teardown、当前 Run 失效或 Code A 已到 terminal 时，若当前为 Opening，则仅将精确匹配 BodyContainer 标记为 Interrupted，并清理 timer、DragOperation、preview、mounted projection 和焦点；不得移动、揭示、重抽、补造或返回任何物品。
4. process recovery 或 UI 重建遇到没有有效 runtime action 的遗留 Opening，必须安全归为 Interrupted。Open 状态重开时仅读取既有 P11 graph 与可见状态；不得更改 DeathReceiptId、definition digest、ContainerId、已有 ItemId、已揭示状态或已转移物品。

### 5.3 单件搜尸揭示

1. P12 以 P11 BodyContainer item visibility 为唯一来源。尸体栏的 Hidden 与 Searching 格只能显示非泄露的未知表现，不得暴露 Definition、名称、数量、图标、详情、ItemId、ContainerId 或可拖拽 payload；这些格不可移动、合并、交换、装备或作为任何事务来源。
2. 增加一个显式的单件搜尸动作。它只能对当前 Open、精确匹配、同一 committed session 内的一个 Hidden body item 发起；合法状态为：

       Hidden → Searching → Revealed
       Searching → Hidden（取消、距离失效、尸体失效、Run terminal 或页面关闭）

   Revealed 在同一 Run 内不得退回 Hidden，不得重新抽取或改成另一物品。
3. 搜索开始与完成由 Code B 验证并持久化；Code A／UI 只承载计时与显示。使用命名的 production search duration 配置，可复用 P10 的通用搜索节奏；不得以 fixture-only、隐藏加速路径或 UI magic number 实现。丢失临时 timer 不能直接把 Searching 揭示为 Revealed。
4. 同一 BodyContainer 同一时刻至多一个 Searching item。重复触发、非 Open、非 Hidden、不同 target、session mismatch、terminal、stale revision 或 body record 不存在均无写入。搜尸控制是非位置写入的明确 UI 操作；它不是 Take、QuickMove、自动揭示所有物品或详情写入旁路。

### 5.4 双栏生产 UI 与尸体跨图拖拽

1. 复用 P7 已挂载的 production Host、P3/P4 Cell、详情展示、DragOperation、单一 pointer-owner／Hit Test 规则和 P10 的双栏呈现经验；不得创建 fixture page、平行 Host、开发命令或仅测试可达的 UI。为尸体目标新增的 transient presenter reference 必须只表达当前精确 BodyContainerProjection，不能变成 P5、P6、P9 或 P11 的第二份 durable layout truth。
2. 页面仅在 BodyContainer 为 Open、Code A interaction edge 有效且 exact P6/P11 session gate 成立时显示玩家完整 P7 布局与尸体合法格位。Closed、Opening、Interrupted、Hidden／Searching slot、缺少 body record、session 不匹配、Actor 失效或 Run terminal 时不得构造旧库存镜像、假尸体或可编辑 payload；应保持不可编辑或关闭。
3. 尸体栏与玩家栏之间的唯一位置写入路径为：

       已挂载 production Cell 的 NativeOnDrop
       → P12 body-aware interaction／controller
       → 既有 P4/P3/P2 可复用语义
       → P1 完整布局与唯一性校验
       → 单次 P6 玩家图 + P11 尸体图 durable commit
       → 刷新两栏权威 projection

   Widget、Presenter、Actor、Input adapter、菜单或详情控件不得直接写 Repository、P5、P6 或 P11。
4. 支持玩家 ↔ 已 Open 尸体的显式拖拽 Move、兼容 Merge 与双方合法时的 Swap。尸体来源仅限 Revealed item；玩家可放入尸体的目标必须是已 Open 的合法空格或已揭示且兼容的 item，绝不能覆盖 Hidden／Searching 项。所有动作先完整验证 source、ItemId、状态、数量、位置、目标容量、标签／装备限制、ChildContainer graph、session、BodyTargetId、DefinitionId 和两个 revision，再一次性提交。
5. 接受的跨图操作必须保持 ItemId、Definition、数量、父位置、ContainerId 与 ChildContainerId；不得复制、重新编号、半提交或先移出来源后验证目标。P6 与 P11 必须在同一 Owner record 中通过 temp-verify-backup-replace 或等价的既有 durable atomic transaction 共同更新；任一方保存失败、revision 冲突或图校验失败时，两边都保持旧状态。
6. 继续执行 Loaded Spatial 原子拒绝、装备栏规则、容量规则、同源拒绝和 P1 不变量。不得借尸体 UI 绕过 P7 装备／卸下语义，也不得把尸体当装备栏、快捷栏、世界掉落区、自动回收区或 Code A loot inventory。
7. 双击、右键、点击详情、搜尸以外的按钮、I、Esc、Close、Cancel、空格、UI 外区域和无 payload 的 Drop 均不得形成物品位置写入。只有本节明确的真实 Drop 才能提交 P6/P11 跨图转移。
8. 关闭尸体页面只清理 transient preview、DragOperation、timer、焦点和 mounted projection；它不得将尸体剩余内容放入 P6 或 P5。P8 的既有精确 session close 仍只处理 P6 结算并丢弃 P11 尸体残余，不得因 P12 改写其撤离、死亡、RecoveredAbandon、idempotency、lock、顺序或 Code A terminal authority。

## 6. 允许的改动范围

允许：

- 在 P11 BodyContainer section 内增加最小 action／searching state、只读 projection、body-aware reveal 与 P6/P11 原子 transfer service；
- 为唯一绑定 Low Skirmisher 尸体接入最小 Code A corpse interaction／range／timer／Host lifecycle adapter，必要时仅为该 source 局部切断同一物品真值的重复旧写入出口；
- 调整 P7/P3/P4 的可复用 production Host、Cell、Presenter 与 DragOperation，使其承载一个精确匹配的 BodyContainer target，同时保持玩家内背包的 Drop-only 规则；
- 复用但不改写 P1 图校验、P6 session gate、P8 terminal finalizer、P9/P10 NormalContainer 语义和 Code A 死亡权威；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档和本任务 Report。

## 7. 明确不在本任务内

不得实现、启动、重构或接管：

- 第二个尸体、全敌人尸体迁移、第二种 BodyContainer definition、P9/P10 普通容器迁移、敌人随机 loot table、概率池、灵石领取、世界掉落／拾取、地面物品、掉落动画或资源奖励；
- Code A 的敌人生成、伤害、战斗、AI、死亡判定、死亡视觉、击杀奖励、地图、任务、HUD、Run、终局分类、Run Save、旧库存、正式结算或返回值权威；
- P5 的局外页面或迁移、P6 bridge／Prepared receipt／RecoveredAbandon rebind、P7 玩家背包核心语义、P8 核心 terminal 语义、Profile lock 或复原语义；
- 快捷栏 1—9、物品使用、消耗品、装备效果、自动整理、自动拾取、QuickMove、双击、右键、详情写入、分堆 UI、自动回收或任何 A/B 双写；
- 实际运行、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 8. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查唯一绑定尸体的静态来源、BodyTargetId、DefinitionId 与 DeathReceiptId，确认 P12 只读取既有 P11 body record，所有 durable identity 都不依赖 Actor pointer、随机 GUID、显示名、UI 或临时 runtime 状态。
2. 审查 Code A interaction adapter，确认它只在 Code A 已提交死亡、自己决定尸体可交互的范围内转发距离／输入／生命周期边缘；body projection 缺失、Code B 拒绝、保存失败、UI 失败或尸体销毁均不改变 Code A 死亡、AI、奖励、地图、Run 或终局。
3. 审查 P11 扩展，确认 P11 首次物质化、固定 recipe、death receipt、ContainerId、ItemId、ChildContainer graph 与 schema recovery 语义没有被 P12 重新抽取、替换或隐式迁移；取消／恢复只能改变合法 action／visibility state。
4. 审查尸体 UI 与 reveal，确认 Hidden／Searching 绝不泄露物品信息或可拖拽 payload，且每个 BodyContainer 同时至多一个 Searching item；Open/reopen 不产生新 body graph。
5. 审查 P6/P11 transfer，确认 Move、Merge、Swap 均先完整验证、后以一项 durable transaction 提交，保留一物一实例／一父位置以及 ItemId／ContainerId／ChildContainerId；失败、保存中断或版本冲突不能半提交或复制。
6. 审查 P4x/P7 输入与 P8 终局边界，确认本轮没有 QuickMove、非 Drop 位置写入口、尸体内容回流 P5、P8 语义改写、P6 Prepared/rebind 改动或 P9/P10 语义漂移。
7. 审查全部 Code A 改动，确认它们仅限唯一尸体的 interaction／distance／Actor／timer／Host lifecycle 适配，以及必要的单源重复真值断开；没有转移地图、Player、Loot、战斗、死亡、Run、Run Save 或结算权威。
8. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

9. 若编译失败，只修正 P12 引入的局部声明、include、类型、BodyContainer schema、持久化结构、action／reveal state、UI lifecycle、transfer transaction 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要越过本 Prompt 边界，停止受影响工作并报告；不得自行扩展到 P13、0.0.9B.F 或其他任务。

## 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P12.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. P12 新增／修改／未修改文件及职责；
2. 唯一尸体生产来源、BodyTargetId、DefinitionId、DeathReceiptId 的既有组成，Code A interaction adapter 与其不改变 Code A 权威的理由；
3. P11 body action／reveal 的状态、duration 配置、取消／中断／恢复政策，以及首次物质化、固定 recipe、receipt、ItemId／ContainerId／ChildContainerId 不变的结论；
4. 双栏 production UI、Hidden／Searching 非泄露规则、唯一真实 Drop 写入链，以及 P6/P11 原子 transfer 的身份与回滚保护；
5. P8 如何只结算 P6 并丢弃 P11 仍存残余，特别说明玩家放入尸体的物品不会回流 P5／P6；
6. 对 P4x、P5、P6 Prepared rebind、P7 Drop-only、P8 finalizer、P9/P10 NormalContainer、P11 death receipt 与 Code A 权威的静态审查结论；
7. 实际 Editor 编译命令、目标、最终 native exit code 和关键结果；
8. 明确列出未执行的 F 阶段项目：真实产品死亡／重复死亡／尸体销毁／重开／终局验证、CTA、wrapper、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验证仍由 0.0.9B.F 负责；
9. 明确列出未启动的后续功能：第二个尸体、全敌人迁移、世界掉落／拾取、随机 loot、其他地图容器或其他 P 阶段。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code 0 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P12_COMPILE_REWORK

若不存在安全的生产尸体交互挂点、无法保持 P11 稳定 identity、无法建立单次 P6/P11 原子提交，或无法仅切断同一 source 的重复真值而需要新的产品裁决时，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P13、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P12.0.r0","file":"Dev.D.UE.0.0.9B.P12.0.r0_report.md"}
