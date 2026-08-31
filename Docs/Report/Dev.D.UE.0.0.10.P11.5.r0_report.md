# Dev.D.UE.0.0.10.P11.5.r0 Report

## 1. 结论

P11.5 已完成并通过 P 阶段门禁。

本阶段没有把 P11.4 直接塞进大型 Combat Run coordinator，而是先补齐审计发现的真实前置缺口：现有 incoming Impact 路径具备 World、registry、attacker/target、candidate 与基础 defense，却没有任何玩家武器格挡动作、窗口、时序和角度策略的产品状态所有者。新增 `Fdemo_mapShanmenWeaponGuardProductHost` 作为唯一值类型宿主，持有一条已经 commit 的 guard action runtime、P11.0 window、P11.1 timing policy、P11.2 arc policy 与单调 observation fence；每次 incoming Impact 只把显式 World/candidate/base defense 交给 P11.4 组合。

最终结果：

- 产品宿主与 6 个 focused tests 编译成功；
- 10 组最终 Automation 日志共记录 549 次 Success、0 次 Fail，其中 0.0.10 全量为 489/489；
- final pre-document regression gate：`PASS Changed=5 Rules=1 Required=10 Logs=10`；
- mapping 94 rules，自检 148/148；
- `git diff --check` 与边界扫描通过；
- Editor Development 与 Game Development 单并发最终构建均为原生退出码 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 唯一启动链

`TryStart` 只接受 canonical weapon-guard action/definition 与显式 authored policy：

1. 复制输入为 frozen values；
2. 通过既有 action orchestrator 执行 `Idle -> Startup -> Active`；
3. 在 exact commit receipt 上打开 P11.0 guard window；
4. 捕获同一 window 的 P11.1 timeline policy；
5. 捕获同一 window 的 P11.2 arc policy；
6. 由 window、timing policy 与 arc policy 生成确定性 host ID；
7. 任一步失败都不发布部分宿主。

`PerfectEndTick` 必须严格晚于 `ActiveStartTick`；非法 timeline、非 guard action、错误 definition 或越界 facing dot 均 fail closed。

### 2.2 Per-impact 组合

`TryComposeDefense` 要求宿主仍处于 Active，并拒绝倒退的 observation tick。合法调用捕获 caller-owned timeline observation，随后把宿主已有 window/runtime/policies 与显式 World、registry、defender/threat Actor、candidate、基础 defense 一次性交给 P11.4。

- Perfect 正面：100 原始伤害 -> 0；
- Ordinary 正面：25% Guard + 10 Armor，100 -> 65；
- 背面：不追加 Guard，只保留 10 Armor，100 -> 90；
- 相同 frozen inputs 重放产生相同 host/defense receipt；
- nested World/identity/layer conflict 不推进 observation fence；
- 只有成功组合才原子更新 `LastObservedTick`。

### 2.3 生命周期

- Active 可进入 Recovery；
- Recovery 后关闭 defense composition；
- Recovery 可完成为 Completed；
- Active/Recovery 可中断为 Interrupted；
- terminal complete/interrupt 重放返回 `AlreadyTerminal`，不制造第二条 transition。

## 3. 完整性

```text
authored guard action + definition + policy inputs
  -> existing action orchestrator commit
  -> P11.0 exact active window
  -> P11.1 exact timing policy
  -> P11.2 exact arc policy
  -> value-type product host + monotonic observation fence

per incoming impact:
explicit World/registry/Actors/candidate/base defense
  -> P11.4 append-only defense composition
  -> deterministic host-bound receipt
```

宿主没有新增 Tick、timer、异步任务、输入绑定、Actor discovery、pointer retention、Impact resolver、生命值写入、资源 prepare/commit/cancel、库存或耐久权威。`UWorld*` 与 `AActor*` 仅是单次同步调用参数，不进入宿主成员。

## 4. 兼容性

- 未修改 P11.0–P11.4、CombatCore、CombatRuntime、WorldGameplay 或现有 Combat Run coordinator；
- P11.0 仍是 window 生命周期权威；
- P11.1 仍是 Perfect/Ordinary 时序互斥权威；
- P11.2 仍是 arc 与 selected layer 权威；
- P11.3 仍是 live Actor/world identity/transform 证据权威；
- P11.4 仍是 append-only defense snapshot 组合权威；
- 宿主只拥有上述权威的 exact bindings 与 action lifecycle，不复制其算法；
- 现有 incoming Impact 路径本阶段未改变，避免在缺少 guard command owner 时引入第二套输入状态。

## 5. 修改范围

实现与门禁共 5 个文件、1496 行新增、0 行删除：

