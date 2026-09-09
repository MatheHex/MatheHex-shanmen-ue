# Dev.D.UE.0.0.10.P22.3.r0 Report

## 1. 结论

P22.3 在 P 阶段边界内完成，结论为 **PASS**。

本轮为标准投掷物的同一个物理 Carrier 增加附着式飞行光提示。飞刀只有在耐久数量提交完成、正式进入 `InFlight` 后才发光；直线轨迹为暖金色，抛物线轨迹为青蓝色。暂存、提交失败回滚、有效命中和无命中终止都会保持或恢复熄灭，不建立第二套飞行状态。

```text
Thrown-weapon World delivery focus:      6 Success / 0 Fail
Changed-file mapped regression:        262 Success / 0 Fail (6 healthy logs)
Changed-file regression coverage:      PASS (3 files / 1 rule / 6 required groups)
Regression gate self-test:              PASS 439/439
Game + Editor Development:              PASS (Game 118 actions; Editor 0 actions; both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明组件状态、轨迹色码和终态熄灭契约，不宣称最终亮度、色彩、材质、粒子、地图对比度或玩家体验已经人工验收。

## 2. 玩家可见结果

| 飞刀阶段 | 光提示 | 颜色 / 结果 |
|---|---|---|
| Empty | 不可见 | 无飞行提示 |
| Staged | 不可见 | 不提前泄露尚未耐久提交的投掷 |
| InFlight / Straight | 可见 | 暖金 `Linear(1.00, 0.48, 0.08)` |
| InFlight / BallisticArc | 可见 | 青蓝 `Linear(0.20, 0.72, 1.00)` |
| Spent | 不可见 | 命中、阻挡、超程或飞行时间耗尽后熄灭 |

提示使用 `1600` 强度、`140 cm` 衰减半径并关闭阴影。它附着于现有投掷物碰撞根，不生成额外 Actor，也不改变飞刀尺寸、朝向、速度、重力、碰撞或伤害。

## 3. 单一事实链

```text
existing immutable launch receipt
  -> existing durable Quantity publication
  -> existing projectile State becomes InFlight
  -> attached PointLight visibility + receipt trajectory color
  -> existing contact / no-impact terminal
  -> existing projectile State becomes Spent
  -> attached PointLight hidden
