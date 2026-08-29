# Dev.D.UE.0.0.10.P7.0.r0 Report

## 1. 结论

P7.0 已完成“初级暗器百解”的第一份纯 Runtime 直线投掷契约，结论为 **PASS**。

每次执行必须绑定一个真实 `SourceItemInstanceId`，由内容冻结发射速度和伤害公式，由调用方在 Action commit 后提供起点与瞄准方向。Runtime 只接受一次 canonical 直线发射，随后以 `Projectile` candidate 进入既有 CombatCore Impact 管线；没有 Redirect、Recall、弧线、重力、自动寻路、Actor、输入或库存写入。

本轮把“暗器”和“御器”在类型层明确分开：暗器出手后不暴露持续控制接口，御器已有的 `ControlledObject + Redirect/Recall` 契约未被复用或改写。

## 2. 物品、定义与冻结数值

- 新增 `FShanmenThrownWeaponDefinitionCapture/Definition`；canonical action 为 `Combat.Action.ThrownWeapon.Straight01`；
- Definition 只接受有效 Detector、Formula、非负伤害参数、正发射速度、Physical damage 与 Living target 约束；
- 新增 `FShanmenThrownWeaponOffenseSnapshot`，在 activation 时冻结 technique power；
- 缺少 exact `SourceItemInstanceId` 的 Action 无法创建 execution；
- 测试使用的 `12 + 40 * 0.3 = 24` 与 `900` 速度只是夹具，不是 Runtime 默认数值或正式平衡内容；
- Definition、Offense 和 Action 都复制进入 execution，后续外部改动不会改变本次激活。

## 3. 直线发射与状态机

状态机为：

```text
Ready --LaunchStraight--> InFlight --FinishFlight--> Spent
```

- Startup 阶段拒绝发射，只有 Active/commit 后可以 Launch；
- 起点必须有限，方向必须有限且非零；方向在边界处单位化并规范正负零；
- `LaunchId` 由 Run、Owner、Activation、Source Entity、exact item、Action Definition、Content stamp、起点、canonical direction 与 authored speed 确定性派生；
- 相同起点和同向比例向量精确重放原 receipt，不重新启动飞行；
- 不同起点或方向构成冲突并失败关闭；
- Spent 后仍可幂等查询原 launch replay，但状态不会回到 InFlight；
- Flight 有活动 contact emission 时拒绝完成，必须先显式关闭。

## 4. Projectile Impact 管线

- 发射前和 Spent 后均不能开启 candidate emission；
- detector kind 固定为 `Projectile`，不会伪装成御器的 `ControlledObject`；
- self target、缺少 required target tags、错 activation/source/detector/kind 的 candidate 均失败关闭；
- target policy 失败不会消耗合法 candidate，修正 evidence 后可在同一 emission 接受；
- 重复 physics callback 由 emission 与 Impact ledger 双重去重；
- 同一 emission 的不同目标获得不同 ImpactId；后续 emission ordinal 使同一目标获得新 ImpactId；
- DamagePacket、DefenseResolver 和守恒 receipt 完全复用既有 CombatCore，不写 vitality 或 item authority。

## 5. 自动化证据

最终无头 `-NullRHI` 自动化全部通过：

| Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.ThrownWeapon` | 4 | 0 | 0 | `0.050s` | `A40BF32C46599BB56ACC04174B1BFF7514BF43A61804FA4175E0C396E8740A6A` |
| `Shanmen.0_0_10` | 175 | 0 | 0 | `8.395s` | `DC0241C824A32C81CA92EE3801E245D2003E38E56CC3D1B19E09BDA0BECB93DC` |

新增四项证明：

1. `PhysicalItemBoundary`：exact item、canonical definition、正速度与 frozen offense；
2. `StraightLaunchAndReplay`：commit fence、方向规范化、重放/冲突、open-window finish fence 与 Spent；
3. `ImpactPolicyAndOrdinals`：Projectile policy、24 点公式、重复去重、多目标与跨 sample ordinal；
4. `DeterminismAndTermination`：等价执行的 Launch/Impact identity、Content stamp 隔离与 terminal cleanup。

两份最终日志各只有一个实际 RunTests 命令、一个 queue-empty、Fail `0`、Fatal/assert/ensure `0`，原生退出码均为 `0`。测试发现前各有 13 条既存 `LogAutomationTest: Error: Condition failed` 诊断噪声；目标执行的 ControllerErrors 为 `0`，175 项结果全部 Success。

## 6. 改动—回归与静态门禁

- `REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=2`；
- mapping self-test：`16/16 PASS`；
- `git diff --check`：native exit `0`；
- 生产文件的 GetWorld/SpawnActor/ApplyDamage/TakeDamage/item Commit/Consume/Tick/timer/RNG 调用扫描命中 `0`；
- Source 合计 `+1170/-0`，生产 `+748/-0`、自动化 `+422/-0`；
- 未修改 Build.cs、GameplayTags、schema、存档、item authority、GameMode、输入或资产。

## 7. 构建与失败记录

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次 Editor integration：`8/8` actions，Succeeded，native exit `0`，`34.41s`；
- receipt identity hardening 后最终 Editor：`4/4` actions，Succeeded，native exit `0`，`4.52s`；
- 最终 Game：`5/5` actions，Succeeded，native exit `0`，`23.39s`；
- Editor Runtime DLL UTC：`2026-08-29T09:03:30Z`；
- Game executable UTC：`2026-08-29T09:05:09Z`。

本阶段没有源码、环境、内存、SDK、外层超时、自动化或链接失败。初始聚焦测试也是 `4/4`；最终日志是在 receipt 自校验增强并重编译后重新生成。Game executable 仅构建，未启动。

## 8. 修改范围与兼容性

- `Source/ShanmenCombatRuntime/Public/ShanmenThrownWeaponExecution.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenThrownWeaponExecution.cpp`；
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenThrownWeaponExecutionTests.cpp`；
- 本 Report 与同名 Development Log。

P0–P6 的 CombatCore、Items、剑法、御器、产品 Host、Threat、Vitality 与 Defense API 均保留。P7.0 没有把暗器接入 `Ademo_mapSkillProjectile`，避免在权威、物品扣除和 Actor 生命周期尚未设计前复制 legacy fire-and-forget 路径。

## 9. P/F 边界与后续

本轮只执行 P 阶段源码、静态检查、无头 Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

下一阶段应建立 Items authority 的暗器 Reserve/Commit/Cancel adapter：Action commit 前预留 exact item，真实发射 commit 后消费，取消或失败释放；仍不应在同一阶段混入 Actor 弹道、输入或中级弧线求解。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-0-thrown-weapon-core/Docs/Report/Dev.D.UE.0.0.10.P7.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-0-thrown-weapon-core/Docs/Log/Dev.D.UE.0.0.10.P7.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-0-thrown-weapon-core>
