# Dev.D.UE.0.0.10.P20.11.r0 Report

## 1. 结论

P20.11 已把 P20.10 的纯输入选择 reducer 接成 consumer-owned session，并让 `Ademo_mapGameMode` 以该 session 的不可变 state 作为投掷武器轨迹配置的唯一真值。

GameMode 不再单独保存 `ConfiguredThrownWeaponTrajectoryKind`。新的 typed command 入口统一经过 session；旧 `TryConfigureThrownWeaponTrajectory` 仅作为兼容适配器捕获 typed command。combat Run 激活时，P20.9 lifecycle 从同一 state 读取 `Straight`/`BallisticArc`，`RunBound` 审计同时记录 choice state ID 与 revision。

session 先让 reducer 判定命令。会改变状态的命令在 active combat Run 或非空 thrown-weapon lifecycle 下失败关闭；精确 replay 和新鲜 no-op 不改变任何配置，因此锁定期间仍可安全确认。成功变更恰好推进一次 revision，拒绝结果不发布替代 state。

本轮未增加 PlayerController、键鼠/手柄绑定、UI、world target projection、trace/sweep、轨迹预览、库存或伤害路径；没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有真实输入、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`60d5be120f2f8b3395ca93bbb973907226a27310`（P20.10）；
- 分支：`agent/0.0.10-p20-11-thrown-weapon-input-choice-session`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Consumer-owned session

新增 `Fdemo_mapShanmenThrownWeaponInputChoiceSession`。构造时创建 canonical `Straight / revision 0` state，外部只能读取 state 与 trajectory，不能直接改字段。

`Submit` 对 P20.10 reducer 的结果进行薄封装，显式区分 `Applied`、`NoChange`、`Replay`、`CombatRunActive`、`ProductLifecycleNotEmpty` 与 `ReductionRejected`。reducer 的精确状态码同时保留，revision mismatch、非法命令等原因不会被 session 抹平。

只有 `Reduced` 候选会接受 Run/lifecycle 锁检查；`Replay` 和 `NoChange` 直接返回当前有效 state。锁定拒绝携带诊断和 reducer 的 `Reduced` 判定，但 result state 保持无效，session 内部 state 不变。

## 4. GameMode 单一真值

GameMode 新增 `SubmitThrownWeaponInputChoiceCommand` 和只读 `GetThrownWeaponInputChoiceState`。前者把真实 `CombatRunCoordinator.IsActive()` 与 `ThrownWeaponProductLifecycle.IsEmpty()` 作为 session fence；后者公开唯一 canonical read model。

旧 trajectory 字段已移除。兼容配置方法按当前 revision 捕获选择命令并提交到同一 session；成功时保持原有空 diagnostic 语义，非法 enum 或产品锁定时返回精确原因。

`TryActivateCombatRun` 从 session 读取 trajectory 传给唯一 lifecycle。成功日志新增 `ThrownWeaponChoiceStateId` 和 `ThrownWeaponChoiceRevision`，使一次 Run 实际冻结的选择可审计；没有创建第二套 lifecycle、controller、inventory 或 world delivery。

## 5. 自动化覆盖

新增 session exact 4 项：

- canonical lifecycle 与只读 state；
- active Run / stale lifecycle fence；
- reducer failure reason 透传；
- 等价序列确定性与 trajectory reset。

既有 GameMode trajectory 测试扩展为验证：初始 state、非法请求不变、typed Arc 提交、exact replay、兼容 no-op、恢复 Straight 及 revision/state identity。InputAdapter 测试总数不变。

## 6. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `input_choice_final.log` | `Product.ThrownWeaponInputChoice` | 9/0 | `93B920744EBE09B38C6C2D7F976754F65B225C7ECB843F1B0E142C0D229ECC02` |
| `input_choice_session_final.log` | `Product.ThrownWeaponInputChoiceSession` | 4/0 | `9EB3C303ACC2A44472B2379EC2DB630D0C8B495EE8579834E138B2FC25C670E8` |
| `input_adapter_final.log` | `Product.ThrownWeaponInputAdapter` | 9/0 | `F95349C508D97FE896BC19310B5673DA9842CCE657F128A324A48BCDF9E80853` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `872/0` | `EEBE88F5351D5390F94A87F5830AA5525CD81E24AB5071E349ACD158D5EACF6E` |
| `legacy_v3_attributes_final.log` | `demo_map.V3.Attributes` | `4/0` | `0F7260869627FA79F75519048A78ADD47D57C745BF124FFB54B7ED1040C8BC2A` |
| `legacy_enemy_skill_final.log` | `demo_map.EnemySkillFramework` | `44/0` | `896960933E7A6E12ADED9BA287427470F1793A08DB29A27C5D0C12D88EB71A30` |
| `legacy_v2_ranged_final.log` | `demo_map.V2RangedCompatibility` | `22/0` | `00D12270DF253176ABF14B5CC3C3C78267506FAE14AFFCD875B8F3EFA9F835C6` |
| `legacy_item_armor_final.log` | `demo_map.ItemUseAndArmor` | `46/0` | `6D9ECDF72D2CB1FDB24D85ADECDF05F87F888BFE65BA30BFDAEF5412FEA71884` |

