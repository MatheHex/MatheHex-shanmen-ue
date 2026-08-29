# Dev.D.UE.0.0.10.P8.2.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.2.r0`；
- 基线：`5094e757b2f3850a309dd1dbf5e139d16862313a`（P8.1）；
- 分支：`agent/0.0.10-p8-2-formation-product-session`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.0 已提供纯 deployment contract，P8.1 已提供 durable material adapter。P8.2 只闭合产品调用顺序与 replay/recovery ownership，防止调用方在 action、deployment 和 item authority 之间制造半完成状态。

明确后置：anchor Actor/world delivery、投料交互、区域/效果 provider、阵法战斗规则、UI、正式阵图商品、配方与数值。

## 契约设计

### 显式状态机

Session 状态为 `Empty / Deploying / Active / Cancelled / Ended`。启动必须在一个候选值中成功完成 action Startup/Active 和 deployment begin，才发布给调用方。

### 一个 pending attempt

Session 最多保存一个 `Fdemo_mapShanmenFormationMaterialResult` pending。它必须属于 frozen correlation、DeploymentId 和尚未 committed 的 anchor；AttemptId 不能与历史 audit 冲突。该纪律在产品边界消除了重叠 quantity intent。

### 双证据 audit

Material commit 后不能直接把阵眼视为完成。只有 P8.0 `TryCommitAnchor` 同时成功，才生成 `Fdemo_mapShanmenFormationAnchorAudit`。Audit 校验 material evidence 与 deployment progress 中 FulfillmentId、lines、revision、anchor、deployment 完全相同。

### 前向恢复

若 durable material terminal 已存在但 deployment commit 尚未发布，pending 可以保存 committed material。相同 attempt 重放时不会再调用 material commit，而是只补 deployment edge。

若进程内 pending 仍是 prepared、但 ledger 已在外部完成 commit，Session 会通过 P8.1 `PrepareMaterials`/`CommitMaterials` 的 deterministic replay 重建同一 receipts，再完成 deployment。取消在观察到 committed line 后明确返回 forward-only recovery。

## 实现范围

新增：

- `Source/demo_map/demo_mapShanmenFormationProductSession.h`；
- `Source/demo_map/demo_mapShanmenFormationProductSession.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductSessionTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

生产 Session 不引用 World/Actor、timer/Tick、RNG 或 legacy inventory。测试使用 isolated profile、真实 migration/cutover、真实 `Udemo_mapShanmenItemAuthoritySubsystem`、真实 active-Run correlation 和两个 SpiritWood stacks；没有创建 UWorld 或产品 Actor。

## 提交前审查修正

初版 `TryPrepareAnchor` 在查询历史 audit 前先要求 `Deploying`，因此 final anchor 进入 `Active` 或 Session 进入 `Ended` 后，prepare replay 无法返回既有 audit；commit replay 却可以，语义不一致。

最终把有效 Session、correlation、request 与历史 audit replay 放在新增阵眼状态门之前。`CommitReplayAndEnd` 增加 End 后 prepare replay 断言，随后重新构建并重跑全部自动化。该修正不改变 durable authority 或产品范围。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationSession.log` | `Shanmen.0_0_10.Product.FormationSession` | 4 | 0 | 0 | `508FFE36315F3C0CE49D5AAF18C9362E1F89F192858B86EBFEE4DBFC210F2BC2` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 219 | 0 | 0 | `F9D9E244ADBFC7497CE95D74AF1622B09E97DED06D9C2444AF6C7588E664D1E3` |

每份最终日志均为 one command、one queue-empty、Fail `0`、fatal/assert/ensure `0`。

## 测试内容

1. `PrepareAndSinglePendingFence`：foreign correlation、same prepare replay、second-anchor fence；
2. `CommitReplayAndEnd`：两阵眼顺序、exact receipt replay、attempt conflict、Active/End 与 terminal audit replay；
3. `CancelReleasesPending`：durable cancel、quantity 不变、deployment/action terminal 同步；
4. `ForwardCommitRecovery`：模拟 material 已 commit 而 deployment 尚未 commit，取消 forward-only、exact replay 完成。

## Changed-file regression gate

新增 FormationProductSession rule，要求 focused session、FormationMaterialAdapter、Items 与 FormationDeployment。full suite 可覆盖全部 parent groups；child-only fixture 必须失败，防止只跑新增四项就声称跨 authority 兼容。

```text
REGRESSION_MAP_JSON: PASS Rules=38
SELF_TEST: PASS 37/37
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=2
```

- coverage SHA-256：`42A2F7113E7736E0B0FA3D08CB21CEB5935418F17AFFC122A33B024515CCE814`；
- self-test SHA-256：`CAD60BE85AA404F5CCD317FF3F489CC67CD746685E53F82C3C7DEC3B9E9FBB65`。

## 构建与异常记录

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | Succeeded | 0 | 10.57s | 未保留为最终工件 |
| Editor final | Succeeded | 0 | 9.10s | `AC71CB6BA2C98C17624970B131AC63F90A1B356025EAD03341B77B8CC598CD4E` |
| Game final | Succeeded | 0 | 22.01s | `CBB54F063DF5CBF258F32AC200EAEBE36C5A65584226AFC83111C06C587F3818` |

首次源码、focused 与 full 验证均成功。没有源码、UHT、link、automation、commit-memory 或环境失败。提交前语义审查修正后，最终证据全部重跑。

## 静态、兼容性与 P/F 边界

- map JSON parse：PASS；
- boundary `rg`：no match / exit `1`（预期）；
- tracked `git diff --check` 与新增 source no-index whitespace check：PASS；
- 未修改 Build.cs、tags、schema、Content、ShanmenItems、P8.0/P8.1、GameMode、输入或旧产品链；
- P8.1 的 215 项既有 0.0.10 测试继续通过；
- 长期未跟踪用户文件未修改、未 stage。

本轮仅执行源码、静态门禁、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.3 建议建立 world-delivery seam，以 P8.2 audit 作为唯一 placement 输入，冻结重复投放与 spawn failure recovery；不让 World 层拥有材料、deployment 或 action 权威。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-2-formation-product-session/Docs/Report/Dev.D.UE.0.0.10.P8.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-2-formation-product-session/Docs/Log/Dev.D.UE.0.0.10.P8.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-2-formation-product-session>
