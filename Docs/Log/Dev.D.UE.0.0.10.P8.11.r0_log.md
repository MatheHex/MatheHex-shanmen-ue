# Dev.D.UE.0.0.10.P8.11.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.11.r0`；
- 基线：`6566598478e6c60cb8ce104d052c11acb2a5e7df`（P8.10）；
- 分支：`agent/0.0.10-p8-11-formation-influence-intents`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.11 把 immutable formation coverage transition 映射成 deterministic influence apply/remove intents，冻结身份、cause、顺序、重放、no-op 与 mutation seal。

明确后置：Prime baseline、Rebase、Reset/terminal reconciliation，dispatch/ack ledger，正式 cadence、Actor subset policy、GAS 编排、damage／Buff、AI、输入/UI 与正式阵法 content／数值。

## 设计决策

### envelope 与证据 batch 分离

通用 intent envelope 只表达稳定 scope、policy/influence identity、Apply/Remove、cause 与 content。P8.11 transition batch 内嵌 Area 和完整 transition receipt，并要求 cause 精确引用 fact。未来 lifecycle planner 可以复用 envelope，但不能复用或伪造 transition batch。

### 不把数值塞入意图

Policy 只有 authored definition IDs 与 content stamp。Magnitude、duration、stacking、GameplayEffect class 和 executor 全部留给后续 effect authority，避免纯值 coverage 层演化成第二套结算系统。

### no-op 也是证据

只含 StayedCovered／RemainedOutside 的 transition 返回 sealed no-op batch，而不是无返回。它保留 exact transition receipt 和 deterministic batch ID，可用于审计和重放判定。

## 实现范围

新增生产代码：

- `Source/demo_map/demo_mapShanmenFormationInfluenceIntentPlanner.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceIntentPlanner.cpp`。

新增测试并更新流程：

- `Source/demo_map/demo_mapShanmenFormationInfluenceIntentPlannerTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

没有修改现有 Formation Host、World、Area、tracker、coordinator、GameplayTags、Content、schema 或 Build.cs。

## 执行记录

1. 复审 P8.5–P8.10 的 Area、coverage transition、tracker、coordinator 与 host identity contracts。
2. 定义 authored influence policy、通用 immutable operation envelope 与 self-contained transition batch。
3. 实现 Entered→Apply、Left→Remove、unchanged→no-op 的纯函数 planner。
4. 使用 canonical namespaces 封印 intent/batch identity、顺序、计数、cause 与 nested evidence。
5. 新增 input fences、exact replay、no-op 与 mutation tests。
6. 新增 regression mapping rule，并补 broad-pass 与 narrow-evidence fail-closed self-tests。
7. mapping self-test 从 `51/51` 增至 `53/53`。
8. Editor source build 首次成功；focused `4/4`、full `253/253` 首次成功。
9. changed-file gate、静态门禁与最终 Editor/Game 均首次通过。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `FormationInfluenceIntents.log` | `Shanmen.0_0_10.Product.FormationInfluenceIntents` | 4 | 0 | 0 | 1 | `3B44AEBCC2A3E2AE3712ABEFD0F703E3B26F5577AD9B7CB811894D0AF4386D9C` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 253 | 0 | 0 | 1 | `517D4842EC47E297E4CB2E050C78C712341075B185E433E1E883EEFB7C69F550` |

两份日志均 fatal／unhandled `0`。引擎启动时 UnifiedError self-test 的固定 `Condition failed` 与此前日志一致，不属于项目 Automation case；项目结果按 `Test Completed Result`、queue completion 与 fatal／unhandled 判定。

## 新增测试内容

### DeltaPlan

- 四 subject 混合 Entered、Left、StayedCovered 与 RemainedOutside；
- 只发布两个 intent，canonical 顺序与 transition facts 一致；
- operation 与 exact fact cause 对应。

### CanonicalReplayAndNoOp

- exact replay 保持 batch identity；
- influence definition 参与 identity；
- unchanged transition 形成 valid no-op batch。

### InputFences

- invalid Area、missing source、malformed policy、invalid transition；
- foreign Area 与 content mismatch 分别失败关闭；
- rejection 不携带 valid batch。

### MutationSeal

- count、operation、cause、order、policy、nested fact、batch ID 与 standalone intent identity 漂移全部被拒绝；
- 原始 batch 保持 valid。

## Changed-file regression gate

新增 rule 要求八组依赖证据；focused suite 提供 intent 精确证据，full suite 覆盖全部上游 contracts。Self-test 新增一个 broad pass 和一个 narrow-evidence rejection。

```text
REGRESSION_MAP_JSON: PASS Rules=46
SELF_TEST: PASS 53/53
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=8 Logs=2
```

- map SHA-256：`DD7AE44544B1EAE717B960C2B145A1914C780832ABE1E98BA4AE323FB93526EA`；
- self-test SHA-256：`A75BB903625DCCD0DE2EDB725F6C5DDC59ACFFEB7F944827ACB9E09C4A978551`。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source build | Succeeded / 5 actions | 0 | 9.67s | `2AEC702F5670C163334E3348AC10A27EAF16D860E4C1F9684A884B70DFB88E4E` |
| Editor final check | Succeeded / up to date | 0 | 0.91s | `62C61560BA67F3097F835E550E9C616C31E17CBEC247387943CB61F02465D345` |
| Game final | Succeeded / 4 actions | 0 | 20.72s | `A666F01B71143FA298383402D4533DDC1F215843D3D677B81EFC71F6154B90DD` |

- Editor module：`11267584` bytes，UTC `2026-08-29T20:37:42.4502417Z`，SHA-256 `889CFE50AF8A067DBAF68178BAB42CF7B3E8955C2BCEDEAC7ED8B22CA2614834`；
- Game executable：`352442368` bytes，UTC `2026-08-29T20:40:18.2444790Z`，SHA-256 `6B54735D60DEB379917A0BEE77270AE77DE2A10577C7A0CA4BEF49801A5CFAB0`。

## 静态、兼容性与 P/F 边界

- map JSON、53/53 mapping self-test、changed-file gate 与 `git diff --check`：PASS；
- production boundary scan 对 World/Actor/object、Tick/timer、Actor discovery、SpawnActor、damage/effect、AbilitySystem、RNG、async 与 persistence API：0 matches；
- 没有增加第二套 host、World、Area、tracker、effect executor、inventory 或 persistence authority；
- 长期未跟踪用户和 0.0.9B 文件未修改、未 stage；
- Automation 只使用纯值 fixtures 与既有无 Tick test Area receipts，没有启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.12 建议建立 Prime/Rebase/Reset/terminal influence reconciliation batches，补齐初始 Apply 与生命周期 Remove 的显式撤销语义；完整生命周期冻结后，再接 host-owned dispatch ledger。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-11-formation-influence-intents/Docs/Report/Dev.D.UE.0.0.10.P8.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-11-formation-influence-intents/Docs/Log/Dev.D.UE.0.0.10.P8.11.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-11-formation-influence-intents>
