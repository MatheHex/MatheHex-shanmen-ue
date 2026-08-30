# Dev.D.UE.0.0.10.P8.39.r0 Report

## 1. 结论

P8.39 已建立首个显式 formation consumer combat-Run composition command。新入口只接受调用方已经持有的 CombatRun coordinator、registered subject、ProductHost、LifecycleHost、不可变 delivery 与 AttributeComponent capability，并严格顺序组合：

1. CombatRunCoordinator 建立 source-derived entity alias；
2. P8.37 resolver 从当前 Run registry 生成 pointer-free World resolution；
3. P8.36 LifecycleCommandHost 应用原生 consumer delivery。

本阶段结论为 **PASS**：composition focused `1/1`、CombatRunCoordinator `17/17`、World resolution `1/1`、LifecycleCommandHost `5/5`、FormationProductHost `6/6`、WorldGameplay `10/10`、legacy Attributes `4/4`、`Shanmen.0_0_10` 全量 `337/337`；changed-file regression gate、映射 JSON、自检 `100/100`、局部静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 单次显式组合

`Fdemo_mapShanmenFormationInfluenceConsumerRunComposition::TryActivate(...)` 不再要求上层手工拼接三份 receipt。它先让 CombatRunCoordinator 从 registered subject 推导 EntityId，并把 AttributeComponent 绑定为同一实体 alias；随后使用 coordinator 的 const registry view 解析该 capability；最后把不可变 delivery、resolution 和 live capability 交给 LifecycleCommandHost。

入口不扫描 World、不查找组件、不取 subsystem、不调度 Tick、不轮询、不重试，也不保存任何 UObject pointer。所有 live pointer 都是调用期借用 capability。

### 2.2 pointer-free 聚合证据

新增 `Fdemo_mapShanmenFormationInfluenceConsumerRunCompositionResult`，完整保留 alias、World resolution 与 delivery application 三层 receipt，并增加组合状态、RunId、阶段检查位与诊断。

`IsSuccess()` 会交叉验证：

- 三层 receipt 均自证成功；
- RunId 在 alias 与 resolver 间一致；
- alias EntityId、resolver EntityId 与 delivery subject 一致；
- AttributeComponent UniqueId 在 alias、resolution 与 application 间一致；
- LifecycleHost 实际采用的 resolution id 等于本轮 resolver 生成的 resolution id；
- 顶层 `Activated` / `ActivationReplayed` 与底层 application 状态严格对应。

任何意外成功状态或嵌套证据分歧都会转为 `StateInvalid`，不会伪装成成功。

### 2.3 forward-only identity 与幂等

Entity alias 是当前 Run 的身份事实，不因后续 ProductHost/LifecycleHost 拒绝而回滚。专项用例证明：foreign LifecycleHost 拒绝 delivery 时，正确的 AttributeComponent alias 仍可供正确 Host 重试；被拒绝的 Host 与 AttributeComponent 没有原生 modifier 副作用。

正确 Host 首次调用返回 `Activated`；相同 delivery 重放返回 `ActivationReplayed`，registry binding、consumer binding 与 native modifier 均不重复。显式 deactivation 会移除 modifier 并排空 consumer runtime，随后 Run end 清空 registry alias。

## 3. 拒绝语义与兼容性

顶层状态区分 `AliasRejected`、`WorldResolutionRejected`、`DeliveryApplicationRejected` 与 `StateInvalid`：

- inactive CombatRun 在 resolver 与 delivery 之前拒绝；
- 未注册 source 不能铸造 EntityId 或 alias；
- alias 成功后仍必须经当前 Run registry 复核；
- foreign LifecycleHost 找不到 source receipt 时不修改 runtime/attributes；
- 既有 CombatRunCoordinator、resolver 与 LifecycleHost API 未改变；
- 没有引入 GameMode glue、后台 controller、第二套 registry 或反向 EntityId→Actor 表。

本阶段只提供窄化的显式 activation composition；生命周期 owner 尚未自动调用它，deactivation 仍显式走既有 LifecycleHost。这是刻意保留的 P 阶段边界，不宣称已经完成产品运行时接线。

## 4. 修改范围

