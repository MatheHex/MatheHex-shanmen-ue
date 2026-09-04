# Dev.D.UE.0.0.10.P20.10.r0 Report

## 1. 结论

P20.10 已建立平台无关、纯值的投掷武器输入选择契约与确定性归约器。调用方现在可以用 typed command 表达轨迹选择、Arc 目标意图设置、Arc 高点调整和目标清除；归约器以调用方持有的不可变状态、revision fence 和确定性 ID 得到唯一结果。

初始状态固定为 `Straight / revision 0`。Arc 专属命令在 Straight 模式失败关闭；切换轨迹会清空旧 Arc 目标和高点状态；完全相同的已执行命令返回 `Replay`，新鲜但不改变值的命令返回 `NoChange`，两者都不重复推进 revision。

二维目标输入被规范化到闭单位圆，并落到输入 float 精度网格；Arc 高点增量限制在 `[-1, 1]`。因此数学等价的超长向量与单位向量得到相同 command ID，输入浮点边界不会把同一物理意图分裂成不同身份。

本轮未接 GameMode、PlayerController、设备输入、按键映射、鼠标/手柄、UI、world target projection、trace/sweep 或轨迹预览；没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`e52a77c61c08d37b2448de694dca2bbcf1464102`（P20.9）；
- 分支：`agent/0.0.10-p20-10-thrown-weapon-input-choice-reducer`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Typed 输入命令

新增 `Fdemo_mapShanmenThrownWeaponInputChoiceCommand`，只允许通过四个捕获入口形成合法命令：

- `SelectTrajectory`：选择既有 `Straight` 或 `BallisticArc`；
- `SetArcTargetIntent`：提交二维 Arc 目标意图；
- `AdjustArcApex`：提交归一化高点增量；
- `ClearArcTargetIntent`：显式清除 Arc 目标。

每条命令携带 `ExpectedRevision` 与由 canonical payload 派生的 `CommandId`。非法枚举、`MAX_uint64` revision、NaN/Infinity、近零目标、零高点增量或身份不匹配均无法成为有效命令。

## 4. 不可变状态与归约语义

`Fdemo_mapShanmenThrownWeaponInputChoiceState` 的字段私有，只有无状态 reducer 可以生成下一状态。状态携带 revision、最后命令 ID、轨迹类型、可选 Arc 目标、高点设置与自校验 `StateId`。

归约结果显式区分：`Reduced`、`NoChange`、`Replay`、`StateInvalid`、`CommandInvalid`、`RevisionMismatch`、`RevisionExhausted`、`ModeMismatch` 与 `StateRejected`。失败结果不发布替代状态；成功变更恰好推进一次 revision。

选择另一轨迹时 Arc 专属字段统一归零，避免从旧 Arc 会话泄漏目标或高点。Straight 模式拒绝 Arc 编辑，Arc 模式允许幂等清除和饱和高点调整。

## 5. 数值规范化与确定性身份

目标输入必须有限且非零。长度大于 1 的向量先径向归一化，然后两个轴统一量化到输入 float 精度并规范 signed zero；单位圆边界使用与该精度相容的验证容差。

高点增量先限制到 `[-1, 1]`，再落到同一输入精度网格。状态累加结果继续按 double 位模式形成身份，因此同一规范命令序列可跨重建重复得到相同 state ID，而不同命令顺序保持不同身份。

命令与状态分别使用版本化命名空间 `demo_map.ShanmenThrownWeapon.InputChoiceCommand.r1` 和 `demo_map.ShanmenThrownWeapon.InputChoiceState.r1`，防止与其它确定性 ID 域碰撞。

## 6. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | 5/0 | `453D064D014B704700125380FC9E4C445599B0A26E2F46A0131FFFBE01A3EB67` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | 868/0 | `86A4A67632360EA171ECF0D054A1E383525A7063BF9D21F2810B71A35D684561` |

两份有效日志均只有一个 canonical `RunTests` command、一个成功终止信号、至少一个 Success、0 Fail，并且无 Fatal/Unhandled/Ensure：`PASS Logs=2 RecordedSuccess=873 Invalid=0`。审计 SHA-256：`EEC69AD0FE51423DCC1E926B93B02CBA498B5ACE53A921DA4DB55D956CD00D3B`。

新增 exact 自动化共 5 项：command 规范化、模式/目标转换、高点饱和与重置、非法输入失败关闭，以及同序列确定性/重放。0.0.10 全量从 P20.9 的 863 增至 868。

