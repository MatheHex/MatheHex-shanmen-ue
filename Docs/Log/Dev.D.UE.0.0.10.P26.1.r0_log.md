# Dev.D.UE.0.0.10.P26.1.r0 Development Log

## 1. 目标

- 把 P26.0 纯抗性投影接到唯一持久化 active-Run ArmorSlot 权威；
- 禁止调用方自行选择防具、使用旧可变装备槽或绕过部署证据；
- 在正式抗性数值未授权前保持所有 canonical 防具行为不变；
- 更新改动文件回归映射并运行精确矩阵与双目标构建；
- 生成 Report/Log，独立提交并推送本阶段分支。

## 2. 基线与范围

- 基线：`df3a2feb44f2f3df443f4944b4f47b1d70007560`（P26.0）；
- 分支：`agent/0.0.10-p26-1-armor-resistance-item-adapter`；
- 起始 tracked tree clean；103 个既有未跟踪用户条目保持未暂存；
- 新增一个只读 product adapter、5 项自动化、1 条回归映射及其正反自检；
- 不修改 canonical 物品数值、存档 schema、ShanmenItems repository、CombatCore resolver、
  GameMode、PlayerController、UI、World、地图或资产。

## 3. 设计选择

审查发现旧 `Fdemo_mapItemAuthority` 仍提供 `GetEquippedInstance()`，但 0.0.10 的唯一持久
产品真值已经是 GameInstance 持有的 `Udemo_mapShanmenItemAuthoritySubsystem`。因此本轮
没有复用旧槽位查询，而是沿用受控飞剑适配器已经验证的 durable evidence 模式：

1. ready authority；
2. 从准备与 lifecycle 收据重建 `Fdemo_mapShanmenRunCorrelation`；
3. 捕获同一 authority snapshot；
4. 只接受 correlation 中的 `ArmorItemInstanceId`；
5. 核对 deployed item、definition 与 committed DeploymentLock；
6. 将 exact identity 交给 P26.0 纯投影。

资源型防御仍由 `Fdemo_mapShanmenDefenseResourceAdapter` 独立负责。本轮没有把被动抗性混入
Reserve/Commit 流程，也没有建立第三套物品或防御权威。

## 4. 新增契约

新增：

1. `demo_mapShanmenArmorResistanceItemAdapter.h`
2. `demo_mapShanmenArmorResistanceItemAdapter.cpp`
3. `demo_mapShanmenArmorResistanceItemAdapterTests.cpp`

`ProjectActiveRun()` 是产品入口，只读获取 correlation 与 snapshot；
`ProjectFromEvidence()` 是无副作用证据门，供自动化和已捕获值调用。输出 evidence 冻结：

- correlation、active Run、owner；
- armor item、definition、DeploymentLock；
- authority revision、item revision、reserve 时 revision；
- authority product content stamp；
- Code A canonical catalog version 与 digest。

authority definition 与 Code A definition 必须在 DefinitionId、stack、durability、charges 上
完全一致，Code A definition 还必须是唯一 ArmorSlot 的 singleton armor。最终抗性数学只由
P26.0 执行。

## 5. 状态不变量

- `Projected`：必须有合法 exact evidence 和至少一个 P26.0 layer；
- `NotApplicable`：表示 exact 防具有效但没有抗性授权，必须保留 evidence；
- `NoArmorEquipped`：表示 correlation 没有 ArmorSlot 身份，evidence 必须为空；
- 三种成功都必须返回合法 Defense，失败不返回部分投影或伪证据；
- 目标/Defense 验证早于空槽分支，空槽不能掩盖坏输入。

初版把后两种状态合并。实现复审后主动拆分并强化 `IsSuccess()`，随后完整重跑全部证据。

## 6. 聚焦自动化

`Shanmen.0_0_10.Product.ArmorResistanceItemAdapter`：

| Test | 主要断言 | Result |
|---|---|---|
| `AuthoritySelection` | exact deployed TrainingVest、有效 evidence、无调参 no-op、只读 | Success |
| `NoArmorReplay` | 独立空槽状态、确定性重放、坏 Defense 不被掩盖 | Success |
| `IdentityFences` | 缺 item、Stored、未知定义、WeaponSlot 冒充全部拒绝 | Success |
| `EvidenceFences` | 旧 revision、错 content、pending lock、revision 未推进、定义漂移拒绝 | Success |
| `ProductFacadeBoundary` | 未绑定 GameInstance authority 不可绕过 | Success |

