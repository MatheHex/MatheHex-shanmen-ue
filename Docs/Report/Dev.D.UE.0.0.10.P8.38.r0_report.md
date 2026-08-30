# Dev.D.UE.0.0.10.P8.38.r0 Report

## 1. 结论

P8.38 已把 P8.37 的 World/entity capability resolution 接入真实 `Fdemo_mapCombatRunCoordinator` 权威边界。新增的显式 alias 事务只能从一个已经注册的 caller-owned UObject 推导 EntityId，再把另一 caller-owned UObject 绑定为同一实体的别名；调用方不能注入 EntityId，Coordinator 也没有暴露可变 registry。

本阶段结论为 **PASS**：CombatRunCoordinator `17/17`、formation consumer World resolution `1/1`、WorldGameplay `10/10`、legacy Attributes `4/4`、`Shanmen.0_0_10` 全量 `336/336`；changed-file regression gate、映射 JSON、自检 `98/98`、事务局部静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 显式 Entity alias 事务

`Fdemo_mapCombatRunCoordinator::TryBindEntityAlias(...)` 接受：

- 已经存在于当前 Run registry 的 source object 与可选 body；
- 调用方已经持有的 alias object 与可选 body。

事务先通过现有 `TryResolveObject(...)` 从 source object 推导稳定 EntityId，然后在 registry 副本上执行 `BindObject(...)`、反向复核 alias 是否解析回同一 EntityId，最后才提交副本。API 没有 EntityId 输入参数，因此不能由调用方伪造主体身份。

### 2.2 copy-on-success 与幂等重放

- 新 binding 成功时返回 `Bound` 并原子替换 registry；
- exact alias 已存在时返回 `AlreadyBound`，registry 数量不变；
- alias 已属于另一实体时返回 `AliasConflict`，原 registry 不变；
- prepared registry 无法自证时返回 `StateInvalid`，不会提交部分状态。

generic alias 继续沿用 registry 的任意 body 语义；exact-body alias 只允许对应 body。Run 结束仍由现有 registry reset 一次性清理全部派生 alias。

### 2.3 pointer-free receipt 与下游接续

新增 `Fdemo_mapCombatRunEntityAliasResult`，只冻结状态、诊断、RunId、EntityId、对象 UniqueId、body、binding 结果、计数和验证位，不保存 UObject pointer。`IsSuccess()` 会复核状态、身份、计数变化与提交语义。

专项用例证明 Coordinator 创建的 AttributeComponent alias 可以直接供 P8.37 `Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver` 使用，生成同一 player EntityId 的 pointer-free formation consumer resolution。

## 3. 拒绝语义与兼容性

新增精确状态：Coordinator 未就绪、source object 不可用、alias object 不可用、body 非法、source 未注册、alias 冲突、registry 拒绝与内部状态不一致。

- 无效请求在 registry mutation 前失败；
- source 必须已属于当前 Run，不能跨 Run 或凭空铸造 EntityId；
- alias conflict 在副本中发现，正式 registry 的 binding 数量与内容不变；
- exact-body alias 的错误 body 无法通过 P8.37 resolver；
- 既有 `GetEntityRegistry()` 仍为 const，只增加窄化的事务入口；
- 既有 M01 enemy、player、weapon 与 Run lifecycle 行为保持兼容。

## 4. 修改范围

生产代码：

- `Source/demo_map/demo_mapCombatRunCoordinator.h`；
- `Source/demo_map/demo_mapCombatRunCoordinator.cpp`。

验证与流程：

- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

