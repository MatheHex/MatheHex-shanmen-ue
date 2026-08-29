# Dev.D.UE.0.0.10.P6.18.r0 Report

## 1. 结论

P6.18 已完成 Orbit threat 的精确 activation 生命周期准入与逐物品原子退役，结论为 **PASS**。

P6.17 的逐物品 sample 水位线会在同一 Run 中保留已退役物品的 checkpoint；只要还有其它物品存活，旧状态既持续占用 authority，也会阻止同一物理物品以新 activation、从 ordinal `0` 重新加入。本阶段将 authority 的消费资格绑定为 `SourceItemInstanceId -> ActivationId`，并在 Host 移除 terminal item 时只退役该 item 的 registration、checkpoint 与 retained receipts，不干扰同 Run 的其它物品。

## 2. 功能性

- authority 显式登记当前活动的 item/activation 一对一映射；同一映射重复登记保持幂等；
- 同 item 不得同时登记第二个 activation，同 activation 也不得同时归属第二个 item；
- 未登记 item 的 presence 返回 `ItemNotRegistered`；登记 item 携带旧 activation 时返回 `ActivationMismatch`；
- Run Host attach 在候选副本中同时加入 Controller 与 authority registration，任一步失败都不提交 live Host；
- terminal item 移除在候选副本中先执行 exact activation retirement，再删除 Controller；
- retirement 只裁剪目标 item 的最新 checkpoint 与 retained intent receipts；其它 item 的水位线和精确 replay 保持有效；
- 同一物理 item 退役后可在同一 Run 中以新 activation 重新 attach，新 activation 的 sample ordinal `0` 可正常提交；
- 旧 activation 即使重放旧 presence 也失败关闭，不能污染新 activation 的水位线；
- 最后一个 item 退出时仍沿用既有 Host reset，整体清空 Run authority。

## 3. 修订语义与原子性

本阶段保留 P6.17 的两条修订线：

- `AuthorityRevision` 继续表示生命周期内已接受 intent 的累计数量；退役只裁剪 replay retention，不回退累计审计值；
- `SampleCheckpointRevision` 在接受新 sample 时增加；若退役时确实删除了 checkpoint，再增加一次以公开记录该状态变化；没有 checkpoint 的 registration 退役不增加。

Registration、retirement、attach 与 terminal removal 均在结构副本上修改并执行完整 `IsValid()`，只有候选状态全部满足不变量时才整体提交。失败路径不改变 live authority、Controller 集合、revision 或其它 item 的 checkpoint。

## 4. 完整性与失败关闭

Authority 自校验新增以下约束：

- 所有 registered item ID 与 activation ID 均有效；
- active activation ID 全局唯一；
- tracked checkpoint 数不得超过 registered activation 数；
- 每个 checkpoint 必须属于同 item 当前登记的 exact activation；
- 原有 sample ID、intent ID、receipt revision 与 checkpoint/receipt 双向归属约束继续成立。

Host 自校验同时要求 registration 数量与 Controller 数量一致，并逐项验证 Controller action 的 item/activation 与 authority 映射完全相同。因此不存在“Controller 已加入但 authority 未准入”或“Controller 已删除但旧 activation 仍可消费”的半提交状态。

## 5. 测试覆盖

Runtime authority 测试新增或扩展：未登记消费、exact registration 幂等、item/activation 双向唯一、错误 activation 退役拒绝、退役裁剪 checkpoint/receipts、累计 revision 不回退、replacement activation 登记、旧 activation 拒绝，以及无 checkpoint 退役不推进 sample revision。

产品测试扩展 `PerItemThreatSampleWatermarks`，覆盖：

- 两个物品同时登记并建立独立水位线；
- 低序物品完成 launch/recall 后被移除，其 registration/checkpoint 被裁剪而高序物品保持可重放；
- 同一低序物理物品以新 activation 重新 attach；
- replacement activation 的 ordinal `0` 被接受并建立新 checkpoint；
- 旧 activation 无法重放，高序物品的 exact replay 不受影响；
- replacement item 再次退役只推进一次 checkpoint revision，最后一个 item 退出后整体 reset。

最终无头自动化：

| Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 8 | 0 | 0 | `0.124s` | `B158B14179F19C8DDACD18D2E2931116073C24FA36C6843C112E0CDF1A9FD08D` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 31 | 0 | 0 | `0.545s` | `7A69D2C958D0295DA3C2AB9F5479C3E43CF32535EA8F71837AEDA6F17E00E984` |
| `Shanmen.0_0_10` | 164 | 0 | 0 | `8.213s` | `75C9CC3E361493DA50A27AC46459478180435F7DA91581726EB790CA9A215353` |

每份日志均只有一个实际 `Automation RunTests` 命令、一个 queue-empty 终止、Fail `0`、Fatal/unhandled/assert/ensure `0`，原生退出码均为 `0`。

## 6. 改动—回归门禁

CombatRuntime 与 ControlledWeaponRunHost 两类改动路径共同要求 11 个测试组，完整 0.0.10 日志覆盖全部必跑组：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=11 Logs=3
SELF_TEST: PASS 14/14
```

## 7. 构建与静态检查

- `git diff --check`：native exit `0`；
- Editor Development build：`32/32` actions，`Result: Succeeded`，native exit `0`，`95.88s`；
- Game Development build：`29/29` actions，`Result: Succeeded`，native exit `0`，`95.64s`；
- 最终 Runtime Editor DLL UTC：`2026-08-29T05:48:12.7315351Z`；
- 最终 demo_map Editor DLL UTC：`2026-08-29T05:49:38.9721216Z`；
- 最终 Game executable UTC：`2026-08-29T05:53:11.6766496Z`；
- 新增生产可执行代码扫描 `Tick(`、`DeltaSeconds`、`SetTimer`、`ApplyDamage`、impact/vitality commit、spawn、Actor/World、RNG、Cooldown、Duration 与 inventory transaction：命中 `0`。

两个构建均为首次执行成功，没有源码、环境、内存、SDK、外层超时或链接失败。Game executable 仅构建，未启动。

## 8. 修改范围与兼容性

- `Source/ShanmenCombatRuntime/Public/ShanmenControlledWeaponThreatPresenceAuthority.h`
- `Source/ShanmenCombatRuntime/Private/ShanmenControlledWeaponThreatPresenceAuthority.cpp`
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenControlledWeaponExecutionTests.cpp`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.cpp`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`
- 本 Report 与同名 Development Log。

Source 新增 `310` 行、删除 `7` 行。未修改 cadence、威慑效果、damage/control、Actor/World 状态、inventory authority、Profile schema、存档、GameplayTags、Build.cs、输入或资产；未纳入长期未跟踪历史文件。

新增拒绝枚举只附加在既有枚举尾部的 sample 错误之前；所有现有调用均通过全量 0.0.10 回归。P6.17 的累计 intent 审计、逐 item sample 水位线与 exact replay 语义保持不变。

## 9. P/F 边界

本 Report 只包含 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-18-threat-activation-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P6.18.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-18-threat-activation-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P6.18.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-18-threat-activation-lifecycle>
