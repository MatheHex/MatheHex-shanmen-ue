# Dev.D.UE.0.0.10.P20.11.r0 Development Log

## 1. 目标与基线

- 基线：`60d5be120f2f8b3395ca93bbb973907226a27310`（P20.10）；
- 分支：`agent/0.0.10-p20-11-thrown-weapon-input-choice-session`；
- 目标：让一个 consumer-owned session 独占 P20.10 choice state，并成为 GameMode/lifecycle 的唯一 trajectory 配置来源；
- 约束：不增加设备输入、UI、World 查询、轨迹预览、库存、伤害或 projectile 执行。

## 2. 接入前审计

P20.10 已冻结 command/state/reducer 语义，但 GameMode 仍单独持有 `ConfiguredThrownWeaponTrajectoryKind`。继续在该字段旁累积 target/apex 会形成第二份状态权威，也无法统一 revision、replay 和 active Run fence。

审计确认 P20.9 的 lifecycle 只需 typed trajectory 即可启动；GameMode 已持有唯一 Run coordinator 与 lifecycle。因此本轮以 P20.10 state 替换旧字段，无需改 inventory、controller、host、world delivery 或 projectile。

## 3. Session 契约

session 构造 canonical initial state，并仅公开 const read model。每次 `Submit` 先调用 P20.10 reducer，保留其精确 reduce status。

`Reduced` 候选只有在 combat Run 非活跃且 lifecycle 为空时提交。active Run 优先返回 `CombatRunActive`，stale lifecycle 返回 `ProductLifecycleNotEmpty`；拒绝 result 不携带有效 state，内部 state ID/revision 不变。

`Replay` 与 `NoChange` 不会改变 choice，因此在锁定状态仍可确认，避免重试方因 Run 恰好开始而把已成功命令误判失败。

## 4. GameMode 组合

GameMode 移除独立 trajectory 字段，新增唯一 session 成员。typed command 入口把真实 coordinator/lifecycle 状态传给 session；read-only getter 暴露 canonical state。

兼容 `TryConfigureThrownWeaponTrajectory` 按当前 revision 捕获 typed selection，提交后仅把失败诊断返回旧调用方。成功 no-op/replay 继续清空 diagnostic。

Run activation 从 session state 读取 trajectory。`RunBound` 增加 choice state ID/revision，既能核对配置身份，也不复制 target/apex 或 lifecycle 数据。

## 5. 自动化扩展

新增 `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceSession` 4 项：

- `LifecycleAndReadModel`；
- `RunAndLifecycleFences`；
- `ReductionFailurePropagation`；
- `DeterminismAndModeReset`。

覆盖 exact replay 与 fresh no-op 锁内确认、active/stale 双 fence、拒绝后可恢复提交、Arc-only state 防护、command invalid/revision mismatch 透传、等价向量序列同 ID、不同序列异 ID，以及 Straight 转换清除 Arc 状态。

既有 GameMode config 测试改为同时验证 typed submit、read model、last command ID、revision、replay 和兼容 no-op；InputAdapter exact 保持 9 项。

## 6. 首次验证

- Editor initial：28 actions / native 0，SHA-256 `C7B072490DAFBE0979F30B1A39767B8EBFFBBEDBA982773F0D95CD612EA9EF31`；
- InputChoice aggregate：9/0，SHA-256 `93B920744EBE09B38C6C2D7F976754F65B225C7ECB843F1B0E142C0D229ECC02`；
- InputChoiceSession exact：4/0，SHA-256 `9EB3C303ACC2A44472B2379EC2DB630D0C8B495EE8579834E138B2FC25C670E8`；
- InputAdapter exact：9/0，SHA-256 `F95349C508D97FE896BC19310B5673DA9842CCE657F128A324A48BCDF9E80853`。

所有首次验证均通过；没有失败重试或被覆盖的失败日志。`Product.ThrownWeaponInputChoice` 是父组，因此其 9 项包含 reducer 5 项与新 session 4 项。

