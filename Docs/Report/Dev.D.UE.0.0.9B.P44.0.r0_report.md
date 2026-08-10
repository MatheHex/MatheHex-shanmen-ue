# Dev.D.UE.0.0.9B.P44.0.r0 Report

## 结论

READY_FOR_P45_PLANNING

P44 已静态闭合：当前已打开、已揭示且 identity-valid 的 P21 `Body.Weapon` / `Body.ArmorRobe` / `Body.Accessory0` standard non-spatial root 可通过既有 normal Drag 与既有 `GroundDropZone` 整根落地为一个独立 P31 record；该 record 仅接入既有 standard-equipment normal pickup 与 shared frozen-target Ctrl QuickTransfer。Editor 与 Game 均以 native exit code 0 编译完成。未启动产品，未执行 F 阶段真实验证。

## 文件范围

新增：

- `Docs/Report/Dev.D.UE.0.0.9B.P44.0.r0_report.md`：本报告。

修改：

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：把既有 P38/P39 P21 source proof 补全为 P44 可复用的 exact source container/slot/item/definition、loot/materialization digest 与 composite revision；仍为瞬时只读证明。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`：仅更新共享 Drag payload 的 P38/P39/P44 职责注释；未新增 payload、输入或 resolver。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`：声明 P44 GroundDrop gate 与 P44 provenance read gate。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`：从既有 P21 Cell 填充完整 source proof；在既有 `RequestGroundDrop` 清除玩家目标装饰并重验 P44；把 P44 provenance 加入既有 standard-root normal/QuickTransfer admission。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`：扩展既有尸体到地面唯一 writer 的签名，使其恰好接受 P43 simple 或 P44 equipment source proof 之一。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：复用同一 P43/P44 composite transaction，新增 P21 equipment exact source shape、P44 record provenance、既有 standard pickup/Ctrl admission 与 exact cleanup。
- `Source/demo_map/demo_mapV3ProgressionManager.h`：更新既有 body GroundDrop adapter 的 P43/P44 职责注释。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：既有 body GroundDrop adapter 选择 P43 或 P44 exact proof，转发同一 floor placement，并仅在 durable success 后刷新 body/world projection 与 Actor diff。

明确未修改：

- 所有测试文件；P1/P2/P3/P4 transaction core、P11/P12 search state machine、P21 deterministic materialization/history、P17 graph construction、P31 schema、P8 terminal product logic、P13/P15、P5/P6 bridge、Code A inventory/Loot authority、Actor 类、Widget 类、PROJECT 文档均未修改。
- 未新增 Button、hotkey、pointer handler、Widget、Actor、ItemId、WorldDropId allocator、second resolver、second Repository、second save 或 Code A item writer。

## 调用图与单一真值

尸体装备落地调用图：

1. 既有 P21 equipment `Cell` → 既有 `UCodeBP4DragOperation`；`PopulateTransferContext` 冻结 Owner/Run、BodyTarget、death receipt、open/body/composite revision、actual container/slot/item/definition、slot semantic、r3 profile/candidate/materialization digest。
2. 既有 `UCodeBP3GroundDropZone::NativeOnDrop` → `HandleGroundDropZoneDrop` → `UCodeBP3UIHostSubsystem::RequestGroundDrop`；P44 清除 generic current-child decoration，`ValidateP44BodyEquipmentGroundDropContext` 再调用既有 P38 exact P21 source validator，不形成玩家 target、quantity draft 或 world identity。
3. 既有 `FCodeBP3GroundDropPresentation::RequestDrop` → `Ademo_mapV3ProgressionManager::RequestCodeBBodyGroundDrop`；Code A adapter 只解析 route/floor transform 与活动 Host 生命周期，不创建任何物品或 record identity。
4. `FCodeBOutOfRaidProfileStore::DropMatchedRunBodyContainerWorldDropItem` 从 P11/P6 truth 重验 exact source，派生 P31 WorldDropId/Ordinal/container；一个 P1 whole-root `Move` 从 exact P21 equipment slot 直达新 world container slot 0。
5. accepted composite 以相同 TransactionId/source/target/revision 重放一次并要求 snapshot 完全相等；随后分区为 remaining P11 body graph 与 P6 graph，插入一个 provenance=`P44.AcceptedGroundDrop.CorpseStandardEquipment` 的 P31 record，执行一次 Owner record replacement 与函数内唯一一次 `SaveRecord`。
6. 只有 durable success 后，Manager 才重载 P11/P6 projection、更新 exact BodyTarget source，并调用既有 `RefreshCodeBWorldDropActors` 做 accepted-only matching Actor diff。

P44 record 拾回调用图：

