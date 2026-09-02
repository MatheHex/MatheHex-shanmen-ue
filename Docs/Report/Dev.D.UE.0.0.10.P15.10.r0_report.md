# Dev.D.UE.0.0.10.P15.10.r0 Report

## 1. 结论

P15.10 完成 legacy `demo_map` 父组最后一项失败 `demo_map.V3.Lifecycle.E.AtomicInventoryRollback` 的独立根因审计与有界收敛。

该失败不是正式库存事务缺陷，而是 0.0.9B 基线 fixture 把当前定义驱动的固定六格库存写死为 12 格。正式 `AddDefinition` 已在任何写入前完成整批容量预检，并在超容量时返回 `InventoryFull`；现行 `demo_map.GridInventory.08.FullSixRejectsAtomically` 也独立覆盖相同事务边界。

最终结果：

```text
AtomicInventoryRollback baseline: 0 Success / 1 Fail
demo_map.V3.Lifecycle after:       15 Success / 0 Fail
legacy demo_map after:             1330 Success / 0 Fail
Shanmen.0_0_10 full:               712 Success / 0 Fail
```

本阶段 production C++ 零修改；只修正 legacy test contract，并补齐该测试文件的精确改动—回归映射。

## 2. 根因

旧 fixture 的第一步是：

```text
AddDefinition(TrainingBlade, 12) -> expected Success
```

但库存权威从同一历史基线起就使用：

```text
BaseInventoryCapacityWithoutBackpack = 6
```

`TrainingBlade` 的 `MaxStackSize` 为 1，因此 12 件请求需要 12 个单元格。正式 `AddDefinition` 先计算整批可用容量；12 大于 6 时正确返回 `InventoryFull`，且不产生任何实例。

旧测试随后继续执行：空库存添加一件 `TrainingVest` 会合法成功，因此原来的 `fill`、`rejected`、`unchanged` 三个断言全部失败。失败链由 fixture 的错误前置条件完整解释，不需要放宽正式容量或事务规则。

## 3. 实现

`AtomicInventoryRollback` 现在：

- 从 `Fdemo_mapItemAuthority::GetInventoryCapacity()` 读取当前定义驱动容量；
- 先验证容量大于 0；
- 使用非堆叠 `TrainingBlade` 精确填满全部单元格；
- 验证占用格数等于容量；
- 尝试再添加一件 `TrainingVest`，要求返回 `InventoryFull`；
- 验证库存 GUID 顺序、实例数量与 Authority Revision 均保持不变。

没有改动 `Fdemo_mapItemAuthority`、`AddDefinition`、Definition catalog、容量公式或运行时库存写路径。

## 4. 回归覆盖门禁

新增精确映射：

```text
Source/demo_map/demo_mapRunLifecycleTests.cpp
  -> demo_map.V3.Lifecycle
```

流程自检同时新增一正一反：完整 Lifecycle 组可以覆盖该文件；无关 `Shanmen.0_0_10` 全量证据不能替代。最终结果：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 268/268
JSON_PARSE: PASS Rules=159
GIT_DIFF_CHECK: PASS
```

流程证据 SHA-256：

- gate：`221B503D237007157A059A8F24A5A6C63D3CDE8EAA835CCD9303A2F78CA9A18C`；
- self-test：`B56BBC8808B3CF4CB93F6EE72BC1A062060F697CCF435FB2D013B03420E5454E`；
- JSON parse：`3ABE40598FB966648462AF31327C8449C10EAAEAF8B3E172E478359B2B01D8F2`；
- diff check：`6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`。

## 5. Automation 证据

| Evidence | Success | Fail | Total | SHA-256 |
|---|---:|---:|---:|---|
| Atomic rollback baseline | 0 | 1 | 1 | `FD3B955EEC4868CA64455BBC012E898ADD45D0E90CB0C4A02042C03B9B50D96F` |
| `demo_map.V3.Lifecycle` after | 15 | 0 | 15 | `F2C28972C2B863F3A366B28F46AED669389DD611F1443299E3D86315B590FEC0` |
| legacy `demo_map` after | 1330 | 0 | 1330 | `65709CDA2F10ABD53699CACB894337E41ABF0A1B40963BA1DDD32DB322A9FC77` |
| `Shanmen.0_0_10` full | 712 | 0 | 712 | `8D12D5D094B70D9D9B7CC090F7B59C169634317E9FAE20478BDCC7E87FCEED69` |

四份 Automation 日志均有原生成功退出；三份成功日志均有 queue-empty terminal evidence。Fatal error、Unhandled Exception 与 Ensure condition failed 均为 0。

0.0.10 全量首末 Success 时间为 `01:04:17.248 -> 01:39:58.951 UTC`，约 `35m41.70s`，随后出现 `Automation Test Queue Empty 712 tests performed`。

## 6. 构建证据

本轮继续使用 `-NoUBA -MaxParallelActions=1`，控制当前主机的高提交内存压力。

| Target | Result | Actions / UBT total time | Status | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 4 / 20.52s | 0 | `1BE88923D31E0617E9CA83FA6D67FAF3DB7747FC73332616C65FB48FA56446E9` |
| Game final | Succeeded | 3 / 27.59s | 0 | `73E8EACB267F1ED2F933239BB4A4F978EAF43CD64130D5549553FBB1CE99743D` |
| Editor final | Succeeded, up to date | 0 / 1.09s | 0 | `0D458C2791288C9295C5656D1EA9EA0F729F57F0BA634799248211E013926C95` |

最终产物：

- `demo_map.exe`：355,744,768 bytes，SHA-256 `D5F6DB782A6B33651E6C7B7CC4BDD428915BEAA5DED1702D1FFD41C658C903EE`；
- `UnrealEditor-demo_map.dll`：14,330,880 bytes，SHA-256 `02CC69008CFEF2CC9CD3EF23ABFC27C767D55514F967929CD57ECBCE78A1FDEB`。

## 7. 修改范围

- `demo_mapRunLifecycleTests.cpp`：移除固定 12 格前提，按 Definition-backed capacity 构造原子拒绝场景；
- `ShanmenRegressionMap.json`：新增 legacy Run Lifecycle exact mapping；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 exact mapping 正反自检；
- Report/Log；
- production C++ 零修改，raw logs 仅本地保存；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 8. P/F 边界

本 Report 仅包含 P 阶段 legacy test contract 修复、回归门禁、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 9. 后续

legacy `demo_map` 父组现无剩余失败。后续开发不再为 P15 legacy 清理循环制造新修复轮；回到 0.0.10 产品路线时，继续由改动文件映射选择必跑测试，并保留 `Shanmen.0_0_10` 全量作为阶段闭合证据。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-10-atomic-inventory-fixture-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-10-atomic-inventory-fixture-regression/Docs/Report/Dev.D.UE.0.0.10.P15.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-10-atomic-inventory-fixture-regression/Docs/Log/Dev.D.UE.0.0.10.P15.10.r0_log.md>
