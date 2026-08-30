# Dev.D.UE.0.0.10.P8.40.r0 Report

## 1. 结论

P8.40 已为 P8.39 的 formation consumer Run composition 补齐对称、显式且可重放的 deactivation command。成功 activation 返回的 pointer-free receipt 现在同时是精确撤销 capability；调用方无需重新发现组件，也不要求 CombatRun World registry 在清理时仍然存活。

本阶段结论为 **PASS**：focused composition `1/1`、CombatRunCoordinator `17/17`、World resolution `1/1`、LifecycleCommandHost `5/5`、Consumer ProductRuntime `4/4`、Consumer ProductBridge `5/5`、FormationProductHost `6/6`、WorldGameplay `10/10`、legacy Attributes `4/4`、`Shanmen.0_0_10` 全量 `337/337`；共 `390` 条 success、`0` fail。映射 JSON、自检 `100/100`、changed-file regression gate、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 activation receipt 即撤销 capability

新增 `Fdemo_mapShanmenFormationInfluenceConsumerRunComposition::TryDeactivate(...)`。入口只接受既有 ProductHost、既有 LifecycleHost 和一次成功的 P8.39 activation result，然后把 activation 中冻结的 exact delivery 交回同一个 product-owned lifecycle boundary。

这条路径不接收 live AttributeComponent、不查询 CombatRun coordinator、不读取 World registry。即使上层已先执行 registry 的 forward teardown，原生 modifier 仍能通过 activation receipt 中的 delivery identity 被精确清理。

### 2.2 pointer-free deactivation evidence

新增 deactivation status/result，保留：

- 原 activation 聚合证据；
- 本次 RunId、evidence-check 与 delivery-attempt 阶段位；
- LifecycleHost 返回的完整 application receipt；
- `Deactivated` / `DeactivationReplayed` / 拒绝 / 状态不一致诊断。

`IsSuccess()` 会交叉验证 activation 自证成功、RunId 一致、exact delivery 一致、runtime identity 一致，以及顶层状态与底层 application 状态严格对应。嵌套证据发生任何分歧都会失败关闭为 `StateInvalid`。

### 2.3 精确撤销与幂等重放

专项用例证明：

- 被篡改或失效的 activation receipt 在 delivery 前拒绝；
- foreign LifecycleHost 因没有 source receipt 而拒绝，不修改原 Host runtime 或 AttributeComponent；
- 正确 Host 精确移除 activation 对应的 delivery，并排空原生 modifier/runtime；
- 相同 receipt 再次撤销返回 `DeactivationReplayed`，不增加 completed transaction、不重复修改 runtime；
- consumer 清理完成后 CombatRun 才结束并清空 registry alias。

## 3. 完整性与兼容性

- 保留 P8.39 `TryActivate(...)` API 与全部既有 receipt；
- 继续复用唯一的 `Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost`，未新增 Session、ledger、registry 或 modifier authority；
- deactivation 不依赖已销毁的 World resolution capability，但仍要求 activation receipt 与 Host 中 source receipt/runtime identity 完整匹配；
- 不在 GameMode、Tick 或后台 controller 中加入隐式清理；
- 旧 `demo_map.V3.Attributes` 与完整 0.0.10 契约均通过。

本阶段建立的是可供上层生命周期 owner 显式调用的组合边界，不宣称已经完成产品 UI、PIE 或真实运行时接线。

## 4. 修改范围

