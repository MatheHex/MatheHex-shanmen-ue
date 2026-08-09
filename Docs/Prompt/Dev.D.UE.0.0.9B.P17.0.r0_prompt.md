# Dev.D.UE.0.0.9B.P17.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P17——真实空间道具的嵌套容器真值与局内背包接入
- 任务编号：Dev.D.UE.0.0.9B.P17.0.r0
- 任务性质：在 P5/P6/P8 的真实 Profile／活动 Run 生命周期、P7 的局内个人背包、P13 快捷栏引用、P14 单物品地面路径、P15 RestoreHealth 回执链与 P16 确定性战利品 Profile 已建立后，将既有 Code B 空间戒指（快捷空间道具）和空间储物囊（非快捷空间道具）从通用 P1 原语接入真实玩家物品图。P17 只完成空间道具自身的嵌套容器、真实 P5↔P6↔P8 生命周期及 P7 的薄投影入口；不创造物品来源、不扩展地图拾取、不实现武器／装备效果，也不启动 F 阶段验证。
- 执行文件：Dev.D.UE.0.0.9B.P17.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P17.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 引擎：C:\Program Files\Epic Games\UE_5.8
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r1、P10.0.r0、P11.0.r0、P12.0.r0、P13.0.r0、P14.0.r0、P15.0.r0 与 P16.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P17 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 持续有效的真值与边界

1. Code A 继续拥有默认地图、Actor、交互距离、输入、Player Actor、战斗、生命、HUD、正式 Run 生命周期、终局分类、Run Save、旧库存、旧 Loot／搜索及所有尚未迁移的运行时。Code A 不得保存、镜像、计算、展示为权威或修改 Code B 空间道具、其容量、ChildContainer graph 或其中物品。
2. Code B P1 Repository 是新物品图唯一可变真值。P5 是局外 Profile snapshot；P6 是精确 OwnerId + RunInstanceId 的活动 Run 玩家携带 snapshot；P9/P11 是独立 Run-local 残余内容。P8 仅在 Code A 已提交终局后结算 P6 玩家图并丢弃未取得的 P9/P11 残余。不存在 A／B 镜像、同步或双写。
3. P7 只按精确活动 P6 会话展示局内个人背包；P10/P12 各自只处理已经物质化的普通／身体容器；P13 的 1—9 仅保存稳定 ItemId 引用；P14 只处理已经离开玩家图的单一地面路径；P15 只可使用已绑定且仍在 P6 BaseQuick 的 RestoreHealth；P16 只决定 P9/P11 首次物质化的内容来源。
4. P1 已有唯一 parent-container、slot placement、ChildContainer graph 与原子 Move／Swap／Merge／Split／Equip／Unequip 原语。P17 必须复用这些原语和 P5/P6/P8 durable transaction；不得平行建立第二份“空间仓库”、Widget inventory、临时数组、Code A inventory 或独立空间道具 save。
5. P4x 已确定两类产品语义：空间戒指是快捷空间道具，空间储物囊是非快捷空间道具。P17 不得凭显示名、Actor、Widget 文本或临时 fixture 猜测它们；必须由当前正式 Code B Definition Catalog 的稳定 DefinitionId、既有 item semantic／slot compatibility 和 P1 schema 确认实际映射。
6. P16 已使 BasicCache／BasicCorpse 的未物质化内容使用一次性确定性加权 Profile。P17 不得把空间道具加入 P16 Profile、P5 starter grant、初始库存、地图 Actor、旧 Code A Loot 或任意演示／测试来源；本轮仅让已经合法进入 P5/P6 的空间道具能够保有真实子容器。

## 3. P17 产品裁决

P17 的单一纵向切片是：一件已在真实 P5 或活动 P6 中存在的、正式 Code B 空间道具，拥有唯一、持久、可验证的 ChildContainer graph；P7 可以以该真实图的只读投影进入其内容，而全部物品移动仍走既有 P6 事务。

