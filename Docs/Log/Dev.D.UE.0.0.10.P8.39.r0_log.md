# Dev.D.UE.0.0.10.P8.39.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.39.r0`；
- 基线：`99eacedfe010a17b39080444ddd5143ccce01e15`（P8.38）；
- 分支：`agent/0.0.10-p8-39-formation-consumer-composition-root`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

在不增加 GameMode glue、World scan、组件发现、后台 controller 或 UObject pointer retention 的前提下，建立首个真实 formation consumer composition command，将 P8.38 CombatRun alias、P8.37 World resolution 与 P8.36 LifecycleHost native delivery 顺序闭合。

## 决策记录

### composition 是无状态命令，不是新 authority

新类型只编排三个既有权威，不复制 registry、consumer runtime、delivery ledger 或 AttributeComponent 状态。所有 capability 由调用方显式提供，命令执行一次后只返回 pointer-free evidence。

### EntityId 仍不可由调用方注入

composition 接受 registered subject 与 AttributeComponent，不接受 EntityId。CombatRunCoordinator 从已注册 subject 推导 identity 并建立 alias；resolver 再从 coordinator 的 const registry view 反向复核 alias。

### alias 是 forward-only identity fact

alias 提交成功后不因 downstream Host 拒绝而回滚。这样正确的 Run identity 不依赖某个 delivery application 是否成功，也允许 caller 用正确 LifecycleHost 重试。downstream 失败仍严格保证 consumer runtime 与 native modifier 无副作用。

### result 聚合但不隐藏 nested receipt

顶层 result 不把三段状态压成一个 bool。alias、World resolution 与 application receipt 全部保留，`IsSuccess()` 负责跨层一致性检查，便于报告、回放与故障定位。

### 回归按改动路径映射

新增 composition 规则要求 focused composition、CombatRunCoordinator、World resolution、LifecycleCommandHost、FormationProductHost、WorldGameplay、legacy Attributes 与全量 0.0.10。ProductHostTests 作为共享文件还匹配既有 product 规则，因此 gate 对全部 required groups 取并集。

## 执行序列

1. 审查 P8.36–P8.38 的 Host、resolver 与 CombatRun seams，确认最小缺口是显式单次组合，而不是新增 GameMode/controller。
2. 新建 pointer-free composition status/result 与 `TryActivate(...)`。
3. 顺序接入 `TryBindEntityAlias`、registry-backed `Resolve`、`TryActivateConsumerDelivery`。
4. 为成功结果增加 RunId、EntityId、UniqueId、resolution id 与底层状态交叉验证。
5. 新增真实 ProductHost/CombatRun/Player/AttributeComponent fixture，覆盖前置拒绝、foreign Host、激活、重放、deactivation 与 Run teardown。
6. 首次 Editor 构建暴露 incomplete-type 转换错误；补充 AttributeComponent 完整 include。
7. 首次 focused 用例暴露 abstract UObject fixture 错误；替换为 concrete SceneComponent。
8. focused retry `1/1` 后，收紧顶层 success-status 映射并完成 Editor final。
9. 生成八组最终 Automation；Attributes 前两份异步日志证据不完整，使用 `-forcelogflush` 复跑取得完整 `4/4`。
10. 执行 implementation changed-file gate、映射 JSON、自检 `100/100`、局部静态扫描与 `git diff --check`。
11. Game Development 单并发构建成功。
12. 生成 Report/Log，执行 exact-stage、staged gate、commit 与 push。

## Automation 证据

| Log | Success | Fail | Queue | Fatal | Exit | SHA-256 |
|---|---:|---:|---:|---:|---:|---|
| `ConsumerRunComposition-final.log` | 1 | 0 | yes | 0 | 0 | `FB8E2611F4CB3B2AE19BB69BE9CE35AC02A8FC2FEB05945F47165290AFA79750` |
| `CombatRunCoordinator-final.log` | 17 | 0 | yes | 0 | 0 | `FBFDF255F88AFCC6A2F7BBA6F01EF7DDE306CA7DB49FD130FD6C58EDE5F85695` |
| `ConsumerWorldResolution-final.log` | 1 | 0 | yes | 0 | 0 | `70583F4CF369677220EA04A41DC5917C68015186924D4932D9A557A006BF9910` |
| `LifecycleCommandHost-final.log` | 5 | 0 | yes | 0 | 0 | `1A150903B245519BC8E1C207FB833305C9A2CE173458A28765ED7EB07BEF4C79` |
| `FormationProductHost-final.log` | 6 | 0 | yes | 0 | 0 | `10A8E2314842A89B10033E21962588E56635604D4951ED3092747448FB73E025` |
| `WorldGameplay-final.log` | 10 | 0 | yes | 0 | 0 | `767059FBC02A9453FC122787F4507DE5306344E6EB4F77E04DA07AB0DFD89EC7` |
| `Attributes-final2.log` | 4 | 0 | yes | 0 | 0 | `AFD735D3A8ED8A856223470EA85B9244AF54F117E634C3231731AF32784C44F6` |
| `Shanmen-full-final.log` | 337 | 0 | yes | 0 | 0 | `2FDB97A9A50D4CC45797594F897DBEDF0B7F97D83B8224DA42A9ABA528183259` |

