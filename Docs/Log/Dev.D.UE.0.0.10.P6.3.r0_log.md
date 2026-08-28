# Dev.D.UE.0.0.10.P6.3.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.3.r0`；
- 基线提交：`c64ea791ef94a509f929c39a83c153e2dece379d`（P6.2）；
- 分支：`agent/0.0.10-p6-3-controlled-weapon-world-delivery`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.2 已把 action lifecycle、exact deployed item、控制命令和 detector emission 收进一个原子 Session，但尚未接 UE 世界 contact，也没有把合法 ControlledWeapon receipt 交给产品唯一 vitality authority。

P6.3 的边界因此冻结为三层：UE contact 只转稳定候选；ControlledWeapon runtime 只做纯结算；CombatRunCoordinator 继续拥有唯一 vitality commit。任何一层失败都不能让正式 Session 先前移。

## 实现记录

### Contact translation

新增 `ResolveSweepContact` 和 `ResolveOverlapContact`。两者先检查 Coordinator、Session、开放 emission、ControlledObject detector 与完整 action/context 一致性，再复用 `FShanmenWorldHitAdapter` 和当前 Run 的 Entity Registry。Transient pointer、Actor name 和 callback order 不参与身份。

### Frozen target input

候选目标必须实现并绑定 `Idemo_mapCombatVitalityHost`，且其 canonical entity id 必须与 Registry candidate 一致。适配器只捕获 vitality snapshot，并补充既有 `Target.Living` 标签；不直接读取或写入敌人私有生命字段。

### Atomic delivery

适配器复制完整 Session，在副本中解析 candidate。合法 receipt 经新 `DeliverControlledWeaponImpactToM01Enemy` overload 进入既有 `DeliverResolvedPlayerImpactToM01Enemy`，继续使用同一 `FShanmenVitalityCommitLedger`。只有 delivery success 后才把副本写回正式 Session。

### Regression map

新增 `ControlledWeaponWorldDelivery` 路径规则，强制要求新组、Session、CombatRunCoordinator、WorldGameplay 与 CombatRuntime 五个测试组。与 Coordinator 既有规则取并集，禁止只跑主题测试。

## 自动化覆盖

- sweep 与 overlap 的稳定 candidate 和 canonical commit；
- exact source item identity；
- 同一 callback 重复拒绝；
- contact window 后续 ordinal 与 distinct ImpactId；
- 旧 context 不能消费新窗口；
- Coordinator source mismatch 后 Session/vitality rollback；
- 未注册 Actor 不进入候选。

## 验证期间修正

首次 focused 日志 `p63_controlled_weapon_world_delivery.log`：`2/3` Success、`1/3` Fail、queue empty、native exit `0`、SHA-256 `ACA001CB08ED71E1EF8418D0A941DF5E9E2E32754C32DA59D83278441489F942`。

失败测试把 delivery、commit status、item id、raw damage 和 vitality delta 六个条件压在一个断言中。运行日志同时记录两次 `0.9` 的真实 canonical damage，说明生产路径已执行。修正将不变量拆成独立断言，并给浮点 vitality delta 显式 `0.001` 容差。未修改生产代码。增量 Editor build `4/4` 成功后，新组最终 `3/3`。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p63_controlled_weapon_world_delivery_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `165F458DC5567B1A0C66ECE2B3C7C1B4A02AEF844D34AD407E7903F5814393AB` |
| `p63_controlled_weapon_session_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `43A4F6D61472987DDF97D2804FD5DD61283A27D07D1AA87A4C1DA08361B99FDB` |
| `p63_combat_run_coordinator_final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `8504EDAC1E87D960C8FE766B9ECC9DF7F1CA4F06EF90D22C3674D92342C9317D` |
| `p63_world_gameplay_final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `BAB27D5A37D3B1ED34644C88C340AC1E4D5CA0E360498042ED3CE95955D17690` |
| `p63_combat_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `E3B86141FDC106B1DF38C46E0F4ABB086F3FED88C9F4A1EF2809E228D3BFAA1D` |
| `p63_full_final.log` | `Shanmen.0_0_10` | 140 | 0 | 0 | `8B9826624D74B9286126806A6FCE12A5F8B9E36E250BB7F47D6D4D1C869BE96C` |

六条最终日志都存在唯一 RunTests command、至少一个 success、fail 0、queue empty，且 Fatal / unhandled / handled ensure 为 0。全量唯一计数 `140/140`。

## Changed-file gate

```text
REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=5 Logs=6
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=p63_combat_runtime_final.log,p63_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.CombatRunCoordinator Evidence=p63_combat_run_coordinator_final.log,p63_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponSession Evidence=p63_controlled_weapon_session_final.log,p63_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery Evidence=p63_controlled_weapon_world_delivery_final.log,p63_full_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.WorldGameplay Evidence=p63_world_gameplay_final.log,p63_full_final.log
```

Regression coverage self-test：`8/8 PASS`。

## 构建

首轮完整 Editor integration build：`28/28` actions，`Result: Succeeded`，native exit `0`，`111.41s`。

测试断言修正后的最终 Editor build：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `4/4` actions；
- `Result: Succeeded`；
- native exit `0`；
- `5.46s`。

最终 Game build：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `25/25` actions；
- `Result: Succeeded`；
- native exit `0`；
- `92.28s`；
- 生成 `Binaries/Win64/demo_map.exe`，未启动。

## 静态与兼容性

- `git diff --check`：native exit `0`；
- regression JSON parse：PASS；
- 新 World adapter 的 `ApplyDamage` / `TakeDamage` / direct inventory-resource mutation：0；
- WorldGameplay / CombatRuntime 生产契约未改；
- item definition、Profile schema、资源事务、存档格式未改；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

P6.4 可在不改变本轮 contact/delivery 契约的前提下接入受控武器产品 Actor/input movement owner：Actor 负责位置与 steer sampling，Session 负责 lifecycle，World adapter 负责接触，Coordinator 负责唯一生命写入。具体飞剑数值与正式 item content 仍需单独冻结。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-3-controlled-weapon-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P6.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-3-controlled-weapon-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P6.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-3-controlled-weapon-world-delivery>