最终：5 Success / 0 Fail / 0 Fatal / Queue Empty；SHA-256：
`53A3D82ECC74D71CAE4E51B0289D700FE2C8F8B08A5B31571B0608C702971E8A`。

## 7. 改动映射回归

新增映射 `ArmorResistanceItemAdapter`，要求：

| Exact group | Success | Log SHA-256 |
|---|---:|---|
| `Shanmen.0_0_10.Product.ArmorResistanceItemAdapter` | 5 | `53A3D82E...02971E8A` |
| `Shanmen.0_0_10.Product.ArmorResistanceProjection` | 4 | `6E6F86A6...FD362FE` |
| `Shanmen.0_0_10.Items` | 77 | `D76DC7BA...6D1FEC52` |
| `Shanmen.0_0_10.CombatCore` | 9 | `0057F122...8E77E3E7` |
| `demo_map.ItemUseAndArmor` | 46 | `78E7C649...515748BF` |
| **合计** | **141** | **全部原始日志保留** |

每个日志有唯一 exact RunTests、原生完成标记、退出码 0、0 Fail 和 0 Fatal。最终覆盖门：
`PASS Changed=7 Rules=1 Required=5 Logs=5`；SHA-256：
`9CFA259DBE9F18DDD6BBBB06035904F67DA3F4F4EBFFB055C2018342849DB7EB`。

映射自检新增完整证据正例和 focus-only 失败反例。最终 455/455 PASS；SHA-256：
`2B7C907F801E411CF76A642C37D32FF02E9D08058117F9826D13D69F94888B2B`。

## 8. 构建与静态结果

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `P26.1_EditorBuild_attempt-1.log` | 初版 Editor Succeeded | 0 | `63054EC4...F05DE9A` |
| `P26.1_EditorBuild_final.log` | 状态强化后 Editor Succeeded | 0 | `5ED2F460...4601043` |
| `P26.1_GameBuild_final.log` | 最终 Game Succeeded | 0 | `34DB186C...7636B10` |

静态边界：

- `git diff --check` PASS；map schema 1 / 251 rules；
- World/Actor/Component：0；
- 旧 `Fdemo_mapItemAuthority` / `GetEquippedInstance()`：0；
- durable mutation API：0；
- ApplyDamage / RNG：0；
- 最终证据 SHA-256：
  `CE7807AE504251A8DB116C1606596AACE5141E2E77DD5A40F572CB583B7954B5`。

最终二进制：

- `UnrealEditor-demo_map.dll`：19264000 bytes / SHA-256
  `3AAAE0BABCD9AC0F54C3DE9E28F8A8B3785D977ABDBDD62F008369C398411F54`；
- `demo_map.exe`：359952384 bytes / SHA-256
  `8CD0CD6A30C7A6ECF9A10E0CA05311EC7FEEDF5E802841FA47AE2FA45BCDEE5E`。

## 9. P/F 边界

P 阶段完成：权威选择、跨目录一致性、部署证据、状态不变量、只读性、确定性 no-op、
P26.0 组合、141 项回归、覆盖门、静态边界和双目标构建。

P26.1 尚未接入 incoming Impact 主路径，正式抗性元数据仍为空；现有玩家行为为零变化。
F 阶段未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook
或 Package。

## 10. GitHub 交接

精确提交 7 个文件：

1. `Source/demo_map/demo_mapShanmenArmorResistanceItemAdapter.h`
2. `Source/demo_map/demo_mapShanmenArmorResistanceItemAdapter.cpp`
3. `Source/demo_map/demo_mapShanmenArmorResistanceItemAdapterTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P26.1.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P26.1.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-1-armor-resistance-item-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-1-armor-resistance-item-adapter/Docs/Report/Dev.D.UE.0.0.10.P26.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-1-armor-resistance-item-adapter/Docs/Log/Dev.D.UE.0.0.10.P26.1.r0_log.md>
