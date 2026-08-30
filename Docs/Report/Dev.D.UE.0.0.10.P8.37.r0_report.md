# Dev.D.UE.0.0.10.P8.37.r0 Report

## 1. 结论

P8.37 已在 caller-owned World/entity 边界落地 registry-backed consumer resolution。调用方必须先把其已经持有的 `Udemo_mapAttributeComponent` capability 显式绑定到现有 `FShanmenWorldEntityRegistry`，再由无状态 resolver 生成 P8.36 的 pointer-free subject resolution。LifecycleCommandHost 不扫描 Actor、不调用 `FindComponent`、不持有 UObject pointer，也没有新增后台 controller。

本阶段结论为 **PASS**：新 resolver `1/1`、LifecycleCommandHost `5/5`、ProductHost `6/6`、WorldGameplay `10/10`、legacy Attributes `4/4`、`Shanmen.0_0_10` 全量 `335/335`；changed-file regression gate、映射 JSON、自检 `96/96`、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 精确 registry capability resolution

新增 `Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(...)`：

- 输入为 caller-owned entity registry、明确 `ExpectedRunId`、caller 已取得的 AttributeComponent 与可选 BodyIndex；
- 精确调用既有 `TryResolveObject(...)`，不从 EntityId 反查 Actor 或组件；
- 只有该组件对象已经存在同 Run、同 body 规则的显式绑定时，才生成 consumer subject resolution；
- generic binding 可服务任意 body，exact-body binding 只接受对应 body；
- resolver 不修改 registry，不执行 Apply/Remove，不保存输入 pointer。

因此 Actor 与其 AttributeComponent 可以按 registry 既有设计分别作为同一 EntityId 的显式别名，但二者关系不会由新代码猜测或自动发现。

### 2.2 pointer-free 审计结果

新增 `Fdemo_mapShanmenFormationInfluenceConsumerWorldResolutionResult`，冻结：

- expected 与 registry RunId；
- registry 是否已检查；
- resolved EntityId 与 BodyIndex；
- P8.36 的 pointer-free `SubjectResolution`；
- 状态与诊断文本。

`IsSuccess()` 会重新校验状态、Run 对齐、EntityId、BodyIndex 与 nested resolution；结果中没有 UObject pointer。

### 2.3 真实 ProductHost 路径

既有 LifecycleCommandHost consumer-fence 用例不再直接把任意 subject id 与组件拼成 resolution。测试现在先把本地与 foreign AttributeComponent 分别绑定到 registry，再通过新 resolver 生成证据，最后调用 `TryActivateConsumerDelivery(...)`。

这条链路同时证明：

1. World registry 确认组件 capability 对应的稳定实体；
2. resolution 冻结 pointer-free identity；
3. delivery application 重新校验 durable lifecycle receipt 与 live component；
4. native consumer runtime 才执行实际 mutation。

## 3. 拒绝语义与兼容性

新增精确状态：invalid expected Run、registry unavailable、registry Run mismatch、component unavailable、invalid body index、entity not found、resolution rejected 与 impossible state。

- 未绑定组件、错误 body、foreign Run 与空组件均 fail-closed，不能铸造 EntityId；
- 相同组件重复解析得到相同 `ResolutionId`，且 registry binding 数量不变；
- foreign subject 使用其自身已绑定组件生成合法 resolution，但在 delivery subject fence 被拒绝；
- 现有 registry、LifecycleCommandHost、consumer runtime 与 AttributeComponent 权威边界未修改；
- P8.36 的低层 `SubjectResolution::TryCreate(...)` 保持兼容，但真实 ProductHost 用例已采用 registry-backed 路径。

## 4. 修改范围

