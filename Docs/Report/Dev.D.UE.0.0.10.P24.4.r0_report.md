# Dev.D.UE.0.0.10.P24.4.r0 Report

## 1. 结论

P24.4 在 P 阶段边界内完成，结论为 **PASS**。

本轮关闭了一条直接影响产品可读性的剑气缺口：P24.3 的真实剑气投射物已经具备物理输入、飞行、碰撞、伤害和终态反馈，但世界 Actor 只有不可见碰撞球。现在同一个 `Ademo_mapShanmenSwordQiProjectile` 在正式发布飞行时显示一枚青蓝色能量刃和附着光源；空载、拒绝、暂存、取消、耗散及中断状态均保持隐藏。

```text
Sword Qi world delivery:                3 Success / 0 Fail
Sword Qi physical input:                5 Success / 0 Fail
Sword Qi complete product family:      38 Success / 0 Fail
Complete Shanmen.0_0_10 regression: 1,277 Success / 0 Fail
Complete demo_map regression:       1,330 Success / 0 Fail
Changed-file regression coverage:      PASS (4 paths / 2 rules / 25 groups)
Regression gate self-test:              PASS 442/442
Game + Editor Development:              PASS (both native 0)
git diff --check:                        PASS
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实键盘、截图、Smoke、Cook 或 Package。本轮证明最终编译代码中的表现组件、颜色、可见性生命周期和真实产品生成链，不声明实际画面中的尺寸、亮度、材质观感、拖影、帧节奏或手感已经人工验收。

## 2. 缺口与责任归属

原始 0.0.10 规划明确剑法可延伸为一定距离的剑气攻击。P24.0–P24.3 已把该攻击贯通到物理 `B`、真实 World Actor、M01 生命值和 HUD 终态，但 `Ademo_mapShanmenSwordQiProjectile` 仅包含 `USphereComponent` 与 `UProjectileMovementComponent`，玩家在实际场景中无法从载体本身识别剑气。

本轮没有新增第二套 Projectile、Manager、Subsystem、Session、RunHost、WorldDelivery 或 HUD 通道。表现责任直接归属现有投射物 Actor，生命周期仍由既有 WorldAdapter 和 RunHost 发布、终止。

## 3. 可见能量载体

现有投射物新增两个无权威表现组件：

| 组件 | 产品职责 | 权威边界 |
|---|---|---|
| `SwordQiEnergyBladeVisual` | 72 × 12 × 3 cm 青蓝色原型能量刃 | 使用 Engine 内置 Cube；无碰撞、无 overlap |
| `SwordQiFlightCueLight` | 2200 强度、170 cm 衰减半径的青蓝色可读性光源 | 不投射阴影；不参与命中或数值 |

能量刃复用 `/Engine/BasicShapes/Cube.Cube`，通过独立动态材质写入 `(0.22, 0.82, 1.0)` 的 `Color` / `BaseColor`。仓库没有增加 `.uasset`、Niagara 系统或外部素材。

碰撞球半径、碰撞响应、直线速度、零重力、最大射程和伤害公式均未改变。表现几何不会扩大或替代命中体积。

## 4. 生命周期约束

可见性严格绑定已有状态机：

| Projectile 状态/路径 | 能量刃 | 光源 |
|---|---|---|
| `Empty` | 隐藏 | 隐藏 |
| 非法来源被拒绝 | 隐藏 | 隐藏 |
| `Staged` | 隐藏 | 隐藏 |
| `InFlight` | 可见 | 可见 |
| `CancelStagedLaunch` | 隐藏 | 隐藏 |
| `Dissipated` / Impact / Range / Interrupt | 隐藏 | 隐藏 |

暂存仍是碰撞与表现双重惰性。只有 WorldAdapter 完成 copy-on-write 发布后，运动、碰撞和表现才一起开放；终态则先关闭碰撞、运动与表现，再由既有 Host 退役。

表现有效性不反向成为玩法权威：WorldAdapter 的命中、飞行与幂等判定没有依赖材质或光源，因此表现资源异常不会改变伤害规则。

## 5. 产品链证明

`LaunchGateAndFlight` 现在验证：

- Actor 确实拥有一枚附着到既有碰撞根的静态网格；
- 网格有内置几何、精确原型尺寸、无碰撞、无 overlap；
- 动态材质持有精确青蓝色；
- 空载、来源拒绝和暂存阶段均不可见；
- 发布后运动、QueryOnly 碰撞、能量刃和光源同时激活；
- `FinishFlight` 后二者同时隐藏。

接触、超距与中断测试也分别保护“无效接触保持飞行可见”和“任何终态关闭表现”的反向边界。

物理输入用例进一步从真实 `GameMode` / ProductController Session 取得 World 中生成的首发、冻结 Retry 和新 Issue 投射物，确认三条正式产品路径均出现可见能量刃与光源，而非仅在纯值测试夹具中成立。

## 6. 聚焦验证

| Group | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordQiWorldDelivery` | 3 | 0 | 264,786 | `88699459306977166719F180CE4864ED5AD155753F7AA1A5A02B5459E2C05BF7` |
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 | 0 | 269,727 | `B16735E2CCE49A3B50990845DFE868FA2D3F48814B4476122FBDBB8A04BECFF2` |
| `Shanmen.0_0_10.Product.SwordQi` | 38 | 0 | 306,742 | `E164C8B947DAC110BB79F4030740E332D66A5C27E3305F47B9CD31DE8EC05EBC` |

