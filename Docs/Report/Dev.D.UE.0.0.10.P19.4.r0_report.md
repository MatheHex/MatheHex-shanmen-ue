# Dev.D.UE.0.0.10.P19.4.r0 Report

## 1. 结论

P19.4 在 P 阶段边界内完成，结论为 **PASS**。

本轮在 P19.3 Run-scoped Divine Sense Product Host 前新增 `Fdemo_mapShanmenDivineSenseCommandRouter`。Router 把上层已准备的动作快照、神识定义、SpiritEnergy 费用、扫描序号、显式 Actor 预算和显式 subject Actor 集合冻结为一个 pointer-free 乐观命令；首次执行在 Host 与 Router 私有副本上整体提交，失败不发布任何状态，精确重放不读取 World、Source Actor、subject Actor 或 evidence provider。

```text
P19.4 exact:                         4 Success / 0 Fail
P19.3 Product Host:                  4 Success / 0 Fail
P19.2 Pulse Coordinator:             4 Success / 0 Fail
P19.1 World Observation:             4 Success / 0 Fail
P19.0 Divine Sense runtime:          4 Success / 0 Fail
ActionResource:                      7 Success / 0 Fail
ActionLifecycle:                     1 Success / 0 Fail
WorldGameplay:                      10 Success / 0 Fail
Shanmen.0_0_10 full:               808 Success / 0 Fail
Regression self-test:              297 / 297 PASS
Regression coverage:                PASS, Changed=5 Required=8 Logs=8
Boundary scan:                      PASS, 2 files / 0 matches
Game + Editor Development:          PASS / native status 0
```

## 2. 路由边界

P19.3 已经保证单次脉冲内部的 Action、SpiritEnergy 与 World observation 原子性，但调用方仍需自行完成以下工作：

- 从上层输入组合 P19.3 pulse command；
- 证明本次调用仍指向同一个 Run、Host 与 source entity；
- 证明实际 Actor 集合没有从命令捕获到执行之间发生替换；
- 避免两个同时从同一资源状态捕获的命令在状态变化后继续执行；
- 为已接受命令提供不依赖 live UObject 的路由级重放。

P19.4 只收束这些编排职责，不新增第二份资源、动作、扫描、目标或内容权威。

## 3. 不可变 Route Command

`Fdemo_mapShanmenDivineSenseRouteCommand` 冻结：

- `ExpectedRouterId`；
- `ExpectedHostId`；
- 捕获时的 `ExpectedResourceSnapshotId`；
- 完整 P19.3 `Fdemo_mapShanmenDivineSensePulseCommand`；
- 从显式 Actor 集合解析出的、严格升序且无重复的 subject entity IDs。

`RouteCommandId` 从以上完整内容确定性派生。Route command 不保存 `UWorld*`、`AActor*` 或 provider 指针。捕获时只读取调用方显式提供的 Actor，并通过同 Run 的 `FShanmenWorldEntityRegistry` 转换为稳定身份；不进行 Actor 枚举或空间发现。

同一动作、定义、费用、序号、预算和等价 Actor 集合在同一资源投影上产生相同命令。Actor 输入顺序不会改变命令身份；subject 数量超过冻结预算、无注册绑定、重复身份、跨 Run 或跨 source 的输入均失败关闭。

## 4. Router 身份与单一状态链

Router 只能附着到尚未处理任何 pulse 的有效 P19.3 Host。它冻结 HostId、RunId、source entity、opening resource snapshot 和 Host 声明的 processed capacity，并确定性派生 RouterId。

Router ledger 与 Host ledger 保持一一对应：

- `NumProcessedCommands == Host.NumProcessedPulses`；
- Router 当前资源快照必须等于 Host 当前快照；
- 第一个命令的 expected snapshot 必须等于 opening snapshot；
- 每个已接受命令的 `ResourceAfter` 必须成为下一个命令的 expected snapshot；
- 每个命令只推进一次 Host 资源 revision（reserve + commit，共 `+2`）；
- 已记录结果必须是首次 `Applied` 证明，不用 replay 结果覆盖原始证据。

Host 若在 Router 外被直接推进，下一次捕获或路由会以 `StateDesynchronized` 失败，而不是尝试猜测或合并状态。

## 5. 首次执行与原子发布

新命令的路由顺序固定为：

1. 验证 Router、Host、命令及其 Run/source/Host 身份；
2. 验证 Router 与 Host 当前 ledger、capacity 和资源快照一致；
3. 检查 processed capacity 与乐观资源投影；
4. 验证 active registry，并精确解析 source Actor；
5. 重新解析本次显式 subject Actors，规范排序后与冻结 entity IDs 逐项比较；
6. 在 Host 私有副本上调用 P19.3；
7. 只有 Host proof、Router result、快照链和 replay record 全部有效时，才同时发布 Host 与 Router 副本。

Actor 列表次序可不同，但身份集合必须完全相同；新增、缺失、替换、重复或未注册 Actor 都在 provider 调用前被拒绝。

## 6. 重放、冲突与恢复

Router 先检查已接受的 `RouteCommandId`，再检查 live World/Actor 输入。精确重放因此可以传入空 World、空 source 和空 subject 集合；Router 只要求 P19.3 Host 返回同一 receipt 的 `AlreadyApplied` 证明。该路径不调用 evidence provider、不再次扣费、不新增 Router/Host ledger 项，也不改变当前资源快照。

同一 `ActivationId` 若带不同 pulse payload 或 subject set，会以 `ActivationConflict` 失败关闭。两个命令若从同一资源快照捕获，先提交者成功，后提交者以 `ResourceProjectionStale` 被拒；调用方可从新的当前投影重新捕获。达到声明容量后，新命令在任何 live Actor/evidence 读取前被拒。

