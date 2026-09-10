# Dev.D.UE.0.0.10.P25.6.r0 Development Log

## 1. 目标

- 把既有灵力护盾 Product Session 投影为玩家周围真实可见的 World 能量罩；
- 激活且容量大于 0 时显示，耗尽、到期、释放及无效状态时隐藏；
- 表现不得成为容量、Deadline、身份、伤害或 Timeline 的第二权威；
- 表现组件不得影响碰撞、重叠、导航、伤害或阴影；
- 按改动文件执行精确回归、双目标构建，并推送 Report 与 Log。

## 2. 基线与范围

- 基线：`150b71f302c4c609fcaad5c7831320796fe9ce79`（P25.5）；
- 分支：`agent/0.0.10-p25-6-spirit-shield-world-visual`；
- 起始 tracked tree clean；
- 用户原有 103 个未跟踪文件全程保持未暂存；
- 不修改护盾成本、容量、Deadline、防御顺序、伤害公式、资源权威或输入映射；
- 不修改地图、Pawn 蓝图、Widget、存档 Schema 或现有材质资产。

## 3. 产品对象核验与架构纠偏

最初候选接线点是 `Ademo_mapCharacter`。无头测试随后证明
`/Game/TopDown/Blueprints/BP_TopDownCharacter` 的 Generated Class 不是其子类。
对资产父类的核验确认该蓝图直接继承 `/Script/Engine.Character`。

继续修改 `Ademo_mapCharacter` 会产生“代码和测试存在、实际产品 Pawn 不使用”的伪闭环。
因此撤回该类上的全部改动，改用 GameMode 已固定采用的 `Ademo_mapPlayerController`：

1. PlayerController 始终能取得当前实际 `GetPawn()`；
2. Product Session 已由实际 `Ademo_mapGameMode` 持有；
3. 同步不依赖 Pawn 的具体 C++ 子类；
4. 无需变更蓝图、地图或第二个 Actor 层级。

最终 `demo_mapCharacter.h/.cpp` 与基线内容哈希完全相同，未进入提交。

## 4. 无状态 World 表现适配器

新增 `Fdemo_mapShanmenSpiritShieldWorldPresentation`，提供：

- `EnsureInstalled()`：在 Pawn Root 上幂等安装两项表现组件；
- `Synchronize()`：只读投影 Product Session 到组件显隐；
- `IsVisible()`、`IsGeometryValid()`：验证组合状态与几何安全；
- `HasCanonicalMaterialColor()`、`GetCueColor()`：验证固定视觉参数。

该类没有成员字段。每次同步重新读取唯一 Product Session：

```text
Session != null
&& Session.IsValid()
&& Session.IsActive()
&& Session.GetAvailableCapacity() > 0
    => Shell + Light visible
otherwise
    => Shell + Light hidden
```

不读取自由格式 Diagnostic，不访问 World 或墙钟，不生成随机数，不保存容量、Deadline 或身份。

## 5. 组件、资产与物理边界

`EnsureInstalled()` 按固定名称查找现有组件，只有两项均不存在时才创建：

- `SpiritShieldShellVisual`：`/Engine/BasicShapes/Sphere.Sphere`；
- `SpiritShieldCueLight`：局部 Point Light；
- 材质：`/Game/LevelPrototyping/Interactable/JumpPad/Assets/Materials/M_SimpleGlow`；
- 颜色：线性 `(0.08, 0.68, 1.0)`；
- Shell Scale：`(1.35, 1.35, 2.10)`；
- Light：Intensity 1800、Attenuation Radius 220。

Shell 使用 `NoCollision`、全部通道 Ignore、Overlap=false、Navigation=false、CastShadow=false；
Light 使用 CastShadows=false。两项均为 Pawn 的 transient instance component，
不创建 gameplay Actor、触发器或伤害体。

## 6. PlayerController 接线

PlayerController 构造函数通过 `ConstructorHelpers::FObjectFinder` 建立 Mesh 与材质硬引用，
确保非 Editor 构建也包含明确资产依赖。

`PlayerTick()` 在输入允许性提前返回之前同步当前 Pawn：

- World 与 GameMode 有效：读取 `GetSpiritShieldProductSession()`；
- GameMode 不可用：传入空会话并失败关闭；
- Pawn/资产尚不可用：不显示且不影响 gameplay；
- `OnUnPossess()`：在原 Pawn 脱离前用空会话隐藏提示。

控制器不持有表现会话副本，不修改 GameMode 或 Session。

## 7. 自动化实现

新增四项无头测试，使用 transient GamePreview World 与普通 `ACharacter`，避免把测试限定在
某个未被产品蓝图继承的自定义角色类：

