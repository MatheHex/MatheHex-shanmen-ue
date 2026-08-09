# Dev.D.UE.0.0.9B.P1.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P1 ——代码 B 基础物品权威与事务内核
- 任务编号：`Dev.D.UE.0.0.9B.P1.0.r0`
- 任务性质：正式功能开发；建立可独立验证的代码 B 数据与事务基础
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 工程级开发基线：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`
- 预期引擎：Unreal Engine 5.8
- 上一任务：`Dev.D.UE.0.0.9B.I0.0B.r0`
- 报告文件：`Dev.D.UE.0.0.9B.P1.0.r0_report.md`

## 已接受的初始化结果

`Dev.D.UE.0.0.9B.I0.0B.r0_report.md` 已通过策划验收并确认：

1. `Dev.D.UE.0.0.9-XFix1` 是唯一授权的工程级开发基线。
2. 活动工程 `Dev.D.UE.0.0.9B` 已恢复并可由 UE 5.8 构建、启动和退出。
3. 原 XFix1 源保持未修改。
4. 旧 `I0.0.r2` 已由 `I0.0B.r0` 取代，不再追查 0.0.8。
5. 基线旧物品管理实现登记为“代码 A”；0.0.9B 新背包、仓库、装备、Loot、搜索、拖拽、转移与持久化体系登记为“代码 B”。
6. I 阶段已经结束，本任务是代码 B 的第一份正式 P 阶段实现任务。

## 版本目标与权威边界

建立：备战整理 → 进入地图 → 搜索容器与尸体 → 转移、装备与管理物品 → 撤离 → 同一批真实物品进入局外仓库的底层基础。

核心约束：一件真实物品只有一个稳定实例、一个真实父位置、一个统一修改入口，并能保持 GUID、数量、属性、词条与容器关系一致。当前只实现稳定的 `1x1` 物品；多格、旋转和不规则形状只保留扩展空间。

代码 A 可以继续支撑当前游戏运行，但不再是 0.0.9B 新物品系统的最终设计权威。本任务不删除或大规模重构代码 A，也不修改地图、怪物、战斗、Run、撤离、输入、资产或既有功能。

代码 B 是后续物品管理体系的最终权威。本任务建立隔离的底层数据与事务内核；现阶段不接入玩家、仓库、Loot、世界掉落或结算正式运行路径，不允许 A/B 同时写入同一批物品，不建立临时双写。允许单向只读复用稳定定义、Gameplay Tags 或通用值类型，但不得把代码 A 可变 Authority 状态引入代码 B。

## 单一目标

在活动工程中建立清晰隔离的代码 B 物品领域，实现：

- 明确物品定义规则；
- 唯一 ItemInstance；
- 唯一位置／容器关系；
- 单一 Repository；
- 统一 `Move`、`Swap`、`Merge`、`Split`、`Equip`、`Unequip` 事务；
- 只读快照与明确错误结果；
- 原子提交、失败回滚、Revision 与不变量检查；
- 真实可编译、可运行测试的 C++ 实现，而非架构说明、伪代码或空接口。

## 核心不变量

- 每个有效 `ItemId` 在 Repository 中只对应一个实例。
- 每件已放置物品只有一个真实父位置。
- 每个位置最多由一件 `1x1` 物品占用。
- 未放置状态必须与已放置状态明确区分。
- 不可堆叠物品与装备 `Quantity` 恒为 1；可堆叠物品数量始终为 `1..MaxStack`。
- 装备类型必须匹配装备槽；兵器、道袍、空间道具等单一槽最多一件。
- 容器容量、格位与槽位规则始终有效。
- 失败事务不得留下部分修改；成功事务只提交一次并产生新快照版本。
- UI、Widget、Actor、Presenter 不拥有物品真值。
- 代码 B 只能有一个 Repository 权威；不得存在原型 Repository 与正式 Repository 双套真值。
- 当前模型不保留嵌套容器引用；若未来加入，必须禁止自包含与容器循环。
- 代码 B 正式修改必须经过同一事务入口，测试辅助函数不得成为运行时旁路写入口。

## 实现要求

### 源码边界

在 `Source/demo_map` 内建立容易识别、便于后续迁移的代码 B 目录、类型前缀或等价边界。Report 列出代码 B 根目录、核心类型、与代码 A 的编译依赖方向、复用类型及无 A 可变 Authority 写入证据。不新建 Unreal Project，不建立多个互相竞争的 Repository。

### 基础数据

物品定义至少包含稳定 Definition ID、可堆叠标志、Max Stack、物品类型／标签、装备槽类别、当前固定 `1x1` 尺寸和容器／槽位规则。定义不得保存具体实例 GUID、数量或位置。

物品实例至少包含稳定唯一 ItemId、Definition ID、Quantity、等级／品质、随机种子／词条等可扩展实例状态，以及未来空间道具内部容器引用的扩展位置。

位置与容器必须能表达普通储物格、装备固定槽、ItemId 与 Parent Container／Location 的唯一关系、仓库、基础 6 格、空间道具储物和装备等未来容器种类，以及容量、允许类型、槽位和当前 `1x1` 坐标。

单一 Repository 负责按 ItemId 管理实例、按 ContainerId 管理容器、维护唯一位置索引、提供受控 seed／fixture API、验证与提交、只读快照、完整不变量检查和单调递增 Revision。

### 事务内核

每个请求可携带 Transaction ID、Operation Type、ItemId、可验证来源、目标容器与目标格／槽、数量及 Expected Repository Revision。每次事务读取权威状态，验证来源／目标／数量／容量／类型／版本，预计算变更，在工作副本或等价安全机制中完整形成结果，全部通过后一次性提交，提交后检查不变量并返回明确结果、受影响对象和新 Revision。任一步失败时状态与 Revision 不变。

- Move：支持同容器和跨容器移动，源匹配、目标空位、容量与类型有效后唯一迁移。
- Swap：目标已有物品时双方同时验证，均合法才一次提交，任一非法则原状保留。
- Merge：只允许相同可堆叠定义，支持部分／完整合并，不超过 Max Stack，来源归零时从 Repository 和位置索引一致移除。
- Split：只允许合法拆分数量，新堆叠取得新 GUID，原实例数量正确减少，创建与放置在同一事务提交。
- Equip：验证类型与槽位匹配，装备不可堆叠且数量为 1；空槽可视为受规则约束的 Move，占用槽位的替换必须同时验证旧装备去向。
- Unequip：先验证目标储物格可用，成功后同一实例从装备槽迁移到储物格，失败不得先清空装备槽。

错误结果至少能区分 Item Not Found、Source Mismatch、Target Not Found、Target Occupied／Full、Invalid Slot／Tag Rejected、Invalid Quantity、Stack Mismatch／Full、Duplicate Item／Placement、Container Cycle、Stale Revision、Invariant Violation 和 Internal Commit Failure。

### 只读快照

至少提供 Revision、物品实例、容器规则、每件物品唯一位置、容器有序内容、装备槽内容及最近事务影响的 Item／Container ID。快照不得让调用方直接修改 Repository。

## 自动化与验收

确定性测试至少覆盖：唯一物品／容器创建、普通与同容器 Move、满容器／占用格失败、合法与单边非法 Swap、部分／完整／异定义 Merge、Split 新 GUID 与非法数量回滚、兵器／道袍／饰品／空间道具装备槽、非法 Equip、占用槽合法替换、无空间 Unequip、重复 GUID／双位置／无效数量、不变量、Stale Revision、失败前后快照逐字段一致、成功 Revision 递增。

固定 Seed 的随机事务测试至少执行 1,000 次 Move、Swap、Merge、Split、Equip 或 Unequip 尝试；每次检查核心不变量、Revision 规则、ItemId 唯一、位置唯一、数量守恒和无零数量实例。失败可以是合法失败，但状态必须保持一致。

完成 Editor Development、Game Development、代码 B 自动化测试和默认地图 Smoke。Smoke 需正常加载默认地图并退出，无新增 Fatal、崩溃或阻断错误；确认 XFix1 源未修改。不要求 Cook、Package、Latest_Demo 或人工试玩。

## 不做内容

不删除／大改代码 A，不迁移旧 Profile，不接管正式玩家或 Run，不制作仓库／背包／Loot／搜索／拖拽／详情 UI，不实现快捷栏、世界拾取、完整空间道具内部移动、代码 B 持久化、多格／旋转／不规则物品、自动整理、地图／怪物／战斗／美术改动、发布包或下一份 P 任务。

## 停止条件

若活动根、基线、代码 A/B 隔离、共享核心、关键不变量、失败回滚或构建来源出现无法安全解决的不确定性，保留安全工作，生成同名 Report，使用 `NEEDS_P1_REWORK`、`NEEDS_PLANNER_DECISION` 或 `BLOCKED`，不得用临时双写或绕过不变量强行完成。

## Report 与交接

生成 `Dev.D.UE.0.0.9B.P1.0.r0_report.md` 到 `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report`，至少包含实际数据模型、事务语义、原子提交／回滚／Revision、快照、不变量、A/B 依赖、新建／修改／保留文件、确定性测试命令和结果、Seed／尝试／成功／合法失败计数、构建／Smoke 日志与产物、XFix1 未修改核对、已知问题、下一单一 P 任务建议和最终结论。

最终状态仅可为：

- `READY_FOR_NEXT_CODE_B_VERTICAL_SLICE`
- `READY_FOR_NEXT_CODE_B_VERTICAL_SLICE_WITH_NONBLOCKING_FINDINGS`
- `NEEDS_P1_REWORK`
- `NEEDS_PLANNER_DECISION`
- `BLOCKED`

若核心事务、失败回滚、不变量、自动化测试或构建存在未解决失败，不得使用 READY 状态。

完成后不要自动开始下一份 P 任务。向策划 Chat 回传并只附带本次同名 Report，正文首行必须为：

`[CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P1.0.r0","file":"Dev.D.UE.0.0.9B.P1.0.r0_report.md"}`
