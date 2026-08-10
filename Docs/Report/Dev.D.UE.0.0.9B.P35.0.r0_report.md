# Dev.D.UE.0.0.9B.P35.0.r0 Report

## 1. 结果

- 任务：`Dev.D.UE.0.0.9B.P35.0.r0`。
- Prompt 已按真实下载文件归档；SHA-256：`D0E5A2A655455CC435C064C46DBF7BA8F9AEAC57B87165E22A42775BCEC4D8F9`。
- 当前已由用户明确激活、仍 identity-valid 的 P17 child 内，direct standard non-spatial Weapon、Armor/ArmorRobe 或普通 Accessory root 现可经既有 normal `InventoryDragOperation` 拖到唯一 `GroundDropZone`。
- accepted create 仍只执行一次 P1 whole-root `Move`、一个 P31/P6 Owner candidate、一次 `SaveRecord`，并生成独立 provenance：`P35.AcceptedGroundDrop.ChildStandardEquipment`。
- exact opened P35 record 可 normal Drag 到用户明确指定的空 `BaseQuick`、当前仍打开的 exact P17 child 空普通格，或空、正式、Definition-compatible equipment slot。
- 没有新增 pointer handler、快捷键、Actor direct pickup、quick-drop、P35 quick-pickup、auto target、auto equip、Swap、Replacement、Sort、Compact、第二 Repository、第二事务或第二保存。
- Editor 与 Game 均编译成功；未启动产品，未执行 PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook 或 Package。

## 2. 改动前调用链与边界审计

### 2.1 P17 current child identity

- P17/P7 从 P1/P6 snapshot读取空间 parent 的 `ItemId -> ChildContainerId`；Repository invariant保证 child container唯一 parent、一层、无环，并拒绝 child item继续携带 `ChildContainerId`或 spatial/backpack definition。
- P2 继续按 active P6 layout投影 `QuickSpatial`与 `PouchInternal`真实 child container；容量来自 canonical parent definition的 `ChildContainerCapacity`，Cell 使用真实 stable `SlotIndex`。
- 既有工作台以 `ActiveDestinationContainerId`表达用户明确选择的 P17 child。P35在同一 transient context上增加 `ActiveDestinationOpenGeneration`；激活／切换时递增，close、page destruct、真实 focus loss或 projection失效时清零。
- generation只是一项 stale-gesture proof，不拥有 item/container authority。Preview、manager callback和 durable Store仍重新读取 P1/P6。

### 2.2 P26 与 P32/P33/P34

- P26 normal child route只接受 canonical simple stack：`bStackable=true`、`MaxStack>1`、正数量、无 child；其 Split/Merge/quantity语义不能泛化为 P35 whole standard root。
- P32 canonical standard classification要求 canonical definition全等、non-stack、`MaxStack=1`、`Quantity=1`、spatial semantic none、child capacity 0、无 `ChildContainerId`，且只接受正式 Weapon/Weapon、Armor/Armor、Accessory/Accessory组合。
- P32/P33正常落地与 normal pickup继续使用既有 source normalization、P1 Move/Unequip/Equip、P31 record cleanup和 P13 reconcile。
- P34只接受 P32/P33 provenance且目标为首空 BaseQuick；P35 provenance未加入 P34 gate，`Ctrl + 左键`因此继续零写入。

### 2.3 唯一活动写入图

`UCodeBP3CellButton`共享拖拽
→ `UCodeBP4DragOperation`稳定 payload
→ `UCodeBP3GroundDropZone::NativeOnDrop`
→ `UCodeBP3UIHostSubsystem::RequestGroundDrop`
→ `Ademo_mapV3ProgressionManager::RequestCodeBGroundDrop`
→ Code A只读 floor-placement adapter
→ `FCodeBOutOfRaidProfileStore::DropMatchedActiveRunWorldDropItem`
→ 一个 P1 candidate／一次 `ExecuteTransaction(Move)`
→ P31 Registry insert、P13 reconcile、full validation
→ 一次 `SaveRecord(Candidate)`
→ durable accepted后 Actor diff刷新。

拾回调用图保持：exact Actor只打开 exact record
→ `WorldDropTarget`单 root投影
→ shared normal Drag/P4/P2/P1
→ accepted profile callback
→ `CommitAcceptedMatchedRunWorldDropPickup`
→ exact record proof、one-command replay、exact cleanup、一次 Owner save
→ matching Actor diff。

未发现或新增 Widget direct Move、child UI cache write、Actor direct pickup、预建 ItemId/WorldDropId、先删 record/Actor、Code A inventory写入或第二保存旁路。

## 3. P35 child standard-root gate

