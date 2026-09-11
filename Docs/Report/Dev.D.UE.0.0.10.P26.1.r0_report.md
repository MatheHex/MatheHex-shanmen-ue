# Dev.D.UE.0.0.10.P26.1.r0 Report

## 1. 结论

P26.1 已把 P26.0 的纯防具抗性投影接到 0.0.10 唯一持久化物品权威：
`Udemo_mapShanmenItemAuthoritySubsystem`。新适配器只接受从权威收据重建的 active-Run
correlation，并只选择其中冻结的精确 `ArmorItemInstanceId`；调用方不能传入另一个防具
实例、从旧可变装备槽推断身份，或跳过已提交的 `DeploymentLock` 证据。

当前 canonical 防具仍没有正式抗性元数据，因此精确已装备防具会得到带完整权威证据的
`NotApplicable`，空防具槽得到独立的 `NoArmorEquipped`。两者都原样保留基础防御快照，
没有改变任何现有数值或玩家行为。

## 2. 唯一权威链

产品入口 `ProjectActiveRun()` 只读取以下现有链路：

1. ready 的 GameInstance 物品权威；
2. `TryGetActiveRunCorrelation()` 从持久收据重建的 immutable correlation；
3. 同一权威捕获的只读 `FShanmenItemAuthoritySnapshot`；
4. correlation 冻结的唯一 `ArmorItemInstanceId`；
5. 对应 item、authority definition 与 committed `DeploymentLock`；
6. 同 DefinitionId 的 Code A canonical 物品定义；
7. P26.0 `TryProject()` 的纯 CombatCore 防御层投影。

没有读取旧 `Fdemo_mapItemAuthority::GetEquippedInstance()`，也没有新增装备、库存、Run 或
防御真值。适配器不持久化副本，不修改物品状态，不扣耐久/次数，也不改写调用方快照。

## 3. 跨权威一致性

适配器在投影前同时验证：

- snapshot 必须使用当前 `Shanmen.Items.0.0.10` product content stamp，且 authority revision
  不早于 active-Run lifecycle revision；
- exact item 必须是 correlation 的 ArmorSlot 身份，并处于同 owner、同 Run scope 的
  `Deployed` singleton 状态；
- authority definition 必须支持 `DeploymentLock`；
- authority 与 Code A definition 的 DefinitionId、MaxStack、MaxDurability、MaxCharges 必须
  一致；
- Code A definition 必须是唯一 ArmorSlot 兼容的 ArmorCategory、MaxStackSize 1；
- item 必须引用同 owner、同 scope、同 item 的 committed DeploymentLock，且部署确实推进
  过 item revision；
- 输出 evidence 同时冻结 authority content stamp 与当前 canonical catalog identity。

任何缺失、漂移、过期或跨槽位证据均失败关闭，不会退回名字、类别猜测或旧装备槽。

## 4. 成功状态语义

三个成功状态互不混淆：

| Status | 含义 | Evidence |
|---|---|---|
| `Projected` | exact 防具有合法抗性元数据，P26.0 已追加确定性层 | 必须有效 |
| `NotApplicable` | exact 防具已验证，但当前定义未授权抗性 | 必须有效 |
| `NoArmorEquipped` | active Run 没有 ArmorSlot 身份 | 必须为空 |

`IsSuccess()` 对三种状态执行不同不变量。尤其是已装备但未调参的防具不能丢失证据后仍被
误判为正常空槽；空槽也不能掩盖无效目标或无效基础 Defense。

## 5. 自动化证明

精确组 `Shanmen.0_0_10.Product.ArmorResistanceItemAdapter` 共 5 项：

| Test | Result |
|---|---|
| `AuthoritySelection` | Success |
| `NoArmorReplay` | Success |
| `IdentityFences` | Success |
| `EvidenceFences` | Success |
| `ProductFacadeBoundary` | Success |

关键证明包括：精确 deployed `TrainingVest` 被选择且当前为成功无操作；snapshot、correlation
和基础 Defense 均不被修改；空槽可确定性重放；无效 Defense 即使在空槽下仍被拒绝；缺失
item、Stored 状态、未知定义、WeaponSlot 定义、旧 revision、错误 content stamp、pending
deployment、未推进 item revision 及 authority/catalog 资源形状漂移全部失败关闭；未绑定
GameInstance 权威不能绕过产品入口。

聚焦日志最终 5 Success / 0 Fail / 0 Fatal；SHA-256：
`53A3D82ECC74D71CAE4E51B0289D700FE2C8F8B08A5B31571B0608C702971E8A`。

