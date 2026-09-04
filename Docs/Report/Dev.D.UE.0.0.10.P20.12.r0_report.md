# Dev.D.UE.0.0.10.P20.12.r0 Report

## 1. 结论

P20.12 已新增投掷武器 Arc choice 的纯值投射契约：接收 P20.10/P20.11 已冻结的 choice state、调用方提供的平面 basis，以及冻结的距离/顶点策略，确定性地产生世界 target、正值 apex clearance、可审计距离分量和 projection identity。

本轮没有把投射器接进 `Ademo_mapGameMode`、设备输入或 P20.8 Arc InputAdapter。它不读取 World、不做 trace/sweep、不查询 actor/controller，也不触碰库存、伤害、projectile 执行或 UI。后续消费者只能显式提供一次采样后的 basis 与策略，避免纯计算层形成第二个世界或产品权威。

最终验证为：新投射 exact `5/0`、P20.11 choice aggregate `9/0`、既有 Arc planner `10/0`、0.0.10 全量 `877/0`；Editor 与 Game Development 构建均成功。changed-file regression gate 为 `PASS Changed=5 Rules=1 Required=4 Logs=4`。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`d9a83f72f9314f7cbc3195ed3a9d4108f2981d85`（P20.11）；
- 分支：`agent/0.0.10-p20-12-thrown-weapon-arc-choice-projection`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Caller-supplied basis

新增 `Fdemo_mapShanmenThrownWeaponArcChoiceBasis`。调用方必须显式传入 origin、forward 与 right；capture 会检查有限值、归一化两个方向，并拒绝零向量、平行或偏斜轴。

basis 字段保持私有，只通过只读 getter 暴露。canonical origin 与归一化轴参与 `BasisId` 派生，因此等价缩放方向获得相同 identity，而 origin 或轴改变会改变 identity。无效 capture 会清空输出，不留下旧 ID 或旧几何。

## 4. 冻结策略与映射语义

新增 `Fdemo_mapShanmenThrownWeaponArcChoicePolicy`，冻结五个值：最小/最大前向距离、最大横向偏移、最小/最大 apex clearance。所有值必须有限；前向距离与 apex clearance 必须为正且区间有序，横向上限不得为负。

投射语义固定为：

- target intent X 表示有符号横向意图，乘以最大横向偏移；
- target intent Y 从 `[-1, 1]` 线性映射到最小/最大前向距离；
- 累积 apex adjustment 从 `[-1, 1]` 线性映射到最小/最大正值 apex clearance；
- target 为 `origin + normalized forward * forward distance + normalized right * lateral offset`。

策略同样拥有确定性 `PolicyId`。策略值变化会改变 policy 与最终 projection identity。

## 5. 投射结果与失败关闭

`Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project` 按固定顺序验证 choice、Arc trajectory、target intent、basis、policy 与派生输出。成功结果封存：choice state ID/revision、basis ID、policy ID、target、apex clearance、前向距离、横向偏移及 `ProjectionId`。

状态显式区分 `ChoiceStateInvalid`、`TrajectoryNotArc`、`TargetIntentMissing`、`BasisInvalid`、`PolicyInvalid` 与 `OutputRejected`。任何失败结果都只保留精确诊断与失败状态；projection/source identity、target 与数值分量保持空值，不能携带上一轮陈旧输出。

有限但极大的输入若使派生世界几何溢出，同样以 `OutputRejected` 失败关闭。结果的 `IsValid()` 会重算确定性 identity，防止字段与 ID 不一致的对象被接受。

## 6. 自动化覆盖与结果

新增 `Shanmen.0_0_10.Product.ThrownWeaponArcChoiceProjection` 5 项：

- basis/policy capture、canonical identity 与失败清空；
- canonical target/apex 映射；
- 前向、横向与 apex 边界映射；
- 等价归一化输入重放与 basis/policy 敏感性；
- 非 Arc、缺 target、无效 basis/policy 与派生溢出的精确失败关闭。

