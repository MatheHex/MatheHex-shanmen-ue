# Dev.D.UE.0.0.10.P19.4.r0 Development Log

## 1. 目标与基线

- 基线提交：`f239727e7bd4644cd56dce2edfb4d7e7661da2ae`（P19.3）；
- 分支：`agent/0.0.10-p19-4-divine-sense-command-router`；
- 目标：建立 Run-scoped Divine Sense command router，把已准备的 immutable action/content/cost 与显式 Actor batch 安全路由至唯一 P19.3 Host；
- 约束：不接物理输入、Actor discovery、持续 Tick、Timer、UI/表现、目标 mutation、存档或最终数值定案。

## 2. 设计审计

审计了 P19.3 Product Host、P19.2 Pulse Coordinator、P19.1 World Observation Adapter，以及已有的 ControlledWeapon、ThrownWeapon、SpiritEvasion 和 SwordQi command router。

P19.3 已经提供 deterministic pulse command、Host 内原子 staging、资源/Action/World 证明和无 World I/O replay。缺口位于调用边界：Actor 集合尚未成为 pointer-free command 身份，调用方没有乐观资源快照围栏，Host 与上层 replay ledger 也未形成一一对应关系。

选定状态化 Router，而不是复制资源或扫描权威。Router 只持有已接受命令证明和资源 snapshot chain；唯一可变 SpiritEnergy 仍由 P19.3 Host 内的 `FShanmenActionResourceAuthority` 持有。

## 3. 新增文件

- `Source/demo_map/demo_mapShanmenDivineSenseCommandRouter.h`（164 行）；
- `Source/demo_map/demo_mapShanmenDivineSenseCommandRouter.cpp`（788 行）；
- `Source/demo_map/demo_mapShanmenDivineSenseCommandRouterTests.cpp`（699 行）。

回归工具更新：

- `Scripts/ShanmenRegressionMap.json`：新增 14 行 Router 映射；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 25 行正/反例。

## 4. Route Command 捕获

`TryCaptureCommand` 完成以下有界操作：

1. 验证 Router 与 Host 当前状态一致；
2. 验证 registry 与 action 都属于固定 Run/source；
3. 用 P19.3 factory 捕获完整 pulse command；
4. 只遍历调用方显式提供的 subject Actor 数组；
5. 用 registry 解析每个稳定 entity ID，拒绝失效、未注册或重复对象；
6. 按 GUID digits 规范排序；
7. 冻结当前 resource snapshot，并从完整 payload 派生 RouteCommandId。

同一已接受 activation 的等价再次捕获返回原 command；同 activation 的不同 pulse payload 或 subject set 拒绝重新捕获。命令本身不保存 UObject 指针。

## 5. Router 打开与一致性

`TryCreate` 只接受有效且 `NumProcessedPulses == 0` 的 Host。RouterId 绑定 HostId、opening snapshot 和 processed capacity。

`IsValid` 对 processed map 建立序列视图并验证：

- map key、route command、首次 Applied result 三者身份一致；
- sequence 连续且唯一；
- Run/source/Host/Router 身份一致；
- opening → 每个 command expected snapshot → Host `ResourceBefore/After` → current snapshot 构成连续链；
- current revision 等于 opening revision 加 `2 * accepted count`；
- 每项 Host receipt 的 observed subject count 等于冻结 entity count。

`IsConsistentWithHost` 再验证 Host opening/current snapshots、capacity 和 processed count。Router 外推进 Host 会被检测为 desynchronization。

## 6. 路由与原子提交

首次 route 在 live 读取前依次执行 command、Host、Run、source、capacity、resource projection 和 registry 围栏。Source Actor 必须解析为冻结 source entity；subject Actors 重新解析、规范排序后必须与 command entity IDs 完全相同。

通过围栏后，Router 复制 Host，在 Host copy 上执行 P19.3，再复制 Router 并加入首次 Applied record。只有 Host result、Router result、完整 snapshot chain 与 Host/Router consistency 同时成立时，才把两个副本一起发布。

Host rejection 会保留完整 nested error 与 World failure；原 Host、Router、余额、revision 和 ledger 不变。修复 provider evidence 后，相同 command 可恢复。

## 7. Replay、stale 与冲突

已接受 RouteCommandId 在 live registry/Actor 检查前命中。Router 以空 World/source/subjects 调用 Host replay seam；P19.3 必须返回同 receipt 的 `AlreadyApplied`。该路径不调用 provider，也不修改 Host/Router。

未记录命令按以下优先级失败关闭：

- 同 ActivationId 不同 payload/set：`ActivationConflict`；
- processed capacity 已满：`ProcessedCapacityExceeded`；
- expected resource snapshot 不再是 current：`ResourceProjectionStale`；
- registry/source/subject 身份错误：对应 typed status；
- P19.3 拒绝：`HostRejected`，保留 nested proof；
- staged proof 或 ledger 链异常：`StateDesynchronized`。

