# Dev.D.UE.0.0.10.P22.4.r0 Development Log

## 1. 基线与目标

- base：`1adb86d4d69ebce3f5a7ad08c517372717bb4f5e`（P22.3 trajectory-coded flight cue）；
- branch：`agent/0.0.10-p22-4-thrown-weapon-flight-roll`；
- 目标：让既有物理飞刀在真实飞行阶段获得沿刀身轴的持续滚转，提高高速小型刀身的动态可读性；
- 边界：只转无碰撞 Visual，不改变 Actor、根碰撞、弹道、库存、动作、伤害或输入权威，不启动 UI/PIE/产品 executable。

## 2. 审计与方案

P22.1 已建立真实飞刀 Carrier、可见刀身原型与精确碰撞，P22.2 已闭合终态 HUD 反馈，P22.3 已增加轨迹色飞行光提示。但 `30 × 4.5 × 1.2 cm` 的薄刀身仍保持静态姿态，高速飞行时缺少武器自身的运动特征。

现有 Actor 已有规范 `Empty -> Staged -> InFlight -> Spent` 状态和独立 collisionless Visual。最小方案是在同一个 Actor 上使用引擎 `URotatingMovementComponent`，只把 Updated Component 指向 Visual，并让既有 State 控制启停。不新增 Actor、自定义 Tick、Timer、动画资产或第二份飞行状态。

## 3. 组件实现

新增私有 `VisualRoll`：

- 类型为引擎内置 `URotatingMovementComponent`；
- `UpdatedComponent = Visual`；
- 本地 Roll 速率为 `720°/s`，Pivot 为零；
- `bRotationInLocalSpace = true`，`bAutoActivate = false`；
- 外部只读取得实际 Up 方向和激活状态，不能取得组件写入口。

刀身局部 X 前向轴也是原型长轴。Roll 因而改变刀身 Up/厚度朝向，但不改变 Visual Forward，更不改变 Actor 或碰撞根。

## 4. 生命周期接线

| Transition | Roll behavior |
|---|---|
| constructor -> Empty | inactive |
| Empty -> Staged | deactivate + relative rotation zero |
| Staged -> InFlight | activate after durable publication |
| Staged -> Empty / cancel | deactivate + zero |
| InFlight -> Spent / contact or miss | deactivate + zero |

`IsStagedFor` 验证组件存在、精确更新 Visual、未激活且相对旋转为零；`IsInFlightFor` 验证组件仍精确更新 Visual 且已激活。任何配置漂移都失败关闭。

## 5. 测试扩展

扩展既有真实 World fixture，没有新增绕过生产路径的独立模型：

- Durable Launch Gate：Staged 关闭、durability reject 关闭、publish 后开启；
- 将实际旋转组件推进 `0.125s`，验证 Up 变化与 `90°` 滚转一致；
- 同时冻结并对比 Visual Forward、Actor Quat、Collision Quat，证明三者不受滚转影响；
- Arc Receipt Motion：抛物线投掷遵守相同启停边界；
- Contact to Vitality：有效接触后关闭；
- Fail Closed and Miss：未注册接触保持滚转，显式无命中终止后关闭。

## 6. 首次执行

首次 Editor 构建通过 `119 actions`，总执行时间 `220.01s`，原生退出码 0。随后首个 World Delivery 专项自然完成 `6 Success / 0 Fail`，原生退出码 0：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `Focused-ThrownWeaponWorldDelivery.log` | 6/0 | 264,792 | `39F8438FB6472618ACFD9DDA714D954235A77B191DF947BADF196B1F2F6B9DC2` |

本阶段没有产品测试失败，因此没有为了通过而放宽断言或改变范围。最终回归从同一未修改代码点再次生成所有日志。

## 7. 改动文件映射回归

3 个改动路径由既有 1 条规则推导 6 个精确组：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 6 | 264,504 | `51F296C1F2AE9C78843004C3204A503EBA56698A48BC09B5F9FAAF67C88DDC69` |
| `Product.ThrownWeaponItemAdapter` | 5 | 262,827 | `481A00B48829E0AA8FA6271A3626BC4671485F399C7C19D1F9E2AED2B22B85CF` |
| `Product.CombatRunCoordinator` | 18 | 284,240 | `085872585543D6AD571302B729A680074B97F7EA89136F500BD99F61DA4FF62C` |
| `Shanmen.0_0_10.Items` | 77 | 348,263 | `A5812718F52BAD6EAE9CDCBC75B9829CBD5997D97A8FFFFB4DC8B1465D8EB931` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 266,980 | `91E2BE5FA40D9BDDDDD9E038B7CF63C086C9DC55FD287E451D2095E9865F86B9` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 408,096 | `7A47F70B62A4D191151E93131F5E9E99C7D6A65856FD2DEA0E793ACF9A4003B2` |

合计 `262/0`；六份日志均为单一命令、自然终止、零 Fail、零 Fatal/Unhandled/Ensure。

## 8. 覆盖门

既有 `ThrownWeaponWorldDelivery` 规则完整覆盖本轮 3 个源码/测试路径，不需要修改映射：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=6 Logs=6
```

门禁日志为 1,814 bytes，SHA-256 `4C7906F25FDBEEC756C0F09D27705C1F3EB895F33D4AE481C382EE3E4951A88B`。门禁自测保持 `439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

## 9. 构建与静态检查

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.4_game_build_final.log` | 118 actions / PASS / native 0 | 11,415 | `56884FC0FEBD7523A00B3E927159AF65AEB4BAC8F6BF5D6B744CFFB4C516432D` |
| `P22.4_editor_build_final.log` | 0 actions / PASS / native 0 | 974 | `BEE55DFE5ADE67287EB1A6C5382887705E590B58C1A91F3633B67253F463CB61` |

`demo_map.exe` 为 359,559,680 bytes，SHA-256 `669EA71F9BBE6509C3BB64AB2B41691765334B7D188241FCBB20034A725AA626`。`UnrealEditor-demo_map.dll` 为 18,758,144 bytes，SHA-256 `B126837728CA1A024667C5E65719048DF5F4DBBEA01D0B00BE8B5C12153005EC`。

实现/测试 diff 为 3 files / 93 insertions / 1 deletion。新增生产行 64；Timer、SetTimer、自定义 Tick、PrimaryActorTick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0。`git diff --check` 为 0。

## 10. 提交边界与 GitHub

精确提交 3 个实现/测试文件与本 Report/Log。103 个用户原有 untracked 文件不暂存；所有 raw evidence 留在 `Saved/Codex/P22.4` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-4-thrown-weapon-flight-roll>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-4-thrown-weapon-flight-roll/Docs/Report/Dev.D.UE.0.0.10.P22.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-4-thrown-weapon-flight-roll/Docs/Log/Dev.D.UE.0.0.10.P22.4.r0_log.md>
