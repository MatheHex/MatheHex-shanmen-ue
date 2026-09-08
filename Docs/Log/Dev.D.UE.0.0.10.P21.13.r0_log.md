# Dev.D.UE.0.0.10.P21.13.r0 Development Log

## 1. 基线与目标

- base：`7a46caa44c71a53d412b83280fb6baf92e17d68f`（P21.12 contextual flying-sword input hints）；
- branch：`agent/0.0.10-p21-13-flying-sword-impact-feedback`；
- 目标：让飞剑在完成真实权威伤害提交后，通过既有 HUD 状态行给出一次明确命中结果；
- 边界：复用现有 RunHost、world delivery、飞剑 Actor 与 HUD planner，不创建第二套伤害、目标、生命值、计时或面板权威。

## 2. 路径审计与实现决策

P21.12 已让玩家看到飞剑阶段、近身威胁与当前操作键，但真实阻挡接触完成后只有目标生命值变化和内部计数，HUD 没有回答“是否命中、造成多少、是否击破”。现有 `Fdemo_mapShanmenControlledWeaponWorldDeliveryResult` 已包含完整 Impact 请求与生命值提交回执，因此最短闭环是保留并投影这份事实，而不是新增命中检测或重新算伤害。

采用单向路线：

```text
RunHost canonical delivery
  -> DirectedTimelineResult.DeliveredImpacts
  -> GameMode current-lifecycle filter
  -> current ControlledWeaponActor presentation snapshot
  -> existing HUD threat/state planner
```

## 3. 实现

### 3.1 保留精确投递

`Fdemo_mapShanmenControlledWeaponDirectedTimelineResult` 新增 `DeliveredImpacts`。RunHost 仅在 `Delivery.IsDelivered()` 时移动保存结果并增加计数；terminal no-op 不产生新项。

`IsSuccess()` 同时验证数量、Run、物品有效性、目标、Impact receipt 和目标 receipt 身份，并拒绝同一批次中的重复 Impact ID。表现消费者因此拿到的是已验证原始交付，不是根据计数合成的近似事件。

### 3.2 GameMode 单向路由

定向时间线成功且有投递时，GameMode 取得当前 world lifecycle Actor 与物品 ID。只有来源物品等于当前 lifecycle item 且提交状态严格为 `Committed` 的项才可投影；`AlreadyCommitted` 不产生第二次玩家反馈。拒绝路径写入带 Run、Item、Activation、Impact、Target 和 Actor 的结构化错误日志。

### 3.3 Actor 表现快照

Actor 新增 `TryPresentCommittedImpactFeedback()`，要求：

- delivery 自身成功且 commit status 为 `Committed`；
- Actor 的 Run、Item、SourceActor 与请求完全一致；
- Activation、Target、Impact receipt 身份一致；
- 实际伤害与提交后生命值有限且非负；
- 已存在 flight read model 时必须属于同一 Activation。

首次接受后只保存 Impact/Activation/Target ID、实际伤害和生命值；精确重放幂等成功，任一字段冲突失败。新的 Activation read model 或产品停用会清空快照。

### 3.4 既有 HUD 文案

HUD 将 Actor 快照作为额外输入传给原 `ControlledWeaponThreatReadout` planner。旧签名保留为无反馈兼容入口。planner 仅在 Returning 接受反馈，并生成：

- 正伤害且目标未归零：`命中 -N.N`；
- 正伤害且目标归零：`击破 -N.N`；
- 已提交零伤害：`未造成伤害`。

隐藏反馈必须保持零值，跨阶段、NaN、负值和“零伤害却击破”全部失败关闭。`IsValid()` 可重建完整文本，`Matches()` 纳入反馈字段。

## 4. 测试开发

真实阻挡接触测试 fixture 从通用 `AActor` 改为产品实际 `Ademo_mapShanmenControlledWeaponActor`，并继续使用真实 enemy identity/vitality commit 路径。新增断言覆盖：

- 时间线保留一项自洽 canonical delivery；
- 来源物品、目标、fresh commit 与目标生命值差值一致；
- 同一 Actor 接受一次并幂等接受精确重放；
- Returning plan 输出精确命中文案；
- 下一 Activation 清除反馈；
- terminal 后续 tick 的 `DeliveredImpacts` 为空。

新增 planner 专项 `ImpactFeedback` 覆盖命中、击破、零伤害、屏外组合、阶段 fence、复用输出清空、隐藏残留和非有限值。

## 5. 首次失败与修正

初始 Editor 构建为 62 actions、54.99s、native 0。首轮 focused 执行 `61 Success / 1 Fail`，失败断言为 `the timeline preserves the exact canonical delivery`。

该断言最初把多项身份、状态和默认 `FMath::IsNearlyEqual` 浮点比较合并。拆成逐字段断言后，Run、Item、Target、Impact receipt、commit status 全部通过；隔离用例唯一失败变为 `the preserved damage matches target vitality`。原因是回执伤害与 float 生命值相减在正常舍入后超过默认极小容差，而非 delivery 内容或生命值提交错误。

测试改为 `TestEqual(..., KINDA_SMALL_NUMBER)`；没有修改生产伤害值、投递身份或失败关闭规则。隔离复查 `1/0`，最终 focused `62/0`。两份失败日志和修正后日志均保留：

