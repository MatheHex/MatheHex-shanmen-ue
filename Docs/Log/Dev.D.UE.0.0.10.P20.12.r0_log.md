# Dev.D.UE.0.0.10.P20.12.r0 Development Log

## 1. 目标与基线

- 基线：`d9a83f72f9314f7cbc3195ed3a9d4108f2981d85`（P20.11）；
- 分支：`agent/0.0.10-p20-12-thrown-weapon-arc-choice-projection`；
- 目标：把冻结的 Arc choice 与调用方 basis/策略映射为确定性 target/apex 纯值；
- 约束：不接 GameMode/InputAdapter/设备/UI，不查询 World，不 trace，不执行 projectile、库存或伤害。

## 2. 接入前审计

P20.10 reducer 已冻结 normalized target intent 与累计 apex adjustment；P20.11 session 已提供 choice state 的唯一所有权、revision 与 Run/lifecycle fence。P20.8 Arc InputAdapter 接受调用方提供的世界 target 与正值 apex clearance，但两者之间没有纯值映射层。

本轮选择在两侧之间建立独立 projector。它不采样 actor transform，不持有 session，也不调用 InputAdapter，从而把“选择到几何”的确定性数学与“何时读取世界、何时发命令”的产品编排分开。

## 3. Basis 契约

`Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture` 接收 origin、forward、right。它先清空输出，再验证有限值、方向模长与正交性；成功后规范化方向并派生 `BasisId`。

零向量、非有限轴、平行或偏斜轴失败关闭。等价的轴缩放归一成相同 ID；origin/方向变化则改变 ID。字段私有，不能在 capture 后由外部改写。

## 4. Policy 契约

`Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture` 冻结：

- minimum/maximum forward distance；
- maximum lateral offset；
- minimum/maximum positive apex clearance。

capture 拒绝非有限值、非正前向/顶点最小值、逆序区间及负横向上限。成功策略派生 `PolicyId`，并由 `IsValid` 重算核验。

## 5. Projector 契约

Projector 的验证顺序固定为 choice validity、Arc trajectory、target intent、basis、policy、derived output。映射公式为：

```text
forwardAlpha = (clamp(targetY, -1, 1) + 1) / 2
forwardDistance = lerp(minForward, maxForward, forwardAlpha)
lateralOffset = clamp(targetX, -1, 1) * maxLateral
apexAlpha = (clamp(apexAdjustment, -1, 1) + 1) / 2
apexClearance = lerp(minApex, maxApex, apexAlpha)
target = origin + forward * forwardDistance + right * lateralOffset
```

成功 result 封存 choice state ID/revision、basis/policy ID、target、apex、距离分量与 projection ID。任何校验失败只返回状态和诊断，其余输出保持 canonical empty。有限输入造成派生溢出时返回 `OutputRejected`。

## 6. 新增自动化

新增 exact 5 项：

- `CaptureContracts`；
- `CanonicalMapping`；
- `BoundaryMapping`；
- `DeterminismAndSensitivity`；
- `FailClosed`。

canonical 示例使用 origin `(100, 200, 50)`、forward `+X`、right `+Y`、policy `400..1200 / 300 / 100..500`，choice `(0.5, 0.5)` 与 apex `0.25`，断言 target `(1100, 350, 50)`、forward `1000`、lateral `150`、apex `350`。

边界覆盖 target Y/apex 的 `-1/+1`、target X 的 `-1/+1`；确定性覆盖等价 normalized choice 与缩放 basis；失败覆盖 default choice、Straight、Arc 缺 target、无效 basis/policy 及有限大数造成的派生溢出。

## 7. 验证时间线

首次 Editor build：5 actions / native 0，SHA-256 `1211C286ED584F05976DC10B4933E12916AA0A365A40FBD222962386A0D21637`。

首次 projection exact：`5/0`、fatal/unhandled/ensure 0，SHA-256 `35879DDCAE4DD7EFC43750E683937EBB9377A49D9448DB16BE1CD045B3B8E5BB`。实现与断言无需重试。

