# Dev.D.UE.0.0.10.P26.4.r0 Report

## 1. 结论

P26.4 已把 P26.3 的真实防具抗性结算回执接入现有玩家 HUD：一阶道袍实际抵消物理伤害时，
玩家会看到一次短时提示 `一阶道袍 · 抵消 0.1`。该文本来自同一 active-Run 物品证据、
Defense layer、Impact result 与生命提交回执，不重新计算减免，也不建立第二套伤害或 UI 权威。

同一 Impact 的重放不会二次提示；没有投放抗性的训练防具不会产生提示。六类现有敌方命中入口
均使用同一投影与发布函数，护盾提示同时存在时两条消息按既有 HUD 画布纵向排列。

## 2. 阶段范围

本阶段只完成“已发生的防具减免可被玩家读取”这一闭环：

- 不新增防具、伤害通道或平衡数值；
- 不改 P26.3 的 10% 物理抗性、CombatCore 公式、生命或物品权威；
- 不改存档 schema、地图、资产或输入绑定；
- 不新增 Widget、Actor、Tick、Timer、随机数或资源提交；
- 复用 `Ademo_mapHUD`、`Ademo_mapPlayerController` 与 M01 incoming Impact 路径。

## 3. 权威回执投影

新增 `Fdemo_mapShanmenArmorResistanceImpactFeedbackPresentation`。只有同时满足下列事实才可生成：

- 敌方攻击已执行且防具适配器确实检查了 active-Run ArmorSlot；
- 防具投影存在，生命提交状态严格为 `Committed`；
- Impact request 有效、result accepted 且满足伤害守恒；
- 生命回执携带同一个 ImpactId，requested damage 等于 Impact final damage；
- 实际触发层来自投影记录的精确 layer ID 与精确 armor item instance；
- 层为 `ReduceFraction`、`FShanmenDefenseOrder::Resistance`、带精确 `DefenseArmor` tag；
- 该被动层不要求资源提交，且实际 prevented damage 为正；
- definition 仍由 canonical catalog 解析，且具有 `DamageResistance` 产品语义。

投影保存精确 Impact、物品实例、定义、显示名、raw/prevented/final/applied damage。它只解释既有
回执，不拥有伤害、生命、库存或防具状态。

## 4. 产品路由与 HUD

`Ademo_mapGameMode::PublishArmorResistanceImpactFeedback()` 被接入六条真实玩家受击路径：普通近战、
近战突进接触、远程投射物、重型扇区、Boss 形状攻击与 Boss 齐射投射物。

PlayerController 只接受有效且 ImpactId 未出现过的提示，并使用现有 World time 保留 1.25 秒。
HUD 继续使用既有 Canvas/`DrawHUDPanel`：单独出现时位于 y=206；若灵盾命中反馈仍有效，则位于
y=248，避免两条消息覆盖。未创建第二个 UI 树或独立刷新时钟。

## 5. 幂等与 no-op 边界

canonical 测试从真实 Profile、物品权威与 prepared Run 执行 M01 普通近战，得到：

| Armor | Raw | Armor prevented | Final / applied | Visible feedback |
|---|---:|---:|---:|---|
| 一阶道袍 | 1.00 | 0.10 | 0.90 / 0.90 | `一阶道袍 · 抵消 0.1` |
| TrainingVest | 1.00 | 0 | 1.00 / 1.00 | 无 |

同一 committed 结果重复投影保持确定；把同一 delivery 重放为 `AlreadyCommitted` 时投影失败并清空
复用输出。真实 PlayerController 第一次接受提示，第二次同 ImpactId 拒绝，形成投影层与宿主层双重
重放防线。

## 6. 首错与修复闭环

首次回归映射器自测在已有 MainHUD “应通过”夹具处停止：HUD 新增了
`ArmorResistanceImpactFeedback` 必跑组，但该旧正向夹具没有提供覆盖它的 broad evidence。初始日志
停在 142 条已通过检查，SHA-256：
`DC9E9A768615CA6CE2A5B6BE8A7FECBC1CF05D3A75CE3A3AA6B2E15A6F9FC1B1`。

