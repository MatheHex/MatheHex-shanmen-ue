# Dev.D.UE.0.0.9B.P5.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P5——Code B 真实局外 Profile、仓库／人物配置与持久化接入
- 任务编号：Dev.D.UE.0.0.9B.P5.0.r0
- 任务性质：将已验收的 Code B 局外整理纵向切片从 fixture 提升为真实玩家局外物品状态；不进入局内 Loot、搜索、世界掉落、撤离或死亡结算。
- 执行文件：Dev.D.UE.0.0.9B.P5.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P5.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：Dev.D.UE.0.0.9B.P4x.0.r2

## 上半部分：只读决策、现状与边界

### 1. 已接受的状态

P4x.0.r2 已被策划验收。以下事实必须继承，不能回退或重新解释：

- 0.0.9-XFix1 是完整可运行工程基线；Code A 仍承担当前地图、怪物、战斗、默认玩家、Run、旧 Loot、结算与旧运行时。
- Code B 的 P1 Repository 是新物品系统的唯一可变真值，唯一写入链为：真实 UMG／Slate 输入 → P4 Interaction Controller → P3 UI Controller → P2 Application Service → P1 Repository。
- P4／P4x 已证明真实 Cell 命中、拖拽、Preview、Drop、一次手势一次事务、取消、关闭重开，以及双击／右键真实到达但零写入。
- 普通仓库格、基础 6 格、已装备空间戒指的快捷空间内部格、空间储物囊内部格都不按物品类别互斥；只有装备栏位实施类别和单槽互斥。
- 位置变化、合并、交换、替换、装备和向明确普通目标的卸下均只由真实拖拽 Drop 提交；不存在 QuickMove、放回、双击、右键、详情按钮或隐藏命令的可达写入入口。
- 空间戒指仅在装备后显示“快捷空间（空间戒指）”；空间储物囊是普通位置可打开的“非快捷储物囊”。两者都有稳定 Child ContainerId；带内容整体移动必须原子拒绝。
- 旧备战已从实际 Start Run 链移除。Start Run 不得再创建、恢复、校验或依赖备战布局、装备、空间引用、Code B Host、Fixture、Repository、页面或拖拽对象。
- P4x.0.r0 是未接受的候选证据，必须保持只读历史记录；P4x.0.r2 是当前有效 P4x 验收记录。

当前的不足也必须如实面对：P1–P4x 的 UI 仍显示确定性 fixture，而不是正式玩家 Profile 的真实局外物品。因此用户在游戏结束后看到的 WPN 等 fixture 内容不能被当成战利品或正式仓库。P5 的目标就是结束这一点，但不提前把 Code B 接进局内运行时。

### 2. 本阶段的产品裁决

0.0.9B 的最终闭环是：真实玩家背包／装备 → 搜索容器与尸体 → 转移与装备 → 撤离 → 同一批真实物品进入局外仓库。P5 只完成这个闭环的第一段正式落点：真实 Profile 的局外仓库／人物配置和持久化。

因此，自本任务成功完成后：

- 玩家在宗门／局外的正常、可点击入口打开“仓库／人物配置”时，看到的是该 Profile 的 Code B 持久化物品状态，而不是 fixture；
- 该页面的拖拽、装备、卸下、合并和空间容器操作修改同一份持久化 Code B 状态；关闭页面、重新打开、退出并重新启动工程后，实例、位置、数量、词条、Child ContainerId 与 Revision 保持一致；
- 旧 Code A 局外物品页面不能继续作为同一 Profile 的第二个可编辑、用户可达真值入口。它应被新入口路由替代、不可达或明确只读历史化；不得保留两套可编辑仓库。

本任务对 P4x 的“Code B 尚未接管 Profile／SaveGame”限制作唯一、精确的阶段性授权：允许 Code B 以现有正式 Profile 的稳定身份为所有者，建立并持久化版本化的“Code B 局外物品记录”。该授权只覆盖局外物品、装备布局、仓库、基础 6 格、空间容器和页面状态所需的 Profile 持久化边界；不授权 Code B 接管 Run、地图、玩家 Actor、战斗、世界掉落、Loot、搜索、撤离／死亡结算或它们的 SaveGame 逻辑。

