# Dev.D.UE.0.0.9B.P49.0.r0 Report

## 1. 文件范围

- 新增：`Docs/Report/Dev.D.UE.0.0.9B.P49.0.r0_report.md`，本轮可审计交付报告。
- 修改：`Source/demo_map/CodeB/demo_mapCodeBP2.h`（P49 source proof）；`demo_mapCodeBP4.h`（drag payload 携带 proof）；`demo_mapCodeBP3UI.h/.cpp`（P10 proof 冻结、GroundDrop pre-commit gate、P6 revision projection refresh）；`demo_mapCodeBOutOfRaidProfile.h/.cpp`（唯一 P9→P31/P6 durable writer）；`Source/demo_map/demo_mapV3ProgressionManager.h/.cpp`（P10 GroundDrop callback、lifecycle/floor gate、durable reload 与 Actor refresh）。
- 本轮 Prompt 已原字节归档为 `Docs/Prompt/Dev.D.UE.0.0.9B.P49.0.r0_prompt.md`。
- 明确未修改：P1 transaction 实现、P9/P18 materializer、P26—P29 pickup resolver、P31 schema、P8 terminal finalizer、P5/P6/P13/P15/P17、Code A、PROJECT.md、PROJECT_INFO_CARD.md、地图/资产/Widget/Actor 类及测试文件。

## 2. 调用图与入口唯一性

`P10 ordinary Cell normal drag` → `UCodeBP3GroundDropZone::NativeOnDrop` → `UCodeBP3InventoryWidget::HandleGroundDropZoneDrop` → `UCodeBP3UIHostSubsystem::RequestGroundDrop` → `Populate/ValidateP49NormalContainerSimpleStackGroundDropProof` → `Ademo_mapV3ProgressionManager::RequestCodeBNormalContainerGroundDrop` → `FCodeBOutOfRaidProfileStore::DropMatchedRunNormalContainerWorldDropItem` → one P1 `Move(Quantity=0)` → one P9/P6/P31 Owner replacement → `UpdateNormalContainerProjection`/`RefreshCodeBWorldDropActors`。

复用了既有 GroundDropZone、P3 Host、P1 repository、P31 registry 与 Actor projection；未新增 raw pointer、Widget、Actor、second resolver、Code A 写入口或 P6 staging 路由。Actor 仅在 SaveRecord 成功并重载 durable session 后刷新。

## 3. P10 actual stable source 与历史保持

Proof 仅由 current Open、无 active action/search、Revealed 的 exact `CodeB.NormalContainer.BasicCache` ordinary root 建立；限定 canonical `SpiritDust`/`IronShard`、stackable、MaxStack>1、无 child/spatial/equipment，冻结 ItemId、DefinitionId/StackKey、Quantity/MaxStack、ContainerId/SlotIndex、Owner/Run、SearchTargetId、ReceiptId、Definition/Profile/Algorithm/Result/Materialization digest、P9/P6/composite revision、target-open generation 与 workspace focus。

Commit 再读 durable P9 record/receipt/snapshot 并逐项重验。未调用或修改 P9/P18 materialization，因此不 reroll、不补料、不迁移 schema、不改 receipt/history/determinism；actual 已物质化 profile/version/digest 原样保留。

## 4. exact gate、失效与零写入

UI 与 Manager 双层要求 exact Host、`InRun.External` focus、NormalContainerTarget、active Owner/Run lifecycle、P9/P6 revision、source slot/reveal/definition/quantity。Manager 在 commit 前解析当前 map route 与 blocking floor transform；Store 再验 session、record、receipt、registry identity/collision 和 finite placement。

close/reopen generation、focus、search/action、Owner/Run、receipt/root/container、P9/P6/composite revision、source shape、floor、ordinal/record collision 或 Host/lifecycle 失效均直接拒绝；candidate 仅在内存中构造，SaveRecord 前不改 durable state、不建 Actor、不选择 fallback。SaveRecord 失败不发布 projection/Actor。

## 5. 原子事务、replay 与边界

