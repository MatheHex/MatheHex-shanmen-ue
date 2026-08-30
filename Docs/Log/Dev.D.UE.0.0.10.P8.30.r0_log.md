# Dev.D.UE.0.0.10.P8.30.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.30.r0`；
- 基线：`881b97d85d4cc51b279f36776954897fb91dec61`（P8.29）；
- 分支：`agent/0.0.10-p8-30-formation-influence-consumer-product-bridge`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

在 P8.29 multi-subject command host 与既有 formation ProductHost/Session 之间增加显式产品桥。桥冻结 exact product identity，每次 bind/route 均由调用方提交 ProductHost 和 component，产品级结构化结果保留 P8.29/P8.28 的原始 receipt/rejection/replay。不得自动发现 component、自动重试、创建第二套 transaction/history 或修改 ProductHost 生命周期权威。

## 设计记录

### exact product identity

bridge 保存完整 correlation、action activation/source、deployment、content 与确定性 bridge ID。ProductHost 可以正常推进内部合法状态，但只要其中任何冻结身份不同，bind/route 都在进入 command host 前拒绝。

### explicit narrow delegation

bridge 内只组合一个 P8.29 command host。bind/route 成功状态映射到产品级 enum，同时嵌套保留原 binding/route/transaction evidence；replay 不标记 state changed。component、application、native modifier 与 completed history 均由既有层继续拥有。

### lifecycle fence

terminal ProductHost 不接受新 bind/route。上层通过 `IsDrained` 在产品 teardown 前检查所有 active application 已 Remove；drained binding 仍不能 replacement，历史 Apply 仍由 P8.28 replay fence 截断。

## 执行序列

1. 审查 P8.29 command host、FormationProductHost/Session 与 lifecycle command-host identity 模式。
2. 确定桥只冻结值身份，不持有 ProductHost pointer，不自动发现 engine objects。
3. 新增 product binding/route status/result 与 product bridge。
4. 建立纯值有效 ProductHost 测试夹具，并经真实 area/intent/evaluator/lease/projector 生成 command。
5. 增加 RouteAndDrain、ProductIdentityFence、BindingLifecycleFence、RouteAndReplayFence 四个 case。
6. 首次 Editor 编译因测试缺 `ShanmenCombatResolver.h` 失败，退出 `6`；补 include 后 `4 actions` 成功。
7. bridge 首轮与最终专项均 `4/4`；十一组最终 Automation 全部退出 `0`。
8. regression map 增至 `66` 条，自检增至 `92/92`，gate 为 `Changed=5 / Rules=1 / Required=14 / Logs=11`。
9. Editor final up-to-date；Game final `4 actions` 成功。
10. 完成静态扫描、`git diff --check`、文档与 exact-stage 准备。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `P8.30-FormationInfluenceConsumerProductBridge-final.log` | 4 | 0 | 0 | `A655A73EC8A8FA5E5E36D5843AD8D4EAE68E1A53B5992CE593BA59650DA84FA8` |
| `P8.30-FormationProductHost-final.log` | 6 | 0 | 0 | `E360239415D8D10674FC7404C4624FFE334F976886FCD54278A7DDD35A4EA975` |
| `P8.30-FormationSession-final.log` | 4 | 0 | 0 | `0FC3C6EE68DBB7854FF6AC9AC77B99DCAFBD19BC446FBD669EBCDC5631D2A0E3` |
| `P8.30-FormationInfluenceConsumerCommandHost-final.log` | 4 | 0 | 0 | `5A16D4C58436722210CAD47E1A4839E25CD8D25A55F38FB1739BC8B463814911` |
| `P8.30-FormationInfluenceConsumerApplicationCoordinator-final.log` | 4 | 0 | 0 | `EFD6F2AF5719E472A270194370AD4BA187AF48C386F06B5F819523D1620DE1BE` |
| `P8.30-FormationInfluenceConsumerAttributeAdapter-final.log` | 4 | 0 | 0 | `62D70BF487F1C56621FE0A90F5109F9A27EBC480F0780EC4990D3F14485CD423` |
| `P8.30-FormationInfluenceConsumerRegistry-final.log` | 4 | 0 | 0 | `B605C9376379ADB9477AF6DE6B0C194B039EE5508BFD6EF0B758F0F41F0BEA90` |
| `P8.30-FormationInfluenceConsumerProjection-final.log` | 3 | 0 | 0 | `26673BAAB5D2A676A1CD461665CA8465548DE6A9A99452967D28284FC7C59889` |
| `P8.30-Attributes-final.log` | 4 | 0 | 0 | `4B80A0E8FD7C49447E746DD740744163EC38CE096AB960C88A3DCCF233C25270` |
| `P8.30-FormationInfluence-final.log` | 79 | 0 | 0 | `D2DE10E7DD55AEA999994EE33DD8A5141E8AECBC2D148EF5A4B258BAB6F05A58` |
| `P8.30-Shanmen-full-final.log` | 328 | 0 | 0 | `A920D4181FDBC7A03952A2CAAC5557E30271065A22F6C66C487BE0C0E1C70E42` |

全部日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=66
SELF_TEST: PASS 92/92
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=14 Logs=11
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=14 Logs=11
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

- Editor first attempt：测试 include 缺失，`OtherCompilationError / exit 6`；
- Editor corrected：`4 actions / exit 0`；
- Editor final：`0 actions / 0.92s / exit 0`，log SHA-256 `08659575C02ECE3439852ABE1969FF1E0CFA61371F90F816979CC41F4C9237EC`；
- Game final：`4 actions / 23.97s / exit 0`，log SHA-256 `3AE082A4C7EB1DF17175710E8406CF7B575EAECCEA42E0E4239724678EACE4ED`；
- `UnrealEditor-demo_map.dll`：`12003840` bytes，SHA-256 `D5B196A75811E53F11953FB72B6CAA2FBCF00AC727FA847074C3206DC4AC5E4D`；
- `demo_map.exe`：`353066496` bytes，SHA-256 `F611AF65DE7EE08AF582CEC86764E599C92375AC797BF9B0FF095BA59222A898`。

## 真实异常记录

首次 Editor 编译仅因新增测试未 include `ShanmenCombatResolver.h`，导致 `FShanmenCombatIdFactory` 未声明。补齐 include 后产品源码、测试、Automation 与双目标构建全部通过。UE 启动前置自检日志与非 Win64 SDK 枚举警告未影响 selected tests、Win64 VALID 或原生退出码。

文档加入后的第一次 gate 复查误把两份 `*-build-final.log` 当作 Automation evidence，validator 正确 fail closed；收窄为十一份 Automation log 后通过。随后一次 `pwsh -Command` 的 `$logs` 引用被外层提前展开，改用当前 PowerShell 显式数组调用后 `Changed=7` 通过。两项均为证据选择/命令引用错误。

## P/F 边界

仅执行源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.31 可增加显式 consumer product runtime，把已经冻结的 lease/projected command 与 caller-provided component binding 按 Apply/Remove 生命周期交给 P8.30，并在 ProductHost teardown 前强制 `IsDrained`；不自动发现、不自动重试、不重采样、不复制权威。
