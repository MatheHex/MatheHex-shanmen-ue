# Dev.D.UE.0.0.9B.P50.0.r0 Report

## 1. 文件与职责

本轮实现提交：`315f967 feat: add P50 BasicCache spatial world drop`。

- 修改 `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`：声明 P50 BasicCache P18 complete-graph GroundDrop 门禁。
- 修改 `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`：复用 P48 immutable source proof；在既有 GroundDropZone 请求中区分 P49 simple stack 与 P50 spatial graph；把 P50 provenance 加入既有 P19 complete-world source classification。
- 修改 `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`：声明 P50 唯一 durable writer。
- 修改 `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：实现 P9/P6/P31 单候选、完整图 P1 relocation、replay、partition、一次保存；把 P50 provenance 加入既有 P19/P30 durable admission。
- 修改 `Source/demo_map/demo_mapV3ProgressionManager.cpp`：复用既有 floor-placement、active Host/lifecycle、durable reload 与 accepted-only Actor refresh，分派 P49/P50 writer。
- 新增 `Docs/Report/Dev.D.UE.0.0.9B.P50.0.r0_report.md`：本报告。
- 未修改 `demo_mapCodeBP2.*`、P18 materialization/catalog/profile/history、P17 graph construction、P31 schema、P13/P15、P5/P6 bridge、P8 terminal/recovery、WorldDrop Actor、GroundDropZone Widget、Code A、测试文件与项目配置；它们继续承担原职责。

## 2. 生产输入与权威调用图

唯一调用图：

`P10 existing parent Cell normal DragOperation`
→ `existing GroundDropZone::NativeOnDrop`
→ `UCodeBP3UIHostSubsystem::RequestGroundDrop`
→ P50 exact source gate（复用 `FCodeBP48NormalContainerSpatialGraphEquipmentTransferProof` 的 source-only 形态）
→ `GroundDropPresentation::RequestDrop`
→ `Ademo_mapV3ProgressionManager::RequestCodeBNormalContainerGroundDrop`
→ existing Code A read-only floor-placement adapter
→ `FCodeBOutOfRaidProfileStore::DropMatchedRunNormalContainerSpatialWorldDropItem`
→ one P1 `Move`
→ one P9/P6/P31 Owner candidate
→ one `SaveRecord`
→ durable reload / exact NormalContainer projection / Actor diff。

没有新增 pointer handler、Button、hotkey、Widget、Actor、第二 resolver 或 Code A inventory 写入口。P47 Ctrl、P48 explicit equipment Drag、P49 simple GroundDrop 仍走原分支。

## 3. P18 provenance 与完整图保持

source gate 同时重验 current P10 Open、无 active action/search、Revealed root、Owner/Run/SearchTarget、ReceiptId、BasicCache definition、target-open generation、P9/P6/composite revision、workspace pane，以及 actual P18 receipt 的 definition/profile/result/materialization digests。仅接受 `CodeB.LootProfile.BasicCache.r2` version 2、`CodeB.DeterministicWeightedLoot.Crc32.r2` 已物质化记录。

root 仅允许 canonical `WindTalisman` 或 `BackpackLevel1`。Store 重验 canonical definition、stable `SpatialChildGuid(ItemId)`、唯一 child owner、正式 child type/capacity、空 child、P19 closure、无嵌套，并原样保留 parent ItemId、child ContainerId、Definition、Quantity 与完整 closure。未改写 P18 r1/r2 gate、weight、candidate、digest、materialization 或历史记录。

## 4. exact source、record identity 与零写入拒绝

UI、Manager 与 Store 逐层重验 exact source address、Host、active Owner/Run lifecycle、P9/P6 revision、actual receipt/profile/digest、Open/Revealed、workspace focus、合法 map route/floor transform。Store 派生并碰撞检查 `NextWorldDropOrdinal`、WorldDropId 与 world container；新 record 固定 OwnerId、RunInstanceId、Ordinal、route/floor、Available、root、child、record revision 与独立 provenance：

`P50.AcceptedGroundDrop.BasicCacheSpatialCompleteGraph`

所有验证和 P1 execution 都发生在内存 Candidate/Repository 上；持久化前的 `PriorSession`、P9 snapshot 与 composite 即 BeforeSnapshot。任一 stale、closure、floor、collision、reconcile、projection 或 save gate 失败均在唯一 `SaveRecord` 前返回，P9、P6 Registry、ordinal、Actor 与 other records 零写入。

## 5. 单事务、replay、partition 与边界

- 新建一个 derived world container，执行一次 whole-root P1 `Move`（`Quantity=0`），P1 随 parent 保持其 child closure；没有 P6 staging、Merge、Split、Swap、clone、new ItemId 或 new child。
- accepted composite 必须证明 parent 离开 exact P9 slot、进入新 world container slot 0，child Container 与 closure 不变；P6、existing WorldDrops、P9 unrelated containers/items 逐项保持。
- 使用相同 `FCodeBTransactionRequest`（相同 TransactionId/source/target/revision）重放一次，要求 replay snapshot 与 accepted composite 完全相等。
- accepted composite 只 partition 为 residual P9 与 P6/P31：P9 移除 parent/child，P6/P31 接收 world container/root/child；两侧各自重新通过 P1 snapshot validation。
- Candidate 只插入一个 P31 record、ordinal 只递增一次、P13 仅调用既有 reconcile、Owner document 只替换一次、函数内只有一次 `Store.SaveRecord(Candidate, Error)`。
- accepted durable reload 后才更新 exact NormalContainer projection 与 Actor diff；未修改 selection、scroll 或 unrelated stable SlotIndex。
- P8 player-only filtering 继续按既有 WorldDrop Registry/closure 结构排除所有未拾回 world records；P50 未改 P8 产品逻辑。

## 6. 既有 P19/P30 拾回衔接

P50 没有专用 pickup input、pointer router 或 resolver，仅把 exact provenance 加入既有 canonical complete-world-root admission：

- normal pickup 继续由 P19 验证 current opened exact record、closure 与明确目标；允许用户明确选择的空 BaseQuick ordinary cell，或 definition-compatible 空 `SpatialRing` / `Backpack`；existing policy 继续拒绝 child、占用/错误目标、auto equip、target guess 与 fallback。
- Ctrl pickup 继续由 P30 shared router 与 frozen `BaseQuickOnly` first-empty stable SlotIndex policy 处理；不重扫、不装备、不进入 current child。
- durable pickup 仍由 `CommitAcceptedMatchedRunWorldDropPickup` 完成一个 whole-graph relocation、exact record cleanup、一个 Owner replacement 与一次 save；P50 只增加 provenance admission。

## 7. 非回归结论

静态审查确认 P47/P48/P49 分流不变；P19/P30 只扩展 accepted provenance；P43—P45、P26—P29、P31 other-record isolation、P5/P6/P8/P13/P15/P17/P18 与 Code A authority 未改。P10 simple stack、P18 child、corpse、existing WorldDrop、player source、another NormalContainer、Hidden/Searching 与 unknown family 均无法建立 P50 intent。无 quantity/child draft、QuickTransfer drop、right-click、double-click、Actor direct pickup、second truth 或第二保存路径。

## 8. 静态检查与明确未实现范围

- `git diff --check`：exit code `0`，无 whitespace error。
- 实际实现范围仅为上列 5 个 C++ 文件及本 Report；未修改测试、Code A、schema、profile/history 或项目配置。
- 未实现 partial graph drop、simple-stack P50、P6→BasicCache return、quick-drop、Actor direct pickup、Take All、auto target/equip/open child、record-to-record transfer、Swap、Replacement、Sort、Compact、Bind、Use、网络或多人。

## 9. 原生编译

Editor：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- native exit code：`0`
- 结果：`Result: Succeeded`；生成 `UnrealEditor-demo_map.dll`。

Game：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- native exit code：`0`
- 结果：`Result: Succeeded`；生成 `demo_map.exe`。

## 10. 留给 0.0.9B.F 的真实验证

本 P 阶段未执行产品启动、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook 或 Package。F 阶段需覆盖：future BasicCache materialize/open/reveal；canonical WindTalisman/BackpackLevel1 各自 normal Drag 到 GroundDropZone；multiple existing WorldDrop isolation；Actor open/close；P50 normal pickup 到 BaseQuick/compatible SpatialRing/Backpack；P30 Ctrl 到 BaseQuickOnly；empty/full/wrong slot；child closure/topology stale；simple/child/equipment/Hidden/Searching/player/another-normal source；receipt/profile/candidate/Owner/Run/P6 revision stale；floor/record/ordinal/container/root/child mismatch；close/reopen、search/action、Prepared/terminal/Host invalid/SaveRecord failure；P19/P30/P47/P48/P49、P43—P45、P26—P31、P8 terminal/recovery；以及真实输入、PIE、Standalone、截图、Smoke、Automation、回归、Cook、Package。

READY_FOR_P51_PLANNING
