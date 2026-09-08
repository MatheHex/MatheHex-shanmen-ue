# Dev.D.UE.0.0.10.P22.0.r0 Report

## 1. 结论

P22.0 在 P 阶段边界内完成，结论为 **PASS**。

本轮让既有投掷物权威 Actor 获得可见、无碰撞的飞刀原型网格。网格在预备阶段保持隐藏，只在资源与运行权威已经耐久提交、正式发布发射之后显示；取消预备或进入终态时再次隐藏。直线与抛物线发射均由既有 `UProjectileMovementComponent` 速度控制 Actor 朝向，因此玩家能从可见载体辨认真实飞行方向，命中仍只由既有球形根碰撞体负责。

```text
Thrown-weapon World focused initial:          4 Success / 1 Fail (preserved)
Thrown-weapon World focused final:            5 Success / 0 Fail
Shanmen.0_0_10 full:                       1,247 Success / 0 Fail (natural queue empty, native 0)
Changed-file regression coverage:          PASS (3 files / 1 rule / 6 required groups)
Regression gate self-test:                 PASS 437/437
Game + Editor Development:                 PASS (Game 117 actions; Editor 0 actions; both native 0)
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。因此本轮证明可见载体生命周期、直线/抛物线初始朝向、碰撞隔离与编译闭合，不宣称最终飞刀美术、轨迹观感、镜头可读性或玩家手感已经人工验收。

## 2. 玩家可见结果

投掷物不再只有不可见的碰撞球和运动组件。既有 Actor 现在带有一个使用 Engine Cube 的细长飞刀原型：

- 原型尺寸约为 `30 × 4.5 × 1.2 cm`；
- 预备发射时隐藏，避免尚未耐久提交的动作提前出现在世界中；
- 发射正式发布后显示；
- 直线发射面向真实速度方向；
- 抛物线发射面向弧线规划器给出的初始速度方向；
- 取消预备或无命中终止后隐藏，不留下失效表现。

该网格明确禁用碰撞与重叠事件，玩家可见形状不会成为第二命中来源。

## 3. 单一事实链

本轮不创建第二套投掷运动、命中或朝向权威：

```text
existing canonical launch receipt
  + existing World hit context
  + durable launch publication
  -> existing projectile movement velocity
  -> existing bRotationFollowsVelocity actor facing
  -> collisionless prototype Visual becomes visible
