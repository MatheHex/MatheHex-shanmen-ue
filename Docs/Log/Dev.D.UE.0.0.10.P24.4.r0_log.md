# Dev.D.UE.0.0.10.P24.4.r0 Development Log

## 1. 目标

- 从 P24.3 的物理输入、伤害和终态闭环继续审计剑气在真实 World 中是否可被玩家看见；
- 在现有 `Ademo_mapShanmenSwordQiProjectile` 上提供最小、无新仓库资产的可见能量载体；
- 把表现可见性绑定到既有 Empty / Staged / InFlight / Dissipated 生命周期；
- 保持碰撞、运动、伤害、幂等、Session、RunHost 与 HUD 权威不变；
- 按改动文件映射完成聚焦测试、完整回归、双构建、Report 与 GitHub 交接。

## 2. 基线与分支

- 基线提交：`3c6a3313aad0f2b73ee474822cf8471961548c72`；
- 工作分支：`agent/0.0.10-p24-4-sword-qi-visible-carrier`；
- 开始时 tracked tree clean，保留用户 103 个未跟踪文件；
- P24.3 已修复 Terminal 自动退役，但其 P/F 边界仍明确列出“特效/动画未验收”；
- 原始战斗规划确认剑法可延伸为一定距离的剑气攻击，因此优先补载体可见性，而非继续增加协调封装。

## 3. 现状审计与决策

全仓审计确认 Sword Qi 已有且只需要一条正式产品链：Physical Input → Command Owner → ProductController → Session → RunHost → WorldAdapter → Projectile → Impact/Vitality → Terminal/HUD。

缺口位于链路末端的现有 Projectile：构造函数只有不可见 `USphereComponent` 和 `UProjectileMovementComponent`。这会让“世界中确实发射了剑气”只能由测试、日志或终态文案证明，玩家无法看到飞行载体。

决策为复用同一 Actor，增加一个碰撞惰性的 Engine Cube 原型和一个附着光源。不创建第二个 Actor、特效 Manager、Niagara 资产、材质资产或新产品 Host。视觉失效也不参与 WorldAdapter 的玩法有效性判断。

## 4. 生产实现

### 4.1 能量刃

`SwordQiEnergyBladeVisual` 附着于既有 `SwordQiCollision`：

- 使用 `/Engine/BasicShapes/Cube.Cube`；
- 原型尺寸 72 × 12 × 3 cm；
- `NoCollision`，不生成 overlap；
- 动态材质写入青蓝色 `(0.22, 0.82, 1.0)`；
- 不新增仓库 `.uasset`。

### 4.2 飞行可读性光源

`SwordQiFlightCueLight` 同样附着到碰撞根：

- 颜色与能量刃一致；
- 强度 2200；
- 衰减半径 170 cm；
- 不投射阴影；
- 不参与碰撞、伤害或任何权威分支。

### 4.3 状态机绑定

新增 `SetPresentationActive()` 作为 Actor 内部唯一开关：构造、暂存、取消和耗散均关闭；`ActivateStagedLaunch()` 在发布为 InFlight 后开启。`IsPresentationVisible()`、`HasPresentationMaterialColor()`、`IsFlightCueVisible()` 与 `GetFlightCueColor()` 提供只读产品/自动化观察面。

没有把表现有效性加入 `IsInFlightFor()` 或 WorldAdapter 的 `FlightMatches()`；表现资源不会反向决定动作或伤害是否有效。

## 5. 测试升级

`SwordQiWorldDelivery.LaunchGateAndFlight` 增加以下断言：

- 组件名称、附着关系、Engine mesh、精确缩放、无碰撞与无 overlap；
- 动态材质及精确颜色；
- Empty、来源拒绝、Staged 均隐藏；
- InFlight 同时显示能量刃和光源；
- 光源持有精确颜色；
- FinishFlight 同时关闭二者。

`ContactToVitalityAndReplay` 与 `FailClosedRangeAndTermination` 增加反向生命周期断言：未注册接触仍保持可见飞行，Impact、RangeExpired 与 Interrupted 全部关闭表现。

`SwordQiPhysicalInput.IssueRetryAndLock` 从真实 ProductController Session 获取 World 中生成的首发、冻结 Retry 和新 Issue 投射物，逐一确认正式产品路径的能量刃与光源可见。终态继续由 P24.3 的 GameMode Tick 自动退役，不新增测试专用完成通道。

## 6. 首次构建与聚焦结果

生产实现首次 Editor 构建为 62 actions、Result Succeeded、native 0。首次 WorldDelivery 测试即为 3/0。随后增加真实物理输入链的表现断言，增量 Editor 构建为 4 actions、native 0，PhysicalInput 为 5/0，完整 SwordQi 家族为 38/0。

