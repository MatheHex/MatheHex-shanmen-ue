# Dev.D.UE.0.0.10.P22.1.r0 Development Log

## 1. 基线与目标

- base：`58ec67e6587725f766d8ce1a5930dac10c14e1b6`（P22.0 visible thrown-weapon carrier）；
- branch：`agent/0.0.10-p22-1-thrown-weapon-collision-profile`；
- 目标：让标准投掷物的真实碰撞轮廓与 P22.0 可见飞刀一致，并证明真实移动阻挡会进入既有接触链；
- 边界：不新建运动、目标、资源、Combat Run、Impact、生命值或计时权威，不启动 UI/PIE/产品 executable。

## 2. 审计与决策

P22.0 的视觉为 `30 × 4.5 × 1.2 cm` 细长 Cube，但根命中体仍是半径 `12 cm` 球体。球体侧向宽出可见刀身约 5 倍，前向又比刀尖短 `3 cm`。这会造成“看起来没碰到却命中”和“刀尖已经穿入但尚未命中”两种相反误差。

没有新增碰撞代理 Actor。最小修复直接修改 canonical `Ademo_mapShanmenThrownWeaponProjectile`：根组件改为 `UBoxComponent`，视觉与碰撞共同读取一个尺寸常量，运动仍由既有 `UProjectileMovementComponent` 提供。

## 3. 实现

- 新增 `ThrownWeaponPrototypeFullSize(30.0, 4.5, 1.2)`；
- 根组件由 `USphereComponent` 切换为 `UBoxComponent`，半尺寸由完整尺寸除以 2；
- Engine Cube 的相对缩放由同一完整尺寸除以 `100 cm` 得到；
- 保留 `QueryOnly`、WorldStatic/WorldDynamic/Pawn Block 与来源 Actor Ignore；
- 删除不适用于 Query-only 投射物终止证明的刚体 Hit 通知；
- 在 `PostInitializeComponents()` 把标准 `OnProjectileStop` 绑定到当前 Actor 实例；
- 停止回调经过 InFlight、有效 Actor、自身与来源 Actor 过滤后，广播既有原生 `OnContact`；
- Run Host 只把精确组件 include 从 Sphere 改为 Box，业务链未变化。

## 4. 测试夹具与断言

新增 `FThrownCollisionWorldFixture`：建立 `GamePreview` World、物理场景、Trace Collision、来源 Pawn 和可移动窄盒障碍，并调用 `InitializeActorsForPlay()`，使 Actor 动态回调按真实运行时规则执行。

`CollisionProfileSweep` 依次证明：

1. 根盒半尺寸与视觉方向正确；
2. `X=8 cm` 的窄障碍不在可见刀身宽度内，200 cm Sweep 无阻通过；
3. 障碍对齐 `X=0` 后，同一 Sweep 返回真实阻挡 Actor/Component；
4. 标准运动组件以 `750 cm/s` 推进 `0.2 s` 时在障碍前停止；
5. 停止回调归当前飞刀实例所有；
6. 原生接触 seam 恰好收到一次同一障碍。

## 5. 失败记录与修正

首次 Editor 构建日志 `P22.1_editor_build_initial.log` 同时记录：

- 默认并行下 `C3859`/`C1076` PCH 虚拟内存与内部堆上限；
- Run Host 的旧 Sphere include 造成 `UBoxComponent` 不完整类型 `C2027`。

随后固定 `-MaxParallelActions=4` 并修正 include。首个收紧后的碰撞测试证明直接 `MoveComponent` 的阻挡结果不等于 Actor 接触事件；进一步改为 `OnProjectileStop` 后，诊断测试又暴露未初始化临时 World 会让 `AActor::ProcessEvent` 跳过动态回调。最终修复是让测试 World 进入 Actor 初始化阶段，而不是删除接触断言。

一次诊断编译还误用了不存在的 `GetUpdatedComponent()`；改为 UE 5.8 公共的 `UpdatedComponent` 后继续验证。上述失败均保留在 `Saved/Codex/P22.1`，没有覆盖成成功日志。

