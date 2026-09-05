# Dev.D.UE.0.0.10.P20.28.r0 Report

## 1. 结论

P20.28 已把 P20.27 当前重映射后的 Ballistic Arc target、apex increase、apex decrease 与 target clear 键名接入现有 HUD。新增纯值、不可变的 input-hint presentation；它只接收一份 P20.24 Arc presentation 与四个显示键名，不拥有输入配置、choice state、revision、route、World、Actor 或 UI 状态。

HUD 延续单次读取当前 choice interaction model：先生成既有 trajectory/Arc presentation，再从统一 `Fdemo_mapInputBindingSettings` 读取四个当前键名并生成提示。提示明确显示 apex 上下界 `(LIMIT)` 与无 target 时的 `(NO TARGET)`，不会硬编码 P20.27 默认键，也没有第二套输入或显示权威。

新增聚焦自动化 `6/0`，完整 0.0.10 由 `979` 增至 `985/0`。按改动文件映射运行 9 份最终自动化证据，合计 `1151/0`；Editor 与 Game Development 构建均成功。

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实鼠标/键盘输入、可见 HUD 验收、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`36d8f4194c31aa9553b7d87b2846b68b77fda122`（P20.27）；
- 分支：`agent/0.0.10-p20-28-thrown-weapon-arc-input-hud-hint`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Input-hint presentation 契约

`Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation::TryProject()` 接收：

1. 一份有效的 P20.24 Arc editing presentation；
2. target set 当前显示键名；
3. apex increase 当前显示键名；
4. apex decrease 当前显示键名；
5. target clear 当前显示键名。

四个键名会先去除首尾空白；任一为空或忽略大小写后重复时，投影失败并清空复用的输出。成功结果只保存规范键名、三个 capability 布尔值和从这些值确定性生成的文本；`IsValid()` 会重新生成文本并核对全部不变量。

## 4. 提示语义

规范文本格式为：

`ARC INPUT: [Target] SET TARGET | [Increase] APEX + | [Decrease] APEX - | [Clear] CLEAR`

- apex 已达上界时，仅 increase 后附加 `(LIMIT)`；
- apex 已达下界时，仅 decrease 后附加 `(LIMIT)`；
- 当前没有 target 时，clear 后附加 `(NO TARGET)`；
- 当前有 target 时，clear 不显示错误的不可用标记；
- 非 Arc/无效 Arc presentation 不产生 hint。

这些标记来自既有 P20.24 capability，不由 HUD 猜测 choice 状态。

## 5. HUD 接入

`Ademo_mapHUD::DrawHUD()` 仍只调用一次 `ReadThrownWeaponInputChoiceInteraction()`。Arc presentation 成功后，HUD 从统一输入设置分别读取：

- `ThrownWeaponArcTargetSet`；
- `ThrownWeaponArcApexIncrease`；
- `ThrownWeaponArcApexDecrease`；
- `ThrownWeaponArcTargetClear`。

input hint 绘制在现有 Arc target、apex 与 trajectory 行上方，垂直偏移依次为 `180 / 158 / 136 / 112`。本轮没有添加 Widget、定时器、轮询、缓存状态或独立刷新路径；live remap 的显示真值继续来自统一输入设置。

## 6. 新增自动化覆盖

`Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation` 新增 6 项无头自动化：

1. `NeutralDefaults`：默认键名与无 target 提示；
2. `TargetEnablesClear`：已有 target 时 clear 可用；
3. `ApexCapabilityBoundaries`：上下界只标记被阻止的方向；
4. `RemappedLabels`：自定义键名规范化并完整替换默认显示；
5. `InvalidLabelsFailClosed`：空白或大小写重复键名失败关闭并清空旧输出；
6. `InvalidArcFailsClosed`：无效 Arc presentation 失败关闭并清空旧输出。

测试只验证纯值投影和已有 HUD 源码接线，没有声明屏幕上实际可见或真实设备输入通过。

## 7. 改动文件回归映射与有界修复

新增 `ThrownWeaponArcEditingInputHintPresentation` 映射规则，并把新组加入 `UnifiedInput` 与 `MainHUD` 所需证据；`MainHUD` 同时要求 P20.27 Arc physical input、`demo_map.InputRestore` 与既有 Arc/trajectory/interaction 组。正反映射 self-test 由 `349` 增至 `351/351`。

最终 changed-file gate 对 6 个生产、测试与流程路径求并集：`Changed=6 Rules=2 Required=9 Logs=9`，全部具备健康证据。

第一次检查时，`interaction_port` 日志已有 `7/0`，但在写入 UE 原生终止标记之前结束，因此门禁按“不健康证据”拒绝；未修改源码，只串行重跑该组一次，得到 `7/0`、唯一终止标记与 native exit `0` 后门禁通过。该修复没有把缺终止标记的日志误报为成功。

