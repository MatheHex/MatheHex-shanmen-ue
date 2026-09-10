# Dev.D.UE.0.0.10.P25.2.r0 Development Log

## 1. 目标

- 让 P25.1 已激活的短时灵力护盾真实拦截 M01 敌方 Impact；
- 所有敌方攻击通过一个共享 Coordinator 汇聚点，不在敌人类中复制容量逻辑；
- 护盾必须在既有防御顺序中投影，并仅按实际吸收量扣减；
- 容量、装备资源与玩家生命交付必须保留各自唯一权威和失败关闭语义；
- 禁止调用方直接取得可写底层 Shield Session；
- 完成幂等、失败不发布、顺序、期限和真实产品集成测试；
- 按改动文件映射执行回归、构建并交付 Report/Log。

## 2. 基线与范围

- 基线：`c1addef9a8fb5a12f69ce2e432e6589b9a15e8f4`（P25.1）；
- 分支：`agent/0.0.10-p25-2-spirit-shield-impact`；
- 起始 tracked tree clean；
- 用户原有 103 个未跟踪文件保持原样；
- 本轮不修改护盾激活成本、容量、期限、输入键、敌方伤害公式、存档 Schema、地图或 UI 资产。

## 3. Product Session 实现

`demo_mapShanmenSpiritShieldProductSession` 新增：

- `Fdemo_mapShanmenSpiritShieldImpactDefenseResult`：冻结 ImpactId、Run 时间样本、Projection 与组合后 Defense；
- `Fdemo_mapShanmenSpiritShieldImpactCommitResult`：区分 NotTriggered、Committed、AlreadyCommitted 与结构化拒绝；
- `TryComposeImpactDefense()`：验证活动容量、固定时间线、Deadline、基础快照和唯一 Layer 身份；
- `CommitImpact()`：在候选 Session 上提交规范容量命令，随后调用下游交付；只有下游返回成功才发布候选。

可写 `GetSession()` 已移除，只保留 const 读取。外部因此不能直接调用底层 Capacity Authority 改写产品状态。

## 4. 防御顺序与提交不变量

共享敌方执行链的顺序为：

1. 玩家 Health Component 捕获基础防御；
2. Defense Resource Adapter 准备装备资源层；
3. Weapon Guard 追加时机防御；
4. Spirit Shield 追加容量层；
5. CombatCore Resolver 纯结算；
6. Spirit Shield Product Session 根据真实触发 Receipt 暂存容量扣减；
7. 既有装备资源/玩家生命权威完成下游交付；
8. 下游成功后发布护盾候选。

容量命令只接受完全匹配的 Layer/Impact/Resolution。更早层完全阻止伤害时，Resolver 不产生护盾触发 Receipt，Product Session 仍交付规范零伤害 Impact，但容量保持不变。

## 5. Coordinator 与 GameMode 接线

`Fdemo_mapM01EnemyAttackSpiritShieldContext` 只借用当前 Product Session 和一次固定时间线样本。GameMode 对六类敌方入口统一捕获该上下文；空、已关闭或已耗尽容量的护盾不参与，非空但无效的状态会作为启用但无效的上下文送入 Coordinator 并失败关闭。

Coordinator 的六个公开敌方 API 只新增尾部可选参数，既有调用保持源码兼容。共享内部执行函数负责唯一投影、结算和提交路径。

## 6. 装备资源适配

Defense Resource Adapter 的 `BuildIntentRequest()` 与 `CoordinateImpact()` 新增可选的外部协调 LayerId 集合。Coordinator 只传入本次 Product Session Projection 已证明的 Shield LayerId。

适配器会：

- 验证排除项确实是请求中需要触发提交的非装备层；
- 跳过该护盾层，不把它误当成物品 Reservation；
- 继续要求所有其余提交型 Layer 对应精确的已准备装备实例；
- 保持既有 durable intent、生命 recover/commit 和 finalize 协议不变。

## 7. 测试与首错历史

新增 Product Session 测试：

- `ImpactCommitAndReplay`：12 点吸收，`30 -> 18`，相同 Impact 不二次扣减；
- `ImpactDeliveryAtomicity`：下游拒绝时保持 30 和零提交，重试后 `30 -> 20`；
- `ImpactOrderingAndWindow`：前置闪避不扣容量，外来/Deadline 样本拒绝。

新增 Coordinator 集成测试 `M01EnemySpiritShieldProduct`：真实 Run、玩家/敌人注册、共享 SpiritEnergy、护盾激活、3 点 M01 近战和生命 Ledger 全链执行。