- normal Drag：既有 WorldDrop Cell → `PreviewInventoryTransfer` / `FCodeBP4InteractionController` → P2/P1 → `CommitAcceptedMatchedRunWorldDropPickup`；只接受用户明确选择的空 BaseQuick、current identity-valid P17 child 空普通格，或空且 Definition-compatible 的正式装备位。
- Ctrl + 左键：既有 `HandleQuickTransfer` shared pointer router → input-time frozen `CurrentP17Child` 或 `BaseQuickNoChildAtInput` → 既有 P36/P37 delta proof → 同一 P2/P1 与 exact-record cleanup；child 后续失效时拒绝，不回退 BaseQuick、另一 child 或装备位。
- 每个 accepted pickup 只处理 matching `WorldDropId`/Ordinal/revision/container/root；root 离开 exact derived container 后才删除该 record/container/Actor projection，other Registry records 由 `IsWorldDropClosureUnchanged` 保持。

## P21 actual source 与边界

- `Body.Weapon`：definition=`HeavyPracticeBlade`，equip slot=`Weapon`，container type=`CodeB.Body.Weapon`，stable slot 0。
- `Body.ArmorRobe`：definition=`ReinforcedVest`，equip slot=`Armor`，container type=`CodeB.Body.ArmorRobe`，stable slot 0。
- `Body.Accessory0`：definition=`EvasionCharm`，equip slot=`Accessory`，container type=`CodeB.Body.Accessory0`，stable slot 0。
- actual container identity 继续由 `P21BodyEquipmentContainerGuid(BodyTargetId, CodeB.BodyContainer.BasicCorpse, SlotSemantic)` 派生；candidate 必须来自 `CodeB.LootProfile.BasicCorpse.r3` / version 3，并匹配 receipt、profile digest、equipment candidate-set digest 与 materialization digest。
- P44 没有改写 r1/r2/r3、`Optional.EquippedLoadout` 4:1 candidate catalog、weight/digest、already-materialized record、deterministic ItemId/history、三个固定 slot 或 P6→corpse rejection。

exact source gate 同时要求：Open + Revealed、无 search/action；Owner/Run/BodyTarget/death receipt/profile/candidate/open generation/body revision/composite revision/session/focus 一致；root 为 Quantity=1、MaxStack=1、non-stackable、无 child、non-spatial、definition 与 slot semantic/equip slot 一致。ordinary stack、spatial parent/child、Hidden/Searching、player、WorldDrop、another body、unknown family 均不能形成 P44 intent。

## 原子性、回滚与非回归

- corpse→world writer 的静态计数：`ExecuteTransaction` 两处（一次 candidate + 同一命令 replay），`SaveRecord` 一处；没有 P6 staging、Merge、Split、Swap、clone、new ItemId、partial transfer、Actor-first write 或第二物品真值。
- accepted root 的 ItemId、DefinitionId、Quantity=1、Level、Quality、RandomSeed、LegacyAffixDigest、P21 provenance 与 no-child qualification由同一个 P1 Move 保持；P31 identity 仅在 revalidation 后派生。
- 所有 source/placement/collision/P1/replay/partition/P13 reconcile/Registry/SaveRecord 失败都在局部 Candidate 上返回；`Store.Record`、P11 source、P6 Registry、NextWorldDropOrdinal、Actor 与 projection 在成功保存前不变，因此无 ordinal gap、orphan container、phantom root 或 phantom empty slot。
- P44 normal child admission 与 P44 Ctrl admission只扩展既有 canonical standard-equipment source classification；P32/P33/P35 normal semantics、P34/P36/P37 frozen target、P26–P29 simple stack、P30/P41 complete graph、P38/P39 corpse pickup、P42 direct spatial equip、P43 simple drop、P31 other-record isolation均保持。
- P8 仍通过既有 player-only finalization 排除全部 P31 record/root/container；P13 reconcile、P15 use、P17/P19 graph、P5/P6 bridge、Code A terminal authority均未改写。
- successful projection 只更新 exact BodyTarget、新/exact WorldDropTarget、matching Actor 与必要 workspace projection；没有 Sort/Compact、无关 SlotIndex 重排、auto equip、target guess、mode fallback、corpse return 或 record-to-record transfer。

## 静态检查与编译

- `git diff --check`：通过。
- 实际源代码改动：8 个文件；测试文件改动：0。
- P44 corpse writer：1 个 `SaveRecord`；2 个 `ExecuteTransaction` 调用仅表示 accepted candidate 与相同命令 replay。
- 未启动产品、PIE 或 Standalone。

Editor：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
native exit code: 0
Result: Succeeded
Total execution time: 30.14 seconds
```

Game：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
native exit code: 0
Result: Succeeded
Total execution time: 30.06 seconds
```

## 留给 0.0.9B.F 的真实验证

未执行：future BasicCorpse materialize/open/reveal；三个 P21 slot 分别 normal Drag 至 GroundDropZone；multiple existing WorldDrop isolation；新 Actor open/close；P44 normal Drag 至 BaseQuick/current child/compatible equipment；P44 Ctrl 在 valid child 与 no-child BaseQuick mode；child/BaseQuick full、child close/switch/focus/generation/parent/capacity stale；wrong ordinary/spatial/child/Hidden/Searching/player/another-body source；BodyTarget/death receipt/profile/candidate/Owner/Run/P6 revision stale；floor placement/record/ordinal/container/root mismatch；close/reopen、search/action、Prepared/terminal/Host invalid、SaveRecord failure；P38/P39、P43、P26–P31、P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook、Package。

