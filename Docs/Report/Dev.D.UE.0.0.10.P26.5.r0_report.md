# Dev.D.UE.0.0.10.P26.5.r0 Report

## 1. 结论

P26.5 已把 P26.3 已存在的防具抗性数据接入统一物品详情。玩家查看一阶道袍时，基础属性栏现在显示
`物理抗性 10%`；未配置抗性的 TrainingVest 不会误报抗性。

本阶段只读取 canonical item catalog，未新增防具、抗性通道或平衡数值，也未建立第二套装备或 UI
权威。备战条目与局内物品实例通过同一 `BuildDetail()` 逻辑生成详情。

## 2. 阶段范围

- 扩展既有统一物品详情中的 `BaseAttributes` 摘要；
- 保留原有属性修正数量，并与有效抗性条目用中文分号组合；
- 为现有四种伤害通道提供稳定中文名称；
- 为未知但有效的伤害子标签保留完整 tag 名称，避免静默丢失数据；
- 不改 catalog、伤害结算、物品权威、存档 schema、地图、资产或输入。

## 3. 数据来源与格式

详情严格读取 `Fdemo_mapItemDefinition::DamageResistances`。只有通过既有 `IsValid()` 的条目才进入
摘要；百分比为整数时不显示小数，否则保留一位小数。

当前 canonical 数据保持不变：`ArmorRobeLevel1` 的物理抗性为 `0.10`，因此显示为
`物理抗性 10%`。这不是新增或重定标数值。

## 4. 产品表面

两个既有入口共享同一摘要生成器：

- `BuildDetail(Fdemo_mapProfilePreparationStashRow)`：备战、仓库类详情；
- `BuildDetail(Fdemo_mapItemInstance)`：局内背包与搜索容器详情。

现有背包、搜索容器、宗门备战和仓库调用点无需新增状态或接线。P 阶段证明统一详情数据正确；
本轮未启动 UI，因此不声明最终字体、截断、换行或像素布局已验收。

## 5. 正反测试

新增 `Shanmen.0_0_10.Product.ArmorResistanceItemDetail.CanonicalAndControl`：

- 一阶道袍备战详情精确等于 `物理抗性 10%`；
- TrainingVest 有既有属性修正但无抗性，不得出现 `抗性`；
- 从真实 `Fdemo_mapItemAuthority` 创建的一阶道袍实例，与备战条目生成完全相同的抗性摘要。

专项最终结果 1 Success / 0 Fail。

## 6. 首错与修复

首次专项测试错误期待 `2 项基础属性修正；物理抗性 10%`，实际 catalog 中这两个旧数值属于
`EffectParameters`，`Modifiers` 为空。生产实现正确，修复仅把测试期望改为 `物理抗性 10%`。

首次失败日志保留：1 Fail，SHA-256
`CF8B6654E769E653D460A18E4DAC69878A48CD95493EA1836D2134637CBEFA87`。最终专项日志 1 Success /
0 Fail，SHA-256 `B01FDFA563B321321B3D2E2DF76BD55880CC0022938A452DB648BE30847BC9E7`。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10` | 1,321 | 0 | `F3716317...5FE630` |
| `Shanmen.0_0_10.Items` | 77 | 0 | `D96C8E35...B6B84A` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `3884CA8E...0F18D` |
| `demo_map.P4.Hotbar` | 7 | 0 | `CD0B4D55...3B65F` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `65FA5EAD...CDBDB` |
| `demo_map.Profile` | 211 | 0 | `7A32FDC4...B4DED` |
| `demo_map.CodeB` | 60 | 0 | `4EA2F9C9...241DF` |
| `demo_map.V3.WorldInteraction` | 4 | 0 | `D41D40F7...0EC42` |

所有最终测试日志均有原生队列完成标记、0 Fail、0 Fatal 与进程退出码 0。

## 8. 改动驱动回归、构建与静态检查

两个生产/测试路径命中两条映射规则，推导 7 个 required groups；覆盖门结果：
`PASS Changed=2 Rules=2 Required=7 Logs=8`，SHA-256
`07C325C74CF1B50084EF6C3223565AFDF080340B3B3C013B927A17F2565D4E00`。

映射器正反自测 `457/457` PASS，SHA-256
`8C81F7F329B2AD5EFF7334A56D12CFDA10828E94009F32C75B6E1E521AD9EBEF`。

| Target | Result / native exit | Log SHA-256 |
|---|---|---|
| `demo_map` Win64 Development | Succeeded / 0 | `E1F42102...528C6` |
| `demo_mapEditor` Win64 Development | Succeeded / 0 | `597C9776...16285` |

`git diff --check` 与 regression map JSON 解析通过。新增运行时代码的 scoped scan 未发现 Tick、Timer、
RNG、ApplyDamage、SpawnActor、NewObject、存档或物品写入。

## 9. P/F 边界与下一阶段

P 阶段已证明 canonical 抗性、中文格式、无抗性控制样本、备战/局内详情一致性、完整 0.0.10 与
改动文件驱动回归均成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、
Cook 或 Package。下一阶段可在明确 F 边界内检查四个产品表面的实际可读性，或继续下一个正式定义
的产品纵切；不得从本阶段推断新的防具抗性数值。

## 10. GitHub 交接

基线提交：`25b05f4d44cbf3392918243a4789b640d04ca7fa`（P26.4）。
分支：`agent/0.0.10-p26-5-armor-resistance-item-detail`。本阶段只提交两个实现/测试文件、本 Report
与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P26.5` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-5-armor-resistance-item-detail>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-5-armor-resistance-item-detail/Docs/Report/Dev.D.UE.0.0.10.P26.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-5-armor-resistance-item-detail/Docs/Log/Dev.D.UE.0.0.10.P26.5.r0_log.md>