首轮 Coordinator 聚焦结果为 18 Success / 1 Fail。唯一错误位于新增断言：完全吸收被预期为 `Mitigated`，而既有 Resolver 契约正确返回 `FullyPrevented`。诊断单测确认执行状态、容量 `30 -> 27`、生命 `5 -> 5` 与 Ledger 均已正确。只修正新增枚举期望后，正式聚焦回归 19/19 通过；生产实现未为测试让步。

首错证据：

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `P25.2_CombatRunCoordinator_focused_attempt-1.log` | 18 | 1 | `88703A86C026DA5C07B93F451259173A6DF5A6AC6CDF7CD649FD4290C7D9463A` |
| `P25.2_M01EnemySpiritShield_diagnostic.log` | 0 | 1 | `114C089F44155E1837DF460FAA576BD96CE02E1A3D1FF513789DAF7380864FE9` |
| `P25.2_CombatRunCoordinator.log` | 19 | 0 | `785D21C176639691F8BFC82C8BC6FC1B4D082F3F8F2B757065FCC84E2CC4F168` |

诊断日志的 1 Fail 是刻意保留原断言以逐项定位的重复证据，不是另一个产品失败。

## 8. 正式自动化与覆盖

| Evidence | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `P25.2_Shanmen_0_0_10.log` | `Shanmen.0_0_10` | 1291 | 0 | `64654A1B4E70E517234C67E365A9FCC9AC852396332EF906F93F56A622450FF5` |
| `P25.2_demo_map_EnemySkillFramework.log` | `demo_map.EnemySkillFramework` | 44 | 0 | `BCA2CC08B9FE6990B13A61790F65C80B2BDE7F8C75194576FA01BAB4F26D5B26` |
| `P25.2_demo_map_ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | `110AD73DDC4B8C5A46511899E25C14459C3C2E15757BDDDBA2A44D71FB5F53ED` |
| `P25.2_demo_map_V2RangedCompatibility.log` | `demo_map.V2RangedCompatibility` | 22 | 0 | `8181B12C8E782C6EFAD6B15C9F824D6E8A67B7FC6E45EC73B1B4DEC83273191B` |
| `P25.2_demo_map_V3_Attributes.log` | `demo_map.V3.Attributes` | 4 | 0 | `14F82913F212CF7CF4DB45FD8B31BC5FC670EA33D3C6AC7F31850BC1D582A0D0` |

正式总计为全量 1291 项加 116 项精确 legacy 回归，全部成功。

覆盖门禁：`PASS Changed=10 Rules=4 Required=69 Logs=5`，SHA-256 `0E3FE2891E9718557FD41B8297FCBBD73AE183C9BA50E6738B3BEB47982ACB24`。

覆盖器自检：446/446 PASS，SHA-256 `2064B121A357201879B2A267DA01042BF0787CA797EB95F2C0F10AB42BAED09F`。

## 9. 构建、静态检查与边界

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `editor-build-attempt-1.log` | Editor Succeeded | 0 | `159AC302B0D6FCCB24BE9F2C1B31A8E6EE5B948EEC7C8EAF103B516A1CF4511D` |
| `P25.2_EditorBuild_final.log` | Editor Succeeded | 0 | `F3FA40EDE07CE6F4C1A28D0237D0D26BB39C65F9A2009F229CEC49E27755A4FB` |
| `P25.2_GameBuild_final.log` | Game Succeeded | 0 | `D3D4C8A244648968D9981F88F87B1D03C133F250C040FD023BCC9958A7121A93` |

- `git diff --check`：PASS；
- Product Session 禁止依赖扫描：0 命中；
- 最终项目相关进程：0；
- `UnrealEditor-demo_map.dll`：19109376 bytes / `BC5A99FF2D00951CCFA3BEDA8A65EBB8B60DCA3601736857E96ECF034A815868`；
- `demo_map.exe`：359817728 bytes / `402D9749EFE58E3212259B7FE027465FF3F72B6B547BC40515BECDCFF555BF3C`。

## 10. P/F 与精确交接

P 阶段已证明：所有 M01 敌方攻击进入统一护盾边界；活动窗口、容量扣减、余量结算、前置防御不触发、失败不发布、重放幂等、装备资源隔离和玩家生命提交成立。

F 阶段未执行：没有 MainHUD 护盾显示，也没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

只暂存本阶段 10 个明确源码/测试文件、本 Report 与本 Development Log。用户原有 103 个未跟踪文件不暂存；`Saved/Codex/P25.2` 原始日志留在本地忽略目录。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-2-spirit-shield-impact>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-2-spirit-shield-impact/Docs/Report/Dev.D.UE.0.0.10.P25.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-2-spirit-shield-impact/Docs/Log/Dev.D.UE.0.0.10.P25.2.r0_log.md>
