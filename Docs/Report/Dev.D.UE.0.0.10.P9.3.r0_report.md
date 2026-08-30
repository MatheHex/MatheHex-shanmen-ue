# Dev.D.UE.0.0.10.P9.3.r0 Report

## 1. 结论

P9.3 **PASS**。本阶段建立了一个与产品数值解耦的 typed action-resource authority：动作进入 Startup 后先冻结并预留指定资源，精确跨越 Startup→Active commit point 时才扣除；若动作在 commit 前 Cancelled 或 Interrupted，则释放预留而不消费。所有命令、快照与 receipt 均具备确定性身份、精确重放和冲突拒绝语义。

最终验证为 ActionResource `7/7`、ActionLifecycle `1/1`、CombatRuntime `58/58`、`Shanmen.0_0_10` 全量 `361/361`，四份日志合计 `427` 条 success、`0` fail。changed-file gate、mapping self-test、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

本阶段没有虚构 SpiritShield 的正式资源消耗数值，也没有把临时 float 接到玩家属性。`Shanmen.Resource.SpiritEnergy` 只定义 typed channel；最终数值、恢复规则、持久化与产品 owner 仍由后续产品接线决定。

## 2. 功能性

### 2.1 Typed cost 与资源快照

`FShanmenActionResourceCost` 从可编辑 capture 输入冻结：

- 非空规则 ID；
- 有效 GameplayTag 资源通道；
- 有限且严格大于零的消耗量；
- 由规则、通道和 float bit pattern 派生的确定性 CostId。

`FShanmenActionResourceAuthority` 每个实例只拥有一个 entity 和一个 resource channel，并维护 current、maximum、reserved、available 与单调 revision。调用方只能从有效 authority 捕获不可变 snapshot，不能用任意旧余额绕过 revision。

### 2.2 Startup 预留

reservation request 必须同时绑定：

- 有效 action snapshot；
- 同一 activation 的首个 `Idle→Startup` receipt；
- typed cost；
- 同 owner、同 channel 的 authority snapshot。

预留成功只增加 reserved，不减少 current；因此 competing action 看到的 available 已降低，但实际消费尚未发生。余额不足、foreign owner、foreign channel、stale snapshot 与 revision overflow 均原子拒绝，不留下半写状态。

### 2.3 Commit / Release

finalization request 只接受两类 action evidence：

- 精确 `Startup→Active` 且首次跨越 commit point：`Commit`；
- commit 前从 Startup 进入 Cancelled 或 Interrupted：`Release`。

Commit 同时减少 current 与 reserved，available 保持稳定；Release 只减少 reserved，available 恢复。authority 先在候选副本上完成写入和全状态校验，校验成功后才替换正式状态，拒绝路径不会产生部分提交。

### 2.4 幂等与冲突

- exact reservation command replay 返回原 reservation receipt，不重复占用；
- 同 action/channel 试图改写 cost 返回 `ReservationConflict`；
- exact finalization replay 返回原 finalization receipt，不重复扣除；
- commit 后试图把同一 reservation 改写为 release 返回 `FinalizationConflict`；
- 等价冻结输入在独立 authority 中复现 cost、snapshot、reservation、command 与 receipt identities。

## 3. 完整性与兼容性

- 新 authority 复用现有 `FShanmenCombatActionSnapshot`、orchestrator transition receipt 与 commit-point 语义，没有建立第二套动作状态机；
- 新增 native tags `Shanmen.Resource` 与 `Shanmen.Resource.SpiritEnergy`，未改变既有 action tags；
- authority 是普通 C++ 单写者值对象，跨层 proof 为 read-only USTRUCT；
- 未依赖 `demo_map`、World、Actor、Timer、Tick、输入、UI 或随机数；
- 未修改 P9.0 SpiritShield runtime、P9.1 capacity authority 或 P9.2 deadline gate；
- 没有声称完成玩家可见 SpiritShield 激活、资源恢复或存档接线。

## 4. 关键不变量

1. 一个 authority 只服务一个 owner 和一个精确 resource channel；
2. `0 <= reserved <= current <= maximum`；
3. available 始终等于 `current - reserved`；
4. pending reservation 总额必须与 authority reserved 精确对应；
5. reservation 只能由同 activation 的首个 Startup receipt 创建；
6. commit 只能由 action 的真实 commit transition 触发；
7. pre-commit terminal transition 只能 release，不能消费；
8. stale、foreign、conflicting 或 malformed proof 不得写状态；
9. exact replay 返回原 proof，不推进 revision；
10. 所有正式变更先在候选副本验证，再原子替换 authority。

## 5. 测试覆盖

新增 7 个 focused tests：

