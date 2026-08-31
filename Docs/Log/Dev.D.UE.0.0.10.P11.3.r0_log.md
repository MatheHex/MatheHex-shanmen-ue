# Dev.D.UE.0.0.10.P11.3.r0 Development Log

## 目标

把 P11.2 所需的两个显式方向从既有 live Actor 与 Run-scoped entity registry 中一次性采样，并把它们交给纯 evaluator。新增边界必须无状态、同步、fail closed，不保存 UObject 指针，不读取 `HitNormal`，不建立第二套 transform、目标、实体或生命周期权威。

## 基线

- branch：`agent/0.0.10-p11-3-weapon-guard-world-adapter`；
- base：`a0018c1c288e4d47237bd1ed57c73e173f7ee562`；
- P11.0：普通武器格挡 Active-only window 与 Ordinary Guard layer；
- P11.1：同一 window 上 Perfect/Ordinary 互斥 timing projection；
- P11.2：显式 GuardFacing、defender-to-threat direction 与 arc evaluation；
- entity identity：既有 `FShanmenWorldEntityRegistry`；
- P 阶段，不启动产品。

## 审计结论

1. P11.2 已明确禁止从 `HitNormal` 猜方向；
2. 产品层已有 Run-scoped object/actor 到 stable entity 的 registry；
3. 既有 formation World sampler 证明 caller-owned cadence + explicit actor subset 是当前架构惯例；
4. adapter 不应扫描 World、保存 actor、拥有 tick 或复制 combat lifecycle；
5. defender 必须同时等于 guard action source 与 candidate target；
6. threat 必须等于 candidate source；
7. 两个 Actor 必须属于同一显式 World 与同一 active Run registry；
8. transform 只能在一次同步调用内各采样一次；
9. P11.2 是唯一向量规范化、点积和 layer 资格权威；
10. World receipt 必须包含精确位置，防止移动前后证据混淆；
11. 结果应保留 stable IDs、位置与 immutable receipts，不保留 UObject pointer；
12. OutsideArc 是可审计成功结果，但没有可消费 layer。

## 实现

新增：

- `Edemo_mapShanmenWeaponGuardWorldStatus`：17 个精确阶段状态；
- `Fdemo_mapShanmenWeaponGuardWorldResult`：UObject-free audit result；
- `Fdemo_mapShanmenWeaponGuardWorldAdapter::Evaluate`：唯一同步适配入口；
- deterministic namespace：`Shanmen.Sword.WeaponGuard.WorldEvaluation.r1`。

执行顺序：

1. 验证 frozen runtime inputs；
2. 验证 exact active P11.0/P11.1/P11.2 chain；
3. 验证 World；
4. 验证 registry active 与 Run 一致；
5. 验证 Actor live 与 same World；
6. 解析 stable entity IDs；
7. 验证 defender/threat/candidate/action identity；
8. 一次采样两个 location 与 defender forward；
9. 调用 P11.2 `TryCapture`；
10. 调用 P11.2 `TryEvaluate`；
11. 生成 deterministic World receipt；
12. 通过 `IsSuccess` 复核 staged evidence 后返回。

## 测试

新增 5 个 focused tests：

- `QualifiedPerfect`
- `LiveTransforms`
- `IdentityFences`
- `EnvironmentFences`
- `LifecycleReplay`

最终证据：

| Log | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `P11.3-WeaponGuardWorldAdapter-final.log` | 5 | 0 | 1 | `C227980111FE34E6D744B680122E7F0B42065FDE368173EC4DD6CA17B69E2B00` |
| `P11.3-WeaponGuardArc-final.log` | 6 | 0 | 1 | `50213DADD1A2D38687F96A2B1F45A3407C2FEE3300371A8616F100515A2956DF` |
| `P11.3-WeaponPerfectGuard-final.log` | 6 | 0 | 1 | `15867265378B414CFC040A9051067E9BF023B524F48CF0F7991295C2F4F5D04F` |
| `P11.3-WeaponGuard-final.log` | 12 | 0 | 1 | `AA555E1DB9A1D842857F167C1B5EB61841BE6AB9192F4D2146858F72B36331E9` |
| `P11.3-ActionLifecycle-final.log` | 1 | 0 | 1 | `3C485EA52FE787B71CC097C3CCB2932CF2D50F5B8CB9E08EDAEC416BB490FDC8` |
| `P11.3-CombatCore-final.log` | 9 | 0 | 1 | `038552E776B436095FFDB54ACD16C2F95F13E196ADD0102EAC0CFCC19090A942` |
| `P11.3-WorldGameplay-final.log` | 10 | 0 | 1 | `81F68B5A0370FB878B162F56EE57661C33D5F48EB8FCFE528B3042E81C959582` |
| `P11.3-Shanmen-0_0_10-final.log` | 478 | 0 | 1 | `CC339C33D260742631BF313FCFCD84CC21397EEAC7B58718979EB7C8A4B54137` |

总计 527 Success / 0 Fail；包含 focused 与全量的重复覆盖。

## 回归映射

新增 `WeaponGuardWorldAdapter` rule，要求：

- `Shanmen.0_0_10`
- `Shanmen.0_0_10.Product.WeaponGuardWorldAdapter`
- `Shanmen.0_0_10.CombatRuntime.WeaponGuardArc`
- `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard`
- `Shanmen.0_0_10.CombatRuntime.WeaponGuard`
- `Shanmen.0_0_10.CombatRuntime.ActionLifecycle`
- `Shanmen.0_0_10.CombatCore`
- `Shanmen.0_0_10.WorldGameplay`

结果：

```text
REGRESSION_MAP_JSON: PASS Rules=92
SELF_TEST: PASS 144/144
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=8 Logs=8
git diff --cached --check: PASS
BOUNDARY_SCAN: PASS hits=0
```

实现／门禁 5 文件的 pre-document gate 同样通过：`Changed=5 Rules=1 Required=8 Logs=8`。

## 构建

| Build | Result | Actions / Time | Exit | SHA-256 |
|---|---|---|---:|---|
| Editor first | Failed | 7 scheduled / 21.71s | 6 | `36689A30836D950514C4B9AB0088503A01A5F767930B863A195B83BB79511853` |
| Editor final | Succeeded | 4 / 7.50s | 0 | `CEA0272BA9D7DB27EB0FDB681DBC28C8228C8AE33300F19D96FFB2A6D4EC73EC` |
| Game final | Succeeded | 4 / 23.51s | 0 | `71E7C5A3C41D0A3784C0719FEAB44CD4BD804FBA29667E9AEBFC0941065DAD92` |

## 真实异常

1. 首次 Editor 构建：测试夹具缺少 `ShanmenCombatResolver.h`，导致 `FShanmenCombatIdFactory` C2653/C3861，退出码 6。补 include 后最终构建成功。
2. 首次七组批量 Automation：PowerShell 把测试组拆为单独参数，进程退出 0 但日志 `RunTests=''` 且 0 tests。整轮隔离为 `*-invalid-command.log`，修正 canonical `-ExecCmds` 后完整重跑；最终证据不含无效轮。
3. 没有源码逻辑测试失败、内存/页面文件错误或 Win64 SDK 失败。

## 修改统计

```text
5 files changed, 917 insertions(+)
```

加入 Report/Log 后预期 exact stage 为 7 个文件。长期未跟踪资料不进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-3-weapon-guard-world-adapter>
