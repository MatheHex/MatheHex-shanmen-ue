# Dev.D.UE.0.0.10.P19.2.r0 Development Log

## 1. 目标与基线

- 基线提交：`005897e1b6a9bdc2b0d520a1d9b1083df4df7c35`（P19.1）；
- 分支：`agent/0.0.10-p19-2-divine-sense-pulse-coordinator`；
- 目标：把动作生命周期、SpiritEnergy 事务与一次 P19.1 World observation 组合成可回滚、可重放、有容量上限的神识产品脉冲；
- 约束：不做 Actor discovery、持续 Tick、物理输入、UI/表现、目标 mutation、存档或平衡定案。

## 2. 架构决策

协调器置于 `demo_map` 产品组合层，复用：

- `FShanmenActionOrchestrator` 的 action transition 权威；
- `FShanmenActionResourceAuthority` 的 reserve/finalize 与 revision 权威；
- P19.1 World observation adapter；
- P19.0 Divine Sense definition/resolver；
- `FShanmenWorldEntityRegistry` 的 Run/entity 身份权威。

没有建立第二套 SpiritEnergy ledger。调用方继续持有唯一资源 authority，协调器只在私有副本上 staging，成功后整体替换。cost amount 由内容层捕获；协调器只固定其资源 channel 必须是 SpiritEnergy。

## 3. 新增文件

- `Source/demo_map/demo_mapShanmenDivineSensePulseCoordinator.h`；
- `Source/demo_map/demo_mapShanmenDivineSensePulseCoordinator.cpp`；
- `Source/demo_map/demo_mapShanmenDivineSensePulseCoordinatorTests.cpp`。

同时更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

## 4. 协调器实现

`TryCreate` 固定 RunId 和正数 processed-pulse capacity，并派生确定性 coordinator ID。`Execute` 对新 activation：

1. 完成 coordinator、Run、action/definition/cost、ordinal 和 budget admission；
2. 复制 coordinator 与调用方 ResourceAuthority；
3. 在临时 action runtime 中完成 Startup；
4. 从临时 authority snapshot 创建 reservation，并执行 Reserve；
5. 动作进入 Active commit point，并以 Commit disposition Finalize 资源；
6. 调用 P19.1 处理显式 Actor 集合；
7. 动作进入 Recovery，再以 Completed 返回 Idle；
8. 构造并自校验完整脉冲 receipt；
9. 写入临时 ledger，校验两份临时状态后才发布。

所有拒绝都发生在正式状态发布前。World/provider 拒绝会保留 P19.1 原始状态和诊断，但丢弃 staged resource revision 与 reservation/finalization。

## 5. 重放与一致性

processed ledger 以 ActivationId 索引完整 receipt。重放先检查 action、definition、cost、scan ordinal 与 budget 完全一致，再在 ResourceAuthority 副本上重放 reservation/finalization 请求，要求得到精确 `AlreadyReserved` / `AlreadyFinalized` 及相同 receipt ID。

通过后直接返回 stored proof，不访问 World、source Actor、当前 Actor 集合或 evidence provider。这样已提交 activation 不会被当前 World 状态重新采样，也不会二次消耗资源。资源 ledger 不匹配、不可变输入冲突或新 activation 超容量均失败关闭。

## 6. 回执与诊断

新增：

- `Edemo_mapShanmenDivineSensePulseStatus`；
- `Edemo_mapShanmenDivineSensePulseError`；
- `Fdemo_mapShanmenDivineSensePulseReceipt`；
- `Fdemo_mapShanmenDivineSensePulseResult`；
- `Fdemo_mapShanmenDivineSensePulseCoordinator`。

结果区分首次 Applied、AlreadyApplied replay 与结构化 Rejected。错误分类覆盖 coordinator/input/Run/activation/capacity、resource owner/channel/snapshot/reservation/finalization、action 各阶段、World observation 和 state desynchronization。World 拒绝单独保留 `WorldFailure`，便于调用方定位 P19.1 边界。

receipt ID 绑定 coordinator、action/definition/cost、资源证明、World receipt、四段 transition、budget 与 observed count；`IsValid()` 重新验证完整关系而不是信任外部 GUID。

## 7. 自动化改动

新增 4 条 exact 测试：

- `AtomicCommit`；
- `ReplayAndCapacity`；
- `AtomicRollback`；
- `AdmissionFences`。

覆盖一次成功消费、固定动作序列、World reveal、零 I/O 重放、重放冲突、ledger 失同步、容量上限、资源不足、World evidence 失败回滚、同 activation 修复后重试，以及 coordinator/Run/action/resource/budget admission。

回归映射新增 `DivineSensePulseCoordinator` 规则，要求：

- `Shanmen.0_0_10.Product.DivineSensePulseCoordinator`；
- `Shanmen.0_0_10.Product.DivineSenseWorldObservation`；
- `Shanmen.0_0_10.CombatRuntime.DivineSense`；
- `Shanmen.0_0_10.CombatRuntime.ActionResource`；
- `Shanmen.0_0_10.CombatRuntime.ActionLifecycle`；
- `Shanmen.0_0_10.WorldGameplay`。