- UI Preview要求 source role为 `QuickSpatial`或 `PouchInternal`、Cell revealed、无 child、non-stack、`MaxStack=1`、`Quantity=1`及正式 standard item/equipment slot组合。
- payload携带 exact OwnerId、RunInstanceId、P6 revision、source ItemId/DefinitionId/ContainerId/SlotIndex，以及 transient active child id/open generation。
- Host和 manager在 floor placement前重验 payload generation仍匹配当前 workspace；child close、switch、focus loss或 stale projection立即拒绝。
- durable `IsP35ActiveP17ChildContainer`从当前 P1/P6重新建立唯一 parent：
  - child必须为非 equipment真实 container且属于 active equipped P17 parent；
  - parent必须唯一、`Quantity=1`、正式 placed，canonical definition全等；
  - QuickRing parent必须位于 active Spatial equipment，StoragePouch parent必须位于 active Backpack equipment；
  - child动态容量必须等于 canonical definition capacity；
  - child中所有已占格继续满足一层、无 child、非 spatial/backpack规则。
- Store随后再次验证 source exact placement、slot反向引用、whole-closure结果为 non-spatial，并复用 P32 canonical standard-root classification。
- BaseQuick、equipment、Hotbar、warehouse、normal container、corpse、another WorldDrop、space parent、带 child root、stack、未揭示、错误 Owner/Run、stale revision或 terminal session均不进入 P35 accepted分支。

## 4. Child root → exact WorldDrop

- P35只在 `ExpectedActiveChildContainerId == ExpectedSourceContainerId`且 open generation非零时识别 child standard source。
- Store先打开 exact active Run并验证 source与 floor transform；WorldDropId/derived container只在所有资格通过后按 `NextWorldDropOrdinal`生成。
- P1只执行一次 `Move`：同一 ItemId从 exact child/SlotIndex直接移动至新 derived world container slot 0；不经过临时 BaseQuick、不 Unequip、不 Split/Merge、不 clone、不创建 ChildContainer。
- accepted root必须与 Before root全字段相等，仅 `ParentContainerId/SlotIndex`改为 world container/0；DefinitionId、ItemId、Quantity=1、Level、Quality、RandomSeed、LegacyAffixDigest与 no-child资格保持。
- 每一个 existing P31 record都通过 closure unchanged proof；新 record使用 `P35.AcceptedGroundDrop.ChildStandardEquipment`，ordinal仅 accepted create递增。
- Registry、P13 reconcile、payload receipt freeze与 full session validation全部通过后才调用唯一 `SaveRecord(Candidate)`；Save失败不替换内存 Record，source仍在原 child，Registry/ordinal/world container/Actor均无 durable变化。
- Actor只在 save accepted后由 projection diff建立；不存在 actor-first或 phantom transient空格。

## 5. Exact P35 record → explicit player target

共同 gate继续复核 OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived container、root ItemId、map route、target-open generation、P6 revision与 available state。一个 Target仍只投影该 exact record的一个 root。

### 5.1 明确 BaseQuick

- 用户必须 Drop到一个明确、真实、空的 `Basic6` stable SlotIndex。
- P1复用 whole-root `Move(0)`；没有 first-empty搜索、auto target或 merge。

### 5.2 明确当前 P17 child

- 仅 P35 provenance允许该分支；P32/P33 record不会被扩展到 child。
- target必须与 payload／command的 active child id和非零 open generation完全一致，并在 Host、manager、Store三层仍为当前身份。
- Store重新从 P1/P6重建唯一 canonical parent/child关系、动态容量和一层资格；目标必须是 exact空普通 slot。
- P1只执行一次 whole-root `Move(0)`；不自动打开、切换、搜索或猜测 child。

### 5.3 明确兼容 equipment slot

- target必须为空、正式 active P6 equipment container、SlotIndex 0，并与 root canonical `EquipSlot`一致。
- P1复用 formal `Equip(0)`；不搜索候选、不替换占用装备、不恢复历史 binding。

三条路径都通过 one-command replay要求 Candidate与从 BeforeSnapshot执行同一 P1 command的 expected snapshot全等。只有 root已离开 exact derived world container时，才删除该 exact record及空 world container；other-record closure必须完全不变。`NextWorldDropOrdinal`不回退，不误删 first/last/selected record。

## 6. 生命周期、回滚与非回归

