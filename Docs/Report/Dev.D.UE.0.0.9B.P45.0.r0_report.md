# Dev.D.UE.0.0.9B.P45.0.r0 Report

## 结论

READY_FOR_P46_PLANNING

P45 已静态闭合：当前已打开、已揭示且 identity-valid 的 P20 `WindTalisman`／`BackpackLevel1` complete graph 可经既有 normal Drag 与既有 `GroundDropZone` 整图落地为一个独立 P31 record；parent、唯一 child container 与 child 全部 contents 保持同一 P1 图身份。该 record 仅以 canonical P45 provenance 接入既有 P19 normal pickup 与 P30 `BaseQuickOnly` Ctrl QuickTransfer。Editor 与 Game 均以 native exit code 0 编译完成。未启动产品，未执行 F 阶段真实验证。

## 文件范围

修改：

- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`：声明 P45 source gate、P45 provenance gate 与 shared P19 complete-graph provenance gate。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`：复用 P42 source-only proof 验证 exact P20 parent/child closure；在既有 `RequestGroundDrop` 接入 P45；限制 P31/P45 normal pickup 为明确空 BaseQuick 或匹配空 SpatialRing／Backpack，并把 P45 接入现有 P30 shared router。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`：扩展既有 P43/P44 corpse-to-world sole writer，使其恰好接受 P43、P44、P45 三类 proof 之一。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：新增 P45 canonical closure/source revalidation、一个 P1 whole-graph candidate/replay、P11/P6 closure partition、独立 P45 provenance 与 P19/P30 exact admission。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：既有 body GroundDrop adapter 选择 P43/P44/P45 exact proof，继续复用同一 floor placement、durable callback 与 accepted-only projection/Actor refresh。
- `Docs/Report/Dev.D.UE.0.0.9B.P45.0.r0_report.md`：本报告。

明确未修改：

- 测试文件、P1 transaction core、P2/P3/P4 input core、P11/P12 search lifecycle、P20/P21 materialization/receipt/history、P17 graph construction、P31 schema、P8 finalization、P13 reconcile、P5/P6 bridge、Code A inventory/Loot authority、Actor 类与 Widget 类均未修改。
- 未新增 Button、hotkey、pointer handler、Widget、Actor、second resolver、第二 Repository transaction、第二 save、ItemId/ContainerId/WorldDropId allocator 或 Code A 写入口。

实现提交：`08d1f09 feat: add P45 corpse spatial graph ground drop`。

## 调用图与权威边界

尸体 complete graph 落地：

1. 既有 P12 ordinary P20 root `Cell` → 既有 `UCodeBP4DragOperation`；`PopulateTransferContext` 已填充 P42 source-only proof：Owner/Run、BodyTarget、death receipt、open/body/composite revision、exact ordinary slot/item/definition、stable child identity、profile/result/materialization digest。
2. 既有 `UCodeBP3GroundDropZone::NativeOnDrop` → `HandleGroundDropZoneDrop` → `UCodeBP3UIHostSubsystem::RequestGroundDrop`；P45 清除 generic current-child target decoration，保持 P42 source proof 不带 equipment intent/frozen target，并以 `ValidateP45BodySpatialGraphGroundDropContext` 重验 Open/Revealed、receipt/profile、parent、唯一 child owner、one-layer/no-nested closure 与 focus/revision。
3. 既有 `FCodeBP3GroundDropPresentation::RequestDrop` → `Ademo_mapV3ProgressionManager::RequestCodeBBodyGroundDrop`；adapter 只解析活动 route/floor transform 与 Host 生命周期，不生成物品或 record identity。
4. `FCodeBOutOfRaidProfileStore::DropMatchedRunBodyContainerWorldDropItem` 从 P11/P6 durable truth 再验 canonical Definition、`SpatialChildGuid(root ItemId)`、child type/capacity、全部 child contents、reverse pointer 与无嵌套；随后派生 P31 WorldDropId/Ordinal/container，执行一个 P1 `Move(0)` 从 exact P11 ordinary slot 直达新 world container slot 0。
5. accepted composite 以相同 source/target/revision 重放并要求 snapshot 相等；parent、child container 与 child item逐项保持。随后把完整 closure 从 P11 分区到 P6 world graph，插入 provenance=`P45.AcceptedGroundDrop.CorpseSpatialCompleteGraph` 的单一 P31 record，只执行一次 Owner record replacement 与函数内唯一一次 `SaveRecord`。
6. durable success 后 Manager 才重载 P11/P6 projection、更新 exact BodyTarget，并调用既有 `RefreshCodeBWorldDropActors` 做 accepted-only matching Actor diff。

没有 Widget direct Move、Actor direct pickup、尸体缓存写入、right-click Take、double-click、预建 durable identity、record 顺序猜测、第二 transaction/save 或 Code A writer。

## Source gate、图保持与失败策略

- source 只接受 current opened/revealed exact P12 ordinary root，Definition 为 canonical `WindTalisman` 或 `BackpackLevel1`，Quantity=1、non-stackable、MaxStack=1，并匹配 P20 r2 或继承 P21 r3 的 receipt/profile/digests。
- proof 与 durable Commit 均绑定 OwnerId、RunInstanceId、BodyTargetId、death receipt、BodyTarget/open generation/body/composite/P6 revision、ordinary container/slot/root ItemId、Definition、child container 与 workspace target pane。
- authoritative store 使用既有 `ValidateP19WorldDropClosure` 验证 `SpatialChildGuid(root ItemId)`、formal QuickRing/StoragePouch semantic、canonical capacity、唯一 parent/reverse pointer、child placement order、全部 child items无嵌套/无孤儿；P1 move 与 snapshot replay保持 ItemId、ContainerId、Definition、Level、Quality、RandomSeed、LegacyAffixDigest、quantity 与 contents。
- P43 simple stack、P44/P21 equipment、child item、Hidden/Searching、player/WorldDrop/P9/P14/P31 source、another BodyTarget、wrong profile/digest、stale revision/open/focus/session、invalid floor、collision、P1/replay/partition/reconcile/save failure全部在局部 Candidate 上拒绝；成功保存前 P11、P6 Registry、Ordinal、Actor 与 projection 不变，无 partial move、ordinal gap、orphan、duplicate child owner 或 phantom root。

## P19/P30 拾回衔接

- P45 record 继续使用既有 opened WorldDrop Cell、P4 preview/commit、P2/P1 与 `CommitAcceptedMatchedRunWorldDropPickup`；没有 P45 专用输入或 resolver。
- shared complete-graph classification 只接受 provenance=`P31.AcceptedGroundDrop.CompleteGraph` 或 exact P45 provenance；P45 不伪装为 player source、simple stack 或 standard equipment。
- normal Drag 只允许用户明确选择的空 P6 BaseQuick ordinary cell，或 Definition 唯一匹配的空正式 SpatialRing／Backpack slot；禁止 current P17 child、occupied target、Swap、auto equip、target guess、replacement 与 fallback。
- Ctrl + 左键继续由现有 `HandleQuickTransfer` 与 P30 structural delta proof处理，只冻结 stable SlotIndex 升序的 first-empty BaseQuick，`BaseQuickOnly` 且 `Move(0)`；不进入 child 或装备位，失效时不重扫。
- pickup durable callback 继续绑定 exact Owner/Run/WorldDropId/Ordinal/record revision/derived container/root/child/P6 revision。只有完整图离开 exact world container 后才清理该 record/container；`IsWorldDropClosureUnchanged` 保持其他 P31 records。
- P8 既有 player-only finalization 按 Registry closure 排除全部 WorldDrop root/container/child contents；P45 未修改 terminal、recovery 或 Code A authority。

## 静态检查与编译

- `git diff --check`：通过。
- 实际源代码改动：5 个文件；测试文件改动：0。
- P45 corpse writer：1 个 `SaveRecord`；2 个 `ExecuteTransaction` 调用仅表示一个 accepted candidate 与相同命令 replay。
- 未启动产品、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook 或 Package。

Editor：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
native exit code: 0
Result: Succeeded
Total execution time: 7.26 seconds
```

Game：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
native exit code: 0
Result: Succeeded
Total execution time: 12.67 seconds
```

## 留给 0.0.9B.F 的真实验证

未执行：future BasicCorpse materialize/open/reveal；canonical WindTalisman 与 BackpackLevel1 complete graph 各自 normal Drag 至 GroundDropZone；包含 child contents 的完整图保持；multiple existing WorldDrop isolation；新 Actor open/close；P45 normal Drag 至 BaseQuick／compatible SpatialRing／Backpack；P45 Ctrl 至 BaseQuickOnly；empty/full/wrong BaseQuick/slot、child closure/topology/reverse-pointer stale；wrong simple/equipment/child/Hidden/Searching/player/another-body source；BodyTarget/death receipt/profile/candidate/Owner/Run/P6 revision stale；floor placement/record/ordinal/container/root/child mismatch；close/reopen、search/action、Prepared/terminal/Host invalid、SaveRecord failure；P19/P20/P30/P41/P42、P43/P44、P26–P31、P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

