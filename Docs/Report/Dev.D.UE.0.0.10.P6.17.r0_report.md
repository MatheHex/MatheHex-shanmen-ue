# Dev.D.UE.0.0.10.P6.17.r0 Report

## 1. 结论

P6.17 已完成 Orbit threat 的逐物品单调样本水位线与有界重放保留，结论为 **PASS**。

Run authority 现在只保留每件受控物品最新已完成 sample 的重放回执；更旧 sample 失败关闭，精确最新 sample 保持幂等。空 sample 同样推进水位线，因此可以明确终止上一批非空 presence，而不会发明伤害、控制、持续时间或采样频率。

## 2. 功能性

- 以 `SourceItemInstanceId` 为键保存最新 sample checkpoint；不同飞剑的水位线彼此独立；
- sample identity 由 Run、activation、source entity、source item、detector identity/kind 与 hit ordinal 确定性派生；
- 新 ordinal 被接受后替换同一 item 的旧 checkpoint，并删除旧 sample 的 retained intent receipts；
- 精确重放最新非空 sample 返回 `AlreadyConsumed`，精确重放最新空 sample 返回 `NoOp`；
- 小于最新 ordinal 的 sample 返回 `SampleExpired`；
- 相同 ordinal 但 sample identity 或 intent payload 不一致时返回 `SampleConflict`；
- 空 sample 也建立 checkpoint、推进 sample revision 并过期旧非空 sample；
- 最后一个 terminal item 退出 Run Host 时，既有生命周期继续整体清空 authority 与所有 checkpoint。

## 3. 修订与保留语义

本阶段区分两条单调计数：

- `AuthorityRevision`：生命周期内已接受 intent 的累计数量。旧接口 `NumConsumedIntents()` 继续返回这个累计值，避免有界裁剪改变既有审计语义；
- `SampleCheckpointRevision`：每接受一个新的 sample checkpoint 增加一次，空 sample 也增加，精确重放或拒绝不增加。

`NumRetainedIntents()` 只报告当前仍可精确重放的回执数量；旧 sample 被替换后，该值下降或归零，但累计 intent revision 不回退。`NumTrackedSamples()` 与 `GetLatestSampleOrdinal()` 提供逐 item 水位线观察面。

## 4. 原子性与失败关闭

消费仍在 authority 副本上执行。实现先构造并校验 incoming checkpoint，再裁剪候选副本中的旧 item receipts、安装新 checkpoint、验证候选完整性，最后整体提交。

Authority 自校验现在同时保证：

- checkpoint 的确定性 sample ID 可重算；
- checkpoint 内 intent ID 唯一；
- 每个 retained receipt 恰好归属于一个 checkpoint；
- item、activation、detector 与 ordinal 在 checkpoint/receipt 间一致；
- retained receipt revision 唯一且不超过累计 authority revision；
- sample revision 不回退，且不小于当前 tracked item 数。

任何不一致只返回拒绝结果，不修改 live authority。

## 5. 测试覆盖

Runtime authority 测试覆盖：首次消费、精确重放、同 ordinal 不同 payload 冲突、后续 sample 替换、旧 sample 过期、空 sample 推进、空 sample 精确重放、空 sample 过期上一非空 sample、Run/source fence 与零 gameplay effect。

产品测试扩展 `OrbitThreatRouting` 并新增 `PerItemThreatSampleWatermarks`，覆盖：

- 非空 sample 后的 cumulative/retained/checkpoint revision；
- 更新空 sample 后 retained receipts 归零而累计 intent 数不回退；
- 被替换 sample 无法从 Run Host 复活；
- 两件物品各自持有水位线，一件推进不影响另一件精确重放；
- stale replay 不修改任一水位线；
- Run teardown 清空所有 checkpoint。

最终无头自动化：

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 8 | 0 | 0 | `06EF0158B41D5D62F0856C86960D8FE3BC1BBAFAF0013B93CE259B9B17628E22` |
| `Shanmen.0_0_10.Product.ControlledWeapon` | 31 | 0 | 0 | `0B6F2FAC7D564AF88EC822DD642C9D8F32A43A1BBC1E1F45B8AD6E59465DC37C` |
| `Shanmen.0_0_10` | 164 | 0 | 0 | `EFE47EE3D8BAB7401353F71A3C2896079365CCB52EBAC5DCC0F611A3F62E8442` |

每份日志均只有一个实际 RunTests 命令、一个 queue-empty 终止、Fail `0`、Fatal/unhandled/assert/ensure `0`、原生退出码 `0`。

## 6. 改动—回归门禁

CombatRuntime 与 RunHost 两类改动路径共同要求 11 个测试组；完整 0.0.10 日志覆盖全部必跑组：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=11 Logs=3
SELF_TEST: PASS 14/14
```

## 7. 构建与静态检查

- `git diff --check`：native exit `0`；
- 首次 Editor 构建：源码错误 `C2065`，getter 被误置于单条 receipt 类型，原生退出码 `1`；修正类型归属后不再复现；
- 修正后的受影响 Editor rebuild：`32/32` actions，`Result: Succeeded`，native exit `0`，`88.50s`；
- 增加最终冲突测试后的 Editor build：`4/4` actions，`Result: Succeeded`，native exit `0`，`9.53s`；
- 受影响 Game rebuild：`29/29` actions，`Result: Succeeded`，native exit `0`，`93.72s`；
- 最终测试增量后的 Game build：`3/3` actions，`Result: Succeeded`，native exit `0`，`10.79s`；
- 最终 Runtime Editor DLL UTC：`2026-08-29T05:35:47Z`；Game executable UTC：`2026-08-29T05:37:23Z`；
- 新增生产代码扫描 `Tick(`、`DeltaSeconds`、`SetTimer`、`ApplyDamage`、impact/vitality commit、spawn、Actor/World、RNG、Cooldown 与 Duration：命中 `0`。

首次失败是已修复的源码归属错误，不是环境、内存或外层超时；最终两个目标均以原生退出码 `0` 收口。

## 8. 修改范围与兼容性

- `Source/ShanmenCombatRuntime/Public/ShanmenControlledWeaponThreatPresenceAuthority.h`
- `Source/ShanmenCombatRuntime/Private/ShanmenControlledWeaponThreatPresenceAuthority.cpp`
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenControlledWeaponExecutionTests.cpp`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.h`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`
- 本 Report 与同名 Development Log。

Source 新增 `535` 行、删除 `11` 行。Run Host 的累计计数返回类型由 `int32` 扩宽为 `int64`；名称与累计语义保持不变。未修改 Build.cs、GameplayTags、Profile schema、存档、物品 authority、输入或资产，也未纳入长期未跟踪历史文件。

## 9. P/F 边界

本 Report 只包含 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-17-threat-sample-watermarks/Docs/Report/Dev.D.UE.0.0.10.P6.17.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-17-threat-sample-watermarks/Docs/Log/Dev.D.UE.0.0.10.P6.17.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-17-threat-sample-watermarks>