```

颜色只读取既有 `FShanmenThrownWeaponLaunchReceipt::GetTrajectoryKind()`。显隐只读取既有 Projectile State。光提示不选择轨迹、不推进动作、不修改库存、不解析接触，也不拥有任何计时边界。

## 4. 实现

`Ademo_mapShanmenThrownWeaponProjectile` 新增一个私有 `UPointLightComponent`：

- 构造时附着到既有 `UBoxComponent` 根，默认熄灭；
- `TryStageLaunch` 冻结有效回执后预置对应颜色，但仍保持不可见；
- `ActivateCommittedLaunch` 完成原有碰撞、移动和网格发布后，将 State 置为 `InFlight` 并点亮；
- `CancelStagedLaunch` 清理回执并返回 `Empty` 后保持熄灭；
- `MarkSpent` 停止移动并进入 `Spent` 后立即熄灭；
- `IsFlightCueVisible()` 与 `GetFlightCueColor()` 提供只读测试面，不暴露组件写权限。

`IsStagedFor` 同时验证光源存在、附着于碰撞根且不可见；`IsInFlightFor` 要求提示可见。原有 collisionless Cube 视觉组件仍是飞刀本体，点光不承担碰撞或生命周期权威。

## 5. 真实 World 证明

`Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery` 的 6 个测试全部在 UE 5.8 原生命令行自动化中自然结束：

1. `DurableLaunchGate` 证明暂存无光、耐久权威拒绝后仍无光、精确提交后直线飞行显示暖金光；
2. `ArcReceiptMotion` 证明抛物线暂存无光、提交后显示青蓝光、无命中终止后熄灭；
3. `ContactToVitality` 证明真实生命值提交并将物理飞刀置为 `Spent` 后，本体和光提示同时隐藏；
4. `FailClosedAndMiss` 证明无法解析的接触不会错误消耗飞行，光提示继续可见；随后明确无命中终止时熄灭；
5. 原有 collision profile 与 fail-closed 测试继续通过，说明新增光源没有扩大碰撞或 World 权威。

这组测试直接读取引擎组件保存的实际 `LightColor`，不是只比较一个独立呈现值。

## 6. 首次失败与根因修正

首轮专项为 `4 Success / 2 Fail`，进程按测试失败原生返回 `255`；失败只发生在直线和抛物线的颜色断言，生命周期、显隐和另外四个测试均通过。首失败日志保留为 `P22.3_focused_world_delivery_initial.log`，264,956 bytes，SHA-256 `4561DDE4B2E9814EA2CD48A9635F7D68066AEC01200C82E2CF332BED0C5ED3A4`。

根因是初版调用 `SetLightColor(Color, false)`：线性颜色先直接量化为 8-bit `FColor`，而 `GetLightColor()` 再按 sRGB 转回线性值，导致实际组件颜色与预期线性值不一致。修正为使用引擎默认 sRGB 编码，并以 `0.01` 容差覆盖 8-bit 往返量化；显隐、State、耐久发布和终态检查没有放宽。重构建后专项自然完成 `6/0`。

## 7. 改动文件回归

3 个改动路径命中既有 `ThrownWeaponWorldDelivery` 映射规则，并推导 6 个精确测试组：

| Group | Success | SHA-256 |
|---|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 6 | `9A096A415EA87E94BBB5EADEA91F4922150E28B6A579ABFDC6AF0BC928C66B11` |
| `Product.ThrownWeaponItemAdapter` | 5 | `81B872A6AA3366D5FD106F0C7BFEFC28B9E93ED21FBE981EB01059587F2D011D` |
| `Product.CombatRunCoordinator` | 18 | `9B914720DA1BC9807D3E1BE8C023F2E8F194EBB2E8EC60B4484F488CB5CB96C9` |
| `Shanmen.0_0_10.Items` | 77 | `053021F391B369C4EAADABBD5E3BBFE628CCE70E7705EC902CFDF404EF08CA0D` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | `4E34DD787018892B6F5F615321C170179C373B503CC9D4055F705ECC05840AD7` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | `2C1EF9C72A3935D2EED3D6DAB2D910E9B905CD980FE83EE8564AADB2A0E7B155` |

合计 `262 Success / 0 Fail`。每份日志都只有一个精确 `Automation RunTests` 命令、零 Fail、零 Fatal/Unhandled/Ensure，并含 UE 5.8 原生成功终止标记。

覆盖门结果为 `PASS Changed=3 Rules=1 Required=6 Logs=6`；日志 1,890 bytes，SHA-256 `D62978B6677C116DFA95F3ED39493E5D8E96DF8E23F7A0882613C9B3C9859138`。门禁自测为 `439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

## 8. 构建、产物与静态边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development final | Succeeded / native 0 | 118 / 208.17s | `76A09A96F2BEF9B4B6B4537DC6C03754F06F1EC5EE9EBA681821D9FA37B95098` |
| Editor Development final | Succeeded / native 0 | 0 / 1.18s | `24E98CC34A18F8251E23F1E329B2E2AED2D98CA7DD1D3250D77211A5A7DD0C99` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,557,120 bytes；SHA-256 `F2E04A117C9494BE91A4A4FB1982C806E7EE4BC192202E066A7BB7CFB59B5D78`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,754,560 bytes；SHA-256 `96333DD9C62A7EF32A833945C19729CAA8059EB5F1929914DC97D64F3B57572B`。

实现与测试差异为 3 files / 84 insertions / 5 deletions，其中生产新增 64 行。新增生产行中的 Timer、SetTimer、Tick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor 和 Destroy 均为 0；`git diff --check` 原生退出码为 0。

## 9. P/F 边界

PASS：提示使用现有物理 Carrier；只在耐久发布后的 `InFlight` 显示；直线和抛物线颜色可区分；暂存、回滚、有效接触和无命中终止均有显隐证明；飞行、碰撞、库存、动作与伤害权威保持不变；映射回归、覆盖门、自测及双目标构建全部通过。

未声明：点光参数、最终美术、阴影策略、粒子/拖尾、声音、网络复制、真实地图可读性或性能已经人工验收。本轮没有启动任何交互式产品运行。

## 10. 提交边界与 GitHub

本阶段只提交 3 个实现/测试文件、本 Report 与本 Development Log，共 5 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.3` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-3-thrown-weapon-flight-cue>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-3-thrown-weapon-flight-cue/Docs/Report/Dev.D.UE.0.0.10.P22.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-3-thrown-weapon-flight-cue/Docs/Log/Dev.D.UE.0.0.10.P22.3.r0_log.md>