- child open generation在激活／切换时单调变化；close、focus loss、page destruct、projection失效清除 identity。拖拽后切换 child会因 generation不匹配而零写入。
- WorldDrop Actor的 open generation、focus、record revision、map route与 active P6 revision继续独立验证；EndPlay/close/recovery只释放 transient projection，不领取 durable root。
- profile callback的 BeforeSnapshot rollback保持；Store只在 Save成功后替换 Owner record。target occupied/incompatible、parent mismatch、stale、terminal、save failure均保持 world root、record、Actor和 parent/child graph。
- P17 one-layer graph、P19/P30 complete graph、P26—P29 simple stack、P32/P33 normal Drag、P34 QuickTransfer、P21 corpse equipment、P5/P6 bridge、P13/P15均未改写产品语义。
- P8 player-only finalization仍遍历全 Registry并排除 remaining P35 world root/derived world container以及所有 P19 closure；P35未改变 terminal分类、receipt、recovery或 run replacement。
- success只刷新当前 Repository投影与 matching Actor diff；没有 Sort/Compact、无关 SlotIndex重排、自动 Bind/Use/Equip或第二物品真值。

## 7. 修改／未修改文件

修改：

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：command增加 current-child open generation瞬时证明。
- `Source/demo_map/CodeB/demo_mapCodeBP3.h/.cpp`：workspace generation与 P4→P2传递；不新增写入口。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h/.cpp`：稳定 drag payload携带 child id/generation；P30/P34继续清除 child preference。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h/.cpp`：active Run child显式激活、stale generation gate、P35 provenance normal pickup preview及 focus/close清理。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp`：durable P17 child拓扑证明、P35 create provenance、三种 explicit pickup target与 exact candidate proof。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：ground/pickup提交前复核 current workspace generation，并向唯一 Store writer转发瞬时证明；Code A仍只提供 floor/Actor边缘适配。

未修改：

- P1 transaction实现、P2 Apply实现、P17 graph创建/迁移、P31 Registry schema/migration、WorldDrop Actor类、P8 finalization、P13/P15、corpse/warehouse/normal-container模块、Build.cs与项目配置。

提交：

- Prompt归档：`0604c9d docs: archive Dev.D.UE.0.0.9B.P35.0.r0 prompt`
- 实现：`be9c738 feat: add P35 child equipment ground loop`

## 8. 静态审查

- `git diff --check`：通过。
- source gate同时受 UI revealed/role/standard topology、workspace generation、manager exact source address与 Store canonical P1/P6 topology约束。
- create路径只有一个 `Repository.ExecuteTransaction(Move)`和一个 `SaveRecord(Candidate)`；Actor spawn位于 durable accepted后的 projection refresh。
- pickup路径只接受 P32/P33/P35 accepted provenance的 canonical standard root；P35 child target另需 exact current child id/generation，P35 provenance不进入 P34 quick gate。
- candidate equality、other-record unchanged、exact record removal、P13 reconcile与 full session validation保持；不存在 auto target、auto equip、Swap、Replacement或 record顺序猜测。

## 9. 编译

Editor：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native UBT exit code：`0`。
- 结果：`Result: Succeeded`。
- UnrealBuildTool total execution time：`24.90 seconds`。
- 说明：Codex外层首次5秒等待器先返回 timeout，但未终止独立 UBT/dotnet子进程；子进程正常结束并在原生 UBT log记录上述成功结果。为遵守“不重复构建”，未再次运行 Editor命令。

Game：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native exit code：`0`。
- 结果：`Result: Succeeded`。
- UnrealBuildTool total execution time：`27.85 seconds`。
- 输出：`Binaries\Win64\demo_map.exe`。

## 10. 留给 0.0.9B.F 的真实验证

- 多个不同 P17 child内分别放置 Weapon、Armor/ArmorRobe、普通 Accessory root并 normal Drag落地；
- 多个 P35/P32/P33/P19/P26 record同时存在时验证 exact actor/record隔离与 ordinal单调；
- exact P35 record分别拖回明确 BaseQuick、当前明确 child普通格、正确空 equipment slot；
- child未激活、drag后切换／关闭、focus loss、parent mismatch、动态容量变化、occupied child/BaseQuick与 incompatible equipment拒绝；
- P19 graph、simple stack、带 child root、warehouse、normal container、corpse、Hotbar、another WorldDrop拒绝；
- P34对 P35 provenance的 Ctrl拒绝，P29/P30/P32/P33原路径保持；
- stale Owner/Run/P6/record/open generation、Actor EndPlay、map reload、active P6 recovery、terminal、save failure与 BeforeSnapshot rollback；
- Actor projection、P8 full-registry exclusion、P13 reconcile、stable SlotIndex、scroll/selection、right-click、double-click、`Shift + 1—9`与无 Sort/Compact；
- 真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package与最终验收。

READY_FOR_P36_PLANNING
