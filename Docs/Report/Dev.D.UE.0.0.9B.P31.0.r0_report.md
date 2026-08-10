# Dev.D.UE.0.0.9B.P31.0.r0 Report

## 1. 结果

- 任务：`Dev.D.UE.0.0.9B.P31.0.r0`。
- 结果：P14/P19 的活动地面状态已收敛为 P6 durable、Owner/Run scoped、canonical ordered 的多 record Registry；每个 record 仍严格只有一个 root、一个 derived world container 与一个 Actor projection。
- 旧 schema 4/5 的合法单 record 采用同一对象无损提升为 Registry 唯一成员；不会生成或替换 `WorldDropId`、`ContainerId`、`ItemId`、child container、位置或数量。
- simple whole root、P26 `Split(N)` root、P19 complete graph 均沿原 `GroundDropZone` 唯一入口创建新的独立 record。已有 record 的持久化记录和完整 P1 closure 由结构比较证明不变。
- P26—P30 pickup/QuickTransfer 继续只作用于 exact opened record；成功整根拾回只删除该 record 与空 world container，部分拾回保留该 record，其他 Registry 成员不变。
- Editor 与 Game 编译均成功，native exit code 均为 `0`。
- P 阶段未启动产品，未执行 PIE、Standalone、Smoke、Automation、回归、Cook、Package、真实输入或截图巡检。

## 2. 活动调用链与旧 singleton 审计

### 2.1 durable 数据与保存链

审阅链路：

1. `FCodeBRunInventorySession` 是精确 Owner + active Run 的唯一 P6 durable record；`RepositorySnapshot` 仍是唯一可变物品图。
2. P14 旧字段 `WorldDrops` 虽为数组形状，但产品有效边界为最多一个 active record；P31 将其定义为 canonical Registry，不增加第二 Repository、第二 JSON 或 Actor/UI mirror。
3. `DropMatchedActiveRunWorldDropItem` 使用短生命周期 `FCodeBRepository` 加载当前 P6 snapshot，执行一条 P1 `Move`/`Split`/`Unequip`，在 `FCodeBOutOfRaidInventoryRecord Candidate` 内追加 record、推进一次 `NextWorldDropOrdinal`，完成 P13 reconcile、receipt freeze、全 session validation 后只调用一次 `SaveRecord(Candidate)`；保存失败时 `Store.Record` 未替换。
4. `CommitAcceptedMatchedRunWorldDropPickup` 重新打开精确 Owner/Run，按 exact `WorldDropId` 查 record，复核 `Ordinal`、record revision、world container、root、P6 revision 与 P19 closure；其后证明 accepted P2/P1 delta，只修改 Candidate 并只保存一次。
5. `BuildP14PlayerOnlySession` 枚举全部 active Registry records，逐条调用 `DiscardP19WorldDropClosure` 排除 root、derived container 及正式 P19 child closure，最后清空 Registry；P8 terminal receipt 因而不会把任何地面图泄漏回 P5。

### 2.2 Drop 调用图

`UCodeBP3GroundDropZone::NativeOnDrop`
→ `UCodeBP3InventoryWidget::HandleGroundDropZoneDrop`
→ `UCodeBP3UIHostSubsystem::RequestGroundDrop`
→ `Ademo_mapV3ProgressionManager::RequestCodeBGroundDrop`
→ `FCodeBOutOfRaidProfileStore::DropMatchedActiveRunWorldDropItem`
→ 一个 P1 transaction
→ 一个 Owner candidate / 一个 `SaveRecord`
→ committed projection
→ `RefreshCodeBWorldDropActors`。

没有新增按钮、快捷键、Actor direct pickup、右键、双击、quick-drop 或 Widget direct write。

### 2.3 exact opened-record pickup 调用图

`Ademo_mapCodeBWorldDropActor::RequestInteract`
→ `OpenCodeBWorldDropPage` 按 Actor 的 exact `WorldDropId` 查 Registry
→ P3/P4 双栏只投影该 record 的 slot 0 root
→ P3 → P2 → P1 accepted candidate
→ `CommitAcceptedMatchedRunWorldDropPickup(OwnerId, RunId, WorldDropId, Ordinal, RecordRevision, P6Revision, ...)`
→ P26/P27/P28/P29/P30 既有 delta proof
→ 删除或保留 exact record
→ 一个 Owner save
→ Registry-diff Actor refresh。

