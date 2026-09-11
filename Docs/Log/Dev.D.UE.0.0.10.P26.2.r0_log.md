# Dev.D.UE.0.0.10.P26.2.r0 Development Log

## 1. 目标

- 把 P26.1 active-Run ArmorSlot 适配器接入真实敌方 incoming Impact；
- 固定基础 Defense、被动抗性、资源型防御、主动防御与纯结算的顺序；
- 阻止跨 Run 防具证据进入战斗，并保证失败发生在任何资源预留之前；
- 保持未配置抗性的现有防具、护心镜和旧战斗行为不变；
- 按实际改动文件运行回归、双目标构建并交付 Report/Log。

## 2. 基线与范围

- 基线：`8d1df9645de9695ccb0ea9767faa0b71a0c8b063`（P26.1）；
- 分支：`agent/0.0.10-p26-2-armor-resistance-impact-route`；
- 起始 tracked tree clean；103 个既有未跟踪用户条目保持未暂存；
- 修改 CombatRunCoordinator、P26.1 adapter、各自测试与回归映射；
- 不修改 canonical 物品数值、存档 schema、CombatCore resolver、GameMode、PlayerController、
  UI、地图或资产。

## 3. 路由设计

在 `ExecuteM01EnemyAttack()` 捕获基础 Defense 后，若 GameInstance 的唯一物品权威 ready：

1. 标记防具抗性已检查；
2. 以 Coordinator 当前 RunId、玩家 EntityId 与基础 Defense 调用 `ProjectActiveRun()`；
3. 非成功状态立即中断 Action 并返回 `ArmorResistancePreparationFailed`；
4. 成功后用 `Projection.Defense` 替换后续组合输入；
5. 再调用既有 DefenseResourceAdapter；
6. 最后叠加武器格挡、灵盾并进入纯 Impact 结算。

这样被动层不参与资源事务；若其身份或证据错误，后续预留根本不会发生。

## 4. 契约强化

`ProjectActiveRun()` 与 `ProjectFromEvidence()` 增加 `ExpectedActiveRunId`，并新增状态
`RunMismatch`。输入验证要求 expected Run、target 和基础 Defense 有效；durable correlation
有效后还必须与 expected Run 精确相等，才继续 P26.1 的 item/definition/deployment/content
围栏。

攻击执行结果保存完整适配器收据。`IsExecuted()` 在 `bArmorResistanceInspected` 为 true 时
强制要求 `ArmorResistance.IsSuccess()`，避免错误结果被上层当成正常命中。

## 5. 产品自动化

HeartMirror fixture 增加真实 `TrainingVest`：Profile preparation 同时把它部署到 ArmorSlot，
并验证 active-Run correlation 冻结精确实例。两次敌方攻击都确认 adapter 被调用、状态为
`NotApplicable`、evidence 指向该 Armor 与同一 active Run，同时既有护心镜生命值和充能
断言保持成立。

新增 `ArmorResistanceRunFence`：

- 先正常建立 item Run A；
- 结束并把 CombatRunCoordinator 重新绑定到 Run B；
- 在 item authority 仍是 Run A 时执行敌方攻击；
- 断言返回 `ArmorResistancePreparationFailed/RunMismatch`；
- 断言没有 Impact、没有 delivery、生命值和 revision 不变、已提交 Impact 为 0；
- 断言攻击前后完整 item authority snapshot 相等。

P26.1 纯 adapter 的 IdentityFences 同时新增直接错误 expected Run 用例，其余调用点显式传入
correlation 的 active Run。

## 6. 自动化结果

