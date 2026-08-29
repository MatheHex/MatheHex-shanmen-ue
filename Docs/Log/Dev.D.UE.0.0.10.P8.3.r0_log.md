# Dev.D.UE.0.0.10.P8.3.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.3.r0`；
- 基线：`a8e5323d21bd9e35f1373cce8ae33de852bdcdcf`（P8.2）；
- 分支：`agent/0.0.10-p8-3-formation-world-delivery`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.3 只把 P8.2 已提交阵眼 audit 安全投影到 World，冻结 placement identity、same-process replay、adapter reconstruction、duplicate conflict、spawn postcondition 与 terminal teardown。

明确后置：产品 host/controller、投料交互、区域/效果 provider、阵法战斗规则、UI、正式阵图商品、配方与数值。

## 契约设计

### 唯一输入

Placement intent 只能从有效 Session 的 committed audit 与 P8.0 committed anchor progress 构造。它绑定 Run/Owner/Deployment/AnchorInstance、Attempt/Fulfillment/deployment receipt、authority revision 与 content。未提交 anchor 没有世界投放路径。

### 身份与 class fence

PlacementId 不含 Actor class；同阵眼换类不能创造第二 identity。Receipt 额外绑定 class path。Actor 使用 deterministic placement/deployment tags，供同实例 replay、进程重建 adoption 与 terminal scan 使用。

### 弱所有权

Adapter 只保存 `TWeakObjectPtr<AActor>`。首次 spawn 完成 class/location/tag 后置条件才发布记录；失败立即回滚 transient Actor。World、Session、deployment、item authority 与 action 均不被 Adapter 拷贝为第二 authority。

### 终态清理

只有 Cancelled/Ended Session 可 cleanup。Adapter 同时清理已知弱引用与 deployment-tag scan；部分成功后保持 bound recovery state 并累计 removed count，exact replay 前向完成。完整 cleanup 发布稳定 receipt。

## 实现范围

新增：

- `Source/demo_map/demo_mapShanmenFormationWorldAdapter.h`；
- `Source/demo_map/demo_mapShanmenFormationWorldAdapter.cpp`；
- `Source/demo_map/demo_mapShanmenFormationWorldAdapterTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

生产代码没有 item authority、legacy inventory、damage、RNG、Tick/timer、输入或 UI。测试使用 isolated profile、真实 migration/cutover、真实 active Run、P8.2 Session 与 headless GamePreview UWorld。

## 诊断与修正记录

1. 首次 Editor 构建成功；首次 focused 为 `1 Success / 3 Fail`。基础 `AActor` 无 RootComponent，不能保持 placement location，生产 postcondition 正确拒绝并回滚。fixture 改用 `ACharacter`。
2. 第二、第三轮 focused 为 `3 Success / 1 Fail`。terminal 断言揭示 `IsPlacementSuccess()` 会随 weak Actor 后续销毁而改变历史结果。最终让 operation success 只依赖 immutable status/intent/receipt，弱引用单独表示当前 Actor 可达性。
3. 提交前语义审查发现：重建 Adapter 的 partial teardown 若已绑定却尚未完成，旧 `IsValid` 会拒绝 replay。最终允许“bound for teardown retry”状态，并累计同一恢复链已移除数量。
4. 三份失败日志均保留。Automation 进程在测试失败时仍返回 `0`，最终判定始终使用 Result/queue-empty，而不是仅看进程码。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationWorldDelivery.log` | `Shanmen.0_0_10.Product.FormationWorldDelivery` | 4 | 0 | 0 | `E4E405008782B0CAD79D26F2A0D10C4AC9AAA7E61825F0819F496413AF7D46A2` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 223 | 0 | 0 | `85B073889C4ACCB0E4C00EC75A21274D9358B60465CAD1E28DDAB6F78CF0037A` |

最终日志均 one command、one queue-empty、Fail `0`、fatal/assert/ensure `0`。

### 保留的失败日志

| 日志 | Success | Fail | Process exit | SHA-256 |
|---|---:|---:|---:|---|
| `FormationWorldDelivery-first.log` | 1 | 3 | 0 | `675E06794ACFE4FED5AAB3787323EFF999B0AAA22E3A224845AAE8601F521DF8` |
| `FormationWorldDelivery-second.log` | 3 | 1 | 0 | `DA18C9DCC896E1996EFFDCDE02294D2E9DC7766ACB5C7B455B8793F038968F2D` |
| `FormationWorldDelivery-third.log` | 3 | 1 | 0 | `33B94069AB8822827BF3B49F4BCB668B0A23CBC0F06AC6F2436586C2AEFA5D91` |

## 测试内容

1. deterministic placement intent、spawn、tag 与 same Actor/receipt replay；
2. reconstruction adoption、class drift fence、duplicate-tag fail-closed；
3. uncommitted/world/class/session terminal 失败边界；
4. non-terminal teardown fence、两阵眼 End、reconstructed teardown、stable replay receipt。

## Changed-file regression gate

新增 FormationWorldDelivery rule，要求 focused World、FormationSession、WorldGameplay 与 FormationDeployment。child-only fixture 必须失败，broad full 可覆盖全部 parent groups。

```text
REGRESSION_MAP_JSON: PASS Rules=39
SELF_TEST: PASS 39/39
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=2
```

- coverage SHA-256：`F9F06AB72C3636A79528AC8BE20FAFB2BEC58B2C432111D358A86A11BFE46888`；
- self-test SHA-256：`035490D8337FD35B95B0BB63A08304731F8370A3A4C8013EC7B49844D06EA76A`。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | Succeeded | 0 | 15.77s | `B84D867FCD3716C2A6004366D4BDBA6DAECB972D0CE7A74D62644A03202DD8BC` |
| Editor final | Succeeded | 0 | 11.05s | `D70723DC0C30868894D896ADF2B77DD27FAF8C43B723418B2A1AF4F40E5D551C` |
| Game final | Succeeded | 0 | 14.55s | `12E3E3C9A2DE9600D7D2113DA9E1B3B6FEA4BB55A34208BD7A0C302E34E8F4A7` |

所有构建均原生退出码 `0`；无源码、UHT、link、commit-memory 或环境失败。

## 静态、兼容性与 P/F 边界

- map JSON parse：PASS；
- boundary scan：no prohibited production dependency；
- mapping self-test 与 changed-file gate：PASS；
- tracked/untracked whitespace checks：PASS；
- 未修改 Build.cs、tags、schema、Content、ShanmenItems、P8.0/P8.1/P8.2、GameMode、输入或旧产品链；
- 长期未跟踪用户文件未修改、未 stage。

本轮仅执行源码、静态门禁、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.4 建议建立 Formation product host/controller，闭合 material/deployment commit -> placement -> terminal teardown 的 forward-only 调用与重试；效果、交互、UI 与正式 content 继续后置。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-3-formation-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P8.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-3-formation-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P8.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-3-formation-world-delivery>