所有交互查找均以 exact `WorldDropId` 为主键；不存在按列表 first/last、当前 Cell、Actor pointer、坐标、显示名、图标或文本推断写入目标的路径。

## 3. 实际修改文件及职责

### 3.1 durable Store / Registry

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`
  - P6 schema 从 5 升至 6。
  - `FCodeBWorldDropRecord` 增加精确 `OwnerId`、`RunInstanceId`、`Ordinal`、`SpatialChildContainerId`、record revision 与 provenance。
  - projection 增加 Ordinal、child identity 与 record revision。
  - pickup durable callback 增加 expected Ordinal 与 expected record revision 参数。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
  - schema 6 JSON 读写、旧 singleton 无损提升、canonical order。
  - Registry 全量 invariant、single-root/single-container/closure 校验。
  - existing-record graph unchanged proof。
  - accepted create 的独立 record insertion 与 accepted-only ordinal advance。
  - exact pickup 的单 record remove/retain 与 other-record isolation proof。
  - P8 全 Registry ground graph exclusion 保持。

### 3.2 transient P3/P4 identity

- `Source/demo_map/CodeB/demo_mapCodeBP3.h`
  - WorldPickup quantity draft 增加 Ordinal、record revision、target-open generation 与 map route。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`
  - Drag payload 增加同一组 transient exact-record identity；不持有 item/quantity authority。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`
  - `FCodeBP3WorldDropPresentation` 从泛化单目标说明收敛为一个 Registry 成员的 exact projection。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
  - 普通 Drag、数量 Drag 与 Ctrl QuickTransfer 都携带并在 Preview/Commit 前复核 exact Owner/Run/WorldDropId/Ordinal/container/root/record revision/open generation/route/P6 revision。
  - 打开页面仍只显示一个 `WorldDropTarget` Cell；没有 Registry 列表、child list 或第二 Cell。

### 3.3 Actor projection / lifecycle

- `Source/demo_map/demo_mapCodeBWorldDropActor.h`
- `Source/demo_map/demo_mapCodeBWorldDropActor.cpp`
  - Actor 只保存 projection identity：Owner、Run、WorldDropId、Ordinal、container、root、formal child、route 与 record revision。
  - Actor 未取得 Repository、数量写入、record 创建、拾取或终局权威。
- `Source/demo_map/demo_mapV3ProgressionManager.h`
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`
  - active opened target 增加 exact Registry identity 与单调 target-open generation。
  - Actor projection 按 `TMap<WorldDropId, Actor>` diff；只为当前 map route 的 Available records 建立投影。
  - stale Actor EndPlay 仅在它仍是同一 map entry 时移除映射，不能误删替代 projection。
  - open/write gate/commit callback 均复核 exact record；不再以 `NextWorldDropOrdinal` 作为当前 target 身份。

### 3.4 明确未修改

- 未修改 `demo_mapCodeBInventory.{h,cpp}`：P1 Repository、ItemId/ContainerId、transaction 与唯一 parent 规则不变。
- 未修改 P2 command/projection 核心与 P3 controller 实现；只扩展 transient identity carrier。
- 未修改 P5/P6 bridge、P8 receipt 分类、P9/P10、P11/P12、P13/P15、P16、P17、P18、P20/P21、Code A inventory、战斗、地图产品逻辑、经济、制作、网络或多人。
- 未新建项目、Fix、Repository、Widget inventory、world multi-item container、fixture 或第二持久化文件。

## 4. 旧单 record → Registry 无损迁移

读取 schema 4/5 时：