### 3. 单一权威与迁移原则

P5 不是把 fixture 写进存档，也不是创建 Code A／Code B 双写。

对于每个正式 Profile，必须存在一个可审计的 CodeBOutOfRaidInventory（名称可按工程实际调整）记录，至少具有：

- 稳定 Profile OwnerId、版本号、Repository Snapshot、Revision、创建／最近提交时间；
- Code B ItemId、DefinitionId、数量、词条／种子等实例真值、唯一父位置、ContainerId、Child ContainerId；
- 一次性导入／接管收据：来源 Profile、来源指纹、旧实例到 Code B 实例的映射、状态、时间和异常项；
- 仅用于真实迁移诊断的最小历史摘要；它不得成为另一份可编辑物品清单，也不得把完整旧仓库复制成两份可用库存。

若现有 Profile 含有可识别的旧局外物品或装备，第一次进入新的正常局外入口时必须走可恢复、幂等的接管事务：

1. 只在 Profile 不处于活动 Run 时读取允许的旧局外来源；
2. 预检来源、建立映射和可验证 receipt；
3. 原子创建或恢复 Code B Snapshot；
4. 仅在提交成功后，将旧局外 UI／写入口从该 Profile 的正常可达路径移除或路由到 Code B；
5. 再次打开、重启、重复点击或中途失败恢复都不得重复导入、复制物品、清空有效物品、伪造装备或默默丢弃无法识别项目。

旧字段可以保留为不可编辑的兼容／审计来源，但在接管完成后不能再被任何用户可达局外物品 UI 当作活跃真值，也不得在后台写回、覆盖或镜像 Code B Snapshot。若某项旧数据无效，必须保留原因和可审计计数；不得虚构替代物品。若无法安全区分旧局外真值与活动 Run 真值，停止受影响接管路径并报告，不得猜测。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不回退 P4x、也不接管局内系统的前提下，完成真实局外 Code B Profile 闭环：

1. 建立由正式 Profile 身份拥有的版本化、可持久化 Code B 局外 Repository Snapshot；
2. 以可恢复、幂等、无双写的方式接管现有 Profile 的有效局外物品／装备数据，或为真正空 Profile 建立空的真实状态；
3. 把现有 P3/P4x 页面从 fixture Host 改为正常玩家可点击入口所创建的真实 Profile Host；
4. 让真实页面全部继续通过 P4 → P3 → P2 → P1 进行拖拽事务，并在每次接受事务后可靠持久化；
5. 保持 Start Run 完全不依赖、不初始化和不写入 Code B 局外状态；
6. 提供可审计的迁移、重启恢复、真实 UI、回归、构建和截图证据。

不得开始 P6、局内背包、容器／尸体搜索、世界物品、丢弃／拾取、1—9 使用、消耗品、撤离／死亡结算、Run 持久化或任何地图／怪物扩展。

### 5. 实现要求

#### 5.1 真实 Profile 局外状态

- 新的 Code B 局外记录必须按真实正式 Profile 的稳定身份隔离；Profile A 的任何打开、提交、关闭、重启或失败恢复不得读取、修改或显示 Profile B 的物品。
- 实际正常入口不得调用 CreateFixture、ResetFixture、确定性 WPN／测试数据注册或任何仅开发命令的初始化路径。开发命令可保留给测试，但不能是正式入口的后门。
- 可选择扩展既有 Profile 持久化边界或建立与正式 Profile 生命周期绑定的版本化 Code B 记录；无论物理实现如何，它都必须由正式 Profile 的创建、选择、删除／重置与安全恢复正确管理，且 Code A 不可把它当作可编辑旧仓库镜像。
- 写入必须使用原子 snapshot／journal／同等提交方式：任何保存失败、进程中断、版本不兼容、重复导入或部分数据错误后，都只能得到已提交旧状态、完整新状态或可恢复 pending receipt；不得得到半个仓库、重复实例或失去 Child ContainerId 的空间物品。
- 每次真正接受的 P2/P1 事务最多产生一次 Code B snapshot 提交和一次持久 Revision 递增；选择、Hover、Preview、双击、右键、Esc、Cancel、Close、无效 Drop、Stale Revision 和保存失败不得伪造成功或局部写入。