| Evidence | Result | Bytes | SHA-256 |
|---|---:|---:|---|
| `P24.4_EditorBuild_initial.log` | PASS / native 0 | 6,208 | `2F64808332131B0ED291902C9445AED830251AF074B99DF990583331DDA97C7D` |
| `P24.4_SwordQiWorldDelivery_initial.log` | 3/0 / native 0 | 264,786 | `88699459306977166719F180CE4864ED5AD155753F7AA1A5A02B5459E2C05BF7` |
| `P24.4_EditorBuild_product-path.log` | PASS / native 0 | 2,396 | `371549F0997F0E7DF0F79AD37C942A86E0A866092B356BB367C6FF91A4AB061A` |
| `P24.4_SwordQiPhysicalInput_initial.log` | 5/0 / native 0 | 269,727 | `B16735E2CCE49A3B50990845DFE868FA2D3F48814B4476122FBDBB8A04BECFF2` |
| `P24.4_SwordQi_initial.log` | 38/0 / native 0 | 306,742 | `E164C8B947DAC110BB79F4030740E332D66A5C27E3305F47B9CD31DE8EC05EBC` |

没有源码构建失败、自动化 Fail、Fatal、Unhandled、Assertion 或 Ensure 需要隐藏或覆盖。

## 7. 改动映射与完整回归

在完整回归前先用三份聚焦日志运行覆盖器，覆盖器准确拒绝证据并列出未覆盖的广域依赖。这是预期的 preflight，不被计作产品失败或 PASS，也没有通过删规则绕过。

随后运行两棵完整测试树：

| Group | Success | Fail | Test window (UTC) | Bytes | SHA-256 |
|---|---:|---:|---|---:|---|
| `Shanmen.0_0_10` | 1,277 | 0 | 04:33:08.658–05:34:58.822 | 1,943,426 | `345F50E8DD02DB5D1C8B6A7D61B70EB93B33AC4FBE136050ADE1526E1E7C2FE6` |
| `demo_map` | 1,330 | 0 | 05:35:37.567–05:36:14.601 | 1,654,781 | `B422087449759BAC32B66B770E4C025FB9CBFDCB76B4BCCFC84B1EB899FA6AA7` |

合计 2,607/0。两份日志均由单一进程自然 queue-empty，原生退出码 0。

四个精确改动路径命中 2 条规则、25 个必跑组；最终结果为 `REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=25 Logs=2`。覆盖日志为 3,210 bytes、SHA-256 `4696E28D3350E79516C1BB110FF9ACF8AB37E50D69389B1B9BA68FDC1F8E28C8`。

覆盖器自检为 442/442；日志 43,595 bytes、SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 最终构建

| Target | Result | Native exit | Actions | SHA-256 |
|---|---|---:|---:|---|
| `demo_mapEditor Win64 Development`（final） | Succeeded / up to date | 0 | 0 | `F9E614EAD9F47FA6772B937FC9D2B4EAFC25468B33F0C8160269E9BA3B67F770` |
| `demo_map Win64 Development`（final） | Succeeded | 0 | 61 | `C27537D8CF34AFC890514DEF12D89D511A96E4329D75E9B701785E72A0E9A64E` |

最终二进制：

- `demo_map.exe`：359,698,432 bytes，SHA-256 `35E26E0522ED54A22CCB7FB3BB77C668A697EA314ED894EC1A552B9DBD8EBAA0`；
- `UnrealEditor-demo_map.dll`：18,964,480 bytes，SHA-256 `AE5357EBA9898691DC5F5587CF446128E7A25920EEDA2D7784E11A14BC3A615F`。

## 9. 静态与 P/F 边界

- 非文档增量：4 files、`+224/-10`；
- `git diff --check`：native 0；
- 新增行中的 Timer、SetTimer、RNG、ApplyDamage、SpawnActor、Destroy、Manager、Subsystem：全部 0；
- 新增 UCLASS/USTRUCT、Actor、输入动作、存档字段、仓库资产：0；
- 验证结束后项目 UnrealEditor、UnrealEditor-Cmd、demo_map 进程：0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P 阶段证明最终代码具备可见组件及确定生命周期。真实屏幕中的比例、亮度、材质观感、拖影、音效、碰撞感和手感仍由 F 阶段验收。

## 10. 提交与后续

精确提交 6 个文件：

- `Source/demo_map/demo_mapShanmenSwordQiProjectile.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiProjectile.h`；
- `Source/demo_map/demo_mapShanmenSwordQiWorldAdapterTests.cpp`；
- `Source/demo_map/demo_mapShanmenSwordQiPhysicalInputTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P24.4.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P24.4.r0_log.md`。

用户 103 个未跟踪文件保持未暂存，原始验证日志保持本地忽略。后续优先进入 Sword Qi 的正式 F 阶段可读性/手感验收，或继续在现有产品链上增加明确玩法，不复制 Projectile、Session、RunHost、WorldDelivery 或 HUD。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p24-4-sword-qi-visible-carrier>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-4-sword-qi-visible-carrier/Docs/Report/Dev.D.UE.0.0.10.P24.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p24-4-sword-qi-visible-carrier/Docs/Log/Dev.D.UE.0.0.10.P24.4.r0_log.md>
