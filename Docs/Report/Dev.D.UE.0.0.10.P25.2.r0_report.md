# Dev.D.UE.0.0.10.P25.2.r0 Report

## 1. 结论

P25.2 已把 P25.1 可由玩家按 `H` 激活的短时灵力护盾接入真实 M01 敌方受击闭环。活动护盾现在会在统一 Combat Run Coordinator 中参与六类敌方攻击的规范 Impact 结算，以自身剩余容量吸收伤害，并且只在该层真实触发且下游生命/装备资源交付成功后发布容量扣减。

本轮没有在每个敌人类中复制护盾逻辑，也没有开放底层 Session 的可写入口。六类攻击继续汇聚到同一个敌方 Impact 边界；Spirit Shield Product Session 是护盾容量的唯一产品提交入口。

## 2. 玩家可观察行为

- 玩家以 `H` 激活护盾后，30 点容量立即可用于后续敌方物理 Impact；
- 12 点伤害会把容量从 `30 -> 18`，玩家生命不变；
- 3 点真实 M01 基础近战会把容量从 `30 -> 27`，玩家生命保持 `5`；
- 单次伤害超过剩余容量时，护盾只吸收到零，余量继续进入后续防御层与生命权威；
- 相同 Impact 重放不会再次消耗容量，也不会再次扣生命；
- 若更早的闪避/完美防御已完全阻止伤害，护盾不触发、不扣容量；
- 外来 Run 时间线、到期样本、无效防御快照或身份冲突均失败关闭。

## 3. 单一敌方 Impact 汇聚点

GameMode 在每次敌方接触发生时捕获一个短生命周期的护盾上下文，并传入既有 Coordinator。以下产品路径全部复用同一内部执行函数：

1. M01 基础近战；
2. 标准/强化近战突进；
3. 标准远程投射物；
4. 重击扇形；
5. Boss 横扫/冲锋；
6. Boss 三连投射物。

护盾按确定顺序追加在既有装备防御与 Weapon Guard 投影之后、纯结算之前。敌人 Actor 仍只提供已经通过几何/技能规则认可的接触，身份、冻结防御、结算和玩家生命写入仍由 Coordinator 掌握。

## 4. 护盾容量权威

`Fdemo_mapShanmenSpiritShieldProductSession` 新增两个类型化边界：

- `TryComposeImpactDefense`：以 ImpactId、固定 Run 时间样本和既有防御快照生成只读投影证明；
- `CommitImpact`：从规范结算生成容量命令，在候选 Session 上提交，调用下游交付，并仅在下游报告成功后发布候选。

产品 Session 不再提供可写的 `GetSession()`；外部只能读取底层 Session，不能绕过产品会话直接修改容量。投影必须与请求中的 LayerId、RuleId、SourceInstanceId、顺序、幅度、标签与提交标志逐项一致。

## 5. 生命与装备资源协调

Spirit Shield Layer 标记为“触发时需要提交”，但它不是装备物品。Defense Resource Adapter 因此新增显式的外部协调 LayerId 集合：

- 只排除当前 Product Session 已证明的护盾 Layer；
- 既有 Spirit Guard Robe、护心镜等装备层仍必须匹配准备好的物品实例和耐久/次数预留；
- 装备层继续由原有 durable prepare -> vitality commit/recover -> durable finalize 流程负责；
- 没有装备资源层时仍走唯一玩家生命提交；
- 下游不能证明成功时，本次敌方执行失败，护盾候选容量不发布；既有物品事务的 pending/recovery 语义保持原样。

这是一条组合边界，不是新库存、新生命值或第二防御账本。

## 6. 自动化证明

新增 Spirit Shield Product Session 3 项聚焦测试，分别覆盖：

- 容量扣减与相同 Impact 幂等重放；
- 下游交付失败时容量候选不发布，随后重试可正常提交；
- 前置闪避不消耗护盾，以及外来/到期时间样本失败关闭。

Combat Run Coordinator 新增 1 项真实产品集成测试：通过真实玩家/敌人注册、共享 SpiritEnergy、固定时间线、护盾激活、M01 基础近战、纯结算和玩家生命 Authority，证明 3 点伤害完全由护盾吸收、容量 `30 -> 27`、生命 `5 -> 5`，生命 Ledger 只记录一次零伤害规范 Impact。

