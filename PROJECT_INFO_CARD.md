# Shanmen Unreal 0.0.9B 项目信息卡

## 项目身份

- 项目名称：Shanmen Unreal
- 项目编号：`Dev.D.UE.0.0.9B`
- 当前阶段：`0.0.9B.P23`
- 活动开发根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 隔离根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B_QUARANTINE_I0R0_FROM_0.0.9-XFix1_20260804T194204`
- Unreal 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：`C:\Program Files\Epic Games\UE_5.8`

## 当前任务

- 当前任务：`Dev.D.UE.0.0.9B.P23.0.r0`
- 当前状态：`READY_FOR_P24_PLANNING`
- 下一任务：上传并发送 P23 Report 后，只取得并归档策划部下一份 Prompt；本轮不自行执行 P24。
- Prompt 目录：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Prompt`
- 当前 Prompt：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Prompt\Dev.D.UE.0.0.9B.P23.0.r0_prompt.md`
- Task Report 目录：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report`
- Version Report：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Dev.D.UE-0.0.9B.codex.report.md`
- 当前 Report：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report\Dev.D.UE.0.0.9B.P23.0.r0_report.md`

## 基础调用与阶段门禁

- Git 基线：项目根本地仓库；生成目录和候选包已排除。
- 唯一调用入口：`Scripts\Invoke-Shanmen.ps1`。
- 无启动诊断：`Scripts\Invoke-Shanmen.ps1 -Action ResolveLatest`。
- UE 定位：显式 `-EngineRoot` → `SHANMEN_UE_ROOT` → Epic 注册表 → `.uproject` EngineAssociation 默认安装目录。
- Latest Demo：根目录包装器 → 项目 `LATEST_DEMO.bat` → packaged `Latest_Demo`；无候选包时只允许明确的 Editor `-game` 降级。
- 基础调用审计：`Scripts\Invoke-FoundationAudit.ps1` 保留为维护工具；主线不再设置 I/IPF 前置门禁，也不得借审计扩展当前 Prompt 的验证范围。
- 交接账本：`Scripts\Update-HandoffLedger.ps1`；真实 Report 附件在策划 Chat 可见前不得进入 `MARKER_SENT`。
- 阶段分工：P 只做开发／静态审查／Prompt 明确要求的编译；F 承担产品运行、内部回归、真实 Windows 输入、截图、Smoke、Cook/Package 与最终发布验证。
- P23 局外统一工作台：正式宗门 Warehouse／Loadout 已复用 P4 的同一 Cell、stable address、Drag payload、modifier router、dynamic grid 与双栏 scroll。OutOfRaidP5 Context 只保存 Owner／revision／Coordinator gate／pane 与 transient identity；P5/P6 事务隔离，旧仓库拖拽入口零写入。P5 `Ctrl+左键`、`Shift+1—9`、动态空间容量与稳定 SlotIndex 均进入共享 policy；Fix2 已取消。真实验证留给 `0.0.9B.F`。

## 来源与隔离

