# Dev.D.UE.0.0.10.P6.3.r0 Report

## 1. 结论

P6.3 已完成受控武器的 World contact / canonical vitality delivery 接线，结论为 **PASS**。

物理飞剑的 sweep/overlap 回调现在只负责提供几何证据；稳定目标身份由 World Entity Registry 解析，伤害仍由冻结的 ControlledWeapon runtime 纯结算，唯一生命写入仍经 `Fdemo_mapCombatRunCoordinator`。Session 只有在权威 vitality 提交成功后才提交候选副本，因此 delivery 失败不会提前消耗命中序号或幂等 ledger。

## 2. 功能性

- 新增 `Fdemo_mapShanmenControlledWeaponWorldAdapter`，分别接收 UE sweep 与 overlap；
- 只接受当前 active Session、开放的 ControlledObject emission、与 Session 完整一致的 action/context；
- 通过既有 `FShanmenWorldHitAdapter` 和 Run-scoped Registry 生成稳定 `FShanmenHitCandidate`；
- 从目标 `Idemo_mapCombatVitalityHost` 捕获冻结 vitality snapshot；
- 在 Session 副本中执行候选解析，再把 `FShanmenControlledWeaponImpactReceipt` 交给 Coordinator；
- Coordinator 新入口复用既有 `DeliverResolvedPlayerImpactToM01Enemy`，没有新增第二套 ledger 或直接伤害写口；
- 权威提交成功后才替换正式 Session；失败时正式 Session 与目标 vitality 均不前移；
- 结果保留 target entity、纯结算 receipt、canonical commit receipt 与实际新增伤害。

## 3. 完整性

新增 3 个产品级自动化：

1. `SweepOverlapAtomicity`：覆盖 sweep、overlap、后续 ordinal、旧 context 拒绝、重复 callback 幂等、exact item identity 与 vitality receipt；
2. `DeliveryFailureRollback`：覆盖 Coordinator source mismatch 后 Session ledger 和 vitality 同时不变；
3. `UnregisteredContact`：覆盖未注册 transient Actor 不能进入候选或结算。

全量 `Shanmen.0_0_10` 从 P6.2 的 137 个增加到 140 个，最终 `140/140` Success。

## 4. 兼容性

- 未修改 CombatCore / CombatRuntime / WorldGameplay 契约；
- 未修改物品定义、Profile schema、资源事务或存档格式；
- 未把 legacy flying-sword 内容重命名为正式 0.0.10 内容；
- 未调用 `ApplyDamage` / `TakeDamage`，未新增 World、spawn、库存或资源直接写入；
- Coordinator 新 public overload 只做 receipt 类型适配，仍汇入既有 private canonical delivery；
- 旧 BasicSword、shape skill、projectile 和 enemy-to-player delivery 路径未改。

## 5. 修改范围

- `Source/demo_map/demo_mapShanmenControlledWeaponWorldAdapter.h/.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponWorldAdapterTests.cpp`；
- `Source/demo_map/demo_mapCombatRunCoordinator.h/.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- 本 Report 与同名 Log。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 和用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `165F458DC5567B1A0C66ECE2B3C7C1B4A02AEF844D34AD407E7903F5814393AB` |
| `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `43A4F6D61472987DDF97D2804FD5DD61283A27D07D1AA87A4C1DA08361B99FDB` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `8504EDAC1E87D960C8FE766B9ECC9DF7F1CA4F06EF90D22C3674D92342C9317D` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `BAB27D5A37D3B1ED34644C88C340AC1E4D5CA0E360498042ED3CE95955D17690` |
| `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `E3B86141FDC106B1DF38C46E0F4ABB086F3FED88C9F4A1EF2809E228D3BFAA1D` |
| `Shanmen.0_0_10` | 140 | 0 | 0 | `8B9826624D74B9286126806A6FCE12A5F8B9E36E250BB7F47D6D4D1C869BE96C` |

- 六条最终日志均有 queue-empty 标记，目标 fail 0，Fatal / unhandled / handled ensure 0；
- changed-file gate：`PASS Changed=6 Rules=2 Required=5 Logs=6`；
- regression coverage self-test：`8/8 PASS`；
- regression JSON parse：PASS；
- `git diff --check`：native exit `0`。

## 7. 首次失败与修正

首轮新组为 `2/3` Success、`1/3` Fail，native exit `0`，日志 SHA-256：
`ACA001CB08ED71E1EF8418D0A941DF5E9E2E32754C32DA59D83278441489F942`。

失败来自测试把六个不变量合并为一个布尔断言，无法指出具体浮点/receipt 条件；同一日志已显示 sweep 与 overlap 均实际各扣除 `0.9` vitality。修正仅拆分测试断言并为 vitality delta 使用显式 `0.001` 容差，未修改生产实现。重编译后新组 `3/3`，随后全部定向与全量回归通过。

## 8. 编译

Editor 最终命令：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `4/4` actions；
- `Result: Succeeded`；
- native exit `0`；
- `5.46s`。

Game 最终命令：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `25/25` actions；
- `Result: Succeeded`；
- native exit `0`；
- `92.28s`；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头自动化、Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-3-controlled-weapon-world-delivery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-3-controlled-weapon-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P6.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-3-controlled-weapon-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P6.3.r0_log.md>