P19.3 的 World evidence 或资源操作若失败，Router 返回完整 `HostRejected` 证明，并保持 Host 与 Router 原状态。修正外部 evidence 后，同一冻结命令可以恢复并只提交一次。

## 7. 精确自动化

新增组 `Shanmen.0_0_10.Product.DivineSenseCommandRouter` 包含 4 项：

1. `CaptureAndApply`：无序 Actor 输入产生稳定命令，provider 以规范 entity 顺序调用，Host 与 Router 原子提交；
2. `ReplayWithoutLiveInputs`：空 World/Actor 精确重放，receipt 相同、0 evidence I/O、0 二次扣费；
3. `ProjectionCapacityAndConflict`：激活冲突、陈旧投影与容量门均在 live evidence 前失败关闭；
4. `RollbackAndIdentityFences`：Host rejection 可恢复，source/subject/registry/Host 身份围栏不污染已接受状态。

精确结果为 `4 Success / 0 Fail`，原生退出码 `0`，终止标记严格为 `1`，Fatal/Unhandled/Ensure 为 `0`。

## 8. 改动驱动回归证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `DivineSenseCommandRouter` | 4 | 0 | `AF47189D45FE755CB19E00DB4B4E9A6553DC977DA9ED9090B05E8E6BE51B705C` |
| `DivineSenseProductHost` | 4 | 0 | `912ACCF45536F95B4A7B6FC19D5006C4F38F45BAC045676A11B0B69C955A43D4` |
| `DivineSensePulseCoordinator` | 4 | 0 | `1DD6BF8B8AE84A9B6ACE762154A600C80C58292F9CA8C3D0B7378E0F26A3E006` |
| `DivineSenseWorldObservation` | 4 | 0 | `5C916B62CBD25B7CF6060E7568CEFB3E0179AB2F8B693CD11BF026D56AF98FEF` |
| `CombatRuntime.DivineSense` | 4 | 0 | `DEEDE701AB3CDEB343C8DF12F42EF5908C1A6E9B91AE924F894DE4ADE43CF83B` |
| `CombatRuntime.ActionResource` | 7 | 0 | `7617A5CB591992875F0044D463A62653DF7DAB0B5395102615905DE25154569B` |
| `CombatRuntime.ActionLifecycle` | 1 | 0 | `743FFDDC47F676E8F8A4F8A2CBD67223CB42FBE44C4C0F2EA338623E8DECA507` |
| `WorldGameplay` | 10 | 0 | `1CCE3C786F357C9486328E625941CA8CC0381613428EC3BC813314FE5DA995D7` |
| `Shanmen.0_0_10` full | 808 | 0 | `3830DDD6D6B24C47968C16A62E2B7BD366AB82788FF5F91B3C39BB8D3D82D9CD` |

`Scripts/ShanmenRegressionMap.json` 新增 Router 规则，要求上述 8 组证据。真实门禁输出：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=8 Logs=8
```

门禁日志 SHA-256 为 `74DD76E97C834E60BA8CD5FFFFCF5B4EBB8806F56809B4CF8FCA9AC1AAD3914E`；正向和缺失依赖反例已加入 self-test，结果 `297/297 PASS`，日志 SHA-256 为 `3689B996F3E85A306653DF6DEFE7DC3C2DCFB9C71DAF04574B7D2198C7B1710A`。

## 9. 构建、产物与 P/F 边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor initial | PASS | 5 actions / 17.47s | `9F3835A674CBC4D6A85B10F561878A3AF3DB90B0B1C1ADE8787926EAA60CE682` |
| Game final | PASS | 4 actions / 23.30s | `481F0B8DAA4F648BA1D7B48AD40ED2F5CC4C5F9851342AB0A294FE0AB400ADE1` |
| Editor final | PASS | 0 actions / 1.05s | `10557853E3E444FF310E51EEC18B907B4C6C9D6DA3064992C3384AB6B4B8DD25` |

- `Binaries/Win64/demo_map.exe`：356,777,472 bytes，SHA-256 `8B11F82F7BDBBD400F83F708116C5282FE4A515CDA9A391E0F2952DCEDCF56BC`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：15,439,360 bytes，SHA-256 `55346A63C5B8815271029525097D2F0BFB89FDFCC12E0AD5AC2677959925C280`；
- `git diff --check`：PASS；
- 生产 header/cpp 边界扫描：2 files / 0 matches，日志 SHA-256 `E43BC48213367C586F4DE252C7667F002C321B19C0112AC102373097150CBB45`。

生产 Router 未使用 Actor 全局枚举、trace/sweep/overlap、直接伤害、Tick/Timer、随机、Spawn、UI、声音或 Niagara。测试 fixture 只创建 transient、无 physics/audio/FX 的 GamePreview World 与 Actor。

本轮只完成 P 阶段 C++ 架构、无头自动化、静态检查和 Development 构建。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. 提交边界与后续

基线提交为 `f239727e7bd4644cd56dce2edfb4d7e7661da2ae`（P19.3）。本轮只提交 3 个 Router 生产/测试源码、2 个回归门禁文件、本 Report 与本 Development Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；raw logs 仅保留在 `Saved/Codex/P19.4`。

P19.5 建议建立 Run-scoped Divine Sense Product Session：把 Host + Router 的创建、持有和 teardown 绑定到一个已准备的 Combat Run 与显式 SpiritEnergy opening snapshot，并提供只读 availability projection；仍不接物理输入、隐式 Actor discovery、UI/表现，也不冻结最终数值或扫描节奏。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p19-4-divine-sense-command-router>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-4-divine-sense-command-router/Docs/Report/Dev.D.UE.0.0.10.P19.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p19-4-divine-sense-command-router/Docs/Log/Dev.D.UE.0.0.10.P19.4.r0_log.md>