#### 5.2 旧局外数据的安全接管

至少为以下隔离 Profile 场景提供真实、稳定的自动化与日志：

- `LegacyPopulated`：旧局外来源含兵器、道袍／护甲、饰品、空间戒指、空间储物囊、材料及至少一个带内部内容的空间容器。接管后 Code B 保留有效数量、属性／词条、装配语义、内部关系和来源映射；不把物品复制成两份活跃库存。
- `EmptyProfile`：没有旧物品时创建真正空的 Code B 局外状态；无需任何装备，页面和 Start Run 都可正常工作。
- `LegacyInvalid`：包含失效旧武器 ID、失效空间引用或非标准旧 Hotbar 数据。有效内容可安全接管；无效内容必须有明确、可计数记录，且不阻断页面、保存或 Start Run。
- `InterruptedOrRepeatedHandoff`：在 receipt 已开始但提交前、提交后未清理、重复打开、关闭重开和重启的至少一种中断组合中验证幂等恢复。最终只能有一次 Code B 实例集和一份 receipt。
- `TwoProfiles`：两个独立正式 Profile 各自拥有不同状态。反复切换和重启后，所有 ItemId、ContainerId、Revision 与可见物品仍严格按 OwnerId 隔离。

只允许在无活动 Run 的 Profile 上接管。活动 Run 中请求打开／接管时必须无写入拒绝，并以清楚中文提示“结束当前 Run 后再整理”；不得从 Run 复制或删除任何物品。

#### 5.3 正常用户入口与真实页面

- 在宗门／局外已存在的正常玩家导航中提供一个清楚、可点击的“仓库／人物配置”入口；复用既有页面外观和输入层是允许的，但它必须由真实 Profile 创建而非 Console fixture。
- 默认地图启动时不得自动创建 Code B Host、Repository、页面或写入；只有用户实际点击局外入口才可以惰性加载所选 Profile 的 Code B Host。
- 入口打开后，页面须展示真实仓库、装备栏、基础 6 格、空间戒指快捷空间（仅装备后）和空间储物囊非快捷内部格；不得将未装备戒指误显示为快捷空间，也不得让储物囊获得 1—9、快捷使用或 QuickMove 语义。
- 旧备战页面不得借该入口重新成为 Start Run 阶段、初始化器或校验门槛。若旧 Code A 局外物品入口仍存在，必须对已经接管的 Profile 路由到 Code B 页面或不可编辑地历史化，不能让用户在两套可写库存间切换。
- 真实页面必须保留 P4x 已验收规则：拖拽为唯一位置写入、普通格无类型互斥、装备栏唯一互斥位置、条件式“装备／卸下”仅给提示而不猜测移动目标、无“放回”按钮、双击／右键零写入。

#### 5.4 持久化后的真实事务

在真实 Profile 页面、真实缓存几何和 Slate hit-test 路径中，于 1280×720 与 1920×1080 各完成：

1. 仓库、基础 6 格、已装备戒指快捷空间、非快捷储物囊之间的 Move、Merge、Swap；
2. 兵器／道袍／饰品／空间戒指的合法 Equip、已占用兼容栏位的原子 Replacement／Swap，以及向明确普通目标的 Unequip；
3. 不兼容装备栏、Loaded Spatial、容量不足、同源、Stale Revision、UI 外部 Drop 的无写入拒绝；
4. 未装备兼容物只显示“装备”，已装备物只显示“卸下”，不可装备物两个动作均不显示；
5. 空戒指或空储物囊可移动；带内部内容的 Ring／Pouch 整体 Move／Swap／Equip／Unequip 由同一个 P1 原子拒绝；
6. 关闭页面、重新打开、完整退出并重新启动工程、重新选择该 Profile 后，验证接受过的每项事务的 ItemId、DefinitionId、数量、父位置、ContainerId、Child ContainerId、详细信息和 Revision 与提交后 snapshot 完全一致；
7. 双击、右键、详情、Esc、Cancel、Close、重复 Open、禁用 1—9 占位与无 ItemId 格均保持 P2 CommandCount=0、P2 CallCount=0、持久 Revision 不变。

