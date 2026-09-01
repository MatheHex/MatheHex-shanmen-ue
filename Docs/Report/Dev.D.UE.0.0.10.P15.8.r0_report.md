# Dev.D.UE.0.0.10.P15.8.r0 Report

## 1. 结论

P15.8 完成三项同源 legacy fixture 的有界收敛：P5 Runtime Interface 与 P6 Dual Loot 不再假设旧的四装备角色布局，而是跟随当前五角色权威 Weapon、Armor、Accessory、SpatialRing、Backpack；尸体空间物品 fixture 也不再把 Backpack 写入已属于 SpatialRing 的固定 slot 3。

定向结果：

```text
demo_map.P5RuntimeInterface: 8 Success / 1 Fail -> 9 Success / 0 Fail
demo_map.P6.DualLoot:        6 Success / 2 Fail -> 8 Success / 0 Fail
```

legacy `demo_map` 父组从 P15.7 的 `1323 Success / 7 Fail / 1330 Total` 收敛为 `1326 Success / 4 Fail / 1330 Total`，精确消除本阶段三项失败，没有隐藏或重分类剩余失败。`Shanmen.0_0_10` 全量仍为 `712 Success / 0 Fail / 712 Total`。

本阶段 production C++ 零修改；没有删除 SpatialRing 或 Backpack，也没有把产品装备容量降回 4。

## 2. 根因

当前产品权威 `Fdemo_mapItemDefinitions::GetEquipmentSlotIds()` 明确返回五个 canonical role：

```text
0 Weapon
1 Armor
2 Accessory
3 SpatialRing
4 Backpack
```

三项失败均来自旧 fixture 未随该模型迁移：

1. `P5RuntimeInterface.01` 仍断言 Runtime Equipment cell 数为固定 `4`，实际为 `5`；
2. `P6.DualLoot.03` 把 `BackpackLevel1` 写入固定 slot `3`，该位置现属于 SpatialRing，导致 Backpack 无法被识别为提供动态存储的空间物品；
3. `P6.DualLoot.04` 只覆盖 slot `0..3`，并把名称历史遗留的 `AccessoryLevel1` 当成普通 Accessory，但其真实 Definition 是 SpatialRing；因此真实 Accessory role 与 Backpack slot 4 都未被正确覆盖。

这不是产品回归，而是测试数据仍表达旧四角色布局。

## 3. 实现

### P5 Runtime Interface

Runtime Equipment cell 数改为与 `GetEquipmentSlotIds().Num()` 比较。该测试仍在验证 Runtime view 与产品 catalog 的接口一致性，不再复制容易漂移的常量。

### P6 Corpse Spatial Item

- 通过 canonical slot 列表查找 `BackpackSlot` 的真实索引；
- 先断言 Backpack role 存在，再按该索引构造尸体 Equipment seed；
- 通过 `ResolveSpatialStorageCapacity(BackpackLevel1)` 取得 Definition-backed 容量；
- 同时验证空间物品 cell 唯一且已识别、动态容量正确、空间存储起点等于尸体基础快捷栏容量。

### P6 Equipment Slot Canonicalization

- 用 `EvasionCharm` 覆盖真实 Accessory role；
- 保留 `AccessoryLevel1` 覆盖其真实 SpatialRing role；
- 用 `BackpackLevel1` 覆盖 Backpack role；
- 逐项验证 `[0, canonical role count)` 每个 slot 都恰有覆盖，并继续验证重复 Weapon 降级为普通携带物。

## 4. 回归覆盖门禁

此前两个 fixture 被宽泛规则吸收，却没有要求自己的 legacy suite。本轮拆出两个 exact rule：

```text
Source/demo_map/demo_mapRuntimeInterfaceSliceTests.cpp
  -> demo_map.P5RuntimeInterface

Source/demo_map/demo_mapDualLootSliceTests.cpp
  -> demo_map.P6.DualLoot
```