流程映射自测通过 `313/313`，SHA-256 `27D5A5FDEB27704D272C53EEBA712A74897A219FEA930F18F1F4FD72124C7EAD`。

## 7. 失败与重试证据

首次 Editor 构建的生产 reducer 已编译，但新增测试使用了 UE 5.8 中不存在的 `TNumericLimits<double>::QuietNaN`，整体 native exit 6。日志 `editor_build_initial.log` 保留，SHA-256 `5047DC67D3580EEF43FB7C155BA4330C055506932842E75DAE0AAF52FA39D232`。改用标准库 `std::numeric_limits<double>::quiet_NaN()` 后构建恢复 native 0。

首次 exact 为 4/1：`(3,4)` 径向归一化产生的 double 与字面 `(0.6,0.8)` 在最低位不同，导致数学等价目标的 deterministic command ID 不同。原日志 `input_choice_first.log` 保留，SHA-256 `6929019B5186CB89B1B8EDCB5E856F212203EF23C1C464C6C975A873F39B041F`。

第一版“单位圆容差内不归一化”没有解决身份差异；随后将断言拆开并记录实际值，确认差异为 `0.60000000000000009` 对 `0.59999999999999998`。最终修复把规范目标轴与高点输入统一量化到 float 输入网格，exact 达到 5/0。所有中间构建和失败/诊断日志均保留在 `Saved/Codex/P20.10`，没有覆盖失败事实或把失败日志计入最终有效证据。

## 8. 改动门禁与静态边界

新增 `ThrownWeaponInputChoiceReducer` changed-file 映射规则，要求 exact 组和 `Shanmen.0_0_10` 全量证据；正例与缺少 exact 的失败关闭反例均纳入流程自测。

最终 changed-file regression gate：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=2 Logs=2
```

gate SHA-256：`E02416424F5A62DA757E235712CE989DBE172B80087F89CF9F110C7A7FB3FEB4`。

对两个 production 文件扫描 World、Actor、PlayerController、GameplayStatics、spawn、damage、trace/sweep、设备 input binding、RNG 与库存修改 API：`PASS Matches=0`；SHA-256 `B0368F34AD8D06963094AFE1D05B5665BBF558CF72C5009B24DD9E84333E0174`。

`git diff --cached --check`：仅暂存本轮 7 个文件后 PASS / native 0；证据 SHA-256：`052EA44DD06B584805D1E718C1D8B95075E7FF45DB6FDF33DAE3224116AEEA0A`。长期未跟踪文件未被纳入。

## 9. 构建与产物

- final Editor：up to date / 0 actions / native 0，SHA-256 `E4E320E5C37F3D9A241DA8712EC1AC389C7D1F481AC043C27F40A1AB309BC40C`；
- final Game：4 actions / native 0，SHA-256 `4E252FFFCBBE1973344E9E758F5EBFDB6577A5B28F8345FB1DE046F1FD4ABC5B`。

产物：

- `Binaries/Win64/demo_map.exe`：357,228,032 bytes，SHA-256 `1B9F0D71FC379CE7A09EBFF19B8CEF36A8C8B6A907A34BB4759C10D9E13413FD`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,897,088 bytes，SHA-256 `A6F924D66E17D01053CEF18F2E2FA5D032B85B4EA480AF47AEC1F94A534ACC08`。

## 10. P/F 边界与下一步

P20.10 证明的是“设备无关输入意图可被规范捕获，并以 revision/identity fence 纯函数式推进 Straight/Arc 选择状态”。它没有证明该状态已被 GameMode 或 P20.9 lifecycle 消费，也没有证明真实键鼠/手柄、世界目标投影、轨迹呈现或产品操作体验。

建议 P20.11 增加一条薄的 consumer-owned input-choice session/组合层：只在 combat Run 外接受本轮命令，把已选择 trajectory 冻结到 P20.9 的 GameMode/lifecycle 配置，并提供只读状态；仍不接 PlayerController、设备绑定、world target projection 或 UI。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-10-thrown-weapon-input-choice-reducer>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-10-thrown-weapon-input-choice-reducer/Docs/Report/Dev.D.UE.0.0.10.P20.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-10-thrown-weapon-input-choice-reducer/Docs/Log/Dev.D.UE.0.0.10.P20.10.r0_log.md>