生产代码：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRunComposition.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRunComposition.cpp`。

验证与文档：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- 本 Report；
- 同名 Development Log。

源码净变更为 `3` 个文件、`221` insertions、`13` deletions；组合头/实现当前合计 `374` 行。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 5. Automation 证据

| 日志 | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `ConsumerRunComposition-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerRunComposition` | 1 | 0 | `F1A626C8A472439A32355830A77222173753C0B8BFA70703A8ED3C13766C5287` |
| `CombatRunCoordinator-final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 17 | 0 | `542FC043C140E5EAA54067048533B078AE4435E16D929EFBF81264CB14340C6A` |
| `ConsumerWorldResolution-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerWorldResolution` | 1 | 0 | `12336DB112638C0ABA05D981B6253B55C984D4992102EFAADC198A2DB5B3EFD3` |
| `LifecycleCommandHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost` | 5 | 0 | `93976D10D11B2B973629562878B3401C6B67E053E3743773335BBEF5247AE269` |
| `ConsumerProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProductRuntime` | 4 | 0 | `C2E54ACDC656EB68CD50119681018FED3089B2BE8B5B197943E6C874B95F1DB2` |
| `ConsumerProductBridge-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProductBridge` | 5 | 0 | `4B92407D555325EC2D8F466F3A73984D46097CCA05BAE1D3C15A6A479C6A2C62` |
| `FormationProductHost-final.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 6 | 0 | `911F8615A53C98C17DF376A0916FCD11687503A34D58C62573AA7A79A8A440BC` |
| `WorldGameplay-final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | `C2588B1472CBD2D4AD52084971EB8650ED9CEF864953BE997A5B4386F3733535` |
| `Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | `847A26D47A67991BBCBA2DBB4A626542684A4FC8AA2CA9DD11D567A141F5E4BA` |
| `Shanmen-0_0_10-final.log` | `Shanmen.0_0_10` | 337 | 0 | `31D5C974566BE9A2567CEAA89196386507173B0B69EE05584FE09DD529D6ADC6` |

十份日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生进程退出码 `0`。

## 6. Changed-file regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=69
SELF_TEST: PASS 100/100
REGRESSION_COVERAGE (implementation): PASS Changed=3 Rules=2 Required=38 Logs=10
REGRESSION_COVERAGE (exact staged): PASS Changed=5 Rules=2 Required=38 Logs=10
```

ProductHostTests 是共享测试单元，因此匹配规则取并集并要求 `38` 个组。全量 `Shanmen.0_0_10` 覆盖父组，focused 与 legacy 日志保留关键边界的独立证据。

- mapping SHA-256：`F64264C9186448E10283D6B43856F6EB525C6BCD49591A6F5306396903E55448`；
- self-test SHA-256：`E2CBB790626C47C56E2F18194FF6EE9F62B992C854057844E3DFAE47EE81975C`；
- `git diff --check` / `git diff --cached --check`：PASS。

## 7. 静态边界

组合头/实现扫描结果：`FindComponent=0`、`TActorIterator=0`、`GetSubsystem=0`、Tick/while `0`、`TMap=0`、`UWorld=0`、`AActor=0`、RNG `0`。仅有两个既有 activation 输入声明为 `const UObject*`；result 与持久状态中的 raw UObject pointer member 为 `0`。

## 8. 构建证据

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | SHA-256 |
|---|---|---:|---|
| Editor candidate | Succeeded / 5 actions / 36.01s | 0 | `2625C80D086CC2DC608E6B09BDFB751C569F2BC5148C76F99D08A945F23F73AD` |
| Editor final | Succeeded / up to date / 1.05s | 0 | `609D553B3402B0D149E8B884C2FA09708027CF85292CF1D601E83B9CEF635793` |
| Game final | Succeeded / 4 actions / 26.24s | 0 | `B0D750BE12ED27C01AC87457EBDC1D279850D418FE16C5BDE3E038FDC8A2CD65` |

- `UnrealEditor-demo_map.dll`：`12143616` bytes，SHA-256 `6463418694F0C3C29D6458A66AD2BC164332FEAA5D1FBB783795CA5961623B2B`；
- `demo_map.exe`：`353177088` bytes，SHA-256 `CA1608820687F578B21FF84972E16D65F9ED3BB5662589A5E01C696513A4232B`。

## 9. 真实异常与修正

源码、Automation 与构建没有真实失败。changed-file gate 首次调用因证据通配误把 `P8.40-Editor-candidate.log` 当成 Automation 日志而失败关闭；随后改为十份 Automation 日志的显式白名单，未修改或放宽 gate，正式结果通过 `Changed=3 / Rules=2 / Required=38 / Logs=10`。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议 P8.41 审查现有 formation product lifecycle 的真实调用点，将 P8.39/P8.40 的显式 activate/deactivate command 接入一个已有 owner；若不存在明确 owner，则继续保持 capability 边界，不以 GameMode 扫描或轮询 controller 代替所有权设计。
