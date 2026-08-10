# Dev.D.UE.0.0.9B.P30.0.r0 Report

## 1. 结论

P30 已在现有 P4/P23/P29 共享工作台内完成：当前已打开且身份匹配的 P19 `WindTalisman`／`BackpackLevel1` 完整空间图 root，可通过唯一 `Ctrl + 左键` 路由原子移动到当前 P6 `BaseQuick` 按真实 `SlotIndex` 升序的第一个合法空格。实现没有增加按钮、输入旁路、Actor pickup、自动装备、拆包、图复制、第二 Repository 或第二保存。

Editor 与 Game 两个指定目标均以原生 exit code 0 编译成功。未启动产品，未执行 PIE、Standalone、鼠标键盘实测、截图、Smoke、Automation、回归、Cook、Package 或试玩。

## 2. Prompt 与提交

- 下载文件：`Dev.D.UE.0.0.9B.P30.0.r0_prompt.md`
- 下载／归档 SHA-256：`A605EE200A46054C889944449E3FD8C0A513826B7B233BA6307D5337F097A1DB`
- Prompt 归档提交：`de4387f docs: archive Dev.D.UE.0.0.9B.P30.0.r0 prompt`
- 实现提交：`d762423 feat: add complete graph world quick pickup`

## 3. 实际修改文件与职责

### 已修改

1. `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
   - 继续复用 `UCodeBP3CellButton::NativeOnMouseButtonDown` 这一个 `Ctrl + 左键`消费点。
   - 在既有 `HandleQuickTransfer` 中以投影出的正式 `DefinitionId` 与有效 `ChildContainerId` 识别 P30 topology gate；不以文本、图标、Widget class 或 Actor tag 判定。
   - P30 明确清除 P29 active-child transient preference，只解析 `Projection.BasicContainerId` 且 role 为 `Basic6` 的容器。
   - 跳过 merge-first 分支，只按 `SlotIndex` 升序寻找第一个空 BaseQuick 格；Preview 只接受 `Move(Quantity=0)`，拒绝 Equip、Swap、Merge、Split、active child 与隐式 fallback。
   - 普通 P19 Drag/Drop 仍走原有 Preview 分支；P29 simple stack 仍走原有 active child 优先、BaseQuick 回退、merge-first／empty-second。

2. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
   - 新增 `IsExactP30WorldDropCompleteGraphQuickTransferDelta`，对已被 P2/P1 接受的一个 whole-graph Move 做结构证明，不重放第二次 durable 写入。
   - 证明 exact Owner candidate 中：source 为对应 P14 单 root world container slot 0；target 为 P6 BaseQuick；目标是第一个真实空格；command 为同一 `QuickTransfer` intent、`Move`、`Quantity=0`、精确 revision。
   - 复用 `ValidateP19WorldDropClosure` 验证前后完整闭包；用完整 snapshot equality 证明除 root parent placement 与 revision 外，parent、ChildContainer、child ItemId/ContainerId、数量、顺序、容量、provenance 与所有无关状态均未改变。
   - P30 与 P29 在同一 `CommitAcceptedMatchedRunWorldDropPickup` durable callback 内按 topology 分支；成功后 accepted-only 删除同一 WorldDrop record 与空 world container，继续只执行一次 P13 reconcile、一次 `SaveRecord`。

### 明确未修改

- `demo_mapCodeBP2.*`、`demo_mapCodeBP3.*`、`demo_mapCodeBP4.*`：继续传递既有 `QuickTransfer` transient command；没有新增 P30 intent、第二 P2/P1 调用或 UI write path。
- `demo_mapCodeBInventory.*`：继续使用既有 P1 `Move` 与统一 invariant validation；没有新增 graph 操作、ItemId/ContainerId 或仓库真值。
- `demo_mapV3ProgressionManager.*`：沿用已验收 P29 的 opened WorldDrop Owner/Run/record/container/root/revision/ordinal/Actor write gate、single durable callback 与 Actor refresh；没有 Actor direct pickup。
- Code A、搜索、尸体、装备、Hotbar、P5/P8/P9/P11/P15/P17/P20/P21、地图、战斗、生命、死亡、撤离、商店、经济、制作、网络与多人均未改动。

## 4. 活动调用图与唯一输入证据

```text
UCodeBP3CellButton::NativeOnMouseButtonDown
  Ctrl + Left + Revealed
    -> BeginP4PointerGesture
    -> UCodeBP3InventoryWidget::HandleQuickTransfer
       -> BeginP4Drag / PopulateTransferContext
       -> topology gate
          simple stack -> 原 P29 resolver
          formal P19 root -> P30 first-empty BaseQuick resolver
       -> PreviewInventoryTransfer
       -> CommitInventoryTransfer
       -> FCodeBP4InteractionController::CommitDrop
       -> FCodeBP3UIController::CommitP4Operation
       -> FCodeBP2ApplicationService::Apply
       -> FCodeBRepository::ExecuteTransaction(Move)
       -> existing ProfileCommit callback
       -> CommitAcceptedMatchedRunWorldDropPickup
       -> P30 accepted-snapshot proof
       -> P13 reconcile / Freeze receipt / Validate session
       -> one SaveRecord
       -> existing Actor projection refresh
