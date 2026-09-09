# Dev.D.UE.0.0.10.P22.6.r0 Development Log

## 1. 基线与目标

- base：`4f7b325834c304cea856d1b126163addc66ba3ce`（P22.5 thrown-weapon silhouette）；
- branch：`agent/0.0.10-p22-6-thrown-weapon-material-contrast`；
- 目标：以独立材质色区分刀身与握柄，使 P22.5 的双段轮廓在飞行滚转时更易读取；
- 边界：不改碰撞、轨迹、速度、伤害、库存、Run、输入或终态权威，不启动 UI/PIE/产品 executable。

## 2. 审计与方案

P22.2 已有 HUD 终态反馈，P22.3 已有轨迹色飞行光，P22.4 已有沿刀身轴滚转，P22.5 已将单块长方体拆为刀身与窄柄。但两段仍继承同一默认灰色原型材质，高速滚转时只能依靠宽度差识别握柄。

选择最小直接表现改动：保留既有 Engine Cube、尺寸、附着和共享枢轴，为两段各建一个动态材质实例，使用冷色浅钢蓝与深棕形成明度/色相对比。没有引入资产制作、命中反馈包装、第二个 Actor 或新的更新循环。

## 3. 材质实现

构造期在静态网格完成赋值后创建：

```text
ThrownWeaponVisual     -> BladeMaterial MID -> Color (0.62, 0.78, 1.00)
ThrownWeaponGripVisual -> GripMaterial MID  -> Color (0.16, 0.045, 0.012)
```

两个 MID 写入 `Color` 与兼容 `BaseColor` 参数，并以私有 `Transient` 指针保存。`HasPresentationMaterialContrast()` 验证：

- 两个 MID 都存在且不是同一实例；
- 实际材质槽 0 仍指向各自 MID；
- 两个 `Color` 参数仍精确匹配固定原型色。

该检查并入 `IsPresentationGeometryValid()`；既有 Stage/InFlight 状态门因而会对缺失、串槽、共享实例或颜色漂移失败关闭。

## 4. 权威与生命周期

材质只属于 P22.5 的碰撞外表现层：

- 根碰撞继续负责 Sweep 与接触；
- ProjectileMovement 继续负责 Actor 物理位移；
- PresentationPivot 继续只负责刀身和握柄整体滚转；
- Projectile State 继续统一控制两段显隐与终态；
- durable Quantity publication 仍是从 Staged 进入 InFlight 的唯一门。

本轮不读取输入、不发动作、不改库存、不结算伤害，也不增加任何持久化字段。

## 5. 测试扩展

在既有真实 World `DurableLaunchGate` 中直接读取生产 Actor：

- 从两个真实静态网格的材质槽 0 转换出 MID；
- 断言实例存在、互异、颜色精确、Actor 契约成立；
- 保留双段尺寸、附着、无碰撞、Stage 隐藏、durable publish 显示和滚转断言；
- 手动推进实际内置滚转组件 `0.125s` 后，断言两个材质槽仍保留原 MID；
- 同时证明 Actor/Collision 四元数与飞行 Forward 不变。

首个专项即完成 `6/0`，无需放宽断言或修改生产路径。

## 6. 初始构建与日志判读

从第一轮开始使用 `MaxParallelActions=1 -NoUBA` 的受控内存路径：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.6_editor_build_initial.log` | 119 actions / PASS / native 0 | 12,679 | `B202246CED41722FDF6B8824F3608D4D85F7DF65EDD3AAD8F1823D1194149702` |
| `Focused-ThrownWeaponWorldDelivery.log` | 6/0 / native process 0 | 268,524 | `BC3A2E5EC2938BB9F54047D82F5494728845ECE885D685931F6A4FFE8EB88C78` |

专项日志中的 13 条通用 `Condition failed` 位于 Engine 初始化期、早于 `Automation RunTests`，紧随 UE 自带 UnifiedError 测试输出。P22.5 多份基线日志具有相同 13 条；正式测试事件为 6 Success、0 Fail，且无 Fatal/Unhandled/Ensure。

## 7. 改动文件映射回归

3 个改动路径由既有 1 条规则推导 6 个精确组：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 6 | 268,524 | `BC3A2E5EC2938BB9F54047D82F5494728845ECE885D685931F6A4FFE8EB88C78` |
| `Product.ThrownWeaponItemAdapter` | 5 | 267,146 | `856148BA530A6E68B4E6A65A372D58B4DF76D851861A026C29D1CD8867F88557` |
| `Product.CombatRunCoordinator` | 18 | 287,959 | `DF7FB44C7BF8B7F0B5511B626C7B294E305D713B6B7F517458374664A4E17F0B` |
| `Shanmen.0_0_10.Items` | 77 | 351,981 | `C090429088D18BC3F14CC5DC46E7BA5BFAFA65AE9CC9BC564EE289BDB33AEF63` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 270,505 | `A27D5FA7E86E3055461CDB00621C0FCA857F1D14D94A60693918B75CE6DDEBD1` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 411,787 | `1DB9950EF5F22A2A15D074BB18C068C159B8DF0C52300E2B76D75C021D30D76F` |

合计 `262/0`。六份证据均为一个精确测试命令、零 Fail、零 Fatal/Unhandled/Ensure，并自然到达 UE 5.8 成功终止标记。

## 8. 覆盖门、构建与产物

覆盖门：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=6 Logs=6
```

- gate：1,746 bytes，SHA-256 `AD9984A99B219A6249129A8576479708A33C5795E972857763979616FB49E0D6`；
- self-test：`439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P22.6_game_build_final.log` | 118 actions / PASS / native 0 | 12,460 | `BF12B82E2892A051A75CC18DF7E7D58889F093E3A4EE4A972B76F3C2C4908BD4` |
| `P22.6_editor_build_final.log` | 0 actions / PASS / native 0 | 1,035 | `20DD37B66E25D1427456E9B120664410C39253D5815E7F68D82F959BB657966F` |

`demo_map.exe` 为 359,566,848 bytes，SHA-256 `BBBB19D577DCF18001FDB05C3AC33C37991D63ED4CC3FEDAE85F35355814CFB8`。`UnrealEditor-demo_map.dll` 为 18,772,992 bytes，SHA-256 `FDE437B7C858ECF56BD1E1A36CF874E5FFB771EF7F7325AA60A71F28728F07BB`。

## 9. 静态边界

实现/测试 diff 为 3 files / 62 insertions / 0 deletions；生产新增 45 行。新增生产行中 Timer、SetTimer、自定义 Tick、PrimaryActorTick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor、Destroy 均为 0。没有新增资产或模块依赖，`git diff --check` 为 0。

## 10. 提交边界与 GitHub

精确提交 3 个实现/测试文件与本 Report/Log。103 个用户原有 untracked 文件不暂存；所有 raw evidence 留在 `Saved/Codex/P22.6` 且不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-6-thrown-weapon-material-contrast>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-6-thrown-weapon-material-contrast/Docs/Report/Dev.D.UE.0.0.10.P22.6.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-6-thrown-weapon-material-contrast/Docs/Log/Dev.D.UE.0.0.10.P22.6.r0_log.md>
