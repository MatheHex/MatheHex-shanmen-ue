# Dev.D.UE.0.0.10.P19.7.r0 Report

## 1. 结论

P19.7 已完成 canonical Divine Sense Product Authority。

新权威是神识产品配置、Combat Action 与 Intent 身份的唯一 P 阶段来源。调用方只能请求一次神识脉冲，不能选择 definition、SpiritEnergy cost、容量、Action 内容、activation identity、scan ordinal、subject budget 或 IntentId。Combat Run 按本 Run 单调序列分配 activation identity；Run 结束或 reset 后序列归一，新 Run 仍由 RunId 隔离身份。

P19.1-P19.6 的资源余额、World evidence、扫描、事务、receipt、replay 与 Controller 生命周期继续保持原权威。本轮没有建立第二套资源、World 或回放系统。

本轮为 P 阶段。没有接物理输入、UI、表现、隐式 Actor 搜索或最终平衡，也没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 基线与分支

- 基线提交：`f5790511f28c81d72645bb5699b86910d1ec10c8`（P19.6）；
- 分支：`agent/0.0.10-p19-7-divine-sense-product-authority`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 新增与修改

新增：

- `demo_mapShanmenDivineSenseProductAuthority.h`：79 行；
- `demo_mapShanmenDivineSenseProductAuthority.cpp`：278 行；
- `demo_mapShanmenDivineSenseProductAuthorityTests.cpp`：515 行。

修改：

- `demo_mapCombatRunCoordinator.h/.cpp`：增加神识激活序列与 canonical reservation；
- `ShanmenRegressionMap.json`：增加 Product Authority 的 12 组直接证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：增加完整正例与缺证失败关闭反例。

## 4. Canonical 产品配置

唯一权威冻结以下版本化 P 阶段值：

| 字段 | Canonical 值 |
|---|---|
| Content version | `0.0.10.P19.7` |
| Content digest | `Shanmen.DivineSense.ProductAuthority.r1` |
| Action definition | canonical Divine Sense action |
| Scan rule | `Spell.DivineSense.Pulse.Basic01` |
| Radius | `1200.0` |
| Maximum results | `8` |
| Occlusion | `RevealOccluded` |
| Required subject tag | exact `TargetLiving` |
| Self target | rejected |
| Cost rule | `Spell.DivineSense.SpiritEnergy.Basic01` |
| Resource / amount | `SpiritEnergy / 10.0` |
| Pulse capacity | `16` |
| Scan ordinal | `0` |
| Subject Actor budget | `32` |

Config 仍由既有不可变 `ProductConfig` 捕获并确定性派生 ConfigId。`IsCanonicalConfig` 逐字段核对全部策略；结构有效但任一值不同的 config 不能取得 Run identity。

这些数值是版本化开发默认值，不代表最终平衡定案。

## 5. Combat Run 激活权威

`Fdemo_mapPlayerDivineSenseActionReservation` 只允许 Combat Run Coordinator 填充，并保存：

- 本 Run 单调 `ActivationSequence`；
- canonical `ConfigId`；
- 不可变 `FShanmenCombatActionSnapshot`。

Action 的 owner/source 必须是当前 player entity，不允许伪造 source item；definition、content version/digest 与 `SourcePlayer` tag 必须精确匹配产品权威。ActivationId 从 RunId、source、definition 与序列确定性派生，reservation 会重算并验证自身来源。

未开始的 Run、非 canonical config 与耗尽序列全部失败关闭，且不推进序列。有效 reservation 发布后才推进一次。Run teardown/reset 清理局部序列；不同 Run 的 sequence 1 仍产生不同 ActivationId。

## 6. 完整 Intent 与 P19.6 兼容

`PrepareIntent(Coordinator)` 不接受调用方 config、action 或 identity：

1. 创建 canonical config；
2. 从 ready Combat Run 取得一次 reservation；
3. 从 activation/config/ordinal/budget 派生 IntentId；
4. 捕获 P19.6 所需的 immutable Intent。

返回值以 typed status 区分 config、reservation 与 intent 拒绝。若极端情况下 reservation 后 Intent 捕获失败，序列保持已消费，避免身份重用。

Controller compatibility 测试证明该 config/Intent 可穿过既有 P19.6 canonical route：空 subject batch 不读取 provider，首次 accepted 仅扣除 10 SpiritEnergy，且 command/intent 计数各增加一次；Controller 随后在 Run 之前精确 teardown。

## 7. 自动化结果

新增 exact tests 6 项：