- 从 exact P9 slot 对同一 ItemId 执行一次 P1 whole-root `Move`，Quantity=0，直接进入 derived independent WorldDrop container slot 0；无 Merge/Split/Swap/clone/new ItemId/P6 staging。
- accepted root 除 ParentContainerId/SlotIndex 外与 source instance 全等；Definition/StackKey/Quantity/Level/Quality/RandomSeed/LegacyAffixDigest 与无 child qualification 保持。
- 插入一个 provenance=`P49.AcceptedGroundDrop.BasicCacheOrdinarySimpleStack` 的独立 P31 record；Owner/Run/WorldDropId/Ordinal/derived ContainerId/root/revision/route/floor/Available 均由 durable candidate 固定。
- 使用相同 TransactionId/source/target/revision 重放一次 P1 Move，要求 replay snapshot 与 accepted composite 完全相等；既有 P6、P9 非来源对象及既有 WorldDrop closure 必须逐项不变。
- P9 residual 与 P6/P31 player snapshot 独立通过 P1 load validation；P13 hotbar reconciliation、payload receipt、session validation 后只调用一次 SaveRecord。P8 player-only terminal finalizer未改，仍按既有规则排除全部 world record/container/root。
- UI selection、scroll、stable SlotIndex 规则未改；刷新只发生在 durable accepted 后。

## 6. P26—P29 接入

新 record 使用既有 canonical simple world root 结构：独立 WorldDrop container、slot 0 root、Available record、非 spatial closure。`CommitAcceptedMatchedRunWorldDropPickup` 仍只通过 `IsP26SimpleStack` 与 current exact record/Owner/Run/revision/source address 判定，所以既有 P26 full pickup、P27 explicit empty-target N、P28 compatible-stack N、P29 frozen QuickTransfer 可直接接纳。未修改 resolver，未增加 P49-special input、BasicCache return、fallback、自动目标、自动拾取或第二 record。

## 7. 非回归静态结论

P10 normal Move/Merge/Swap、P18 spatial graph、P46/P47/P48、P26—P31、P38—P45 代码路径保持；新增 proof 与既有 body/spatial/equipment proof 互斥。P5/P6/P8/P13/P15/P17、terminal/recovery 和 Code A authority 未扩展。P49 只增加 BasicCache simple source 到既有 P31 world truth 的受限入口。

## 8. 静态检查与未实现范围

`git diff --check`：通过，无 whitespace error。实际实现提交：`ae990db feat: add P49 BasicCache world drop`，8 个 Source 文件，858 insertions / 3 deletions。

按 P 阶段边界未实现：partial/quantity GroundDrop、BasicCache return、spatial/equipment/child/corpse/player/another-normal source、自动 resolver/Actor、P50、Fix、F 测试、地图/资产/UI 新建及 Code A 改动。

## 9. 编译

1. `Build.bat demo_mapEditor Win64 Development C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject -WaitMutex -NoHotReload`：native exit code 0，`Result: Succeeded`，生成 `UnrealEditor-demo_map.dll`。
2. `Build.bat demo_map Win64 Development C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject -WaitMutex -NoHotReload`：native exit code 0，`Result: Succeeded`，生成 `Binaries/Win64/demo_map.exe`。

未启动产品、PIE 或 Standalone。

## 10. 留待 F 阶段的真实验证

未执行：BasicCache materialize/open/reveal；`SpiritDust`、`IronShard` 各自 normal Drag→GroundDropZone；multiple existing WorldDrop isolation；new Actor open/close；P49 record P26 full、P27/P28 partial、P29 QuickTransfer；empty/full/wrong player target、partial quantity；wrong spatial/equipment/child/corpse/player/another-normal/Hidden/Searching source；NormalContainerTarget/SearchTargetId/receipt/Owner/Run/P9/P6 stale；floor/record/ordinal/container/root mismatch；close/reopen、search/action、Prepared/terminal/Host invalid/SaveRecord failure；P10 Move/Merge/Swap、P46/P47/P48、P26—P31、P38—P45、P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook、Package。

READY_FOR_P50_PLANNING