| Log | Success/Fail | Bytes | SHA-256 |
|---|---:|---:|---|
| `P21.13_focused_controlled_weapon_initial.log` | 61/1 | 332,932 | `ED5E156F62A0C953FB6EAA9D6DFD5921733D7A563FF9F1F0724DC6926DD7A3BE` |
| `P21.13_blocking_contact_diagnostic.log` | 0/1 | 262,407 | `5BE42AED1F3CB274A70D0033FED9C2FADCCB8C2D1CEDD514180290BCF822C7A1` |
| `P21.13_blocking_contact_diagnostic_2.log` | 1/0 | 262,030 | `83D816D0B3BFEF159B9EF89F8A7C39C5E8D81A2A8DD4AEC458132288B0CF569A` |
| `P21.13_focused_controlled_weapon_final.log` | 62/0 | 331,930 | `F6F0342EE3AFF582C7E9DED93DD75169C5D17F85D17F990725730404C3EC0444` |

## 6. 最终自动化与覆盖门

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.13_full_0_0_10.log` | `Shanmen.0_0_10` | 1247/0 | 1,907,406 | `A6EAB86B5BB792238EAF2B2D27C199909AAB08FC8A099BA9C35BE3969F34E5A9` |
| `P21.13_legacy_enemy_skill.log` | `demo_map.EnemySkillFramework` | 44/0 | 305,825 | `0902BB7976EDBD476270954A377DE57E8D93FBA967ED2DEA5E8B5A44B6068DA7` |
| `P21.13_legacy_input_restore.log` | `demo_map.InputRestore` | 101/0 | 395,101 | `AE81FC7FF8AA9A5E461E4E55C258F388094869EF87628381A1AAFDB63B08184F` |
| `P21.13_legacy_item_use_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | 309,576 | `32518FFA1D97C795F036CB7A5FDD02F21484FF3E6D46B0AE7D50A37999981F88` |
| `P21.13_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 284,501 | `840A61F6A8C6534385EED3E2349FC828B8B1DCB01E5DA879B66A320254D26E3F` |
| `P21.13_legacy_v3_attributes.log` | `demo_map.V3.Attributes` | 4/0 | 264,159 | `650069CB2379C0C0F61EFAA0F94001EF2020C257F60F913CBE1132526FCC2FE1` |

每份日志均只有一个实际 RunTests 命令和一个成功终止标记；Fail/Fatal/Unhandled/Ensure 为 0。完整 0.0.10 由 1246 增至 1247，原生退出码 0。

changed-file gate 使用 10 个实际改动路径与 full + 五个 legacy 日志，结果为 `PASS Changed=10 Rules=6 Required=83 Logs=6`。门禁自测为 `PASS 437/437`。

| Evidence | Bytes | SHA-256 |
|---|---:|---|
| `P21.13_regression_gate_final.log` | 10,143 | `4B9EA69A1A3FA430100F0D13EA0C75E6862FDC06DB95800D877BDDA79528ACAA` |
| `P21.13_regression_gate_selftest.log` | 43,073 | `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0` |

## 7. 构建与静态检查

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| `P21.13_editor_build_initial.log` | Editor initial | Succeeded / native 0 | 62 / 54.99s | 6,555 | `A37829AD8381BAE49053903A5053EA4C2279DC95414B91D6729270456E20E516` |
| `P21.13_editor_build_pre_focused.log` | Editor after test refinement | Succeeded / native 0 | 6 / 6.37s | 2,654 | `D9AB6F8786F5E4FCD2003E9D9D98BD023893453CC24F3137A9ADCC2FE22EA717` |
| `P21.13_game_build_final.log` | Game final | Succeeded / native 0 | 61 / 55.05s | 6,340 | `0A170AF742E84B6F4DA1ED86E6042624B4451EE7DF53DB4B4F24528A61D54EA4` |
| `P21.13_editor_build_final.log` | Editor final | Succeeded / native 0 | 0 / 0.98s | 1,021 | `E3AD98DC71094A7B84CCE49E49AFE77D6D75CD3DEE6CA66D1132F5817F49B5FF` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,531,520 bytes / SHA-256 `28FF761387F01C945E8E755F7E659BFFC07E6443F3F5E928775B9B5F246BE821`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,722,816 bytes / SHA-256 `4FA50090721467C8B733E17C0F220984F4358E273703FDEC532633DF1734BEFD`。

Static：

- implementation/test diff：10 files，`+540 / -24`；
- 8 个生产文件、331 条新增生产行；Timer/SetTimer/RNG/ApplyDamage/SpawnActor/Destroy 命中 0；
- `git diff --check` native 0，仅有 LF→CRLF 工作树提示；
- 最终 UnrealEditor、UnrealEditor-Cmd、demo_map 进程数均为 0。

## 8. 提交边界

本阶段只提交 10 个实现/测试文件、本 Report 与本 Development Log，共 12 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.13` raw evidence 不进入 Git。

未修改 Content、地图、Engine、Windows、输入 schema、玩家存档 schema、物品权威、Impact resolver 或生命值提交数学。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-13-flying-sword-impact-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-13-flying-sword-impact-feedback/Docs/Report/Dev.D.UE.0.0.10.P21.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-13-flying-sword-impact-feedback/Docs/Log/Dev.D.UE.0.0.10.P21.13.r0_log.md>