## 6. 改动文件驱动回归

新增 `ArmorResistanceItemAdapter` 映射规则，要求 5 个精确组：

| Exact group | Success | Fail |
|---|---:|---:|
| `Shanmen.0_0_10.Product.ArmorResistanceItemAdapter` | 5 | 0 |
| `Shanmen.0_0_10.Product.ArmorResistanceProjection` | 4 | 0 |
| `Shanmen.0_0_10.Items` | 77 | 0 |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 |
| `demo_map.ItemUseAndArmor` | 46 | 0 |
| **合计** | **141** | **0** |

每组均有唯一 RunTests 命令、原生 Queue Empty、退出码 0，且无 Fatal/Unhandled/Ensure。
覆盖门最终结果：`PASS Changed=7 Rules=1 Required=5 Logs=5`；证据 SHA-256：
`9CFA259DBE9F18DDD6BBBB06035904F67DA3F4F4EBFFB055C2018342849DB7EB`。映射器新增
正反例：完整证据必须通过，只有聚焦组必须失败；自检 455/455 PASS，SHA-256：
`2B7C907F801E411CF76A642C37D32FF02E9D08058117F9826D13D69F94888B2B`。

## 7. 构建与静态检查

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| 首次 `demo_mapEditor Win64 Development` | Succeeded | 0 | `63054EC42EC940DE53CD541CA5E3B49B1B8DBCB838302AAC7CEC18514F05DE9A` |
| 最终 `demo_mapEditor Win64 Development` | Succeeded | 0 | `5ED2F46059B46D5F3151DF04923878DE039801FE024FD57DA314804544601043` |
| 最终 `demo_map Win64 Development` | Succeeded | 0 | `34DB186CDC8ECFB0DB7698EB18CB9F50F338CF6E785A67E07357686897636B10` |

- `git diff --check`：PASS；Regression Map：schema 1 / 251 rules；
- 实现对 World/Actor/Component、旧可变装备权威、持久化写 API、ApplyDamage 与 RNG：
  全部 0 命中；
- 最终静态证据 SHA-256：
  `CE7807AE504251A8DB116C1606596AACE5141E2E77DD5A40F572CB583B7954B5`；
- `UnrealEditor-demo_map.dll`：19264000 bytes，SHA-256
  `3AAAE0BABCD9AC0F54C3DE9E28F8A8B3785D977ABDBDD62F008369C398411F54`；
- `demo_map.exe`：359952384 bytes，SHA-256
  `8CD0CD6A30C7A6ECF9A10E0CA05311EC7FEEDF5E802841FA47AE2FA45BCDEE5E`。

## 8. 首错与修复

本阶段源码编译、5 个聚焦测试、全部依赖回归、映射自检、覆盖门和双目标构建均未出现
产品失败。初版成功后进行契约复审，把原先共用的 `NotApplicable` 拆为
`NotApplicable` 与 `NoArmorEquipped`，并要求前者必须携带有效 exact armor evidence；随后
重编译并完整复跑全部 141 项。初版测试日志和构建日志仍保留在 `Saved/Codex/P26.1`，未用
最终成功覆盖过程证据。

## 9. P/F 边界与下一阶段

P 阶段已证明：唯一 active-Run ArmorSlot 选择、content/revision/definition/deployment 围栏、
只读性、空槽与未调参防具的独立语义、P26.0 组合、旧物品/旧防具回归以及 Editor/Game
构建成立。

本轮没有把适配器插入敌方 incoming Impact 主路径，也没有给任何防具写抗性数值。后续
P26.2 可在 player Defense 捕获后、资源型防御准备前调用该适配器，并证明当前目录仍为
零差异；正式抗性数值仍需独立内容授权。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package，不声明已有玩家可见抗性效果。

## 10. GitHub 交接

基线提交：`df3a2feb44f2f3df443f4944b4f47b1d70007560`（P26.0）。
分支：`agent/0.0.10-p26-1-armor-resistance-item-adapter`。只提交本阶段 3 个实现/测试文件、
2 个回归流程文件、本 Report 与本 Development Log；103 个既有未跟踪用户条目保持未暂存，
`Saved/Codex/P26.1` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-1-armor-resistance-item-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-1-armor-resistance-item-adapter/Docs/Report/Dev.D.UE.0.0.10.P26.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-1-armor-resistance-item-adapter/Docs/Log/Dev.D.UE.0.0.10.P26.1.r0_log.md>