实现差异为 `343 insertions / 1 deletion`。回归映射规则总数保持 `68`，但 CombatRunCoordinator 规则由单一 focused group 扩展为 Coordinator、formation resolver、WorldGameplay、legacy Attributes 与全量 0.0.10 五组证据；自检由 `96/96` 增至 `98/98`。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 5. Automation 证据

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `CombatRunCoordinator-final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 17 | 0 | 0 | `E462875F882EE46AC50D9C7F7A1C03E9280D1AA01D976B6C7E3007CA22F72AAB` |
| `ConsumerWorldResolution-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerWorldResolution` | 1 | 0 | 0 | `AB8D31201A35182E719F81A069D449F574E622F7B739D5DC45D983A3410E6637` |
| `WorldGameplay-final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `7267D64C5AC52A843D489D54C542876D0DD716BBB187D712701BF17F83549E32` |
| `Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `1C5392763306BB724F7F74B2995A758FDE3871E62E3103D7E8CDD126FD4FE6C5` |
| `Shanmen-full-final.log` | `Shanmen.0_0_10` | 336 | 0 | 0 | `FC763B83C5A0BE726BBE335DFA63DB090886D13A6862024251FA86DCEC4DBDE4` |

五份最终日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## 6. 专项覆盖

新增 `EntityAliasBinding` 用例覆盖：

- inactive Coordinator、null source、null alias、非法 body 与未注册 source fail-closed；
- player source 推导 AttributeComponent generic alias；
- exact alias 重放只读且 identity 稳定；
- exact-body alias 的正确/错误 body 分流；
- Coordinator registry alias 进入 formation consumer World resolver；
- 已注册 M01 enemy 作为 foreign alias 时冲突且无副作用；
- Run end 后 alias 不再可解析。

## 7. Changed-file regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=68
SELF_TEST: PASS 98/98
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=1 Required=5 Logs=5
REGRESSION_COVERAGE (exact staged): PASS Changed=7 Rules=1 Required=5 Logs=5
```

- mapping SHA-256：`DE61BD48F18F4A4E3F70FD1C7B7F73EEEE0DA7FFF8BF66656F873CFB4F187F97`；
- self-test SHA-256：`8892802D8BAD35100CF3AE4C4A2601C061867475249E8154FDD8ADDEA85A996A`；
- exact stage：PASS，只有本阶段 7 个文件；
- `git diff --check` 与 `git diff --cached --check`：PASS。

## 8. 静态边界

扫描严格限定到新增 alias 事务块（`118` 行）与 pointer-free receipt（`21` 行），避免把既有 Actor-facing CombatRunCoordinator 代码误报为本阶段新增依赖。

- 事务块：`FindComponent=0`、`TActorIterator=0`、`GetSubsystem=0`、Tick/while `0`、`TMap=0`、`UWorld=0`、`AActor=0`、RNG `0`；
- receipt：raw UObject pointer member `0`；
- 事务块中仅有两个预期的 `const UObject*` caller-owned 输入参数，不保存它们；
- 没有新增扫描、反向 EntityId→Actor 表、后台 controller 或第二套 registry。

## 9. 构建与真实异常

构建命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Log SHA-256 |
|---|---|---:|---|
| Editor candidate | Succeeded / 49 actions / 180.63s | 0 | `303C2EFECA35FB8C790CEB544C4DF6C999DAC2B2DD1F2122069F1F486842FDB4` |
| Editor final | Succeeded / up to date / 0 actions / 1.11s | 0 | `0F2E3507B96D84E57A50CF043894FC218C0D2142F3376158CC4D1C9AB7AA12E1` |
| Game final | Succeeded / 48 actions / 151.83s | 0 | `672426FB1F45E3E9C985CC451E33386C913106DC0B80EDC141D27B74DB7ECD7B` |

- `UnrealEditor-demo_map.dll`：`12123136` bytes，SHA-256 `917BC3A8C688FC2C56C50401890B38EED9F80FF76E6BFA7E569A5EC25137F276`；
- `demo_map.exe`：`353161216` bytes，SHA-256 `FB342855689993E71A14B1B884D6C14E1AB1119F8A2800CC83483A6B72819226`。

本阶段没有源码编译失败、UE Automation 失败或内存/环境故障。最终回归首次 launcher 因包含不必要的预删除日志步骤而被本机执行策略在 UE 启动前拒绝；去掉删除后五组一次通过。一次静态扫描命令因 PowerShell 管道语法包装错误在解析阶段停止，修正包装后扫描通过。两者都没有执行或改变产品验证结果。

## 10. P/F 边界与下一步

本 Report 仅包含源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议 P8.39 定位或引入第一个明确的 formation composition root，让它顺序调用“CombatRunCoordinator 显式 alias → P8.37 registry-backed resolution → P8.36 delivery application”；仍不引入后台扫描或通用 controller。
