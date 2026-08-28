# Dev.D.UE.0.0.10.P1.8.r0 Report

## 结论

P1.8 原子战备提交内核已完成，结论为 **PASS**。

本阶段在唯一物品权威 `ShanmenItems` 内建立了有序、多 Reservation、全成或全败的 `CommitBatch`。装备 `DeploymentLock` 与材料/消耗品完整 Stack 的 `Quantity` reservation 会先在同一候选快照中完整验证，再共享一个 `AuthorityRevision` 并只执行一次 durable 文档写入；任一行无效或写盘失败时，不会出现部分装备部署、部分物品消耗或内存/磁盘分叉。

成功批次生成可从 authority ledger 在进程重启后重建的 immutable prepared-loadout receipt，包含五类装备、RunInventory 顺序、九格 Hotbar 和逐行资源凭据。退休 Profile 文件不参与提交，也不接收物品投影回写。

## 功能性

### 原子批次事务

- 新增 `FShanmenItemReservationBatchRequest`，其有序 `ReservationIds` 是命令指纹的一部分。
- `FShanmenItemRepository::CommitBatch` 在任何修改前验证：
  - Request/Content/Owner/Scope；
  - Reservation 存在且唯一；
  - 每行仍处于 `Reserved`；
  - 每行资源、Item 和 Definition 一致；
  - Quantity、DeploymentLock、Durability、Charges 的既有提交规则。
- 所有行在同一 candidate snapshot 上应用；成功只增加一次 authority revision。
- aggregate receipt 保留原始行顺序；同一 RequestId 的精确重试返回同一 receipt，重排行顺序会明确 `RequestIdConflict`。
- durable service 继续使用既有单命令保存协议：一次 batch 只产生一次文档 generation。
- `WriteTemp` 故障会重新打开并安装精确 pre-command 文档，内存与磁盘整批回滚；解除故障后可自主重试。

### Prepared loadout receipt

- 新增 `Fdemo_mapShanmenPreparedLoadoutReceipt` 及逐行 `Fdemo_mapShanmenPreparedLoadoutLine`。
- receipt 固化：
  - Batch RequestId / ReceiptId；
  - OwnerId / ScopeId / AuthorityRevision；
  - Weapon、Armor、Accessory、SpatialRing、Backpack；
  - RunInventory 的稳定顺序；
  - 固定九格 Hotbar；
  - 每行 ReservationId、ItemInstanceId、DefinitionId、ResourceKind、Amount、PurposeId。
- 迁移基线装备在提交前先物化为显式 pending `DeploymentLock`，随后和全部 Quantity intents 一起进入一个 `CommitBatch`。
- 成功后不依赖进程内缓存；重启只读取 ShanmenItems processed-request ledger 与 reservation/item snapshot 即可重建同一 receipt。
- 已存在未消费的有效 prepared receipt 时返回 `NoChange`，不会重复开发、重复消耗或重复写盘。

### Hotbar 原子元数据修改

- 新增 `AmendReservationPurpose` authority 命令，替代 P1.7 的 Cancel → Reserve → 补偿流程。
- 修改发生在原 Reservation 上，ReservationId、占用资源、Item Revision 和可用数量保持不变。
- Request 携带 `ExpectedPurposeId` compare-and-swap；过期调用明确失败，不能覆盖更新后的绑定。
- 命令具有确定性 fingerprint、processed-request ledger、durable 单写、故障回滚与重启重放语义。
- Hotbar 跨槽移动不再存在“旧完整 Stack reservation 已取消、新 reservation 尚未建立”导致选择丢失的进程中断窗口。

## 完整性与兼容性

- `Udemo_mapShanmenItemAuthoritySubsystem` 仍是唯一可变物品权威。
- 未修改 Profile schema 7，也未写入 Profile `PreparationLayout`、`PermanentStash`、`ActiveRun` 或 Code B。
- ShanmenItems 文档 schema 仍为 1；receipt 新字段为 additive，既有非 batch ledger 继续按原语义校验。
- 单 Reservation `Commit`、`Cancel`、`ReleaseDeployment` 保持兼容，并在新 receipt 中保留有效 Purpose。
- P1.6 装备选择、P1.7 Quantity/Hotbar、migration、cutover fence、persistence recovery 和 P0 combat tests 全部回归通过。
- 未创建第二份 loadout JSON、SaveGame、Profile 镜像或 UI 本地真值。

