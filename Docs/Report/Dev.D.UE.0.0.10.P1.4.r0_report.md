# Dev.D.UE.0.0.10.P1.4.r0 Report

## 结论

P1.4 已完成并通过验证。0.0.10 物品权威现在拥有一个由 Unreal `UGameInstanceSubsystem` 自动注册、每个 GameInstance 唯一的产品生命周期所有者。Subsystem 构造保持零存储 I/O；显式绑定永远先打开已有 schema-1 authority，只有确认文档不存在后才读取 Code A/Profile 与 Code B 的只读副本，并且只允许两边均处于无活动 Run、身份完全一致的局外稳定态时发布首次迁移。

已有 authority、迁移、恢复、单一存储绑定和 durable 资源命令都由同一 owner 封装。损坏 authority 进入粘性 `RecoveryRequired`，不能被 legacy 回迁覆盖；GameInstance 重启后可以打开同一文档并精确重放已经持久化的 Reserve/Commit 请求。

本轮刻意没有自动触发产品迁移：0.0.9B 的局内 Code B 写路径尚未全部退役，若现在自动迁移，旧图仍可能继续变化并形成双真值。P1.4 先建立安全的唯一 owner、稳定源捕获和显式切换门；下一阶段必须先冻结或适配旧写者，再连接正式 cutover 调用。

`READY_FOR_0_0_10_P1_5_CUTOVER_ADAPTERS`

## 功能性

### GameInstance 唯一 owner

- 新增 `Udemo_mapShanmenItemAuthoritySubsystem`，由 GameInstance subsystem collection 自动创建并销毁。
- Subsystem 私有持有唯一 `FShanmenItemAuthorityService`；不公开 Repository、Store 或可变 Document。
- 生命周期为 `Uninitialized / Unbound / WaitingForStableLegacy / Ready / RecoveryRequired`。
- `Initialize` 只构造内存 owner，不读取、创建或修复任何 authority 文件。
- 第一次有效绑定冻结规范化 Profile 根目录与 OwnerId；同一 GameInstance 不能改绑另一存储或玩家身份。

### existing-first 与显式首次迁移

- `BindExisting` 只尝试打开当前 0.0.10 authority；缺失时返回 `WaitingForStableLegacy`，不读取旧源、不写盘。
- `BindFromStableLegacy` 也先打开已有 authority。只要有效文档存在，即使传入的 legacy 参数损坏或为空，也不会检查旧参数。
- 只有 existing probe 明确返回 Missing 后，结果中的 `bLegacyInputsRead` 才为 true。
- 首次迁移要求请求 Owner、Code A ProfileId、Code B OwnerId 完全一致，并要求 Code A 与 Code B 都没有活动 Run。
- 候选仍由 P1.1 只读 migrator 完整验证，并使用固定产品内容身份：
  - Version：`Shanmen.Items.0.0.10`
  - Digest：`Shanmen.Items.LegacyAuthority.Schema1.v1`
- Subsystem 只授权 migrator 返回的精确非空 `MigrationId`，再交给 P1.3 service 原子发布 generation 1。
- 活动 Run、身份不一致或 legacy 验证失败只保持 `WaitingForStableLegacy`，不生成半成品；修正为稳定来源后可在同一绑定内重试。

### 粘性恢复与单一绑定

- 损坏、未来 schema、无法读取或无法加载 Repository 的 durable authority 进入 `RecoveryRequired`。
- Recovery 状态不再读取 legacy 参数，也不允许回迁、改绑或写命令。
- Ready owner 收到不同根目录或 OwnerId 时返回 `BindingMismatch`，原 authority 与内存状态保持不变。
- existing-first、首次发布竞争、原子存储、备份恢复和多写者代际检查继续复用 P1.2/P1.3 已验证实现。

### 产品 durable 命令入口

- Subsystem 提供 `ReserveDurable / CommitDurable / CancelDurable / ReleaseDeploymentDurable`。
- 命令只在 Ready 且 Game Thread 上进入 P1.3 service；未绑定、等待迁移或恢复状态统一返回 `NotReady`。
- Service 报告 Recovery 时，Subsystem 同步进入粘性 `RecoveryRequired`。
- 读取只能复制完整 `FShanmenItemAuthoritySnapshot` 或 `FShanmenItemAuthorityDocument`。

### 稳定 legacy 捕获适配器

- Profile Session Coordinator/Subsystem 新增只读完整 Profile 捕获；仅在 `ReadyForPreparation`、无活动 Run、当前 Profile 通过 Repository 验证时成功。
- 宗门 Warehouse Service 新增只读 Code B Record 捕获；仅在已打开、Committed、无活动 Run、无局内容器残留时成功。
- 两个接口都返回值副本，不公开旧 Store、Repository 或写能力。
- 新 Subsystem 本身不调用 `OpenOrMigrate`、`SaveProfile` 或任何 Code B commit API。

