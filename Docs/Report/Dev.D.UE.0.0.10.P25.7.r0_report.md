# Dev.D.UE.0.0.10.P25.7.r0 Report

## 1. 结论

P25.7 已在 P25.6 的灵力护盾 World 表现上增加容量可读状态：护盾有效且容量高于
25% 时保持青色能量罩与 1800 强度点光；容量大于 0 且降至 25% 及以下时，能量罩与
点光切换为琥珀色，点光强度提高至 2600。容量归零仍沿用既有权威规则立即隐藏，
不会用“低容量外观”掩盖已耗尽状态。

本轮只扩展无状态表现适配器及其测试，没有新增计时器、容量副本、战斗分支、Actor、
输入映射或第二套资源权威。

## 2. 玩家可观察行为

- 护盾处于正常容量时：青色能量罩、青色点光、强度 1800；
- 护盾剩余容量不高于最大容量的 25% 且仍大于 0 时：琥珀色能量罩、琥珀色点光、
  强度 2600；
- 护盾容量归零、会话关闭、到期、释放或无效时：能量罩与点光隐藏；
- 表现变化不改变伤害、吸收量、持续时间、碰撞、导航或输入结果。

## 3. 实现方式

`Fdemo_mapShanmenSpiritShieldWorldPresentation::Synchronize()` 继续以现有
`Fdemo_mapShanmenSpiritShieldProductSession` 为唯一业务输入。显示前读取：

```text
available = Session.GetAvailableCapacity()
maximum = Session.GetSession().GetCapacityAuthority().GetMaximumCapacity()

visible = Session valid && active && available > 0
low = visible && available <= maximum * 0.25
```

最大容量无效、非正数或可用容量大于最大容量时失败关闭。每次同步直接把当前权威事实
投影到既有动态材质和点光，不缓存过去状态。

## 4. 单一权威与物理边界

- 最大容量和可用容量只从既有 Product Session/Capacity Authority 读取；
- 不写入容量、Deadline、Timeline、Run、Activation 或 Impact 身份；
- 不调用伤害结算，不提交 Impact，不扣除灵力；
- 不新增 Tick、Timer、随机数、自由格式 Diagnostic 或 World 查询；
- 延续 P25.6 的 `NoCollision`、Overlap=false、Navigation=false、CastShadow=false；
- 零容量优先判定为隐藏，不进入琥珀色分支。

## 5. 新增自动化证明

精确组 `Shanmen.0_0_10.Product.SpiritShieldWorldPresentation`：

| Test | Result |
|---|---|
| `GeometryFailClosed` | Success |
| `ActivationAndRelease` | Success |
| `DeadlineClosure` | Success |
| `CapacityTone` | Success |
| `CapacityDepletion` | Success |

新增 `CapacityTone` 使用真实防御组合、Resolver 与 Commit 路径，使容量从 30 吸收 24 后
精确剩余 6；随后证明护盾仍显示、外观切换为琥珀色且点光强度为 2600。

最终 5 Success / 0 Fail / 队列正常清空；日志 SHA-256：
`684D3FB37152310901806F89C0DC250DC1BFBEEBBABC1295A8F71EF01F8C7F47`。

## 6. 改动文件驱动回归

本阶段 5 个提交路径中，3 个实现/测试路径命中 P25.6 已建立的 World Presentation 映射；
Report 与 Log 属于忽略路径，不需要产品测试。映射要求并独立执行 3 个精确组：

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SpiritShieldWorldPresentation` | 5 | 0 | `684D3FB3...F8C7F47` |
| `Shanmen.0_0_10.Product.SpiritShieldProductSession` | 9 | 0 | `73A95DEA...FA13ED` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `072E4DF8...A2B05F` |
| **合计** | **36** | **0** | — |

覆盖门结果：`PASS Changed=5 Rules=1 Required=3 Logs=3`；证据 SHA-256：
`DE9BDB5AA0B3C8B6F79005ED88FD848E1BF66AD396BA063ED79F732AAB2A190C`。
映射器自检 451/451 PASS；SHA-256：
`27DC9F3C3CECEB5168F19814377827F6DD7B462E96FB4523348DCDF33A8F44FF`。

## 7. 构建与静态检查

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| `demo_mapEditor Win64 Development` | Succeeded | 0 | `E119B5151C651E9BD252E02F3370B1ACFD6E7CFF3ED9A37C3F00FB76B85B33EC` |
| `demo_map Win64 Development` | Succeeded | 0 | `B99156D66C274EE0B2B0CD633A864E063D602C4B45B9E4690A87E4E508002FE1` |

- 首次 Editor 编译一次通过；日志 SHA-256：
  `96F6D8B8164A87716A4CB674C2F7920B0A8C975866FA8DBDB5436CC95B1979D7`；
- `git diff --check`：PASS；
- Regression Map JSON：schema 1 / 249 条规则，PASS；
- 适配器对 `Diagnostic`、`UWorld`、`GetWorld`、RNG、Deadline/容量赋值：0 命中；
- 最终项目相关 Unreal 进程：0；
- `UnrealEditor-demo_map.dll`：19185664 bytes，SHA-256
  `7EEED8D794175E66B7ABD547A14E4BD5651F95F0A232D5CBCD7CA29B390E691D`；
- `demo_map.exe`：359882752 bytes，SHA-256
  `E163938FFA8C67C10ED465A8C8623E4D701710AED9CFF813FF67229918947036`。

## 8. 首错与修复

本阶段首次 Editor 编译、首次聚焦测试、两组首次映射回归及最终双目标构建均一次通过，
没有产品失败或源码失败需要修复。UE 启动期自带的 UnifiedError 测试输出未进入本阶段测试
结果；本阶段 36 项目标测试全部为 Success，且没有 Fail、Fatal、Unhandled Exception 或
Ensure condition failed 标记。

## 9. P/F 边界

P 阶段已证明：容量阈值投影、真实吸收后的低容量外观、零容量隐藏、既有 Session 兼容、
旧远程战斗兼容、覆盖门与 Editor/Game 双目标构建成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package，因此不声明琥珀色最终画面、亮度舒适度或实机辨识度已经目视验收。

## 10. GitHub 交接

基线提交：`f3f353724cbd495bc2950dfb02834eefb33b4430`（P25.6）。
分支：`agent/0.0.10-p25-7-spirit-shield-capacity-visual`。
只提交本阶段 3 个实现/测试文件、本 Report 与本 Development Log；用户原有 103 个未跟踪
文件保持未暂存，`Saved/Codex/P25.7` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-7-spirit-shield-capacity-visual>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-7-spirit-shield-capacity-visual/Docs/Report/Dev.D.UE.0.0.10.P25.7.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-7-spirit-shield-capacity-visual/Docs/Log/Dev.D.UE.0.0.10.P25.7.r0_log.md>
