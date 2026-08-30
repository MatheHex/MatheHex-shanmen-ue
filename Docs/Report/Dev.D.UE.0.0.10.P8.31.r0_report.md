# Dev.D.UE.0.0.10.P8.31.r0 Report

## 1. 结论

P8.31 已为 formation influence consumer product bridge 增加 exact command-to-product identity fence。P8.30 已验证调用者提交的 `ProductHost`，但 P8.29 command host 只约束 run/content；因此同一 run/content 下另一 deployment 的有效 command 在 subject 已绑定时仍可能穿过桥。本阶段在进入 P8.29 之前交叉验证 command lease 的 run、owner、source、deployment 与 content，关闭该跨产品命令路径。

本阶段结论为 **PASS**：product bridge 专项 `5/5`、ProductHost `6/6`、FormationSession `4/4`、P8.29 host `4/4`、P8.28 coordinator `4/4`、P8.27 adapter `4/4`、P8.26 registry `4/4`、P8.25 projection `3/3`、legacy attributes `4/4`、完整 influence 链 `80/80`、`Shanmen.0_0_10` 全量 `329/329`、changed-file regression gate、自检 `92/92`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 exact command product identity

- 新增 `MatchesCommandProductIdentity`，只接受有效 bridge 与有效 command；
- 从 command projection 的 frozen lease key 读取身份，不接受调用方额外声明或可变旁路参数；
- 必须同时满足 `RunId == ActiveRunId`、`OwnerId == Correlation.OwnerId`、`SourceEntityId == ActionSourceEntityId`、`DeploymentId == frozen DeploymentId`、content version/digest 完全一致；
- 该查询公开给后续 runtime 做无副作用 preflight，但 bridge 仍是 route 时的最终执行栅栏。

### 2.2 route 顺序与错误保真

- `TryRoute` 仍先检查 bridge、caller ProductHost、exact product identity 与 terminal 状态；
- ProductHost 合法后，valid foreign command 返回产品级 `CommandProductIdentityMismatch`，不进入 P8.29；
- malformed command 不被新栅栏截断，仍委托 P8.29 并保留原 `CommandInvalid` 嵌套诊断；
- exact own-product command 的 Apply、Remove 与 immutable replay 行为不变。

### 2.3 同 run/content 跨 deployment 防护

专项测试启动两个具有完全相同 correlation、action activation/source 与 content、但 diagram/deployment 不同的真实 ProductHost。foreign deployment command 的 subject 被显式绑定到 bridge 后，route 仍在产品桥拒绝；completed transaction count 与两个 attribute component 的 active modifier count 均保持 `0`。这直接覆盖 P8.30/P8.29 组合原先无法区分的路径。

## 3. 完整性与安全边界

ProductHost/Session 仍是 formation 产品生命周期权威，P8.29 仍是 subject binding 与 route 聚合器，P8.28 仍是 transaction/replay 权威，P8.26 registry 仍是 active application 权威，`Udemo_mapAttributeComponent` 仍是 native modifier 权威。P8.31 只读取 frozen command lease identity 并 fail closed，不改写 command、projection、lease、ProductHost 或任何下层历史。

生产 bridge 文件静态扫描：`UWorld/AActor = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`RNG = 0`、`SaveGame/ProfileRepository = 0`、`Tick/while = 0`、`GetSubsystem/FindComponent/TActorIterator = 0`、`RetryQueue/PendingCommands/ActiveModifiers = 0`、`TMap = 0`。

## 4. 修改范围

更新：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductBridge.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductBridge.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductBridgeTests.cpp`；
- 本 Report 与同名 Development Log。

bridge header/implementation/tests 最终分别为 `136/345/650` 行；文档加入前生产与测试差异共新增 `140` 行、删除 `20` 行。未修改 regression map，因为现有 product-bridge path rule 已精确覆盖本轮三个源码路径；长期未跟踪的用户与 0.0.9B 工件未修改、未 stage。

