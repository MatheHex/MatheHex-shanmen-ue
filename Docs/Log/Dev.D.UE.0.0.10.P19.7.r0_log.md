# Dev.D.UE.0.0.10.P19.7.r0 Development Log

## 1. 目标与基线

- 基线：`f5790511f28c81d72645bb5699b86910d1ec10c8`（P19.6）；
- 分支：`agent/0.0.10-p19-7-divine-sense-product-authority`；
- 目标：建立唯一 canonical Divine Sense Product Authority，由 Combat Run 分配 activation identity，并生成 P19.6 Controller 所需的 immutable Intent；
- 约束：不接物理输入、UI/表现、隐式 Actor discovery、持续 Tick/Timer、存档或最终数值。

## 2. 设计审计

审计了 P19.1 runtime、P19.2 pulse coordinator、P19.3 host、P19.4 router、P19.5 session、P19.6 controller，以及 Sword Qi、Spirit Evasion、Weapon Guard 的 product authority 与 Combat Run reservation 模式。

P19.6 已拥有完整的 Controller transaction，但仍允许上游分别提供 config、action 与 IntentId。即使各结构能自证有效，上游仍可能在不同调用点复制产品常量或自行拼接身份。P19.7 因此只收口策略与身份，不复制既有扫描、资源、World evidence、receipt 或 replay 权威。

## 3. Canonical config

`Fdemo_mapShanmenDivineSenseProductAuthority` 固定 content version/digest、definition、scan rule、radius、maximum results、occlusion、subject tags、自目标策略、SpiritEnergy cost、pulse capacity、scan ordinal 与 subject budget。

`TryCreateCanonicalConfig` 仍通过既有不可变 definition/cost/config capture 链生成 ConfigId；`IsCanonicalConfig` 同时检查 config identity 与全部字段。结构有效但 scan rule 等任一值不同的 config 被拒绝，不能消耗 Run 序列。

配置为 P 阶段版本化默认值，不是最终平衡数据。

## 4. Run-owned reservation

Combat Run Coordinator 增加独立 `NextPlayerDivineSenseActivationSequence`，初始为 1，并在成功 end/reset 时恢复为 1。

`TryReservePlayerDivineSenseAction` 的顺序：

1. 要求 ready Run；
2. 要求 exact canonical config；
3. 拒绝 0 或耗尽序列；
4. 从 current Run/player/canonical definition/sequence 派生 ActivationId；
5. 捕获 immutable action，重算并验证 reservation；
6. 一次发布 reservation 后才推进序列。

Action 不携带 item identity，owner/source 都是 current player，并固定 content version/digest 与 `SourcePlayer` tag。Reservation 的私有字段只有 Coordinator 可写。

## 5. Intent composition

`PrepareIntent` 的公共入口只接收 Coordinator，不接受调用方策略或身份。它依次生成 config、Run reservation 与 authority-derived IntentId，再捕获 scan ordinal 0、subject budget 32 的 P19.6 Intent。

`PrepareResult::IsReady` 验证 status、diagnostic、canonical config、reservation provenance、IntentId、action 全字段、ordinal 与 budget。reservation 后若 Intent 捕获发生不可达异常，序列仍保持已消费，杜绝 identity reuse。

## 6. 测试开发

新增 6 项 exact contract：

- `CanonicalConfig`：稳定 ConfigId、全字段策略与非 canonical 配置识别；
- `ReservationFences`：unready Run、alternate config 与不推进序列；
- `SequentialReservation`：序列 1/2、Action provenance 与不同 identity；
- `RunReset`：teardown 后局部序列重置、跨 Run identity 隔离；
- `IntentComposition`：调用方不可选 config/action/id/ordinal/budget；
- `ControllerCompatibility`：通过 P19.6 route、零 subject 不读 provider、精确扣 10 SpiritEnergy、精确 teardown。

初始 Editor Development 构建：121 actions，181.32 秒，native 0。

## 7. 自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `automation_exact.log` | 6/0 | `46EE64E6C982813ADD9E11FC73A4BA4D456A91C83B3234DB7FDF842DD5E58661` |
| `automation_full.log` | 822/0 | `DEA4EE599F6BB0D0F5B65A8FE43BE44F267EECAC5175FEF077183E7293E251F9` |
| `automation_legacy_attributes.log` | 4/0 | `E8C5E0EAF00C14B32677DBDC8C5B7BC218421BD0BD7AB779B32CC24F4522D65D` |
| `automation_legacy_enemy_skill.log` | 44/0 | `29E8800163750025F9DBD3EBEE8B18AF219B4441DF20A72BB4E520E5F007A573` |
| `automation_legacy_v2_ranged.log` | 22/0 | `6AAFE7186B355E1F0AF8AE920E9B2B5C388BFB0B921ECE5EF7815AD3172231D9` |
| `automation_legacy_item_armor.log` | 46/0 | `CB34D90565B224362D0AC8E7F0BC88F0E1E2231E8575DA3490966DF990673F75` |