## 修改范围

### 修改

- `Source/ShanmenItems/Public/ShanmenItemTypes.h`
- `Source/ShanmenItems/Private/ShanmenItemTypes.cpp`
- `Source/ShanmenItems/Public/ShanmenItemRepository.h`
- `Source/ShanmenItems/Private/ShanmenItemRepository.cpp`
- `Source/ShanmenItems/Public/ShanmenItemAuthorityService.h`
- `Source/ShanmenItems/Private/ShanmenItemAuthorityService.cpp`
- `Source/ShanmenItems/Private/Tests/ShanmenItemsTests.cpp`
- `Source/ShanmenItems/Private/Tests/ShanmenItemAuthorityServiceTests.cpp`
- `Source/demo_map/demo_mapShanmenItemAuthoritySubsystem.h`
- `Source/demo_map/demo_mapShanmenItemAuthoritySubsystem.cpp`
- `Source/demo_map/demo_mapShanmenPreparationAdapter.h`
- `Source/demo_map/demo_mapShanmenPreparationAdapter.cpp`
- `Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp`

### 新增文档

- `Docs/Report/Dev.D.UE.0.0.10.P1.8.r0_report.md`
- `Docs/Log/Dev.D.UE.0.0.10.P1.8.r0_log.md`

## 自动化

### P1.8 定向测试

- 命令：`Automation RunTests Shanmen.0_0_10.Items.AtomicReservationPurposeAmend+Shanmen.0_0_10.Items.PreparationAdapter.QuantityCapacityAndFailureFence+Shanmen.0_0_10.Items.PreparationAdapter.AtomicPreparedLoadout+Shanmen.0_0_10.Items.AuthorityService.AtomicBatchDurability`
- 结果：`4/4 Success`、0 Fail、`Automation Test Queue Empty`
- 覆盖：Purpose CAS/重放、Hotbar 原 Reservation 身份、metadata 写盘回滚、batch 单次 durable 写入、完整 loadout receipt 和重启恢复。

### 完整 0.0.10 回归

- 命令：`Automation RunTests Shanmen.0_0_10`
- 结果：`57/57 Success`、0 Fail、`Automation Test Queue Empty`
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.8.r0_automation.log`
- SHA-256：`28409DFF7F3D2C5EBDB47238E5B546FFE35DEB46ECC4829D384F478AE9FFB18C`
- 原生进程退出码：`0`

## 构建

### Editor

- 命令：`Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`
- 最终构建：24/24 actions，`Result: Succeeded`，退出码 `0`，75.81 秒。

### Game

- 命令：`Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`
- 最终构建：21/21 actions，`Result: Succeeded`，退出码 `0`，87.90 秒。

## 静态审查

- `git diff --check`：原生退出码 `0`。
- ShanmenItems 与 Preparation Adapter 中 `UWorld`、`AActor`、`ApplyDamage`、`UGameplayStatics`、随机数、UI、PIE、Standalone 和产品启动 API：0 匹配。
- Preparation Adapter 中 Profile/Code B 持久化写 API：0 匹配。
- 未跟踪的历史 Prompt、Report、PDF 和自动化交接文档未纳入本阶段提交。

## P/F 边界

本阶段执行了代码开发、静态审查、无头自动化、Editor Development 构建和 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook、Package 或产品级大规模回归。

## 已知边界与下一阶段

- 产品 `StartPreparedRun` 和 `bCanStartRun` 仍保持关闭。旧 Profile `BeginRun` 会写 item-bearing `ActiveRun`，而 P1.5 cutover fence 正确拒绝该回写；本阶段没有为追求“按钮可用”而重新打开双权威。
- 当前 receipt 表达非空 prepared loadout；完全空载的显式启动标记留给后续运行期入口。
- P1.9 应让 Runtime/Settlement 直接消费 ShanmenItems prepared receipt，并增加 terminal consumption/finalization marker，使已结算的旧 batch 不再被识别为 pending loadout。
- 迁移基线装备的 pending lock 在 batch 前物化；batch 失败不会部署或消费任何资源，但该无害选择 intent 会保留以供重试。
- Warehouse Move/Swap/Merge 仍应实现独立 repository transaction，不复用 preparation batch。
