# Dev.D.UE.0.0.10.P19.0.r0 Report

## 1. 结论

P19.0 在 P 阶段边界内完成，结论为 **PASS**。

本轮为规划中的“神识感知 / 探路”建立第一份纯运行时合同：一次神识脉冲由不可变动作、内容规则、空间原点与扫描序号确定；外部世界层只提交已经采样的对象证据；纯 resolver 负责距离、标签、自身与遮挡过滤，以稳定顺序生成不暴露被拒对象的只读回执。

```text
Divine Sense exact:             4 Success / 0 Fail
CombatRuntime mapped:         130 Success / 0 Fail
Shanmen.0_0_10 full:          792 Success / 0 Fail
Regression coverage:           PASS (Changed=3 / Rules=1 / Required=1 / Logs=1)
Regression gate self-test:      PASS 289/289
Boundary scan:                  PASS (Files=2 / Matches=0)
Game + Editor Development:      PASS / native status 0
```

没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件或真实输入；没有执行截图、Smoke、Cook 或 Package。本轮不宣称最终扫描半径、数值消耗、UI、声画表现、世界查询或产品手感已完成。

## 2. P19.0 冻结边界

本轮冻结的是“已经采样的证据如何确定性地产生神识发现结果”，不是实际施法链：

- 冻结 canonical action：`Combat.Action.Spell.DivineSense.Pulse`；
- 冻结内容规则、扫描请求、对象观察、揭示结果与扫描回执的身份；
- 冻结自目标、距离、标签与遮挡策略的过滤顺序；
- 冻结最近优先、同距离按对象 GUID 排序、结果容量裁剪；
- 冻结重复证据的幂等与冲突证据的失败关闭。

半径和容量仍是内容数据，不被 P19.0 写死为最终平衡值。世界枚举、碰撞/视线 trace、SpiritEnergy 消耗、目标变更、输入、UI、提示音、特效和持久化均留给后续消费层。

## 3. 不可变合同

新增 `ShanmenDivineSenseScan.h/.cpp`，包含五层值合同：

1. `FShanmenDivineSenseDefinition`：从可编辑 capture 冻结规则 ID、半径、容量、遮挡策略、必需/阻止标签与 self policy；
2. `FShanmenDivineSenseScanRequest`：绑定完整 action snapshot、definition、canonical origin 与非负 scan ordinal；
3. `FShanmenDivineSenseObservation`：绑定精确 ScanId、对象 GUID、位置、标签、LOS 与 authority revision；
4. `FShanmenDivineSenseReveal`：只保存通过规则的 observation、稳定距离平方和 RevealId；
5. `FShanmenDivineSenseScanReceipt`：只保存 request 与排序、裁剪后的 reveal 列表。

运行时合同的字段均为 private `VisibleAnywhere / BlueprintReadOnly`，只能由 capture 或 resolver 构造。默认对象、未知枚举、非有限坐标、非正半径、空对象身份、负 revision/ordinal 和冲突标签策略均失败关闭。

## 4. 确定性身份

DefinitionId、ScanId、ObservationId、RevealId 和 ReceiptId 均通过既有 `FShanmenDeterministicId` 从 canonical parts 派生：

- 浮点值按 IEEE 位模式编码，`-0.0` 先规范为 `0.0`；
- GameplayTags 在进入身份前按名称排序；
- action identity 包含 Run、Owner、Activation、Source、Item、Definition 与内容版本/摘要；
- observation identity 包含 LOS 与 authority revision，避免把不同权威时刻误判成同一证据；
- scan ordinal 区分同一 activation 的重复脉冲。

等价输入重放得到相同身份；输入排列顺序不会改变最终 ReceiptId。

## 5. 过滤与歧义规则

resolver 对所有输入先做结构与 ScanId 校验，再按对象 GUID 聚合：

- 同一对象、同一 ObservationId 的完全重复证据被幂等去重；
- 同一对象出现不同 ObservationId 时整轮失败关闭，即使其中一条最终会被过滤；
- `bRejectSelf` 在启用时拒绝动作来源对象；
- 半径边界为 inclusive；
- 必需标签全部满足且不得命中阻止标签；
- `VisibleOnly` 拒绝无 LOS 证据，`RevealOccluded` 可揭示并在结果中保留 `WasOccluded`；
- 合格结果先按距离平方升序，再按对象 GUID 升序，最后执行 `MaximumResults` 裁剪。

空输入是有效扫描，返回具有确定身份的空回执。

## 6. 消费者安全

回执只包含通过规则的 reveal。被 self、距离、标签或遮挡策略拒绝的 observation 不进入回执，因此 UI、AI 或网络消费层不能从该值枚举被规则隐藏的对象。

