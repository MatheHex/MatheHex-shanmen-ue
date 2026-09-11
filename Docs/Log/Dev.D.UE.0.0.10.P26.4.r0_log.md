# Dev.D.UE.0.0.10.P26.4.r0 Development Log

## 1. 目标

- 将 P26.3 已提交的防具抗性回执转换为一次可读 HUD 提示；
- 只读取同一个物品、Defense layer、Impact 与生命提交证据，不重算伤害；
- 阻止 `AlreadyCommitted` 或同一 ImpactId 产生第二次提示；
- 证明未配置抗性的 TrainingVest 不误报；
- 覆盖六类既有敌方攻击，并与现有灵盾反馈无重叠显示；
- 按改动文件推导回归，完成双目标构建并交付 Report/Log。

## 2. 基线与范围

- 基线：`cb97f8f21678c53c594b42ee597e931278918328`（P26.3）；
- 分支：`agent/0.0.10-p26-4-armor-resistance-impact-feedback`；
- 起始 tracked tree clean；103 个既有未跟踪用户条目保持未暂存；
- 不改 P26.3 catalog 数值、CombatCore、生命/物品权威、存档 schema、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. 反馈投影实现

新增 `demo_mapShanmenArmorResistanceImpactFeedback.h/.cpp`，定义不可变、非 UObject 的表现投影。
`TryProject()` 先清空输出，随后交叉验证：

1. attack executed、armor inspected、projection present；
2. vitality status 恰为 `Committed`；
3. request/result/守恒与 commit receipt 一致；
4. projected layer ID 无效或重复即失败关闭；
5. triggered/request/projected 三份 layer 的 item instance、operation、order 与 commit flag 一致；
6. triggered layer 带精确 `DefenseArmor` tag 且 prevented damage 为正；
7. canonical definition 仍有 `DamageResistance` 语义。

通过后保存 exact ImpactId、armor item instance、definition/display name 与四个伤害值，显示文本由
实际 prevented damage 生成。该类型不写任何产品状态。

## 4. 宿主与 HUD 接线

GameMode 新增单一发布函数，并在 basic melee、melee dash、ranged projectile、heavy sector、Boss
shape、Boss volley 六条已存在的 incoming Impact 路径上调用。

PlayerController 保存最近一次投影和 1.25 秒到期时间；无效投影或同 ImpactId 返回 false。HUD 复用
Canvas panel：无灵盾反馈时 y=206，有灵盾反馈时 y=248。没有新增 Widget、Actor、Tick、Timer、
damage call、inventory call 或随机源。

## 5. 自动化测试

在现有真实战斗协调器夹具中加入两条测试：

- `CanonicalCommitAndReplayFence`：Tier1 robe 的 1.0 physical hit 生成 0.1 prevented、0.9 final/
  applied、精确中文提示；纯投影两次完全一致；`AlreadyCommitted` 清空输出；真实 PlayerController
  接受一次并拒绝同 Impact 重复；
- `NoOpArmorFence`：TrainingVest 的适配结果为成功 `NotApplicable`，生命仍提交 1.0 damage，投影
  与可见提示均不存在。

专项最终结果 2 Success / 0 Fail，协调器组 21 Success / 0 Fail，完整 0.0.10 套件 1,320
Success / 0 Fail。

## 6. 回归映射扩展与首错

新文件规则要求 Feedback、Coordinator、Armor item adapter/projection、CombatCore、Items、完整
0.0.10 与 legacy ItemUseAndArmor。GameMode、PlayerController、HUD 与 Coordinator 规则也加入新组，
使未来任何相关宿主改动都必须带反馈证据。

首次自测在 MainHUD 旧正向夹具处停止：规则已正确要求新组，但夹具未提供覆盖它的 broad log。
初始日志保留 142 个已通过项目，SHA-256：
`DC9E9A768615CA6CE2A5B6BE8A7FECBC1CF05D3A75CE3A3AA6B2E15A6F9FC1B1`。

修复夹具后增加新规则的 expected-pass / expected-fail 两个样本，最终 `457/457` PASS，SHA-256：
`8C81F7F329B2AD5EFF7334A56D12CFDA10828E94009F32C75B6E1E521AD9EBEF`。

## 7. 自动化结果

