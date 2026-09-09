# Dev.D.UE.0.0.10.P22.5.r0 Development Log

## 1. 基线与目标

- base：`f09b89ce98d872c0150e03c195d6dd2c209f3128`（P22.4 thrown-weapon flight roll）；
- branch：`agent/0.0.10-p22-5-thrown-weapon-silhouette`；
- 目标：将单块飞刀原型升级为刀身与窄柄组成的可辨识轮廓，并让两段服从同一表现滚转；
- 边界：保持既有碰撞包络、飞行、命中、耐久、动作、伤害与输入权威，不启动 UI/PIE/产品 executable。

## 2. 审计与方案

P22.1 已建立真实物理 Carrier，P22.3 增加轨迹色飞行提示，P22.4 增加沿刀身轴的滚转；但实际可见网格仍是一个等宽 Cube，玩家只能看见“长方体”，不能从轮廓区分刀身与握柄。

选择最小的直接表现改动：保留原 `30 × 4.5 × 1.2 cm` 根碰撞，将可见网格拆为 `22 cm` 刀身与 `8 cm` 窄柄，并在两者上方增加一个共享 `USceneComponent` 枢轴。既有 `URotatingMovementComponent` 改为更新该枢轴，使两段整体转动。没有增加命中提示包装层、Actor、Tick、Timer 或新资产依赖。

## 3. 几何与组件实现

构造期建立以下层级：

```text
Collision (30 × 4.5 × 1.2 cm; existing authority)
  -> PresentationPivot (unit transform)
       -> ThrownWeaponVisual     blade 22 × 4.5 × 1.2 cm @ X=+4
       -> ThrownWeaponGripVisual grip   8 × 3.0 × 1.2 cm @ X=-11
```

刀身范围为 `[-7,+15]`，握柄范围为 `[-15,-7]`，在 `-7` 精确相接。两者共享 Engine Cube，只承担表现，均关闭 collision 与 overlap。共享枢轴直接附着 Collision，但不改变 Collision 的尺寸、通道或运动目标。

## 4. 生命周期与滚转

- `VisualRoll.UpdatedComponent` 从刀身改为 `PresentationPivot`；
- `Empty/Staged/Spent`：枢轴、刀身、握柄隐藏，滚转关闭，枢轴相对旋转归零；
- `InFlight`：三者显示，既有 `720°/s` 本地 Roll 驱动整个轮廓；
- Stage/InFlight 验证同时检查几何契约与滚转目标；
- 对外 Forward/Up 来自共享枢轴，不暴露组件写入口；
- 可见性显式写入三个组件，兼容注册与未注册的 Actor 路径。

## 5. 测试扩展

在既有真实 World `DurableLaunchGate` 中读取生产组件，不建立平行模型：

- 精确断言两个静态网格、共享资产、共享父枢轴与根碰撞层级；
- 精确断言两段尺寸、位置、首尾连续和原包络边界；
- 精确断言两个表现网格均无碰撞；
- Stage、authority reject 与 publish 分别证明双段显隐；
- 推进实际内置滚转组件 `0.125s`，证明刀身与握柄 Up 同步变化；
- 同时证明 Actor Quat、Collision Quat 与飞行 Forward 不变。

既有 Contact、Arc 与 Miss 测试继续覆盖命中/未命中终态，不改变产品路径。

## 6. 首次构建与环境修复

首次 Editor 构建使用 `MaxParallelActions=4`。机器当时提交内存约 `94.0/96.4 GB`，超过 UBA `91.5 GB` kill threshold，日志连续记录 204 次低内存杀进程。该构建没有产生源码诊断，也无法前进；有界停止后保留原始日志：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.5_editor_build_initial.log` | environment memory pressure / bounded stop | 31,214 | `14C3921B34A360FFE8812E2A50803A22E50246234D4DEAA2AC5D9867E125C615` |

未关闭任何用户进程。改用 `MaxParallelActions=1 -NoUBA` 后完成全部 119 actions，总执行时间 421.87s，原生退出码 0：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.5_editor_build_retry_low_memory.log` | 119 actions / PASS / native 0 | 12,640 | `6C30BD928A13386373FDBD0650D05D3D7F6E98DA065EE68A7D5F83E7B86A7A44` |

## 7. 首次测试与源码修复

