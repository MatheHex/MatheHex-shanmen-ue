# Dev.D.UE.0.0.10.P11.3.r0 Report

## 1. 结论

P11.3 已完成并通过 P 阶段门禁。

本阶段新增唯一的武器格挡 World 方向采样适配边界：调用方显式提供当前 `UWorld`、Run-scoped `FShanmenWorldEntityRegistry`、防御者 Actor、威胁 Actor、P11.0 窗口、P11.1 时序 receipt、P11.2 夹角 policy 与 hit candidate；适配器在一次同步调用内验证完整身份链，各读取一次 Actor transform，并把明确的 `GuardFacing` 与 defender-to-threat 方向交给 P11.2 纯 evaluator。

最终结果：

- 适配器与 5 个 focused tests 编译成功；
- 8 组最终 Automation 日志共记录 527 次 Success、0 次 Fail，其中 0.0.10 全量为 478/478；
- final exact-stage regression gate：`PASS Changed=7 Rules=1 Required=8 Logs=8`；
- mapping 92 rules，自检 144/144；
- `git diff --cached --check` 通过；
- Editor Development 与 Game Development 单并发最终构建均为原生退出码 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 同步 World 采样

`Fdemo_mapShanmenWeaponGuardWorldAdapter::Evaluate` 执行以下顺序：

1. 验证 P11.0/P11.1/P11.2 frozen inputs 与 candidate；
2. 验证 exact active guard window、timing projection 与 arc policy 绑定；
3. 验证调用方提供的 World、active registry 与同一 Run；
4. 验证两个 Actor 存活、未销毁且属于同一显式 World；
5. 通过现有 `FShanmenWorldEntityRegistry` 解析稳定实体身份；
6. 验证 defender、guard action source、candidate target 三者一致；
7. 验证 threat Actor 与 candidate source 一致；
8. 各读取一次 defender location、threat location 与 defender forward；
9. 生成 P11.2 canonical direction sample 并执行 arc evaluation；
10. 返回不含 UObject 指针的确定性审计结果。

### 2.2 结果语义

- `Qualified`：原样保留 P11.1 已互斥选择的 Perfect 或 Ordinary layer；
- `OutsideArc`：仍是成功且可审计的 World evaluation，但不携带可消费防御 layer；
- actor coincident、transform 非有限、inactive registry、跨 World、错误 Run、未注册实体或身份不一致：fail closed，不发布部分 receipt；
- Recovery/Interrupted：由 P11.0 lifecycle 先行关闭，World adapter 不复制窗口状态机；
- 相同 frozen inputs 与相同 transform 产生同一 receipt；位置变化会生成新的 World receipt；
- receipt identity 包含 Run、稳定实体、两个精确位置、P11.2 sample ID 与 evaluation ID。

## 3. 完整性

适配器只建立产品 World 到纯 Runtime evaluator 的窄桥，不建立第二套 transform、目标、实体或生命周期权威：

```text
caller-owned cadence
  + explicit World
  + active Run entity registry
  + explicit defender/threat Actors
  + exact P11.0 window
  + exact P11.1 timing receipt
  + exact P11.2 arc policy
  + explicit hit candidate
  -> World/run/identity fences
  -> one defender location sample
  -> one threat location sample
  -> one defender forward sample
  -> P11.2 TryCapture + TryEvaluate
  -> UObject-free deterministic World receipt
```

没有新增：

- Tick、timer、异步任务或重试循环；
- actor discovery、trace 或碰撞查询；
- 输入读取或 action lifecycle；
- damage、Impact、资源、库存或耐久写入；
- `HitNormal` 方向推断；
- UObject/Actor 指针持久化。

## 4. 兼容性

- 未修改 `ShanmenCombatCore`、`ShanmenCombatRuntime`、P11.0、P11.1、P11.2 或 World registry；
- 使用既有 generic Actor registry binding，不创建武器格挡专用实体表；
- P11.2 继续是唯一夹角数学与 selected-layer 判定权威；
- P11.0 继续是唯一窗口生命周期权威；
- P11.1 继续是 Perfect/Ordinary 互斥选择权威；
- adapter 只消费显式 live Actor 与 frozen receipts，不改变旧战斗、物品、属性或存档路径。

## 5. 修改范围

实现与门禁共 5 个文件、917 行新增、0 行删除：

