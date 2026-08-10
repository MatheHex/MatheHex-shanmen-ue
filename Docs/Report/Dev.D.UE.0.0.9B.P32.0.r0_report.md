# Dev.D.UE.0.0.9B.P32.0.r0 Report

## 1. 结果

- 任务：`Dev.D.UE.0.0.9B.P32.0.r0`。
- 已将 active P6 当前正式 Weapon、Armor/ArmorRobe、普通 Accessory 装备位中的 canonical、非空间、不可堆叠、`Quantity=1`、无 child 的 standard equipment root 接入既有 GroundDrop 工作流。
- normal Drag 到 `GroundDropZone` 直接执行一条 P1 `Unequip`，将同一 ItemId 从真实装备位放入新的 P31 derived world container slot 0；没有临时 BaseQuick、第二 P1 transaction、clone 或新 ItemId。
- exact opened WorldDrop root 只允许 normal Drag 到用户明确选择的空 BaseQuick 格，或明确选择的空、Definition-compatible active P6 equipment slot；分别复用 P1 `Move` / `Equip`。
- UI preview 与 durable Store 都拒绝 Ctrl、占用目标、Replacement、Swap、自动选槽、错误 slot、stale revision、非 canonical definition、空间 parent/child、stack 与非 active equipment source。
- P31 Registry identity、other-record isolation、ordinal、Actor diff、migration、P8 full-registry exclusion及 P26—P30 语义未改写。
- Editor 与 Game 编译均以 native exit code `0` 完成。
- 按 P 阶段规则未启动产品，也未执行 PIE、Standalone、Automation、Smoke、回归、截图巡检、Cook 或 Package。

## 2. 改动前调用链与资格审计

### 2.1 standard equipment / P1 / P4x / P7

1. P6 materialization 在 `FCodeBRunInventorySession::Layout` 中持有正式 `WeaponContainerId`、`ArmorContainerId` 与 `AccessoryContainerIds`，并用 P1 `EquipmentSlot` 分别建立 Weapon、Armor、Accessory 单格装备容器。
2. P2 projection 从 P1 snapshot 投影 `ItemType`、`EquipSlot`、stack、quantity 与 child identity；P3 revealed Cell 生成共享 P4 stable Drag payload。没有装备专用 Widget、第二 Drag 类型或 UI item authority。
3. P4 `PreviewDrop` 对 storage → equipment 采用 definition/slot compatibility并产生 `Equip`，对 equipment → storage 产生 `Unequip`；P1 `ExecuteEquip` / `ExecuteUnequip` 再从 Repository definition、source placement、target kind/slot及 revision复核。
4. P1 `Equip` 原本允许 replacement，因此 P32 在 UI preview后和 durable commit前都另外要求目标在 BeforeSnapshot 中为空；P32不调用 `Swap`。
5. P3 profile callback只在 P2/P1 accepted candidate后调用 durable commit；durable拒绝时用 BeforeSnapshot恢复 P1 projection，保持 single candidate rollback链。

### 2.2 P14/P19/P26—P31 GroundDrop / pickup

player → world：

`GroundDropZone::NativeOnDrop` → shared inventory Drag payload → `RequestGroundDrop` identity/write gate → Code A floor route/transform adapter → `DropMatchedActiveRunWorldDropItem` → one P1 transaction → one P31 Candidate insertion → P13 reconcile / full session validation → one `SaveRecord(Candidate)` → committed Actor projection diff。

world → player：

exact Actor interaction → exact `WorldDropId` target open → one-root `WorldDropTarget` projection → shared P4 preview → P2/P1 accepted command → `CommitAcceptedMatchedRunWorldDropPickup` exact Owner/Run/WorldDropId/Ordinal/record revision/container/root/P6 revision proof → exact record removal → one Owner save → Actor diff。

P8 `BuildP14PlayerOnlySession` 已按完整 `WorldDrops` Registry逐 record调用 generic root/closure discard；P32 standard root仍是同一 single-root shape，因而无需 P8 特例或第二终局路径。

### 2.3 Catalog / Revealed / provenance

- Store 使用 `BuildCanonicalCodeBItemDefinition` 从活动 Catalog重建 definition，并要求它与 Repository definition全字段相等；不依据显示名、图标、Widget class、Actor tag、fixture或 Code A旧库存。
- P32 closed slice只接受 `ItemType` 与 `EquipSlot` 均为 Weapon/Armor/Accessory、`bStackable=false`、`MaxStack=1`、spatial semantic none、child capacity 0、instance `Quantity=1` 且无 `ChildContainerId`。
- source container必须是活动 Layout中与 definition slot严格匹配的单格 equipment root，且 slot 0反向指向同一 ItemId。尸体装备、P5、P9/P11、BaseQuick、P17 child及其他 WorldDrop不是合法 source。
- P3只允许 `IsRevealed()` 的 Cell开始共享 Drag；active P6装备投影为 revealed。外部 Hidden/Searching Cell已有保护并不进入 P32 Store source gate。
- ItemId、DefinitionId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest及无-child资格保持；新 record只记录 `P32.AcceptedGroundDrop.StandardEquipment` 的创建路径 provenance。

## 3. 实现

### 3.1 durable eligibility

`Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp` 新增三个窄 helper：

- `IsP32StandardEquipmentRoot`：canonical Catalog/Repository全字段一致及 standard non-spatial single-root检查；
- `IsP32ActiveStandardEquipmentContainer`：只接受当前 P6 Layout的 Weapon、Armor、普通 Accessory正式 root及匹配 slot semantic；
- `IsP32EquippedStandardRoot`：将 ItemId、source container、slot 0与 definition语义绑定。