```

`NativeOnMouseButtonDown` 命中后仍只产生一次既有 QuickTransfer command 并返回 `Handled`。本任务没有增加另一个 pointer handler、Button、hotkey、right-click Take、double-click、Actor click、Widget direct mutation 或第二 transaction path。

## 5. Current opened target 与 stale／零写入边界

- UI Preview 前继续执行 `ValidateTransferContext` 与 `ValidateWorldDropTransferContext`，要求当前 Workspace 为 in-run，Owner、Run、WorldDropId、derived world container、root ItemId、P6 revision、source address 与 external-target scope 全部匹配。
- Manager 的既有动态 write gate 在 Commit 前重新打开 matched active P6 session，并核对活动 Run、Actor Owner/Run/WorldDropId、record `Available`、map route、world container、root、repository revision 与 `NextWorldDropOrdinal`。
- durable callback 再次打开 Owner-matched active session，重新核对 WorldDrop record、revision、root placement、P19 closure 与 candidate。
- Close、切换 drop、失焦导致 gate unavailable、terminal／Prepared、Actor EndPlay、record/root/container mismatch、revision stale、错误 Owner/Run、目标满、目标非 BaseQuick、SaveRecord failure 均在 durable success 前失败；P3 使用 BeforeSnapshot 恢复内存 Repository 并刷新投影，因此零持久化、无空格幻象、无 Actor／Cell 残留。

P30 gesture 同步生成并提交，且只能来自当前 revealed root Cell；旧 payload 不能绕过 Host optional presentation、动态 write gate 或 durable record 复核。

## 6. P17/P19 complete graph closure

Store 在 BeforeSnapshot 与 accepted Candidate 两侧均复用 canonical `ValidateP19WorldDropClosure`：

- 只接受 `Fdemo_mapItemIds::WindTalisman` 与 `Fdemo_mapItemIds::BackpackLevel1`；
- parent 必须拥有由 stable parent ItemId 推导出的唯一正式 `SpatialChildGuid`；
- ChildContainer type、动态 capacity 与正式 QuickRing／StoragePouch semantic 必须匹配；
- child 可空或非空，均作为完整 closure；
- 每个 child 的 parent container、SlotIndex、Definition、数量与 placement 必须有效；
- child 不得再拥有 ChildContainer，不得为 SpatialItem／Backpack，从而保持 one-layer、no-cycle；
- Repository invariant、canonical closure 与完整 snapshot equality 联合排除 orphan、duplicate、嵌套、clone、新 ItemId／ContainerId、数量变化及 provenance 漂移。

若 parent 有 child 但 definition 不是上述两项，或正式 parent 缺少／冲突 child identity，P30 拒绝且零写入。空 child 不会降级为 simple item 分支。

## 7. BaseQuick 唯一 candidate

- UI 只选 `Projection.BasicContainerId` 且 role 为 `Basic6` 的正式 P6 容器。
- 扫描从 `SlotIndex 0` 到 capacity-1；P30 跳过 occupied merge loop，第一次遇到真实投影空格才 Preview／Commit。
- Store 重新读取 durable BaseQuick，要求 target index 有效、真实为空，并要求所有更小 SlotIndex 均已占用；因此 stale UI、显示空但 P1 非空、不可写或非第一个空格都会失败。
- `QuickTransferActivePlayerContainerId` 在 P30 command 中必须无效，防止继承 P29 active P17 child；没有 SpatialRing／Backpack 装备位、P17 child、Hotbar、P5/P9/P11/P14、Swap、Sort、Compact、挤位、auto equip 或 no-space fallback。

BaseQuick 无空格时 resolver 只返回反馈并零写入。

## 8. 单一 whole-graph 事务、记录清理与回滚

- Preview 成功只建立一个 candidate；P2 只提交一个已有 P1 `Move`，没有 Merge、Split、Swap、Equip、Quantity draft 或 graph recreation。
- P30 structural proof 从 durable BeforeSnapshot 构造唯一 Expected：revision +1、world slot 0 清空、目标 BaseQuick slot 指向同一 root、root parent/slot 改为目标；`Candidate == Expected` 才可继续。
- candidate 中 parent、ChildContainer 与全部 child identity/content 不变；root placement 是唯一物品图变化。
- durable callback 内统计：`SaveRecord` 1 次、P13 `ReconcileHotbarBindings` 1 次、WorldDrop record removal 1 处、`NextWorldDropOrdinal` 写入 0 次。
- 仅在 accepted proof 后移除同一 record 与空 world container；Actor 由既有 committed refresh 更新。没有预先删 Actor/record、第二保存、新 ordinal、新 Actor、新 parent/child/item 或自动 Bind。
- candidate validation、closure、session guard、P13 reconcile、freeze、final validation 或 SaveRecord 任一失败时，Store record 不替换；P3 BeforeSnapshot rollback 恢复 UI Repository。

## 9. 非回归静态结论

- P29 simple stack world → player：active P17 child 优先、BaseQuick 回退、merge-first／empty-second 未改。
- P29 player → world：仍只允许 BaseQuick／当前 active child 的 compatible simple root `Merge(Quantity=0)`；complete graph player source仍因没有安全目的地而零写入拒绝。
- P26—P28 explicit Drag/Drop、Split(N)、Merge(N) 分支未改。
- P19 normal Drag/Drop 仍允许完整图拖到空 BaseQuick 或匹配空装备位；P30 quick branch只额外限制自身，不收窄普通 Drag/Drop。
- SlotIndex、动态容量、双栏 scroll、selection 与其他 transient state 未重排；无 Sort／Compact／auto behavior。
- 普通左键、右键详情、双击、Tab、I、Esc、Close、Cancel、空白、滚动、Actor interaction 与 `Shift + 1—9` 语义未改。
- Code A 未取得 item graph、数量、Container、Owner/Run、Loot、搜索、终局或 WorldDrop 持久化权威。

## 10. 编译

### Editor

```powershell
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- Native exit code：`0`
- 关键结果：`Result: Succeeded`
- 总执行时间：`26.42 seconds`

### Game

```powershell
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- Native exit code：`0`
- 关键结果：`Result: Succeeded`
- 总执行时间：`29.64 seconds`

## 11. 明确未执行的 F 阶段验证

未启动或执行：产品运行、PIE、Standalone、真实 Ctrl+左键、真实 Drag/Drop、鼠标键盘输入、截图巡检、Smoke、Automation、回归、试玩、Cook、Package、网络／多人验证。上述验证统一留到 `0.0.9B.F`。

READY_FOR_P31_PLANNING
