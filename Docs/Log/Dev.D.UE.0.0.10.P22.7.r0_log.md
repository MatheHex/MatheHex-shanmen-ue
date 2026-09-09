# Dev.D.UE.0.0.10.P22.7.r0 Development Log

## 1. 基线与目标

- base：`b9b42b6ba7696689c1545d18a150bdf4c9347ce5`（P22.6 thrown-weapon material contrast）；
- branch：`agent/0.0.10-p22-7-thrown-weapon-edge-cue`；
- 目标：为飞刀增加单侧亮刃，使既有滚转具有可辨认的非对称相位提示；
- 边界：不改碰撞、轨迹、速度、伤害、库存、Run、输入或终态所有权，不启动 UI/PIE/产品 executable。

## 2. 审计与取舍

P22.2 已把权威命中/落空终态投影到主 HUD。接触后的 Carrier 由 Run Host 立即进入终态并销毁；若加入命中后停留，会改变现有所有权与清理语义，不适合作为小型表现阶段。

P22.4 的滚转已经存在，但 P22.5/P22.6 的纵向刀身、握柄和颜色分区绕 X 轴转动时仍缺少稳定的单侧标记。选择在同一刀身上增加不对称刃线：它直接增强飞行读向，又不触及终态、计时或物理链。

## 3. 几何实现

保持原 `22 × 4.5 × 1.2 cm` 刀身联合边界，将其拆为：

```text
BladeBody: 22 × 4.0 × 1.2 @ (4, +0.25, 0) -> Y [-1.75, +2.25]
BladeEdge: 22 × 0.5 × 1.2 @ (4, -2.00, 0) -> Y [-2.25, -1.75]
Grip:       8 × 3.0 × 1.2 @ (-11, 0, 0)
```

主体与刃线只共享边界平面，不重叠、不留缝。两者 X 范围均为 `[-7,+15]`，继续与握柄 `[-15,-7]` 相接，并完整留在原 `30 × 4.5 × 1.2 cm` 碰撞盒中。

新增 `ThrownWeaponBladeEdgeVisual` 使用现有 Engine Cube，附着现有 `PresentationPivot`，关闭 collision/overlap。既有滚转组件仍只更新该枢轴。

## 4. 材质与失败关闭

新增私有 transient `BladeEdgeMaterial` MID，设置亮银蓝 `Color`/`BaseColor (0.92, 0.97, 1.00)`。P22.6 的材质契约扩展为同时验证刀身主体、刃线和握柄：

- 三个 MID 均存在且两两不同；
- 三个实际材质槽保持正确实例；
- 三组 `Color` 参数精确匹配固定原型值。

`IsPresentationGeometryValid()` 同时验证刃线附着、共享网格、无碰撞、位置、缩放与零局部旋转。`SetPresentationVisibility()`、`IsPresentationVisible()` 和 Staged 门均显式包含刃线，避免未注册组件树的递归可见性差异。

## 5. 真实 World 测试

扩展既有生产 World `DurableLaunchGate`，未新建平行模型：

- 精确发现三个命名表现组件并要求总数为 3；
- 证明主体/刃线横向无缝拼接且联合边界等于根碰撞宽度；
- 证明三段共享枢轴、网格且均无碰撞；
- 证明三个实际 MID 两两不同并持有精确颜色；
- 证明 Staged、耐久拒绝与 InFlight 三种状态的完整显隐；
- 将实际滚转推进 `0.125s`，证明单侧刃线 Up 与位置同时变化、三段姿态一致、材质槽不漂移；
- 继续证明 Actor/Collision 旋转和飞行 Forward 不变。

首次专项即为 `6 Success / 0 Fail`。

## 6. 初始证据

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.7_editor_build_initial.log` | 119 actions / PASS / native 0 | 12,678 | `773DF18BFE041B3C3E2850D3B1100151F089D9EA35052B84B63089A777AA7353` |
| `Focused-ThrownWeaponWorldDelivery.log` | 6/0 / native process 0 | 268,346 | `E59482D0D8F21578D4544F00C55CAE888CB52E7C611F8F0895BECBDA467A1202` |

没有源码失败或测试失败。UE 5.8 的固定 UnifiedError 启动自检发生在测试命令之前，不属于本轮测试事件。

## 7. 改动文件映射回归

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 6 | 268,346 | `E59482D0D8F21578D4544F00C55CAE888CB52E7C611F8F0895BECBDA467A1202` |
| `Product.ThrownWeaponItemAdapter` | 5 | 266,660 | `A426D4DDA9C1F12A412D108BFBE6A39BF8D9BDD15449B681ACC11CE560FAFD32` |
| `Product.CombatRunCoordinator` | 18 | 287,352 | `7CB93645A844CFD2AF6C847E722ADF9E2C6484C60AB0CE0076EC1597812F1EBE` |
| `Shanmen.0_0_10.Items` | 77 | 351,785 | `7DC97F260A83155683D784A06466CD447F4DE819C59B19AB9D676D266FFE3CE4` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 270,502 | `D22734E7F14CD61EB297BCC8D5843A918A89F9D3D27CA2865E6D31E100440B8D` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 411,103 | `C4FB762CDDC931EB618FB9C5AF147381B6FCB1F2B8F6FF4B879D65F42115717B` |

合计 `262/0`。六份日志均为单一精确命令、零 Fail、零 Fatal/Unhandled/Ensure，并自然到达 UE 5.8 成功终止标记。

## 8. 覆盖门、构建与产物

覆盖门为 `REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=6 Logs=6`；门日志 1,746 bytes，SHA-256 `F7A763DDAD57D53A02C5342ADBDFE4B4891017DD25D57A8E95B822EC3ECA559B`。自测为 `439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.7_game_build_final.log` | 118 actions / PASS / native 0 | 12,460 | `3A3CD2E52E7AB5ED60076B5D948243024AE638655817340236C73F46D03C2022` |
| `P22.7_editor_build_final.log` | 0 actions / PASS / native 0 | 1,035 | `ED4DBF6A54545BA02D1B6B2E4229C1DFFD351B46912DB48313CE81D8DAF2ACA6` |

`demo_map.exe` 为 359,569,408 bytes，SHA-256 `78BD2788C896DF73FFDD05455757930311F225FBC60CAEE35101D5BD869B21C2`。`UnrealEditor-demo_map.dll` 为 18,778,112 bytes，SHA-256 `C903872D7EDFB38EB112C48F93E4C80356538FCEC18D775ACAF8D297EE90F4B5`。

## 9. 静态边界

实现/测试 diff 为 3 files / 134 insertions / 16 deletions，生产新增 60 行。新增生产行中 Timer、SetTimer、自定义 Tick、PrimaryActorTick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0。没有新增资产或模块依赖；`git diff --check` 为 0。

## 10. 提交边界与 GitHub

精确提交 3 个实现/测试文件与本 Report/Log。103 个用户原有 untracked 文件不暂存；所有 raw evidence 留在 `Saved/Codex/P22.7` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-7-thrown-weapon-edge-cue>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-7-thrown-weapon-edge-cue/Docs/Report/Dev.D.UE.0.0.10.P22.7.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-7-thrown-weapon-edge-cue/Docs/Log/Dev.D.UE.0.0.10.P22.7.r0_log.md>
