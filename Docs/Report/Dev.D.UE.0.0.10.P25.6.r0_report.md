# Dev.D.UE.0.0.10.P25.6.r0 Report

## 1. 结论

P25.6 已为 P25.2–P25.5 完成的灵力护盾权威闭环增加真实 World 表现：
当前受控玩家 Pawn 周围会安装一个青色能量罩和同色点光提示。
表现严格读取既有 `Fdemo_mapShanmenSpiritShieldProductSession`，不保存第二份容量、
Deadline、激活身份或战斗状态。

会话有效、激活且可用容量大于 0 时显示；容量精确归零、90 Tick Deadline 关闭、
所有者释放、无效/空会话或没有 GameMode 时隐藏。表现组件不参与碰撞、重叠、导航或伤害。

## 2. 玩家可观察行为

- 护盾激活后，玩家周围显示球形青色能量罩；
- 能量罩采用 `(1.35, 1.35, 2.10)` 相对缩放，形成包围角色的纵向壳体；
- 同步显示青色局部点光，强度 1800、半径 220；
- 容量归零、到期或 Run/控制权释放后立即隐藏两项提示；
- 未激活时组件保持隐藏，不影响移动、命中、导航和阴影；
- 重复同步不会创建第二套表现组件。

## 3. 实际产品接线

核验发现 `BP_TopDownCharacter` 直接继承 `/Script/Engine.Character`，并不继承
仓库中的 `Ademo_mapCharacter`。因此本轮没有把表现留在未被该蓝图使用的角色类中，
而是由实际 GameMode 固定使用的 `Ademo_mapPlayerController` 对当前 `GetPawn()`
安装并同步表现。

PlayerController 构造时持有 Sphere Mesh 与 `M_SimpleGlow` 的硬引用；
`PlayerTick()` 在输入锁定提前返回之前执行只读同步，`OnUnPossess()` 对旧 Pawn
执行空会话同步并隐藏提示。这样覆盖实际产品 Pawn，同时不要求修改现有蓝图资产。

## 4. 单一权威边界

新增 `Fdemo_mapShanmenSpiritShieldWorldPresentation` 是无状态适配器：

- 唯一业务输入是既有 Product Session 的只读指针；
- 只读取 `IsValid()`、`IsActive()` 和 `GetAvailableCapacity()`；
- 不推进 Timeline，不提交 Impact，不扣除灵力或容量；
- 不缓存容量、Deadline、Run/Activation/Impact 身份；
- `nullptr`、无效、关闭、释放或耗尽状态全部失败关闭；
- 每次同步只投影当前权威事实，不推断过去状态。

## 5. 表现与物理隔离

适配器在实际 Pawn Root 上最多创建一次：

1. `SpiritShieldShellVisual`：Engine Sphere、动态 Glow 材质、青色参数；
2. `SpiritShieldCueLight`：同色点光。

Shell 明确设置 `NoCollision`、忽略所有通道、关闭 Overlap、关闭导航影响与投影；
Light 关闭阴影。材质保持全精度线性色，Light 按 UE 的 `FColor` 存储规则验证量化后的
线性色，避免把引擎存储精度误报为产品失败。

## 6. 新增自动化证明

精确组 `Shanmen.0_0_10.Product.SpiritShieldWorldPresentation`：

| Test | Result |
|---|---|
| `GeometryFailClosed` | Success |
| `ActivationAndRelease` | Success |
| `DeadlineClosure` | Success |
| `CapacityDepletion` | Success |

最终结果 4 Success / 0 Fail / 队列正常清空；日志 SHA-256：
`7652FD6003AF0ECE629221CB81100BC29833CFC0A1915E6A1356031C2AAD8DD1`。

证明覆盖实际 `ACharacter` Pawn、组件唯一安装、材质与灯光颜色、无碰撞几何、
激活显示、释放隐藏、90 Tick 到期隐藏和容量 30→0 后隐藏。

## 7. 改动文件驱动回归

最终 9 个提交路径中，7 个实现/测试/流程路径命中 2 条映射规则，要求 30 个精确测试组。
30 组均以独立 `RunTests` 执行，合计 329 Success / 0 Fail：

