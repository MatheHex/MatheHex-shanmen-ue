# Dev.D.UE.0.0.9B.P6.0.r2 Report

## 结论

`NEEDS_P6_REWORK`

P6 r2 已完成规定的真实产品入口审计和最小可执行核查；没有运行 P1--P6 全量回归、截图巡检、Game 构建或 P7/P8/F 工作。六个可独立完成的产品场景通过。`PreparedRecovery` 在真实 CTA 已创建同一 Owner/RunId 的 `Prepared` receipt 后，按照 r2 禁令没有再直接调用 observer；因此不能证明“第二次真实 CTA 恢复同一 receipt”。这不是伪造的失败：现有 Code A 重启初始化会把旧 ActiveRun 结算为 `RecoveredAbandon`，下一次 CTA 取得新的 RunId；而未关闭的 P6 session 对不同 RunId 明确返回 `ActiveSessionConflict`。

另一个需要策划裁决的审计结果是：进程内确实记录 `RequestExitWithStatus(1)`，但 `UnrealEditor-Cmd.exe` 对该失败场景的父 PowerShell 真实退出码仍为 `0`。报告如实保留二者，未将其写成成功。

## Prompt、边界与变更

- 已完整归档下载的 `Dev.D.UE.0.0.9B.P6.0.r2_prompt.md`，SHA-256 为 `4DBEEAD326DED6302DE783CC309144FE635E3483CAF2126C9047795657086B47`。
- r0 建立 Owner sidecar / Prepared--Committed receipt；r1 首次接到真实 CTA 链路，但 Prepared recovery 仍直接重放 observer；r2 仅加入开发测试 trace、物品映射与审计文档，不改变 Code A 生命周期、P5/P6 正常语义、P7/P8 或 F。
- r2 修改：`Source/demo_map/demo_mapV3ProgressionManager.h/.cpp`、`PROJECT.md`、`PROJECT_INFO_CARD.md`、本报告。新增逻辑都在 `!UE_BUILD_SHIPPING` / `WITH_DEV_AUTOMATION_TESTS` 的产品审计入口内。
- r2 的 `-P6ProductStartBridgeR2Trace` 使用既有 `SectTeleportCTA` 的真实点击。它记录 CTA、`StartPreparedRunDirect`、world activation、observer、bridge final 和 Start Run 返回；每条记录带 OwnerId、RunInstanceId、Owner document revision、Run session revision、receipt state、bridge status 和受控退出请求。

## Code A / Code B 边界审计

- `demo_mapSectNavigationWidget.cpp:1441` 的正式传送 CTA 进入 `StartPreparedProfileRunFromSect`；`demo_mapV3ProgressionManager.cpp:3474` 才调用 `StartPreparedRunDirect`。
- r2 trace 只在成功的 `ActivatePreparedProfileWorld` 后（`demo_mapV3ProgressionManager.cpp:3508`）进入唯一的 `ObserveCodeBRunAfterActivation`；该 observer 在 `:3550` 传入稳定 `ProfileId + ActiveRunId` 调用 `NotifySuccessfulRun`。
- `demo_mapCodeBOutOfRaidProfile.cpp:1534--1539` 保持既有契约：未关闭 P6 session 收到不同 RunId 时返回 `ActiveSessionConflict`，没有修改该语义。
- `demo_mapProfileSessionCoordinator.cpp:86--104` 的启动恢复对持久化 ActiveRun 使用 `RecoveredAbandon`。因此进程重启后真实 CTA 无法再取得原 RunId；此项和 P6 active-session guard 共同阻止了 r2 所要求的“第二 CTA、同 RunId、无直接 observer”恢复。
- r1 的直接 `ObserveCodeBRunAfterActivation(Active)` 仍作为 r1 兼容分支保留在 `demo_mapV3ProgressionManager.cpp:2270`；r2 trace 在它之前的 `:2259--2266` 记录 `PreparedReceiptAwaitingSecondProductCTA` 并以失败退出，故本次 r2 实际执行从未走该直接调用。
- r2 未改 Player Actor、地图、Loot、搜索、Hotbar、消耗、结算、Run Save、P1/P2/P3/P4 控制器/Widget/Store/Repository；P7 继续只能读取 P6 snapshot，P8 才能定义回收和结算。

## 构建