生产代码：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRunComposition.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRunComposition.cpp`。

验证与流程：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

新增生产组合边界共 `222` 行；专项用例增加 `240` 行；回归映射增加 `14` 行，自检增加 `16` 行。映射规则由 `68` 增至 `69`，自检由 `98/98` 增至 `100/100`。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 5. Automation 证据

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `ConsumerRunComposition-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerRunComposition` | 1 | 0 | 0 | `FB8E2611F4CB3B2AE19BB69BE9CE35AC02A8FC2FEB05945F47165290AFA79750` |
| `CombatRunCoordinator-final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 17 | 0 | 0 | `FBFDF255F88AFCC6A2F7BBA6F01EF7DDE306CA7DB49FD130FD6C58EDE5F85695` |
| `ConsumerWorldResolution-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerWorldResolution` | 1 | 0 | 0 | `70583F4CF369677220EA04A41DC5917C68015186924D4932D9A557A006BF9910` |
| `LifecycleCommandHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost` | 5 | 0 | 0 | `1A150903B245519BC8E1C207FB833305C9A2CE173458A28765ED7EB07BEF4C79` |
| `FormationProductHost-final.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 6 | 0 | 0 | `10A8E2314842A89B10033E21962588E56635604D4951ED3092747448FB73E025` |
| `WorldGameplay-final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `767059FBC02A9453FC122787F4507DE5306344E6EB4F77E04DA07AB0DFD89EC7` |
| `Attributes-final2.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `AFD735D3A8ED8A856223470EA85B9244AF54F117E634C3231731AF32784C44F6` |
| `Shanmen-full-final.log` | `Shanmen.0_0_10` | 337 | 0 | 0 | `2FDB97A9A50D4CC45797594F897DBEDF0B7F97D83B8224DA42A9ABA528183259` |

八份最终日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## 6. 专项覆盖

新增 `ExplicitActivation` 用例覆盖：

- inactive coordinator 与未注册 source 在 delivery 前 fail-closed；
- foreign LifecycleHost 拒绝 delivery、无 modifier 副作用，同时保留有效 identity alias；
- 正确 Host 完成 alias → registry resolution → native delivery；
- nested receipt 的 EntityId、RunId、resolution id 与 capability UniqueId 一致；
- exact replay 不增加 registry binding、consumer binding 或 modifier；
- explicit deactivation 排空 native state；
- Run end 清空派生 alias。

## 7. Changed-file regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=69
SELF_TEST: PASS 100/100
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=2 Required=38 Logs=8
REGRESSION_COVERAGE (exact staged): PASS Changed=7 Rules=2 Required=38 Logs=8
```

ProductHostTests 是多个 product rule 的共享测试文件，因此 implementation gate 对其匹配规则取并集，要求 `38` 个组；`Shanmen.0_0_10` 全量日志覆盖其余父组，七份 focused/legacy 日志提供关键边界的独立证据。

- mapping SHA-256：`F64264C9186448E10283D6B43856F6EB525C6BCD49591A6F5306396903E55448`；
- self-test SHA-256：`E2CBB790626C47C56E2F18194FF6EE9F62B992C854057844E3DFAE47EE81975C`；
- exact stage：PASS，只有本阶段 `7` 个文件；
- `git diff --check` 与 `git diff --cached --check`：PASS。

## 8. 静态边界

扫描限定到新增组合头/实现（合计 `222` 行）：

- `FindComponent=0`、`TActorIterator=0`、`GetSubsystem=0`；
- Tick/while `0`、`TMap=0`；
- `UWorld=0`、`AActor=0`、RNG `0`；
- result 中 raw UObject pointer member `0`；
- 只存在预期的 caller-owned UObject/AttributeComponent 输入参数，不进入 result 或新状态。

## 9. 构建与真实异常

构建命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Log SHA-256 |
|---|---|---:|---|
| Editor candidate | Failed / `C2664` / 31.15s | 6 | `13D65E66AD88BCC340DDFD13EA1D78063A83ACF8E5F3D16E958706424C9ABEC6` |
| Editor retry | Succeeded / 4 actions / 6.40s | 0 | `D15DBFDDB8CAA3D5039D96400EFECB726401CB19D93CF66EFCE4BD7C4205FD33` |
| Editor fixture fix | Succeeded / 4 actions / 7.28s | 0 | `E449EDC6FEA3FBA4917BA1BC49191FA76CF2E6E882D3AB8C086850E2564FC823` |
| Editor final | Succeeded / 4 actions / 5.88s | 0 | `1B7BB83C9F361A814BF9D70EA61754105551888351CD5B8CD2EA6C6FBC3DCC32` |
| Game final | Succeeded / 4 actions / 25.67s | 0 | `723DD09F39C121F333B24794D2FF8C21827E669AFD466F51A57F013179DF7584` |

- 首次 Editor 构建因 cpp 只有 AttributeComponent 前置声明，编译器无法证明它可转换为 `const UObject*`；加入完整类型 include 后成功。该失败是源码失败，不是内存或环境故障。
- 首次 focused 用例错误地实例化 abstract `UObject` 作为“未注册 source”，触发 handled ensure 并 `1/1 Fail`；改为 concrete `USceneComponent` 后 focused `1/1` 成功。失败日志 SHA-256 为 `6354599C456002B0AD4829EA3DFB40B88D97C098763E23C08E9738681C62CB12`。
- legacy Attributes 两次运行均在 `ModifierSemantics` 完成消息落盘前结束，原生退出码均为 `0`，但只有 `3/4` success 且无 queue-empty，因此按证据不完整处理，未计为通过。加入 `-forcelogflush` 后同一组正式 `4/4`、queue-empty、exit `0`；未修改 Attributes 产品代码或测试断言。两份不完整日志 SHA-256 分别为 `4C31A728948E1171B6592F90BFDE72A0367FB0DD36406003842789B6910EDA9B`、`6C458C56C94A13B2734FCA865EEE937B206A7D69A2D78100358EE65C6E2955AB`。

最终产物：

- `UnrealEditor-demo_map.dll`：`12137472` bytes，SHA-256 `CE44A80A2D326DB01F71E27E5BE6BDFAB4D55C812A320EC273EF93B6613ECC1D`；
- `demo_map.exe`：`353171968` bytes，SHA-256 `00C1FBBAFFB1A6AB392D9B1DD4D78E1F7B7CDA974051D591316D4577F522C117`。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议 P8.40 增加对称的显式 deactivation composition，并把 activation/deactivation command 接入第一个明确的 product-owned formation lifecycle owner；继续禁止 GameMode 扫描、隐式组件发现与后台轮询。
