# Dev.D.UE.0.0.10.P6.16.r0 Report

## 1. 结论

P6.16 已完成 Orbit threat 的显式样本原子收尾，结论为 **PASS**。

产品层现在可以把一个由外部调用方明确结束的 threat sample，以单次事务完成：

`World evidence -> target policy -> presence intents -> Run authority consume`

本阶段没有引入 Tick、自动采样频率、冷却、持续时间、伤害、控制或其它 gameplay effect；采样节奏仍由更上层产品策略拥有。

## 2. 功能性

- 新增 `TryFinalizeOrbitThreatSample`，只接收 exact item、ready Coordinator、已结束 emission 和同一批次 Actor 证据；
- 一次调用返回完整 `ThreatFinalizationResult`，聚合 transient value evidence、policy/presence 与 authority consumption；
- 首次合法样本得到 `Consumed`，同一 receipt 的精确重放得到 `AlreadyConsumed`；
- 合法空样本得到 `NoOp`，不增加 authority revision；
- 未知 item、缺失 Actor 证据、身份不匹配或任一中间结果无效时失败关闭；
- 收尾后 item 仍可进入 Launch，最后一个 terminal item 移除时既有 Run 生命周期继续回收 authority。

## 3. 原子性与自校验

收尾不直接修改 live ledger。实现先复制 `ThreatPresenceAuthority`，在候选 authority 上消费，再构造并验证完整结果；只有候选 authority 和结果均有效时才整体提交。

`ThreatFinalizationResult::IsFinalized()` 核对：

- item、Run 与 action source item 的一致性；
- captured evidence 数量与 policy target 数量；
- 每个 evidence target identity/tags 与 policy target 的一一对应；
- presence intent 与 consume receipt 的 intent identity；
- receipt source entity 与 action source entity；
- `NoOp` 必须同时没有 intent 和 consume receipt。

失败路径会清空输出，且不会留下部分 revision 或部分 intent。

## 4. 生命周期与所有权

- 调用方继续拥有“何时开始/结束一个 sample”的决定权；
- Run Host 只负责 exact-item 路由和显式完成样本的原子收尾；
- Controller/Session/Execution 仍分别负责 World evidence、policy 和 presence 的既有单向链路；
- Run-scoped authority 仍是唯一消费 ledger；
- 最终结果只保留值类型证据，不保留 `AActor*`；
- 既有 `TryEvaluateOrbitThreatActors`、`TryBuildOrbitThreatPresenceIntents` 和 `TryConsumeOrbitThreatPresence` API 保持兼容。

## 5. 测试覆盖

扩展 `Shanmen.0_0_10.Product.ControlledWeaponRunHost.OrbitThreatRouting`，覆盖：

- 首次 exact-item 原子收尾；
- exact replay 幂等与 revision 稳定；
- unknown-item 拒绝；
- 缺失 World Actor 证据拒绝且 ledger 不变；
- 显式空 sample 的零效果 `NoOp`；
- vitality 与 impact ledger 不变；
- 收尾后 Launch、后续 detector ordinal 与 terminal authority 回收。

最终自动化：

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeapon` | 30 | 0 | 0 | `0FF67AD877E7CDBBDADEABECACB560A21141D851AF5427F777DA3F0FB62F6092` |
| `Shanmen.0_0_10` | 163 | 0 | 0 | `2CA5399C4956EA8C17AAFDBDD53F88131352C1F1FEC90EB1DD3145FDC74CDC90` |

## 6. 改动—回归门禁

改动路径映射到 `ControlledWeaponRunHost` 规则，要求 11 个产品/运行时组。完整 0.0.10 日志覆盖全部必跑组：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=11 Logs=2
SELF_TEST: PASS 14/14
```

## 7. 构建与静态检查

- `git diff --check`：native exit `0`；
- Editor Development：`28/28` actions，`Result: Succeeded`，native exit `0`；
- Game Development：`27/27` actions，`Result: Succeeded`，native exit `0`；
- 新增生产代码扫描 `Tick(`、`DeltaSeconds`、`SetTimer`、`ApplyDamage`、impact/vitality commit、spawn 与 RNG：命中 `0`；
- 没有源码构建失败、环境错误、内存错误或外层超时。

## 8. 修改范围

- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.h`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.cpp`
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`
- 本 Report 与同名 Development Log。

Source 净改动：新增 `264` 行，删除 `48` 行。未修改 Build.cs、GameplayTags、Profile schema、存档、物品 authority、输入或资产。

## 9. P/F 边界

本 Report 只包含 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-16-orbit-threat-finalization/Docs/Report/Dev.D.UE.0.0.10.P6.16.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-16-orbit-threat-finalization/Docs/Log/Dev.D.UE.0.0.10.P6.16.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-16-orbit-threat-finalization>