最终增量命令：

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' demo_mapEditor Win64 Development '-Project=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' -WaitMutex -FromMsBuild -NoHotReloadFromIDE -MaxParallelActions=1
```

结果：`Succeeded`，4 actions，14.17 s。只为 r2 审计代码执行此 Editor 编译；没有执行全量或截图测试。

## 真实产品链路证据

所有命令都以独立 Profile 根运行，并由测试内的 `FPlatformMisc::RequestExitWithStatus` 作为受控退出机制。命令外层记录的真实退出码为 `0`；PreparedRecovery 的进程内失败请求为 `1`，但外层仍为 `0`，这是本 Report 的未通过项而非通过项。

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' '/Game/M01/Maps/L_M01_Expedition?Name=Player' -game -windowed -ResX=1280 -ResY=720 -P6ProductStartBridgeR2Trace '-P6ProductStartBridgeStorageRoot=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Automation\Dev.D.UE.0.0.4.14.r0\P6ProductStartBridge\R2\CompleteCarry_20260806_1600' '-P6ProductStartBridgeCase=CompleteCarry' -unattended -nop4 '-abslog=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\P6r2.ProductStart.CompleteCarry.20260806_1600.log'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' '/Game/M01/Maps/L_M01_Expedition?Name=Player' -game -windowed -ResX=1280 -ResY=720 -P6ProductStartBridgeR2Trace '-P6ProductStartBridgeStorageRoot=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Automation\Dev.D.UE.0.0.4.14.r0\P6ProductStartBridge\R2\Empty_20260806_1600' '-P6ProductStartBridgeCase=Empty' -unattended -nop4 '-abslog=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\P6r2.ProductStart.Empty.20260806_1600.log'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' '/Game/M01/Maps/L_M01_Expedition?Name=Player' -game -windowed -ResX=1280 -ResY=720 -P6ProductStartBridgeR2Trace '-P6ProductStartBridgeStorageRoot=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Automation\Dev.D.UE.0.0.4.14.r0\P6ProductStartBridge\R2\NotEnrolled_20260806_1600' '-P6ProductStartBridgeCase=NotEnrolled' -unattended -nop4 '-abslog=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\P6r2.ProductStart.NotEnrolled.20260806_1600.log'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' '/Game/M01/Maps/L_M01_Expedition?Name=Player' -game -windowed -ResX=1280 -ResY=720 -P6ProductStartBridgeR2Trace '-P6ProductStartBridgeStorageRoot=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Automation\Dev.D.UE.0.0.4.14.r0\P6ProductStartBridge\R2\BridgeFailure_20260806_1600' '-P6ProductStartBridgeCase=BridgeFailure' -unattended -nop4 '-abslog=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\P6r2.ProductStart.BridgeFailure.20260806_1600.log'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' '/Game/M01/Maps/L_M01_Expedition?Name=Player' -game -windowed -ResX=1280 -ResY=720 -P6ProductStartBridgeR2Trace '-P6ProductStartBridgeStorageRoot=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Automation\Dev.D.UE.0.0.4.14.r0\P6ProductStartBridge\R2\PreparedRecovery_20260806_1600' '-P6ProductStartBridgeCase=PreparedRecovery' -unattended -nop4 '-abslog=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\P6r2.ProductStart.PreparedRecovery.20260806_1600.log'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' '/Game/M01/Maps/L_M01_Expedition?Name=Player' -game -windowed -ResX=1280 -ResY=720 -P6ProductStartBridgeR2Trace '-P6ProductStartBridgeStorageRoot=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Automation\Dev.D.UE.0.0.4.14.r0\P6ProductStartBridge\R2\ActiveSessionConflict_retry_20260806_1600' '-P6ProductStartBridgeCase=ActiveSessionConflict' -unattended -nop4 '-abslog=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\P6r2.ProductStart.ActiveSessionConflict.retry.20260806_1600.log'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' '/Game/M01/Maps/L_M01_Expedition?Name=Player' -game -windowed -ResX=1280 -ResY=720 -P6ProductStartBridgeR2Trace '-P6ProductStartBridgeStorageRoot=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Automation\Dev.D.UE.0.0.4.14.r0\P6ProductStartBridge\R2\OutOfRaidLock_retry_20260806_1600' '-P6ProductStartBridgeCase=OutOfRaidLock' -unattended -nop4 '-abslog=C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Saved\Logs\P6r2.ProductStart.OutOfRaidLock.retry.20260806_1600.log'
```