1. `GeometryFailClosed`：组件安装、唯一性、资产、材质、颜色与无碰撞；
2. `ActivationAndRelease`：真实激活显示，所有者释放后隐藏；
3. `DeadlineClosure`：固定 Timeline 前进 3 秒/90 Tick 后关闭并隐藏；
4. `CapacityDepletion`：真实 Impact 组合与提交把容量 30→0，随后隐藏。

最终 4/4 Success、0 Fail、队列正常清空；SHA-256：
`7652FD6003AF0ECE629221CB81100BC29833CFC0A1915E6A1356031C2AAD8DD1`。

## 8. 回归映射与完整结果

新增 `SpiritShieldWorldPresentation` 路径规则，匹配：

- `demo_mapPlayerController.h/.cpp`；
- `demo_mapShanmenSpiritShieldWorldPresentation.h/.cpp`；
- `demo_mapShanmenSpiritShieldWorldPresentationTests.cpp`。

规则要求 World 表现、既有 Product Session 和旧远程兼容证据。
PlayerController 同时命中原有输入路由规则，映射器对重叠规则取并集。

自检新增正向适配器、正向规则重叠和负向缺证三类夹具，最终 451/451 PASS。
最终 9 个提交路径中，7 个实现/测试/流程路径命中 2 条规则，
共要求并独立执行 30 个精确组：

| Category | Groups | Success | Fail |
|---|---:|---:|---:|
| Legacy `demo_map` | 2 | 123 | 0 |
| Spirit Shield World | 1 | 4 | 0 |
| Other Product | 27 | 202 | 0 |
| Total | 30 | 329 | 0 |

覆盖门：`PASS Changed=9 Rules=2 Required=30 Logs=30`；
SHA-256：`FC2BB7DC1D227AB5D75FE3DE1030E177B57FF4EC2A067F764EB66D1733E81755`。
自检日志 SHA-256：
`27DC9F3C3CECEB5168F19814377827F6DD7B462E96FB4523348DCDF33A8F44FF`。

## 9. 首错证据

| Evidence | 观察 | 处理 | SHA-256 |
|---|---|---|---|
| `...WorldPresentation_attempt-1.log` | 旧蓝图不是 `Ademo_mapCharacter` 子类，4/4 在产品断言前失败 | 改接实际 PlayerController/Pawn | `2B6FB9BEF4F75C8A01222FCD0CD4E2A0980EBEC81947A67E8CAFB03BFBC8BEAE` |
| `P25.6_EditorBuild_attempt-3.log` | UE 5.8 材质读取 API 非 const | 只调整局部读取指针 | `02B9AE737C978F8A0C61BC39269F7F3FC868E29046BDF4729D96342E8E1B0210` |
| `...WorldPresentation_attempt-3.log` | Light 实际颜色按 `FColor` 量化，4 项逐项失败但无进程崩溃 | 按引擎存储契约验证量化值 | `79E74C67922A919017784758D443A13872455B8767C64C3B8712A53C9E634D3E` |

中间一次测试中的硬 `check` 使失败进程以退出码 3 终止；其后改为可报告断言，
没有把进程退出当作成功，也没有删除失败证据或降低最终 4/4 判据。

## 10. 构建、静态检查与交接

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `P25.6_EditorBuild_final.log` | Editor Succeeded | 0 | `BACFC361475FBA902754E306EC8886E324C11CE3FE8D56F762B8F4B7E9ACFE2B` |
| `P25.6_GameBuild_final.log` | Game Succeeded，33 个动作 | 0 | `FCFE8B1183116F6F8E733DEC558F6996583188E42898F93EDE016268B38137B7` |

- `git diff --check`：PASS；
- Regression Map JSON：249 条规则，PASS；
- 无状态依赖扫描：0 命中；
- 最终 Unreal 进程：0；
- Editor DLL：19180032 bytes / `72D3ACD3BE7A34FF06DCDB4F8888EFB96C3A4A1ABC6C3373F4F12BF92644A9DC`；
- Game EXE：359878144 bytes / `7D4ED3A45D1515FDD22A5EE371B81A9F5A167A167AB12A54B8EB989F14402D3E`。

P 阶段完成。F 阶段未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、
截图、Smoke、Cook 或 Package，不声明目视或实机验收。

精确提交 7 个实现/测试/流程文件、本 Report 与本 Log。
103 个用户未跟踪文件不暂存，`Saved/Codex/P25.6` 证据仅本地保留。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-6-spirit-shield-world-visual>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-6-spirit-shield-world-visual/Docs/Report/Dev.D.UE.0.0.10.P25.6.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-6-spirit-shield-world-visual/Docs/Log/Dev.D.UE.0.0.10.P25.6.r0_log.md>