| 道具语义 | P17 所作的最小接入 | 明确不作 |
| --- | --- | --- |
| 快捷空间戒指 | 仅在其处于当前已定义的快捷空间兼容槽时，可由 P7 打开其唯一 ChildContainer。离开该兼容槽时只失去快捷入口，不改写其图。 | 不绑定 P13 1—9、不触发 P15、不授予技能、不改变战斗或 HUD。 |
| 非快捷空间储物囊 | 作为真实 P6 携带物的可选择嵌套容器，由 P7 从该物品进入。 | 不新建第二背包系统、不创建地图 pickup、不作为快捷／消耗品／装备效果。 |

P17 只支持一层空间道具子容器：空间戒指／储物囊本体、其 ChildContainer 或其任一后代不得容纳另一空间道具，也不得容纳自身或任一祖先。未来若要允许嵌套袋、整包地面丢弃或空间装备效果，必须另行拆分任务。

若当前正式 Definition Catalog 找不到可由稳定 ID 和既有语义明确识别的一枚快捷空间戒指和一枚非快捷空间储物囊，或既有兼容槽无法表达二者的产品语义，停止受影响部分并使用 NEEDS_PLANNER_DECISION；不得新建 starter、临时 Definition、别名、fixture、假 ItemId 或以 UI 文本硬编码替代。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

在 Code B 内为两类既有空间道具建立真实、幂等、不可循环的 ChildContainer 生命周期，并将其接入 P5/P6/P8 与 P7。空间道具本体、内部 ContainerId、内部 ItemId、slot placement 和内容必须始终属于同一份 P1 graph；P7 只投影并请求既有事务，不拥有第二份库存。

### 4.1 正式定义映射与图不变量

1. 先审阅当前正式 Code B Definition Catalog、P1 item schema、既有 slot compatibility 和 P4x 已存在的空间道具语义。确定并在 Report 记录：
   - 快捷空间戒指的稳定 DefinitionId、其允许的快捷空间兼容槽、内部容量／布局规则；
   - 非快捷空间储物囊的稳定 DefinitionId、其允许携带位置、内部容量／布局规则；
   - 两者被认定为空间道具的稳定数据语义，而非显示名、Widget 文本、Actor 标签或临时测试开关。
2. 对每件合法空间道具，本体 ItemId 与唯一 ChildContainerId 必须具有稳定 parent relation。ChildContainer 的容量、布局、slot compatibility 与可放入定义必须完全由该正式定义和既有 P1 规则导出；不得从 UI、地图、时钟、随机数、Actor 状态或 P16 roll 临时生成。
3. 首次创建、迁移或补足缺失 ChildContainer 时，必须在同一 Owner durable replacement 内完成完整 P1 图、parent relation、容量／layout provenance 与 P5 或 P6 现有 snapshot。重复读取、重复 P5→P6 bridge、P6 recovery／rebind、重复 P7 打开或保存冲突只能读取既有图或零写入拒绝，绝不得创建第二个 ChildContainer、重排 ItemId 或抹掉内部内容。
4. 在写入前验证完整无环图：唯一 parent、无自引用、无祖先／后代回边、无重复 child owner、无超过一层的空间道具嵌套、无越界 slot、无超过容量、无未知／非法 DefinitionId。若任一验证不能通过，拒绝整次候选写入，保持 P5/P6/P8/P13/P14/P15/P16 不变。
5. 已经具有真实 ChildContainer graph 的合法历史 P5/P6 record 必须逐字节保留 item、container、内容和 placement 身份；仅可增加兼容读取或缺失 provenance 的只读解释。不得为“统一格式”重建、清空、复制、flatten 或替换其内容。

### 4.2 P5、P6 与 P8 的真实生命周期

