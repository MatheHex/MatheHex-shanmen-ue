# Dev.D.UE.0.0.10.P7.3.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.3.r0`；
- 基线提交：`d90328b473cc4214763e26b3bcf3c741ae5cde2c`（P7.2）；
- 分支：`agent/0.0.10-p7-3-thrown-weapon-run-host`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与决策

P7.2 已冻结 durable launch 和 World delivery，但没有对象负责生成 Actor、保存 action/execution、绑定 contact delegate 或结束 action。把这些职责塞进 projectile 会重新制造 Actor 自算伤害/库存的第二套 authority；直接塞进输入又会让键位层拥有事务与 World 生命周期。

P7.3 因此新增一个不可复制的 product RunHost。World spawner 只生成 inert carrier；P7.2 仍拥有 stage/commit/publication；RunHost 只在 publication 成功后采用 exact flight，并拥有 transient lifecycle 与 action terminal。

## 生成与采用

`SpawnStagedCarrier` 验证 World、source、class 和 origin，以 `AlwaysSpawn` 创建 P7.2 Actor。构造器保证生成结果仍为 Empty/NoCollision/inactive。

产品入口有两条：

- `TrySpawnAndLaunchPrepared`：生成 + P7.2 durable launch + host adopt；
- `TryLaunchPreparedCarrier`：适配外部 deferred/pre-spawn carrier。

恢复入口 `TryAdoptPublishedFlight` 必须同时匹配 active action、in-flight execution、committed item evidence、projectile launch/context、coordinator Run 与 source entity。它不执行 inventory IO，因此 recovery/replay 不会二次扣除。

## Delegate 与 terminal

host 绑定 projectile contact 和 range expiry 两个 native delegate。contact 成功交付后结束 execution、完成 action、解除委托并销毁 owned Actor。不可交付 blocking contact 记为带原错误诊断的 BlockingMiss，再走 P7.2 no-impact terminal。range lifespan 来源严格为 `MaximumDistance / LaunchReceipt.Speed`。

显式 interruption 先在 action/execution 副本上验证两个终态均可成立，再发布 Interrupted，避免半终态。普通 impact/miss/range 使用 Active -> Recovery -> Completed。terminal receipt 在 Actor 销毁后保留稳定 LaunchId、delivery 与 action receipts。

RunHost 是 GameThread-owned。若 owner 在 active flight 中直接析构，析构函数自动走同一 Interrupted terminal 并销毁 owned World carrier，避免只解除委托后留下孤立飞行 Actor。

## 自动化增量

新增三项：

- `SpawnGate`：null World 失败关闭，真实无头 GamePreview World 生成 exact inert carrier，recovery adoption、active reset gate 与 interruption；
- `ContactLifecycle`：native delegate 自动交付 0.7 vitality、完整 execution/action/Actor terminal，解除绑定后 replay 不前进 authority revision；
- `MissAndRangeExpiry`：未注册 blocking contact 保留 ContactNotResolved 诊断并无 impact 收口，range expiry 幂等 terminal。

## 最终自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p73_thrown_host_final.log` | `Shanmen.0_0_10.Product.ThrownWeaponRunHost` | 3 | 0 | 0 | `3830F50CBF81F62A10A2FB8B29CB1711327456EFD09B85E2C73E23B2DBC64DDB` |
| `p73_thrown_world_final.log` | `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery` | 3 | 0 | 0 | `D46082C935AB0C8607DDD3BE6D22B7B989934285BAE8893CA8370BE1251D9A5E` |
| `p73_item_adapter_final.log` | `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter` | 4 | 0 | 0 | `F53FB699D5AECEBFD59793B8C2073DB2EF3072198DFE3C0C1660DB2281941A51` |
| `p73_coordinator_final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `DF02B67DDD3A67FABF7473FF61D2B6CD4E8E1C9CDEE64040B3F956C9E7C9E49A` |
| `p73_items_final.log` | `Shanmen.0_0_10.Items` | 72 | 0 | 0 | `2E4163129C5D2A513CB096EACED31673633BBECF4B054130FEFABD179AD87EA5` |
| `p73_world_gameplay_final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `CA192DFB212D4A24E20810E9D907E506E3868272D90109D5112C4490C8B3E6F9` |
| `p73_combat_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 30 | 0 | 0 | `DE8D112C50099AC1C61BC7A0FB8D09FBA6E3937E44F327592887718C22C6B7DA` |
| `p73_full_final.log` | `Shanmen.0_0_10` | 188 | 0 | 0 | `23B981CDA717336BDB95AD15358C1BE13078E3A0A39EFEC134A3417CB9AA5EFA` |

最终日志均有 queue-empty，fatal/unhandled/ensure marker 为 0。Automation discovery 阶段保留项目既有 `Condition failed` 噪声，但目标 ControllerResults 全部 Success。

## Changed-file gate

新增 `ThrownWeaponRunHost` 映射，并要求 RunHost、World delivery、item adapter、coordinator、Items、WorldGameplay、CombatRuntime 七组：

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=7 Logs=8
SELF_TEST: PASS 18/18
```

## 构建时间线

统一使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor integration：`8/8`，Succeeded，exit `0`，`22.03s`；
- tests integration：`4/4`，Succeeded，exit `0`；
- copy-on-write interruption 修正：`5/5`，Succeeded，exit `0`；
- 最终 Editor：`5/5`，Succeeded，exit `0`，`9.86s`；
- 首次 Game：`7/7`，Succeeded，exit `0`，`34.91s`；
- 最终 Game：`4/4`，Succeeded，exit `0`，`14.18s`。

本轮没有失败构建或失败测试。首次 host suite 即为 3/3；真实 World spawn 证据加入后重新执行了最终 host 与 full suite。

## 静态、范围与兼容性

- staged diff check：native exit `0`；
- mapping self-test：`18/18`；
- 新 host 无 ApplyDamage、legacy projectile、GameMode、timer authority、RNG 或资产加载；
- SpawnActor 只位于明确的 World carrier factory；
- 未改 schema、Build.cs、GameplayTags、Content、输入、GameMode、Profile 或 CodeB；
- 0.0.10 full suite `188/188`；
- 长期未跟踪的 0.0.9B 与用户文件保持未跟踪且未 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook 或 Package。

## 下一阶段

P7.4 建立 thrown weapon Run command router，把选择后的 item/action request、P7.1 prepare 与 P7.3 spawn-and-launch 串成单一 command/receipt。输入映射、动画与视觉继续后置，输入层不得直接修改 inventory、Actor 或 vitality。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-3-thrown-weapon-run-host/Docs/Report/Dev.D.UE.0.0.10.P7.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-3-thrown-weapon-run-host/Docs/Log/Dev.D.UE.0.0.10.P7.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-3-thrown-weapon-run-host>