新增或适配的真实验证入口可以命名为：

```text
CodeB.P5.RunRealProfileTrace [ProfileCase] [expected width] [expected height] [Quit]
```

它必须使用真实 Profile Host、生产 Cell 的 GetCachedGeometry()／等价坐标、Slate hit-test 和真实鼠标／键盘事件。不得直接调用 Widget NativeOn...、P3/P4 Controller、P2 Service、P1 Repository、存档写接口或 fixture setup；只能读取持久化后 Projection／snapshot 做断言。每条 trace 至少记录 OwnerId、HandoffState、实际命中 Cell／Container／Slot、ItemId、Gesture、DragOperation、Preview、P2 CommandCount、P2 CallCount、Repository RevisionBefore／After、PersistentRevisionBefore／After、保存结果和恢复结果。

#### 5.5 Start Run、运行时与旧边界回归

P5 不得让正常 Start Run 从 Code B 读取装备、初始化仓库、恢复 fixture、写 snapshot 或成为页面是否打开／迁移是否完成的函数。至少验证：

- 未点击局外入口的默认地图启动，Code B Host／Repository／页面／写入均为 0；
- 已成功接管并关闭真实局外页面后，Normal、NoEquipment、MissingPreparation、StaleLegacy 四类实际产品 Start Run 及返回后再次 Start Run 均通过；
- 真实局外页面打开时，Start Run 不得偷偷以页面或 Code B 状态作为前置；若产品规则要求先关闭页面，关闭只处理 UI 生命周期，不能写入或改变 Code B inventory；
- Code A 继续拥有当前 Run、地图、玩家 Actor、战斗、Loot、搜索、结算与 Run Save；本任务不得添加任何 A/B 同步、镜像、双写或由 Code B 生成正式运行时物品的路径。

### 6. 验证、资料与来源审计

至少完成并在 Report 逐项列出命令、结果、退出码和最终日志／产物：

- 全部 demo_map.CodeB.P1、P2、P3、经 P4x 规则更新的 P4、P4x 与新增 P5 自动化；
- 第 5.2 节五种真实 Profile／接管场景；
- 第 5.4 节两档分辨率真实 Profile 输入与重启恢复 trace；
- 受影响的 ProfileSession、ProfileNormalStartup、ProfilePreparationFlow、ProfileSettlement 和现有正常产品 Start Run 回归；
- `demo_mapEditor Win64 Development` 与 `demo_map Win64 Development` 构建；
- 默认地图 `/Game/M01/Maps/L_M01_Expedition?Name=Player` 在未点击入口时的 Smoke；
- 当前 P5 可见 Smoke／截图；
- 对每份最终日志独立检索 Fatal error、crash、Ensure condition failed、Assertion failed、LogAutomationController: Error、P5_* FAIL、存档／迁移失败和 trace failure。

在 `Saved\P5Screenshots` 或等价新目录生成带任务 ID、时间和分辨率的当前 PNG。至少提供：

1. 默认地图未点击入口、没有 Code B Host 的证据；
2. 正常玩家可点击的宗门“仓库／人物配置”入口；
3. LegacyPopulated 真实 Profile 接管后的页面，清楚展示非 fixture 的仓库、装备与两类空间容器；
4. 空间戒指装备前后的快捷空间状态与储物囊非快捷内部格；
5. 真实 Profile 的 Merge、装备栏拖拽高亮／成功、Loaded Spatial 或类型拒绝；
6. 三种详情条件显示；
7. Close／restart／reopen 后同一真实状态恢复；
8. DoubleClick／RightClick 到达但持久化零写入的可追溯 trace 片段；
9. EmptyProfile、LegacyInvalid、两 Profile 隔离和一次 Direct Start Run 的证据。

对活动工程和 0.0.9-XFix1 的 `Source\demo_map` 做逐文件 SHA-256 或同等内容审计。Report 必须将差异分为：