## 6. 专项与映射回归

最终专项：`P22.1_focused_thrown_world_final.log`，`6 Success / 0 Fail`，263,213 bytes，SHA-256 `33454346DD2A04EAE1719DB99A2A6FFDB8713F2753EBAFB2F8E9E133A2213294`。

按 4 个实际改动路径推导并独立运行 7 个必跑组：

| Group | Success | SHA-256 |
|---|---:|---|
| CombatRuntime | 146 | `187529D853B51FD3209CDF13B6E6B2AA6581F7A67154449D49DAC6AC0A229355` |
| Items | 77 | `672256647E2E2279B9A7FBBA9EF3509EE73A979D98C04A9B32B3851CB161B8C3` |
| Product.CombatRunCoordinator | 18 | `60A19F1267DBB43424E83555FFD1ED38A44AE2168725CDD4C6415CCC2CB2A052` |
| Product.ThrownWeaponItemAdapter | 5 | `0110A64FD43D1F695519A9D4F9C667827F91E1521EF76759F07E7B89AB37CA64` |
| Product.ThrownWeaponRunHost | 4 | `B4E4997B9DD66A5E0BD1AFDF615FB1C32CD721D5C580C7A03FB36EF5C74F14B3` |
| Product.ThrownWeaponWorldDelivery | 6 | `70B3819A82BB86869A2F4C5415835DA6574FF0AC76928D4103A2213CCF79D2A5` |
| WorldGameplay | 10 | `AE610FE31B496CC54D7106573488BA1D3B37E2C6B1CB04C5BF52FC515B7E2DBA` |

合计 `266 Success / 0 Fail`，每份日志都有唯一 RunTests 组、自然完成标记、0 Fatal/Ensure。广域诊断在 `720/0` 时主动停止，不作为健康终态证据。

## 7. 自动覆盖门

`Test-ShanmenRegressionCoverage.ps1` 输出：

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=7 Logs=7
```

门禁日志 2,137 bytes，SHA-256 `8088093B18D72039D328829A967A3BDFB76B30283C653B4265CB9323547167A8`。门禁自测 `PASS 437/437`，43,073 bytes，SHA-256 `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0`。

## 8. 构建与静态检查

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.1_game_build_final.log` | 117 actions / PASS / native 0 | 11,435 | `BF9954AC686F380794C06BE920AE368B6DB3FBFC90EFB34050F69FF472D0FC7F` |
| `P22.1_editor_build_final.log` | 0 actions / PASS / native 0 | 965 | `96B5D7AC66040D299E65ED23443EB6B92477514E4F09206E79AA257F0EC9032D` |

`git diff --check` 为 0。实现/测试 diff 为 4 files / 270 insertions / 24 deletions；26 条新增生产行无 Timer、SetTimer、RNG、ApplyDamage、SpawnActor 或 Destroy。最终 `demo_map.exe` 与 `UnrealEditor-demo_map.dll` 均重新核验哈希，相关运行进程数为 0。

## 9. 提交边界

精确提交以下 6 个文件：

- `Source/demo_map/demo_mapShanmenThrownWeaponProjectile.h`；
- `Source/demo_map/demo_mapShanmenThrownWeaponProjectile.cpp`；
- `Source/demo_map/demo_mapShanmenThrownWeaponRunHost.cpp`；
- `Source/demo_map/demo_mapShanmenThrownWeaponWorldAdapterTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P22.1.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P22.1.r0_log.md`。

103 个用户原有 untracked 文件不暂存；raw evidence 保留于 `Saved/Codex/P22.1` 且不进入 Git。未修改 Content、地图、Engine、Windows、存档 schema、物品权威、Impact resolver 或目标生命值。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-1-thrown-weapon-collision-profile>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-1-thrown-weapon-collision-profile/Docs/Report/Dev.D.UE.0.0.10.P22.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-1-thrown-weapon-collision-profile/Docs/Log/Dev.D.UE.0.0.10.P22.1.r0_log.md>
