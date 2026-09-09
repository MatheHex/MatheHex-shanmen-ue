# Dev.D.UE.0.0.10.P22.7.r0 Report

## 1. 结论

P22.7 在 P 阶段边界内完成，结论为 **PASS**。

本轮在 P22.6 三色飞刀原型上增加一条单侧亮刃：原 `22 × 4.5 × 1.2 cm` 刀身被无缝分为 `22 × 4.0 × 1.2 cm` 主体与 `22 × 0.5 × 1.2 cm` 亮色刃线，组合仍精确占满既有刀身与根碰撞宽度。刃线只位于局部 `-Y` 一侧，因此既有 `720°/s` 绕 X 轴滚转不再只有对称截面变化，而会带动明确的非对称亮线绕刀身移动。

```text
Thrown-weapon World delivery focus:      6 Success / 0 Fail
Changed-file mapped regression:        262 Success / 0 Fail (6 healthy logs)
Changed-file regression coverage:      PASS (3 files / 1 rule / 6 required groups)
Regression gate self-test:              PASS 439/439
Game + Editor Development:              PASS (Game 118 actions; Editor 0 actions; both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明实际组件几何、材质、滚转位移、显隐和物理隔离，不宣称最终像素效果、真实地图可读性或玩家手感已经人工验收。

## 2. 玩家可见结果

| 部位 | 尺寸与局部位置 | 原型颜色 | 用途 |
|---|---|---|---|
| 刀身主体 | `22 × 4.0 × 1.2 cm @ (4, 0.25, 0)` | 浅钢蓝 `(0.62, 0.78, 1.00)` | 保留主要刃面 |
| 单侧刃线 | `22 × 0.5 × 1.2 cm @ (4, -2.0, 0)` | 亮银蓝 `(0.92, 0.97, 1.00)` | 提供非对称滚转相位提示 |
| 握柄 | `8 × 3.0 × 1.2 cm @ (-11, 0, 0)` | 深棕 `(0.16, 0.045, 0.012)` | 保留前后方向与窄柄轮廓 |

主体局部 Y 范围为 `[-1.75, +2.25]`，刃线为 `[-2.25, -1.75]`：二者无间隙、无重叠，联合范围仍是原刀身的 `[-2.25, +2.25]`。X 范围共同保持 `[-7, +15]`，握柄继续从 `-15` 到 `-7`，完整轮廓不超出既有 `30 × 4.5 × 1.2 cm` 根碰撞包络。

## 3. 单一事实链

```text
existing durable launch publication
  -> existing projectile State becomes InFlight
  -> existing presentation pivot shows blade body + one-sided edge + grip
  -> existing built-in roll rotates all three sections
  -> asymmetric edge changes world position around the physical X axis
  -> existing contact / no-impact terminal hides the complete silhouette
```

刃线只读取既有表现枢轴变换，不选择轨迹、不推进动作、不修改库存、不解析接触，也不拥有第二份滚转或飞行状态。

## 4. 实现

`Ademo_mapShanmenThrownWeaponProjectile` 增加一个私有 `ThrownWeaponBladeEdgeVisual` 与对应 `BladeEdgeMaterial`：

- 刃线使用既有 Engine Cube，直接附着 P22.5 的 `PresentationPivot`；
- 刀身主体横向偏移 `+0.25 cm` 并缩窄为 `4.0 cm`，刃线位于 `-Y` 侧、宽 `0.5 cm`；
- 两者精确拼成原 `4.5 cm` 刀身宽度，碰撞盒、总长度与厚度不变；
- 刃线关闭 collision 与 overlap，与主体、握柄共同服从既有显隐和滚转；
- `BladeEdgeMaterial` 是独立动态材质实例，`Color`/`BaseColor` 固定为亮银蓝；
- `HasPresentationMaterialContrast()` 扩展为验证三个 MID 均存在、两两不同、槽位正确且三色精确；
- `IsPresentationGeometryValid()` 扩展为验证刃线资产、附着、尺寸、位置、旋转、无碰撞及材质契约，Stage/InFlight 继续失败关闭。

未新增 Actor、Tick、Timer、Timeline、材质资产、纹理或新模块依赖。

## 5. 真实 World 证明

扩展既有 `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.DurableLaunchGate`，通过真实测试 World 生成生产 Projectile 并读取实际组件：

1. Actor 精确包含刀身主体、单侧刃线和握柄三个表现网格；
2. 三者共享同一 Engine Cube、同一无碰撞 `PresentationPivot`，且均禁用 collision；
3. 主体与刃线在 Y 轴精确拼接，联合边界等于根碰撞宽度；两者 X 边界继续连接握柄并到达根碰撞前端；
4. 三个真实材质槽分别持有互异 MID 与精确浅钢蓝、亮银蓝、深棕参数；
5. Staged 与耐久拒绝隐藏三段，durable publication 后三段同时显示；
6. 推进实际 `URotatingMovementComponent` `0.125s`（90°）后，刃线 Up 与整体枢轴同步变化，其世界位置也发生变化；Actor、根碰撞和飞行前向保持不变。

专项首轮自然完成 `6 Success / 0 Fail`、进程原生退出码 0。日志 268,346 bytes，SHA-256 `E59482D0D8F21578D4544F00C55CAE888CB52E7C611F8F0895BECBDA467A1202`。

## 6. 首轮结果

本轮从第一步沿用单并发低内存构建路径。首次 Editor 构建完成 119 actions、原生退出码 0；首次专项即为 `6/0`，没有 P22.7 源码失败、测试失败或修复轮。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.7_editor_build_initial.log` | 119 actions / PASS / native 0 | 12,678 | `773DF18BFE041B3C3E2850D3B1100151F089D9EA35052B84B63089A777AA7353` |
| `Focused-ThrownWeaponWorldDelivery.log` | 6/0 / native process 0 | 268,346 | `E59482D0D8F21578D4544F00C55CAE888CB52E7C611F8F0895BECBDA467A1202` |