最终证据合计 `381` 条 success、`0` fail。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=69
SELF_TEST: PASS 100/100
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=2 Required=38 Logs=8
REGRESSION_COVERAGE (exact staged): PASS Changed=7 Rules=2 Required=38 Logs=8
exact stage: PASS Files=7
git diff --check / git diff --cached --check: PASS
```

- mapping SHA-256：`F64264C9186448E10283D6B43856F6EB525C6BCD49591A6F5306396903E55448`；
- self-test SHA-256：`E2CBB790626C47C56E2F18194FF6EE9F62B992C854057844E3DFAE47EE81975C`；
- 新生产组合文件共 `222` 行；
- `FindComponent`、Actor iteration、subsystem lookup、Tick/while、TMap、UWorld、AActor 与 RNG 扫描均为 `0`；
- result raw UObject pointer member 为 `0`。

## 构建证据

| Build | Actions | Time | Exit | SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | stopped on `C2664` | 31.15s | 6 | `13D65E66AD88BCC340DDFD13EA1D78063A83ACF8E5F3D16E958706424C9ABEC6` |
| Editor retry | 4 | 6.40s | 0 | `D15DBFDDB8CAA3D5039D96400EFECB726401CB19D93CF66EFCE4BD7C4205FD33` |
| Editor fixture fix | 4 | 7.28s | 0 | `E449EDC6FEA3FBA4917BA1BC49191FA76CF2E6E882D3AB8C086850E2564FC823` |
| Editor final | 4 | 5.88s | 0 | `1B7BB83C9F361A814BF9D70EA61754105551888351CD5B8CD2EA6C6FBC3DCC32` |
| Game final | 4 | 25.67s | 0 | `723DD09F39C121F333B24794D2FF8C21827E669AFD466F51A57F013179DF7584` |

- `UnrealEditor-demo_map.dll`：`12137472` bytes，SHA-256 `CE44A80A2D326DB01F71E27E5BE6BDFAB4D55C812A320EC273EF93B6613ECC1D`；
- `demo_map.exe`：`353171968` bytes，SHA-256 `00C1FBBAFFB1A6AB392D9B1DD4D78E1F7B7CDA974051D591316D4577F522C117`。

## 首次失败与证据修正

### Editor source failure

`P8.39-Editor-candidate.log` 原生退出码 `6`，`OtherCompilationError`。直接原因是 composition cpp 只看见 `Udemo_mapAttributeComponent` 前置声明，无法把 pointer 转为 `const UObject*`。加入 `demo_mapAttributeComponent.h` 后 Editor retry 成功。未描述为环境/内存错误。

### Focused fixture failure

`ConsumerRunComposition-candidate.log` 的唯一用例失败并触发 handled ensure：测试尝试 `NewObject<UObject>()`，而 `UObject` 在该路径被视为 abstract。替换为 concrete、未注册的 `USceneComponent` 后，测试保持相同拒绝语义并通过。失败日志 SHA-256：`6354599C456002B0AD4829EA3DFB40B88D97C098763E23C08E9738681C62CB12`。

### Attributes 日志落盘不完整

`Attributes-final.log` 与 `Attributes-retry.log` 的原生进程退出码均为 `0`，但日志都只有前三条 success，停在收到 `ModifierSemantics`，没有最终结果与 queue-empty。它们按失败关闭规则不作为通过证据：

- `Attributes-final.log`：`4C31A728948E1171B6592F90BFDE72A0367FB0DD36406003842789B6910EDA9B`；
- `Attributes-retry.log`：`6C458C56C94A13B2734FCA865EEE937B206A7D69A2D78100358EE65C6E2955AB`。

单独运行 `ModifierSemantics` 为 `1/1` success；在完整组命令增加 `-forcelogflush` 后，`Attributes-final2.log` 完整记录 `4/4`、queue-empty 与 exit `0`。判断为异步日志退出竞态，不修改 Attributes 产品代码或断言。

一次 `Start-Process` 诊断因 PowerShell 参数拆分使 `-TestExit` 仅收到 `Automation`，在测试发现前提前退出；该日志不属于产品测试证据并已排除。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
