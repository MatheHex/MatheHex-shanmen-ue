# Dev.D.UE.0.0.10.P22.9.r0 Report

## 1. 结论

P22.9 在 P 阶段边界内完成，结论为 **PASS**。

本轮补齐 P22.8 手侧释放点的物理安全门：飞刀在耐久物品提交前，使用自身现有盒体、现有碰撞响应和本次初速度姿态检查释放体积。释放点若位于阻挡几何体内，暂存失败关闭，Carrier 保持 Empty 且不可见、无碰撞、无运动；阻挡移开后，同一个发射输入可以重新暂存。

```text
Initial Editor Development build:       PASS (5 actions / native 0)
ThrownWeaponWorldDelivery:               7 Success / 0 Fail
Changed-file mapped regression:        263 Success / 0 Fail (6 healthy logs)
Changed-file regression coverage:      PASS (2 files / 1 rule / 6 groups)
Regression gate self-test:              PASS 439/439
Game + Editor Development:              PASS (4 + 0 actions / both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明真实 World 释放体积检查与既有事务顺序，不宣称所有关卡墙角、角色骨骼姿态或玩家视觉手感已经人工验收。

## 2. 玩家侧变化

P22.8 已让飞刀从角色右手侧代理点释放；P22.9 防止该点在贴墙、狭窄空间或阻挡体内时直接生成并穿出。

```text
手侧释放点清空
  -> 正常暂存
  -> 耐久物品提交
  -> 碰撞、表现和飞行一次性发布

手侧释放点被阻挡
  -> 暂存拒绝
  -> 不进入耐久提交
  -> Carrier 保持 Empty / inert
  -> 移开阻挡后可重试
