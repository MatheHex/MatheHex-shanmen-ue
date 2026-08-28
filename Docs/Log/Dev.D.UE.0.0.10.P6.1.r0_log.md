# Dev.D.UE.0.0.10.P6.1.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.1.r0`；
- 基线提交：`5bb7fdd63ae7f55ade159b78a6c011fb6233d3bc`（P6.0）；
- 分支：`agent/0.0.10-p6-1-controlled-weapon-authority-adapter`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标与边界判断

P6.0 已冻结受控飞剑纯执行，但明确把“该物理物品是否真的部署到 active Run”留给产品 Adapter。现有 ShanmenItems 已有 `Shanmen.Item.Weapon.FlyingSword` 语义标签、`DeploymentLock` 与 durable Run lifecycle；当前内容却没有冻结哪件 legacy weapon 应被迁移成飞剑。

因此本轮只建立 exact authority gate，不修改旧 Definition，不把 `TrainingBlade` 当作临时飞剑，不增加世界 Actor、输入或表现。

## 实现记录

### Product facade

`PrepareActiveRun` 只在 Game Thread 且 authority 为 Ready 时工作。它通过现有 Run lifecycle adapter 重建 durable correlation，要求 transient Runtime 的 active RunId 精确一致，再捕获只读 authority snapshot。

### Pure evidence gate

`PrepareFromEvidence` 逐项核对：

- snapshot content 与 lifecycle revision；
- exact weapon-slot / prepared-item identity；
- item owner、scope、singleton 与 `Deployed` state；
- authority Definition 的 deploy capability 与 exact FlyingSword tag；
- committed DeploymentLock 的 reservation、item、owner、scope、amount 与 revision advance。

拒绝结果不携带有效 evidence 或 execution。

### Frozen action

证据通过后捕获 P6.0 Definition/Offense，按 active Run、source entity、canonical action 与 activation sequence 派生 ActivationId。Action 保留 exact source item，并冻结 authority item tags。最后交给 `FShanmenControlledWeaponExecution::TryCreate`；适配器自身不复制控制状态机。

### Read-only discipline

生产实现没有 Reserve、Commit、Cancel、Start/End Run、Save/Write 等 mutation call，也不依赖 World、Actor、Pawn、Input、Spawn、ApplyDamage 或 RNG。正例测试比较完整 snapshot 与 correlation，验证调用前后值不变。

## 新增自动化

- `Shanmen.0_0_10.Product.ControlledWeaponAdapter.AuthorityGate`；
- `Shanmen.0_0_10.Product.ControlledWeaponAdapter.FailClosed`；
- `Shanmen.0_0_10.Product.ControlledWeaponAdapter.DeterministicReplay`；
- `Shanmen.0_0_10.Product.ControlledWeaponAdapter.ProductFacadeBoundary`。

覆盖 exact authority success、authority tags、无 mutation、wrong item、stored item、generic deployable、uncommitted lock、stale snapshot、item revision mismatch、deterministic replay 与 unbound facade。

## 首次失败记录

### 首次 Editor build

测试比较 `FShanmenContentStamp` 时使用了不存在的 `operator==`：

- UBT：`Result: Failed (OtherCompilationError)`；
- native exit：`1`；
- 约 `34.07s`；
- 修复：分别比较 `Version` 与 `Digest`。

### 首次 focused automation

| 日志 | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `p61_controlled_weapon_adapter_focused.log` | 3 | 1 | 0 | `80CDD3270394533DFD90A168ABEA6CC5A3F34E62EADC32110DBF46DB5E441297` |

失败测试是 `ProductFacadeBoundary`。旧夹具直接以 `GetTransientPackage()` 为 Outer 创建 GameInstanceSubsystem，UE 在 UObjectGlobals 3318 触发 handled ensure。修复为构造 transient `UGameInstance`、`Init()` 后通过 `GetSubsystem` 取得真实子系统，并在结束时 `Shutdown()`。首次失败日志保留为独立文件。

## 最终自动化

统一命令形态：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p61_controlled_weapon_adapter_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `365065E0B8C76F0D61441457A6DD137016970143A3DA3E20EA371A730187F1F9` |
| `p61_items_regression_final.log` | `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `84A58A8AF0EE4113732DC88C0F7D0DFD13C078B4F21DF45E463498E589EF0A58` |
| `p61_combat_runtime_regression_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `AA4DA38EB64E381E9FD83A9ACFB0E0412EA24D260405F67119D09D055AE09828` |
| `p61_full_regression_final.log` | `Shanmen.0_0_10` | 133 | 0 | 0 | `CE79AD03964CBF06A2585025AE8F3FDA2EA44C443FDBA21593E83335B9521982` |

四条最终日志均 queue empty、native exit `0`、无 test failure、handled ensure、assert 或 Fatal。最终唯一计数为 `133/133`。

父组日志在 test discovery 前保留既有 13 行 `LogAutomationTest: Error: Condition failed` 启动诊断；目标测试全部成功。

## Changed-file gate

新增 adapter 路径映射到 Product adapter、Items、CombatRuntime 三组。实际门禁：

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=1 Required=3 Logs=4
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=p61_combat_runtime_regression_final.log,p61_full_regression_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Items Evidence=p61_items_regression_final.log,p61_full_regression_final.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Product.ControlledWeaponAdapter Evidence=p61_controlled_weapon_adapter_final.log,p61_full_regression_final.log
```

`Test-ShanmenRegressionCoverageSelfTest.ps1`：`8/8 PASS`。

## 构建

最终 Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `4/4` actions；
- `Result: Succeeded`；
- native exit `0`；
- `6.99s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `4/4` actions；
- `Result: Succeeded`；
- native exit `0`；
- `27.22s`；
- 输出 `Binaries/Win64/demo_map.exe`，未启动。

## 静态审查

- `git diff --check`：native exit `0`；
- regression JSON parse：PASS；
- regression coverage self-test：8/8 PASS；
- 生产 adapter forbidden World/Actor/Input/Spawn/Damage/RNG：0；
- 生产 adapter item/resource mutation call：0；
- `TrainingBlade` / fallback：0。

## P/F 边界

只执行 P 阶段代码、静态审查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续约束

真实产品接线必须继续保留 exact ItemInstanceId 与 active Run correlation；世界运动、输入、碰撞和 VFX 只能作为外层 Adapter。具体飞剑内容、旧物品迁移和耐久/修理事务尚未冻结，不能以 legacy name fallback 或直接资源写入替代正式设计。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-1-controlled-weapon-authority-adapter/Docs/Report/Dev.D.UE.0.0.10.P6.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-1-controlled-weapon-authority-adapter/Docs/Log/Dev.D.UE.0.0.10.P6.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-1-controlled-weapon-authority-adapter>
