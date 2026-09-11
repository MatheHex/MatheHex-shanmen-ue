# Dev.D.UE.0.0.10.P27.4.r0 Report

## 1. 结论

P27.4 已在 P27.2/P27.3 的唯一阵法 Run 生命周期之上建立无状态、设备无关的输入适配层。
调用方每次只提供一个稳定输入事件身份和一个不可变样本；适配器为阵法启动与锚点投放分别派生
确定性 `IntentId` / `AttemptId`，并最多一次委托现有生命周期入口。

适配器不持有按键、UI、World、Actor、GameMode、物品、序列、重试或产品状态。精确输入重放继续
由既有 Controller/Lifecycle/Host 权威复用回执；相同事件身份携带变化后的阵图空间载荷会失败关闭，
不会再次消耗阵法激活序列。

## 2. 阶段问题与范围

P27.3 已提供设备无关的 `FormationIntent` 与 `FormationAnchorOperation`，但未来物理输入调用方仍需自行
决定身份命名、样本规范化及前置拒绝顺序。这会让不同按键或设备入口生成不一致身份，或在输入已被
Gameplay/生命周期门阻断时仍读取空间状态。

P27.4 新增：

- 启动样本：冻结有效阵图、有限 Origin 与归一化水平 Forward；
- 锚点样本：冻结非空 AnchorDefinitionId；
- 启动/锚点各自独立的确定性事件身份命名空间；
- 显式 `SampleCount` 与 `LifecycleInvocationCount` 审计；
- Gameplay、路由、Run、事件、样本、不可变契约、生命周期的顺序失败状态；
- 3 项真实生命周期测试和对应改动驱动回归映射。

## 3. 输入采样契约

`Fdemo_mapShanmenFormationStartInputSample::TryCapture()` 只接受有效阵图、有限 Origin 和非零有限
Forward。Forward 在采样边界投影到 XY 平面并归一化，之后以私有字段只读暴露，避免下游再次解释
设备或相机的 Z 分量。

`Fdemo_mapShanmenFormationAnchorInputSample::TryCapture()` 只冻结一个非空锚点定义身份。两个样本均
由调用方回调一次性产生；任何 Gameplay、路由、Run 或事件身份前置失败都发生在回调前。

## 4. 确定性身份与唯一委托

启动身份使用命名空间 `demo_map.Formation.StartInputIntent.r1`，锚点尝试身份使用
`demo_map.Formation.AnchorInputAttempt.r1`。两者都由 `RunId + InputEventId` 规范派生，因此：

- 同 Run、同事件稳定得到同一身份；
- 改变事件身份会得到不同身份；
- 启动与锚点即使复用同一事件 GUID 也不会碰撞；
- 无效 Run 或事件身份不会产生派生 GUID。

通过全部前置门后，适配器恰好采样一次、构造一次既有不可变 Intent/Operation，并恰好调用一次现有
生命周期回调。适配器自身不缓存重放结果，也不建立第二本 ledger。

## 5. 失败关闭与审计不变量

启动与锚点结果都要求明确状态、非空诊断及 0..1 的采样/委托计数。结果 `IsValid()` 会交叉验证：

- 前置拒绝没有样本、派生身份、契约或生命周期成功证据；
- 样本拒绝只记录一次采样，不调用生命周期；
- 已委托结果必须重新派生并匹配确定性身份；
- 启动 Intent 必须与冻结样本完全匹配；
- 锚点 Operation 必须与 Run、锚点和 AttemptId 完全匹配；
- Applied 必须同时拥有下游成功证据及一致身份。

下游拒绝保留原始 Controller 或 AnchorOperation 结果；若下游未提供诊断，适配器补充稳定的失败诊断，
不伪造成功。

## 6. 正反测试

新增 3 项 `Shanmen.0_0_10.Product.FormationInputAdapter` 测试：

- `DeterministicIdentityAndPreflight`：验证跨命名空间确定性、无效身份、Gameplay/路由门、拒绝前不采样、
  无效样本不委托，以及每个有效调用最多一次采样/一次生命周期调用；
- `StartReplayAndConflict`：通过真实物品权威、Combat Run 与 Lifecycle 启动阵图；精确事件重放复用
  Controller 回执且只消耗一个激活序列；同一事件改变 Origin 后返回 `IntentIdConflict`；
- `AnchorReplayAndLifecycleFence`：通过适配器完成两个真实锚点、精确重放保持物品快照与世界 receipt、
  第二锚点激活阵图、正常 End 移除两个 Actor；Run 结束后的有效样本仍由生命周期返回
  `LifecycleInactive`。

既有 `FormationRunLifecycle` 4 项独立重跑，确认 P27.2/P27.3 的结束顺序、Coordinator 恢复、锚点
操作重放和 teardown 隔离未改变。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationInputAdapter` | 3 | 0 | `F1531489...03B6A660` |
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 4 | 0 | `74FB3A93...02F8E576` |
| `Shanmen.0_0_10` | 1,331 | 0 | `72B31DCB...44DE0FD4` |
| `demo_map.V3.Attributes` | 4 | 0 | `ADB2818B...E32321D2` |

四份日志均包含精确的一条 `Automation RunTests` 命令、至少一个 Success、0 Fail、0
Fatal/Unhandled/Ensure/Assertion、原生队列完成标记；对应进程退出码均为 0。

## 8. 改动驱动回归、构建与静态检查

5 个实现/测试/门禁改动路径命中 2 条映射规则，最终覆盖门：
`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=40 Logs=4`，SHA-256
`49D1A6C406DA4C48C032F06BEDE37999DFD6F02C29B8DF1FAA938D019359F833`。

映射器正反自测 `465/465` PASS，SHA-256
`06E12DACF499A261B2E8EA4BB1BCCE82251CD49D60094C3D91BCA0AB5310DAE5`。

| Target | Result / native exit | Duration | Log SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 24.51s | `0756FBD1...89222B51` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 1.14s | `DCF5C61F...D2883F0` |

最终 `demo_map.exe` 为 360,090,112 bytes，SHA-256
`E58BFD54E27A81E04C1537F96C2B801005660973F4250804EB8C6B3152103BEB`；
`UnrealEditor-demo_map.dll` 为 19,427,840 bytes，SHA-256
`C136754454AF0CC629C39997C88044A11155E1AFF93EAD45A153133A6C4CBA56`。

`git diff --check` 与 regression map JSON 解析通过；适配器实现 scoped scan 未发现 `UWorld`、Actor、
GameMode、Spawn/Destroy/扫描、输入框架、Tick、RNG、`ApplyDamage`、库存、序列或重试权威。

## 9. P/F 边界与下一阶段

P 阶段证明了设备无关单次采样、确定性事件身份、严格前置拒绝、一次委托、精确重放、冲突隔离、
生命周期最终权威、完整阵图激活/清理、完整 0.0.10 回归和改动驱动覆盖。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、
Cook 或 Package。因此不声明玩家按键已经触发阵法。下一阶段可把既有统一物理输入注册表的一条明确
动作绑定到本适配器；物理层只提供事件身份与采样回调，不得读取 ProductHost 或复制生命周期权威。

## 10. GitHub 交接

基线提交：`afe7ffd5966dea6a53c5a5bf2a87bd97aa018926`（P27.3）。
分支：`agent/0.0.10-p27-4-formation-input-adapter`。本阶段只提交 5 个实现/测试/门禁文件、本 Report
与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.4` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-4-formation-input-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-4-formation-input-adapter/Docs/Report/Dev.D.UE.0.0.10.P27.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-4-formation-input-adapter/Docs/Log/Dev.D.UE.0.0.10.P27.4.r0_log.md>
