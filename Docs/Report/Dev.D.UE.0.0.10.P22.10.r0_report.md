# Dev.D.UE.0.0.10.P22.10.r0 Report

## 1. 结论

P22.10 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P22.9 的“释放终点不可被占用”扩展为完整短距释放走廊：飞刀在耐久物品提交前，以自身盒体和实际初速度姿态，从 Source Actor 位置扫掠到手侧释放点。即使终点已经在薄墙另一侧且本身没有重叠，只要中间路径穿过阻挡体，暂存仍会失败关闭；移开阻挡后，同一请求可重试。

```text
Initial Editor Development build:       PASS (5 actions / native 0)
First WorldDelivery run:                  7 Success / 1 Fail (legacy fixture order)
Final WorldDelivery run:                  8 Success / 0 Fail
Changed-file mapped regression:         264 Success / 0 Fail (6 healthy logs)
Changed-file regression coverage:       PASS (2 files / 1 rule / 6 groups)
Regression gate self-test:               PASS 439/439
Game + Editor Development:               PASS (4 + 0 actions / both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明真实 World 中的薄墙释放拦截、失败关闭与既有事务顺序，不宣称所有角色姿态、关卡夹缝或玩家视觉手感已经人工验收。

## 2. 玩家侧变化

旧边界只处理“手侧点在墙里”；新边界也处理“身体在墙内侧、手侧点已经穿到墙外”的情形。

```text
Source Actor
  -> [以飞刀盒体扫过短距释放走廊]
  -> Hand-side Launch Origin

走廊清空：允许暂存，再提交消耗并发布飞行
走廊受阻：拒绝暂存，不扣物品，不显示、不运动
```

这避免飞刀从薄墙另一侧凭空出现，同时不自动改写玩家瞄准方向或寻找隐式替代发射点。

## 3. 单一物理契约

P22.9 的终点 Overlap 与 P22.10 的走廊 Sweep 共享同一组数据：

- `Launch.GetOrigin()`；
- `Motion.InitialVelocity.Rotation().Quaternion()`；
- `Collision->GetScaledBoxExtent()`；
- Collision Object Type；
- Collision Response Container；
- 仅忽略 Projectile 自身与 Source Actor。

因此终点检查和路径检查不会使用不同半径、不同朝向或不同通道。直线与 Arc 也继续经由同一个 `TryStageLaunch()` 门。

## 4. 实现

内部检查由 `IsLaunchVolumeClear()` 收敛为 `IsLaunchCorridorClear()`：

1. 先在 Launch Origin 执行 blocking overlap；
2. 终点被占用时立即失败；
3. 终点清空后，从 `SourceActor->GetActorLocation()` 到 Launch Origin 执行 blocking sweep；
4. 起点与终点近似相同时跳过零长度 sweep；
5. 任一 blocking 结果使 `TryStageLaunch()` 在任何状态写入之前返回 false。

没有新增第二个碰撞组件、轨迹算法、状态机、Actor、Subsystem 或异步任务。

## 5. 事务与失败关闭

检查仍位于 `TryBuildMotionConfig()` 之后、Projectile 写入之前。失败时：

- `LaunchReceipt`、`HitContext` 与 Source 绑定均为空；
- Collision 为 `NoCollision`；
- Movement、Presentation、Roll 与 FlightCue 均未激活；
- Projectile 状态保持 `Empty`；
- World Adapter 返回 `ProjectileStageRejected`；
- Run Host 在 `CommitStagedLaunch(ItemAuthority)` 之前返回，因此没有耐久消耗。

## 6. 自动化证明

新增 `LaunchCorridorGate` 使用真实 GamePreview World：

1. Source 与 Launch Origin 相距 `200 cm`；
2. 一个 query-only、全通道 Block 的薄盒位于二者中点；
3. 薄盒距终点 `100 cm`，明确不是终点重叠；
4. 首次暂存返回 `ProjectileStageRejected`，Carrier 保持 Empty/inert；
5. 把同一个阻挡盒移到远处；
6. 复用同一动作、同一 Origin、同一方向后暂存成功且仍未 committed；
7. 取消后安全回到 Empty。

既有 `LaunchClearanceGate` 继续覆盖终点占用，二者把“终点”和“到达终点的短路径”分开证明。

## 7. 首轮失败与修复

首次专项中新增 `LaunchCorridorGate` 已通过；唯一失败是既有 `CollisionProfileSweep`。该旧测试在暂存前就把“稍后用于飞行碰撞测试”的阻挡体放入 World，其夹具变换恰好进入新释放走廊，于是生产代码正确拒绝发布。

修复只调整测试阶段顺序：暂存/发布前先把该阻挡体移远；发布成功后再把它放回原 `(8,100,100)` 位置，继续验证侧偏扫掠不命中与对齐扫掠命中。没有削弱生产门，也没有改变旧碰撞断言。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.10_editor_build_initial.log` | 5 actions / PASS / native 0 / 15.07s | 2,442 | `1D9DA64F8819582D9EB2E4345914077A29FF858B77EF04CA1F74E529DE62AA9C` |
| `Focused-ThrownWeaponWorldDelivery_initial.log` | 7/1 / native 255 | 271,980 | `09BEC6E69C1F23C99245292AC2271A06E7CC81612910D478772B2BF332862D64` |
| `P22.10_editor_build_repair.log` | 4 actions / PASS / native 0 / 7.73s | 2,368 | `40A00B9A545AEABD7F7226C6974C67019521BB9B731B3E081F2F382D86281441` |