正式证据审计逐份要求：唯一 RunTests command、Success 非零、Fail 0、唯一 terminal marker、Fatal/Unhandled/Ensure 0。结果：

```text
EVIDENCE_AUDIT: PASS Logs=6 RecordedSuccess=944
```

审计日志 SHA-256：`01EBCA3779FA861299ED86DBFB791F5FC88678CBDE70826F9A497751F50FDA19`。

## 8. Changed-file regression gate

新增 `DivineSenseProductAuthority` mapping，要求 Product Authority、Controller、Session、Router、Host、Pulse Coordinator、World Observation、Combat Run、Divine Sense runtime、Action Resource、Action Lifecycle 与 WorldGameplay。

Coordinator 本身的既有广域映射叠加后，真实 gate 为：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=26 Logs=6
```

完整 `Shanmen.0_0_10` 父组覆盖 0.0.10 的 required subgroups；四个 legacy 组分别提供 Attributes、Enemy Skill、V2 Ranged 与 Item/Armor 证据。

- gate SHA-256：`A222D60DA0A2A55E13056B3EEDE7E3EAF65ABE5F9D3FC74F989A798E15B9B5FA`；
- self-test：新增一正一反后 `303/303 PASS`；
- self-test SHA-256：`47E1CB744DCA6F7356C4FC3BD55C119F4A0EE13653599CDA2691348971B482AD`。

## 9. 静态边界与构建

生产 authority header/cpp 禁止 World/Actor 查询、trace/sweep/overlap、spawn/destroy、damage mutation、RNG、Timer/Tick、输入与 Widget：

```text
BOUNDARY_SCAN: PASS Files=2 Matches=0
```

boundary log SHA-256：`E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`。`git diff --check` 通过。

构建：

- Editor initial：121 actions / 181.32s / native 0；log SHA-256 `0A657F695A1F18C0DE160239A3AD4287F0313CB048B8BF220E5A7D03BD735641`；
- Game final：120 actions / 153.90s / native 0；log SHA-256 `848CFC8632AF4C798513760BA71FA72D76BDD1621AB6F264A6D0942D5094268A`；
- Editor final：up to date / 1.22s / native 0；log SHA-256 `588168F2A8FEA803448C8BEDF6B6A434890A5F4252496CB5212311F170BC2FF8`。

产物：

- `demo_map.exe`：356,916,736 bytes / `3CA8E2F80881D5D85004062DCC10295CB7BFADC55A7D56787195363300260021`；
- `UnrealEditor-demo_map.dll`：15,606,784 bytes / `89FD4D0D793355B1AE95EA6D4AD325A18F00136A19DD2F0EE59BC59EDDE17EC7`。

## 10. 异常记录

第一次全量测试使用实时 `Tee` 回显约 1 MB UE 日志。受控终端输出反压使 Automation tick 出现 30–90 秒停顿。源码核查确认受影响测试为纯内存 checkpoint/identity 操作；进程与日志没有 Fail 或 crash。该运行在 674 Success / 0 Fail 时主动停止，原始日志保留为 `automation_full_streaming_aborted.log`，SHA-256 `F158C6B47FCC8EB46AE9EB3CD98321AD0D1E0327BDDAB624DE8580B8D9C7B3AA`。

同一命令、同一 822 项范围改为 file-only capture 后原生退出 0，正式日志即上表 `automation_full.log`。没有裁剪测试范围或删除首次运行证据。

真实 gate 首次外层包装在门禁正文已 PASS 后检查空 `$LASTEXITCODE`，错误抛出 wrapper failure。PowerShell 脚本本体依靠异常传播而不保证设置 native code；去掉错误检查后，PASS 正文、全部 evidence 与 gate log 均复核一致。

## 11. P/F 与提交边界

本轮只执行 P 阶段源码、无头 Automation、静态扫描和 Development build。没有运行 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

提交范围为 3 个新增源码、2 个 Coordinator 文件、2 个回归流程文件、本 Report 与本 Log。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件不修改、不暂存；raw logs 只保留在 `Saved/Codex/P19.7`。

下一阶段建议 P19.8：建立 device-independent、Run-scoped Divine Sense Product Route，组合本权威与 P19.6 Controller/既有资源和显式 World batch；继续禁止物理输入、UI 与隐式 Actor discovery。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-7-divine-sense-product-authority>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-7-divine-sense-product-authority/Docs/Report/Dev.D.UE.0.0.10.P19.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-7-divine-sense-product-authority/Docs/Log/Dev.D.UE.0.0.10.P19.7.r0_log.md>
