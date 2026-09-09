# Dev.D.UE.0.0.10.P22.9.r0 Development Log

## 1. 基线与目标

- base：`f5367865d929b4959ca5144f5cd53e40d732f293`（P22.8 hand-side release origin）；
- branch：`agent/0.0.10-p22-9-thrown-weapon-release-clearance`；
- 目标：阻止手侧飞刀释放体积位于阻挡几何体内部时进入耐久提交与飞行发布；
- 边界：不改释放点、轨迹、速度、伤害、库存权威、Run 身份或终态，不启动 UI/PIE/产品 executable。

## 2. 风险审计

P22.8 把 Origin 移到角色手侧，但 Carrier 为保证事务顺序而使用 `AlwaysSpawn`，且在 durable commit 前保持 `NoCollision`。若手侧点位于墙内，旧流程仍能完成暂存，提交物品后才启用碰撞，可能造成穿墙起飞、零距离阻挡或异常终止。

正确拦截点不是 Spawn，也不是飞行首帧，而是 `TryStageLaunch()`：这里已经拥有冻结的 Launch、实际初速度与 Projectile 物理组件，同时仍位于物品提交之前。

## 3. 实现取舍

在 `demo_mapShanmenThrownWeaponProjectile.cpp` 的匿名命名空间新增 `IsLaunchVolumeClear()`：

- 从 Projectile 读取 World 与 BoxCollision；
- 使用 `Launch.GetOrigin()` 和初速度旋转；
- 使用盒体缩放后范围、Object Type 与 Response Container；
- 查询参数只忽略 Projectile 和 SourceActor；
- blocking overlap 返回失败。

检查插入在现有 validation / `TryBuildMotionConfig()` 之后、任何 staged state 写入之前。没有新增组件、Subsystem、Actor、wrapper 或持久状态。

## 4. 事务闭环

`LaunchPreparedCarrierImpl()` 的既有顺序为：

```text
Stage(ExecutionCandidate)
  -> if not staged: return LaunchRejected
  -> CommitStagedLaunch(ItemAuthority)
  -> AdoptPublishedFlight(...)
```

因此 clearance rejection 不可能越过阶段门进入 `CommitStagedLaunch()`。Projectile 自身也尚未写入 receipt/context/source，碰撞、移动、表现和 FlightCue 都保持关闭。

## 5. 测试实现

新增真实 GamePreview World 测试 `LaunchClearanceGate`：

- 复用既有 query-only、全通道 Block 的 `WorldStatic` 窄盒；
- 在阻挡盒中心以 `AlwaysSpawn` 创建 inert Projectile；
- 执行直线 `StagePreparedLaunch()` 并验证精确错误与完整 Empty/inert 状态；
- 将阻挡盒传送到远处并更新 World components；
- 复用原动作、原点与方向重试，验证成功 Staged、尚未 Committed；
- 取消 staged launch，验证回到 Empty。

既有同组测试继续覆盖 Arc、直线、碰撞轮廓、真实 sweep、命中到 Vitality 以及 durable publication。

## 6. 首轮与修复记录

生产实现和测试没有经历源码修复：首次 Editor 构建 `5 actions / native 0`，首次 `ThrownWeaponWorldDelivery` 专项 `7/0 / native 0`。

覆盖门首次失败属于工具调用环境：通过 `powershell.exe` 启动了 Windows PowerShell 5.1，而脚本需要 PowerShell 7 的前置管道语法。两个初始失败日志原样保留；切换到当前已安装的 PowerShell 7.6.5 后直接通过。没有为通过门禁而改脚本、映射或产品代码。

## 7. 改动文件映射

改动路径：

- `Source/demo_map/demo_mapShanmenThrownWeaponProjectile.cpp`；
- `Source/demo_map/demo_mapShanmenThrownWeaponWorldAdapterTests.cpp`。

二者命中 `ThrownWeaponWorldDelivery` 规则，去重后要求：WorldDelivery、ItemAdapter、CombatRunCoordinator、Items、WorldGameplay、CombatRuntime。六份日志均含一个精确 RunTests 命令、至少一个成功结果、零失败、原生 `TEST COMPLETE EXIT CODE 0` 且无 fatal/unhandled/ensure。

最终计数：`7 + 5 + 18 + 77 + 10 + 146 = 263 Success / 0 Fail`。

## 8. 覆盖门与构建

- coverage：`PASS Changed=2 Rules=1 Required=6 Logs=6`；
- self-test：`PASS 439/439`；
- Game Development：4 actions，native 0，26.57s；
- Editor Development：0 actions，native 0，1.06s。

| Evidence | Bytes | SHA-256 |
|---|---:|---|
| `Focused-ThrownWeaponWorldDelivery.log` | 269,776 | `18B0197351B9CFE953D81F61EC800CFE53A23A9A355352CAC33A5E53C03C97E2` |
| `Final-ThrownWeaponItemAdapter.log` | 266,439 | `BC920495FD3DFBE50B578F787CC486AACB0A14D2BAA94DA0A45AF8EB8B1C96D4` |
| `Final-CombatRunCoordinator.log` | 288,503 | `179D09DA6E7186B22634AD4C5682F148EE3B6A6B6A5F737D5E1E57392815C830` |
| `Final-Items.log` | 351,933 | `F2BC8367767A1BAF8ABBB1427E7C855F198D879E0465EF44ACA3502AF8A8F5A9` |
| `Final-WorldGameplay.log` | 269,912 | `68B63964C0E26D3C04298306B5FFF74DCAE257F18B901DC573388E32195F8EA3` |
| `Final-CombatRuntime.log` | 411,133 | `50A114180B790967A7A98C31E51378A2DF6C77E8975B9D0EAB97C62F3B0D3C6B` |
| `P22.9_regression_coverage.log` | 1,746 | `886F99A6D0B98EC23718E60677B5A830A557C6A684BB2215011EF7CFBC2415FE` |
| `P22.9_regression_coverage_selftest.log` | 43,303 | `7B0B3AA9A77B556B23D3A7AD8FFE70C8C05D3E1A3990B99963FEF1DF5EDB0C87` |

## 9. 静态边界

提交前非文档 diff 为 `2 files / +119/-0`：生产 `+32`，测试 `+87`。新增生产行中 Timer、SetTimer、自定义 Tick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0。

`git diff --check` 为 0；没有新增资产、Build.cs 依赖、Gameplay 状态权威或第二条碰撞/飞行链。

## 10. 提交边界与 GitHub

精确提交两个源码文件与本 Report/Log，共 4 个文件。103 个用户原有 untracked 文件不暂存；所有 raw evidence 留在 `Saved/Codex/P22.9` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-9-thrown-weapon-release-clearance>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-9-thrown-weapon-release-clearance/Docs/Report/Dev.D.UE.0.0.10.P22.9.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-9-thrown-weapon-release-clearance/Docs/Log/Dev.D.UE.0.0.10.P22.9.r0_log.md>