## 8. 改动文件回归与构建

最终六份精确日志合计 `264 Success / 0 Fail`：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 8 | 271,122 | `986AB44B1653B5AC7D449F04CE8199D364865AC69E8CC38B7BFF6E275EE02BA3` |
| `Product.ThrownWeaponItemAdapter` | 5 | 266,440 | `E76E9AEA4FCF95DD183607ED603911D8D3EA03D80D3BAC019B0BE991380087A0` |
| `Product.CombatRunCoordinator` | 18 | 288,103 | `5A0B7B64F63ED8C4F0053A30ED1F05FD0577F88DEC941ADABDF673786A1E2652` |
| `Shanmen.0_0_10.Items` | 77 | 351,077 | `F8BC7373809D7F7BF9BDFF2CB92669BDBA4B3B72E8DF77A5F3E4FD3320824AE8` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 270,814 | `A9DDA6E9753ED970A0A1AC2D3DD251A6D976E13297D1E3D080A4FECD68A9649C` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 411,135 | `601A5526CB65EB0A757D0635641E55E84E11AFD02EF677AC896C28C68BA99082` |

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.10_regression_coverage.log` | PASS 2/1/6/6 | 1,742 | `03C2FAF48C6BB2DFB1E67C3D34B5A68CF3C060F184DCC9E6F775784FFB429BD6` |
| `P22.10_regression_coverage_selftest.log` | PASS 439/439 | 43,303 | `7B0B3AA9A77B556B23D3A7AD8FFE70C8C05D3E1A3990B99963FEF1DF5EDB0C87` |
| `P22.10_game_build_final.log` | 4 actions / PASS / native 0 / 25.45s | 2,227 | `D0A5E00B71D75194FA3545EED8F8CC5C78458196B8C6AE2EC3BB3949F0A744F5` |
| `P22.10_editor_build_final.log` | 0 actions / PASS / native 0 / 0.95s | 1,035 | `34E8DF7D6D1A1D7693755CBE6F19EF03B75415230A6038C68D00125369A6D9E5` |

产物：`demo_map.exe` 为 359,584,256 bytes，SHA-256 `F728953E8D805FB1A10A731EE0D9B35529DB86A12B5A11D8D7B6EA4A9CF206E5`；`UnrealEditor-demo_map.dll` 为 18,793,984 bytes，SHA-256 `B2CF1E0102E1B397EA50B315339F3D47BF58FFB8151DF574AAF0D03CAAE75A7D`。

## 9. P/F 与静态边界

PASS：终点占用继续被拒绝；终点清空但短距路径穿过薄墙也被拒绝；两类失败均发生在耐久提交前；移开阻挡后可重试；六组回归零失败；覆盖门、自测与双构建通过。

未声明：自动寻找替代发射点、墙边提示文本、骨骼 Socket、网络复制、动态群体阻挡或所有关卡夹缝已经验收；没有执行交互式产品验收。

非文档 diff 为 `2 files / 132 insertions / 7 deletions`，其中生产实现 `1 file / +26/-7`、测试 `1 file / +106/-0`。新增生产行中的 Timer、SetTimer、自定义 Tick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0；没有新增资产、模块依赖或状态权威；`git diff --check` 为 0。

## 10. 提交边界与 GitHub

本阶段只提交 1 个生产文件、1 个测试文件、本 Report 与本 Development Log，共 4 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.10` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-10-thrown-weapon-release-corridor>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-10-thrown-weapon-release-corridor/Docs/Report/Dev.D.UE.0.0.10.P22.10.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-10-thrown-weapon-release-corridor/Docs/Log/Dev.D.UE.0.0.10.P22.10.r0_log.md>