UE 5.8 启动期的固定 UnifiedError 自检输出位于测试命令之前；正式测试事件无 Fatal、Unhandled、Ensure 或 Test Fail。

## 7. 改动文件回归

3 个实现/测试路径命中既有 `ThrownWeaponWorldDelivery` 映射规则，并推导 6 个精确测试组：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 6 | 268,346 | `E59482D0D8F21578D4544F00C55CAE888CB52E7C611F8F0895BECBDA467A1202` |
| `Product.ThrownWeaponItemAdapter` | 5 | 266,660 | `A426D4DDA9C1F12A412D108BFBE6A39BF8D9BDD15449B681ACC11CE560FAFD32` |
| `Product.CombatRunCoordinator` | 18 | 287,352 | `7CB93645A844CFD2AF6C847E722ADF9E2C6484C60AB0CE0076EC1597812F1EBE` |
| `Shanmen.0_0_10.Items` | 77 | 351,785 | `7DC97F260A83155683D784A06466CD447F4DE819C59B19AB9D676D266FFE3CE4` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 270,502 | `D22734E7F14CD61EB297BCC8D5843A918A89F9D3D27CA2865E6D31E100440B8D` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 411,103 | `C4FB762CDDC931EB618FB9C5AF147381B6FCB1F2B8F6FF4B879D65F42115717B` |

合计 `262 Success / 0 Fail`。覆盖门为 `PASS Changed=3 Rules=1 Required=6 Logs=6`；门日志 1,746 bytes，SHA-256 `F7A763DDAD57D53A02C5342ADBDFE4B4891017DD25D57A8E95B822EC3ECA559B`。门禁自测保持 `439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

## 8. 构建、产物与静态边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development final | Succeeded / native 0 | 118 / 355.26s | `3A3CD2E52E7AB5ED60076B5D948243024AE638655817340236C73F46D03C2022` |
| Editor Development final | Succeeded / native 0 | 0 / 1.08s | `ED4DBF6A54545BA02D1B6B2E4229C1DFFD351B46912DB48313CE81D8DAF2ACA6` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,569,408 bytes；SHA-256 `78BD2788C896DF73FFDD05455757930311F225FBC60CAEE35101D5BD869B21C2`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,778,112 bytes；SHA-256 `C903872D7EDFB38EB112C48F93E4C80356538FCEC18D775ACAF8D297EE90F4B5`。

实现与测试差异为 3 files / 134 insertions / 16 deletions，其中生产新增 60 行。新增生产行中的 Timer、SetTimer、自定义 Tick、PrimaryActorTick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor 和 Destroy 均为 0；`git diff --check` 原生退出码为 0。

## 9. P/F 边界

PASS：单侧亮刃与刀身主体无缝拼成原包络；三段表现无碰撞且共享既有滚转枢轴；三色 MID 两两独立；刃线在 90° 滚转后产生可验证世界位移；耐久门、显隐、碰撞、轨迹、终态、伤害、库存和 Run 权威保持不变；映射回归、覆盖门、自测及双目标构建全部通过。

未声明：这是最终刀模、刃口几何或 PBR 材质；未验证光照、运动模糊、真实地图像素可读性、网络复制或玩家手感。没有执行交互式产品验收。

## 10. 提交边界与 GitHub

本阶段只提交 3 个实现/测试文件、本 Report 与本 Development Log，共 5 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.7` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-7-thrown-weapon-edge-cue>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-7-thrown-weapon-edge-cue/Docs/Report/Dev.D.UE.0.0.10.P22.7.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-7-thrown-weapon-edge-cue/Docs/Log/Dev.D.UE.0.0.10.P22.7.r0_log.md>