| 场景 | 真实链路与结果 | 受控 / 外层退出 |
| --- | --- | --- |
| CompleteCarry | CTA、Code A activation、observer、bridge 都使用 `56503318-4028-8945-EF60-588D1DA40BED`；Owner document revision `2 → 4`，session revision `2`，receipt `Prepared(0) → Committed(1)`，status `Committed(0)`。 | request `0` / actual `0` |
| Empty | 同一条真实链路，空 P5 snapshot 生成空 committed Run snapshot；revision `2 → 4`。 | request `0` / actual `0` |
| NotEnrolled | Code A 正常 active，bridge `NotEnrolled(3)`，前后都没有 Code B record。 | request `0` / actual `0` |
| BridgeFailure | Code A 正常 active；fault hook 后 receipt 为 `Prepared(0)`、Owner document revision `2 → 3`、session revision `1`、status `StorageFailure(6)`；P5 source 未抽取。 | request `0` / actual `0` |
| PreparedRecovery | 第一 CTA 的 RunId 为 `597B0648-45A1-82D0-9232-4281724AAF0D`，留下 verified Prepared receipt（revision `2 → 3`）；r2 没有直接 observer 或第二 CTA 的伪造路径，因此停止并报告阻塞。 | request `1` / actual `0` |
| ActiveSessionConflict | 真实 CTA 获得新 RunId，bridge 返回 `ActiveSessionConflict(4)`；旧 active session 仍为 revision `2`，Owner document revision `4 → 4`。 | request `0` / actual `0` |
| OutOfRaidLock | CTA 后 bridge committed；正式宗门仓库入口返回精确文本 `当前 Run 中，返回后再整理`，并记录 `PersistentWrite=0`。 | request `0` / actual `0` |

### CompleteCarry 的逐项映射

`P6R2_ITEM_MAPPING` / `P6R2_BASIC6` 记录表明每个 ItemId 在 P5 before 与 Run snapshot 中相同、选中项在 P5 after 中不存在，仓库余项只留在 P5：

- Weapon `A462DDF5-4944-04AA-57EF-73AEB20D3297`、Armor `07BEC879-4C2D-677C-4710-1FBBC0DD5B8C`、Accessory0 `5A037284-4A17-85E4-2E33-87B2D165253C`：各自装备 container/slot `0` 不变。
- SpatialRing `0BF01105-4850-DB1D-F06C-AC906E8DBF00` 的 ChildContainer 为 `51FC1105-4850-1018-4061-A9903EDFF046`；其 child `A7F99DB0-435D-AF5C-BEF9-1D97B2CF223D` 仍在该 ChildContainer slot `0`。
- Pouch `BA7C8D39-45AB-13B0-D50B-B3A1D342C272` 的 ChildContainer 为 `E0708D39-45AB-D8B5-6506-B6A183108D34`；其 child `93D855DB-4DBC-10FC-6E6E-0DB336A100AC` 仍在该 ChildContainer slot `0`。
- Basic6：slot `0` 的 SpiritDust `B9FD81EB-4932-505C-A930-2E99212120A0` 被携带；slot `1--5` 在 P5 before 和 Run snapshot 都为空。
- Warehouse-only `214EC197-4801-E5D4-05AE-ABA3D9115435` 保持 P5 warehouse parent `853854C9-F900-70D8-1E9D-D5A3E2113301`、slot `8`，Run 不存在该项。
- 正式 Code A `Fdemo_mapPersistentPreparationLayout` 当前只有一个 `AccessoryItemInstanceId`；因此本真实 Profile 的“所有饰品”是已选择的 Accessory0，未伪造不存在的第二个 Code A 饰品选择字段。

## 需要 P6 重工的最小决策

1. 定义 **真实第二 CTA** 如何在不直接 observer 的前提下恢复同一 P6 receipt：保留/重新进入原 Code A RunId，或在 P6 中明确定义可审计且安全的新 RunId 恢复语义。当前两者都与既有 `RecoveredAbandon` + different-Run refusal 冲突。
2. 为失败场景定义一个能让外层进程返回非零的受控 test-exit wrapper（或明确接受 UnrealEditor-Cmd 的真实 `0`）；不能以日志中的 request code 替代实际退出码。
3. r1 compatibility direct-observer recovery 不能作为 r2/P6 最终产品证据；在上述契约明确后，用唯一真实 CTA 入口重新验证 PreparedRecovery。其余验证仍留到 `0.0.9B.F`。

## 未做事项与过程备注

- 未做 P1--P6 全量回归、真实 UI 截图、Game 编译、P7、P8、F 或任何非 P6 语义改动。
- 一次错误隔离根的 `ActiveSessionConflict` 初次启动未生成有效 P6 trace 且未自行退出；已停止该精确测试进程，结果完全排除，随后以正确项目内隔离根成功重跑。没有将该无效运行计入场景结果。