| Group | Success | Fail | Fatal | Completion | SHA-256 |
|---|---:|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.CombatRunCoordinator`（首轮聚焦） | 20 | 0 | 0 | 1 | `7CF9D8A9...65429E` |
| `Shanmen.0_0_10` | 1,317 | 0 | 0 | 1 | `AF38D7A0...731F36` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | 1 | `2DFE480F...9BB8AA` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 0 | 1 | `D5D23ED3...AC53A` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | 1 | `76226C41...586C58` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | 1 | `317BC727...BFBD3` |

聚焦 20 项被完整套件包含，不重复计入最终映射总数。最终改动文件回归为 1,433 Success /
0 Fail。

## 7. 回归映射与流程自检

`CombatRunCoordinator` 映射新增两个直接依赖组：

- `Shanmen.0_0_10.Product.ArmorResistanceItemAdapter`；
- `Shanmen.0_0_10.Product.ArmorResistanceProjection`。

实际 7 个改动路径命中 2 条规则，推导 18 个 required groups；Broad
`Shanmen.0_0_10` 覆盖其子组，4 个 legacy `demo_map.*` 组由独立日志覆盖。覆盖门结果：
`PASS Changed=7 Rules=2 Required=18 Logs=5`，SHA-256
`ECA6CD91EFBAF40B583D2CDFF6A75D12CE295A40D156DD790A089DA5F2F3A336`。

映射脚本正反自检 455/455 PASS，SHA-256
`2B7C907F801E411CF76A642C37D32FF02E9D08058117F9826D13D69F94888B2B`。

## 8. 构建与静态检查

| Build | Native result | SHA-256 |
|---|---|---|
| 首次 Editor，222 actions | Succeeded / 0 | `5427800E5A94FC4B2D64E840B022C6F033A1DAAA89D65134CB35CD1C9320E7D8` |
| 最终 Game，221 actions | Succeeded / 0 | `D5CA2259B71203B15E70798916C98E0867A038DB02DA9846449D1CE318F231AC` |
| 最终 Editor，up to date | Succeeded / 0 | `69AC32179D9F4FB2C9ABD64D2F06ADF736D95DCA0498A63A95F544E52DD3BF99` |

静态检查：`git diff --check` PASS；Regression Map schema 1 / 251 rules；adapter 的 World/Actor/
Component、ApplyDamage/RNG、旧物品权威和物品写操作均 0 命中。证据 SHA-256：
`7F160E657D1E7623CFAA17906E091F5A1DCDFE10DF46FE1627613BA173A9BFBC`。

最终二进制：

- `UnrealEditor-demo_map.dll` 19,266,048 bytes，SHA-256
  `3CC8ACF5CF96E967FB932112919EF4A20C259F88BB2B0574A3BBC2D438990BC7`；
- `demo_map.exe` 359,952,384 bytes，SHA-256
  `1EF4F9EEC3685DD2B5861A252D1823A6FE461AF1862403A17C33B4977A062511`。

## 9. 首错、边界与未执行项

本阶段首次 Editor 构建、首次聚焦自动化、完整回归、legacy 依赖回归、映射自检、覆盖门和
最终双目标构建均成功，没有产品首错可保留。原始日志均保存在 `Saved/Codex/P26.2`，没有用
摘要替代原生证据，也不进入 Git。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或
Package。当前目录仍无正式防具抗性数值，因此只声明产品路由与失败关闭成立，不声明玩家
可见效果。

## 10. 改动与交接

本阶段精确提交：

1. `Source/demo_map/demo_mapCombatRunCoordinator.cpp`
2. `Source/demo_map/demo_mapCombatRunCoordinator.h`
3. `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
4. `Source/demo_map/demo_mapShanmenArmorResistanceItemAdapter.cpp`
5. `Source/demo_map/demo_mapShanmenArmorResistanceItemAdapter.h`
6. `Source/demo_map/demo_mapShanmenArmorResistanceItemAdapterTests.cpp`
7. `Scripts/ShanmenRegressionMap.json`
8. `Docs/Report/Dev.D.UE.0.0.10.P26.2.r0_report.md`
9. `Docs/Log/Dev.D.UE.0.0.10.P26.2.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-2-armor-resistance-impact-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-2-armor-resistance-impact-route/Docs/Report/Dev.D.UE.0.0.10.P26.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-2-armor-resistance-impact-route/Docs/Log/Dev.D.UE.0.0.10.P26.2.r0_log.md>