正式 `Shanmen.0_0_10` 全量结果：1291 Success / 0 Fail，原生退出码 0；日志 SHA-256 为 `64654A1B4E70E517234C67E365A9FCC9AC852396332EF906F93F56A622450FF5`。

## 7. 改动文件驱动回归

现有回归映射已完整覆盖本轮 10 个改动源码/测试文件，无需扩写映射。除全量 `Shanmen.0_0_10` 外，按路径映射精确执行：

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `demo_map.EnemySkillFramework` | 44 | 0 | `BCA2CC08B9FE6990B13A61790F65C80B2BDE7F8C75194576FA01BAB4F26D5B26` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `110AD73DDC4B8C5A46511899E25C14459C3C2E15757BDDDBA2A44D71FB5F53ED` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `8181B12C8E782C6EFAD6B15C9F824D6E8A67B7FC6E45EC73B1B4DEC83273191B` |
| `demo_map.V3.Attributes` | 4 | 0 | `14F82913F212CF7CF4DB45FD8B31BC5FC670EA33D3C6AC7F31850BC1D582A0D0` |

覆盖门禁：`PASS Changed=10 Rules=4 Required=69 Logs=5`，SHA-256 `0E3FE2891E9718557FD41B8297FCBBD73AE183C9BA50E6738B3BEB47982ACB24`。

覆盖器自检：446/446 PASS，SHA-256 `2064B121A357201879B2A267DA01042BF0787CA797EB95F2C0F10AB42BAED09F`。

## 8. 首错、构建与静态检查

首次 Coordinator 聚焦回归为 18 Success / 1 Fail。唯一失败是新增测试把“3 点伤害被 30 点护盾完全吸收”的既有枚举语义误写为 `Mitigated`；纯 Resolver 契约正确返回 `FullyPrevented`。修复只校正新增断言，没有修改生产结算、旧断言或枚举语义。原始失败日志保留，SHA-256 `88703A86C026DA5C07B93F451259173A6DF5A6AC6CDF7CD649FD4290C7D9463A`；正式 Coordinator 复测 19/19 通过。

最终构建：

| Target | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `demo_mapEditor Win64 Development` | Succeeded | 0 | `F3FA40EDE07CE6F4C1A28D0237D0D26BB39C65F9A2009F229CEC49E27755A4FB` |
| `demo_map Win64 Development` | Succeeded | 0 | `D3D4C8A244648968D9981F88F87B1D03C133F250C040FD023BCC9958A7121A93` |

- `git diff --check`：PASS；
- Product Session 对 `UWorld`、`AActor`、`ApplyDamage`、RNG、Timer、SaveGame、物品 Subsystem 和可写底层访问：0 命中；
- 最终项目相关进程：0；
- `UnrealEditor-demo_map.dll`：19109376 bytes，SHA-256 `BC5A99FF2D00951CCFA3BEDA8A65EBB8B60DCA3601736857E96ECF034A815868`；
- `demo_map.exe`：359817728 bytes，SHA-256 `402D9749EFE58E3212259B7FE027465FF3F72B6B547BC40515BECDCFF555BF3C`。

## 9. P/F 边界与下一步

PASS：活动护盾已进入全部六类 M01 敌方 Impact；容量吸收、余量传递、前置防御顺序、触发时提交、失败不发布、相同 Impact 幂等、固定时间线与玩家生命提交均有无头证明；文件驱动回归、双目标构建和静态门禁完成。

未声明：HUD 已显示护盾活动状态或剩余容量，也未验证真实按键到敌方碰撞的画面表现。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

建议 P25.3 直接完成最小 HUD 可读性：把现有 Product Session 的活动状态、剩余容量和固定期限投影到 MainHUD，只读显示，不新增第二状态或 Tick 时间源。

## 10. GitHub 交接

基线提交：`c1addef9a8fb5a12f69ce2e432e6589b9a15e8f4`。

工作分支：`agent/0.0.10-p25-2-spirit-shield-impact`。

本阶段只提交精确的 10 个实现/测试文件、本 Report 与本 Development Log；用户原有 103 个未跟踪文件保持未暂存，`Saved/Codex/P25.2` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-2-spirit-shield-impact>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-2-spirit-shield-impact/Docs/Report/Dev.D.UE.0.0.10.P25.2.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-2-spirit-shield-impact/Docs/Log/Dev.D.UE.0.0.10.P25.2.r0_log.md>