流程自检为每条规则各增加一正一反：exact suite 可以覆盖对应 fixture；无关的 full/item 日志不能替代。最终证据：

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=2 Logs=2
SELF_TEST: PASS 262/262
JSON_PARSE: PASS Rules=156
git diff --check: PASS
```

流程日志 SHA：gate `EBFA2F36260B5A0FD6020C6F4296D651274C5348092614562194CFA284152085`；self-test `D840E610D11FDC94A58F03CEE9BC5FD3DC05C9A895B6ABD2DE0058C062035ECD`；JSON parse `C0E104D261CD67449699034727FF72657D928A0950E8FF461A2634F6A3C7C3F9`。

## 5. Automation 证据

| Evidence | Success | Fail | Total | SHA-256 |
|---|---:|---:|---:|---|
| P5 Runtime Interface baseline | 8 | 1 | 9 | `FE06BC565899DEBA8593FFD7BA3FAD2CA28D9E22386EEDF24474918565B9765A` |
| P5 Runtime Interface after | 9 | 0 | 9 | `88D329C5FBA0DDCC9505CF618111174D3704F2D6F32861979FE27FAE507F4481` |
| P6 Dual Loot baseline | 6 | 2 | 8 | `0E3E2F809D8C787D836FB2A360413D305739C6CD6657A9333685AA6F775B58FA` |
| P6 Dual Loot after | 8 | 0 | 8 | `B102DD89922FB601FF1CB0C6D615047A882026EC2EF89E10DB156BF0C98866AF` |
| legacy `demo_map` after | 1326 | 4 | 1330 | `0A44835AE3380FA7918E49EA8FD0E593F3DBC26B55FAFAC64639817409A3B7D4` |
| `Shanmen.0_0_10` full | 712 | 0 | 712 | `9D745573A66B766C0AA242C937D1461DCF5DE99AC49C09372A2008DA355DB8E4` |

六份 Automation 日志均有 queue-empty terminal evidence；Fatal error、Unhandled Exception 与 Ensure condition failed 均为 0。legacy 父组 status `0` 不被当作全绿，仍按逐项结果记录剩余 4 项失败。

0.0.10 全量首末 Success 时间为 `21:54:06.311 -> 22:26:56.930 UTC`，约 `32m50.62s`，随后出现 `712 tests performed` queue-empty terminal evidence。

## 6. 构建证据

初始 Editor 构建使用单进程非 UBA 模式，5 actions，成功。最终 Game 的第一次普通并行尝试遇到主机提交内存过高：UBA 连续 286 次因 low-memory threshold 杀死两个 compile action，没有产生源码诊断或原生终止结果。该无进展实例被有界中止，原始日志完整保留；随后以 `-NoUBA -MaxParallelActions=1` 重跑并成功。

| Target | Result | Actions / UBT total time | Status | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 5 / 26.83s | 0 | `ECF811E45489ADAC88F70E3DB193A85D1A44718B93635A88E15B70373B036C48` |
| Game first attempt | Bounded abort: 286 UBA low-memory kills | no completed action / no native result | n/a | `973CB620A60F35C7F0E36C4497D66C303B71E52B8EAFF79ADBFEFD5404946273` |
| Game recovery | Succeeded, `-NoUBA -MaxParallelActions=1` | 4 / 32.03s | 0 | `41DD989A9BCB78457395E23DF6B1FC60A1394FCEA48F87E0DD35B0B5A9D88402` |
| Editor final | Succeeded, up to date | 0 / 1.13s | 0 | `C82A66F8CACBC28A2348CC3A6A35A9FF2C5F4946B605F3993CC914790A9E192F` |

最终产物：

- `demo_map.exe`：355,743,744 bytes，SHA-256 `0FAAFC6469A7329234B38ABB73E4D0FA44309E8E5B90C804C314A43DE8147281`；
- `UnrealEditor-demo_map.dll`：14,330,368 bytes，SHA-256 `2BFF817E3C952F650D206D325031D1DC29179C03A057F91740AA6C842555D2D8`。

## 7. 修改范围

- `demo_mapRuntimeInterfaceSliceTests.cpp`：去除旧四角色固定断言；
- `demo_mapDualLootSliceTests.cpp`：按 Definition 与 canonical role 构造 P6 fixtures；
- `ShanmenRegressionMap.json`：新增 P5/P6 exact mappings，并从宽泛规则移除这两个 fixture；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 exact mapping 正反自检；
- Report/Log；
- production C++ 零修改，raw logs 仅本地保存；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 8. P/F 边界

本 Report 仅包含 P 阶段 legacy test contract 修复、回归门禁、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 9. 剩余失败与下一步

legacy 父组剩余 4 项，均原样保留：

- `demo_map.P1.SectNavigation.WidgetSmoke`；
- `demo_map.P6.Integration.ProductLoopPersistence`；
- `demo_map.P6.Integration.SectRouteClosure`；
- `demo_map.V3.Lifecycle.E.AtomicInventoryRollback`。

下一阶段可优先合并调查两项 P6 Integration 失败，确认它们是否共享存档/路线闭环契约；不会在未复现根因前改动 production 逻辑。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-8-equipment-role-contract-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-8-equipment-role-contract-regression/Docs/Report/Dev.D.UE.0.0.10.P15.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-8-equipment-role-contract-regression/Docs/Log/Dev.D.UE.0.0.10.P15.8.r0_log.md>
