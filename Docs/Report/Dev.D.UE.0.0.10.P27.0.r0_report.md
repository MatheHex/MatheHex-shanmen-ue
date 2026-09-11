# Dev.D.UE.0.0.10.P27.0.r0 Report

## 1. 结论

P27.0 已建立阵法投放的产品权威准备边界：把物品系统的持久 `ActiveRun`、战斗 Run 的玩家实体、
当前物品权威内容版本、已编写阵图和一次外部采样的位姿，冻结为可直接交给既有
`Fdemo_mapShanmenFormationProductHost` 的不可变投放命令。

本阶段不修改库存、不扫描 World、不生成 Actor，也未新增阵法平衡数值。它解决的是“谁有权为一
次阵法投放签发身份”的前置问题；GameMode、输入、UI 与世界放置仍未接线。

## 2. 阶段问题与范围

既有 FormationProductHost、Session、材料适配与影响链已有测试，但缺少一个受产品权威约束的生产
入口。P27.0 新增：

- `Fdemo_mapPlayerFormationActionReservation`：保存战斗 Run 签发的单调序号与冻结 Action；
- `Fdemo_mapShanmenFormationDeploymentCommand`：冻结相关性、Action、阵图、原点和单位前向；
- `Fdemo_mapShanmenFormationProductAuthority::PrepareDeployment()`：只读组合物品权威与战斗 Run；
- CombatRunCoordinator 的阵法专用序号和预留接口；
- 一条贯通真实物品权威、CombatRun 与既有 ProductHost 的自动化测试；
- FormationProductAuthority 的改动驱动回归映射和正反自测。

## 3. 权威契约

准备流程按固定顺序关闭失败：

1. 阵图必须有效，原点必须有限，前向必须有限且非零；
2. `Udemo_mapShanmenItemAuthoritySubsystem` 必须处于 Ready；
3. 通过既有 RunLifecycleAdapter 取得真实 `ActiveRun` 相关性；
4. 当前 authority snapshot、绑定 Owner 与 lifecycle revision 必须一致；
5. CombatRunCoordinator 必须正好运行同一个 `ActiveRunId`；
6. 前述条件全部通过后，才消耗一个阵法激活序号并冻结命令。

该边界不接受调用者自造的 Owner、Run 或内容版本。

## 4. 身份与生命周期

阵法 Action 同时保留两类身份：

- `OwnerId` 来自持久物品/档案权威；
- `SourceEntityId` 来自当前战斗 Run 的玩家实体。

Action 使用 canonical formation action definition、`Source.Player` 标签和既有确定性 ID 工厂。相同
Run 内每次成功投放取得不同且可重放的激活 ID；Combat Run 正常结束或 Reset 时，阵法专用序号
恢复为 1。无效阵图、零方向或未启动的 CombatRun 均不会消耗序号。

## 5. 既有 ProductHost 接入证明

专项测试把准备命令中的相关性、Action、阵图与位姿直接交给既有
`Fdemo_mapShanmenFormationProductHost::TryStart()`，Host 成功进入有效状态，Session 保留同一个
Run 相关性与命令 ID。

这证明新边界与现有 Host 契约兼容，但不代表 GameMode、玩家输入、UI、Actor 放置或最终视觉已经
接入产品运行路径。

## 6. 正反测试与只读边界

新增 `Shanmen.0_0_10.Product.FormationProductAuthority.AuthorityRunAndHostStart`，覆盖：

- 无效阵图、零前向、未启动 CombatRun 失败关闭；
- 所有预检失败均不消耗活动序号；
- 持久 Owner、战斗 SourceEntity 与当前内容 stamp 被准确冻结；
- 外部方向在命令中归一化；
- 准备命令成功启动既有 FormationProductHost；
- 第二个命令取得下一确定性 ID；
- 准备前后物品 authority snapshot 完全相同；
- Combat Run 结束后序号复位。

专项最终结果 1 Success / 0 Fail。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationProductAuthority` | 1 | 0 | `148D44B5...ABB0A4B` |
| `Shanmen.0_0_10` | 1,322 | 0 | `977F2A24...540F29` |
| `demo_map.V3.Attributes` | 4 | 0 | `612AAA80...7CFE0` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `640BBAAB...3DF8C` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `DFC38023...D9D49` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `A67C38F1...F1399` |

所有最终日志均为原生退出码 0、0 Fatal，并含终端队列完成标记。

首次 `ItemUseAndArmor` 运行虽返回 0，但日志只完成 45 项并在第 46 项中途截断，没有终止标记；
覆盖门因此拒绝该证据。该首错日志已保留，SHA-256
`9DA3969E03D2134233B3EE8E323D5B2C231080DEB291B023AFA12AF2A2D146B9`。独立重跑完成 46/46 后才
计入最终通过。

## 8. 改动驱动回归、构建与静态检查

7 个本阶段路径命中 3 条映射规则，推导 50 个 required groups；最终覆盖门：
`PASS Changed=7 Rules=3 Required=50 Logs=6`，SHA-256
`948ECB6E82E9CE0D3E7E7E6529E75BDC1AFF79C61AFDE7DC3F0CA177FCD5FFB4`。

映射器正反自测 `459/459` PASS，SHA-256
`37554C33817CB707356992239A8C2C8A0D7F24F8BF0050EBEF9C29FB5F7A7CF2`。

| Target | Result / native exit | Duration | Log SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 557.92s | `2AB370AF...F26A4B` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 0.98s | `E012FA6B...5D762B` |

最终 `demo_map.exe` 为 360,004,096 bytes，SHA-256
`667E13B45B91ED1EA4B4869B9D6D8D6110AFD5D51A933BCA1BBB7402B35CCFDC`；
`UnrealEditor-demo_map.dll` 为 19,324,416 bytes，SHA-256
`71DD073C8F06A629A78A83A5BF781D5C8B84107A4A62AF65BCE0F0F098593B91`。

`git diff --check` 与 regression map JSON 解析通过。新增运行时代码的 scoped scan 未发现 `UWorld`、
`AActor`、`SpawnActor`、`NewObject`、`GetWorld`、World iterator、RNG 或 `ApplyDamage`。

## 9. P/F 边界与下一阶段

P 阶段已证明权威来源、确定性身份、预检原子性、物品只读、Host 兼容、完整 0.0.10 回归与改动文件
驱动覆盖均成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、
Cook 或 Package。下一阶段可在明确产品边界后，把该命令接到唯一 GameMode/输入入口与既有世界投放
链；不得由本阶段推断阵法材料、范围、持续时间或数值。

## 10. GitHub 交接

基线提交：`d18ab44664906c6b6e176ac5a910a84ba38480c2`（P26.5）。
分支：`agent/0.0.10-p27-0-formation-product-authority`。本阶段只提交 7 个实现/测试/回归映射文件、
本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.0`
原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-0-formation-product-authority>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-0-formation-product-authority/Docs/Report/Dev.D.UE.0.0.10.P27.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-0-formation-product-authority/Docs/Log/Dev.D.UE.0.0.10.P27.0.r0_log.md>
