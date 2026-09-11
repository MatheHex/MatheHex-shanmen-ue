# Dev.D.UE.0.0.10.P26.2.r0 Report

## 1. 结论

P26.2 已把 P26.1 的 active-Run 防具抗性适配器接入敌方攻击命中玩家的唯一
`Fdemo_mapCombatRunCoordinator` 路径。每次 incoming Impact 先捕获玩家基础 Defense，再只读
投影当前 Run 冻结的精确 ArmorSlot 防具，随后才准备护盾/护心镜等资源型防御、武器格挡与
灵盾，最终交给纯 `ShanmenCombatCore` 结算并提交生命值。

当前 canonical 防具没有正式抗性元数据，因此有效已装备防具返回带证据的
`NotApplicable`，最终伤害与 P26.1 前完全一致。本阶段建立了真实产品路由和失败关闭边界，
没有擅自加入数值调参。

## 2. Incoming Impact 顺序

M01 敌方攻击现在按固定顺序执行：

1. 构造 Action、Candidate 与确定性 ImpactId；
2. 从玩家生命组件捕获基础 Defense；
3. 从唯一物品权威读取 active-Run correlation 与 ArmorSlot snapshot；
4. 校验 combat Run 与 item Run 一致，并执行 P26.0 纯抗性投影；
5. 用投影后的 Defense 准备资源型防御；
6. 组合武器格挡与灵盾；
7. 捕获生命值、纯结算、提交资源与生命值收据。

被动防具抗性因此位于基础属性之后、可消耗防御之前。它不进入 Reserve/Commit，不占用耐久、
充能或库存事务，也不会建立第三套防御权威。

## 3. Active-Run 身份围栏

`ProjectActiveRun()` 与 `ProjectFromEvidence()` 新增调用方期望的 `ExpectedActiveRunId`。
适配器除验证 durable correlation 自身有效外，还要求：

- correlation 的 `ActiveRunId` 精确等于 CombatRunCoordinator 当前 Run；
- TargetEntityId 与基础 Defense 均有效；
- exact ArmorSlot item、definition、DeploymentLock 与 content stamp 延续 P26.1 全部围栏；
- 所有检查成功后才返回投影后的 immutable Defense。

Run 不一致返回独立 `RunMismatch`，不能使用另一个 Run 的已部署防具保护当前战斗，也不能
在错误身份下继续资源准备。

## 4. 产品失败关闭

Coordinator 的攻击结果新增：

- `bArmorResistanceInspected`；
- 完整 `Fdemo_mapShanmenArmorResistanceItemResult`；
- `ArmorResistancePreparationFailed`。

只要 ready 物品权威被检查而抗性证据被拒绝，Action 即被中断，攻击不会生成有效 Impact，
不会交付生命值，也不会先创建护盾或护心镜资源预留。`IsExecuted()` 同时要求已检查的防具
结果成功，防止调用方把部分失败误判为已执行。

新增产品测试故意令 Combat coordinator 切换到 Run B、物品权威仍保持 Run A。结果精确为
`RunMismatch`，攻击、玩家生命值、已提交 Impact 数及完整物品权威 snapshot 均保持不变。

## 5. 当前目录零差异

HeartMirror 真实产品 fixture 现在同时部署 `TrainingVest` 与护心镜，并在连续两次敌方命中中
证明：

- ArmorSlot exact item 与 active Run 身份被读取；
- 每次抗性状态均为成功的 `NotApplicable`，保留有效防具证据且没有投影层；
- 第一次致死攻击仍只消耗护心镜并保留 1 点生命；
- 第二次独立攻击仍在充能耗尽后正常击败玩家；
- 本轮接线没有改变既有伤害、充能或幂等行为。

## 6. 自动化证明

聚焦组 `Shanmen.0_0_10.Product.CombatRunCoordinator` 首轮执行 20 项：20 Success / 0 Fail /
0 Fatal，原生完成，SHA-256：
`7CF9D8A9DAC5D6102AF6B787C44B36C0D2BCC75D9139EADA284EAEFF2765429E`。

新增与强化的关键证明：