- `Source/demo_map/demo_mapShanmenWeaponGuardWorldAdapter.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardWorldAdapter.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardWorldAdapterTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与其它资料未修改、未暂存、未提交。

## 6. 测试覆盖

新增 focused tests：

1. `QualifiedPerfect`：注册的正面威胁透传 Perfect layer，并且不修改 registry；
2. `LiveTransforms`：正面 Ordinary 成功，威胁移到背面后得到 auditable no-layer，静止重放 ID 相同；
3. `IdentityFences`：错误 target/source、未注册 threat 与 actor alias self-threat 全部 fail closed；
4. `EnvironmentFences`：null World、跨 World、coincident actors 与 inactive registry 全部拒绝；
5. `LifecycleReplay`：相同 World sample 可确定性重放，Recovery 后不发布部分 receipt。

最终 Automation 证据：

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardWorldAdapter` | 5 | 0 | 1 | `C227980111FE34E6D744B680122E7F0B42065FDE368173EC4DD6CA17B69E2B00` |
| `Shanmen.0_0_10.CombatRuntime.WeaponGuardArc` | 6 | 0 | 1 | `50213DADD1A2D38687F96A2B1F45A3407C2FEE3300371A8616F100515A2956DF` |
| `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard` | 6 | 0 | 1 | `15867265378B414CFC040A9051067E9BF023B524F48CF0F7991295C2F4F5D04F` |
| `Shanmen.0_0_10.CombatRuntime.WeaponGuard` | 12 | 0 | 1 | `AA555E1DB9A1D842857F167C1B5EB61841BE6AB9192F4D2146858F72B36331E9` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | 1 | `3C485EA52FE787B71CC097C3CCB2932CF2D50F5B8CB9E08EDAEC416BB490FDC8` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | 1 | `038552E776B436095FFDB54ACD16C2F95F13E196ADD0102EAC0CFCC19090A942` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 1 | `81F68B5A0370FB878B162F56EE57661C33D5F48EB8FCFE528B3042E81C959582` |
| `Shanmen.0_0_10` | 478 | 0 | 1 | `CC339C33D260742631BF313FCFCD84CC21397EEAC7B58718979EB7C8A4B54137` |

八组日志合计 527 次 Success / 0 次 Fail；该合计包含 focused 与全量之间的重复覆盖，不表示 527 个唯一用例。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=92
SELF_TEST: PASS 144/144
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=8 Logs=8
git diff --cached --check: PASS
BOUNDARY_SCAN: PASS hits=0
```

- 实现／门禁 5 文件的 pre-document gate 同样通过：`Changed=5 Rules=1 Required=8 Logs=8`；
- mapping SHA-256：`11A78425392127F2C5B40FB16640CD5F344273EC4204DBF886A02DB76F52E2A2`；
- self-test SHA-256：`610F8AF6F687A9E0E64CFC9F623CB6D9F8AFCD37C05564670D41DA5DF1F45AFC`；
- boundary scan 检查 production adapter 不含 `HitNormal` 读取、trace/sweep、timer/input、damage、资源事务、actor discovery 或 UObject pointer retention。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development（首次） | Failed / missing direct test include | 7 scheduled / 21.71s | 6 | `36689A30836D950514C4B9AB0088503A01A5F767930B863A195B83BB79511853` |
| Editor Development（最终） | Succeeded | 4 / 7.50s | 0 | `CEA0272BA9D7DB27EB0FDB681DBC28C8228C8AE33300F19D96FFB2A6D4EC73EC` |
| Game Development（最终） | Succeeded | 4 / 23.51s | 0 | `71E7C5A3C41D0A3784C0719FEAB44CD4BD804FBA29667E9AEBFC0941065DAD92` |

- `UnrealEditor-demo_map.dll`：12,551,680 bytes，SHA-256 `672F2BFF7C122F663558FC974F367FD155277587FE6B2F78486E88245683D8A3`；
- `demo_map.exe`：354,123,264 bytes，SHA-256 `CF07DECFC96EB1E81D263695E3C111A47A4082BDB9C97AF59CECD7517F109C99`。

## 9. 真实异常与修正

首次 Editor 构建原生退出码为 6，UBT 分类为 `OtherCompilationError`。生产 adapter 已通过编译，失败位于新增测试夹具：测试直接调用 `FShanmenCombatIdFactory`，但未直接包含 `ShanmenCombatResolver.h`，产生 C2653/C3861。补齐显式 include 后，Editor 与 Game 最终构建均成功。该失败是测试源码依赖缺失，不是内存、页面文件或环境错误。

首次批量启动其余七组 Automation 时，PowerShell 参数拼接把组名拆为独立参数。七个进程虽然退出 0，但日志明确显示 `RunTests=''`、Success=0。证据审计将整轮判为无效，原日志另存为 `*-invalid-command.log`；修正为单一 `-ExecCmds=Automation RunTests <group>` 参数后完整重跑七组。最终表格、SHA 与 changed-file gate 只使用重跑日志，没有把“零测试退出 0”记作成功，也没有放宽 parser 或 required groups。

UE 5.8 启动日志中的 UnifiedErrorTest、非 Win64 SDK metadata 与可选 profiling DLL 信息不属于 selected tests；Win64 构建、最终 selected tests、terminal marker 与原生退出码均正常。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段产品适配代码、代码审查、无头 Automation、静态／路径门禁以及一次必要的最终 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或大规模 F 阶段回归。

建议 P11.4 建立唯一的武器格挡产品协调器：组合 caller-owned cadence、P11.0/P11.1/P11.2/P11.3，并只把合格的 selected defense layer 交给既有 Impact 路由；不得复制 window、timing、arc、entity registry 或 resolver。若加入武器耐久，必须走 ShanmenItems durable prepare/commit/cancel 事务，不能由 World adapter 直接写库存或耐久。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-3-weapon-guard-world-adapter>
