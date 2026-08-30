# Dev.D.UE.0.0.10.P8.36.r0 Report

## 1. 结论

P8.36 已让既有 `Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost` 显式消费 P8.35 command delivery，并把 subject/component resolution、durable source receipt 与 native runtime receipt 组成一条可审计证据链。没有新增 controller，没有自动发现组件，也没有保存 UObject pointer。

本阶段结论为 **PASS**：LifecycleCommandHost `5/5`、ProductHost `6/6`、FormationInfluence `85/85`、legacy Attributes `4/4`、`Shanmen.0_0_10` 全量 `334/334`；changed-file regression gate、映射 JSON、自检 `94/94`、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 pointer-free subject resolution

新增 `Fdemo_mapShanmenFormationInfluenceConsumerSubjectResolution`：

- caller 显式提供 subject id 与已解析的 `Udemo_mapAttributeComponent*`；
- resolution 只冻结 subject id、组件进程内 `GetUniqueID()` 与确定性 `ResolutionId`；
- 组件 pointer 仅作为本次调用的瞬时 capability，不进入 Host、delivery、result 或 durable state；
- 每次 Activate 都重新检查组件仍存活、subject 对齐、unique id 与 resolution 完全一致。

### 2.2 delivery-bound application

新增两个显式入口：

- `TryActivateConsumerDelivery(...)`；
- `TryDeactivateConsumerDelivery(...)`。

两者都先按 delivery 的 lifecycle `CommandId` 从本 Host 重新读取 durable source receipt，并验证其确实为成功 Apply、intent/subject/lease 与 delivery 一致。之后才把 delivery 中 frozen Apply/Remove command 交给既有 consumer runtime。

因此调用方不再需要拆出 delivery 后退回裸 command API；实际 Apply/Remove 仍是具名、单次、caller 驱动的操作，不与 command preparation 合并，不自动调度或重试。

### 2.3 自校验 application receipt

`Fdemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationResult` 显式记录：

- 是否检查 source receipt；
- 是否检查 subject resolution；
- Host 中实际读取的 durable source receipt；
- 完整 delivery；
- pointer-free resolution；
- nested consumer runtime result 与 authoritative lease evidence。

成功结果会重新校验 source receipt、delivery、resolution、runtime operation/status 与 authoritative lease；不能仅凭 status 伪造成功。

## 3. 拒绝语义与兼容性

新增精确拒绝状态覆盖：invalid delivery、source receipt missing/rejected、invalid resolution、subject mismatch、component unavailable/mismatch、runtime rejection 与 impossible state。

- parallel Host 即使拿到一份结构有效 delivery，也因没有原 Host 的 durable source receipt 而返回 `SourceReceiptNotFound`；
- 把 delivery 的 lifecycle id 换成同 Host 的 terminal receipt，会返回 `SourceReceiptRejected`；
- 空 resolution、错误 subject、空组件或另一组件均在 native binding 前失败；
- exact activation/deactivation replay 继续使用既有 idempotent consumer transaction；
- authoritative lease 删除后，已完成 deactivation 仍可从同一 delivery 只读 replay。

旧的低层 `TryActivateConsumer` / `TryDeactivateConsumer` 保留用于组合和独立测试。ProductHost/Session、lease executor、consumer runtime 与 `Udemo_mapAttributeComponent` 的权威边界均未改变。

## 4. 修改范围

更新生产代码：

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.cpp`。

更新验证：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- 本 Report 与同名 Development Log。

regression map 已覆盖上述路径，本阶段无需扩展规则，仍为 67 条。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 5. Automation 证据

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationInfluenceLifecycleCommandHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost` | 5 | 0 | 0 | `579EC5ED03F27A9C4C5F87A032F3A5952B350B5A3839690CF0EC650AE22E890A` |
| `FormationProductHost-final.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 6 | 0 | 0 | `A58B42C2A2BEA3BD22E9B43EECB1BE447F032C36A38C4AA463D82BF4080F137D` |
| `FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 85 | 0 | 0 | `F45EBABDC26AB38C8242CDD481934BD1086FCC91EF7E7AEFBE9CF28349D96EDD` |
| `Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `F5ECA2983389E3D72FA7D26A904501EF8846584A8FECBEBD06A7B1E68C94D8C3` |
| `Shanmen-full-final.log` | `Shanmen.0_0_10` | 334 | 0 | 0 | `35E123E85860F141ACFF992BD4BF0A1A6D614D97D5CACC75D6AF5856EBD67325` |

五份最终日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。首次专项命令候选带显式 `Quit`，虽为 `5/5` 且退出 `0`，但缺正式 queue-empty；它未作为证据，移除 `Quit` 后重新生成表中最终日志。

## 6. 专项覆盖

真实 ProductHost 用例新增或强化以下断言：

- correct subject/component 可创建稳定 pointer-free resolution；
- foreign Host 无法消费另一 Host 的 delivery；
- invalid resolution、foreign subject、null component、different component 与 invalid delivery 全部在 binding 前 fail-closed；
- delivery lifecycle id 指向同 Host 的另一 receipt 时被 provenance gate 拒绝；
- exact Activate 成功，重复 Activate 返回 `ActivationReplayed`，binding/application/modifier 数量保持 `1`；
- exact Deactivate 成功，authoritative Remove 后重复 Deactivate 返回 `DeactivationReplayed`；
- P8.34 causal-order fence、P8.35 delivery expiry、terminal failure、forward recovery 与 immutable replay 继续成立。

## 7. Changed-file regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=67
SELF_TEST: PASS 94/94
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=35 Logs=5
```

- mapping SHA-256：`1FC61E3CA0586BC39F637980557D329F33D3B19AEECD0E1E43F93E67CC780B9A`；
- self-test SHA-256：`F11AE76FBDF73AFAD67469D68B0DF47748D4114ADC04F79577328A0A7C3C2A98`；
- `git diff --check` 与 `git diff --cached --check`：PASS。

## 8. 静态边界

两个生产 header/cpp 扫描：`GetSubsystem=0`、`FindComponent=0`、`TActorIterator=0`、`Tick=0`、`while=0`、`TMap=0`、`AActor=0`、GAS symbols `0`、RNG `0`、SaveGame/ProfileRepository `0`、raw object-pointer member `0`。

## 9. 构建与真实异常

构建命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Log SHA-256 |
|---|---|---:|---|
| Editor final | Succeeded / up to date / 0 actions / 0.92s | 0 | `B5794E536DB7F9029345FAFC6D13782FFC8FFC5198BF192225F94C559CBB7D07` |
| Game final | Succeeded / 4 actions / 25.00s | 0 | `67DF44FA919E9E4D40BFEE1E7107598CC4CA3E65D522AF077AF20576B1A617CF` |

- `UnrealEditor-demo_map.dll`：`12106240` bytes，SHA-256 `2CF8A407D4C1DED3BAFCD9B3C530FD25E06CB44D8D55FCE0DA7C81022A382586`；
- `demo_map.exe`：`353145856` bytes，SHA-256 `87ECE6EF9AD59232D8E662925AB477A67EA527851D0D81EF071627F654C6F5E5`。

本阶段没有源码编译失败、UE Automation 失败或内存/环境故障。唯一候选证据问题是首个专项命令的 queue-empty 终止格式，修正命令后闭环。

## 10. P/F 边界与下一步

本 Report 仅包含源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议 P8.37 在 caller-owned World/entity 适配层增加一个显式 resolution factory：由调用方提供 entity registry 查询结果与组件 capability，再生成本阶段 resolution；仍不得由 lifecycle Host 扫描 Actor、查找组件或持有 UObject pointer。