自测从 291 增至 293，包含完整组合证据通过与仅提供 focused evidence 时失败关闭。

## 8. 测试与门禁证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | P19.2 exact | 4/0 | `902C219E88B652CA14BD42E75EC2D9C6625B0402D1156B4D76F6626795BF683A` |
| `automation_world_observation.log` | P19.1 World observation | 4/0 | `FAF1F76852C4622D879C18F88682530C5F36DC1993C490E38E2B41F499676677` |
| `automation_divine_sense_runtime.log` | P19.0 Divine Sense | 4/0 | `A297FDAADB85765B22C97FA1C412BD86281B9A82F8CC534529D000AF2E651CDB` |
| `automation_action_resource.log` | ActionResource | 7/0 | `08EC00CDDD421FA8982704DC0332A99E48C8AC023E2E36CCF44D15CC0DC1882C` |
| `automation_action_lifecycle.log` | ActionLifecycle | 1/0 | `766B5CB9AA716E3F940C75900DBED97FBA32A11CC3B2998C59A7A9F33A0A4D25` |
| `automation_world_gameplay.log` | WorldGameplay | 10/0 | `254A5D3BF1B98402C670CD56A079A112DB3E56356D04FB99A2CBD8E808CA5F83` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 800/0 | `1F3BED185721167A50DDB14B4BD38FACA87859CE95B77750C4AC0EC799D06CF0` |
| `regression_coverage.log` | changed-path gate | PASS | `EDDC169A52CD911D2E8B725817CE0C7399D7F89FB301D7A8E70B21A0714FC2C1` |
| `regression_coverage_selftest.log` | gate self-test | 293/293 | `2C05FA17F1C9FF95EE3FC62088B0AFC98729C9C40CBB2D58592A54FEE6E6F8AB` |

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=6 Logs=6
SELF_TEST: PASS 293/293
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

六个 focused 日志与全量日志均为 Fail=0，具有 UE 5.8 原生 terminal 0，且未出现 Fatal/Unhandled/Ensure 标记。全量首末 Success 间隔约 33m03.513s。

## 9. 构建、诊断与产物

- Editor initial：5 actions，15.23s，native 0，log SHA `3975B3100B4B9E3A6F0F64A0CAC49D2D9B2DC6602219474DBE6A816DB893F5B1`；
- Game final：4 actions，22.19s，native 0，log SHA `2BA2193E96A3842339170564AE06E5E9F16F0A47D304576D88D023F5A82726FF`；
- Editor final：up to date，0 actions，0.98s，native 0，log SHA `4379F9D11145E8663CD05488E6E933292D791F9E59B03BC4BC6DE7A52C65F1D3`；
- `demo_map.exe`：356,654,080 bytes，SHA `96E993847AEA21563B5F5B23AD24B1A820C74DEAC69990A8D7D4C2F38BECBAE8`；
- `UnrealEditor-demo_map.dll`：15,296,000 bytes，SHA `C6A1662F192DDE5DEFBCE5CC11A8874D7DA1B3CA533765BF92187F984581E755`。

新增源码第一次 Editor 构建和 exact 测试均直接通过，没有产品代码修正或失败测试。最终双构建也通过。构建完成后，留档命令尝试以不兼容的 `Tee-Object -LiteralPath -Append` 参数追加退出码，产生一次仅限终端的参数错误；实际 Game/Editor 原生退出均为 0，构建日志未被该错误污染，随后用 `apply_patch` 补入 `NATIVE_EXIT=0`，未把留档问题误报为构建失败，也未重复构建。

全量仍经过既有 SwordRhythm checkpoint/journal 慢用例；这些用例持续推进并最终全部 Success。该耗时未触发代码或测试修正。

## 10. P/F 与提交边界

本轮只完成 P 阶段 C++ 产品协调、无头自动化、静态边界审计和 Development 构建。没有运行 Editor UI、PIE、Standalone 或游戏可执行文件，也没有执行真实输入、截图、Smoke、Cook、Package。

计划精确提交 3 个新增源码、2 个回归工具文件、本 Report 与本 Development Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 和用户文件不修改、不暂存；raw logs 仅保留在 `Saved/Codex/P19.2`。

P19.3 建议由 Run-scoped product host 持有协调器和资源 authority，接收显式 command/候选集并发布本轮 immutable result；仍不自行发现 Actor，不接真实输入/UI/表现，不冻结最终 SpiritEnergy 数值。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-2-divine-sense-pulse-coordinator>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-2-divine-sense-pulse-coordinator/Docs/Report/Dev.D.UE.0.0.10.P19.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-2-divine-sense-pulse-coordinator/Docs/Log/Dev.D.UE.0.0.10.P19.2.r0_log.md>