1. P5 局外 Profile 中的空间道具及其完整 ChildContainer graph 是局外真值的一部分。P6 Start Run／bridge 仅以既有精确 OwnerId + RunInstanceId 事务将整份合法玩家图带入活动会话；不得分离空间道具本体与内部内容，不得引入第二次 clone 或新的 P5 starter。
2. P6 内的空间道具及其内容必须和其他 P6 物品走同一份 owner durable document、revision／冲突控制与既有 Move/Swap/Merge/Split/Equip/Unequip 事务。P17 不新建“直接写入空间格”的绕行入口。
3. P8 保持 Code A 后置终局观察与既有 terminal authority。Extracted 只按当前 P8 规则把仍在 P6 的完整玩家 graph（包括合法空间道具图）返回 P5；Dead 与 RecoveredAbandon 继续按既有 P8 规则没收活动 P6 玩家图。P17 不改变终局分类、P8 时序、P9/P11 残余丢弃、P5/P6 bridge 或任一 Code A Run authority。
4. P13 引用与 P15 receipt 必须继续只引用 P6 内仍合法、仍可用的 ItemId。空间道具本体和其后代离开 P6、被终局没收、失效或不可见时，复用既有 P13／P15 清理规则；不得让 1—9、receipt 或 Widget 持有悬空对象、复制数量或绕过 P6。
5. P14 不在本任务中扩展。先静态审阅其现有图转移能力：若它已经能完整保留合法空间道具 graph，可保持现有行为；若不能，P17 必须以现有 P6 eligibility gate 拒绝该类空间道具的地面移动，而不得 flatten、只丢 parent、复制 child、另建地面仓库或篡改 P14 单物品真值。具体“整包地面丢弃／拾回”留给后续单任务。

### 4.3 P7 局内背包薄投影与访问语义

1. 仅在 P7 已确认的精确活动 P6 OwnerId + RunInstanceId 会话中，增加空间道具 ChildContainer 的选择／进入／返回投影。Widget 可保留稳定 ContainerId、ParentItemId 和必要的 display metadata，但 Definition、数量、slot、容量和 mutation 的权威读取全部来自 P6；不得缓存或保存独立 UI inventory。
2. 快捷空间戒指只有在当前既有快捷空间兼容槽内时，才可出现其快速进入入口。该物品被移动、交换、卸下、终局关闭或 P6 恢复失败时，P7 必须关闭／隐藏对应投影并重新读取 P6；不得删除、转移或重新生成其中的内容。
3. 非快捷空间储物囊可由其真实 P6 物品条目进入，但不得因此被纳入 P13 1—9、P15 RestoreHealth 或新建按键动作。P7 不得以“方便”为由把它伪装成 BaseQuick、消耗品、装备效果或新的快捷栏。
4. 所有从空间 ChildContainer 到 P6 其他位置、或反向的物品移动，都必须复用已有 P6 原子 Move/Swap/Merge/Split／slot validation；不新增一次性“收纳全部”、自动整理、自动装备、自动拾取、批量转移或直接 Widget 写入。若目的地是空间 ChildContainer，先验证一层限制与无环规则。
5. P17 不修改 P10/P12 的容器 UI、读条、揭示、拖拽状态机、地图 Actor 或 target identity。它们可继续依既有 P6 事务处理普通物品；空间道具能否在未来作为 Loot／尸体转移对象不在本任务中扩大，且不得为此改变 P16 Profile。

## 5. 允许的改动范围

允许：

- 在 Code B 内最小扩展正式空间道具 definition semantic、P1 child-container validation、P5/P6/P8 graph serialization／migration 与同一 Owner durable transaction；
- 仅在 P7 内加入真实 P6 空间容器的选择／投影／关闭逻辑和对既有 P6 事务的调用；
- 为确保合法 graph 迁移和编译兼容而最小调整 P13/P14/P15 的 Code B eligibility／无效引用清理声明；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档和本任务 Report。

## 6. 明确不在本任务内

不得实现、启动、重构或接管：

