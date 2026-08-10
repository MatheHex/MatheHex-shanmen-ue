# Dev.D.UE.0.0.9B.P33.0.r0 Report

## 1. 结果

- 任务：`Dev.D.UE.0.0.9B.P33.0.r0`。
- active P6 exact BaseQuick 中已 Revealed、尚未装备的 canonical standard non-spatial Weapon、Armor/ArmorRobe 与普通 Accessory root，现已获得显式 P33 source classification。
- 该 root 继续使用共享 normal Drag 与既有 GroundDropZone；accepted path只执行一次 P1 whole-root `Move`，直接从原 stable BaseQuick SlotIndex进入新 P31 derived world container slot 0。
- P32 equipped source仍独立分类并执行一次 `Unequip`；P33没有扩大 P32 source eligibility。
- exact P32/P33 standard WorldDrop仍复用 P32已接受的显式空 BaseQuick `Move` / 显式空兼容 equipment `Equip` proof；没有自动选槽、Replacement、Swap或 Ctrl拾回。
- P31 Registry、P8 full-registry exclusion、P26—P30 stack/space语义与 P13 reconcile保持。
- Editor与 Game均以 native exit code `0` 编译成功。
- 未启动产品，未执行 PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook或 Package。

## 2. 活动调用链审计

### 2.1 BaseQuick / P1 / P4 / P7

1. `FCodeBRunInventorySession::Layout.BasicContainerId` 是 active P6 exact BaseQuick root；SlotIndex与 placement只从 P1 Repository snapshot读取。
2. P2 projection投影同一 ItemId、DefinitionId、Quantity、stack、EquipSlot与 child identity；P3仅为 `IsRevealed()` Cell建立共享 P4 stable Drag payload。
3. `GroundDropZone::NativeOnDrop` → `RequestGroundDrop`重新验证 Owner/Run/scope/P6 revision/source stable address → manager只解析 map route与 finite floor transform → durable Store重读 exact P1/P6 source。
4. P1 `ExecuteMove`要求 source不是 equipment、target为明确空 storage slot，并在一个 Candidate内只改变同一 root的 parent/slot；失败不替换原 snapshot。
5. P3/P2 accepted profile callback已有 BeforeSnapshot rollback；P33未建立 Widget direct Move、专用 Drag、第二 Repository或第二 save。

### 2.2 P14/P31 GroundDrop Registry

`DropMatchedActiveRunWorldDropItem`
→ exact Owner/Run/P6 revision/source/route/floor gate
→ create one deterministic WorldDropId + one derived single-slot storage container in transient Repository
→ one P1 `Move`
→ existing-record closure unchanged proof
→ append one P31 record / accepted-only ordinal advance
→ P13 reconcile / receipt freeze / full session validation
→ one `SaveRecord(Candidate)`
→ committed projection / Actor diff。

Actor只打开 exact `WorldDropId`；WorldDropTarget仍只显示该 record的 slot 0 root。pickup按 exact Owner/Run/WorldDropId/Ordinal/record revision/container/root/route/open generation/P6 revision提交，只删除 matching record及空 world container。

## 3. 实际 eligibility 与 source 分流

- canonical truth由 `BuildCanonicalCodeBItemDefinition`从活动 Catalog重建，并要求与 Repository definition全字段相等。
- standard closed slice要求：
  - `ItemType/EquipSlot = Weapon/Weapon`；当前 Catalog正式例包括 `HeavyPracticeBlade`；
  - `ItemType/EquipSlot = Armor/Armor`；当前正式 ArmorRobe/护甲例包括 `ReinforcedVest`；
  - `ItemType/EquipSlot = Accessory/Accessory`；当前普通饰品例包括 `EvasionCharm`。
- 三类统一要求 `bStackable=false`、`MaxStack=1`、instance `Quantity=1`、spatial semantic none、child capacity 0、无 `ChildContainerId`。
- P33 source还要求 `ParentContainerId == Layout.BasicContainerId`、source container非 equipment、SlotIndex有效且该格反向指向同一 ItemId。
- P32 source仍要求 active Layout Weapon/Armor/Accessory equipment root slot 0。两者共享 helper与 durable chain，但用 source placement明确分流：P33 `Move`、P32 `Unequip`。
- P17 child、SpatialRing、WindTalisman、BackpackLevel1、stack、P5 warehouse、P9 normal container、P11 corpse、尸体装备、另一 WorldDrop与 Code A旧库存均不满足 P33 exact BaseQuick source。
- P14既有普通 simple whole-root语义继续保留；P33只对其中 canonical standard equipment形成显式分类与证明，没有把其他 storage泛化为 P33。

## 4. BaseQuick root → exact WorldDrop

- `bP33BaseQuickStandardSource`在 durable Store从 active snapshot重新计算，不信任 UI显示名、图标、Cell class、Actor tag或 fixture。
- one P1 request的 operation为 `Move`，source是 payload绑定并重验的 exact BaseQuick address，target是新 derived world container slot 0；没有临时容器、two-step relocation或第二 revision。
- accepted snapshot用原 root副本只替换 parent container与 slot，然后通过 `FCodeBItemInstance::operator==`全字段比较，证明 ItemId、DefinitionId、Quantity=1、Level、Quality、RandomSeed、LegacyAffixDigest及 no-child资格不变。
- record provenance为 `P33.AcceptedGroundDrop.BaseQuickStandardEquipment`；P32 record provenance保持 `P32.AcceptedGroundDrop.StandardEquipment`。
- existing records逐条执行 closure unchanged proof。ordinal只在 Candidate append后前进一次；validation/save失败不会替换 Store或创建 Actor-first/phantom状态。

