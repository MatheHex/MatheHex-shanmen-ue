# Dev.D.UE.0.0.9B.P4x.0.r2 Report

```text
task_id = Dev.D.UE.0.0.9B.P4x.0.r2
project_id = Dev.D.UE.0.0.9B
final_status = READY_FOR_CODE_B_OUT_OF_RAID_ORGANIZATION
prompt = Docs\Prompt\Dev.D.UE.0.0.9B.P4x.0.r2_prompt.md
prompt_sha256 = E8C2243A148910FABAF38CFE60A0BCBD97D7E85BC93CE644A9AFC49122E5B951
report = Docs\Report\Dev.D.UE.0.0.9B.P4x.0.r2_report.md
activity_root = C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
baseline_root = C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
engine = UE 5.8
preceding_accepted_task = Dev.D.UE.0.0.9B.P4.0.r1
execution_date = 2026-08-05
```

## 1. 裁决与候选边界

`P4x.0.r0` 是有价值但未接受的只读候选证据：它未被删除、重命名、回滚或改写，也没有被事后改称为 r2。本报告只记录本 Prompt 所要求的 r2 身份、当前构建、当前日志、当前截图和当前源码审计。

本轮没有开始 P5。Code B 仍是默认关闭、显式打开的开发态纵向切片；它没有接管正式 Profile、Run、SaveGame、Loot、地图、玩家 Actor 或默认 UI。

用户反馈的“战斗结束后装备/武器显示为初始化”已通过代码路径核对：截图中 `WPN` 等确定性内容属于显式 `CodeB.P3.Open` 后的开发 fixture，不是默认产品仓库或结算后 Profile。最终默认地图 Smoke 也确认未创建 Code B Host/fixture。正式结算/会话回归组均已在本轮通过。

## 2. 实际产品 Start Run

入口是现有产品传送页的可见 `SectTeleportCTA`，由该 CTA 的真实点击委托启动；没有创建 Preparation Widget、没有用 Code B 初始化正式 Run，也没有以旧 `PreparationLayout` 作为首轮 Start Run 门槛。

| 隔离场景 | 首次 Start Run | 返回后再次 Start Run | 旧布局首次保持 | 重启后旧布局 | 备战 Widget / 部署物品 | 结果 |
|---|---:|---:|---:|---:|---:|---|
| Normal | 通过 | 通过 | 1 | 1 | 0 / 0 | PASS |
| NoEquipment | 通过 | 通过 | 1 | 1 | 0 / 0 | PASS |
| MissingPreparation | 通过 | 通过 | 1 | 1 | 0 / 0 | PASS |
| StaleLegacy | 通过 | 通过 | 1 | 0 | 0 / 0 | PASS |

`StaleLegacy` 在首次 Start Run 保持旧失效引用；第二次启动后的 `0` 是终端结算的既有规范化结果，并非启动门槛或隐式装备。每个场景都使用 `Saved\Automation\Dev.D.UE.0.0.4.14.r0\P4xStartRun-r2-current\<case>` 的新隔离根，日志为 `Saved\Logs\P4x.r2.ProductStart.<case>.current.log`。

## 3. Code B：真实输入与写入边界

`CodeB.P4x.RunRealInputTrace [width] [height] [Quit]` 在已挂载的生产 P3/P4 Host 中，以 Cell 的缓存几何、Slate hit-test 和真实鼠标/键盘事件运行；它不直接调用 Widget `NativeOn...`、P3/P4 Controller、P2 或 P1。1280×720 和 1920×1080 两轮均通过。

每条 trace 都输出 Gesture、实际命中 Container/Slot、ItemId、DragOperation、Preview、P2 CommandCount、P2 CallCount、RevisionBefore/After、结果与详情。典型已接受 Drop 的实测记录为 `P2CommandCount=1`、`P2CallCount=1`，revision 恰好加一；选择、双击、右键、Escape、关闭/重开与拒绝 Drop 均保持 `0/0` 和不变 revision。

覆盖并通过：仓库/基础 6 格/QuickSpatial/PouchInternal 的 Move，兼容 Merge，Swap，兼容 Equip，已占用装备栏 Replacement，向明确普通格 Unequip，Loaded Spatial/不兼容/同源/容量等拒绝，详情、双击、右键、Escape、Close/Reopen。空间戒指仅在装备后表现为 `QuickSpatial`；空间储物囊是无快捷栏/热键语义的普通 `PouchInternal`，两者都保持稳定 Child ContainerId 且 Loaded Spatial 原子拒绝。

静态审计结论：

