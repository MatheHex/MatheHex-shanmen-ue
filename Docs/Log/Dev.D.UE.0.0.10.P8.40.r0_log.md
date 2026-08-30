# Dev.D.UE.0.0.10.P8.40.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.40.r0`；
- 基线：`126b96d4d11198e04f95cb9478e0d7c75b7af239`（P8.39）；
- 分支：`agent/0.0.10-p8-40-formation-consumer-deactivation-composition`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

为 P8.39 的显式 formation consumer activation composition 增加对称 deactivation，同时保证 forward teardown 后仍有精确清理原生 modifier 的恢复路径，不新增 authority、World scan、组件发现、轮询或 pointer retention。

## 决策记录

### activation evidence 是最小撤销 capability

P8.39 result 已冻结 Run、delivery、resolution 和 runtime identity。P8.40 直接消费它，而不是再次要求 AttributeComponent 或 World registry。这样清理依赖的是已经提交的生命周期事实，不是仍然存活的发现通道。

### LifecycleCommandHost 保持唯一 owner

既有 Host 已拥有 source receipt、consumer runtime 与幂等 command history。P8.40 不创建 Session 或第二套 ledger，只把 exact delivery 路由回该 Host。

### teardown 顺序可恢复但不放松身份

deactivation 不要求 registry 存活，从而允许 registry forward teardown 后清理；但 foreign Host 必须因缺少 source receipt 而拒绝，且 nested activation/application 的 delivery 与 runtime identity 必须完全一致。

### 回归按改动文件取并集

共享 `demo_mapShanmenFormationProductHostTests.cpp` 匹配多个 product rule，因此继续执行 full 0.0.10，并为 composition、coordinator、World resolution、LifecycleHost、ProductRuntime、ProductBridge、ProductHost、WorldGameplay 与 legacy Attributes 保留独立日志。

## 执行序列

1. 审查 composition、LifecycleHost 与 CombatRun teardown，确认已有 Host 是明确 product-owned owner。
2. 增加 pointer-free deactivation status/result 与严格 `IsSuccess()`。
3. 增加 `TryDeactivate(ProductHost, LifecycleHost, ActivationEvidence)`，只使用冻结 exact delivery。
4. 扩展现有真实 fixture，覆盖无效证据、foreign Host、正确撤销、重放与 Run end。
5. Editor candidate 单并发构建成功。
6. focused `1/1` 后顺序执行九组映射/全量 Automation，全部成功。
7. 执行映射 JSON、自检 `100/100`、changed-file gate、静态扫描和 `git diff --check`。
8. Editor final 与 Game final 单并发构建成功。
9. 生成 Report/Log，执行 exact-stage、staged gate、commit 与 push。

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `ConsumerRunComposition-final.log` | 1 | 0 | yes | 0 | `F1A626C8A472439A32355830A77222173753C0B8BFA70703A8ED3C13766C5287` |
| `CombatRunCoordinator-final.log` | 17 | 0 | yes | 0 | `542FC043C140E5EAA54067048533B078AE4435E16D929EFBF81264CB14340C6A` |
| `ConsumerWorldResolution-final.log` | 1 | 0 | yes | 0 | `12336DB112638C0ABA05D981B6253B55C984D4992102EFAADC198A2DB5B3EFD3` |
| `LifecycleCommandHost-final.log` | 5 | 0 | yes | 0 | `93976D10D11B2B973629562878B3401C6B67E053E3743773335BBEF5247AE269` |
| `ConsumerProductRuntime-final.log` | 4 | 0 | yes | 0 | `C2E54ACDC656EB68CD50119681018FED3089B2BE8B5B197943E6C874B95F1DB2` |
| `ConsumerProductBridge-final.log` | 5 | 0 | yes | 0 | `4B92407D555325EC2D8F466F3A73984D46097CCA05BAE1D3C15A6A479C6A2C62` |
| `FormationProductHost-final.log` | 6 | 0 | yes | 0 | `911F8615A53C98C17DF376A0916FCD11687503A34D58C62573AA7A79A8A440BC` |
| `WorldGameplay-final.log` | 10 | 0 | yes | 0 | `C2588B1472CBD2D4AD52084971EB8650ED9CEF864953BE997A5B4386F3733535` |
| `Attributes-final.log` | 4 | 0 | yes | 0 | `847A26D47A67991BBCBA2DBB4A626542684A4FC8AA2CA9DD11D567A141F5E4BA` |
| `Shanmen-0_0_10-final.log` | 337 | 0 | yes | 0 | `31D5C974566BE9A2567CEAA89196386507173B0B69EE05584FE09DD529D6ADC6` |

最终证据合计 `390` 条 success、`0` fail。

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=69
SELF_TEST: PASS 100/100
REGRESSION_COVERAGE (implementation): PASS Changed=3 Rules=2 Required=38 Logs=10
REGRESSION_COVERAGE (exact staged): PASS Changed=5 Rules=2 Required=38 Logs=10
exact stage: PASS Files=5
git diff --check / git diff --cached --check: PASS
```

- mapping SHA-256：`F64264C9186448E10283D6B43856F6EB525C6BCD49591A6F5306396903E55448`；
- self-test SHA-256：`E2CBB790626C47C56E2F18194FF6EE9F62B992C854057844E3DFAE47EE81975C`；
- 组合头/实现合计 `374` 行；
- World/component discovery、Tick/while、container authority、World/Actor、RNG 扫描均为 `0`；
- result/persistent state raw UObject pointer member 为 `0`。

## 构建证据

| Build | Actions | Time | Exit | SHA-256 |
|---|---:|---:|---:|---|
| Editor candidate | 5 | 36.01s | 0 | `2625C80D086CC2DC608E6B09BDFB751C569F2BC5148C76F99D08A945F23F73AD` |
| Editor final | 0 / up to date | 1.05s | 0 | `609D553B3402B0D149E8B884C2FA09708027CF85292CF1D601E83B9CEF635793` |
| Game final | 4 | 26.24s | 0 | `B0D750BE12ED27C01AC87457EBDC1D279850D418FE16C5BDE3E038FDC8A2CD65` |

- `UnrealEditor-demo_map.dll`：`12143616` bytes，SHA-256 `6463418694F0C3C29D6458A66AD2BC164332FEAA5D1FBB783795CA5961623B2B`；
- `demo_map.exe`：`353177088` bytes，SHA-256 `CA1608820687F578B21FF84972E16D65F9ED3BB5662589A5E01C696513A4232B`。

## 真实异常

没有源码、Automation 或构建失败。第一次 changed-file gate 调用把同目录的 Editor build log 误纳入 Automation evidence，解析器因没有 RunTests/queue-empty 而正确失败关闭。修正仅限证据输入改为十份显式 Automation 白名单；未修改产品代码、测试或 gate 规则。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