1. 继续按旧字段读取原 `WorldDropId`、world container、root、route、floor transform、state 与 `NextWorldDropOrdinal`。
2. 合法旧状态只允许零或一个 active record；空状态迁移为空 Registry。
3. 单 record 的 Ordinal 使用旧 `NextWorldDropOrdinal - 1` 恢复，并用既有 deterministic `WorldDropGuid(Owner, Run, Ordinal)` 反向验证它与原 `WorldDropId` 完全一致；不创建新身份。
4. Owner/Run 从同一 P6 session 填入；formal child identity直接取既有 root 的 `ChildContainerId`；record revision/provenance 是 schema 6 的兼容元数据，不替换旧 P6 snapshot revision 或物品图。
5. Registry 按 Ordinal 升序、`WorldDropId` 稳定文本打破并列；排序只用于序列化、投影与静态比较，交互从不按顺序选择目标。
6. 下一次正常 durable save 写 schema 6；没有 singleton/Registry 双写。

## 5. 独立 record 创建与 existing-record 不变证据

- simple whole：沿原 P1 Move/Unequip，将同一 root 放入新 derived world container slot 0。
- P26 partial：沿原精确 P1 `Split(N)`，只接受 transaction 返回的新 root；原玩家堆保留非零数量。
- P19 graph：只接受 `WindTalisman`/`BackpackLevel1` canonical parent；parent、stable child container、child slots、contents、provenance 与一层无环 closure 保持。
- 每次 create 使用当前 `NextWorldDropOrdinal` 派生新 `WorldDropId` 和 world container；duplicate ID/container 立即拒绝。
- P1 accepted snapshot 后，代码逐条比较所有已有 record 的 world container、root、formal child container 及 child items；任一变化都会在 durable Candidate 建立前拒绝。
- 新 record 追加后才在 Candidate 内推进一次 ordinal；validation 或 `SaveRecord` 失败不会替换 Store，因而没有 durable/in-memory ordinal gap、孤儿 container、phantom record 或 Actor-first write。

## 6. 每 record single-root 边界

Registry validator 对每个成员验证：

- exact Owner/Run、deterministic WorldDropId/Ordinal、unique ID/Ordinal/container/root；
- derived world container 必须是 `WorldDrop` storage、capacity 1、slot 0 恰好为该 root；
- root 的 parent/slot 必须反向匹配该 container/slot 0；
- simple root 的 `SpatialChildContainerId` 必须为空；P19 root 必须与正式 child identity 完全一致；
- record revision、provenance、route、transform 与 Available state 有效；
- Ordinal 必须小于 `NextWorldDropOrdinal`，且 Registry 保持 canonical order。

因此没有 world multi-item inventory、Actor 多 root、child list、跨 record Merge/Swap、Take All、自动整理或 implicit target。

## 7. P26—P30 exact-record 非回归

- P26 whole/partial pickup、P27 empty-cell split、P28 exact-N merge、P29 simple quick transfer、P30 complete-graph quick pickup 都继续复用原 accepted delta proof。
- 新 durable callback 参数把原 `WorldDropId + P6Revision` 扩展为 `WorldDropId + Ordinal + RecordRevision + P6Revision`；source world container/root/child closure仍逐项复核。
- partial pickup 保留 exact record 与 container，数量变化由 P6 snapshot revision覆盖；record identity不重建、不重新编号。
- full pickup 只允许 `RemoveAll(WorldDropId)` 返回恰好 1；随后只移除 exact 空 world container。
- pickup candidate 对 Registry 中所有其他 records 执行 closure unchanged comparison；其他 root、container、child、Actor identity 与 Ordinal不能变化。
- P29 仍只对当前打开 compatible simple root执行 `Merge(Quantity=0)`，不搜索其他 Registry member。
- P30 仍只把当前 complete graph移到 SlotIndex 最小的合法空 `BaseQuick`，不进入 child/equipment/P5/P9/P11/Hotbar/其他 WorldDrop。
- player-side complete graph Ctrl 路径仍零写入拒绝；P19 正常 `GroundDropZone` Drag/Drop保持唯一主动落地入口。

## 8. Actor、open/close/stale、恢复与终局