新增生产代码：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerWorldResolution.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerWorldResolution.cpp`。

新增/更新验证：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerWorldResolutionTests.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

回归映射新增 1 条精确规则，规则总数由 67 增至 68；自检新增对应正反例，由 `94/94` 增至 `96/96`。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 5. Automation 证据

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `ConsumerWorldResolution-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerWorldResolution` | 1 | 0 | 0 | `FF70BE1ECE79870E32A63AAB3BBEE8FCA00D6FD75D72D87305810E6888C425DB` |
| `LifecycleCommandHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost` | 5 | 0 | 0 | `47E2DBE4DBE59381CEAF411A06D06693C4CD57BD209262E0957C4B64AF8AB3CE` |
| `FormationProductHost-final.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 6 | 0 | 0 | `08CCF4D6738EFF91AA932C31FCB3EE428EA5500470B1122CECF3148E157586E2` |
| `WorldGameplay-final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `AAE108A8F597C070395BD479BF1101E1910755574A7B20B1D890F45704B134B7` |
| `Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `B33635EE65DA06175384A583C2FFC2BB509AF444B1F15B629E798032910F0350` |
| `Shanmen-full-final.log` | `Shanmen.0_0_10` | 335 | 0 | 0 | `5E55CF1A2DDFACCD26E5A8F28ED83DFF663B2221AD393150DA5A1223BC096A76` |

六份最终日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## 6. 专项覆盖

新增 resolver 用例覆盖：

- invalid expected Run、inactive registry、foreign Run、null component 与 invalid body；
- unbound component 与 wrong exact body 不产生 identity；
- generic binding 生成自校验 pointer-free resolution；
- 重复解析 identity 稳定且不修改 registry；
- exact-body binding 只在正确 body 下成功。

真实 LifecycleCommandHost 用例继续覆盖 source receipt provenance、subject/component mismatch、parallel Host、Activate/Deactivate replay、authoritative Remove fence 与 terminal failure；其 subject resolution 已改由 registry-backed factory 产生。

## 7. Changed-file regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=68
SELF_TEST: PASS 96/96
REGRESSION_COVERAGE (implementation): PASS Changed=6 Rules=2 Required=36 Logs=6
REGRESSION_COVERAGE (exact staged): PASS Changed=8 Rules=2 Required=36 Logs=6
```

- mapping SHA-256：`D72396E917793A06AAE2B40451DEBB21C1E78CCC6564A49A12FA0044030D7C2D`；
- self-test SHA-256：`BF4CADC87E3C1D5FD0C1B938975813921BB55F2C9CC243549F09A49875AE6C4A`；
- exact stage：PASS，只有本阶段 8 个文件；
- `git diff --check` 与 `git diff --cached --check`：PASS。

## 8. 静态边界

两个新增生产 header/cpp 共 `161` 行。扫描结果：`GetSubsystem=0`、`FindComponent=0`、`TActorIterator=0`、`Tick=0`、`while=0`、`TMap=0`、`AActor=0`、`UWorld=0`、GAS symbols `0`、RNG `0`、SaveGame/ProfileRepository `0`、raw object-pointer member `0`。

## 9. 构建与真实异常

构建命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Log SHA-256 |
|---|---|---:|---|
| Editor final | Succeeded / up to date / 0 actions / 0.89s | 0 | `45E0867D85C8A3C33E4682BA06AE2A12B7E4851F2FE71D08B622855B4285F9E7` |
| Game final | Succeeded / 5 actions / 27.82s | 0 | `DA6DC0AD2723B7634B73C06BF4BAF717CEE93581144A405BD70EB899FAD6ED99` |

- `UnrealEditor-demo_map.dll`：`12112896` bytes，SHA-256 `DF7D5DD69766DFF480988AAD2E3CEB0B7AAA011D312CA4AE542C3DFE67D736E5`；
- `demo_map.exe`：`353152512` bytes，SHA-256 `F873F1E044D95C3553965A66F6ADE93B3A16629E54BC290AF93F7CBE2A3A7136`。

本阶段没有源码编译失败、UE Automation 失败或内存/环境故障。验证期间有两次命令包装修正：Windows PowerShell 5.1 无法解析项目 PowerShell 7 的行首管道语法；另一次 `pwsh -File` 会展平数组参数。两者均未进入产品验证结果，改用已配置的 PowerShell 7 直接调用脚本后通过。

## 10. P/F 边界与下一步

本 Report 仅包含源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议 P8.38 先定位第一个真实非测试 caller，把“显式注册组件别名 → registry-backed resolution → delivery application”三步接入该 caller；在存在真实调用点之前不再新增通用 controller 或后台编排层。
