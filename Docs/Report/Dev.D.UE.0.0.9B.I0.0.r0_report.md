# Dev.D.UE.0.0.9B.I0.0.r0 Report

```text
task_id = Dev.D.UE.0.0.9B.I0.0.r0
project_id = Dev.D.UE.0.0.9B
status = READY_FOR_P1_PLANNING_WITH_NONBLOCKING_FINDINGS
blocking_items = 0
formal_P_stage_started = false
```

## 1. 任务结论

0.0.9B 已建立为独立开发目录，工程可以完成增量 Editor/Game Development 构建，并通过 Unreal Editor `-game -nullrhi` 的默认 M01 地图启动 Smoke。I0 未开始任何新的背包、仓库、搜索、装备或地图功能开发。

有一项来源偏差：Prompt 指定的 0.0.8 稳定镜像目录在本机不存在。根据用户本轮“以现有的游戏开始执行开发 Prompt”的明确指令，本任务从现有 `Dev.D.UE.0.0.9-XFix1` 建立了隔离副本，并在本 Report、项目卡和 PROJECT.md 中保留该事实；没有将它伪称为 0.0.8 镜像。

## 2. 工程与来源

```text
active_root = C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
uproject = C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
engine = C:\Program Files\Epic Games\UE_5.8
requested_source = C:\AIDev\shanmen-ue\Dev.D.UE.0.0.8
requested_source_exists = false
requested_xfix_source = C:\AIDev\shanmen-ue\Dev.D.UE.0.0.8-XFix1
requested_xfix_source_exists = false
actual_fallback_source = C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
actual_fallback_source_exists = true
```

源目录保持未修改。目标最初为空目录；复制范围如下，排除了旧 `Binaries`、`Intermediate`、`DerivedDataCache`、`Saved`、`Report`、`Latest_Demo`、备份和临时 staging。

| 区域 | 文件数 | 字节数 |
|---|---:|---:|
| `Source` | 296 | 3,587,866 |
| `Content` | 485 | 142,107,585 |
| `Config` | 5 | 14,253 |
| `Build` | 2 | 2,322,324 |
| `Scripts` | 32 | 157,966 |

目标中的 `Binaries`、`Intermediate` 和 `Saved` 均为本任务构建／Smoke 后新生成的输出，不是迁移的旧生成物。`Latest_Demo` 与旧 `Report` 均未复制。

抽样 SHA-256 与 fallback 源一致：

```text
demo_map.uproject = 0E0827368BAEF28B63DAF2812AB3CEE71BC1ACC28FFE5FCD2E8A74A64423EB84
Config/DefaultEngine.ini = 7CBD40C9B83B8971B8DE195D35201054D042A4E765DBB8D4070747727D70C289
Source/demo_map/demo_mapItemAuthority.h = BFAA163F35F00BD6BCE59553BCFA294105A5BC56125284414673C97EF014329F
```

## 3. 本任务创建或修改

- 建立 `Source`、`Content`、`Config`、`Build`、`Scripts` 的 0.0.9B 独立副本。
- 归档 Prompt：`Docs\Prompt\Dev.D.UE.0.0.9B.I0.0.r0_prompt.md`。
- 归档技术分析：`Docs\Dev.D.UE.0.0.9B_类塔科夫背包仓库搜索系统技术分析报告.md`。
- 创建 `PROJECT_INFO_CARD.md` 与 `PROJECT.md`。
- 将 `OPEN_UE_EDITOR.bat` 的窗口标题从 0.0.8 改为 0.0.9B；未改变启动路径或功能。
- 生成本 Task Report。

没有修改现有 `.9-XFix1` 源码、原工程、原 `Latest_Demo` 或原报告。

## 4. 构建与 Smoke

### 4.1 构建

第一次执行 `BUILD_DEVELOPMENT.bat` 触发了新目录的全量编译；外层命令包装器在约 604 秒时超时，但 UBT 子进程继续完成编译。随后使用 UE 5.8 `Build.bat` 进行增量复核，得到明确结果：

```text
demo_mapEditor Win64 Development = Result: Succeeded / ExitCode 0
demo_map Win64 Development = Result: Succeeded / ExitCode 0
```

已验证产物：

```text
C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Binaries\Win64\demo_map.exe
C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Binaries\Win64\demo_map.target
C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Binaries\Win64\UnrealEditor-demo_map.dll
```

### 4.2 启动 Smoke

直接运行未 Cook 的 `Binaries\Win64\demo_map.exe` 能完成 UE/D3D12 初始化，但随后因源码目录没有 Cook 生成的 Global Shader Library 退出；这不是编译错误，因此未将该路径当作可玩包入口。

使用以下等价 Editor Smoke 完成接收验证：

```text
UnrealEditor.exe demo_map.uproject -game -nullrhi -unattended -nop4 -nosplash -NoSound -ExecCmds=Quit
```

结果：

```text
exit_code = 0
default_map = /Game/M01/Maps/L_M01_Expedition
map_load_complete = 1
clean_exit = 1
fatal_lines = 0
error_lines = 0
log = C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\demo_map.log
```

本任务没有制作或替换 `Latest_Demo`，也没有进行用户人工试玩。

## 5. 现场系统盘点

### 5.1 物品实例与装备／储物权威