最终日志：

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_choice_projection_final.log` | `Product.ThrownWeaponArcChoiceProjection` | `5/0` | `275F183BF03C1C5000E4316554BB502A8B0884505064D4369E76233E5E5C218E` |
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | `9/0` | `5CBA6612AAE82EFF42AEFA4423EC1BB0EA7A3CDBB137B1B447F13B8A3D335C6E` |
| `arc_planner_final.log` | `CombatRuntime.ThrownWeaponArc` | `10/0` | `F4C46C7FEF2CA54916049D8971734C8FE0C7B4308B28968987ED94BFFF0E08EA` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `877/0` | `DD6E2053A33CBCAB4FA5F67764390FFF6BCDE4DE10436963F6033E8F1402727E` |

四份最终日志均只有一个 canonical RunTests、一个 queue-empty、一个 TestExit，合计 901 个 success marker、0 fail、0 fatal/unhandled/ensure。审计 SHA-256：`8C10889BB4D2175947394BC71567FDEB46609DEC9370057EC845208B96BA81DA`。

0.0.10 全量从 P20.11 的 872 增至 `877`，恰好增加本轮 5 项。

## 7. 首次验证与环境修复证据

首次 Editor 构建直接通过：5 actions / native 0，SHA-256 `1211C286ED584F05976DC10B4933E12916AA0A365A40FBD222962386A0D21637`。首次 projection exact 直接为 `5/0`、无 fatal/unhandled/ensure，SHA-256 `35879DDCAE4DD7EFC43750E683937EBB9377A49D9448DB16BE1CD045B3B8E5BB`；没有源码或测试断言重试。

流程自测第一次被错误地通过嵌套 `powershell` 调用，实际选择 Windows PowerShell 5.1，因其不能解析仓库使用的 PowerShell 7 管道续行语法而以 native 1 退出。该环境失败已保留，SHA-256 `07D55352D65D51EEC69A081C7ED5D5D48B633A47A594054821608E0CB9179613`。

改用当前仓库兼容的 PowerShell 7.6.5 后，流程自测为 `317/317`，SHA-256 `A8C0D1AC69CB4A3E4A3B2ED0741DE03B2A944D6849B664E10B3727E8EDDABF15`。修复只更正执行器，没有改产品代码、测试断言或映射预期。

## 8. 改动门禁与静态边界

新增 `ThrownWeaponArcChoiceProjection` 映射，要求 projection exact、InputChoice aggregate、既有 Arc planner 与 0.0.10 full。流程自测增加一个 broad-evidence 正例与一个无关 item evidence 必须失败的反例。

最终 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=4
```

gate SHA-256：`377B8A320F032A790D70094172722032AC899427041303AD7951E38662844521`。

对 2 个 production 文件的 537 行扫描 GameplayStatics、World/Actor API、spawn、trace/sweep、PlayerController、设备输入、RNG、库存/物品权威、旧 hotbar route 与 Arc planner 引用：`PASS Matches=0`；SHA-256 `04F43E43E723B758CB551DBF6AD21192CA10A5E141489043AE5FAFAA1587D0F8`。

`git diff --cached --check`：精确暂存本轮 7 个文件后 `PASS / native 0`；证据 SHA-256 `052EA44DD06B584805D1E718C1D8B95075E7FF45DB6FDF33DAE3224116AEEA0A`。长期未跟踪文件未被纳入。

## 9. 构建与产物

- final Editor：5 actions / native 0，SHA-256 `649D894D386F14158328B1EF676F52BF2D7F9D055F5F35B95F7BE9B1EADEA0AF`；
- final Game：4 actions / native 0，SHA-256 `E1DDAF97E7FE83D537EC077AE80E40803303E69C1A52651AB36E580EE5AB434A`。

产物：

- `Binaries/Win64/demo_map.exe`：`357268992` bytes，SHA-256 `12BE30CA843DB3438A856D8343F07B4CB5240F7F458247A787761ABAC0EE027B`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：`15945216` bytes，SHA-256 `AE87797B1774C15D0167FA0EEDD57AF0E75FA60796A0BA04F8264BA15C681969`。

## 10. P/F 边界与下一步

P20.12 证明的是：“一个有效且已冻结的 Arc choice，在调用方提供有效 basis 与冻结策略时，可确定性地产生 Arc InputAdapter 所需的 target/apex 纯值，并对非法或溢出输入失败关闭。”

它没有证明真实世界 source transform 的采样、设备输入、GameMode 组合、InputAdapter 委托、轨迹预览、碰撞、命中、库存扣减或伤害。不能把本轮 `877/0` 描述为产品运行验收。

建议 P20.13 只增加一个 consumer-owned composition：在命令边界采样调用方 source basis 一次，读取当前冻结 choice，调用本轮 projector，然后把 target/apex 委托给既有 P20.8 Arc InputAdapter；继续复用唯一 item/run/world authority，不创建第二套路由或状态权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-12-thrown-weapon-arc-choice-projection>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-12-thrown-weapon-arc-choice-projection/Docs/Report/Dev.D.UE.0.0.10.P20.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-12-thrown-weapon-arc-choice-projection/Docs/Log/Dev.D.UE.0.0.10.P20.12.r0_log.md>