## 8. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_input_hint_presentation_final.log` | `Product.ThrownWeaponArcEditingInputHintPresentation` | `6/0` | `83774F2E50C4ECD08AA4BD05A189F3FD5B3ACFB07AB511AE5D548A8EC8D7BF95` |
| `arc_editing_presentation_final.log` | `Product.ThrownWeaponArcEditingPresentation` | `7/0` | `4045D92DDCC26212525E98CEC2ECED08CA6BDBF10A58D9763E8E935380C6EDF6` |
| `interaction_port_final.log` | `Product.ThrownWeaponInputChoiceInteractionPort` | `7/0` | `8B8BA9006A24D1C4B77CFFB87CE75E7CEE579BB10776609AFF0ABB3FD92C3BCF` |
| `trajectory_presentation_final.log` | `Product.ThrownWeaponTrajectoryPresentation` | `6/0` | `DF05248FAFABAC76A007BAFD37F78BA897B7B602517CC54437EDFFB40B612EFC` |
| `trajectory_toggle_physical_input_final.log` | `Product.ThrownWeaponTrajectoryTogglePhysicalInput` | `7/0` | `3A96908711C79B6E7DA56238001AC5493A2C27F048C47780803F02680A99D929` |
| `arc_editing_physical_input_final.log` | `Product.ThrownWeaponArcEditingPhysicalInput` | `10/0` | `F36DF79DC80A130F1A87B504E63A29E8637377AC88C2EACC127DB6DACB06C91B` |
| `legacy_input_restore_final.log` | `demo_map.InputRestore` | `101/0` | `E5C51629821BCA41D8E5A160D52722C42C390733B85D0E7757AF46EBE3BA0FB0` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `CD42CC7E5D4F6F92709DE112E76181FFB7554980F7D62DF61CA5A12FACB67B07` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `985/0` | `AD2EE0E42A3D704714835C8E624326A0CE1970EBAC85E8763AD8BA983D363654` |

最终证据合计 `1151/0`。日志审计逐份核对唯一 RunTests 命令、精确成功数、零失败、唯一 UE 5.8 原生终止标记及零 fatal/unhandled/ensure；审计日志 SHA-256 为 `048E7BA6F6AB2C4765B0FBDF63FB11B0FA0D24B88CB6F6163BCB61D7CB5A9787`。

## 9. 静态、流程、构建与产物

- regression self-test：`351/351`，SHA-256 `760DBBD335284E50A9BBCB1A0DB62DBCD38D098AB0F6A0775578501B820D4B49`；
- static audit：`26/26`，验证单次 choice read、四个当前键读取、投影失败关闭、键名唯一性、四行布局及无 runtime authority/loop，SHA-256 `AA6199D4BC10CB569024BC894AE333FB52FB9E5402830C53FE1F21B6F924FBB8`；
- changed-file gate：`PASS Changed=6 Rules=2 Required=9 Logs=9`，SHA-256 `DF9F18F70939F3B6F5BEF1B5B09DB05ADDC28413D26698842FF59A03EFC1540C`；
- `git diff --check`：PASS，SHA-256 `73CD9640FFF26DC1B19EECB07087B0095046B76052E4C6F3D54A6039858DE852`；
- first Editor：6 actions / native 0 / 24.03 秒，SHA-256 `C9CD71364328DF4824D2C748277DAF54121D377223E87EFF4091E9E8BF78333F`；
- final Editor：up to date / native 0 / 0.96 秒，SHA-256 `342CA488D8684E2C4596AFC1B9DD819F6B340D6014AAD15302FE2E109F2EB4BA`；
- final Game：5 actions / native 0 / 26.37 秒，SHA-256 `F2839CA36F736BF27D1A60451DC744A22B1A75098573012C190349D4A2349C74`。

产物：

- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,456,704 bytes，SHA-256 `B7B94D721A135A4C7156FD9845E7E8590B545B576A5D24A12802597902AE1411`；
- `Binaries/Win64/demo_map.exe`：357,686,272 bytes，SHA-256 `54E219A09F85C439E7FCE28CC27DC491D4C67A675CCF46637CE9689AB083ABC9`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：纯值 Arc input-hint 投影、当前 remap 键名、空白/重复标签失败关闭、target 与 apex capability 标记、HUD 单次 choice read 接线、完整 0.0.10 与受影响旧输入/远程兼容回归。

未验证：实际 HUD 布局/可读性、真实鼠标/键盘、OS 焦点与 cursor hit、Editor UI/PIE/Standalone、真实轨迹/落点、投掷/碰撞/命中、库存、伤害、Smoke、Cook 或 Package。Development 构建与无头自动化不能描述为可见产品验收。

建议 P20.29 增加纯值 Ballistic Arc preview sampler：从既有已验证 Arc plan 按固定样本数生成确定性 world-space 轨迹点与落点描述，强制首尾点、飞行时间与重力一致；不查询 World、不 trace、不创建 Actor/Widget/渲染状态。可见绘制仍留给后续独立适配阶段。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-28-thrown-weapon-arc-input-hud-hint>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-28-thrown-weapon-arc-input-hud-hint/Docs/Report/Dev.D.UE.0.0.10.P20.28.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-28-thrown-weapon-arc-input-hud-hint/Docs/Log/Dev.D.UE.0.0.10.P20.28.r0_log.md>