- `ContractAndSnapshot`：typed tag、cost identity、零成本拒绝与 snapshot identity；
- `ReserveAndCommit`：Startup 预留与 commit point 单次消费；
- `CancelReleases`：commit 前取消释放且不消费；
- `ReplayAndConflict`：reservation/finalization exact replay 与改写冲突；
- `FailClosedProofs`：余额不足、stale snapshot、foreign owner/channel 原子拒绝；
- `MultipleReservations`：同一 owner 多动作预留、分别 commit/release 后无余额漂移；
- `DeterministicReplay`：等价冻结输入复现全部事务 identity。

既有 `ActionLifecycle` 独立运行 `1/1`，验证资源事务所依赖的动作 commit contract 未被破坏；CombatRuntime 与 0.0.10 全量分别为 `58/58`、`361/361`。

## 6. 修改范围

生产代码：

- `Source/ShanmenCombatRuntime/Public/ShanmenActionResourceAuthority.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenActionResourceAuthority.cpp`；
- `Source/ShanmenCombatRuntime/Public/ShanmenCombatRuntimeTags.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenCombatRuntimeTags.cpp`。

测试与流程：

- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenActionResourceAuthorityTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

代码与脚本为 `7` 个文件、`1839` insertions、`0` deletions；加入本 Report 与同名 Development Log 后 exact stage 为 `9` 个文件。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 7. Automation 与 changed-file 证据

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `ActionResource-final.log` | `Shanmen.0_0_10.CombatRuntime.ActionResource` | 7 | 0 | `05B0FE83D33FEA66EAA9112D86AAA718B02B850FD4566A11BE19D4DFDAC466C0` |
| `ActionLifecycle-final.log` | `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `2615A0E13FFEE1DB453D9BE3456D391795EFDAC375ADEB09065CE9FE4A09DBA1` |
| `CombatRuntime-final.log` | `Shanmen.0_0_10.CombatRuntime` | 58 | 0 | `9AA33C4935B78127B4971E3C7D8E2EB7B702E8B5363FD7A7FF8126FD449E9CA0` |
| `Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 361 | 0 | `857A2FA57DEB67BF8E9614E3CB64E12130E24B02D2881E8157608D528B30EC46` |

每份日志均只有一个目标 RunTests 命令、一个正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0`，命令进程原生退出码均为 `0`。测试发现前仍保留 UE 5.8 既有的 13 条 `Condition failed` 启动诊断；它们不属于目标用例，目标用例随后逐项成功。

```text
REGRESSION_MAP_JSON: PASS Rules=73
SELF_TEST: PASS 108/108
REGRESSION_COVERAGE (implementation): PASS Changed=7 Rules=2 Required=3 Logs=4
Required: CombatRuntime.ActionResource, CombatRuntime.ActionLifecycle,
          CombatRuntime
git diff --check: PASS
BOUNDARY_SCAN_MATCHES=0
```

- mapping SHA-256：`9B476EC30AA3DCA790CF5EFF19C110A174738E0E92F5D2281B8D9F48EA8B74DF`；
- self-test SHA-256：`B1F4E4981C9218218E652EF3DEDD89D1C73BC7B547B767C74B09F1ABE99E8E1E`。

## 8. 构建证据

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor candidate | Succeeded | 11 / 52.30s | 0 | `FD53A32A99E43E88C5442FA5A86F00783BB0AA102BD010766D2AC20339367FDF` |
| Editor final | Succeeded, target up to date | 0 / 0.90s | 0 | `3D546769EBFFC153C2FD598F47FE2F0FAAE699F97D6644061AAC37AF8BCDC624` |
| Game final | Succeeded | 10 / 40.45s | 0 | `1CD0EE4F4EBCD5A3842E8F4E9E5F94C84E7616CF2384B1BD471322CB60A145A4` |

- `UnrealEditor-ShanmenCombatRuntime.dll`：`992768` bytes，SHA-256 `D54258082EC9F26C0B61351B966B8DFDEA69608FB416B2A8083CF5C1CC39EB70`；
- `demo_map.exe`：`353411072` bytes，SHA-256 `F90E47B1D05CFD03D0700CCA7381886D5A8DA5EEB9FC83C008010D84F7963869`。

## 9. 真实异常

本阶段没有源码、构建或目标测试失败。Automation 启动仍报告非目标平台 LinuxArm64/VisionOS 缺少 `MainVersion` metadata，Win64 SDK 为 VALID；这不影响本轮 Win64 目标。未发生 C3859、C1076、系统代码 1455、UBT 非零退出或外层超时。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段纯值开发、代码审查、无头 Automation、静态扫描、changed-file gate、Editor/Game Development 构建与 Git 证据。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P9.4 建立一个原子 SpiritShield activation composition：同一产品 host 依次创建 action Startup、预留 typed spirit energy、跨越 commit point 时完成资源消费并激活 P9.0 shield；任一步失败都按 receipt 释放或拒绝，随后再接 P9.2 external deadline 与 P9.1 capacity projection。正式 cost 数值必须来自产品内容，不在 runtime contract 中硬编码。