| Test/fixture | 证明 |
|---|---|
| `ArmorResistanceRunFence` | 跨 Run 防具证据失败关闭，资源与生命值均不变 |
| `HeartMirrorProductLifecycle` | exact ArmorSlot 被实际路由，当前目录 no-op，资源防御行为不变 |
| `ArmorResistanceItemAdapter.IdentityFences` | 纯适配器直接拒绝错误 expected Run |

完整 `Shanmen.0_0_10` 回归执行 1,317 项：1,317 Success / 0 Fail / 0 Fatal，SHA-256：
`AF38D7A061E866A84509AD40FB9F5988E27FFB89195AAC72B33F29B7E3731F36`。

## 7. 改动文件驱动回归

Coordinator 映射新增 `ArmorResistanceItemAdapter` 与 `ArmorResistanceProjection`，最终门根据
7 个实际改动路径推导 18 个必跑组；5 份最终日志覆盖全部要求：

| Executed group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10` | 1,317 | 0 | `AF38D7A...731F36` |
| `demo_map.V3.Attributes` | 4 | 0 | `2DFE480F...9BB8AA` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `D5D23ED3...AC53A` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `76226C41...586C58` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `317BC727...BFBD3` |
| **最终映射合计** | **1,433** | **0** | **全部原始日志保留** |

覆盖门：`PASS Changed=7 Rules=2 Required=18 Logs=5`，SHA-256：
`ECA6CD91EFBAF40B583D2CDFF6A75D12CE295A40D156DD790A089DA5F2F3A336`。
映射器正反自检 455/455 PASS，SHA-256：
`2B7C907F801E411CF76A642C37D32FF02E9D08058117F9826D13D69F94888B2B`。

## 8. 构建与静态检查

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| 首次 `demo_mapEditor Win64 Development` | Succeeded，222 actions | 0 | `5427800E...20E7D8` |
| 最终 `demo_map Win64 Development` | Succeeded，221 actions | 0 | `D5CA2259...F231AC` |
| 最终 `demo_mapEditor Win64 Development` | Succeeded，up to date | 0 | `69AC3217...D3BF99` |

- `git diff --check`：PASS；Regression Map：schema 1 / 251 rules；
- 防具适配器对 World/Actor/Component、ApplyDamage/RNG、旧可变物品权威及物品写操作：0 命中；
- 静态证据 SHA-256：`7F160E657D1E7623CFAA17906E091F5A1DCDFE10DF46FE1627613BA173A9BFBC`；
- `UnrealEditor-demo_map.dll`：19,266,048 bytes，SHA-256
  `3CC8ACF5CF96E967FB932112919EF4A20C259F88BB2B0574A3BBC2D438990BC7`；
- `demo_map.exe`：359,952,384 bytes，SHA-256
  `1EF4F9EEC3685DD2B5861A252D1823A6FE461AF1862403A17C33B4977A062511`。

## 9. P/F 边界与下一阶段

P 阶段已证明：真实敌方 incoming Impact 接线、基础/被动/资源防御顺序、active-Run 身份围栏、
错误 Run 失败关闭、当前目录零差异、完整 0.0.10 与所有改动映射回归、Editor/Game 构建成立。

本轮没有给任何防具增加正式抗性数值。后续 P26.3 可为一个非训练用 canonical 防具加入首个
保守、标签限定的抗性配置，并证明匹配伤害标签生效、不匹配标签无效、receipt 守恒和旧存档
兼容；数值投放与本轮基础设施接线保持独立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package，不声明已有玩家可见抗性效果。

## 10. GitHub 交接

基线提交：`8d1df9645de9695ccb0ea9767faa0b71a0c8b063`（P26.1）。
分支：`agent/0.0.10-p26-2-armor-resistance-impact-route`。只提交本阶段 6 个实现/测试文件、
1 个回归映射、本 Report 与本 Development Log；103 个既有未跟踪用户条目保持未暂存，
`Saved/Codex/P26.2` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-2-armor-resistance-impact-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-2-armor-resistance-impact-route/Docs/Report/Dev.D.UE.0.0.10.P26.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-2-armor-resistance-impact-route/Docs/Log/Dev.D.UE.0.0.10.P26.2.r0_log.md>