修复只更新流程夹具：MainHUD 正向样本加入完整套件证据，并为新文件规则补充一正一反自测。
最终映射器自测 `457/457` PASS。该失败未修改产品契约，也未被隐藏或删除。

## 7. 自动化证明

| Group | Success | Fail | Fatal | Log SHA-256 |
|---|---:|---:|---:|---|
| `ArmorResistanceImpactFeedback` | 2 | 0 | 0 | `45445B5E...754BB1` |
| `CombatRunCoordinator` | 21 | 0 | 0 | `DDB92799...CB917` |
| `Shanmen.0_0_10` | 1,320 | 0 | 0 | `2BBEE917...DAF3E` |
| `demo_map.InputRestore` | 101 | 0 | 0 | `CF301AC5...65C3E` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `0A56D71A...F52C5` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `F14A9677...3A4C` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | `9972849F...88A4` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 0 | `42769FB1...08E3` |

所有最终日志均有原生队列完成标记、0 Fail、0 Fatal 与进程退出码 0。聚焦组已包含在完整套件中，
不重复计入映射合计；完整套件加五组 legacy evidence 为 1,537 Success / 0 Fail。

## 8. 改动驱动回归、构建与静态检查

12 个最终提交路径命中 6 条规则，推导 99 个 required groups；完整 0.0.10 日志加五组 legacy
日志覆盖全部要求。覆盖门：`PASS Changed=12 Rules=6 Required=99 Logs=6`，SHA-256：
`391A542D1ABFB037207ADFFE76807036632F0377E9EBF151FB50ED4C46EDA155`。

映射器正反自测 `457/457` PASS，SHA-256：
`8C81F7F329B2AD5EFF7334A56D12CFDA10828E94009F32C75B6E1E521AD9EBEF`。

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| 首次 `demo_mapEditor`，50 actions | Succeeded | 0 | `97A80CB3...E2CF5` |
| 测试扩展后 Editor，33 actions | Succeeded | 0 | `7A4E1DA0...8CA4F` |
| 最终 `demo_map`，49 actions | Succeeded | 0 | `79E44E4B...DB58` |
| 最终 `demo_mapEditor`，up to date | Succeeded | 0 | `B49F1DCF...0958` |

`git diff --check` 与 regression map JSON 解析通过；映射为 schema 1 / 252 rules；六个产品攻击
调用点均存在；新增运行时代码的 scoped scan 未发现新增 Tick、Timer、RNG、ApplyDamage、
SpawnActor 或 NewObject。

## 9. P/F 边界与下一阶段

P 阶段已证明：权威防具回执到显示文本的确定性投影、精确 item/layer/Impact 身份、真实 M01 生命
提交、TrainingVest no-op、AlreadyCommitted 与控制器重复抑制、六类攻击接线、护盾并列布局、完整
0.0.10 和改动文件驱动回归均成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、
Cook 或 Package，因此不声明像素级布局、字体可读性或真实玩家体验已经验收。下一阶段可在明确授权的
F 边界中做可视验收，或继续扩展另一种已正式定义的防御来源；不得从本阶段推断新平衡数值。

## 10. GitHub 交接

基线提交：`cb97f8f21678c53c594b42ee597e931278918328`（P26.3）。
分支：`agent/0.0.10-p26-4-armor-resistance-impact-feedback`。只提交本阶段 10 个实现/测试/流程
文件、本 Report 与本 Development Log；103 个既有未跟踪用户条目保持未暂存，
`Saved/Codex/P26.4` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-4-armor-resistance-impact-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-4-armor-resistance-impact-feedback/Docs/Report/Dev.D.UE.0.0.10.P26.4.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-4-armor-resistance-impact-feedback/Docs/Log/Dev.D.UE.0.0.10.P26.4.r0_log.md>