| Group | Success | Fail | Fatal | Completion | SHA-256 |
|---|---:|---:|---:|---:|---|
| `ArmorResistanceImpactFeedback` | 2 | 0 | 0 | present | `45445B5E63BD1ABB5C799E61CDA1B3DB6C32246E5FA0716C65C2EA9C28754BB1` |
| `CombatRunCoordinator` | 21 | 0 | 0 | present | `DDB9279912E6DF46EF67E779A70729CB36BECBA953C94A0C0AC80DE6D07CB917` |
| `Shanmen.0_0_10` | 1,320 | 0 | 0 | present | `2BBEE9173E073904D03474EFDB57765917BF0D43A0DEE8264FBBD9D5D48DAF3E` |
| `demo_map.InputRestore` | 101 | 0 | 0 | present | `CF301AC5C0496BCF55003720E7381E08E56730BF9ED6AC8DC2906024B3565C3E` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | present | `0A56D71A7F49D9B49A1F4FAA641727A7A3F729DF3B4A8117FD546D4CFB2F52C5` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | present | `F14A967701004354C1045536087754CCC6CE0B6FA737EBB670E2C112824A3A4C` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | present | `9972849F12BC974CB9EB989437265AA4ED1EA1E913D63351FE02DB43474688A4` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 0 | present | `42769FB1C1E36FF92F60B12C6CE415C594E1175233B40DC687DE61FB57EA08E3` |

聚焦日志包含在完整套件中，不重复计数。覆盖门所用 6 个日志合计 1,537 Success / 0 Fail。

## 8. 改动驱动覆盖与静态检查

最终 12 个提交路径命中 6 条规则，要求 99 个 groups；覆盖门输出：
`REGRESSION_COVERAGE: PASS Changed=12 Rules=6 Required=99 Logs=6`，SHA-256：
`391A542D1ABFB037207ADFFE76807036632F0377E9EBF151FB50ED4C46EDA155`。

静态结果：

- regression map：schema 1，252 rules，JSON 解析 PASS；
- `git diff --check`：PASS；
- GameMode 产品发布调用：精确 6 处；
- 新增投影文件与 added runtime lines：无新增 Tick/Timer/RNG/ApplyDamage/SpawnActor/NewObject；
- 新投影无 UObject、UWorld、AActor、第二套伤害或物品权威。

## 9. 构建、二进制与未执行项

| Build | Native result | Duration | SHA-256 |
|---|---|---:|---|
| 首次 Editor，50 actions | Succeeded / 0 | 29.05s | `97A80CB3C5D4AC25572AC15F29667072845AF39A1FB5A1EAFD02B8662C3E2CF5` |
| 测试扩展后 Editor，33 actions | Succeeded / 0 | 23.76s | `7A4E1DA0D437F5BCDF1EFCF6286AEE5F75B11A3C14C0EE251ED1DF18BF08CA4F` |
| 最终 Game，49 actions | Succeeded / 0 | 36.11s | `79E44E4BE1EB04F216DF4E6BA2413AB923ADB62570ECA346D381487D8BA6DB58` |
| 最终 Editor，up to date | Succeeded / 0 | 0.90s | `B49F1DCFDABC0AF2572D6F256E2B6CF34D18C197FB5742887A4723AFBB090958` |

最终二进制：

- `UnrealEditor-demo_map.dll`：19,298,304 bytes，SHA-256
  `2C6C8790F1B22881CD47B1C04AA6A72B1FC2694EA2E6A49EDEB3EEFBA4647BC8`；
- `demo_map.exe`：359,980,032 bytes，SHA-256
  `301CAFF9B93276BA966796B2F051BDE3BF28DEF84DC8B221FD307F74DA1D2909`。

未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
原始证据位于 `Saved/Codex/P26.4`，不进入 Git。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenArmorResistanceImpactFeedback.h`
2. `Source/demo_map/demo_mapShanmenArmorResistanceImpactFeedback.cpp`
3. `Source/demo_map/demo_mapPlayerController.h`
4. `Source/demo_map/demo_mapPlayerController.cpp`
5. `Source/demo_map/demo_mapGameMode.h`
6. `Source/demo_map/demo_mapGameMode.cpp`
7. `Source/demo_map/demo_mapHUD.cpp`
8. `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
9. `Scripts/ShanmenRegressionMap.json`
10. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
11. `Docs/Report/Dev.D.UE.0.0.10.P26.4.r0_report.md`
12. `Docs/Log/Dev.D.UE.0.0.10.P26.4.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-4-armor-resistance-impact-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-4-armor-resistance-impact-feedback/Docs/Report/Dev.D.UE.0.0.10.P26.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-4-armor-resistance-impact-feedback/Docs/Log/Dev.D.UE.0.0.10.P26.4.r0_log.md>