首个专项自然完成 `4 Success / 2 Fail`。失败发生在使用 `NewObject<Actor>` 且未注册到 World 的瞬态路径：枢轴的递归 visibility 传播依赖已建立的附着子树，未可靠同步两个子网格，导致 Stage 校验失败。原始日志保留：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `Focused-ThrownWeaponWorldDelivery.log` | 4/2 / native process 0 | 269,395 | `F44A6F7FE74C11C1516225F61B0C17AF6C118988E1E343527BB4BEF021D0619B` |

修复把 `SetPresentationVisibility` 改为显式设置枢轴、刀身、握柄，不改状态机和断言。增量 Editor 构建完成 4 actions、原生退出码 0；专项重跑为 `6/0`：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.5_editor_build_after_visibility_fix.log` | 4 actions / PASS / native 0 | 2,364 | `73E204EC7D37BC162718CC0D935EDE8EB68D1D8F434FF308568F40EDC1B8E0E3` |
| `Focused-ThrownWeaponWorldDelivery-after-fix.log` | 6/0 / native 0 | 268,731 | `49B2E1D00FE9C56C0B6FDAE23CD8A35E586A3E1599CE7AB21E6BBEE7D5020E41` |

## 8. 改动文件映射回归

3 个改动路径由既有 1 条规则推导 6 个精确组：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 6 | 268,716 | `0AE6348A07CC10EC979AE26CE17BA7658CAA19DB869B5C6893EA9560CB2853DF` |
| `Product.ThrownWeaponItemAdapter` | 5 | 266,851 | `9DE5B1F4B0EC401F679A5F7F79C5D819F25FDBE3FB004FC23403E4DA6DEE5AAA` |
| `Product.CombatRunCoordinator` | 18 | 287,850 | `2678A709FDCFCD6C2F605ED3FAF7F9CE9216BCDC95BD217A4C9BC6AF5C9915FE` |
| `Shanmen.0_0_10.Items` | 77 | 351,005 | `62446BFC947BF21F7C24C7B4592690ACBD56778B9284B8E1BF55FEB3F5B55A51` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 270,518 | `B89A1874B2302164740AE7745A05B27DE08EBB1DD785CFEFB3F9279766386959` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 411,002 | `8F7A10EC1E519BB1208B13D12EAFDC7766D1D90D95BF057968C694F71990A322` |

合计 `262/0`。六份日志均为单一命令、自然终止、零 Fail、零 Fatal/Unhandled/Ensure。

## 9. 覆盖门、构建与静态检查

覆盖门：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=6 Logs=6
```

- gate：1,814 bytes，SHA-256 `916460BA9A04487FD8562F2E74FBFF9111D64DEF6E86E83B8C25CECAC92C4537`；
- self-test：`439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.5_game_build_final.log` | 118 actions / PASS / native 0 | 12,460 | `E49ABF69ACBCBF16A111E6A08BF5C54161A370E51299BEE106F987A1C29CD9EC` |
| `P22.5_editor_build_final.log` | 0 actions / PASS / native 0 | 1,035 | `5E82A2EC23D34EDEB0BB22F8B6C8FC3E94EE0221AA94A187C09D85C254DCD19E` |

`demo_map.exe` 为 359,564,288 bytes，SHA-256 `0FA4813797CD1C69BD317205E00BED5A46C03B96505BAD3C649E7C5D473D0665`。`UnrealEditor-demo_map.dll` 为 18,770,432 bytes，SHA-256 `6F1B1B921F7B2B5AFD6AECB2651CD43F4EE58A6E7F1841636BDCA40D06A6B9B5`。

实现/测试 diff 为 3 files / 193 insertions / 30 deletions。生产新增 111 行；Timer、SetTimer、自定义 Tick、PrimaryActorTick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0。`git diff --check` 为 0。

## 10. 提交边界与 GitHub

精确提交 3 个实现/测试文件与本 Report/Log。103 个用户原有 untracked 文件不暂存；所有 raw evidence 留在 `Saved/Codex/P22.5` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-5-thrown-weapon-silhouette>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-5-thrown-weapon-silhouette/Docs/Report/Dev.D.UE.0.0.10.P22.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-5-thrown-weapon-silhouette/Docs/Log/Dev.D.UE.0.0.10.P22.5.r0_log.md>
