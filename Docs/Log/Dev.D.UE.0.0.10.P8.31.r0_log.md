# Dev.D.UE.0.0.10.P8.31.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.31.r0`；
- 基线：`03c1419cc89674c48f956b28b20f8cd846a874d7`（P8.30）；
- 分支：`agent/0.0.10-p8-31-formation-influence-consumer-product-command-fence`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

关闭 P8.30 product bridge 与 P8.29 run/content command host 之间的身份缺口。valid command 必须来自 bridge 冻结的 exact run、owner、source、deployment 与 content；同一 run/content 下另一 formation deployment 不得因 subject 已绑定而进入 native mutation。invalid command 仍交给 P8.29 保留原错误契约。

## 设计记录

### frozen lease identity

command 已携带 projector 冻结的 lease snapshot，因此 bridge 直接读取 lease key，不增加第二个身份 payload。`MatchesCommandProductIdentity` 是纯查询，route 仍执行同一检查，避免调用方跳过 preflight。

### fail-closed ordering

caller ProductHost 检查保持第一层；valid command product mismatch 是第二层；只有 exact command 或 malformed command 才进入 P8.29。这样 foreign valid command 在任何 binding/coordinator/native 状态前被拒绝，malformed command 则保留 `CommandInvalid`。

### authority preservation

bridge 不修改 lease、projection、command 或 ProductHost，不创建 command 副本、不保存 rejected command history。P8.29/P8.28/P8.26/attribute component 的既有职责不变。

## 执行序列

1. 复审 P8.30 route 顺序，确认 P8.29 只检查 run/content。
2. 审查 lease key，确定 product identity 可由 run/owner/source/deployment/content 完整约束。
3. 增加 `CommandProductIdentityMismatch` 与 `MatchesCommandProductIdentity`。
4. 在 ProductHost 校验后、P8.29 route 前加入 valid-command identity fence。
5. 扩展 fixture，使 correlation/action/content 可保持相同而 diagram/deployment 独立变化。
6. 新增 CommandProductIdentityFence，显式绑定 foreign deployment subject 后验证零 mutation。
7. 更新 route-fence 预期；malformed/unbound 继续验证嵌套错误保真。
8. Editor 首编 `5 actions / exit 0`；专项首轮 `5/5`。
9. 顺序回归在 projection 第三例启动后产生一次不完整日志，fail-closed 中止；单组重跑 `3/3` 后继续。
10. 十一份最终 Automation、regression gate、自检、静态扫描与双目标最终构建全部通过。
11. 完成 `git diff --check`、Report/Log 与 exact-stage 准备。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `P8.31-FormationInfluenceConsumerProductBridge-final.log` | 5 | 0 | 0 | `4CFF80B88800C40A422FBA06B60D53AB15818EBD5904CE26E8E8AC5C48F25811` |
| `P8.31-FormationProductHost-final.log` | 6 | 0 | 0 | `226398191353E8FAF7B021AD588F4126470DDD8305A60C2B26EA9C72BED0A785` |
| `P8.31-FormationSession-final.log` | 4 | 0 | 0 | `B2BE4E8C4A9F83509EE6655B5D1CFE7B636D7B05FB627F436CB6DEB8647B4197` |
| `P8.31-FormationInfluenceConsumerCommandHost-final.log` | 4 | 0 | 0 | `365381E195D401ACCDD61630EE06F18061FC8D7ED7AD5288A0BDA7E49EEED1E3` |
| `P8.31-FormationInfluenceConsumerApplicationCoordinator-final.log` | 4 | 0 | 0 | `1AAB5FC7A2EAAADB795BE942C642BA190C555C2033256C89DEE7AE429D25161F` |
| `P8.31-FormationInfluenceConsumerAttributeAdapter-final.log` | 4 | 0 | 0 | `395A5F80CEA886248AF24FDDF34AA4089D9842CB6DA508F6C0191BDF505854EE` |
| `P8.31-FormationInfluenceConsumerRegistry-final.log` | 4 | 0 | 0 | `7AC92DAF8F1F712AA3084B62E1D4C3A863F91FB60CA9A032E62001B873BAD883` |
| `P8.31-FormationInfluenceConsumerProjection-final.log` | 3 | 0 | 0 | `C45C29A86D489C34C0F3800C82AA2B265BFDFCE925E95812C99BB3A72E2FF5F0` |
| `P8.31-Attributes-final.log` | 4 | 0 | 0 | `CD502434D531E90B11A6005766CE833F241DC45429D111CC7095DBC09452C561` |
| `P8.31-FormationInfluence-final.log` | 80 | 0 | 0 | `AE477910A71BA6B2EEA77A73367C9A786CCED4185262C14F5B258D17588B6817` |
| `P8.31-Shanmen-full-final.log` | 329 | 0 | 0 | `1DD31CB6CED89AC9986BF3495E80AC6F5DC8786B5059CE7C4272FBC2B3B1C644` |

全部最终日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=66
SELF_TEST: PASS 92/92
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=14 Logs=11
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=14 Logs=11
```

- mapping SHA-256：`E48DD688C2017B7B49C07A9EA74BF6045C87DAE737F73EC90EB407BC8E379D46`；
- self-test SHA-256：`9C1A64FE9F31B4E0153C11B2D29994ABC35B315FB089872F5CCD777730A01879`。

## 静态边界

```text
UWorld/AActor = 0
GameplayAbility/GameplayEffect/AbilitySystem = 0
RNG = 0
SaveGame/ProfileRepository = 0
Tick/while = 0
GetSubsystem/FindComponent/TActorIterator = 0
RetryQueue/PendingCommands/ActiveModifiers = 0
TMap = 0
```

## 构建证据

- Editor initial：`5 actions / 26.61s / exit 0`，log SHA-256 `F8BD4FB17A5DBD0A6C4FA8DDDEA1719279441D16BAB1827A8CDB70AFA848D91B`；
- Editor final：`0 actions / 0.96s / exit 0`，log SHA-256 `04CF4FEB69A27E56FB98BA97A66010EFAFDA0236F36E616309DE44B67D02FD23`；
- Game final：`4 actions / 23.20s / exit 0`，log SHA-256 `6E89655D655DB356C2A650C92E32A3B57E1A3B39C93268BD9703799F59968860`；
- `UnrealEditor-demo_map.dll`：`12010496` bytes，SHA-256 `AABA5DACFE795BC3825B296782573822DE02D612F053AC99CEF67498A4AC22AC`；
- `demo_map.exe`：`353071616` bytes，SHA-256 `85E246B7CAC300537276E340D24212A15CF09340F6E528A3626FD170704D18FF`。

## 真实异常记录

顺序回归第一次执行 projection 时，无头进程在第三个测试已经启动后以 `0` 退出，留下 `2 Success / 0 Fail / 0 queue-empty`。runner 没有接受该日志并立即停止后续组。保留日志 SHA-256 为 `F5FA52526DB94281AD750E261893D1AB5587C9C0D1A539843A5685F26119E9FA`。同组独立重跑为 `3/3 / queue-empty / exit 0`，其后 influence `80/80`、全量 `329/329`；没有产品源码修正。

UE 前置条件演示错误与非 Win64 SDK 枚举警告不属于 selected test 失败。最终日志的正式测试、queue-empty、fatal/unhandled/ensure 与原生退出码全部满足 gate。

## P/F 边界

仅执行源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.32 可在 exact command product fence 之上增加显式 consumer product runtime，负责 caller-provided binding 与 Apply/Remove 生命周期编排，并在 teardown 前要求 bridge drained；不自动发现、不自动重试、不重采样、不复制权威。
