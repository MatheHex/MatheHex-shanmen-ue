# Dev.D.UE.0.0.10.P21.14.r0 Development Log

## 1. 基线与目标

- base：`b057f6c64b15bf0cff0c4696ee478575aa322d3b`（P21.13 flying-sword impact HUD feedback）；
- branch：`agent/0.0.10-p21-14-flying-sword-impact-cue`；
- 目标：让同一份 canonical committed-impact 快照驱动飞剑已有动态材质，在返航阶段给出直接命中反馈；
- 边界：不新增伤害、目标、生命值、计时、Actor、材质资产或 HUD 权威，不启动 UI/PIE/产品 executable。

## 2. 审计与决策

P21.13 已完成 RunHost delivery → current Actor snapshot → HUD 文案闭环。Actor 同时已有由 `GetResolvedPresentationColor()` 统一解析的阶段色、threat 色和一个动态材质，因此无需建立新表现组件。

采用最小单向投影：committed impact 只在现有 flight read model 为同一 Activation 且阶段为 `Returning` 时拥有材质优先级。反馈寿命复用 Activation 生命周期，不增加 Timer。

## 3. 实现

在 `demo_mapShanmenControlledWeaponActor` 中加入三个只读颜色常量：零伤害浅色、正伤害亮金色、击破红橙色。

新增 `IsCommittedImpactCueActive()`，要求：

- 当前 Actor 已持有自洽 committed-impact 快照；
- flight read model 有效；
- 当前 phase 严格为 `Returning`。

`TryPresentCommittedImpactFeedback()` 首次接受回执后刷新表现。原 `RefreshThreatPresenceCue()` 改名为 `RefreshPresentation()`，覆盖构造、threat 更新/清除、flight read model 更新、impact 接受和停用清理，名称与职责保持一致。

## 4. 优先级

颜色解析优先级为 committed return impact → threat → phase → idle。命中回执可能在 GameMode 发布本 tick 最终 Returning read model 前到达；此时刷新仍保持 Directed 色。随后 Returning read model 到达并再次刷新，命中色才生效。

Threat 点光源继续严格由 `bThreatPresenceCueActive` 控制。此次只改变已有材质的 `Color` / `BaseColor`，避免把“命中”伪装成“附近存在目标”。

## 5. 测试开发

扩展 `ControlledWeaponRunHost.BlockingContactTerminal` 的真实 Actor/敌人/伤害链用例：

- 保存 Directed 初始表现色；
- 接受并幂等重放权威 delivery 后，断言 cue 尚未激活且颜色不变；
- 发布 Returning read model 后，断言 cue 激活且颜色精确为亮金色；
- 发布下一 Activation 后，断言 feedback/cue 均清空并离开命中色。

没有创建绕过真实 delivery 的测试专用后门，也没有复制 vitality receipt 私有构造逻辑。

## 6. 执行与修正

Editor 初始编译首次通过，12 actions / 21.88s / native 0。Controlled-weapon 专项首次通过，`62 Success / 0 Fail`；完整 0.0.10 首次通过，`1247 Success / 0 Fail`。

产品代码、测试和构建没有首次失败。第一次生成回归门 probe 证据时，输出目录手误写成 `P21..14`，命令在形成有效 gate 证据前被 PowerShell 拒绝；改正为既有 `P21.14` 目录后一次通过。该操作错误未修改文件、未运行产品、未改变测试选择，最终 gate 另存为独立完整日志。

## 7. 自动化与门禁

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P21.14_focused_controlled_weapon_initial.log` | 62/0 | 332,544 | `19325B6B7195C4E5F16B9E940C9933AA3A846A8FFC8739ACAC746473EA3CA619` |
| `P21.14_full_0_0_10.log` | 1247/0 | 1,907,220 | `8291ADF0520FBBA41054083024B2BC3837C655898C80249ED71F93AD18526E5F` |
| `P21.14_regression_gate_final.log` | PASS 3/3/17/1 | 2,033 | `31B5B16BACCE2A405D20C89342CF1B46EEA08B180F471CD328F2970AEE9AFED5` |
| `P21.14_regression_gate_selftest.log` | PASS 437/437 | 43,073 | `E55EB7830CB49BA7786AC919E8A2DEAD485D91D376952090B7DF9394F55ECCA0` |

覆盖门读取三个实际改动路径，并由完整 `Shanmen.0_0_10` 日志覆盖映射得到的 17 个 required groups；没有用专项组冒充全量依赖。

## 8. 构建与静态检查

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P21.14_editor_build_initial.log` | 12 actions / PASS | 2,882 | `711984CF904703AC1E4DE229820E46A24059B06F48D15A0DEDA941939DF23A61` |
| `P21.14_game_build_final.log` | 11 actions / PASS | 2,786 | `72B6BBD8760D40B19CA452D28DE8B0ADEA51CD5A91ACDA525C68AC8B76A3F487` |
| `P21.14_editor_build_final.log` | 0 actions / PASS | 1,021 | `338CB16CA1E8509DF390AE6A2B5F6328763FC056494A5616C7685446C75BB7F5` |

`git diff --check` native 0。实现/测试为 3 files、`+45 / -9`；新增生产行无 Timer、SetTimer、RNG、ApplyDamage、SpawnActor 或 Destroy。最终 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程数均为 0。

## 9. 提交边界

精确提交以下五个文件：

- `Source/demo_map/demo_mapShanmenControlledWeaponActor.h`；
- `Source/demo_map/demo_mapShanmenControlledWeaponActor.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`；
- `Docs/Report/Dev.D.UE.0.0.10.P21.14.r0_report.md`；
- `Docs/Log/Dev.D.UE.0.0.10.P21.14.r0_log.md`。

103 个用户原有 untracked 文件不暂存；raw evidence 保留于 `Saved/Codex/P21.14` 且不进入 Git。未修改 Content、地图、Engine、Windows、存档 schema、物品权威、Impact resolver 或 vitality commit 数学。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-14-flying-sword-impact-cue>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-14-flying-sword-impact-cue/Docs/Report/Dev.D.UE.0.0.10.P21.14.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-14-flying-sword-impact-cue/Docs/Log/Dev.D.UE.0.0.10.P21.14.r0_log.md>