- `UCodeBP3CellButton::NativeOnMouseButtonUp`、双击和右键只做选择/详情/反馈；双击的旧 QuickMove 用户入口不可达。
- `UCodeBP3CellButton::NativeOnDrop` 是挂载 UI 的唯一位置写入路由，调用 `UCodeBP3InventoryWidget::HandleP4Drop`，随后通过 `FCodeBP4InteractionController::CommitDrop` → P3 → P2 → P1 Repository。
- 只有 Preview=Allowed 的真实 Drop 递增本手势的 command/call 计数；拒绝预览仅本地反馈、零写入。
- P2 的唯一提交点为 `Repository.ExecuteTransaction(Request)`；不存在默认产品 UI、双击、右键、详情或隐藏 Console 的成功写入路径。

## 4. 当前可见 Smoke 证据

截图仅作当前可见 Smoke 的佐证；验收主证据是上节的真实输入 trace 与源码写入边界审计，符合“优先代码审查”的要求。所有文件均在本任务运行中由真实挂载 UI/产品入口请求，未引用旧 `Saved\P4Screenshots`。

| 相对路径 | 生成时间（本地） | 分辨率 | 画面 |
|---|---|---|---|
| `Saved\P4xScreenshots\P4x.0.r2_{Initial,DetailNonEquipable,Merge,DetailEquipable,DetailEquipped,RingEquippedQuickSpace,RejectLoadedRing,PouchNonQuick,DragHighlight}_1280x720.png` | 2026-08-05 19:22:29–19:22:38 | 1280×720 | 生产挂载 UI 的真实输入状态 |
| `Saved\P4xScreenshots\P4x.0.r2_{Initial,DetailNonEquipable,Merge,DetailEquipable,DetailEquipped,RingEquippedQuickSpace,RejectLoadedRing,PouchNonQuick,DragHighlight}_1920x1080.png` | 2026-08-05 19:23:01–19:23:10 | 1920×1080 | 同一真实链路的高分辨率状态 |
| `Saved\P4xScreenshots\P4x.0.r2_ProductStart_Normal_1280x720.png` | 2026-08-05 19:25:13 | 1280×720 | 产品传送页 CTA 的无备战 Start Run |
| `Saved\P4xScreenshots\P4x.0.r2_ProductStart_NoEquipment_1280x720.png` | 2026-08-05 19:25:29 | 1280×720 | 无装备场景 |
| `Saved\P4xScreenshots\P4x.0.r2_ProductStart_MissingPreparation_1280x720.png` | 2026-08-05 19:25:46 | 1280×720 | 缺失旧备战场景 |
| `Saved\P4xScreenshots\P4x.0.r2_ProductStart_StaleLegacy_1280x720.png` | 2026-08-05 19:26:02 | 1280×720 | 失效旧空间/Hotbar 引用场景 |

## 5. 最终构建、回归与日志审计

| 验证 | 结果 | 当前日志/产物 |
|---|---:|---|
| `demo_mapEditor Win64 Development` | PASS | 最终源码构建成功 |
| `demo_map Win64 Development` | PASS | `Binaries\Win64\demo_map.exe`；UBT `Result: Succeeded` |
| Code B 自动化 | 58/58 | `Saved\Logs\P4x.r2.Automation.CodeB.current.log` |
| ProfileSettlement | 19/19 | `Saved\Logs\P4x.r2.Automation.ProfileSettlement.current.log` |
| ProfileSession（含 Subsystem） | 33/33 | `Saved\Logs\P4x.r2.Automation.ProfileSession.current.log` |
| ProfileNormalStartup | 17/17 | `Saved\Logs\P4x.r2.Automation.ProfileNormalStartup.current.log` |
| ProfilePreparationFlow | 17/17 | `Saved\Logs\P4x.r2.Automation.ProfilePreparationFlow.current.log` |
| P4x 真实输入 | PASS / PASS | `Saved\Logs\P4x.r2.RealInput.1280x720.current.log`；`...1920x1080.current.log` |
| 四个产品 Start Run 场景 | 4/4 PASS，均含第二次 Start Run | `Saved\Logs\P4x.r2.ProductStart.*.current.log` |
| 默认地图无 Code B Host | PASS | `Saved\Logs\P4x.r2.DefaultMapNoCodeBHost.current.log` |

逐份最终日志检索 `Fatal error`、crash、`Ensure condition failed`、`Assertion failed`、`LogAutomationController: Error`、测试失败和 `P4X_* FAIL` 均为 0。启动过程中的可选 profiler DLL 与未安装平台 SDK 提示不属于项目错误，未影响 Win64 构建或 Automation 队列。

## 6. 起止基线与源码来源审计

活动工程与 `C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1` 的 `Source\demo_map` 逐文件 SHA-256 比对共 32 个差异：14 个 Code B 新增文件，18 个已知 Code A/P4x 文件；无来源不明差异。

P4.0.r1 已登记并仍保留的 4 个 Code A 文件是 `demo_mapItemPresentation.cpp`、`demo_mapProfileSessionTests.cpp`、`demo_mapProfileSettlementTests.cpp`、`demo_mapProfileSettlementTransaction.cpp`，用途为真实 DisplayName、死亡结算 SpatialRing 清理及对应回归。

