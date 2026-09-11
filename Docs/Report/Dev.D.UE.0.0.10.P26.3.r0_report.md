# Dev.D.UE.0.0.10.P26.3.r0 Report

## 1. 结论

P26.3 已为 canonical `ArmorRobeLevel1`（一阶道袍）投放首个正式防具抗性：
`Shanmen.Damage.Physical` 物理伤害减免 10%。P26.0 的纯标签投影、P26.1 的 active-Run
物品证据适配器与 P26.2 的真实 incoming Impact 路由因此首次形成玩家战斗中的完整产品链路。

投放保持最小：`TrainingVest` 继续作为明确未配置抗性的 no-op 对照，其他防具、伤害类型、
存档 schema、战斗公式与资源型防御均未调整。

## 2. Canonical 内容契约

一阶道袍新增且仅新增：

- `DamageResistance` 产品语义；
- 一个 `ReduceFraction` 投影来源；
- required damage tag：`Shanmen.Damage.Physical`；
- resistance fraction：`0.10`；
- source instance：active-Run `ArmorSlot` 中的精确物品实例。

目录校验要求全目录恰有一个正式 `DamageResistance` 定义，防止未审查的第二件抗性防具随
其他改动进入产品。训练防具同时被测试为无语义、无抗性数组的显式对照。

## 3. 标签匹配与守恒

纯投影/结算测试证明：

| Incoming damage | Raw | Prevented | Final | Result |
|---|---:|---:|---:|---|
| `Shanmen.Damage.Physical.Slash` | 100 | 10 | 90 | 父标签物理抗性命中 |
| `Shanmen.Damage.Mental` | 100 | 0 | 100 | 非匹配标签绕过 |

两条路径均满足 `RawDamage == PreventedDamage + FinalDamage`。物理子标签通过 GameplayTags
层级匹配，不需要为 Slash、Pierce 等子类型复制数值；精神伤害不会误用物理抗性。

## 4. 真实 M01 产品路径

新增 `CombatRunCoordinator.CanonicalArmorResistance` 从真实 Profile 准备开始：只部署一阶
道袍，不部署护心镜；建立 active-Run correlation 后，由 M01 敌方普通近战生成带
`Damage.Physical` 标签的 1.0 incoming Impact。

结果精确证明：

- 适配器读取同一 Run、同一 ArmorSlot、同一 item instance 与当前 catalog stamp；
- Defense 中只有该道袍生成的 10% 层，且只触发一次；
- 1.0 raw damage 结算为 0.10 prevented / 0.90 final；
- 玩家生命从 3.00 降至 2.10，authority revision 与 committed Impact 都仅增加一次；
- 防具不进入 Reserve/Commit，攻击前后完整物品权威 snapshot 相等；
- 同一 delivery 重放返回 `AlreadyCommitted`，不再次扣血或改变防具。

## 5. 内容身份与兼容

当前目录身份升级为 `CodeB.Content.0.0.10.P26.3`，摘要为：
`FB773D9692445401D0C97772498A85F591B735D36E7F2309789ED1EC47EADE74`。

摘要由无尾换行 UTF-8 canonical 串重新计算并精确匹配。P21.0 父身份继续由
`IsKnownContentIdentity()` 接受；持久物品仍保存同一个 `DefinitionId`，因此本阶段是内容
版本升级而不是 save schema 迁移，不重写既有一阶道袍实例。

## 6. 首错与修复闭环

首次完整回归发现两个旧适配器测试仍把 P21.0 写死为“当前目录”。该失败准确反映测试契约
落后于内容版本，而不是产品伤害或物品路由错误。首次失败日志保留，SHA-256：
`8C0CA31ADE2887FF1B80FEDF1E009200B75B9A14F357DA4CE9D63142815D92AD`。

同类静态搜索共定位四处断言，全部改为“P26.3 是当前身份、P21.0 是已知历史身份”。四个
受影响组随后分别复测通过：Meridian Shock 32、Sword Qi 4、Weapon Guard 4、Thrown
Weapon canonical content 1，合计 41 Success / 0 Fail。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `ArmorResistanceProjection`（首轮聚焦） | 4 | 0 | `32A6FF23...118B82` |
| `CombatRunCoordinator`（首轮聚焦） | 21 | 0 | `098CFB1B...FAB746` |
| `demo_map.V3.Items.DefinitionRegistry` | 1 | 0 | `1B82B8DD...28D36C` |
| `Shanmen.0_0_10`（最终完整套件） | 1,318 | 0 | `0F1AE549...165656D` |

完整套件聚焦项不重复计入改动映射总数。所有最终日志均要求原生
`Automation Test Queue Empty`、0 Fail、0 Fatal 与进程退出码 0。

## 8. 改动文件驱动回归、构建与静态检查

9 个生产/测试改动路径命中 7 条回归规则，推导 28 个 required groups。完整
`Shanmen.0_0_10` 覆盖所有新模块组，另以独立日志执行 9 个 legacy 组；最终映射合计
1,740 Success / 0 Fail。覆盖门：
`PASS Changed=11 Rules=7 Required=28 Logs=10`，SHA-256
`F991A5B69A5661D3B405482677A6832820EE5C795609A6701BF759BBA19044B7`。

映射器正反自检 455/455 PASS，SHA-256
`2B7C907F801E411CF76A642C37D32FF02E9D08058117F9826D13D69F94888B2B`。

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| 首次 `demo_mapEditor Win64 Development` | Succeeded，369 actions | 0 | `A51F348F...CCA4B6` |
| 修复后 Editor 增量构建 | Succeeded，7 actions | 0 | `404ACFEC...6059722` |
| 最终 `demo_map Win64 Development` | Succeeded，368 actions | 0 | `A9B37F98...C98A4EF` |
| 最终 `demo_mapEditor Win64 Development` | Succeeded，up to date | 0 | `35FE8BB3...18C39D8` |

`git diff --check`、catalog invariant、canonical digest 与 scoped boundary scans 均须 PASS；
静态证据 SHA-256：
`CB23C2DA1832AAF4F1DF08ACD846F01FB6ECFB822650E70895EF8CF1991C2FBA`。最终二进制大小与摘要
记录在 Development Log。

## 9. P/F 边界与下一阶段

P 阶段已证明：正式目录数值、标签匹配/绕过、receipt 守恒、active-Run 精确防具证据、真实
M01 生命提交、被动只读、重放幂等、旧内容身份兼容、完整 0.0.10 与改动映射回归成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package。下一阶段可独立评审第二件防具/第二种伤害通道，或把已结算抗性
收据接入玩家可见反馈；不得在未指定平衡目标时批量复制本轮 10% 数值。

## 10. GitHub 交接

基线提交：`7e42e85c486e3edcf8cbb0f5e35086d7c518ff50`（P26.2）。
分支：`agent/0.0.10-p26-3-tier1-robe-physical-resistance`。只提交本阶段 9 个实现/测试文件、
本 Report 与本 Development Log；103 个既有未跟踪用户条目保持未暂存，
`Saved/Codex/P26.3` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-3-tier1-robe-physical-resistance>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-3-tier1-robe-physical-resistance/Docs/Report/Dev.D.UE.0.0.10.P26.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-3-tier1-robe-physical-resistance/Docs/Log/Dev.D.UE.0.0.10.P26.3.r0_log.md>