## 自动化

最终命令目标：`Automation RunTests Shanmen.0_0_10`

P1.4 新增 7 项：

1. `Items.ProductAuthority.LazyGameInstanceOwner`
2. `Items.ProductAuthority.StableLegacyBinding`
3. `Items.ProductAuthority.ExistingWinsBeforeLegacyRead`
4. `Items.ProductAuthority.ActiveRunAndIdentityGates`
5. `Items.ProductAuthority.BindingAndRecoveryFailClosed`
6. `Items.ProductAuthority.DurableCommandsAndRestart`
7. `Items.ProductAuthority.StableSourceCaptureAdapters`

| 范围 | 结果 |
|---|---|
| 既有 P0—P1.3 | 37/37 Success |
| P1.4 ProductAuthority | 7/7 Success |
| 合计 | `44/44 Success`、`0 Fail` |

- P1.4 首轮专项：`7/7 Success`，原生退出码 `0`。
- 最终完整回归：`44/44 Success`，原生退出码 `0`。
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.4.r0_automation.log`
- 最终日志 SHA-256：`7F7F2EA97E6119EB77F8901EBCB0EF63F9215C9E8C66BFD68E7559625D2637BF`
- 首轮专项日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.4.r0_automation.first.log`
- 首轮专项 SHA-256：`0D04137014C964EB29E82CD74E5EB8D2D2596BBD8A188D17628D319166916B5C`
- 两轮均记录 `Automation Test Queue Empty`，没有目标测试失败。
- 测试启动前 13 条 UE 内置 `Condition failed` 与既有基线相同，不属于目标测试。

## 首次失败与修复

本轮没有源码、UHT、链接或自动化首次失败。Editor 首编一次成功，P1.4 专项首轮 7/7 成功，完整回归首轮 44/44 成功。无需把环境警告或 UE 启动基线误记为源码失败。

## 构建与静态检查

### Editor

- 命令：`Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`
- 结果：33/33 actions，`Result: Succeeded`
- 原生退出码：`0`
- 总时间：133.02 秒

### Game

- 命令：`Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`
- 结果：30/30 actions，`Result: Succeeded`
- 原生退出码：`0`
- 总时间：109.99 秒

### 静态

- `git diff --check`：退出码 `0`。
- 新 Subsystem 的 `UWorld / AActor / UUserWidget / ApplyDamage / UGameplayAbility / FMath::Rand / FRandomStream` 扫描：0 匹配。
- 新 Subsystem 的 `OpenOrMigrate / SaveProfile / CommitAccepted` 扫描：0 匹配。
- ProductAuthority 调用扫描：当前只有 Subsystem 定义与自动化；不存在未审核的产品自动迁移调用。

## 修改范围

- `Source/demo_map/demo_mapShanmenItemAuthoritySubsystem.h`
- `Source/demo_map/demo_mapShanmenItemAuthoritySubsystem.cpp`
- `Source/demo_map/demo_mapShanmenItemAuthoritySubsystemTests.cpp`
- `Source/demo_map/demo_mapProfileSessionCoordinator.h/.cpp`
- `Source/demo_map/demo_mapProfileSessionSubsystem.h/.cpp`
- `Source/demo_map/demo_map0909BSectWarehouseService.h/.cpp`
- 本 Report 与同名 Log

工作区原有未跟踪 Prompt、旧 Report、自动化文档及用户文件没有被修改、暂存或提交；Saved 自动化日志不进入 Git。

## 未包含与下一步

- 本轮注册了产品级唯一 owner，但没有在当前 0.0.9B Framework、GameMode、UI、Actor 或 Run 生命周期中自动调用绑定/迁移。
- 旧 Code A/Profile 与 Code B 继续按 0.0.9B 产品路径运行；P1.4 没有双写，也没有谎称它们已经退役。
- 尚未把仓库整理、出战携带、飞剑/暗器/阵材消耗、拾取、结算或局内装备移动改接新 authority。
- 尚未提供玩家迁移提示、恢复 UI、云存档、网络复制或跨进程文件锁。
- P1.5 应先列出并冻结所有会在首次迁移后继续修改 Code A/Code B 的产品写者，再为它们建立单向 Adapter；确认旧源不再变化后，由一个 cutover coordinator 捕获两个稳定副本并调用 `BindFromStableLegacy`。在此之前禁止产品自动迁移。

## P/F 边界

本轮属于 P 阶段开发与无头验证。执行了静态审查、`git diff --check`、Editor/Game Development 构建及 `UnrealEditor-Cmd -unattended -NullRHI` 自动化；未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、大规模回归、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p1-4-product-lifecycle-binding/Docs/Report/Dev.D.UE.0.0.10.P1.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p1-4-product-lifecycle-binding/Docs/Log/Dev.D.UE.0.0.10.P1.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p1-4-product-lifecycle-binding>