- `CanonicalConfig`；
- `ReservationFences`；
- `SequentialReservation`；
- `RunReset`；
- `IntentComposition`；
- `ControllerCompatibility`。

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | `Product.DivineSenseProductAuthority` | 6/0 | `46EE64E6C982813ADD9E11FC73A4BA4D456A91C83B3234DB7FDF842DD5E58661` |
| `automation_full.log` | `Shanmen.0_0_10` | 822/0 | `DEA4EE599F6BB0D0F5B65A8FE43BE44F267EECAC5175FEF077183E7293E251F9` |
| `automation_legacy_attributes.log` | `demo_map.V3.Attributes` | 4/0 | `E8C5E0EAF00C14B32677DBDC8C5B7BC218421BD0BD7AB779B32CC24F4522D65D` |
| `automation_legacy_enemy_skill.log` | `demo_map.EnemySkillFramework` | 44/0 | `29E8800163750025F9DBD3EBEE8B18AF219B4441DF20A72BB4E520E5F007A573` |
| `automation_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | `6AAFE7186B355E1F0AF8AE920E9B2B5C388BFB0B921ECE5EF7815AD3172231D9` |
| `automation_legacy_item_armor.log` | `demo_map.ItemUseAndArmor` | 46/0 | `CB34D90565B224362D0AC8E7F0BC88F0E1E2231E8575DA3490966DF990673F75` |

全量由 P19.6 的 816 增加到 822。六份正式日志均只有一个 canonical RunTests command、Fail 0、一个 queue-empty terminal、Fatal/Unhandled/Ensure 0。证据审计：`PASS Logs=6 RecordedSuccess=944`，SHA-256 `01EBCA3779FA861299ED86DBFB791F5FC88678CBDE70826F9A497751F50FDA19`。

## 8. 改动路径门禁与静态边界

真实 changed-file gate：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=26 Logs=6
```

- gate SHA-256：`A222D60DA0A2A55E13056B3EEDE7E3EAF65ABE5F9D3FC74F989A798E15B9B5FA`；
- self-test：`303/303 PASS`，SHA-256 `47E1CB744DCA6F7356C4FC3BD55C119F4A0EE13653599CDA2691348971B482AD`；
- production boundary scan：`PASS Files=2 Matches=0`，SHA-256 `E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`；
- `git diff --check`：PASS。

边界扫描禁止生产权威直接依赖 World/Actor 查询、trace/sweep/overlap、spawn/destroy、damage mutation、RNG、Timer/Tick、输入组件或 Widget。测试可创建 transient World 证明与既有 Controller 的组合，但生产 authority 保持纯值与 Run 协调边界。

## 9. 构建与产物

- Editor initial：121 actions / 181.32s / native 0，SHA-256 `0A657F695A1F18C0DE160239A3AD4287F0313CB048B8BF220E5A7D03BD735641`；
- Game final：120 actions / 153.90s / native 0，SHA-256 `848CFC8632AF4C798513760BA71FA72D76BDD1621AB6F264A6D0942D5094268A`；
- Editor final：up to date / 1.22s / native 0，SHA-256 `588168F2A8FEA803448C8BEDF6B6A434890A5F4252496CB5212311F170BC2FF8`。

产物：

- `demo_map.exe`：356,916,736 bytes，SHA-256 `3CA8E2F80881D5D85004062DCC10295CB7BFADC55A7D56787195363300260021`；
- `UnrealEditor-demo_map.dll`：15,606,784 bytes，SHA-256 `89FD4D0D793355B1AE95EA6D4AD325A18F00136A19DD2F0EE59BC59EDDE17EC7`。

## 10. 异常、P/F 边界与下一步

首次全量运行把约 1 MB UE 日志持续回显到受控终端，输出管道反压导致纯内存测试出现长 tick。该运行在 674 Success / 0 Fail 时主动终止，原始日志完整保留为 `automation_full_streaming_aborted.log`，SHA-256 `F158C6B47FCC8EB46AE9EB3CD98321AD0D1E0327BDDAB624DE8580B8D9C7B3AA`。改为完整文件直写后，同一 822 项范围原生退出 0；没有缩小范围或把中断结果当成成功。

门禁第一次外层调用在门禁正文 PASS 后错误读取了空 `$LASTEXITCODE`；去除错误包装后按门禁证据与脚本异常传播复核通过。这不是产品、测试或门禁本体失败。

本轮没有运行 Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

P19.8 建议建立 Run-scoped Divine Sense Product Route：接收不含策略与身份的 device-independent use request，组合本权威、P19.6 Controller、既有 SpiritEnergy snapshot 与显式 World subject batch；仍不接物理按键、UI 或隐式 Actor discovery。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-7-divine-sense-product-authority>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-7-divine-sense-product-authority/Docs/Report/Dev.D.UE.0.0.10.P19.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-7-divine-sense-product-authority/Docs/Log/Dev.D.UE.0.0.10.P19.7.r0_log.md>
