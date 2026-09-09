# Dev.D.UE.0.0.10.P22.6.r0 Report

## 1. 结论

P22.6 在 P 阶段边界内完成，结论为 **PASS**。

本轮为 P22.5 的双段飞刀原型补上独立材质对比：刀身使用冷色浅钢蓝 `Linear RGB (0.62, 0.78, 1.00)`，握柄使用深棕 `Linear RGB (0.16, 0.045, 0.012)`。两段各自持有动态材质实例，因此共享滚转时仍保留明确的刀身/握柄分区；材质契约同时纳入既有 Stage/InFlight 失败关闭校验。

```text
Thrown-weapon World delivery focus:      6 Success / 0 Fail
Changed-file mapped regression:        262 Success / 0 Fail (6 healthy logs)
Changed-file regression coverage:      PASS (3 files / 1 rule / 6 required groups)
Regression gate self-test:              PASS 439/439
Game + Editor Development:              PASS (Game 118 actions; Editor 0 actions; both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明生产 Actor 的材质实例、精确参数、组件归属、滚转保持和生命周期门，不宣称最终美术材质、像素输出、真实地图视距或玩家手感已经人工验收。

## 2. 玩家可见结果

| 部位 | 原型颜色 | 表现意图 | 物理作用 |
|---|---|---|---|
| 刀身 | 冷色浅钢蓝 `(0.62, 0.78, 1.00)` | 从背景与深色握柄中读出长刃主体 | 无；继续无碰撞 |
| 握柄 | 深棕 `(0.16, 0.045, 0.012)` | 强化 P22.5 的窄柄轮廓与前后方向 | 无；继续无碰撞 |

颜色只作用于既有两段表现网格。根碰撞仍为 `30 × 4.5 × 1.2 cm`，刀身与握柄尺寸、位置、飞行速度、轨迹、伤害和库存提交均未改变。

## 3. 单一事实链

```text
existing immutable launch receipt
  -> existing durable Quantity publication
  -> existing projectile State becomes InFlight
  -> one shared presentation pivot shows blade + grip
  -> blade MID uses cool steel color; grip MID uses dark brown color
  -> existing built-in roll rotates both colored sections together
  -> existing contact / no-impact terminal
  -> existing projectile State becomes Spent and hides both sections