- 工程级开发基线：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`（用户 2026-08-04 最新裁决）
- 0.0.8 源搜索：已由用户取消；不再继续追踪。
- 旧代码 A：XFix1 中现存的物品、容器、装备、仓库、搜索、世界物品、结算与 UI 实现，保留以维持当前游戏运行。
- 新代码 B：后续 0.0.9B 背包相关能力的最终权威；P2 已在 `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Source\demo_map\CodeB` 增加确定性玩家配置 fixture、只读 Projection 与单一 Application Service/Command Adapter，P1 Repository 仍是唯一可变权威。
- Code B 事务：已覆盖 Move、Swap、Merge、Split、Equip、Unequip；成功单次提交并递增 revision，失败保持原 snapshot/revision。
- P2 fixture：仓库 `30` 格、基础储存 `6` 格、武器槽、护甲槽、空间装备槽、2 个饰品槽、空间道具内部 `4` 格；含稳定 DefinitionId/ItemId/ContainerId 及确定性初始物品。
- P2 Projection：按稳定布局角色与槽位顺序只读导出 Revision、布局、容器/槽位 ID、ItemId、DefinitionId、数量、等级、品质、类型；不暴露可变 Repository 引用，也不建立第二份物品真相。
- P2 Application Service：统一承接 stash↔basic、equip/unequip/replacement、Split/Merge、stash/basic↔spatial internal，以及 occupied/full/type/source/stale 失败路径；全部翻译为 P1 事务。
- Code B 测试：23 个自动化测试通过（P1 `4` + P2 `19`），固定 Seed `20260805` 完成 `1000` 次 P1 随机事务尝试，成功 `181`、合法拒绝 `819`；P2 固定命令序列重放 `100` 轮，成功 `2000`、失败 `300`。
- P3 UI Host：真实 C++ UMG/Slate 页面，默认关闭；只在 `CodeB.P3.Open` 显式打开时创建单一 fixture/service/root，可用 `CodeB.P3.Close`、`CodeB.P3.Reset` 关闭或重置。关闭/重开保留 fixture 和 revision，退出清理。
- P3 展示与操作：兵器、道袍、双饰品、空间道具、基础 6 格、局外仓库 30 格、空间储物门控、物品详情、九个“未接入”快捷栏；支持选择、移动、装备、显式卸下、替换、拆分、合并、空间内部移动和 stale 刷新反馈。展示层只持有稳定地址与 ItemId；全部写回继续经 P2 → P1。
- P3 测试：全量 Code B 自动化 `47/47` 通过（P1 `4` + P2 `19` + P3 `24`）；P3 覆盖默认关闭、单 fixture、稳定地址、详情、操作、失败反馈、stale、关闭重开、重复打开、Escape、退出清理。可见 Smoke 已核验 1920×1080 和 1280×720。
- P4 实现：`FCodeBP4InteractionController` 只读取 P2 Projection，构造只含稳定地址、ItemId、Expected Revision、会话标识与展示快照的 payload；真实 UMG `UDragDropOperation` 与格位 Enter/Leave/Drop 路由到 P3 Controller，再由 P2 → P1 原子裁决。支持 Move、完整堆叠 Merge、储物 Swap、Equip、Replacement、Unequip、空间内部储物和明确拒绝。
- P4 QuickMove（历史 P4 行为）：该用户入口在 P4x r2 已废止。双击、右键、详情、Escape、关闭/重开均只读；真实 Drop 是唯一位置写入入口。
- P4.0.r0 状态：Controller 自动化 `57/57` 通过，但实际 UMG 格位点击/双击输入回归，最终为 `P4_NEEDS_REWORK`。
- P4.0.r1 完成：格位本体成为唯一 Hit Test / pointer owner，视觉按钮保持非交互；左键 DetectDrag 移至同一对象的常规按下回调，双击与第二次 MouseUp 通过页面级手势识别去重。`UCodeBP4DragOperation` 自身接收 Slate 取消并清理 Preview。`CodeB.P4.RunRealInputTrace [width] [height] [Quit]` 在实际 viewport 挂载页面上，按生产 Cell/Control 屏幕坐标送入 `FSlateApplication` hit-test；它不是 Widget/Controller/P2 直调，并记录 `P4.UITrace`（手势、稳定格位、DragOperation、Enter/Leave、Preview、提交数、Revision）。1280×720、1920×1080 下均完成真实选择、Move、Merge、Swap、Equip、Replacement、Unequip、空间内部移动、拒绝、双击、右键菜单、Escape、Close/Reopen；每条写入手势断言仅一次事务。
- P4 期间用户授权的生产修复更新了非 CodeB 源：死亡结算现在清理失效空间道具布局，物品格优先显示真实 DisplayName；该修复由 ProfileSettlement `19/19` 与 ProfileSession.13 验证，不构成 P4 UI 验收。
- P4x r0 是未接受、只读的候选证据；P4x r2 以当前构建完成正式验证。产品传送页 `SectTeleportCTA` 是无备战实际 Start Run 入口：不创建 Preparation Widget、不以旧布局为门槛、不由 Code B 初始化正式 Run。r2 覆盖 Normal、NoEquipment、MissingPreparation、StaleLegacy 及返回后二次 Start Run；首次启动不改旧布局，StaleLegacy 在结算后可由既有规范化清理。
- P4x Code B 语义：普通仓库、基础 6 格、空间戒指 `QuickSpatial` 内部格、储物囊 `PouchInternal` 内部格均不按物品类型互斥；仅装备栏位裁决类别与单槽 Replacement。`Spatial.Ring` 只能进入空间戒指栏，装备后显示快捷空间；`Spatial.Pouch` 只能作为普通空间物品，始终标为非快捷储物、无 1—9/QuickMove/使用语义。两者均有稳定 Child ContainerId；有内容时整体移动被原子拒绝，内部实例不复制。
- P4x r2 UI：真实 Drop 是唯一位置写入入口。Move/Merge/Swap/QuickMove/放回按钮、右键写入、双击写入和旧截图 Console 成功写入命令均已移除；详情只按当前位置条件显示“装备”或“卸下”提示，并要求拖到明确目标。静态审计确认 UI 中 `BeginOperation` 直调为 `0`，`CommitDrop` 仅存在于真实 `NativeOnDrop` 路由。`CodeB.P4x.RunRealInputTrace` 在 1280×720 与 1920×1080 均通过，并记录 P2 command/call 与 revision 边界。
- A/B 边界：Code A 仍是默认地图正式运行时路径；P3 不接入正式玩家、仓库、Loot、Profile、Run、SaveGame 或持久化，也未产生 A/B 双写。
- P5 局外状态：正常宗门“仓库／人物配置”入口懒加载与正式 Profile `OwnerId` 绑定的 Code B 版本化 sidecar；它保存 Code B snapshot、Revision、receipt、实例映射与 child-container 关系。首次接管仅在非活动 Run 发生；旧 Code A 清单只是只读来源和审计摘要，绝不作为第二个可编辑库存或双写目标。失效旧项目／空间引用记入 receipt；有效旧空间内容迁入所属 `ChildContainerId`。P5 保持 Run、地图、玩家、Loot、搜索、结算和 Run Save 的 Code A 权威。
- P5 验收安排：用户要求避免阶段性过度验证，因此 P5 的全量回归、真实输入矩阵、截图与 Smoke 整体移至 `0.0.9B.F`。P5 实施 Report 使用 `NEEDS_PLANNER_DECISION`，不表示已经通过完整验收。
- P6 Run session：`FCodeBRunInventorySession` 是 P5 Owner sidecar 内的独立 Code B Run snapshot／receipt，不是 Code A Run Save，也不接入 Player Actor、Loot、搜索、世界物品、Hotbar、消耗、结算或旧库存。只有 `StartPreparedProfileRun` 成功 `ActivatePreparedProfileWorld` 后，唯一 observer 才通知 P6；bridge 永远不能改变 Start Run 成败。P5 常规入口在 session 未关闭期间只读锁定为“当前 Run 中，返回后再整理”。P7 只读投影；P8 才定义回收与结算。
- P6 r3 结论与 F 债务：r3 实现以只读 Code A `RecoveredAbandon` context 允许同 Owner 的 verified Prepared receipt 绑定不同新 RunId，保留 receipt identity/origin、原 source revision、payload digest、完整 item/container/child-container graph 并追加连续 recovery history；普通冲突仍拒绝。新增 wrapper/result contract 代码，但按用户最新统一规则，P 阶段不运行真实 CTA、wrapper、截图、回归或 Game build；它们以及 F 的完整验收仍待执行。因此状态为 `NEEDS_PLANNER_DECISION`，不是通过。
- P6 r4 编译收尾：r3 的功能与静态审查已接受；仅执行 `demo_mapEditor Win64 Development` 代码编译，退出码 `0`、UBT `Succeeded`，未发现并未修正任何源码错误。P6 的功能交接状态为 `READY_FOR_P7_FUNCTIONAL_WITH_F_DEBT`；CTA、wrapper、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验收全部仍留给 `0.0.9B.F`。
- P7 活动 Run 个人背包：现有库存输入在 `RunActive` 时先取 Code A 当前 `ProfileId` 与已启动 `RunId`，只读打开 Owner／Run 精确匹配的已提交 P6 session；失败时不创建磁盘 sidecar、P5 迁移、fixture 或临时 Repository，并保持 Code A 非阻塞。匹配时才建立内存 P1 Repository，复用 P3/P4 的生产 cell 和真实 DragOperation Drop，隐藏局外仓库并显示玩家装备、基础 6 格、已装备空间戒指快捷内部格、储物囊非快捷内部格及禁用 1—9。P7 活动作用域拒绝点击／菜单指令，唯一写链是 `NativeOnDrop → P4 → P3 → P2 → P1 → P6 session`；每个已接受 Drop 仅持久化一次并推进 P6 session／记录 revision。首次 Run 内提交冻结原携行 item graph，以保留 P6 receipt digest 的不可变性；P5 snapshot、P6 bridge/rebind、Code A Run/Player/Loot/结算/旧库存均未改动。最终 Editor 目标编译 exit code `0`、`Succeeded`；真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验收全部仍由 `0.0.9B.F` 承担。
- P8 活动 Run 终局：同一个 P5 Owner record 现保存可审计 terminal receipt history；每个 receipt 冻结 P7 当前的 P6 snapshot/layout/digest。后置 observer 仅在 Code A 的 Runtime 与 Profile terminal 已成功提交后（或已经完成的 `RecoveredAbandon` 启动恢复后）传递精确 OwnerId、RunId 与分类，且其结果不可改变 Code A。`Extracted` 在一次原子记录替换中把 P6 当前完整根/ChildContainer 图按原 ItemId、数量、定义、父位置、槽位与 ContainerId 合回 P5，仓库未出战物品不变；`Dead`／匹配 committed 的 `RecoveredAbandon` 仅冻结并没收该图。成功时同一提交关闭 P6、释放 P5 lock；重复同分类幂等，冲突、失配、Prepared、未知均无写入。终局观察会关闭精确匹配的 P7 Host，清理 UI/Drag/Preview 而不经 UI 写 P1/P5/P6。Editor 编译 exit code `0`、UBT `Succeeded`；所有真实/自动化/可见验证和 Game Build、Cook、Package 仍留给 `0.0.9B.F`。
- P9 Run-local 普通容器：`RunLocalNormalContainers` 与 P5 局外仓库、P6/P7 玩家携带图分离，按精确 Owner／已 committed Active Run／SearchTarget／Definition 建立。`CodeB.NormalContainer.BasicCache` 只引用正式全局的 `SpiritDust` 与 `IronShard` 定义，首次原子 materialize 后保存根 Container、真实 Item graph、全部 Hidden reveal、receipt 与 digest；同 target 同定义只读返回，异定义冲突和所有不匹配/非 committed 情况无写入。P8 closing commit 显式清除这些 Run-local records，绝不合并至 P5。P9 未改 Code A 或 P7 UI/Input/Drop。P9.0.r1 静态复核确认 P9 identifier 仍仅存在于 `CodeB/demo_mapCodeBOutOfRaidProfile.{h,cpp}`；无功能源码修正。`demo_mapEditor Win64 Development` 的同步原生编译 exit code 为 `0`，UBT `Succeeded`、目标最新且执行 `0` actions。状态为 `READY_FOR_P10_INTERACTION_FUNCTIONAL_WITH_F_DEBT`；所有真实运行和 F 阶段项目未执行。
- P10 普通容器交互：唯一生产 target 是 `M01.CodeBNormalContainer.BasicCache.01`，由最小 map adapter 提供既有 Focus/Input/距离入口，静态 map identity 决定 target GUID；Code A 不保存容器、物品或 reveal。P9 schema 1→2 保存 Opening／Searching action identity，Owner schema 3→4，允许 exact interrupt/recovery 并避免 Open 重复写入／reveal reroll。P7 生产 Host 复用 P3/P4 双栏，P2 的 target 列只是 transient presentation；Hidden／Searching 不泄漏定义、数量、ItemId 或 details 且禁止拖拽。仅 Revealed item 可经 `NativeOnDrop → P4 → P3 → P2 → P1` 移动、合并或交换；最终服务在同一 Owner document 单次原子保存 P6 carry 和 P9 target，拒绝隐藏／搜索中离开。M01 投影失败不会回滚 Code A Run，target destroy 关闭页面；P8 仍只处理 P6 并清理 P9。2026-08-07 唯一一次 Editor 目标编译 `Succeeded`（28.28 秒）；没有运行任何产品、自动化、回归、截图、Smoke、Game Build、Cook 或 Package，F 债务保持到 `0.0.9B.F`。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。
- P11 身体容器：生产绑定唯一为 `M01.Encounter.LOW.Skirmisher.01` / `M01.Spawn.LOW.Skirmisher.01` 的 ordinal `0`。Code A 在该敌人已提交死亡状态后仅发送确定性 `OwnerId + committed RunInstanceId + BodyTargetId + DefinitionId + DeathReceiptId`；其成功、拒绝或保存失败只记录审计日志，不能改变 Code A 战斗、死亡、Actor、地图、Run、奖励或终局。Code B Owner sidecar schema 4→5 的 `RunLocalBodyContainers` 独立保存 `CodeB.BodyContainer.BasicCorpse` 固定 recipe、真实 P1 Container/Item/Child graph、全 Hidden visibility、immutable receipt/digest 及只读 projection；无 UI、revealing、drag、move 或 P6/P5 写入。相同 receipt 无写入返回，冲突/失配/Prepared/terminal/非法输入拒绝；P8 只在精确 terminal close 中丢弃 P11 残余，不将其返还、没收进 P5、或写入世界。2026-08-07 只执行一次 Editor 编译，native exit code `0`、UBT `Succeeded`；所有真实验证仍留在 `0.0.9B.F`。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。
- P12 单一生产尸体交互：只为 `M01.Encounter.LOW.Skirmisher.01` / `M01.Spawn.LOW.Skirmisher.01` ordinal `0` 的既有 P11 record 路由 `M01.BodyTarget.LOW.Skirmisher.01.Ordinal.0`。复用 Code A 的尸体 Actor/距离/输入外壳，但 adapter 仅传递静态 identity、timer 和 Host 生命周期，且严格先读 committed P6 + P11 projection；不得 materialize、重绑或建立空白 record，任何 Code B 拒绝均不改变 Code A。Owner schema 5→6 在 P11 body section 持久化 action/search identity；`BodyMaterialized → Opening → Open` 与 `Hidden → Searching → Revealed` 由 Code B 提交，cancel/distance/destroy/terminal/page close 只中断对应 action（search 复位 Hidden）。`FCodeBBodyContainerInteractionTiming` 的 `0.85s` 开启和 `0.70s` 单件搜查是命名 production 配置。P7 P3/P4 Host 临时添加 `BodyContainerTarget` 栏：未知/搜查中不带 ItemId/payload、无详情和无 drag；揭示物品与玩家栏之间只能经 `NativeOnDrop → P4 → P3 → P2 → P1` 进行 Move/Merge/Swap。P12 原子 service 完整校验 identity、revision、P1、visibility、parent/child graph，随后同一 Owner replace P6 与 P11；失败零半提交。回放尸体的玩家物品显式为 Revealed；P8 仍只结算 P6、丢弃 P11 残余。P4x/P5/P6 Prepared-rebind/P7 Drop-only/P8 finalizer/P9-P10/P11 receipt 与 Code A 权威不变。Editor 目标最终 native exit `0`、UBT `Succeeded`；F 阶段真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验收均未执行。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。
- P13 快捷栏引用：P5 Profile 与 P6 精确活动 Run 各持有版本化的 9 槽 `SlotIndex + ItemId / Empty` Code B binding；快照、P1 图、Code A hotbar、P9/P11 和旧 SaveGame 不产生第二份真值。候选必须存在、数量大于零、`bQuickUsable` 且当前父容器等于 `BasicContainerId`；重绑同一 ItemId 先原子清除旧槽，解绑不改物品。schema 为 P5 `6→7`、P6 `2→3`；历史 definition 仅由既有 consumable category 迁移到显式 Code B semantic，不凭空创建物品或效果。P5→P6、P8 Extracted、P7/P10/P12 已接受事务、bridge/rebind 都在同 Owner durable save 前 reconcile，失效引用清空但绝不自动绑定；Dead/RecoveredAbandon 丢弃 P6 binding。P3 Host 呈现只读九槽与显式 bind/unbind，拒绝拖拽绑定，不实现 1—9 键、使用或消耗。最终指定 Editor 编译 native exit `0`、UBT `Succeeded`；P 阶段未运行产品、自动化、回归、截图、Smoke、Game Build、Cook 或 Package，真实功能验证留待 `0.0.9B.F`。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。
- 恢复路线：从已审计 r0 隔离副本复制恢复；原 XFix1 源保持未修改，隔离副本继续保留。

## P15.0.r0

- P15 快捷消耗：只有正常 Code A Run 的离散 1--9 输入才可发起，并严格拒绝满血、死亡、terminal、UI 打开、Run/Owner 不匹配或无 P6 committed session。Code B 是唯一 item/receipt authority：复核 P13 BaseQuick reference 与 data-defined `RestoreHealth`，原子扣同一 ItemId 一单位、归零移除、reconcile binding 并保存 Pending receipt。P6 schema `4→5` receipt ledger 以 owner/run/ordinal 生成稳定 id，包含 slot/source/effect/amount/state，不镜像库存。Code A health 组件只保留本 pawn/run 的已处理 ReceiptId，以 capped restore 幂等应用；完成后 Code B ACK，失败保持 Pending 供正常游戏重试。P8 不返还已消耗物品或 ledger；P5 schema `7→8` 只增加 definition effect serialization，旧数据只从既有 HealAmount 定义迁移。Editor 最终编译 exit `0`、`Succeeded`；未进行真实功能测试，全部留给 `0.0.9B.F`。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。

## P16.0.r0

- P16 确定性加权 Loot：P9 `BasicCache` 和 P11 `BasicCorpse` 的首次物质化现在各解析一个 Code B-only Profile：`CodeB.LootProfile.BasicCache.r1` 为 `SpiritDust`（weight 3，quantity 2--3）或 `IronShard`（weight 2，quantity 1--2）；`CodeB.LootProfile.BasicCorpse.r1` 为 `IronShard`（weight 3，quantity 1--2）或 `SpiritDust`（weight 2，quantity 1）。两者仅有一个固定必出 group，候选只引用现有正式 Code B definition。稳定候选排序、CRC32 r1 算法、Owner/Run/目标/来源 Definition/尸体 DeathReceipt/Profile id+version+digest 输入保证相同身份重建相同 item、数量、slot 与 digest；不存在时钟、地图 Actor、UI、显示名、临时 GUID、旧 Loot 或可变 RNG 输入。
- P16 持久化与边界：P9/P11 record schema `2→3` 增加 Profile provenance/result digest；在同一个 Owner durable replacement 内一次写入完整 P1 graph、Hidden state、既有 receipt/digest 与 provenance。旧 fixed-recipe materialized record 的 item/container/child graph、visibility、receipt/digest 保持历史原样；缺少 provenance 的读取显式视为 legacy。重复 materialize/replay/query/open-close/interruption/recovery/冲突继续读取原 record 或零写入。P10/P12 仍只读取/领取 materialized truth；P13/P14/P15、P5/P6、P8 和 Code A 的地图、Actor、Loot、Run Save、战斗、HUD、终局权威未改。Editor 编译 native exit `0`、UBT `Succeeded`；真实 roll、重开、死亡/打开重放、P10/P12 转移、恢复、P8 三种结局、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验收均留给 `0.0.9B.F`。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。

## P18.0.r0

- P18 将仅限未来未 materialized 的 `CodeB.NormalContainer.BasicCache` 升级到 `CodeB.LootProfile.BasicCache.r2`。`BasicCache.r1` 与 `BasicCorpse.r1` 继续由既有 receipt 的 Profile id/version/digest 选择并保持历史图、result digest 与验证不变。r2 保留 r1 `Guaranteed.Main`，增加确定性 `Optional.SpatialUtility`：`NoDrop:9 / Spawn:1` gate，命中后 WindTalisman（weight 1）与 BackpackLevel1（weight 3）选一，数量均为 1。
- 命中项仅作为 BasicCache root 的正式空间 parent 生成；稳定 ItemId 与 P17 `SpatialChildGuid(ItemId)` 产生唯一、空、具正式 capacity/layout 的 child container。P1 验证阻止环、嵌套、重复 owner、非法 slot 和半图；P10 复用原 Drop/Move 与 owner-document replacement，新增提交前整图检查确保只移动 Revealed parent + 不变 child graph 到 exact P6，拒绝 parent-only、flatten、clone、新 ItemId、P6 外目标或 partial commit。P7/P5/P6/P8、P11/P12、P13/P14/P15 和 Code A 未获新语义。
- 2026-08-08 仅执行指定的 `demo_mapEditor Win64 Development`，native exit code `0`、UBT `Succeeded`。未启动产品、真实 roll/转移、P7/P8/recovery、自动化、回归、截图、Smoke、Game Build、Cook、Package 或最终验收；全部由 `0.0.9B.F` 负责。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。

## P19.0.r0

- P19 只扩展 P14 的单 root WorldDrop：`WindTalisman` 与 `BackpackLevel1` 可把 stable parent、唯一 P17 ChildContainer 与全部合法普通 child contents 作为不可拆分图，从真实 P7 BaseQuick／匹配装备 cell 经 GroundDropZone 转入同一 P6 record；原 P14 simple BaseQuick path 不变，不存在 child list、第二世界库存、child WorldDrop、clone 或 flatten。
- WorldDropTarget 仍只挂一个 root cell，不打开、渲染或操作 child。真实 P4 Drop 只准 root 回到空 BaseQuick，或 parent 自身的匹配空装备栏；Store 以精确 Owner/Run/revision、P1 candidate equality、formal child semantic/capacity、empty target 和 Owner durable replacement 复核。成功丢弃清理 transient 选择／drag 状态，成功拾回才移除同一 record；P13 只 reconcile、不自动恢复，P15 与 Code A 的图、库存及终局权威不变。
- P8 player-only closure 逐个移除仍在 WorldDrop 中的 root、child container 与 child contents，因此 Extracted、Dead、RecoveredAbandon 都不会将其任何部分带回 P5。2026-08-08 指定 Editor 编译 native exit code `0`、UBT `Succeeded`；未启动产品或执行真实 graph 往返／Actor／rebind／终局、自动化、回归、截图、Smoke、Game Build、Cook、Package 或最终验收，均由 `0.0.9B.F` 负责。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。

## P20.0.r0

- 未来首次物质化的唯一 BasicCorpse 使用 `CodeB.LootProfile.BasicCorpse.r2`；r1 与 BasicCache.r1/r2 的既有 receipt 和 record 不改写。r2 保留原主材料并追加 `Optional.SpatialUtility`：NoDrop 9、Spawn 1，命中后按 WindTalisman 1 / BackpackLevel1 3 选择一个 qty 1 parent；所有 draw、stable ItemId、child id、profile/result digest 均由 OwnerId、RunInstanceId、BodyTargetId、DefinitionId、DeathReceiptId、profile provenance 和 CRC32 r2 计算。
- P11 只保存完整且空的正式 parent/ChildContainer graph；P12 UI 仍只显示 root。生产 NativeOnDrop 和 Store 共同拒绝 Hidden/Searching、child content、partial/clone/flatten、新 id、非空 BaseQuick、direct equip 及 P6→P11 空间图回流；simple P12 路径不改。P7/P8/P13/P14/P15/P17/P18/P19、P9/P10 与 Code A 权威均无新产品语义。
- 2026-08-08 一次 `demo_mapEditor Win64 Development` 编译以 native exit code `0`、UBT `Succeeded` 完成。真实 roll、死亡/开尸/揭示、P12 graph 转移、P7/P19/P8、recovery、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验证均留待 `0.0.9B.F`。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。

## P21.0.r0

- P21 只影响未来首次 materialize 的 `CodeB.BodyContainer.BasicCorpse`：新增 `CodeB.LootProfile.BasicCorpse.r3` / `CodeB.DeterministicWeightedLoot.Crc32.r3`。r3 原样继承 r2 的 Guaranteed.Main 与 Optional.SpatialUtility（NoDrop 9 / Spawn 1），并增加 `Optional.EquippedLoadout`（NoDrop 4 / Spawn 1）：按 DefinitionId 字节序从 `EvasionCharm`、`ReinforcedVest`、`HeavyPracticeBlade` 中确定一个或不生成。候选集 digest、OwnerId、RunInstanceId、BodyTargetId、DefinitionId、DeathReceiptId、Profile digest/version 和 algorithm 都在 deterministic identity/receipt 中固定；r1/r2/BasicCache 历史 receipt、图和选择行为均保持不变。
- r3 尸体 P11 记录在原材料 root 之外固定保存三个 capacity-1 P1 equipment containers：`Body.Weapon`、`Body.ArmorRobe`、`Body.Accessory0`。三格始终存在、默认空；命中时只在匹配槽保存同一真实简单非空间 item，数量 1、无 child。P12 只读展示它们；Hidden/Searching 掩码继续不暴露 ItemId，揭示 item 只能由 NativeOnDrop 进入空的 P6 BaseQuick。
- 提交端通过 P1 candidate snapshot 的完整等价复核，仅接受该固定 corpse equipment cell 到空 P6 BaseQuick 的单一 `Move`；拒绝 direct equip、合并、交换、回存、clone、new id、非空 target 和 Hidden/Searching extraction。同一 Owner durable replacement 原子更新 P11/P6，失败没有半提交。P1 只为 `CodeB.Body.*` corpse equipment source 开放该受 P12 限制的 Move；普通玩家 equipment 仍必须走 Unequip。没有新增 Code A 运行时权威。
- 本轮首次 Editor 编译发现并修正了 receipt 字段归属；随后按同一指定目标重编译，native exit code `0`、UBT `Succeeded`。未运行产品、自动化、回归、截图、Smoke、Game Build、Cook 或 Package；所有真实功能验证仍留给 `0.0.9B.F`。状态：`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`。

## 当前入口

- 固定入口：`C:\AIDev\shanmen-ue\latest demo.bat`；它只委托统一调用器，优先 packaged `Latest_Demo`，否则明确使用 Editor `-game` 回退。
- 默认 Smoke 地图：`/Game/M01/Maps/L_M01_Expedition`
- P4.r1 构建：`demo_mapEditor Win64 Development` 与 `demo_map Win64 Development` 均成功，游戏二进制为 `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Binaries\Win64\demo_map.exe`。
- P4x r2 回归：Code B `58/58`、ProfileSettlement `19/19`、ProfileSession `33/33`、ProfileNormalStartup `17/17`、ProfilePreparationFlow `17/17`、四个产品 Start Run 场景及第二次启动均成功；`P4x.RealInputTrace` 在 1280×720/1920×1080 均通过。日志见 `Saved\Logs\P4x.r2.*.current.log`。
- 默认 Smoke：`/Game/M01/Maps/L_M01_Expedition?Name=Player` 成功启动并退出，未显式 Open Host，Fatal/ensure/assert/Automation errors 均为 `0`。
- P3 截图：`Saved\P3Screenshots\P3_Initial.png`、`P3_SelectedDetail.png`、`P3_MoveSuccess.png`、`P3_OccupiedError.png`。
- P4x r2 可见证据：`Saved\P4xScreenshots\P4x.0.r2_*.png` 均为本轮真实挂载 UI/产品入口生成；不使用 P4 旧截图或 Console 直调成功态。主证据仍为真实 Slate/UMG trace 和代码审查，符合用户“优先代码审查”要求。
- P4.r1 日志：`Saved\Logs\Dev.D.UE.0.0.9B.P4.0.r1_all_codeb_automation.log`、`...profile_settlement.log`、`...profile_session13.log`、`...real_input_1280.log`、`...real_input_1920.log` 与 `...default_map_smoke.log`。
