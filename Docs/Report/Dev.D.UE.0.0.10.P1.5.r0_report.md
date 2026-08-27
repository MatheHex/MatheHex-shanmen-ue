# Dev.D.UE.0.0.10.P1.5.r0 Report

## 结论

P1.5 已完成并通过验证。0.0.10 物品权威现在具备一条显式、existing-first、失败关闭的产品切换路径；一旦新 authority 的主文档或备份文档出现，0.0.9B 的 Profile 物品字段与 Code B 持久化写者会在各自唯一磁盘提交点被永久围栏，不能继续形成双真值，也不能在新 authority 损坏后用旧数据静默重建。

切换协调器只在新 authority 明确缺失时读取稳定的 Code A/Profile 与 Code B 副本。已有 authority 永远优先，损坏 authority 进入 Recovery，旧源保持只读。Profile 中与物品无关的货币/进度仍可持久化，但物品、商店库存、配装、仓库布局及局内物品投影不得改变；Code B 的全部持久化写入统一在 `SaveRecord` 前拒绝。

本轮没有自动挂接 Framework/GameMode 启动点。当前产品仍不会自行切换 authority；这是有意保留的安全边界。下一阶段应先把仓库、携带、消耗、拾取与结算的产品调用适配到新 authority，再启用唯一自动 cutover 调用。

`READY_FOR_0_0_10_P1_6_PRODUCT_ADAPTERS`

## 功能性

### durable cutover marker

- 新增 `Fdemo_mapShanmenLegacyItemWriteFence`。
- `FShanmenItemAuthorityDocument` 的 Owner 专属主文档或备份文档本身就是持久切换标记，不另建可能失同步的布尔状态文件。
- 只要主文档或备份存在，旧写者即视为退休；即使字节损坏，也保持关闭并等待新 authority 恢复。
- 根目录为空或 OwnerId 无效时返回 `InvalidIdentity`，不会错误地开放或声明已切换。

### Profile 中央写入围栏

- `Fdemo_mapProfileRepository::SaveProfile` 在 Profile 结构校验后、任何临时文件 I/O 前检查围栏。
- 围栏关闭时，从 Owner 匹配且可验证的 Profile 主文档或备份文档读取基线。
- 以下物品承载字段必须与基线完全一致：
  - `ProfileId`
  - `PermanentStash`
  - `ShopStock`
  - `PreparationLayout`
  - `WarehouseLayout`
  - Active Run 身份、状态、部署物品、运行时物品与奖励来源
- 不改变上述字段的灵石、宗门进度等 Profile 写入继续允许，避免切换物品权威时冻结无关系统。
- 找不到有效 Owner 匹配基线时失败关闭，不写临时文件、不覆盖磁盘。

### Code B 中央写入围栏

- `FCodeBOutOfRaidProfileStore::SaveRecord` 在验证和临时文件 I/O 前检查同一 durable marker。
- 全部 Code B commit 路径最终收敛到这一私有提交点，因此不需要逐个修改 UI、Actor 或事务调用者。
- 切换后所有 Code B 持久化改写统一拒绝，并返回精确 `retired` 诊断。

### existing-first 切换协调器

- 新增 `Fdemo_mapShanmenItemCutoverCoordinator::Execute`。
- 要求 Game Thread、有效存储根和有效 OwnerId。
- 第一步始终调用 `Udemo_mapShanmenItemAuthoritySubsystem::BindExisting`。
- Opened/Recovered/AlreadyReady 路径不读取 Profile 或 Code B。
- 只有 `WaitingForStableLegacy` 才捕获两份只读稳定源，并再次由 P1.4 `BindFromStableLegacy` existing-first 发布。
- ProfileId、Code B OwnerId 与请求 OwnerId 必须完全一致。
- 结果记录是否读取两份旧源以及来源 generation/revision，便于审计重放。
- 只有 durable fence 确认关闭后才返回 Ready；否则返回 `SafetyFenceFailure`。

### 产品状态投影

- Profile Session Coordinator/Subsystem 新增动态 `AreLegacyItemWritesRetired` 查询。
- Warehouse Service 保存绑定的根目录与 OwnerId，并公开同一查询。
- 仓库 presentation 在切换后保持可读、`bCanWrite=false`，向 UI 提供围栏诊断。
- Profile 初始化中的 ShopStock 补建在新 authority 已存在时跳过旧物品写入，保留现有只读投影。

## 完整性与兼容性

- 新切换逻辑复用 P1.1—P1.4 的迁移验证、原子存储、备份恢复、持久命令与 GameInstance 唯一 owner；没有建立第二套 Repository 或迁移格式。
- 新 authority 缺失时，旧 Profile 与 Code B 保持原行为，可继续作为迁移来源。
- 新 authority 已存在时，已有旧文档仍可读取用于界面和非物品进度，但不再能更改物品真值。
- 新 authority 损坏时不会回退到 legacy，也不会因恢复流程重新开放旧写者。
- 本轮没有修改现有 0.0.9B 数据 schema、没有删除旧数据，也没有自动执行产品切换。

## 自动化

新增 4 项：