## 7. 最终自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `input_choice_final.log` | 9/0 | `93B920744EBE09B38C6C2D7F976754F65B225C7ECB843F1B0E142C0D229ECC02` |
| `input_choice_session_final.log` | 4/0 | `9EB3C303ACC2A44472B2379EC2DB630D0C8B495EE8579834E138B2FC25C670E8` |
| `input_adapter_final.log` | 9/0 | `F95349C508D97FE896BC19310B5673DA9842CCE657F128A324A48BCDF9E80853` |
| `full_0_0_10_final.log` | `872/0` | `EEBE88F5351D5390F94A87F5830AA5525CD81E24AB5071E349ACD158D5EACF6E` |
| `legacy_v3_attributes_final.log` | `4/0` | `0F7260869627FA79F75519048A78ADD47D57C745BF124FFB54B7ED1040C8BC2A` |
| `legacy_enemy_skill_final.log` | `44/0` | `896960933E7A6E12ADED9BA287427470F1793A08DB29A27C5D0C12D88EB71A30` |
| `legacy_v2_ranged_final.log` | `22/0` | `00D12270DF253176ABF14B5CC3C3C78267506FAE14AFFCD875B8F3EFA9F835C6` |
| `legacy_item_armor_final.log` | `46/0` | `6D9ECDF72D2CB1FDB24D85ADECDF05F87F888BFE65BA30BFDAEF5412FEA71884` |

证据审计：`PASS Logs=8 RecordedSuccess=1010 Invalid=0`；SHA-256 `9A3DDCB33D4540776072ADD94A25BD3D62279D50C31A68539B4B91C34BDA898F`。全量是一个连续进程；既有 SwordRhythm 长测试和 `generate_204` 环境警告未造成失败或拆组。

## 8. Changed-file regression 与边界

新增 session 映射要求 session exact、InputChoice aggregate 与 broad full；M01 GameMode 映射也要求两组 choice 契约。流程自测正反例最终 315/315，SHA-256 `58E2FE22EF62C820A59B5AFF7D21063DF0A98F4D9C75CC5EB14FA72457869E28`。

```text
REGRESSION_COVERAGE: PASS Changed=8 Rules=3 Required=55 Logs=8
```

- gate SHA-256：`78D744CD98A82449EAE77D48E6FF9E9D9F4E70A6346A255CC1852E22BDEDC1F3`；
- production boundary：`PASS Files=4 AddedLines=221 Matches=0`；
- boundary SHA-256：`EDC248E07613EC2D8A996B45B1B9740B8183F6D99D1740B717BBDC767B22347C`；
- `git diff --cached --check`：`PASS / native 0`；证据 SHA-256 `D827FF1E4A8A3C80AF883BC3FBFFC7B55C12EF0FC4D43C89BAAFE167E7EA44D1`；
- 长期未跟踪文件不纳入暂存、提交或推送。

## 9. 构建与产物

- final Editor：`26 actions / native 0`，SHA-256 `1F3CAB169234638982BB126BBB2C91209F87F75B53F3FC0139E690A30926978D`；
- final Game：`27 actions / native 0`，SHA-256 `F1EB2C34F227D3AE1BEF798E8341187C95AFBDA4205939DFBA3742DC9F8663C8`；
- `demo_map.exe`：`357242368` bytes / `151B97DC48720ECB0B69F0092573CC1CE61CD1E4E76240ABAF53ABA537023755`；
- `UnrealEditor-demo_map.dll`：`15915008` bytes / `5E6F60026D74DDBC6D591FB7714937BAAE102AC880A3F95BB9A4EDC1EA12D834`。

## 10. P/F 边界与后续判断

本轮证明 choice state 的单一所有权、锁定语义、GameMode read model 与 lifecycle trajectory 冻结。它不证明 target/apex 的世界映射、设备输入或表现层。

P20.12 建议实现纯值 Arc choice projection：从 caller-supplied source basis 和冻结策略映射 normalized target/apex 到 world target/clearance；继续不查询 World、不 trace、不接设备或 UI。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-11-thrown-weapon-input-choice-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-11-thrown-weapon-input-choice-session/Docs/Report/Dev.D.UE.0.0.10.P20.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-11-thrown-weapon-input-choice-session/Docs/Log/Dev.D.UE.0.0.10.P20.11.r0_log.md>
