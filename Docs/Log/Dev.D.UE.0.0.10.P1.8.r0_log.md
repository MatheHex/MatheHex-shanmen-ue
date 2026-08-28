# Dev.D.UE.0.0.10.P1.8.r0 Development Log

## 元信息

- 日期：2026-08-27
- 分支：`agent/0.0.10-p1-8-atomic-start-run`
- 起点：`58c67aaa0896afabbe94921d3424de8e346f61d0`
- 阶段：0.0.10 P1.8 原子战备提交内核

## 输入审查

P1.7 已建立 authority-native 装备、RunInventory 与 Hotbar，但尚有两个结构性断点：

1. Repository 只能逐条 Commit，无法保证五类装备和全部材料/消耗品全成或全败；
2. Hotbar Purpose 变更采用 Cancel → Reserve，进程可能在两步之间中断并丢失完整 Stack 的选择 intent。

同时审计确认，现有产品 `StartPreparedRun()` 仍调用旧 Profile `BeginRun`。P1.5 cutover 后，Profile write fence 会拒绝任何物品相关 `ActiveRun` 投影；如果把新 authority 结果再次写回旧 BeginRun，会重新制造双权威。因此本轮先完成 ShanmenItems 原子提交和持久 receipt，继续关闭旧产品入口。

## 设计决策

### 原子性下沉到唯一物品权威

Adapter 不执行逐条 Commit 和补偿。`FShanmenItemRepository::CommitBatch` 接收有序 ReservationId 列表，并在 candidate snapshot 上完成：

1. 命令与 Content Stamp 验证；
2. 重复行、Owner、Scope 和状态验证；
3. 每一 Reservation 的既有资源提交验证；
4. 全部行应用；
5. 单次 `AuthorityRevision` 发布；
6. 单个 aggregate processed-request receipt 记录。

任何预检或应用失败都不会发布 candidate。Request fingerprint 包含列表长度和每一行的原始顺序，因此重排不是同一命令。

### durable 层只保存一次

`CommitBatchDurable` 复用 authority service 的 `ExecuteCommandLocked`：Repository 返回完整 candidate 后，Store 只调用一次 `SaveAuthority`。如果写入失败：

- reopen 等于 pre-command 文档：安装 pre-command 状态并返回 rolled back；
- reopen 等于 post-command 文档：证明命令已落盘并恢复 post-command；
- 两者都不匹配：进入 RecoveryRequired，禁止猜测成功。

这使“一个 authority revision”和“一个磁盘 generation”保持一致。

### receipt 来自 ledger，不来自临时缓存

Prepared loadout receipt 不另存 JSON。它由 aggregate receipt 的有序 ReservationIds 联合 authority 中的 Reservation、Item、Definition 重建。重建时再次验证：

- 每行已 Committed；
- equipment Purpose 映射到唯一的五类槽位；
- RunInventory Purpose 可解析且 order/Hotbar 唯一；
- Quantity 行使用完整 Stack reservation 语义；
- unsupported resource kind 失败关闭。

因此进程重启后得到的 loadout 与原提交一致，且 Profile 不是恢复输入。

### Purpose amend 使用同一 Reservation

新增 `FShanmenItemReservationAmendRequest`：

- `ReservationId` 指向仍 pending 的 intent；
- `ExpectedPurposeId` 是 compare-and-swap guard；
- `PurposeId` 是规范目标值；
- fingerprint 同时覆盖 expected 和 target；
- 成功只修改 Reservation Purpose，不改资源、Item 或 Reservation 身份。

Adapter 的 Hotbar 移动现在调用一次 durable amend。写盘失败由 authority service 恢复整个命令前文档，不再需要取消和重建 Stack reservation。

## 实现记录

### ShanmenItems

- 增加 `CommitBatch` 与 `AmendReservationPurpose` operation。
- aggregate receipt 增加有序 `ReservationIds`。
- singular receipt 增加有效 `PurposeId`。
- 抽取 `ApplyCommit`，让单 Commit 与 batch 共用同一资源语义。
- invariant validation 识别 aggregate terminal ledger 与非 terminal Purpose amend ledger。
- repository、authority service 和 GameInstance subsystem 均提供对应 API。

### Preparation Adapter

- 增加 prepared loadout line/receipt/result 类型。
- 对迁移装备基线物化 pending `DeploymentLock`。
- 以固定顺序收集装备和 RunInventory ReservationIds。
- 一次 durable batch 提交后，从 ledger 重建 receipt。
- 最新有效 committed batch 可在重启后恢复并幂等返回。
- Hotbar metadata 替换改为原 Reservation 的 CAS Purpose amend。

### 保持关闭的路径

- `bCanStartRun` 保持 false。
- 不调用旧 Profile `BeginRun`。
- 不写 Profile `ActiveRun`、PreparationLayout 或 Code B。
- 不启动 World、Actor、UI、PIE、Standalone 或产品可执行文件。

## 测试记录

### 新增/扩展测试

- `AtomicBatchCommit`
  - 缺失批次行整批拒绝；
  - quantity 和 equipment 无半提交；
  - 单 authority revision；
  - 精确重试与重启重放；
  - 同 RequestId 重排行冲突。
- `AtomicReservationPurposeAmend`
  - ReservationId 和资源占用不变；
  - CAS 拒绝 stale Purpose；
  - RequestId 冲突；
  - amend 后仍可 batch commit；
  - terminal 后重启仍可重放原 receipt。
- `AuthorityService.AtomicBatchDurability`
  - `WriteTemp` 故障下内存/磁盘精确回滚；
  - 重试只增加一个 generation/revision；
  - 重启精确重放 aggregate receipt。
- `PreparationAdapter.AtomicPreparedLoadout`
  - 迁移基线显式 lock；
  - equipment + 两个完整 Stack + Hotbar；
  - batch 写失败无部分部署/消费；
  - receipt 重启重建；
  - retired Profile 字节不变。
- `PreparationAdapter.QuantityCapacityAndFailureFence`
  - Hotbar 移动保持同一 ReservationId；
  - 写失败回到完整 pre-amend snapshot；
  - retry 只增加一次 authority revision。

### 定向回归

- 结果：4/4 Success、0 Fail、队列正常清空。

### 完整回归

- 结果：57/57 Success、0 Fail、队列正常清空。
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.8.r0_automation.log`
- SHA-256：`28409DFF7F3D2C5EBDB47238E5B546FFE35DEB46ECC4829D384F478AE9FFB18C`
- 原生退出码：0。

## 构建记录

- Editor：24/24 actions，成功，退出码 0，75.81 秒。
- Game：21/21 actions，成功，退出码 0，87.90 秒。

全部构建使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`。

## 静态检查

- `git diff --check`：0。
- ShanmenItems/Preparation Adapter 的 World、Actor、伤害、随机数、UI 和产品启动依赖：0。
- Preparation Adapter 的 Profile/Code B 持久化写 API：0。
- 历史未跟踪 Prompt、Report、PDF 和自动化文档保持原状，未纳入阶段提交。

## 后续入口

P1.9 让 Runtime 与 Settlement 直接消费 prepared loadout receipt，不经过 Profile item-bearing `ActiveRun`。同时增加 terminal consumption/finalization marker，定义 Quantity 结算、DeploymentLock release 和重启恢复的对称闭环；完成该闭环后再开放唯一产品 `StartPreparedRun` 入口。