- `Source\demo_map\demo_mapItemTypes.h` 定义物品实例、结果、物品分类和位置相关数据。
- `Source\demo_map\demo_mapItemAuthority.h/.cpp` 是当前主要物品写入权威，持有 `TMap<FGuid, Fdemo_mapItemInstance>`、运行时库存槽、装备槽和 Session Stash。
- 已有单一写入入口覆盖 `AddDefinition`、`Equip`、`Unequip`、容器转入库存、库存拖放 Move/Swap/Merge、世界丢弃／拾取、空间道具 Bundle 丢弃／恢复、Run 结算以及 `ValidateInvariants`。
- `Source\demo_map\demo_mapItemDefinitions.h/.cpp` 负责定义、分类、容量、槽位和堆叠等规则；`demo_mapItemPresentation.*` 负责视图投影，不应成为数据权威。
- 风险：当前代码存在历史 `V3` 命名和多个旧测试入口，P1 应先确定 0.0.9B 的统一命名与迁移边界，避免并行建立第二个 Repository。

### 5.2 Runtime Container、搜索和奖励

- `demo_mapRuntimeContainer.h/.cpp` 的 `Fdemo_mapRuntimeContainerAuthority` 只保存容器 Entry、搜索状态、Active Action 和 Revision，不直接保存完整 ItemInstance；物品真实数据仍由 Item Authority 提供。
- `demo_mapSearchContainerTypes.*` 定义 Chest/Corpse、Equipment/Backpack/Body 等分区、Entry 状态和动作结果。
- `demo_mapSearchContainerActor.*`、`demo_mapCorpseContainerActor.*` 负责世界目标和尸体初始化。
- `demo_mapSearchContainerPresenter.*` 与 `demo_mapSearchContainerWidget.*` 负责快照投影、搜索读条、未知物品识别、目标操作、双袋窗口、右键上下文和拖放请求。
- `demo_mapRewardGeneration*`、`demo_mapFixedLootTableRegistry.*`、`demo_mapM01RewardDistribution.*` 负责奖励定义／生成／固定表和 M01 来源；奖励生成与搜索 Entry 状态已经分开，但仍需要在 0.0.9B P 阶段重新确认统一容器模型。

### 5.3 持久化、备战和 Run

- `demo_mapPersistentProfileTypes.h` 当前 Profile Schema 为 6，保存 PermanentStash、PreparationLayout、WarehouseLayout、ActiveRun、资源、城镇和结算标识。
- `demo_mapProfileRepository.*` 负责纯数据序列化、临时文件、备份、原子替换、恢复和校验；没有把 Widget 或裸 Actor 指针作为存档字段。
- `demo_mapProfileSessionCoordinator.*`、`demo_mapProfileSessionSubsystem.*` 是持久 Profile Session 入口，提供 Initialize、BeginRun、Preparation、Warehouse、Settlement、SpiritStone 和 Town 操作。
- `demo_mapProfileSettlementTransaction.*` 与 `demo_mapItemAuthority::SettleRunItems` 共同完成 Run 结算；下一阶段需要继续验证同一 ItemId 在 Runtime、Profile 和终局快照之间只有一个权威父位置。

### 5.4 UI、输入与世界物品

- `demo_mapEntityLoadoutPresenter.*` 当前已投影 Weapon、Armor、可配置 Accessory、SpatialRing、SpatialItem/Backpack、Base Quick、Ring Quick 和 Spatial Storage 区域。
- `demo_mapInventoryWidget.*` 提供库存格、详情、丢弃／空间 Bundle 确认和 1—9 快捷栏入口。
- `demo_mapSearchContainerWidget.*` 提供左右目标 UI、目标搜索、真实 ItemCell 拖放、右键操作、玩家／目标袋窗口、滚动和关闭入口。
- `demo_mapPlayerController.*` 持有 Profile Preparation、Search、Inventory、Settlement 等输入锁和恢复逻辑，包含 Esc/输入恢复相关入口。
- `demo_mapWorldItem.*` 与 `demo_mapItemSubsystem.*` 负责世界物品 Actor 的创建、拾取、丢弃和空间 Bundle 投影。
- 风险：UI 能力已较丰富，但当前是原 0.0.9-XFix1 方向的实现；0.0.9B P 阶段应基于统一数据模型重新设计 UI，而不是继续堆叠历史 Widget 分支。

## 6. 与 Prompt 的差异

Prompt 要求严格从 0.0.8 稳定镜像开始；本机没有该源目录，因此实际使用现有 `.9-XFix1` 建立副本。这一差异由用户明确要求“以现有的游戏开始”后执行，并已作为非阻塞发现记录。若策划坚持严格版本隔离，需在进入 P1 前补回 0.0.8 源目录并重新建立基线；当前 Report 不替策划做该历史来源裁决。

Prompt 要求 I0 不提前开始 P 开发，本任务遵守了这一边界；没有新增功能代码、地图、敌人、奖励或经济数值。

## 7. 建议的下一份 P1.0.r0

建议只签发一个基础任务：

> **建立 0.0.9B 的统一 Item Instance / Container / Location 数据边界，并用 1×1 物品完成 Move、Swap、Merge、Equip、Unequip、Container Take 和 Profile Save/Load 的不变量测试；UI 先只接入最小快照，不扩充地图内容。**

P1 应先明确：

- 是否把现有 `Fdemo_mapItemAuthority` 适配为 0.0.9B 的唯一事务服务，还是用新命名重写；
- `Fdemo_mapRuntimeContainerAuthority` 与 Item Authority 的父位置关系；
- Profile Schema 6 的保留／迁移策略；
- SpatialRing、SpatialItem/Backpack、Accessory 的槽位语义；
- 快捷栏只引用 Base Quick 真实物品的约束；
- 原 0.0.9-XFix1 中哪些实现可以被正式确认继续使用。

本任务完成后停止，没有自动进入 P1。