## 5. exact P32/P33 WorldDrop → explicit destination

- standard world root合法性仍由 canonical definition、no-child、non-stackable、Quantity=1与 exact record identity判断；pickup不需要按 create family分叉，因此 P32与 P33 provenance都合法且无需改 schema/migration。
- normal Drag到 exact `Layout.BasicContainerId`明确空格：一条 P1 `Move`。
- normal Drag到 active Layout中 Definition-compatible明确空 Weapon/Armor/Accessory slot 0：一条 P1 `Equip`。
- durable proof要求 `Standard` intent、exact ItemId、exact world container slot 0、`Quantity=0`、expected P6 revision、明确 target address及 BeforeSnapshot target为空；occupied target在 P1 replay前拒绝。
- candidate必须与从 BeforeSnapshot重放同一 P1 request得到的 snapshot全等，随后只移除 exact record和空 world container；其他 Registry records继续逐条证明不变。
- 无 auto equip、first/last record、current selection猜测、Accessory槽搜索、Swap、Replacement、P17/P5/P9/P11/Hotbar fallback或 Actor direct pickup。

## 6. lifecycle、回滚与边界保持

- Owner、Run、P6 revision、WorldDropId、Ordinal、record revision、target-open generation、route、exact root/container在现有 P31/P32 Preview与 durable Commit链中逐项复核。
- close、focus loss、Actor EndPlay、stale payload、重复 Actor refresh和 recovery只处理 matching transient projection；不删除 durable record、不回填 BaseQuick/equipment。
- P8 `BuildP14PlayerOnlySession`继续遍历全 Registry并用 generic root/closure discard排除 P33 world root与 derived container；无需 P33终局特例。
- P13 reconcile只在 accepted Candidate上执行；P15/P17动态空间容量、stable child SlotIndex、independent scroll与 selection未改。
- P29/P30 Ctrl QuickTransfer、P26—P28 stack quantity、P19 complete graph、P21 corpse equipment、P5/P6 bridge、P8 receipt及 Code A边缘 authority未改。
- Ctrl + 左键、Shift + 1—9、右键、双击、普通 player inventory Drag、scroll、Tab/I/Esc/close/cancel均未获得 P33隐式写入语义。
- 无 Sort、Compact、自动 Bind/Use/Equip、无关 SlotIndex重排或空间区域重建。

## 7. 修改／未修改文件

修改：

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
  - 增加 exact BaseQuick canonical standard root的 P33分类；
  - 将 P32/P33 standard root共用 accepted relocation全字段 proof；
  - 为 accepted P33 create记录独立 provenance。

未修改：

- P1 Repository、P2/P3/P4 UI/controller与 P7 layout：既有共享 Drag、stable Slot、Move/Equip/Unequip、preview与 rollback已满足 P33。
- P31 schema/JSON/migration/Registry/Actor、P8 finalizer、Catalog数据、P5/P6 bridge、P13 hotbar、P17/P19、P21、战斗、地图、敌人、搜索、经济、网络与多人。

## 8. 静态审查

- `git diff --check`：通过。
- P33 create沿既有函数只有一个 P1 `Move`、一个 Owner Candidate与一个 `SaveRecord(Candidate)`。
- P32 create仍是一条 `Unequip`；P26 partial、P19 complete graph与 P31 existing-record proof未改。
- accepted standard root只允许 parent/slot变化；没有新 ItemId、player container、child、数量或 definition写入。
- `NextWorldDropOrdinal`仍只有 accepted create Candidate中的一次递增。
- exact pickup仍按 `WorldDropId`要求删除计数恰好为 1，并证明所有 other records不变。

## 9. 编译

Editor：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native exit code：`0`
- 结果：`Result: Succeeded`
- UnrealBuildTool total execution time：`9.63 seconds`

Game：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native exit code：`0`
- 结果：`Result: Succeeded`
- UnrealBuildTool total execution time：`14.93 seconds`

## 10. 留给 0.0.9B.F 的真实验证

- 不同 BaseQuick SlotIndex中的 Weapon、Armor/ArmorRobe与多个普通 Accessory root分别落地；
- P33 BaseQuick source与 P32 equipped source、多 P31 records同时存在；
- P32/P33 standard records分别拖回明确空 BaseQuick及正确空装备位；
- occupied/incompatible equipment、stack、space parent/child、P17 child、warehouse、normal container、corpse与另一个 WorldDrop零写入；
- Ctrl、right-click、double-click、stale/close/focus loss/EndPlay/recovery/terminal与 save failure；
- Actor projection与 P8 full-registry exclusion；
- stable SlotIndex、动态空间容量、scroll、selection与无 Sort/Compact/auto behavior；
- 真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package与最终验收。

## 11. 提交

- 实现提交：`9352873 feat: classify BaseQuick equipment world drops`

READY_FOR_P34_PLANNING
