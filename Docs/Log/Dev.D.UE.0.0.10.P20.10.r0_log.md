# Dev.D.UE.0.0.10.P20.10.r0 Development Log

## 1. 目标与基线

- 基线：`e52a77c61c08d37b2448de694dca2bbcf1464102`（P20.9）；
- 分支：`agent/0.0.10-p20-10-thrown-weapon-input-choice-reducer`；
- 目标：定义平台无关的 thrown-weapon trajectory/Arc 输入选择命令、不可变状态和纯 reducer；
- 约束：不接 GameMode、PlayerController、按键/轴、鼠标/手柄、UI、world projection、trace/sweep、库存、伤害或 projectile 执行。

## 2. 基线审计

P20.9 已允许 GameMode 在 Run 之前选择 `Straight`/`BallisticArc`，并公开 Arc hotbar route，但仍没有一个设备无关的输入选择状态机。直接在 PlayerController 或 GameMode 内累积 target/apex 会把物理输入、产品生命周期和数值规范耦合在一起。

本轮复用既有 `Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind`，只新增 consumer-local value contract。它不持有 UObject、World、Actor、库存或产品 host，不建立第二套 thrown-weapon 权威链。

## 3. Command 捕获

新增四种 command kind：trajectory selection、Arc target set、Arc apex adjustment、Arc target clear。所有命令必须带合法 expected revision；工厂清空输出后才捕获，失败不残留半合法数据。

目标意图要求有限、非零，并规范到闭单位圆；高点增量要求有限、非零并限制到 `[-1,1]`。payload 规范后再以版本化命名空间生成 deterministic command ID，`IsValid` 会重算并核对身份。

## 4. State 与 Reducer

初始状态固定 `Straight / revision 0 / no last command / no Arc target / apex 0`。字段私有，外部只能读取，reducer 是唯一推进入口。state ID 同样从全部 canonical 字段确定性派生并自校验。

reducer 先验证 state、command、精确 replay、revision 和耗尽边界，再应用命令。变化恰好增加一次 revision；相同新鲜值返回 `NoChange`；同一已执行命令返回 `Replay`；失败结果不发布旧状态伪装成成功。

Arc target/apex/clear 只允许在 `BallisticArc`。轨迹发生切换时 Arc 字段全部归零，保证回到 Arc 不继承旧选择。

## 5. 确定性数值修正

开发初版对长度大于 1 的 target 做 double 径向归一化。自动化发现 `(3,4)` 与 `(0.6,0.8)` 的 X 分量最低位不同，进而产生不同 command ID。

最终规则把归一化后的 X/Y 以及高点输入量化到 float 输入精度，再扩展为 double 存储并规范 signed zero。闭单位圆验证容差与 float 网格相容。这样设备常见 float 输入与数学归一化路径共享同一 canonical identity，状态内部仍以稳定 double bit pattern 派生 ID。

## 6. 自动化扩展

新增 exact group `Shanmen.0_0_10.Product.ThrownWeaponInputChoice`，共 5 项：

- `CommandNormalization`；
- `ModeAndTargetTransitions`；
- `ApexSaturationAndReset`；
- `FailClosed`；
- `DeterministicSequence`。

覆盖单位圆归一化、signed zero、NaN/Infinity、非法枚举/revision、Straight/Arc fence、切换清理、清除幂等、apex 饱和、revision mismatch、exact replay、状态/命令身份自校验和同序列/异序列 identity。

## 7. 失败透明度

| Evidence | Result | SHA-256 |
|---|---|---|
| `editor_build_initial.log` | native 6；测试 NaN API 不兼容 | `5047DC67D3580EEF43FB7C155BA4330C055506932842E75DAE0AAF52FA39D232` |
| `editor_build_after_fix.log` | native 0 | `42D66040872410B43C71B02C6DCE5556B179F0DB3EF35BEA8E119D6C03FC6E5F` |
| `input_choice_first.log` | 4/1；等价向量 ID 不同 | `6929019B5186CB89B1B8EDCB5E856F212203EF23C1C464C6C975A873F39B041F` |
| `editor_build_after_normalization_fix.log` | native 0 | `0C3EB8F47E7CD0004154EA29A3525B67C5ACDC524D3EB3D98DEA2EBCBD060109` |
| `input_choice_final-backup-2026.09.04-12.14.54.log` | 4/1；首轮容差修复无效 | `85D3BAC75FE043A76EA742CB53B5800788CC8A53B0D05E6C04643BA419B25ADA` |
| `editor_build_after_assertion_split.log` | native 0 | `1ECCA0A045E3730853E823547C0E15928899A4E9D4CF00214774264F519C7719` |
| `input_choice_diagnostic.log` | 4/1；定位为 command ID | `4212A477E38997B589B7530E441CA7C3CF393FCCC46DF58B75C55C1F816E9ACE` |
| `input_choice_debug_values.log` | 4/1；记录最低位差异 | `9E9E79C0F9B688DDDBA0DEC6703E5B2BF686424612EEC17ECC13DCBDEDA2518E` |
| `editor_build_after_canonical_grid_fix.log` | 5 actions / native 0 | `3A2DF864A864DF50092994A118AE5A2F4FBF2C9E25F292F9049EB962BDE5F489` |
| `input_choice_final.log` | 5/0 / canonical terminal | `453D064D014B704700125380FC9E4C445599B0A26E2F46A0131FFFBE01A3EB67` |