## 8. 自动化与证据

新增 4 项 exact tests：

- `CaptureAndApply`；
- `ReplayWithoutLiveInputs`；
- `ProjectionCapacityAndConflict`；
- `RollbackAndIdentityFences`。

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | `Product.DivineSenseCommandRouter` | 4/0 | `AF47189D45FE755CB19E00DB4B4E9A6553DC977DA9ED9090B05E8E6BE51B705C` |
| `automation_product_host.log` | `Product.DivineSenseProductHost` | 4/0 | `912ACCF45536F95B4A7B6FC19D5006C4F38F45BAC045676A11B0B69C955A43D4` |
| `automation_pulse_coordinator.log` | `Product.DivineSensePulseCoordinator` | 4/0 | `1DD6BF8B8AE84A9B6ACE762154A600C80C58292F9CA8C3D0B7378E0F26A3E006` |
| `automation_world_observation.log` | `Product.DivineSenseWorldObservation` | 4/0 | `5C916B62CBD25B7CF6060E7568CEFB3E0179AB2F8B693CD11BF026D56AF98FEF` |
| `automation_divine_sense_runtime.log` | `CombatRuntime.DivineSense` | 4/0 | `DEEDE701AB3CDEB343C8DF12F42EF5908C1A6E9B91AE924F894DE4ADE43CF83B` |
| `automation_action_resource.log` | `CombatRuntime.ActionResource` | 7/0 | `7617A5CB591992875F0044D463A62653DF7DAB0B5395102615905DE25154569B` |
| `automation_action_lifecycle.log` | `CombatRuntime.ActionLifecycle` | 1/0 | `743FFDDC47F676E8F8A4F8A2CBD67223CB42FBE44C4C0F2EA338623E8DECA507` |
| `automation_world_gameplay.log` | `WorldGameplay` | 10/0 | `1CCE3C786F357C9486328E625941CA8CC0381613428EC3BC813314FE5DA995D7` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 808/0 | `3830DDD6D6B24C47968C16A62E2B7BD366AB82788FF5F91B3C39BB8D3D82D9CD` |

每份 automation log 都有且仅有一个 native success terminal marker，Fatal/Unhandled/Ensure 均为 0。

首次尝试仅传入绝对 `-log` 参数时没有在目标目录产生可审计文件，因此未作为证据；随后改用 stdout 完整捕获，正式 exact evidence 为上表文件，结果 4/0。没有出现产品代码或自动化断言失败。

## 9. 回归门禁、构建与边界

Router rule 要求 8 组证据，真实门禁：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=8 Logs=8
```

- gate SHA-256：`74DD76E97C834E60BA8CD5FFFFCF5B4EBB8806F56809B4CF8FCA9AC1AAD3914E`；
- self-test：`297/297 PASS`，SHA-256 `3689B996F3E85A306653DF6DEFE7DC3C2DCFB9C71DAF04574B7D2198C7B1710A`；
- production boundary scan：`PASS Files=2 Matches=0`，SHA-256 `E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`；
- `git diff --check`：PASS。

构建：

- Editor initial：5 actions / 17.47s / native 0，SHA-256 `9F3835A674CBC4D6A85B10F561878A3AF3DB90B0B1C1ADE8787926EAA60CE682`；
- Game final：4 actions / 23.30s / native 0，SHA-256 `481F0B8DAA4F648BA1D7B48AD40ED2F5CC4C5F9851342AB0A294FE0AB400ADE1`；
- Editor final：0 actions / 1.05s / native 0，SHA-256 `10557853E3E444FF310E51EEC18B907B4C6C9D6DA3064992C3384AB6B4B8DD25`。

产物：

- `demo_map.exe`：356,777,472 bytes，SHA-256 `8B11F82F7BDBBD400F83F708116C5282FE4A515CDA9A391E0F2952DCEDCF56BC`；
- `UnrealEditor-demo_map.dll`：15,439,360 bytes，SHA-256 `55346A63C5B8815271029525097D2F0BFB89FDFCC12E0AD5AC2677959925C280`。

## 10. P/F 与提交边界

本轮只完成 P 阶段 C++ Router、无头自动化、静态边界审计和 Development 构建。没有运行 Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。

计划精确提交 3 个新增源码、2 个回归工具文件、本 Report 与本 Development Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 和用户文件不修改、不暂存；raw logs 仅保留在 `Saved/Codex/P19.4`。

P19.5 建议建立 Run-scoped Divine Sense Product Session，把 Host/Router lifecycle 绑定到一个 prepared Combat Run，并增加只读 availability projection；仍不接物理输入、UI/表现或隐式 Actor discovery。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-4-divine-sense-command-router>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-4-divine-sense-command-router/Docs/Report/Dev.D.UE.0.0.10.P19.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-4-divine-sense-command-router/Docs/Log/Dev.D.UE.0.0.10.P19.4.r0_log.md>
