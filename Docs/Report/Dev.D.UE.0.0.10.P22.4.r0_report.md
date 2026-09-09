# Dev.D.UE.0.0.10.P22.4.r0 Report

## 1. 结论

P22.4 在 P 阶段边界内完成，结论为 **PASS**。

本轮让 P22.1 的同一个物理飞刀网格在真实 `InFlight` 阶段沿自身飞行轴持续滚转。滚转只作用于无碰撞表现子组件；Projectile Actor、根碰撞、速度、重力、命中和库存权威均保持不变。暂存、耐久提交失败、有效命中及无命中终止均不会残留滚转状态。

```text
Thrown-weapon World delivery focus:      6 Success / 0 Fail
Changed-file mapped regression:        262 Success / 0 Fail (6 healthy logs)
Changed-file regression coverage:      PASS (3 files / 1 rule / 6 required groups)
Regression gate self-test:              PASS 439/439
Game + Editor Development:              PASS (Game 118 actions; Editor 0 actions; both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明引擎组件状态、转轴隔离和生命周期契约，不宣称最终刀模、转速、运动模糊、光效组合或玩家体验已经人工验收。

## 2. 玩家可见结果

| 飞刀阶段 | 刀身滚转 | 结果 |
|---|---|---|
| Empty | 停止 | 本体隐藏，无残留姿态 |
| Staged | 停止 | 不提前表现尚未耐久提交的投掷 |
| InFlight | `720°/s` | 每秒两圈，沿刀身局部前向轴滚转 |
| Spent | 停止并归零 | 命中、阻挡、超程或飞行时间耗尽后收束 |

滚转复用既有 `30 × 4.5 × 1.2 cm` collisionless Cube 原型网格，并与 P22.3 的轨迹色光提示共存。它不旋转 Actor 或 `UBoxComponent`，因此不会改变弹道前向、Sweep 形状或接触结果。

## 3. 单一事实链

```text
existing immutable launch receipt
  -> existing durable Quantity publication
  -> existing projectile State becomes InFlight
  -> built-in RotatingMovement activates on existing Visual only
  -> existing contact / no-impact terminal
  -> existing projectile State becomes Spent
  -> roll deactivates and Visual relative rotation resets
```

显隐、滚转和 P22.3 光提示都服从同一份 Projectile State。滚转不选择轨迹、不推进动作、不修改库存、不解析接触，也不拥有第二份飞行生命周期。

## 4. 实现

`Ademo_mapShanmenThrownWeaponProjectile` 新增一个私有 `URotatingMovementComponent`：

- `UpdatedComponent` 精确绑定既有 `Visual`，不绑定 Actor 根或碰撞；
- `RotationRate.Roll = 720°/s`，`bRotationInLocalSpace = true`；
- 默认不自动激活，不新增 Actor、自定义 Tick、Timer 或 Timeline；
- `ActivateCommittedLaunch` 只在 State 已进入 `InFlight` 后激活；
- `TryStageLaunch`、`CancelStagedLaunch` 和 `MarkSpent` 均停用并归零表现相对旋转；
- `IsStagedFor` 要求滚转关闭且姿态归零，`IsInFlightFor` 要求滚转已激活；
- 只读接口暴露实际表现 Up 方向和滚转激活状态，不暴露组件写权限。

`UProjectileMovementComponent` 继续唯一更新碰撞根和 Actor 飞行朝向。新增组件只使用引擎内置旋转行为更新已存在的无碰撞网格。

## 5. 真实 World 证明

`Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery` 的 6 个测试全部在 UE 5.8 原生命令行自动化中自然结束：

1. `DurableLaunchGate` 证明 Staged 与 authority reject 时滚转关闭，精确耐久提交后才激活；
2. 同一测试让真实 `URotatingMovementComponent` 前进 `0.125s`，即产生 `90°` 刀身滚转；
3. 滚转后 Visual Up 方向变化，但 Visual Forward、Actor 四元数和根碰撞四元数均保持原值；
4. `ArcReceiptMotion` 证明抛物线投掷遵守同一启停契约，终态关闭；
5. `ContactToVitality` 证明有效接触提交生命值并进入 `Spent` 后，本体、滚转和光提示同时收束；
6. `FailClosedAndMiss` 证明无法解析的接触保持真实 `InFlight` 与滚转，明确无命中终止后才关闭。

专项首跑即为 `6 Success / 0 Fail`，进程原生返回 0。日志 264,792 bytes，SHA-256 `39F8438FB6472618ACFD9DDA714D954235A77B191DF947BADF196B1F2F6B9DC2`。最终映射阶段又从未修改的同一代码点重复执行该组并保持 `6/0`。

## 6. 改动文件回归

3 个改动路径命中既有 `ThrownWeaponWorldDelivery` 映射规则，并推导 6 个精确测试组：

| Group | Success | SHA-256 |
|---|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 6 | `51F296C1F2AE9C78843004C3204A503EBA56698A48BC09B5F9FAAF67C88DDC69` |
| `Product.ThrownWeaponItemAdapter` | 5 | `481A00B48829E0AA8FA6271A3626BC4671485F399C7C19D1F9E2AED2B22B85CF` |
| `Product.CombatRunCoordinator` | 18 | `085872585543D6AD571302B729A680074B97F7EA89136F500BD99F61DA4FF62C` |
| `Shanmen.0_0_10.Items` | 77 | `A5812718F52BAD6EAE9CDCBC75B9829CBD5997D97A8FFFFB4DC8B1465D8EB931` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | `91E2BE5FA40D9BDDDDD9E038B7CF63C086C9DC55FD287E451D2095E9865F86B9` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | `7A47F70B62A4D191151E93131F5E9E99C7D6A65856FD2DEA0E793ACF9A4003B2` |

合计 `262 Success / 0 Fail`。每份日志都只有一个精确 `Automation RunTests` 命令、零 Fail、零 Fatal/Unhandled/Ensure，并含 UE 5.8 原生成功终止标记。

覆盖门结果为 `PASS Changed=3 Rules=1 Required=6 Logs=6`；日志 1,814 bytes，SHA-256 `4C7906F25FDBEEC756C0F09D27705C1F3EB895F33D4AE481C382EE3E4951A88B`。门禁自测为 `439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