这些 helper不扩大 P19 graph、P26 stack、P21 corpse equipment或普通 storage root。

### 3.2 equipped root → GroundDrop

- 既有 source identity、Owner/Run、P6 revision、route/finite transform、terminal/committed session与 P19 closure gate保持。
- P32只在 whole-drop分支增加 `bP32EquippedStandardSource`；任何 partial quantity立即沿既有 P26规则拒绝。
- P1 request因 source为 equipment只执行一次 `Unequip(source equipment → new world container slot 0)`。
- accepted snapshot额外构造只改变 parent container/slot的 expected root并全字段比较，证明没有 Definition、Quantity、ItemId、instance provenance或 child变化。
- P31继续逐条证明已有 record closure不变；新 record与 `NextWorldDropOrdinal`只在 Candidate中建立，full validation后只保存一次。失败不替换 Store，也不建立 Actor-first状态。

### 3.3 exact WorldDrop → explicit BaseQuick / equipment

- P3/P4 preview从 P1 projection识别 P32 standard root；只放行 normal Drag 的空 Basic6 `Move`，或由 P4 compatibility判定的空 equipment `Equip`。
- durable proof要求 accepted command为 `Standard` intent、exact ItemId、exact opened world container slot 0、exact expected revision、`Quantity=0`、明确 target address，且 BeforeSnapshot target为空。
- BaseQuick目标必须是 exact `Layout.BasicContainerId`；equipment目标必须是 exact active Layout Weapon/Armor/Accessory root且 slot semantic与 canonical Definition一致。
- accepted command与 candidate placement绑定后，代码用同一 P1 request从 BeforeSnapshot重放并要求 snapshot全等；随后只移除 exact record与其空 derived world container。P31 other-record closure unchanged proof保持。
- 不搜索 Accessory槽、不使用 first/last record、不从当前 selection推断、不自动装备、不替换、不 Swap、不回填 Hotbar。

## 4. 边界保持

- `Ctrl + 左键`：P32 UI preview显式拒绝；Store的 QuickTransfer首分支仍只接受 P29 simple stack或 P30 complete graph，standard equipment两者都不满足。
- `Shift + 1—9`、右键、普通点击、双击、滚动、close/focus loss、Actor EndPlay：未修改，均未获得 P32位置写入语义。
- P19/P30空间 parent/child：P32要求无 child、无 spatial semantic；原 complete-graph分支顺序与 proof未改。
- P26—P29 stack/quantity：P32要求 non-stackable、quantity 1；Split/Merge分支未改。
- P21 corpse equipment：active Layout exact-container gate排除 corpse container；既有 read-only/source transfer规则未改。
- P31：没有 schema、JSON、migration、sort、record identity、Actor mapping或 NextOrdinal算法修改；create/pickup仍逐 record证明其他成员不变。
- P8/P5/P6/P13/P15/P17：未改变 terminal分类、bridge、receipt、hotbar reconcile或空间区域；P13只在 accepted Candidate上继续执行既有 reconcile。
- Code A只继续解析 floor placement并转发 Actor生命周期；没有 Item/Registry/数量/装备 authority。

## 5. 修改文件

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
  - canonical P32 root/source/target gate；
  - equipped root single `Unequip` accepted proof与 record provenance；
  - exact world root normal `Move` / `Equip` command及 empty-target proof。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
  - P1 projection-derived P32 preview资格；
  - explicit empty BaseQuick / compatible empty equipment preview；
  - Ctrl、replacement与非法目标零写入拒绝。

未修改 P1 Repository实现、P31 schema/Actor、P8 finalizer、Catalog数据、战斗、地图、敌人、经济、网络或多人。

## 6. 静态审查

- `git diff --check`：通过。
- P32 player → world：一个 P1 `Unequip` request、一个 Candidate、一个 `SaveRecord`；无临时 BaseQuick与 two-step revision。
- P32 world → player：一个 accepted P2/P1 `Move`或`Equip`、一个 durable Candidate、一个 `SaveRecord`；occupied target在 BeforeSnapshot gate被拒绝。
- exact record删除继续要求按 `WorldDropId` 删除计数恰好为 1；其他 Registry records逐条 closure unchanged。
- NextOrdinal只在 accepted create Candidate中递增；pickup、preview、close、Actor refresh均不递增。
- 没有新 Repository、fixture、item/container ID生成器、UI mirror、Actor direct pickup或 Code A inventory写入。

## 7. 编译

Editor：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native exit code：`0`
- 结果：`Result: Succeeded`
- UnrealBuildTool total execution time：`8.70 seconds`

Game：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native exit code：`0`
- 结果：`Result: Succeeded`
- UnrealBuildTool total execution time：`14.64 seconds`

## 8. 留给 0.0.9B.F 的真实验证

本 P 阶段按要求未执行真实产品测试。F阶段应覆盖：

- Weapon、Armor/ArmorRobe、普通 Accessory分别从真实装备 Cell拖至 GroundDrop；
- 同一 ItemId从 exact Actor拖回明确空 BaseQuick及明确空兼容装备位；
- occupied/incompatible equipment、P17 child、另一 WorldDrop、stale/closed target、Ctrl及 replacement拒绝；
- 多 record共存时只创建/删除 exact record，其他 roots/containers/ordinals/Actors不变；
- Save失败、map reload、recovery、Actor EndPlay、terminal与 P8 full-registry排除；
- 真实鼠标键盘、焦点、截图、Smoke、Automation与回归。

## 9. 提交

- 实现提交：`29d8507 feat: support standard equipment world drops`

READY_FOR_P33_PLANNING
