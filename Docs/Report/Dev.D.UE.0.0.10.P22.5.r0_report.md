# Dev.D.UE.0.0.10.P22.5.r0 Report

## 1. 结论

P22.5 在 P 阶段边界内完成，结论为 **PASS**。

本轮将 P22.4 的单块长方体飞刀表现改为同一物理投掷物下可辨识的“双段原型轮廓”：`22 × 4.5 × 1.2 cm` 刀身与 `8 × 3 × 1.2 cm` 窄柄首尾相接，共同占满既有 `30 × 4.5 × 1.2 cm` 碰撞包络的长度。两段表现共用一个无碰撞枢轴，因此既有飞行滚转会驱动完整轮廓，而不是只转其中一段。

```text
Thrown-weapon World delivery focus:      6 Success / 0 Fail
Changed-file mapped regression:        262 Success / 0 Fail (6 healthy logs)
Changed-file regression coverage:      PASS (3 files / 1 rule / 6 required groups)
Regression gate self-test:              PASS 439/439
Game + Editor Development:              PASS (Game 118 actions; Editor 0 actions; both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明原型几何、组件附着、整体滚转与生命周期契约，不宣称最终刀模、材质、握柄工艺、尺寸手感或真实地图可读性已经人工验收。

## 2. 玩家可见结果

| 部位 | 原型尺寸 | 局部 X 范围 | 作用 |
|---|---:|---:|---|
| 刀身 | `22 × 4.5 × 1.2 cm` | `-7 .. +15 cm` | 保留薄、长、前向明确的飞刀主体 |
| 窄柄 | `8 × 3 × 1.2 cm` | `-15 .. -7 cm` | 用收窄宽度形成可辨认的握柄轮廓 |
| 根碰撞 | `30 × 4.5 × 1.2 cm` | `-15 .. +15 cm` | 继续作为唯一 Sweep 与接触包络 |

刀身和握柄在 `X=-7 cm` 精确相接，无间隙、无重叠；两端分别与原碰撞包络的前后端对齐。握柄宽度小于刀身，飞行时不再表现为单一等宽长方体。

两段均使用既有 Engine Cube 作为临时原型资产、关闭碰撞与 overlap。`Empty`、`Staged` 与 `Spent` 隐藏完整轮廓；只有耐久提交成功后的 `InFlight` 才显示并整体滚转。

## 3. 单一事实链

```text
existing immutable launch receipt
  -> existing durable Quantity publication
  -> existing projectile State becomes InFlight
  -> one presentation pivot shows blade + grip
  -> existing built-in roll rotates that shared pivot
  -> existing contact / no-impact terminal
  -> existing projectile State becomes Spent
  -> pivot and both meshes hide; roll resets