三组均自然达到 queue-empty，原生退出码 0；Fatal、Unhandled、Assertion 与 Ensure 均为 0。

## 7. 完整回归与改动映射

| Evidence | Success | Fail | Test window | Bytes | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10` | 1,277 | 0 | 约 61m50s | 1,943,426 | `345F50E8DD02DB5D1C8B6A7D61B70EB93B33AC4FBE136050ADE1526E1E7C2FE6` |
| `demo_map` | 1,330 | 0 | 约 37s | 1,654,781 | `B422087449759BAC32B66B770E4C025FB9CBFDCB76B4BCCFC84B1EB899FA6AA7` |

两棵完整测试树合计 2,607/0，均由单一 UnrealEditor-Cmd 实例自然清空，没有重启、拼接或删改失败行。

按 4 个改动路径推导的覆盖结果为 `PASS Changed=4 Rules=2 Required=25 Logs=2`。覆盖日志 SHA-256 为 `4696E28D3350E79516C1BB110FF9ACF8AB37E50D69389B1B9BA68FDC1F8E28C8`；门禁自检为 442/442，日志 SHA-256 为 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建与静态边界

| Evidence | Result | Actions | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P24.4_EditorBuild_initial.log` | PASS / native 0 | 62 | 6,208 | `2F64808332131B0ED291902C9445AED830251AF074B99DF990583331DDA97C7D` |
| `P24.4_EditorBuild_product-path.log` | PASS / native 0 | 4 | 2,396 | `371549F0997F0E7DF0F79AD37C942A86E0A866092B356BB367C6FF91A4AB061A` |
| `P24.4_EditorBuild_final.log` | PASS / native 0 | 0 / up to date | 1,021 | `F9E614EAD9F47FA6772B937FC9D2B4EAFC25468B33F0C8160269E9BA3B67F770` |
| `P24.4_GameBuild_final.log` | PASS / native 0 | 61 | 6,039 | `C27537D8CF34AFC890514DEF12D89D511A96E4329D75E9B701785E72A0E9A64E` |

最终 `demo_map.exe` 为 359,698,432 bytes，SHA-256 `35E26E0522ED54A22CCB7FB3BB77C668A697EA314ED894EC1A552B9DBD8EBAA0`；`UnrealEditor-demo_map.dll` 为 18,964,480 bytes，SHA-256 `AE5357EBA9898691DC5F5587CF446128E7A25920EEDA2D7784E11A14BC3A615F`。

非文档增量为 4 个文件、`+224/-10`；`git diff --check` 原生退出码 0。新增行对 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor`、`Destroy`、Manager 和 Subsystem 的命中均为 0。没有新增 UCLASS/USTRUCT、Actor、输入动作、存档字段或仓库资产。验证结束后项目 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

## 9. P/F 边界

PASS：最终编译代码拥有可见剑气几何和飞行光源；空载/拒绝/暂存隐藏；发布显示；无效接触维持显示；命中、超距和中断关闭显示；正式物理输入的首发、Retry 和新 Issue 均走同一表现 Actor；聚焦 46/0；完整回归 2,607/0；改动映射、自检和双目标构建通过。

未声明：真实屏幕可读性、摄像机距离、72 × 12 × 3 cm 比例、2200 强度、170 cm 半径、色彩、材质质感、拖影、音效、碰撞观感或战斗手感已通过人工验收。以上仍属于正式 F 阶段；P 阶段没有以截图或主观描述冒充证据。

## 10. 提交边界与 GitHub

基线提交为 `3c6a3313aad0f2b73ee474822cf8471961548c72`，工作分支为 `agent/0.0.10-p24-4-sword-qi-visible-carrier`。本阶段只提交 2 个生产文件、2 个测试文件、本 Report 与本 Development Log。用户原有 103 个未跟踪文件保持未暂存；`Saved/Codex/P24.4` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-4-sword-qi-visible-carrier>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-4-sword-qi-visible-carrier/Docs/Report/Dev.D.UE.0.0.10.P24.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-4-sword-qi-visible-carrier/Docs/Log/Dev.D.UE.0.0.10.P24.4.r0_log.md>