流程自测最初误用 Windows PowerShell 5.1，因现代管道续行语法解析失败，native 1；失败证据 SHA-256 `07D55352D65D51EEC69A081C7ED5D5D48B633A47A594054821608E0CB9179613`。切换回 PowerShell 7.6.5 后为 `317/317`，SHA-256 `A8C0D1AC69CB4A3E4A3B2ED0741DE03B2A944D6849B664E10B3727E8EDDABF15`。

最终 Editor build：5 actions / native 0，SHA-256 `649D894D386F14158328B1EF676F52BF2D7F9D055F5F35B95F7BE9B1EADEA0AF`。

## 8. 最终自动化与 changed-file gate

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_choice_projection_final.log` | `5/0` | `275F183BF03C1C5000E4316554BB502A8B0884505064D4369E76233E5E5C218E` |
| `input_choice_final.log` | `9/0` | `5CBA6612AAE82EFF42AEFA4423EC1BB0EA7A3CDBB137B1B447F13B8A3D335C6E` |
| `arc_planner_final.log` | `10/0` | `F4C46C7FEF2CA54916049D8971734C8FE0C7B4308B28968987ED94BFFF0E08EA` |
| `full_0_0_10_final.log` | `877/0` | `DD6E2053A33CBCAB4FA5F67764390FFF6BCDE4DE10436963F6033E8F1402727E` |

日志审计为 `PASS`：每份均有 1 个 canonical RunTests、1 个 queue-empty、1 个 TestExit，合计 success 901、fail 0、fatal/unhandled/ensure 0；SHA-256 `8C10889BB4D2175947394BC71567FDEB46609DEC9370057EC845208B96BA81DA`。

映射新增一条规则，要求 projection exact、choice aggregate、Arc planner 与 broad full。自测增加正例和无关 item evidence 反例，最终 317/317。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=4
```

- gate SHA-256：`377B8A320F032A790D70094172722032AC899427041303AD7951E38662844521`；
- production boundary：`PASS Files=2 AddedLines=537 Matches=0`；
- boundary SHA-256：`04F43E43E723B758CB551DBF6AD21192CA10A5E141489043AE5FAFAA1587D0F8`；
- `git diff --cached --check`：`PASS / native 0`；证据 SHA-256 `052EA44DD06B584805D1E718C1D8B95075E7FF45DB6FDF33DAE3224116AEEA0A`；
- 长期未跟踪文件不纳入暂存、提交或推送。

## 9. 最终构建与产物

- Game build：4 actions / native 0，SHA-256 `E1DDAF97E7FE83D537EC077AE80E40803303E69C1A52651AB36E580EE5AB434A`；
- `demo_map.exe`：`357268992` bytes / `12BE30CA843DB3438A856D8343F07B4CB5240F7F458247A787761ABAC0EE027B`；
- `UnrealEditor-demo_map.dll`：`15945216` bytes / `AE87797B1774C15D0167FA0EEDD57AF0E75FA60796A0BA04F8264BA15C681969`。

只执行编译与无头自动化，没有启动产品。

## 10. P/F 边界与后续判断

本轮证明冻结 choice + caller basis + frozen policy 能稳定生成 target/apex 纯值及确定性 identity。它不证明 source basis 的世界采样、真实输入、GameMode/InputAdapter route、碰撞或命中。

P20.13 应由单一 consumer-owned composition 在命令边界采样 basis 一次，读取当前冻结 choice，调用 projector，并把结果委托给既有 Arc InputAdapter；不得复制 session、item、Run 或 world delivery 权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-12-thrown-weapon-arc-choice-projection>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-12-thrown-weapon-arc-choice-projection/Docs/Report/Dev.D.UE.0.0.10.P20.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-12-thrown-weapon-arc-choice-projection/Docs/Log/Dev.D.UE.0.0.10.P20.12.r0_log.md>