- projection query先验证完整 Registry，再按 canonical order输出只读 projections。
- manager只为当前 map route 的 records维护 `TMap<WorldDropId, Actor>`；重复 refresh复用同一 Actor，缺失 ID才 spawn，消失 ID才 destroy。
- Actor configure只接收已提交 projection；spawn/refresh失败不回写 Registry或物品图。
- Actor interaction以自身 exact ID打开 record，并复核 Owner/Run/Ordinal/container/root/child/route/record revision。
- 每次打开获得新的 target-open generation；旧页面 payload、数量 draft、关闭/重新打开后的 payload都会因 generation或 exact identity不匹配而零写入。
- UI close、失焦、Actor EndPlay只关闭 transient target；不会决定物品归属或删除 durable record。
- active P6 recovery从同一 Registry恢复所有 Available projections；不会重掷、合并、重新编号或 materialize第二物品图。
- terminal/run replacement仍走原 P8/P6 lifecycle；`BuildP14PlayerOnlySession`枚举全部 records并排除所有 ground root、world container与合法 child closure。

## 9. 单事务、回滚与无自动行为

- create/pickup各自只有一个 P1 candidate与一个 Owner replacement；没有逐 record保存、第二 Repository transaction、UI array mutation或 Actor-first write。
- durable write前复核 Owner/Run、exact registry membership/absence、ID/Ordinal/container/root、route、state、P6 revision、record revision、P19 closure与 ordinal上限。
- Candidate使用当前 Store record的值拷贝；`SaveRecord`成功后才替换内存 `Record`，故失败自然回到 BeforeSnapshot。
- accepted create中 `NextWorldDropOrdinal`只前进一次；pickup、close、refresh、terminal observer均不推进。
- 成功后只执行原 P13 reconcile、receipt freeze与 Actor/current projection refresh；没有自动 Bind、Use、Equip、Sort、Compact、selection切换、其他 record打开或其他 SlotIndex重排。
- Code A、Widget与Actor均未获得 Registry、Item、Container、Owner/Run、数量、Loot或终局持久化权威。

## 10. 静态审查

- `git diff --check`：通过。
- WorldDrop durable create路径：一个 `++Session.NextWorldDropOrdinal`，位于 accepted Candidate insertion后。
- WorldDrop create路径：一个 `SaveRecord(Candidate)`。
- WorldDrop pickup路径：一个 `Store.SaveRecord(Candidate)`。
- record deletion：按 exact `WorldDropId`，且要求删除计数恰好为 1。
- canonical serialization：写 JSON前对记录副本做 Ordinal/WorldDropId排序。
- projection与Actor：按 exact `WorldDropId`管理，不读取列表 first/last。
- P8 player-only filtering：对 `Source.WorldDrops` 全量循环，而非单记录。
- 未发现旧 singleton与新 Registry的并行写路径。

## 11. 编译

### Editor

命令：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- Target：`demo_mapEditor Win64 Development`
- native exit code：`0`
- 关键结果：`Result: Succeeded`
- UnrealBuildTool total execution time：`28.91 seconds`

### Game

命令：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- Target：`demo_map Win64 Development`
- native exit code：`0`
- 关键结果：`Result: Succeeded`
- UnrealBuildTool total execution time：`29.72 seconds`

## 12. 留给 0.0.9B.F 的真实验证

本 P 阶段未执行以下真实验证：

- 同时创建第二、第三及更多 simple whole、P26 partial、P19 complete-graph WorldDrop；
- 分别打开/关闭多个 Actor，确认每次只呈现 exact record 的一个 root Cell；
- 对不同 records分别执行 whole/partial/exact-N Drag pickup；
- 对不同 records分别执行 P29 simple-stack与P30 complete-graph QuickTransfer；
- 拾回一个 record后确认其他 durable records、roots、containers、ordinals与Actors保持；
- map reload、active P6 recovery、duplicate refresh、Actor EndPlay与stale payload；
- Extracted、Dead、RecoveredAbandon与重复 terminal observer的全 Registry排除；
- 真实鼠标键盘、UI focus/失焦、截图巡检、Smoke、Automation与回归。

## 13. 提交

- 实现提交：`e4bc9ce feat: add multi-record world drop registry`

READY_FOR_P32_PLANNING
