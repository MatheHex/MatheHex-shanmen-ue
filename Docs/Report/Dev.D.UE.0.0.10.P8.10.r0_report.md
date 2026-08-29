# Dev.D.UE.0.0.10.P8.10.r0 Report

## 1. 结论

P8.10 已把 Formation coverage authority 接入现有 `Fdemo_mapShanmenFormationProductHost`，结论为 **PASS**。

Host 现在持有唯一 `Fdemo_mapShanmenFormationCoverageTracker`，并暴露显式 `Prime`／`Advance`／`Rebase` 编排入口与 `Reset`。它不接受 caller 提供的 Area，而是从自身 durable anchor audits 重建 placement identity、读取自身 WorldAdapter receipts，再调用 P8.5 建立 Area，避免同 Run／Deployment 下的伪造 Area 绕过 host 权威。

最终 focused `6/6`、0.0.10 full `249/249`、changed-file regression gate、mapping self-test、Editor Development 与 Game Development 全部通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 唯一 owner 与公开命令

没有新建第二个 Formation host。现有 product host 新增：

- host-owned `CoverageTracker`；
- `TryCoordinateCoverage(...)`：显式接收 World、entity registry、Actor 子集、Run correlation 与 P8.9 command；
- `TryResetCoverage(...)`：显式清除 baseline；
- 只读 `HasCoverageBaseline()` 与 value-copy `TryGetCoverageBaseline(...)`。

World、registry、Actor subset 与 cadence 仍由 caller 拥有。Host 不缓存这些对象，不自动 Tick，不发现 Actor，也不施加 damage、Buff 或 effect。

## 3. 生命周期与 World 围栏

Coverage coordination 只有在以下条件全部满足时才运行：

1. Host、session、WorldAdapter 与 tracker 内部一致；
2. correlation 与 host 的完整 Run correlation 精确相等；
3. session 已进入 `Active` 且未终止；
4. 没有 committed placement 等待 World delivery；
5. placement receipt 数量与 durable anchor audit 数量完全相等；
6. 请求 World 与 WorldAdapter 已绑定 World 精确相同。

WorldAdapter 只新增布尔 `IsBoundToWorld` 查询，没有暴露内部 Actor、World weak pointer 或 receipt 集合。

## 4. Host 内部 Area 重建

每次命令都遍历 session 自身 anchor audits，通过既有 `BuildPlacementIntent` 重算 canonical placement ID，再从 host-owned WorldAdapter 按 ID 取得 receipt。Receipt 的完整 intent 必须与重算结果一致，否则以 `PlacementIncomplete` 失败关闭。

只有这些 receipts 可以进入 P8.5 `BuildArea`。Area 的 RunId、OwnerId 与 DeploymentId 随后再次与 host scope 比对。Caller 因此只能提供“采样谁”和“执行哪个命令”，不能注入另一个几何权威。

## 5. Reset、终止与恢复语义

显式 Reset 在非终止 session 中可调用：第一次清除 baseline 返回 `Reset`，无 baseline 的精确重放返回 `ResetReplayed`。

成功的 Cancel/End terminal teardown 会自动清除 coverage baseline。若 terminal teardown 失败，baseline 保留，使同一 terminal path 能继续 forward-only recovery；只有 teardown 成功后 host 才要求 tracker 已清空。终止后新的 coverage 或手工 reset 均返回 `SessionTerminal`，teardown receipt 仍可稳定重放。

## 6. 自动化证据

两份 Automation 日志均为一次命令、一次 queue completion、Fail `0`、fatal／unhandled `0`、进程原生退出码 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationProductHost` / `FormationProductHost.log` | 6 | 0 | `E5BD630D611E846A58F7B274A73D15ACE7ED6CDDE98E0A2416F73E932E0D8670` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 249 | 0 | `7A39FDB0B566C4DB62B7C1DAC153BE9FBF2640E4D16625D129D791ACC73C54FF` |

新增两项产品测试：

1. 四 anchor square deployment 的 host Prime、live movement Advance、Leave fact、exact Advance replay、Reset 与 Reset replay；
2. pre-active、foreign correlation、wrong World 围栏，以及 End teardown 自动清理、terminal rejection 与 teardown replay。

原有四项 Host durable commit、placement recovery、End teardown 与 Cancel boundary 测试继续通过。完整 suite 从 `247` 增至 `249`。

## 7. 改动—回归与静态门禁

`FormationProductHost` mapping rule 现在要求十二组证据：ProductHost、Coordinator、Tracker、Transitions、WorldCoverage、AreaProvider、WorldDelivery、Session、MaterialAdapter、Items、WorldGameplay 与 FormationDeployment。

```text
REGRESSION_MAP_JSON: PASS Rules=45
SELF_TEST: PASS 51/51
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=12 Logs=2
```

- map SHA-256：`AA43336EE01C2B06231CCB49D874D9F242A20C9B2744EE4CCFECA8C5383AE821`；
- self-test SHA-256：`6C1C4E5C0770B79F6F1E0A9F4504A5EBB015B6AE7AB5884A674BCB458CB30B52`；
- production boundary scan 对 Tick、timer、Actor discovery、SpawnActor、damage/effect、AbilitySystem、RNG、async 与 persistence API 的匹配为 `0`；
- `git diff --check`：PASS。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Native exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source build | Succeeded / 17 actions | 0 | 39.92s | `5218AF7594394E2AC5BDB321F7EBFCE881F9F4322B3EA0FBECC51EFEE063D3EA` |
| Editor final check | Succeeded / up to date | 0 | 0.90s | `F278F96A1E0A410DA3ED3CC556F7138CDE3D091E3ACEEF73D88D64051FD5532C` |
| Game final | Succeeded / 16 actions | 0 | 43.92s | `C53100E25DA7E22CAAF3AF8987EC55DAE38A15E90E2E74D418F74FA28EA23AF3` |

- Editor module：`11225088` bytes，UTC `2026-08-29T20:07:26.7109004Z`，SHA-256 `3C1F977BE63E0472D453B2C3E3BA6F332E0D74124041D66B4501662D490471E7`；
- Game executable：`352410112` bytes，UTC `2026-08-29T20:10:48.4260482Z`，SHA-256 `B657C353A188B230E10BD8CBDEEBB342167D8DB1DD3516482EFF55F6B4D4354A`。

## 9. 修改范围与 P/F 边界

更新：

- `demo_mapShanmenFormationProductHost.h/.cpp`；
- `demo_mapShanmenFormationProductHostTests.cpp`；
- `demo_mapShanmenFormationWorldAdapter.h`；
- `Scripts/ShanmenRegressionMap.json`；
- 本 Report 与同名 Development Log。

没有修改 GameplayTags、Content、schema、Build.cs、GameMode、输入、UI、物品权威或旧产品链。长期未跟踪用户与 0.0.9B 文件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。Automation 只创建无 Tick transient test World；未执行真实输入、截图、Smoke、大规模产品回归、Cook 或 Package。

## 10. 下一步与 GitHub

P8.11 建议新增纯值 `FormationInfluenceIntentPlanner`：消费 P8.7 transition receipt 与显式 authored policy，生成 deterministic apply/remove effect intents；先冻结身份、重放与撤销语义，不直接接 GAS、伤害数值或自动 cadence。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-10-formation-host-coverage/Docs/Report/Dev.D.UE.0.0.10.P8.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-10-formation-host-coverage/Docs/Log/Dev.D.UE.0.0.10.P8.10.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-10-formation-host-coverage>