- 任意 P5 starter、初始库存、空间道具新来源、P16 Profile 入口、随机 Loot、第二普通容器、第二尸体、地图 Actor、世界掉落、自动拾取、整包地面丢弃、地图拾回或 Code A 旧 Loot；
- P10/P12 的 UI、交互、读条、拖拽、Move/Merge/Swap、搜索状态机、可见表现或目标 identity；
- P13 新快捷绑定语义、P14 新地面来源、P15 之外的使用效果、武器使用、道袍／饰品属性、空间装备效果、技能、Buff／Debuff、动画、音效、战斗属性、网络同步或多人；
- Code A 的地图、Actor、输入、HUD、Player Actor、生命、战斗、死亡、Run、Run Save、终局分类、旧库存或正式结算权威；
- 产品启动、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查两类空间道具均由当前正式 Definition Catalog 的稳定 ID 与既有 slot semantic 确定；确认没有新 starter、fixture、假 ItemId、P16 loot entry、地图来源或 Code A Loot。
2. 审查 P1 graph：唯一 parent、稳定 ChildContainerId、一层空间容器限制、无自引用／环／重复 child owner、容量／layout／Definition 合法性，以及所有失败路径零写入。
3. 审查 P5↔P6 bridge、P6 recovery／rebind 与 P8 terminal settlement，确认完整 graph 不被拆分、clone、flatten 或双写，且 Code A 仍没有 inventory／space-container authority。
4. 审查 P7：只投影精确活动 P6；快捷空间戒指的入口仅由既有兼容槽决定；非快捷储物囊不获得 1—9、P15 或新输入语义；任何 mutation 都走已有 P6 事务。
5. 审查 P13/P14/P15/P16 影响：悬空引用只由既有 Code B 规则清理，P14 对不兼容 graph 零写入拒绝，P16 Profile／P9/P11/P10/P12 不被改写。
6. 审查 Code A diff。除纯声明／编译兼容外，预期没有 Code A 改动；若有，必须逐文件说明其不涉及地图、Actor、Loot、搜索、输入、HUD、战斗、生命、Run、Run Save、结算或库存权威。
7. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

8. 若编译失败，只修正 P17 引入的局部声明、include、schema、序列化、ChildContainer graph validation、P7 projection 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要扩展到 P18、0.0.9B.F、Code A 权威或其他功能，停止受影响部分并报告。

## 8. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P17.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增／修改／未修改的每个文件及职责；
2. 两个实际空间道具的稳定 DefinitionId、既有兼容槽、容量／layout 语义，以及没有新增 starter、fixture、P16 Profile 条目、地图来源或 Code A Loot 的结论；
3. ChildContainer graph 的 parent／identity、同一 Owner durable save、无环／一层限制、重复／冲突／非法输入零写入和历史图兼容结论；
4. P5→P6→P8 与 recovery／rebind 的完整 graph 生命周期，以及不改变 Code A terminal authority 的结论；
5. P7 的真实 P6 投影、快捷空间戒指进入条件、非快捷储物囊进入条件、关闭／失效行为和全部 mutation 仍走 P6 的结论；
6. P13/P14/P15/P16、P9/P11/P10/P12 与 Code A 权威的静态边界结论；
7. 实际 Editor 编译命令、目标、最终 native exit code 和关键结果；
8. 明确列出未执行的 F 阶段项目：真实 P5→P6→P8 生命周期、两类空间道具 P7 打开／返回、ChildContainer 移动、rebind、P14 兼容性、P13/P15 清理、终局、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证均由 0.0.9B.F 负责；
9. 明确列出尚未启动的功能：整包地面丢弃／拾回、空间道具来源、嵌套袋、空间装备效果、武器／道袍／饰品效果、其他消耗品及后续 P 阶段。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code 0 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P17_COMPILE_REWORK

若缺失正式空间道具定义／兼容槽、既有 graph 无法在不改写真实历史的前提下合法迁移，或接入必须转移 Code A 权威，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P18、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P17.0.r0","file":"Dev.D.UE.0.0.9B.P17.0.r0_report.md"}