## 7. 构建、产物与静态边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development final | Succeeded / native 0 | 118 / 222.17s | `56884FC0FEBD7523A00B3E927159AF65AEB4BAC8F6BF5D6B744CFFB4C516432D` |
| Editor Development final | Succeeded / native 0 | 0 / 1.51s | `BEE55DFE5ADE67287EB1A6C5382887705E590B58C1A91F3633B67253F463CB61` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,559,680 bytes；SHA-256 `669EA71F9BBE6509C3BB64AB2B41691765334B7D188241FCBB20034A725AA626`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,758,144 bytes；SHA-256 `B126837728CA1A024667C5E65719048DF5F4DBBEA01D0B00BE8B5C12153005EC`。

实现与测试差异为 3 files / 93 insertions / 1 deletion，其中生产新增 64 行。新增生产行中的 Timer、SetTimer、自定义 Tick、PrimaryActorTick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor 和 Destroy 均为 0；`git diff --check` 原生退出码为 0。

## 8. 生命周期与失败关闭

滚转组件缺失、更新目标不是 Visual、Staged 时意外激活或姿态未归零，都会使阶段验证失败关闭。耐久提交拒绝不会激活滚转；未注册接触不会提前关闭；只有既有有效接触或显式 no-impact terminal 才进入 `Spent` 并重置表现。

归零只修改 Visual 的相对旋转。Actor 世界旋转、Collision 世界旋转和 Movement Velocity 均未被重写，从结构上避免表现状态反向污染物理权威。

## 9. P/F 边界

PASS：真实 `InFlight` 才有刀身滚转；直线与抛物线均可用；耐久拒绝、有效接触、未注册接触与明确 miss 路径均有证明；滚转轴与碰撞/Actor/弹道隔离；库存、动作、伤害和飞行权威保持单一；映射回归、覆盖门、自测及双目标构建全部通过。

未声明：最终模型枢轴、材质高光、运动模糊、转速舒适度、网络复制、真实地图可读性或性能已经人工验收。本轮没有启动任何交互式产品运行。

## 10. 提交边界与 GitHub

本阶段只提交 3 个实现/测试文件、本 Report 与本 Development Log，共 5 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.4` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-4-thrown-weapon-flight-roll>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-4-thrown-weapon-flight-roll/Docs/Report/Dev.D.UE.0.0.10.P22.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-4-thrown-weapon-flight-roll/Docs/Log/Dev.D.UE.0.0.10.P22.4.r0_log.md>