1. `Items.Cutover.StablePublishRetiresLegacyWriters`
2. `Items.Cutover.MissingSourceDoesNotRetire`
3. `Items.Cutover.CorruptAuthorityStillRetiresLegacy`
4. `Items.Cutover.RestartSourcesAreReadOnly`

覆盖内容：稳定两源首次发布、Profile/Code B 中央拒写、非物品 Profile 进度继续写入、已有 authority 不重读旧源、缺失来源不误关围栏、损坏 authority 恢复失败关闭、GameInstance 重启后旧界面只读。

| 范围 | 结果 |
|---|---|
| P1.5 专项最终轮 | `4/4 Success`、`0 Fail` |
| 既有 P0—P1.4 | `44/44 Success` |
| 最终完整回归 | `48/48 Success`、`0 Fail` |

- 专项最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.5.r0_automation.corrected.log`
- 专项最终 SHA-256：`1838F8DDCF08BC9C25DD53A32FC275DBA1372F4877518AEFA55AD058C59DFD24`
- 完整回归日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.5.r0_automation.log`
- 完整回归 SHA-256：`40387A5E6B9151B1450E9FE4A302ECD79AA4C58AE0A49168405ED70C10025206`
- 两个最终日志均记录 `Automation Test Queue Empty`；完整回归共执行 48 项。
- UE 启动阶段既有 13 条 `Condition failed` 与前序基线一致，不属于目标测试失败。

## 首次失败与修正

专项测试经历两次测试夹具修正，均如实保留日志：

1. 首轮 `3/4 Success`：测试用超出定义上限的 stack count 构造“物品变化”，被既有 Profile 校验先行拒绝，未命中新围栏诊断。生产逻辑未失败。
2. 第二轮 `2/4 Success`：测试改用仓库格换位，但新建 Profile 的 `WarehouseLayout` 尚未初始化；两个测试在夹具前置条件处失败。生产逻辑未失败。
3. 最终改用新档必有的 `TrainingBlade` 进行合法 Weapon 配装变化，确保请求先通过既有 Profile 校验，再由退休围栏拒绝；专项 `4/4`、完整回归 `48/48`。

历史日志：

- `Dev.D.UE.0.0.10.P1.5.r0_automation.first.log`：SHA-256 `5FAE5B4F352F3B8CB7602E7F498F9DCABE2BA597CA29EDD283C7835D5B0E6414`
- `Dev.D.UE.0.0.10.P1.5.r0_automation.fixed.log`：SHA-256 `64EF4B26AF2EBB0336B860CF87F24950BCC3D6FA4238D952F1C7E5A4F4DD8D77`

`UnrealEditor-Cmd` 的 `TestExit` 在目标测试失败时仍返回原生退出码 0，因此本 Report 以逐项 `Result={Success|Fail}` 和队列汇总为准，没有把进程退出码伪装成测试成功。

## 构建与静态检查

### Editor

- 首次命令：`Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`
- 首次结果：35/35 actions，`Result: Succeeded`，原生退出码 `0`，147.37 秒。
- 测试夹具最终修正后的增量结果：4/4 actions，`Result: Succeeded`，原生退出码 `0`，5.34 秒。

### Game

- 命令：`Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`
- 结果：34/34 actions，`Result: Succeeded`，原生退出码 `0`，122.05 秒。

### 静态

- `git diff --check`：原生退出码 `0`。
- 新 Cutover/Fence 文件的 `UWorld / AActor / UUserWidget / ApplyDamage / UGameplayAbility / FMath::Rand / FRandomStream` 扫描：0 匹配。
- Code B 持久化写调用收敛于唯一 `SaveRecord` 定义与内部调用。
- Profile 物品事务均通过 `Fdemo_mapProfileRepository::SaveProfile` 持久化；围栏位于 Repository 中央入口。
- `Fdemo_mapShanmenItemCutoverCoordinator::Execute` 当前只有定义与自动化调用，产品自动切换调用为 0。

## 修改范围

- `Source/demo_map/demo_mapShanmenLegacyItemWriteFence.h/.cpp`
- `Source/demo_map/demo_mapShanmenItemCutover.h/.cpp`
- `Source/demo_map/demo_mapShanmenItemCutoverTests.cpp`
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
- `Source/demo_map/demo_mapProfileRepository.cpp`
- `Source/demo_map/demo_mapProfileSessionCoordinator.h/.cpp`
- `Source/demo_map/demo_mapProfileSessionSubsystem.h/.cpp`
- `Source/demo_map/demo_map0909BSectWarehouseService.h/.cpp`
- 本 Report 与同名 Log

## P/F 边界

- 本轮属于 P 阶段：代码审查、静态检查、无头自动化以及一次必要的 Editor/Game 构建。
- 未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。
- 未执行真实输入、截图、Smoke、大规模产品回归、Cook 或 Package。
- `Saved/Logs` 原始自动化日志不提交仓库；Report/Log 记录命令、逐项结果与 SHA-256。

## 下一阶段

P1.6 应按真实产品数据流逐一接入新 authority：宗门仓库/携带准备、局内消耗与主动触发、世界拾取/掉落、撤离/死亡结算。每个 Adapter 必须单向调用 GameInstance authority owner，不双写 Profile 或 Code B；完成这些调用替换并验证后，才允许在唯一产品启动点调用本轮 cutover coordinator。