```

Actor 根 `USphereComponent` 继续承担接触；`UProjectileMovementComponent` 继续承担直线或重力弧线运动；`Visual` 只呈现这些既有事实。表现层不写资源事务、Combat Run、Impact ledger 或目标生命值。

## 4. 生命周期与失败关闭

构造时加载 `/Engine/BasicShapes/Cube.Cube`，把 `ThrownWeaponVisual` 附着到根碰撞体并关闭碰撞、重叠与初始可见性。若原型网格不可用，`TryStageLaunch()` 失败关闭，避免发布一个仍不可见的载体。

`IsStagedFor()` 现在要求有效网格且表现隐藏；`IsInFlightFor()` 要求表现可见。`ActivateCommittedLaunch()` 在设置并激活既有运动之后显示网格；`CancelStagedLaunch()` 与 `MarkSpent()` 都隐藏网格。这样可见生命周期与既有投掷物状态机保持一一对应。

## 5. 测试覆盖与首次失败

本轮只强化既有 `ThrownWeaponWorldDelivery` 真实 World 测试，没有新增平行测试夹具：

- `DurableLaunchGate` 使用已有 `FThrownSpawnWorldFixture` 与 `SpawnStagedCarrier()` 在真实测试 World 生成 Actor；
- 直线路径改用非默认 `+Y` 方向，防止 Cube 默认 `+X` 朝向使断言恒真；
- 断言预备、耐久提交拒绝和终态均不可见；
- 断言正式发布后可见，速度与表现前向同时为 `+Y`；
- 抛物线路径断言表现前向等于规划器初始速度的规范化方向；
- 既有碰撞、重力、反弹、追踪和终态断言继续保留。

第一次专项运行得到 `4 Success / 1 Fail`。失败不是产品逻辑或编译错误，而是旧测试用 `NewObject` 创建未注册 Actor，非默认 `+Y` 旋转无法向子网格传播；此前默认 `+X` 路径掩盖了这个夹具缺口。修复没有削弱断言，而是改用文件中已有的真实 World 生成链。最终专项为 `5 Success / 0 Fail`，首失败日志完整保留。

## 6. 自动化证据

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| Focused initial | 4/1 | 265,856 | `E96AF3890759F305B63DE0CC1936896A8F10E25E278BF7CBAAB658B767401992` |
| Focused final | 5/0 | 264,639 | `ABB4393E8B063D83D794C8ADE0D5265214AAD0B5D6A42C32C577C2828663A5A3` |
| Full diagnostic, intentionally stopped | 712/0 (intentionally stopped) | 1,190,257 | `16F0A82807DE5A0643D115D09BF57C008053792F152541F36CACDE38350C9575` |
| Fab-disabled probe | 6/0 | 269,356 | `E04236A82A954F0F1ADEE436FBA1694980A1300FEEA9092AB8935EEE752E62BD` |
| Home Panel-disabled probe | 6/0 | 264,213 | `C2B72FE408D236ABF4887C065556F2B05AA9FCD846DA309AE643E1B4198B17F3` |
| `Shanmen.0_0_10` full final | 1,247/0 | 1,857,382 | `3A4D3BE56AF4C6CBD9CFDC96DEC7B4C7B092B1D78216865DBFB2AB42580FE4FE` |

最终全量日志必须包含 1,247 次测试派发和 1,247 个 `Result={Success}`，`Result={Fail}` 与 Fatal 均为 0，队列自然清空，进程原生退出码为 0。

诊断过程中，第一次全量运行在 712/0 时主动停止并保留，因为 UE 5.8 每隔数秒向 `google.com/generate_204` 发起 Home Panel 连通性探测，失败重试污染日志并与大帧间隔同时出现。引擎源码把该 URL 定位到 `MainFrame/Private/HomeScreen/SHomeScreen.cpp`；禁用 Fab 后探测仍存在，排除了 Fab 假设。随后使用引擎源码明确允许的进程级配置覆盖 `HomeScreen.EnableHomeScreen=0`，6 项探针中网络记录由 10 降为 0，且未写入项目、Engine 或 Windows 配置。测试自身的大帧间隔仍存在，因此本 Report 不把全部耗时误归因于网络。

## 7. 改动文件回归门

覆盖门由三个实际生产/测试路径命中既有 `ThrownWeaponWorldDelivery` 映射，要求以下组均出现在健康日志中：

- `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery`；
- `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter`；
- `Shanmen.0_0_10.Product.CombatRunCoordinator`；
- `Shanmen.0_0_10.Items`；
- `Shanmen.0_0_10.WorldGameplay`；
- `Shanmen.0_0_10.CombatRuntime`。

最终结果：`PASS Changed=3 Rules=1 Required=6 Logs=1`。门禁日志 SHA-256 为 `C21E1F98A2A725DF4B76C2A66B03CD767469F014341206D9BABA9D5BEC781F03`；门禁自测为 `PASS 437/437`，SHA-256 为 `BD30B0701682789BB228FB6E7FC062E2C9BB5D870082227B6E91D4B242966C88`。

## 8. 构建、产物与静态边界

| Target | Result | Actions / Time | Bytes | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded / native 0 | 118 / 422.60s | 12,650 | `E1259B2BF3838D9104DA922AA8F45978E3ACC20B84EF6DFFAE824B320B3C53BA` |
| Editor after fixture fix | Succeeded / native 0 | 4 / 10.49s | 2,354 | `69309AEB141BB33A18346F7D08A55E336BA6E195B0379C20C2AFAF2D71E926FD` |
| Game Development final | Succeeded / native 0 | 117 / 66.61s | 11,444 | `B6EE98ABA8AB46AA8448F865309D9BC6879BA5E2F84F20B102E96D5D2B785D83` |
| Editor Development final | Succeeded / native 0 | 0 / 1.09s | 1,036 | `C3784CC4BA60098180CAD0A8E2A5D6B5E4D1A5BA69D3414D92C90D303ACD74A2` |

最终产物：

- `Binaries/Win64/demo_map.exe`：359,537,152 bytes；SHA-256 `533ACBAB0901D09291C722570431ADC3AC32C00546B9DE160D11178B3322FC00`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,729,472 bytes；SHA-256 `FD791E0F22D9046BD24ADF7ADAD737243F1E68BE46BE126C0B17D552F9A99A8A`。

实现/测试 diff 为 3 files / 77 insertions / 9 deletions。新增生产行中 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor` 与 `Destroy` 均为 0；`git diff --check` 原生退出码 0；验证后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

## 9. P/F 边界

PASS：真实 World Actor 具有可见的无碰撞飞刀原型；表现只在耐久发射发布后显示；直线非默认 `+Y` 与抛物线初始速度方向均可验证；取消、拒绝和终态隐藏；既有球形根碰撞、运动、命中与资源/运行权威未被复制；专项、全量、覆盖门、自测及双目标编译全部通过。

未声明：Cube 是最终飞刀美术资产；真实地图中的比例、材质、轨迹、俯仰/翻滚、镜头可读性、动画、音效或玩家手感已通过人工验收。若要证明这些内容，需要另行授权实际 UI/PIE 验收。

## 10. 提交边界与 GitHub

本阶段只提交投掷物 Actor 头/实现、既有 World 测试、本 Report 与本 Development Log，共 5 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P22.0` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p22-0-visible-thrown-weapon-carrier>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-0-visible-thrown-weapon-carrier/Docs/Report/Dev.D.UE.0.0.10.P22.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p22-0-visible-thrown-weapon-carrier/Docs/Log/Dev.D.UE.0.0.10.P22.0.r0_log.md>