r0 继承的去备战必要 Code A 路径为 `demo_mapV3ProgressionManager.*`、`demo_mapProfilePreparationFlow.cpp`、`demo_mapProfileSessionSubsystem.*`、`demo_mapProfileBeginRunTransaction.cpp` 与 `demo_mapProfileRepository.cpp`：它们将正式产品 Start Run 与旧 PreparationLayout 解耦，但不让 Code B 接管正式权威。

r2 的精确增量是：`demo_mapSectNavigationWidget.*` 暴露传送页 CTA 的自动化委托；`demo_mapV3ProgressionManager.*` 补齐四种隔离产品入口、返回再启和当前截图；`demo_mapProfileSessionCoordinator.cpp` 允许已支持的 ActivationFailure 正确走回滚；四个 Profile 测试文件修正空间戒指槽位契约并扩大对白名单测试源和 CTA 的静态审核。它们均不修改战斗、地图、奖励价值、Loot 或 SaveGame schema。

| 文件 | 基线 SHA-256 | 当前 SHA-256 |
|---|---|---|
| `CodeB/demo_mapCodeBInventory.cpp` | `<missing>` | `28C7E6EF39168BE24166E3E1508EE587DFF4EB82DD8313E613872D774C2CAE56` |
| `CodeB/demo_mapCodeBInventory.h` | `<missing>` | `D74A58814D0E8864630785C6508072FDDA1AC7BE660291F12E2687EB1AA97077` |
| `CodeB/demo_mapCodeBInventoryTests.cpp` | `<missing>` | `2E00998FB069FF737C9CA7C0756D15CBE5D40704DD77C7D98F19C521711EB6EA` |
| `CodeB/demo_mapCodeBP2.cpp` | `<missing>` | `6D5450F3FFBFFD30C14EFE0AE5E0FEE91348E7BE2BC7468DBE5600739BD639BF` |
| `CodeB/demo_mapCodeBP2.h` | `<missing>` | `58571D028DB4872FF95BB1075B3D324D61D66D9258F6FBB303D30B5BBD20077D` |
| `CodeB/demo_mapCodeBP2Tests.cpp` | `<missing>` | `D1BA1C300633461C6734841640B2AC62767A9FE48FA982C84088FADC39ACB972` |
| `CodeB/demo_mapCodeBP3.cpp` | `<missing>` | `C0AF17D270474D06776A05A262FA4FEAC0C228E2A01D8B0A0D726BC26C206D77` |
| `CodeB/demo_mapCodeBP3.h` | `<missing>` | `C45A3724DA57B997DEF0E49EBA6284A632A3E13A6520E4745C82555C4BABCF94` |
| `CodeB/demo_mapCodeBP3Tests.cpp` | `<missing>` | `3C1677702B68FE232A68CAD032271A139842732022E0F6E359FDB01C4DEEEF59` |
| `CodeB/demo_mapCodeBP3UI.cpp` | `<missing>` | `8B13C3A1D534D88C9DBD29C27369F5FAFE352A9B28A1BF8CA85982A8976A00D6` |
| `CodeB/demo_mapCodeBP3UI.h` | `<missing>` | `FC358BA83F911D292DDCE7A73FB5DF7B80EFABB61B465AB9E1414D049538AE4C` |
| `CodeB/demo_mapCodeBP4.cpp` | `<missing>` | `F88C461E502F45D35A2A5361DA1724F88EA40950BBB32CA03542EEADAB3927F9` |
| `CodeB/demo_mapCodeBP4.h` | `<missing>` | `31EE6F375430DB5FF9DC88DE8E60842508FB3940403CFDF0DC95BEFB25BF1E98` |
| `CodeB/demo_mapCodeBP4Tests.cpp` | `<missing>` | `F410F0925A1E5626F91EC3D3FB8C04D5FEEB95A2C0E99177939266EB3D260DEC` |
| `demo_mapFullSystemLoopTests.cpp` | `1771900532A116F161990DE733DB3CAD5ACC7369C5DDAAB3709EE9A8F71BFC80` | `0ABBA59002481D4FC18BD1E2A74A8F1B3DA9BC2FC47F93C367897101DBC828C4` |
| `demo_mapItemPresentation.cpp` | `627460A395F51A620EC2F2F6CE2CC620B55C7388D15B8126723AB37F4CBE4D80` | `EB44EFA8E07A50A3A24E0F3EF6CBC7C4D9CA5AD33B5DE84B95EC5EFFA0E96E0A` |
| `demo_mapProfileBeginRunTransaction.cpp` | `A4F9A91C0D1519903F008DE60D31701C46EAE2B4AEE5281914DCFC275C3976FF` | `A9F679CE015BD4778FD55259D856A8C2EB43EAB6FE8444E8F26B54F7F8AF9D93` |
| `demo_mapProfileNormalStartupTests.cpp` | `587A3C1B4753A21BE7CDE7B8563A4D975BF41D67E43C6B09216838BC1833825B` | `516DF5EF988D2BC461A4B5245C7D34ADB53E504499449C68D948FFA7A3AFC446` |
| `demo_mapProfilePreparationFlow.cpp` | `CD338E52A19208D9B5F8ABE53BBB997FE1FC4E3B99497609178A3E01E9B192CF` | `51C810ED81A457B3DF0D17DA2286ECAA77F36A5322D3298CFEDF62D3DC52DECB` |
| `demo_mapProfilePreparationFlowTests.cpp` | `5BC00BDAAB54BB31D0CA0A5D10B76D90E367671B6E3A3D2035E3F8CB53320ABC` | `3E42B100B8CB11601353E83185A2FFED818B38778FF05F57AE9AEF46E8CCAFE8` |
| `demo_mapProfileRepository.cpp` | `5E60F39063ACEEF91D45B18E946D7F5060F487A70B4C635BDF021D154B92E12B` | `D8A226A0AD53BFF4798D1ABB9C374E752C0B1CBB468293A8BE81A26B36836FED` |
| `demo_mapProfileSessionCoordinator.cpp` | `CC63D9E78A202CE3C6097DC6C2420595F0E11FFA0EAA86E5ACEB2B95FD60A596` | `F00960A33C95526FD4C7A2016839385A56B3C014008EC19B3335C8E47018CC78` |
| `demo_mapProfileSessionSubsystem.cpp` | `F61C9440F5AAFB4B1176DFD9341983726AEED56F1AC732A1239C270FF49C0BF9` | `BB1E3E8D3B470E41DE3560DF3A74DBF41982A1263CECC3E2DE049B4FFF5F15E3` |
| `demo_mapProfileSessionSubsystem.h` | `9DD5944B53F72860AA4B496F31C7D777B5B4D4C847CA970F167E9A5ACC05416C` | `DB488BD5491188A3B090C6C897D3BE03C42F18C5E2F85D4A6316CFDFC2AD84FA` |
| `demo_mapProfileSessionSubsystemTests.cpp` | `B62D885D430B4AAC8D861AAF36D8296F9A17788A6F381E2BABB7AB7EAA4A0189` | `854D46648ACD2960383CE51CDF93570BF06216BD16016926C4FAD360DCBAF647` |
| `demo_mapProfileSessionTests.cpp` | `34FD1E1606D4B9DE1316D1791195077A06D112EBD3C8B453D1C4EEBE14CFC213` | `18737452E0C4F2F815E07664838BF095A6BF025244D45D99A8F9990FF36AD107` |
| `demo_mapProfileSettlementTests.cpp` | `8F41EA2E890AB97ED1F35C31CCCE5A7AE5150F33298E64B78E3AB7250F64E787` | `4997A53D3206BD1F4F7611E2FECBB2796442D56970668D9F3FDB2F2F6BEF04F3` |
| `demo_mapProfileSettlementTransaction.cpp` | `22C179703C1D326AD867A830B55235E28DFFC5FA5D79C3C4D3C3AB2DCA5FD448` | `855D65CBAE08049E635AD9DA8501218C92203EA851588859AAF2D8A6484226DF` |
| `demo_mapSectNavigationWidget.cpp` | `48C37A994CDF694642B7547857DABA079144E3C326F6134F391E8ABFE2CC670C` | `47255C58BFB79C6B80A9E6E5DFB65EA5B5F9B9AAF7731A21B819FF77685953B9` |
| `demo_mapSectNavigationWidget.h` | `723FFCA7E4A7A5A5F8DAE756FEA8195B51026658CAA47DE4CCAEA21BC5AF8445` | `3BC375E5A42C228F30D47313E07857EAE031A05972DBBF841A163EBA7B1DA309` |
| `demo_mapV3ProgressionManager.cpp` | `B3BA8FB5750B9AF2C67A3E02DF4EB8219D1C35A13726BC295DE5968210773E7D` | `FB6D870208927DD9009D4CB860A3AF2B4919ABC6C9F3A94552F8AF3DCFC7C877` |
| `demo_mapV3ProgressionManager.h` | `C9425A481D25F09F9DAC7DAE6640DFFEE7EA7650B5405E4C8C87A51EDE29A745` | `EF7B11B24B0A3AE4AD1A84231226290D7427E0A6839B6AC853FF3340D88C0BCE` |

## 7. 交接

项目资料已更新为 r2 当前状态。下一步只能由策划部另行下发；不得自动开始 P5 或其他 Prompt。本次只交付本同名 Report 的真实文件附件。