失败均来自新增测试 API 或新契约的确定性断言；没有把它们描述为内存、机器或 Unreal 环境故障。每次修复后重新构建并重跑 exact，原始失败和诊断日志完整保留。

## 8. 最终自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `input_choice_final.log` | 5/0 | `453D064D014B704700125380FC9E4C445599B0A26E2F46A0131FFFBE01A3EB67` |
| `full_0_0_10_final.log` | 868/0 | `86A4A67632360EA171ECF0D054A1E383525A7063BF9D21F2810B71A35D684561` |

证据审计：`PASS Logs=2 RecordedSuccess=873 Invalid=0`；SHA-256 `EEC69AD0FE51423DCC1E926B93B02CBA498B5ACE53A921DA4DB55D956CD00D3B`。

0.0.10 全量从 863 增至 868。全量是一个连续进程；既有 SwordRhythm 长测试与环境 `generate_204` 警告未导致失败、拆组或重启。

## 9. Changed-file regression 与静态边界

新增 production 路径规则 `ThrownWeaponInputChoiceReducer`，必须同时提供 exact 与 0.0.10 full。流程自测增加匹配正例和“只有无关 item evidence”失败关闭反例，最终 `313/313`，SHA-256 `27D5A5FDEB27704D272C53EEBA712A74897A219FEA930F18F1F4FD72124C7EAD`。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=2 Logs=2
```

- gate SHA-256：`E02416424F5A62DA757E235712CE989DBE172B80087F89CF9F110C7A7FB3FEB4`；
- production boundary：`PASS Matches=0`；
- boundary SHA-256：`B0368F34AD8D06963094AFE1D05B5665BBF558CF72C5009B24DD9E84333E0174`；
- `git diff --cached --check`：PASS / native 0；证据 SHA-256 `052EA44DD06B584805D1E718C1D8B95075E7FF45DB6FDF33DAE3224116AEEA0A`；
- 长期未跟踪文件不纳入暂存、提交或推送。

边界扫描覆盖 World/Actor/PlayerController、GameplayStatics、spawn、damage、trace/sweep、device input binding、RNG 与库存 mutation。

## 10. 构建与产物

- final Editor：up to date / 0 actions / native 0，SHA-256 `E4E320E5C37F3D9A241DA8712EC1AC389C7D1F481AC043C27F40A1AB309BC40C`；
- final Game：4 actions / native 0，SHA-256 `4E252FFFCBBE1973344E9E758F5EBFDB6577A5B28F8345FB1DE046F1FD4ABC5B`；
- `demo_map.exe`：357,228,032 bytes / `1B9F0D71FC379CE7A09EBFF19B8CEF36A8C8B6A907A34BB4759C10D9E13413FD`；
- `UnrealEditor-demo_map.dll`：15,897,088 bytes / `A6F924D66E17D01053CEF18F2E2FA5D032B85B4EA480AF47AEC1F94A534ACC08`。

## 11. P/F 边界与后续判断

本轮证明纯命令/状态/reducer 的规范化、失败关闭、幂等和确定性，不证明 GameMode 消费、设备事件、world target、轨迹可视化或玩家体验。

P20.11 建议增加 consumer-owned session/组合层，在 Run 外接受本轮命令，并把 trajectory 选择冻结进 P20.9 的唯一 GameMode/lifecycle；继续后置 PlayerController、具体设备绑定、world projection 与 UI。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-10-thrown-weapon-input-choice-reducer>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-10-thrown-weapon-input-choice-reducer/Docs/Report/Dev.D.UE.0.0.10.P20.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-10-thrown-weapon-input-choice-reducer/Docs/Log/Dev.D.UE.0.0.10.P20.10.r0_log.md>