P19.0 不保存 resolver 状态、不持有 Actor/World 引用、不建立 ledger，也不把回执解释为资源已扣除、施法已接受或目标已受影响。后续产品层必须显式拥有采样、资源、动作 admission 和表现生命周期。

## 7. 精确自动化

新增 4 条 exact 用例：

1. `DefinitionAndRequest`：定义、canonical zero、重复身份、scan ordinal 及无效动作/数值/策略失败关闭；
2. `ObservationEvidence`：对象、标签、LOS、revision 和坐标身份，以及空身份/空标签/负 revision/非有限位置拒绝；
3. `FilteringAndVisibility`：self、范围内外、边界距离、必需/阻止标签、可见性和有效空结果；
4. `DeterminismAndAmbiguity`：容量最近优先、隐藏对象策略、输入重排、完全重复去重、冲突重复和 foreign ScanId 拒绝。

exact 实测 4/0。完整 `Shanmen.0_0_10` 从 P18.10 的 788 增至 792，实测 792/0；首末 Success 为 `2026-09-03 08:07:24.268 -> 08:39:38.263 UTC`，约 32m13.995s。

UE 5.8 在引擎初始化前输出的 13 条内置 `UE::UnifiedErrorTest` 条件测试噪声与 P18.10 基线相同；它们不属于 Shanmen 测试结果。最终日志中 Shanmen Fail=0，且存在原生 `TEST COMPLETE / EXIT CODE: 0`。

## 8. 改动驱动回归证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Divine Sense exact | 4 | 0 | `6BD46583611F2A338ED58BA634E823FD791378B6671C4F65375955A0097E8E77` |
| `Shanmen.0_0_10.CombatRuntime` | 130 | 0 | `5415BC095282F18142B9846DC443AD1E274F28765B0B097A5C52C7EB4C0C40F5` |
| `Shanmen.0_0_10` full | 792 | 0 | `B0923219C16C2590C173A0068A8BAB9E3D5D0A5BCBD2B28BE3C6D5B88C0343B2` |
| regression coverage | PASS | 0 | `20286C404C442FB6E0134AFF45B263643F3C5A7D569D7BED32B65852C5154514` |
| coverage self-test | 289 | 0 | `5D88EF74BC9E6F9FC610DB687385C9B4B16C4FFB869C9D3FE99C3F48A891A7D4` |

既有 generic CombatRuntime 规则已覆盖本轮 3 个新增源码路径，因此无需扩张映射表：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 289/289
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

## 9. 构建、产物与边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded | 8 / 27.33s | `0627F837A497D211E1BBDC2F30620A6ACF5A3A74BBA4485672E445EA1575A66C` |
| Game Development final | Succeeded / native 0 | 5 / 27.47s | `1367F48F360538E601F6F4858F5C0A9E8311F440DF3F2BF12A763E89B642571C` |
| Editor Development final | Succeeded / native 0 | 0 / 1.05s | `434CDDDD3E5959B5EA624558A2541FFA7103A10D386751FA495E882F2798797C` |

最终产物：

- `demo_map.exe`：356,548,608 bytes，SHA-256 `7183751AD5DA03B399F363D94C2716FD8E892FCF5708FB016267149536BD8693`；
- `UnrealEditor-ShanmenCombatRuntime.dll`：1,922,560 bytes，SHA-256 `6A2F7996B13FB192D9C4B25A54E74B48AAFFF5F4C161C64F67D034CAAD7F756A`。

生产 header/cpp 的严格边界扫描为 0 命中：无 Engine/GameFramework、World/Actor、trace/sweep/overlap、damage、Commit/Consume、Tick/timer、随机、输入、Widget、声音或 Niagara 依赖。

## 10. 提交边界与后续

基线提交为 `49640b6617d3e27ecc9ee0d21feee8ad086ca131`。本轮只提交 3 个生产/测试源码、本 Report 与本 Development Log，共 5 个文件；长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存，`Saved/Codex/P19.0` raw logs 仅本地保留。

下一轮建议为 P19.1 建立 World observation adapter：只负责一次有界采样并产出 P19.0 observation，不拥有过滤、资源、UI 或目标 mutation。真实输入绑定、SpiritEnergy 消耗、表现与产品验收仍需明确进入相应 F 阶段后执行。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-0-divine-sense-scan-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-0-divine-sense-scan-contract/Docs/Report/Dev.D.UE.0.0.10.P19.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-0-divine-sense-scan-contract/Docs/Log/Dev.D.UE.0.0.10.P19.0.r0_log.md>