有效日志审计：`PASS Logs=8 RecordedSuccess=1010 Invalid=0`，SHA-256 `9A3DDCB33D4540776072ADD94A25BD3D62279D50C31A68539B4B91C34BDA898F`。0.0.10 全量从 P20.10 的 868 增至 `872`。

流程映射自测通过 315/315，SHA-256 `58E2FE22EF62C820A59B5AFF7D21063DF0A98F4D9C75CC5EB14FA72457869E28`。

## 7. 失败与重试证据

首次 Editor 构建直接通过：28 actions / native 0，SHA-256 `C7B072490DAFBE0979F30B1A39767B8EBFFBBEDBA982773F0D95CD612EA9EF31`。首次 session exact 为 4/0，GameMode/InputAdapter exact 为 9/0；未发生源码、测试断言或环境重试。

headless 日志中的非 Win64 平台 SDK 不可用信息与 `generate_204` 网络探测警告属于现有环境噪声；Win64 验证、测试结果、canonical queue-empty 与原生退出码均成功，未把其它平台状态描述为产品失败。

## 8. 改动门禁与静态边界

新增 `ThrownWeaponInputChoiceSession` 映射，要求 session exact、reducer aggregate 与 0.0.10 full；M01 GameMode 映射也补入两组 choice 证据。正例及无关 item 日志失败关闭反例均进入 315 项流程自测。

最终 changed-file regression gate：

```text
REGRESSION_COVERAGE: PASS Changed=8 Rules=3 Required=55 Logs=8
```

gate SHA-256：`78D744CD98A82449EAE77D48E6FF9E9D9F4E70A6346A255CC1852E22BDEDC1F3`。

对 4 个 production 文件的 221 条新增行扫描 GameplayStatics、damage、spawn、trace/sweep、device input binding、PlayerController、RNG、库存 mutation 与 Prepare/Commit：`PASS Matches=0`；SHA-256 `EDC248E07613EC2D8A996B45B1B9740B8183F6D99D1740B717BBDC767B22347C`。

`git diff --cached --check`：精确暂存本轮 10 个文件后 `PASS / native 0`；证据 SHA-256 `D827FF1E4A8A3C80AF883BC3FBFFC7B55C12EF0FC4D43C89BAAFE167E7EA44D1`。长期未跟踪文件未被纳入。

## 9. 构建与产物

- final Editor：`26 actions / native 0`，SHA-256 `1F3CAB169234638982BB126BBB2C91209F87F75B53F3FC0139E690A30926978D`；
- final Game：`27 actions / native 0`，SHA-256 `F1EB2C34F227D3AE1BEF798E8341187C95AFBDA4205939DFBA3742DC9F8663C8`。

产物：

- `Binaries/Win64/demo_map.exe`：`357242368` bytes，SHA-256 `151B97DC48720ECB0B69F0092573CC1CE61CD1E4E76240ABAF53ABA537023755`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：`15915008` bytes，SHA-256 `5E6F60026D74DDBC6D591FB7714937BAAE102AC880A3F95BB9A4EDC1EA12D834`。

## 10. P/F 边界与下一步

P20.11 证明的是“输入选择 state 由一个 session 独占，并在 Run/lifecycle 边界冻结后作为 GameMode 与 lifecycle 的单一 trajectory 真值”。它没有把二维 target intent 或 normalized apex 映射到 P20.8 所需的世界 target/apex clearance，也没有证明任何物理设备输入或呈现。

建议 P20.12 定义纯值 Arc choice projection：由调用方提供 source transform/basis 与冻结的距离、高度策略，把本轮 state 的 normalized target/apex 转成 world target 和 apex clearance；不查询 World、不 trace、不接设备或 UI。随后再由独立阶段把该 projection 委托到现有 Arc InputAdapter route。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-11-thrown-weapon-input-choice-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-11-thrown-weapon-input-choice-session/Docs/Report/Dev.D.UE.0.0.10.P20.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-11-thrown-weapon-input-choice-session/Docs/Log/Dev.D.UE.0.0.10.P20.11.r0_log.md>