```

材质不选择轨迹、不推进动作、不修改库存、不解析命中，也不拥有第二份飞行状态。唯一生命周期权威仍是既有 Projectile State。

## 4. 实现

`Ademo_mapShanmenThrownWeaponProjectile` 在构造期为既有 `ThrownWeaponVisual` 与 `ThrownWeaponGripVisual` 分别创建一个 `UMaterialInstanceDynamic`：

- 两个实例互不共享，分别绑定到刀身与握柄的材质槽 0；
- 同时写入 Engine 原型材质常用的 `Color` 与兼容 `BaseColor` 参数；
- `HasPresentationMaterialContrast()` 检查两个实例存在、身份不同、仍绑定到正确组件，并验证两个 `Color` 参数精确等于固定原型色；
- `IsPresentationGeometryValid()` 新增材质对比契约，因此 Stage 与 InFlight 校验会在材质缺失、串槽、共用实例或颜色漂移时失败关闭；
- 材质指针为 Actor 私有 `Transient` 状态，不进入存档、资产或第二套运行时模型。

本轮未新增材质资产、纹理、Actor、Tick、Timer、Timeline 或表现包装链，继续复用 Engine Cube 和项目既有动态材质模式。

## 5. 真实 World 证明

扩展既有 `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery.DurableLaunchGate`，通过真实测试 World 生成生产 Projectile，并直接读取实际组件材质槽：

1. 刀身与握柄材质槽均为有效 `UMaterialInstanceDynamic`；
2. 两个实例身份不同，且 Actor 的材质对比契约返回 true；
3. 刀身 `Color` 精确为 `(0.62, 0.78, 1.00)`，握柄精确为 `(0.16, 0.045, 0.012)`；
4. Stage 与耐久拒绝仍隐藏两段，durable publication 后才显示并开始既有滚转；
5. 推进实际 `URotatingMovementComponent` `0.125s` 后，两段材质槽仍指向各自原实例；
6. 同一断言继续证明 Actor、根碰撞与飞行前向不受表现滚转影响。

专项自然完成 `6 Success / 0 Fail`、进程原生退出码 0。日志 268,524 bytes，SHA-256 `BC3A2E5EC2938BB9F54047D82F5494728845ECE885D685931F6A4FFE8EB88C78`。

## 6. 首轮结果与日志判读

本轮首次 Editor 构建直接使用上一阶段验证过的单并发低内存路径，完成 119 actions、原生退出码 0；没有 P22.6 源码失败或修复轮。日志 12,679 bytes，SHA-256 `B202246CED41722FDF6B8824F3608D4D85F7DF65EDD3AAD8F1823D1194149702`。

专项日志在测试命令执行前含 13 条 UE 5.8 启动期 `LogAutomationTest: Error: Condition failed`。它们紧随 Engine 自带 `UE::UnifiedErrorTest` 输出、早于 `Cmd: Automation RunTests`，并在 P22.5 既有日志中同样固定出现 13 次；不是本轮测试事件。正式测试开始后为 `6/0`，无 Fatal、Unhandled、Ensure 或 Test Fail。

## 7. 改动文件回归

3 个实现/测试路径命中既有 `ThrownWeaponWorldDelivery` 映射规则，并推导 6 个精确测试组：

| Group | Success | Bytes | SHA-256 |
|---|---:|---:|---|
| `Product.ThrownWeaponWorldDelivery` | 6 | 268,524 | `BC3A2E5EC2938BB9F54047D82F5494728845ECE885D685931F6A4FFE8EB88C78` |
| `Product.ThrownWeaponItemAdapter` | 5 | 267,146 | `856148BA530A6E68B4E6A65A372D58B4DF76D851861A026C29D1CD8867F88557` |
| `Product.CombatRunCoordinator` | 18 | 287,959 | `DF7FB44C7BF8B7F0B5511B626C7B294E305D713B6B7F517458374664A4E17F0B` |
| `Shanmen.0_0_10.Items` | 77 | 351,981 | `C090429088D18BC3F14CC5DC46E7BA5BFAFA65AE9CC9BC564EE289BDB33AEF63` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 270,505 | `A27D5FA7E86E3055461CDB00621C0FCA857F1D14D94A60693918B75CE6DDEBD1` |
| `Shanmen.0_0_10.CombatRuntime` | 146 | 411,787 | `1DB9950EF5F22A2A15D074BB18C068C159B8DF0C52300E2B76D75C021D30D76F` |

合计 `262 Success / 0 Fail`。覆盖门结果为 `PASS Changed=3 Rules=1 Required=6 Logs=6`；门日志 1,746 bytes，SHA-256 `AD9984A99B219A6249129A8576479708A33C5795E972857763979616FB49E0D6`。门禁自测保持 `439/439`，43,307 bytes，SHA-256 `32FD3F9757C796420DD350E2B7DAD96E7644B2D174625EB0D65D2F7C9D86BEE5`。

## 8. 构建、产物与静态边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development final | Succeeded / native 0 | 118 / 403.72s | `BF12B82E2892A051A75CC18DF7E7D58889F093E3A4EE4A972B76F3C2C4908BD4` |
| Editor Development final | Succeeded / native 0 | 0 / 1.19s | `20DD37B66E25D1427456E9B120664410C39253D5815E7F68D82F959BB657966F` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,566,848 bytes；SHA-256 `BBBB19D577DCF18001FDB05C3AC33C37991D63ED4CC3FEDAE85F35355814CFB8`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,772,992 bytes；SHA-256 `FDE437B7C858ECF56BD1E1A36CF874E5FFB771EF7F7325AA60A71F28728F07BB`。

实现与测试差异为 3 files / 62 insertions / 0 deletions，其中生产新增 45 行。新增生产行中的 Timer、SetTimer、自定义 Tick、PrimaryActorTick、Random、Rand、RNG、ApplyDamage、TakeDamage、SpawnActor 和 Destroy 均为 0；`git diff --check` 原生退出码为 0。

## 9. P/F 边界

PASS：生产 Projectile 的刀身与握柄拥有两个不同动态材质实例和精确原型色；错误材质状态进入既有失败关闭门；耐久发布、显隐、整体滚转、碰撞、轨迹、终态、库存与 Run 权威保持不变；映射回归、覆盖门、自测及双目标构建全部通过。

未声明：这不是最终 PBR 材质或美术资产；未验证光照、反射、法线、纹理、色觉可访问性、运动模糊、真实地图像素可读性、网络复制或玩家手感。没有执行交互式产品验收。

## 10. 提交边界与 GitHub

本阶段只提交 3 个实现/测试文件、本 Report 与本 Development Log，共 5 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P22.6` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-6-thrown-weapon-material-contrast>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-6-thrown-weapon-material-contrast/Docs/Report/Dev.D.UE.0.0.10.P22.6.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-6-thrown-weapon-material-contrast/Docs/Log/Dev.D.UE.0.0.10.P22.6.r0_log.md>