- 已接受 P4.0.r1 的 4 个 Code A 修复；
- 已接受 P4x 去备战化差异；
- 本 P5 新增／修改的 Code B 文件；
- 本 P5 为 Profile 持久化、正常入口、旧局外 UI 路由、迁移 receipt 或回归所必需的精确 Code A 文件及逐文件理由；
- 任何意外差异。

出现来源不明差异时，停止受影响工作；不得自行吸收、清理、回滚或将其归为 P5。更新 PROJECT.md、PROJECT_INFO_CARD.md 或等价项目资料，真实记录：P4x 已验收、fixture 与真实 Profile Host 的边界、Code B 局外持久化所有权、接管／receipt 规则、旧 UI 路由、P5 仍未接管的运行时范围、日志、trace、截图和恢复测试路径。

### 7. 停止条件

发生任一项即停止受影响工作，生成同名 P5 Report，且不得使用 READY 状态：

- 真实局外入口仍展示／写入 fixture，或需要 Console／测试命令才能接触真实 Profile；
- 同一 Profile 出现 Code A 与 Code B 两套可编辑、用户可达的局外物品真值，或存在后台双写／镜像；
- 接管会复制活跃物品、丢失有效物品、伪造装备、跨 Profile 混用、破坏 Child ContainerId，或无法幂等恢复；
- 保存失败、重启、重复打开或中断后出现半提交、重复 ItemId、位置不唯一、Revision 回退或读到 fixture；
- Start Run 再次依赖旧备战，或因 Code B 页面／迁移状态而失败、初始化 Code B、读取 Code B 物品；
- 需要 Code B 提前接管 Run、地图、玩家 Actor、Loot、搜索、世界物品、撤离／死亡结算或 Run Save 才能完成；
- 任意非拖拽位置写入入口复活，或空间戒指／储物囊语义回归；
- 必需构建、自动化、实际产品 Start Run、真实 Profile trace、重启恢复、当前截图或默认地图 Smoke 失败／缺失；
- 出现来源不明修改、Fatal、crash、ensure、assert、Automation Controller error、持久化 Error 或本任务新增 Error。

### 8. 验收与交接

以下全部满足才可通过：

1. P4x.0.r2 的真实拖拽、零写入、空间容器与去备战结果不回归；
2. 正常玩家入口加载的是按 OwnerId 隔离的真实、持久 Code B 局外状态，不是 fixture；
3. 旧局外数据的接管可恢复、幂等、无双写、无复制，空／无效／重复／中断／双 Profile 场景均安全；
4. 真实页面的一切位置改变继续仅经 P4 → P3 → P2 → P1，且一次接受手势只有一次 Repository 与持久化提交；
5. 关闭、重开、完整重启和重新选择 Profile 后，所有物品实例、位置、数量、空间关系和 Revision 保持一致；
6. 默认启动与所有已验收 Start Run 路径不初始化、不依赖、不写入 Code B；
7. Run、地图、玩家 Actor、Loot、搜索、世界物品、结算和 Run Save 仍没有被 Code B 接管；
8. 自动化、真实 Profile trace、恢复、Profile 回归、构建、默认地图、产品 Start Run 与当前可见 Smoke 全部通过；
9. 来源审计、项目资料、同名 Report、日志、trace 和截图完整且身份真实。

通过时最终 Report 只能使用：

- READY_FOR_CODE_B_RUN_INVENTORY_BRIDGE
- READY_FOR_CODE_B_RUN_INVENTORY_BRIDGE_WITH_NONBLOCKING_FINDINGS

否则只能使用：

- NEEDS_P5_REWORK
- NEEDS_PLANNER_DECISION
- BLOCKED

生成 `Dev.D.UE.0.0.9B.P5.0.r0_report.md`，存入：

```text
C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report
```

本 Prompt 归档到：

```text
C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Prompt
```

完成后不得自动开始 P6 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

```text
[CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P5.0.r0","file":"Dev.D.UE.0.0.9B.P5.0.r0_report.md"}
```

若 ProjectCode、任务编号、Prompt、Report、活动工程、前置 P4x.0.r2 状态、Profile 所有权边界或目标 Chat 无法对应，停止受影响任务，按 IPF 协议生成 Error001 Report；不得自行猜测、改号、切换项目或直接开始 P6。
