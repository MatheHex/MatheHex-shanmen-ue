# Dev.D.UE.0.0.10.P22.10.r0 Development Log

## 1. 基线与目标

- base：`ea972477718e37968f7d6012f4da6a130e0b9fd5`（P22.9 release-volume clearance）；
- branch：`agent/0.0.10-p22-10-thrown-weapon-release-corridor`；
- 目标：防止飞刀的手侧释放点隔着薄阻挡体出现在墙另一侧；
- 边界：不改释放点公式、轨迹、速度、伤害、库存、Run、终态或表现，不启动 UI/PIE/产品 executable。

## 2. 审计结论

P22.9 的 endpoint overlap 能拒绝“Origin 在墙内”，但如果薄墙位于 Source 与 Origin 之间、Origin 本身已经在墙外，Overlap 为 false。Carrier 又以 `AlwaysSpawn + NoCollision` 预备，因此旧流程可绕过薄墙完成暂存。

解决位置应继续留在 `TryStageLaunch()`，因为它拥有实际 Launch/Motion/Collision，又位于 durable item commit 之前。

## 3. 生产改动

`IsLaunchVolumeClear()` 更名并扩展为 `IsLaunchCorridorClear()`：

- 缓存 Launch Origin、初速度旋转与 Box CollisionShape；
- 保留 endpoint blocking overlap；
- endpoint 清空后，从 Source Actor Location 到 Launch Origin 执行 blocking sweep；
- 两次查询共享 Object Type、Response Container 和 ignored actors；
- 零长度 corridor 直接由 endpoint 结果决定。

`TryStageLaunch()` 只把原检查调用替换为新检查；其余生命周期不变。

## 4. 测试改动

新增 `LaunchCorridorGate`：在真实 GamePreview World 中令 Source→Origin 为 200 cm，并把薄阻挡盒放在 100 cm 中点。测试证明远离阻挡体的 endpoint 仍因 corridor 被拒绝，Carrier 完整 inert；移开 blocker 后同一请求成功 staged 并可取消。

既有 `CollisionProfileSweep` 的 blocker 改为在 launch publication 后才进入测试路径，使其继续只验证飞行期侧偏与对齐扫掠。

## 5. 权威顺序

```text
validate launch + build motion
  -> endpoint overlap
  -> Source-to-Origin box sweep
  -> write staged Projectile state
  -> durable item commit
  -> publish movement/collision/presentation
```

任一 overlap/sweep 阻挡都停在第一条状态写入之前，不产生半提交。

## 6. 首轮失败闭环

首次 Editor 构建为 `5 actions / native 0`。首次 WorldDelivery 为 `7/1`：新增 Corridor 测试通过，旧 `CollisionProfileSweep` 在 publish 前被新门拒绝。

根因是旧夹具从 World 初始化开始就放置后续飞行障碍；新门让这个隐藏的阶段耦合可见。修复仅移动夹具设置时机，保持生产门和既有碰撞几何不变。修复构建 `4 actions / native 0`，最终专项 `8/0`。

| Evidence | Bytes | SHA-256 |
|---|---:|---|
| `Focused-ThrownWeaponWorldDelivery_initial.log` | 271,980 | `09BEC6E69C1F23C99245292AC2271A06E7CC81612910D478772B2BF332862D64` |
| `P22.10_editor_build_repair.log` | 2,368 | `40A00B9A545AEABD7F7226C6974C67019521BB9B731B3E081F2F382D86281441` |
| `Final-ThrownWeaponWorldDelivery.log` | 271,122 | `986AB44B1653B5AC7D449F04CE8199D364865AC69E8CC38B7BFF6E275EE02BA3` |

## 7. 映射回归

两个改动文件命中既有 `ThrownWeaponWorldDelivery` 映射。六组健康日志覆盖 WorldDelivery、ItemAdapter、CombatRunCoordinator、Items、WorldGameplay 与 CombatRuntime，合计 `264/0`。

覆盖门：`PASS Changed=2 Rules=1 Required=6 Logs=6`；门禁自测：`PASS 439/439`。

## 8. 构建与产物

- initial Editor：5 actions / native 0 / 15.07s；
- repair Editor：4 actions / native 0 / 7.73s；
- final Game：4 actions / native 0 / 25.45s；
- final Editor：0 actions / native 0 / 0.95s。

最终 `demo_map.exe` SHA-256 为 `F728953E8D805FB1A10A731EE0D9B35529DB86A12B5A11D8D7B6EA4A9CF206E5`；`UnrealEditor-demo_map.dll` SHA-256 为 `B2CF1E0102E1B397EA50B315339F3D47BF58FFB8151DF574AAF0D03CAAE75A7D`。

## 9. 静态边界

非文档 diff：生产 `+26/-7`，测试 `+106/-0`。新增生产行中的 Timer、SetTimer、自定义 Tick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0。

`git diff --check` 为 0；无新增资产、模块依赖、持久状态、第二套碰撞或第二套飞行权威。

## 10. 提交边界与 GitHub

精确提交两个源码文件与本 Report/Log，共 4 个文件。103 个用户原有 untracked 文件不暂存；raw evidence 保留在 `Saved/Codex/P22.10` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-10-thrown-weapon-release-corridor>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-10-thrown-weapon-release-corridor/Docs/Report/Dev.D.UE.0.0.10.P22.10.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-10-thrown-weapon-release-corridor/Docs/Log/Dev.D.UE.0.0.10.P22.10.r0_log.md>