## 5. 自动化验证

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P8.31-FormationInfluenceConsumerProductBridge-final.log` | `FormationInfluenceConsumerProductBridge` | 5 | 0 | 0 | `4CFF80B88800C40A422FBA06B60D53AB15818EBD5904CE26E8E8AC5C48F25811` |
| `P8.31-FormationProductHost-final.log` | `FormationProductHost` | 6 | 0 | 0 | `226398191353E8FAF7B021AD588F4126470DDD8305A60C2B26EA9C72BED0A785` |
| `P8.31-FormationSession-final.log` | `FormationSession` | 4 | 0 | 0 | `B2BE4E8C4A9F83509EE6655B5D1CFE7B636D7B05FB627F436CB6DEB8647B4197` |
| `P8.31-FormationInfluenceConsumerCommandHost-final.log` | `FormationInfluenceConsumerCommandHost` | 4 | 0 | 0 | `365381E195D401ACCDD61630EE06F18061FC8D7ED7AD5288A0BDA7E49EEED1E3` |
| `P8.31-FormationInfluenceConsumerApplicationCoordinator-final.log` | `FormationInfluenceConsumerApplicationCoordinator` | 4 | 0 | 0 | `1AAB5FC7A2EAAADB795BE942C642BA190C555C2033256C89DEE7AE429D25161F` |
| `P8.31-FormationInfluenceConsumerAttributeAdapter-final.log` | `FormationInfluenceConsumerAttributeAdapter` | 4 | 0 | 0 | `395A5F80CEA886248AF24FDDF34AA4089D9842CB6DA508F6C0191BDF505854EE` |
| `P8.31-FormationInfluenceConsumerRegistry-final.log` | `FormationInfluenceConsumerRegistry` | 4 | 0 | 0 | `7AC92DAF8F1F712AA3084B62E1D4C3A863F91FB60CA9A032E62001B873BAD883` |
| `P8.31-FormationInfluenceConsumerProjection-final.log` | `FormationInfluenceConsumerProjection` | 3 | 0 | 0 | `C45C29A86D489C34C0F3800C82AA2B265BFDFCE925E95812C99BB3A72E2FF5F0` |
| `P8.31-Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `CD502434D531E90B11A6005766CE833F241DC45429D111CC7095DBC09452C561` |
| `P8.31-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 80 | 0 | 0 | `AE477910A71BA6B2EEA77A73367C9A786CCED4185262C14F5B258D17588B6817` |
| `P8.31-Shanmen-full-final.log` | `Shanmen.0_0_10` | 329 | 0 | 0 | `1DD31CB6CED89AC9986BF3495E80AC6F5DC8786B5059CE7C4272FBC2B3B1C644` |

十一份最终日志各有一个 RunTests command、一个正式 queue-empty，selected Fail、fatal、unhandled 与 ensure 均为 `0`。

## 6. 专项覆盖与提交前审查

五个 product bridge case 覆盖：

1. deterministic open、exact ProductHost 与 exact command identity；
2. bind/replay、Apply/replay/Remove、drain/count；
3. foreign/invalid ProductHost 在 bind/route 前拒绝；
4. subject replacement、component reuse、invalid subject、null component 与 drained replacement fence；
5. malformed command 和 unbound subject 保留 P8.29 原始诊断；
6. foreign run/owner/source/content command 在 bridge 拒绝；
7. 同 run/owner/source/content、仅 deployment 不同的 command 在 bridge 拒绝且零 mutation；
8. Apply/Remove 后历史 Apply replay 不复活 modifier。

测试通过真实 area → intent → evaluator → lease executor → projector 生成 command；same-scope foreign deployment 通过真实不同 diagram 启动，不修改私有字段、不伪造 bridge 内部状态。

## 7. Changed-file regression gate

现有 product-bridge path rule 覆盖本轮所有源码变化，共要求十四组测试契约；十一份日志通过父组覆盖完整满足。

```text
REGRESSION_MAP_JSON: PASS Rules=66
SELF_TEST: PASS 92/92
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=14 Logs=11
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=14 Logs=11
```

- mapping SHA-256：`E48DD688C2017B7B49C07A9EA74BF6045C87DAE737F73EC90EB407BC8E379D46`；
- self-test SHA-256：`9C1A64FE9F31B4E0153C11B2D29994ABC35B315FB089872F5CCD777730A01879`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial | Succeeded / 5 actions / 26.61s | 0 | `F8BD4FB17A5DBD0A6C4FA8DDDEA1719279441D16BAB1827A8CDB70AFA848D91B` |
| Editor final | Succeeded / 0 actions / 0.96s | 0 | `04CF4FEB69A27E56FB98BA97A66010EFAFDA0236F36E616309DE44B67D02FD23` |
| Game final | Succeeded / 4 actions / 23.20s | 0 | `6E89655D655DB356C2A650C92E32A3B57E1A3B39C93268BD9703799F59968860` |

- `UnrealEditor-demo_map.dll`：`12010496` bytes，SHA-256 `AABA5DACFE795BC3825B296782573822DE02D612F053AC99CEF67498A4AC22AC`；
- `demo_map.exe`：`353071616` bytes，SHA-256 `85E246B7CAC300537276E340D24212A15CF09340F6E528A3626FD170704D18FF`。

## 9. 真实异常与兼容性

十一组顺序回归第一次执行到 projection 时，UE 无头进程在第三个 case 已启动后提前结束：原生退出码为 `0`，日志含 `2` 个 Success、`0` 个 Fail、无 fatal/unhandled/ensure，但缺 queue-empty，因此 runner 按 fail-closed 中止后续组。该不完整证据保留为 `P8.31-FormationInfluenceConsumerProjection-final-backup-2026.08.30-07.29.08.log`，SHA-256 `F5FA52526DB94281AD750E261893D1AB5587C9C0D1A539843A5685F26119E9FA`。同组单独重跑立即 `3/3`、queue-empty、exit `0`，随后 influence `80/80` 与全量 `329/329` 均通过；没有源码修正，也没有把不完整日志冒充成功证据。

UE 启动阶段在正式 RunTests command 前仍会输出引擎自带 UnifiedError/Automation 条件演示日志；selected tests 全部 Success，正式 queue-empty、原生退出码与 fatal/unhandled/ensure 检查通过。非 Win64 SDK 枚举警告未影响 Win64 VALID 状态。

`git diff --check`：PASS。P8.25–30 wire shape、ProductHost/Session、attribute component、regression map 与 legacy 产品权威均未修改。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.32 在该身份栅栏之上增加显式 consumer product runtime：只接收 caller-provided ProductHost/component 与已经冻结的 projected command，按 Apply/Remove 生命周期调用 bridge，并在 teardown 前强制 `IsDrained`；仍不得自动发现、自动 retry、重采样 magnitude 或复制 P8.24–31 任一权威。
