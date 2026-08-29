# Dev.D.UE.0.0.10.P8.10.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.10.r0`；
- 基线：`82aa47863ef2755e8497698c9ba1cbabe1fc5e19`（P8.9）；
- 分支：`agent/0.0.10-p8-10-formation-host-coverage`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.10 扩展既有 Formation product host，让它在 active deployment 生命周期内拥有唯一 coverage tracker，并把 P8.5 Area、P8.6 World sample、P8.7 transition、P8.8 tracker 与 P8.9 coordinator 接成显式产品路径。

明确后置：正式 cadence、Actor subset policy、effect intent policy、GAS 编排、damage／Buff、AI、输入/UI 与正式阵法 content／数值。

## 设计决策

### 不建立第二 host

Tracker 直接成为 `Fdemo_mapShanmenFormationProductHost` 的值成员。Host 不保存 World、registry 或 Actor subset；每个 coverage 命令仍由 caller 同步驱动。

### 不信任 caller Area

公开 API 不接收 Area。Host 用 durable audits 重算 placement intents，从自己的 WorldAdapter 按 canonical placement ID 读取 receipts，再调用 AreaProvider。每份 receipt intent 都与重算结果逐字段比较。

### 失败恢复保留 baseline

Terminal session transition成功但 World teardown 失败时，不清 baseline。相同 terminal call 可继续恢复；World teardown 成功才同时清 pending placement 与 coverage tracker。

### 下层证据完整返回

Host coverage result 保留 Area build、Coordinator result 与 Reset tracker result；顶层 status 区分 host、correlation、session、placement、World、Area 与 coordinator failure。

## 实现范围

更新生产代码：

- `Source/demo_map/demo_mapShanmenFormationProductHost.h`；
- `Source/demo_map/demo_mapShanmenFormationProductHost.cpp`；
- `Source/demo_map/demo_mapShanmenFormationWorldAdapter.h`。

更新测试与流程：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`。

新增本 Report 与同名 Development Log。没有新增模块、Build.cs、GameplayTags、Content 或 schema。

## 执行记录

1. 审查既有 Host 三段式 durable commit→World placement 与 terminal recovery 状态机。
2. 为 WorldAdapter 增加只返回布尔值的 exact World binding 查询。
3. 在 Host 内加入唯一 tracker 与 Prime／Advance／Rebase／Reset 产品 API。
4. 实现 active、pending placement、receipt completeness、exact World 与完整 Run correlation 围栏。
5. 实现 audit→canonical intent→owned receipt→Area 的内部重建，不接受外部 Area。
6. 将 successful Cancel/End teardown 与 tracker reset 绑定，同时保留 teardown failure 的 baseline。
7. 新增四-anchor transient World fixture 与两项 host ownership/lifecycle tests。
8. Editor source build 首次成功；focused `6/6`、full `249/249` 首次成功。
9. regression map、`51/51` self-test、changed-file gate、静态门禁与最终 Editor/Game 均首次通过。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `FormationProductHost.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 6 | 0 | 0 | 1 | `E5BD630D611E846A58F7B274A73D15ACE7ED6CDDE98E0A2416F73E932E0D8670` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 249 | 0 | 0 | 1 | `7A39FDB0B566C4DB62B7C1DAC153BE9FBF2640E4D16625D129D791ACC73C54FF` |

两份日志均 fatal／unhandled `0`。引擎启动时 UnifiedError self-test 的固定 `Condition failed` 与此前日志一致，不属于项目 Automation case；项目结果按 `Test Completed Result`、queue completion 与 fatal／unhandled 判定。

## 新增测试内容

### CoverageOwnership

- 四个 committed anchor 全部进入同一 World 后才允许 Prime；
- Area 从 host receipts 内部重建并封印当前 deployment ID；
- Actor 从 inside 移动到 outside，Advance 产生一个 `Left` fact；
- 最近一次 exact Advance 返回 `AdvanceReplayed`；
- Reset 与无 baseline Reset replay 都保持 host valid。

### CoverageLifecycleFences

- Deploying session 返回 `SessionNotActive`；
- foreign correlation 与 null/wrong World 分别失败关闭；
- Prime 后 End teardown 自动清 baseline；
- terminal coverage/reset 拒绝，End teardown receipt 仍稳定重放。

## Changed-file regression gate

本轮 Host rule 扩充为十二组依赖证据；WorldAdapter header 同时命中既有 WorldDelivery rule。父级 full suite 覆盖全部 required groups，focused suite额外提供 Host 精确证据。

```text
REGRESSION_MAP_JSON: PASS Rules=45
SELF_TEST: PASS 51/51
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=12 Logs=2
```

- map SHA-256：`AA43336EE01C2B06231CCB49D874D9F242A20C9B2744EE4CCFECA8C5383AE821`；
- self-test SHA-256：`6C1C4E5C0770B79F6F1E0A9F4504A5EBB015B6AE7AB5884A674BCB458CB30B52`。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source build | Succeeded / 17 actions | 0 | 39.92s | `5218AF7594394E2AC5BDB321F7EBFCE881F9F4322B3EA0FBECC51EFEE063D3EA` |
| Editor final check | Succeeded / up to date | 0 | 0.90s | `F278F96A1E0A410DA3ED3CC556F7138CDE3D091E3ACEEF73D88D64051FD5532C` |
| Game final | Succeeded / 16 actions | 0 | 43.92s | `C53100E25DA7E22CAAF3AF8987EC55DAE38A15E90E2E74D418F74FA28EA23AF3` |

- Editor module：`11225088` bytes，UTC `2026-08-29T20:07:26.7109004Z`，SHA-256 `3C1F977BE63E0472D453B2C3E3BA6F332E0D74124041D66B4501662D490471E7`；
- Game executable：`352410112` bytes，UTC `2026-08-29T20:10:48.4260482Z`，SHA-256 `B657C353A188B230E10BD8CBDEEBB342167D8DB1DD3516482EFF55F6B4D4354A`。

## 静态、兼容性与 P/F 边界

- map JSON、51/51 mapping self-test、changed-file gate 与 `git diff --check`：PASS；
- production boundary scan 对 Tick/timer、Actor discovery、SpawnActor、damage/effect、AbilitySystem、RNG、async 与 persistence API：0 matches；
- 没有增加第二套 host、World、Area、registry、effect 或 inventory authority；
- 长期未跟踪用户和 0.0.9B 文件未修改、未 stage；
- Automation 只使用无 Tick transient World，没有启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.11 建议实现纯值 Formation influence intent planner，把 immutable coverage transition 映射成 deterministic apply/remove intents；继续把效果应用与数值留在后续编排层。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-10-formation-host-coverage/Docs/Report/Dev.D.UE.0.0.10.P8.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-10-formation-host-coverage/Docs/Log/Dev.D.UE.0.0.10.P8.10.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-10-formation-host-coverage>