```

刀身、握柄、滚转和 P22.3 飞行光提示均服从同一份 Projectile State。新增枢轴只组合表现，不选择轨迹、不推进动作、不修改库存、不解析接触，也不拥有第二份飞行生命周期。

## 4. 实现

`Ademo_mapShanmenThrownWeaponProjectile` 增加一个私有 `USceneComponent` 表现枢轴和一个私有 `UStaticMeshComponent` 握柄：

- `PresentationPivot` 直接附着既有 `Collision`；
- 原 `Visual` 改为刀身，附着 `PresentationPivot`，尺寸为 `22 × 4.5 × 1.2 cm`，局部位置为 `(4, 0, 0)`；
- 新 `GripVisual` 同样附着该枢轴，尺寸为 `8 × 3 × 1.2 cm`，局部位置为 `(-11, 0, 0)`；
- 两段使用同一个静态网格，碰撞与 overlap 均关闭；
- P22.4 的 `URotatingMovementComponent` 从只更新刀身改为更新共享枢轴；
- 对外前向与 Up 方向从共享枢轴读取，Actor、根碰撞和运动组件仍是物理权威；
- `IsPresentationGeometryValid` 对附着层级、共享网格、尺寸、位置、零局部旋转和无碰撞属性执行失败关闭检查；
- 生命周期显式同步枢轴、刀身和握柄的可见性，不依赖组件注册后才可靠的递归传播。

没有新增 Actor、自定义 Tick、Timer、Timeline、动态材质、动画资产或第二套命中系统。

## 5. 真实 World 证明

扩展既有 `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.DurableLaunchGate`，直接检查生产 Actor 的真实组件：

1. Actor 精确包含两个表现静态网格，名字分别为 `ThrownWeaponVisual` 与 `ThrownWeaponGripVisual`；
2. 两者共享同一个实际 Cube 网格和同一个 `PresentationPivot`，枢轴直接附着根碰撞；
3. 刀身、握柄尺寸与局部位置精确匹配本轮契约，边界连续且完整落在原碰撞包络内；
4. 两个网格均为 `NoCollision`，不会形成第二份接触权威；
5. Staged 与耐久拒绝时两段均隐藏，durable publication 后两段同时显示；
6. 将实际 `URotatingMovementComponent` 推进 `0.125s` 后，两段 Up 向量同时变化且保持相等；Actor 四元数、Collision 四元数与飞行前向均保持不变。

修复后专项日志为 `6 Success / 0 Fail`、原生退出码 0，268,731 bytes，SHA-256 `49B2E1D00FE9C56C0B6FDAE23CD8A35E586A3E1599CE7AB21E6BBEE7D5020E41`。最终映射阶段从同一未修改代码点再次执行该组并保持 `6/0`。

## 6. 首次失败与修复

首次 Editor 构建使用 4 个并行任务时，系统提交内存已约为 `94.0/96.4 GB`，高于构建器 `91.5 GB` kill threshold。UBA 连续记录 204 次低内存杀进程，未形成源码失败结论；在有界等待后主动终止无进展构建。原日志 31,214 bytes，SHA-256 `14C3921B34A360FFE8812E2A50803A22E50246234D4DEAA2AC5D9867E125C615`。不关闭用户应用，改用 `MaxParallelActions=1 + NoUBA` 后完成 119 actions，原生退出码 0。

首次聚焦测试随后得到 `4 Success / 2 Fail`。两个失败都经过未注册到 World 的瞬态 Actor 路径；共享枢轴的递归 `SetVisibility` 在未注册附着树上没有可靠传播到两个子网格，使 Stage 校验失败。保留失败日志 269,395 bytes，SHA-256 `F44A6F7FE74C11C1516225F61B0C17AF6C118988E1E343527BB4BEF021D0619B`。

修复只将表现可见性改为显式设置枢轴、刀身和握柄，没有放宽几何、生命周期或耐久门断言。增量 Editor 构建 4 actions 通过，专项恢复为 `6/0`。

## 7. 改动文件回归

3 个改动路径命中既有 `ThrownWeaponWorldDelivery` 映射规则，并推导 6 个精确测试组：

| Group | Success | SHA-256 |
|---|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 6 | `0AE6348A07CC10EC979AE26CE17BA7658CAA19DB869B5C6893EA9560CB2853DF` |
| `Product.ThrownWeaponItemAdapter` | 5 | `9DE5B1F4B0EC401F679A5F7F79C5D819F25FDBE3FB004FC23403E4DA6DEE5AAA` |
| `Product.CombatRunCoordinator` | 18 | `2678A709FDCFCD6C2F605ED3FAF7F9CE9216BCDC95BD217A4C9BC6AF5C9915FE` |
| `Shanmen.0_0_10.Items` | 77 | `62446BFC947BF21F7C24C7B4592690ACBD56778B9284B8E1BF55FEB3F5B55A51` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | `B89A1874B2302164740AE7745A05B27DE08EBB1DD785CFEFB3F9279766386959` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | `8F7A10EC1E519BB1208B13D12EAFDC7766D1D90D95BF057968C694F71990A322` |

合计 `262 Success / 0 Fail`。每份日志都只有一个精确 `Automation RunTests` 命令、零 Fail、零 Fatal/Unhandled/Ensure，并含 UE 5.8 原生成功终止标记。

覆盖门结果为 `PASS Changed=3 Rules=1 Required=6 Logs=6`；日志 1,814 bytes，SHA-256 `916460BA9A04487FD8562F2E74FBFF9111D64DEF6E86E83B8C25CECAC92C4537`。门禁自测保持 `439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

## 8. 构建、产物与静态边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development final | Succeeded / native 0 | 118 / 390.15s | `E49ABF69ACBCBF16A111E6A08BF5C54161A370E51299BEE106F987A1C29CD9EC` |
| Editor Development final | Succeeded / native 0 | 0 / 1.15s | `5E82A2EC23D34EDEB0BB22F8B6C8FC3E94EE0221AA94A187C09D85C254DCD19E` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,564,288 bytes；SHA-256 `0FA4813797CD1C69BD317205E00BED5A46C03B96505BAD3C649E7C5D473D0665`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,770,432 bytes；SHA-256 `6F1B1B921F7B2B5AFD6AECB2651CD43F4EE58A6E7F1841636BDCA40D06A6B9B5`。

实现与测试差异为 3 files / 193 insertions / 30 deletions，其中生产新增 111 行。新增生产行中的 Timer、SetTimer、自定义 Tick、PrimaryActorTick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor 和 Destroy 均为 0；`git diff --check` 原生退出码为 0。

## 9. P/F 边界

PASS：飞刀原型具有刀身与窄柄的可辨识轮廓；两段首尾连续并保持在原碰撞包络内；两段无碰撞且由同一枢轴整体滚转；耐久拒绝、有效发布、接触终态和未命中终态继续服从既有单一状态；映射回归、覆盖门、自测及双目标构建全部通过。

未声明：这不是最终美术资产；未验证材质、法线、刀刃厚度、握柄纹理、运动模糊、真实地图视距、网络复制或玩家手感。没有执行交互式产品验收。

## 10. 提交边界与 GitHub

本阶段只提交 3 个实现/测试文件、本 Report 与本 Development Log，共 5 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.5` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-5-thrown-weapon-silhouette>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-5-thrown-weapon-silhouette/Docs/Report/Dev.D.UE.0.0.10.P22.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-5-thrown-weapon-silhouette/Docs/Log/Dev.D.UE.0.0.10.P22.5.r0_log.md>