```

失败不会留下半激活飞刀、不可见碰撞体或已开始运动的投射物。

## 3. 权威与顺序

既有 `RunHost` 顺序保持不变：先调用 World Adapter 暂存，再调用物品权威提交，最后发布飞行并由 Host 接管。新检查位于 `Projectile::TryStageLaunch()` 的所有输入/几何/运动配置验证之后、任何 Projectile 字段写入之前。

因此阻挡失败时：

- 原始动作与执行状态不被改写；
- 只构造于局部结果中的候选不会成为权威；
- 不返回可提交的 staged launch；
- `RunHost` 在物品提交调用之前返回 `LaunchRejected`；
- 同一 Empty Carrier 仍可安全重试或销毁。

## 4. 释放体积契约

检查使用投射物已经拥有的物理契约，而不是另建近似体积：

- 位置：`Launch.GetOrigin()`；
- 旋转：`Motion.InitialVelocity.Rotation().Quaternion()`；
- 形状：`Collision->GetScaledBoxExtent()` 生成的 Box；
- 查询通道：投射物碰撞组件现有 Object Type；
- 响应：投射物碰撞组件现有 Response Container；
- 忽略：Carrier 自身与 Source Actor，除此之外不扩大忽略列表。

该门同时服务直线和 Arc，因为两者在暂存前都已生成统一 `LaunchReceipt` 与实际初速度。

## 5. 失败关闭状态

阻挡命中时 `TryStageLaunch()` 直接返回 false，World Adapter 报告 `ProjectileStageRejected`。检查发生在下列写入之前：

- `LaunchReceipt` / `HitContext` / `SourceActor`；
- Owner / Instigator；
- Presentation 与 FlightCue；
- Movement 激活与速度发布；
- Projectile 状态从 Empty 到 Staged 的迁移。

无 World 或无碰撞组件的纯契约夹具继续走既有无场景测试路径；真实产品 World 与新增 GamePreview World 测试执行物理重叠检查。

## 6. 自动化证明

新增 `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.LaunchClearanceGate`：

1. 在真实 GamePreview World 中，把 query-only `WorldStatic` 阻挡盒放在发射原点；
2. 以 `AlwaysSpawn` 只生成碰撞关闭的 inert Carrier；
3. 证明暂存返回 `ProjectileStageRejected`，并且结果未 staged、未 committed；
4. 证明 Carrier 为 Empty、无 receipt/context、无碰撞、无运动、无表现、无 FlightCue；
5. 把同一阻挡盒移开后，以同一动作、同一原点、同一方向重试；
6. 证明暂存成功但仍未提交，随后可取消并回到 Empty。

同组既有 `DurableLaunchGate` 继续证明“暂存先于耐久提交、提交失败会取消 inert Carrier”。二者共同覆盖新几何拒绝与既有事务边界。

## 7. 改动文件回归

两条改动路径命中既有 `ThrownWeaponWorldDelivery` 映射，要求六个精确测试组；最终合计 `263 Success / 0 Fail`：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 7 | 269,776 | `18B0197351B9CFE953D81F61EC800CFE53A23A9A355352CAC33A5E53C03C97E2` |
| `Product.ThrownWeaponItemAdapter` | 5 | 266,439 | `BC920495FD3DFBE50B578F787CC486AACB0A14D2BAA94DA0A45AF8EB8B1C96D4` |
| `Product.CombatRunCoordinator` | 18 | 288,503 | `179D09DA6E7186B22634AD4C5682F148EE3B6A6B6A5F737D5E1E57392815C830` |
| `Shanmen.0_0_10.Items` | 77 | 351,933 | `F2BC8367767A1BAF8ABBB1427E7C855F198D879E0465EF44ACA3502AF8A8F5A9` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 269,912 | `68B63964C0E26D3C04298306B5FFF74DCAE257F18B901DC573388E32195F8EA3` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 411,133 | `50A114180B790967A7A98C31E51378A2DF6C77E8975B9D0EAB97C62F3B0D3C6B` |

## 8. 门禁、首轮证据与构建

产品实现和专项测试首次即通过。覆盖门第一次由 Windows PowerShell 5.1 调用，因脚本使用 PowerShell 7 管道语法而在解析阶段退出；该错误与产品源码、测试结果无关，原日志保留。改用项目既有 PowerShell 7.6.5 后，覆盖门和 439 项自测全部通过，未修改门禁或产品代码。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.9_editor_build_initial.log` | 5 actions / PASS / native 0 / 11.45s | 2,521 | `74BE48E9C200B10423FA16E9271F15460BBCB06636F95E69C8E54D8765FB6AAA` |
| `P22.9_regression_coverage_initial.log` | PowerShell 5.1 parse failure | 1,595 | `879D1CDD804BC633A346AFA23C7DC94C9A118E3471E9631E74D5D9EFF62CE3AE` |
| `P22.9_regression_coverage_selftest_initial.log` | inherited 5.1 parse failure | 3,499 | `CD715414D92DF2CDE105820340DEC4592560E8AE686CFEF62ACBD8BFA521C1EA` |
| `P22.9_regression_coverage.log` | PASS 2/1/6/6 | 1,746 | `886F99A6D0B98EC23718E60677B5A830A557C6A684BB2215011EF7CFBC2415FE` |
| `P22.9_regression_coverage_selftest.log` | PASS 439/439 | 43,303 | `7B0B3AA9A77B556B23D3A7AD8FFE70C8C05D3E1A3990B99963FEF1DF5EDB0C87` |
| `P22.9_game_build_final.log` | 4 actions / PASS / native 0 / 26.57s | 2,301 | `A338041FEDD79E8816B442C58C0E98C5BA777931FC6BB859EC2A37F4F2299103` |
| `P22.9_editor_build_final.log` | 0 actions / PASS / native 0 / 1.06s | 1,035 | `12E94EF5C4D094F6F91F5A8702BEA61216FD31B9E3A5B75BFD744B96A155E5C8` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,579,648 bytes；SHA-256 `BC71AB758416555CEC474B6417E1725F741916BFF9D19ED26F95A9BBCA217936`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,788,864 bytes；SHA-256 `D713D6099B68F1033C42AAE11EE41A9AE24E6F26B93AFF7A2FB29E17BC8BCBFA`。

## 9. P/F 与静态边界

PASS：阻挡体内释放被拒绝；拒绝发生在耐久提交前；Carrier 保持 Empty/inert；阻挡移开后可重试；直线/Arc 共享检查；六个映射组零失败；覆盖门、自测和双构建通过。

未声明：骨骼 Socket、动画穿插、网络复制、复杂动态角色群、不同缩放体型或所有关卡狭缝已经视觉验收；没有执行交互式产品验收。

非文档 diff 为 `2 files / 119 insertions / 0 deletions`，其中生产实现 `1 file / +32`、测试 `1 file / +87`。新增生产行中的 Timer、SetTimer、自定义 Tick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0；没有新增资产、模块依赖或第二条状态链；`git diff --check` 为 0。

## 10. 提交边界与 GitHub

本阶段只提交 1 个生产文件、1 个测试文件、本 Report 与本 Development Log，共 4 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.9` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-9-thrown-weapon-release-clearance>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-9-thrown-weapon-release-clearance/Docs/Report/Dev.D.UE.0.0.10.P22.9.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-9-thrown-weapon-release-clearance/Docs/Log/Dev.D.UE.0.0.10.P22.9.r0_log.md>