| 范围 | Groups | Success | Fail |
|---|---:|---:|---:|
| 旧 `demo_map` 输入与远程兼容 | 2 | 123 | 0 |
| 灵力护盾 World 表现 | 1 | 4 | 0 |
| 其余 0.0.10 产品链 | 27 | 202 | 0 |
| 合计 | 30 | 329 | 0 |

覆盖门结果：`PASS Changed=9 Rules=2 Required=30 Logs=30`；
证据 SHA-256：`FC2BB7DC1D227AB5D75FE3DE1030E177B57FF4EC2A067F764EB66D1733E81755`。
映射器自检 451/451 PASS；SHA-256：
`27DC9F3C3CECEB5168F19814377827F6DD7B462E96FB4523348DCDF33A8F44FF`。

## 8. 首错与有界修复

1. 初始测试尝试把 `BP_TopDownCharacter` 当作 `Ademo_mapCharacter` 子类加载，4/4
   在产品断言前失败。二进制父类核验确认蓝图直接继承 Engine Character；该错误路线被撤回，
   改为 PlayerController→当前 Pawn 的实际产品接线。首错 SHA-256：
   `2B6FB9BEF4F75C8A01222FCD0CD4E2A0980EBEC81947A67E8CAFB03BFBC8BEAE`。
2. 新适配器首次编译仅发现 UE 5.8 材质参数读取接口缺少 `const` 限定，局部兼容修正后通过；
   失败日志 SHA-256：
   `02B9AE737C978F8A0C61BC39269F7F3FC868E29046BDF4729D96342E8E1B0210`。
3. 运行时逐项断言发现 Point Light 把线性色量化为 `FColor`。测试与几何契约改为验证引擎
   实际量化值，没有放宽可见性、颜色来源或失败判据。量化失败证据 SHA-256：
   `79E74C67922A919017784758D443A13872455B8767C64C3B8712A53C9E634D3E`。

所有失败日志保留在本地 `Saved/Codex/P25.6`，最终证据仅采用 4/4 成功日志。

## 9. 构建与静态检查

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| `demo_mapEditor Win64 Development` | Succeeded | 0 | `BACFC361475FBA902754E306EC8886E324C11CE3FE8D56F762B8F4B7E9ACFE2B` |
| `demo_map Win64 Development` | Succeeded | 0 | `FCFE8B1183116F6F8E733DEC558F6996583188E42898F93EDE016268B38137B7` |

- `git diff --check`：PASS；
- Regression Map JSON：PASS（249 条规则）；
- 无状态适配器对 `Diagnostic`、`UWorld`、`GetWorld`、RNG、容量/Deadline 赋值：0 命中；
- 最终项目相关 Unreal 进程：0；
- `UnrealEditor-demo_map.dll`：19180032 bytes，SHA-256
  `72D3ACD3BE7A34FF06DCDB4F8888EFB96C3A4A1ABC6C3373F4F12BF92644A9DC`；
- `demo_map.exe`：359878144 bytes，SHA-256
  `7D4ED3A45D1515FDD22A5EE371B81A9F5A167A167AB12A54B8EB989F14402D3E`。

## 10. P/F 边界与 GitHub 交接

P 阶段已证明：实际 Pawn 接线、无状态会话投影、组件唯一安装、物理隔离、四个显隐终点、
30 组改动映射回归及 Editor/Game 双目标构建成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package，因此不声明已目视确认能量罩透明度、最终画面层次或实机手感。

基线提交：`150b71f302c4c609fcaad5c7831320796fe9ce79`（P25.5）。
分支：`agent/0.0.10-p25-6-spirit-shield-world-visual`。
只提交本阶段 7 个实现/测试/流程文件、本 Report 与本 Development Log；
用户原有 103 个未跟踪文件保持未暂存，`Saved/Codex/P25.6` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-6-spirit-shield-world-visual>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-6-spirit-shield-world-visual/Docs/Report/Dev.D.UE.0.0.10.P25.6.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-6-spirit-shield-world-visual/Docs/Log/Dev.D.UE.0.0.10.P25.6.r0_log.md>