- `Source/demo_map/demo_mapShanmenWeaponGuardProductHost.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductHost.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductHostTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

加入本 Report/Log 后 exact stage 为 7 个文件。长期未跟踪的 0.0.9B Prompt、Report 与其它历史资料未修改、未暂存、未提交。

## 6. 测试覆盖

新增 focused tests：

1. `PerfectReplay`：Perfect 100 -> 0、host/receipt 确定性重放；
2. `OrdinaryOutside`：Ordinary 100 -> 65、背面 100 -> 90；
3. `MonotonicAuthority`：倒退 tick、错误 threat identity 与 null World fail closed，失败不推进 fence；
4. `Lifecycle`：Recovery 关闭组合，Completed/Interrupted terminal 重放幂等；
5. `StartFences`：host identity、policy binding 与非法启动输入围栏；
6. `ConflictIsolation`：同一 selected layer 冲突拒绝且不消耗 observation tick。

最终 Automation 证据：

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardProductHost` | 6 | 0 | 1 | `8F2AF3F6AAFD46B209F803203299D7DAA5972ECE5D9E207ED047CFFE7ED651EE` |
| `Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator` | 5 | 0 | 1 | `B8B84CB81939D10A8EB174A2B02150AE6393A0BD912E787FC1CC6336F4A8EA2B` |
| `Shanmen.0_0_10.Product.WeaponGuardWorldAdapter` | 5 | 0 | 1 | `0D62E933487F140E4BB22F00FE9DE008EAD809D3EB37CC039B34F2770CB1D694` |
| `Shanmen.0_0_10.CombatRuntime.WeaponGuardArc` | 6 | 0 | 1 | `2826C6AAAAB096AE895AD2E5142123D0CAA73011111285EA9524D522C760667F` |
| `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard` | 6 | 0 | 1 | `BD7B24AD3DB9CD75EB12FFFAEF7F60766C11768B1A6E728E371788A8EF316B7E` |
| `Shanmen.0_0_10.CombatRuntime.WeaponGuard` | 12 | 0 | 1 | `369B002C6F414D10A8A6BF4697AC6094C3D92EC78E07046E672020B82E5A5E55` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | 1 | `60876D14C78DDAF67A573F010C6811E16E238C93B24FF8557A39BC3FA60FAA16` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | 1 | `22E37283169FA05865C3C7E9B06B1A00002726F7D7F9208503E7F47DF5EB0D7B` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 1 | `3F78209F988317E138D492571F1F79BC2F9611BBEAB0CAEBBFB118D954CC812E` |
| `Shanmen.0_0_10` | 489 | 0 | 1 | `96D47A4A94DF8720F9B748562BA80DCD9FAC38EEF2A76B1659CD407D56C464D9` |

十组日志合计 549 次 Success / 0 次 Fail；合计包含 focused 与全量之间的重复覆盖。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=94
SELF_TEST: PASS 148/148
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=10
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

- 新 `WeaponGuardProductHost` rule 强制 full、host、P11.4、P11.3、P11.2、P11.1、P11.0、ActionLifecycle、CombatCore 与 WorldGameplay 共 10 组证据；
- mapping SHA-256：`95407BBAE310E2C60C1827554C4C8CF0371327353A09FECFE884A1DB73CF56EB`；
- self-test SHA-256：`824CACAF9E52DFC0B6D54113762D5AEE14EBE4E9B354218ABA05F40277656A70`；
- 边界扫描未发现 Tick callback、timer、input binding、ApplyDamage 或 RNG。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development（最终） | Succeeded | 4 / 5.62s | 0 | `D9D560CF3177E96940D0D14B9691C30DAAE3A9EE428CB663BC28A6015DCD2F52` |
| Game Development（最终） | Succeeded | 6 / 29.71s | 0 | `826F4C3A268F642A10446998F834094A125F1276A36CF851EA8E59D7DC8C11FE` |

- `UnrealEditor-demo_map.dll`：12,652,032 bytes，SHA-256 `A724079A26FF2A02D1239FFAB816C451BDB3447A90460C22C184A21401C2FD52`；
- `demo_map.exe`：354,202,624 bytes，SHA-256 `04ECBF4824B251ADA41AD9A6CE9A5F9C018DAB983C8803187425723E12C9B7F6`。

## 9. 真实异常与修正

1. 首次 focused Automation：原生进程退出码 0，但结果为 5 Success / 1 Fail，外层证据检查退出 1。`ConflictIsolation` 在 tick 10 产生 Perfect layer，却在 tick 11 期待 Ordinary layer 与其发生同 identity 冲突；这是测试夹具选择了两个不同策略结果。把冲突重放修正为同一 tick 10 后最终 6/6。首次失败日志 SHA-256：`6AFBF3E3CAB2D5D4BBA9351E74E37FE3AFBFC73CC9EEDD17A117023C2D588752`。
2. 首次 changed-file gate 通过 `pwsh -File` 从 Windows PowerShell 转发数组，参数被展平并以“positional parameter”退出 1；改用 PowerShell 7 `-Command` 中的原生 literal arrays 后得到 `PASS Changed=5 Rules=1 Required=10 Logs=10`。未放宽 validator 或 mapping。
3. 没有源码编译失败、产品逻辑失败、Windows commit memory/页面文件错误或 Win64 SDK 失败。UE 启动时打印的非 Win64 SDK metadata 不影响 Win64 构建和选定测试。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段实现、代码审查、无头 Automation、静态／路径门禁以及最终 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或 F 阶段产品回归。

建议 P11.6 在现有玩家 guard command owner 与 incoming Impact route 之间建立窄接线：command owner 创建/结束本宿主，incoming route 只借用 active host 组合基础 defense，再构造既有 `FShanmenImpactRequest`。不得让 Run coordinator 自行复制输入状态、窗口、时序、角度、World identity、资源或 resolver 权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-5-weapon-guard-product-host>
