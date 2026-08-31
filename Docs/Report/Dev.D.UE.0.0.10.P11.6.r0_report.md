# Dev.D.UE.0.0.10.P11.6.r0 Report

## 1. 结论

P11.6 已完成并通过 P 阶段门禁。

本阶段没有直接改写 incoming Impact 或输入链，而是在 P11.5 唯一武器格挡宿主之前补齐产品权威：新增 canonical weapon-guard product config，由 Combat Run 为每次合法尝试分配单调、确定性的 action identity，再把冻结配置、来源武器实例和 caller-owned timeline 组合为一个 active product host。

最终结果：

- 产品配置、Run reservation 与启动组合实现完成；
- 新增 7 个 focused tests，0.0.10 全量达到 496/496；
- 14 组最终 Automation 日志共记录 585 次 Success、0 次 Fail；
- regression mapping 95 rules，自检 150/150；
- changed-file gate：`PASS Changed=7 Rules=2 Required=14 Logs=14`；
- `git diff --check` 与边界扫描通过；
- Editor Development 与 Game Development 单并发构建均首次成功，原生退出码均为 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 Canonical 产品配置

`Fdemo_mapShanmenWeaponGuardProductConfig` 唯一冻结以下产品值：

- content：`0.0.10.P11.6` / `Shanmen.WeaponGuard.ProductConfig.r1`；
- ordinary guard：25%；
- required damage：`Damage.Physical`；
- blocked damage：`Damage.Mental`；
- required target：`Target.Living`；
- perfect window：5 个 caller-owned monotonic ticks；
- minimum facing dot：0.5；
- defense/perfect/arc rules：`Defense.Sword.WeaponGuard01`、`Defense.Sword.PerfectGuard01`、`Defense.Sword.WeaponGuardArc01`。

配置 ID 由全部 canonical 内容按位稳定编码后确定性派生；`IsValid` 同时核对 ID 与每个冻结字段，调用者不能用同一 ID 注入另一组平衡值。

### 2.2 Run-owned action reservation

`Fdemo_mapCombatRunCoordinator::TryReservePlayerWeaponGuardAction`：

1. 要求 ready Combat Run、canonical config 与 exact source item instance ID；
2. 使用玩家 entity、Run ID、guard action definition 和 Run 内单调 sequence 派生 activation ID；
3. 捕获 immutable combat action snapshot；
4. 只有完整 reservation 合法后才推进 sequence；
5. `EndRun` 与 `Reset` 将下一条 guard sequence 恢复为 1。

非法配置、空来源物品、未就绪 Run 或 sequence 耗尽均 fail closed，且不消费 sequence。

### 2.3 Product start composition

`Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart` 按固定顺序：

```text
exact equipped-item identity + caller timeline sample
  -> validate item/timeline/overflow
  -> create canonical product config
  -> reserve deterministic Run action
  -> start P11.5 sole product host
  -> return config + reservation + host proof
```

非法 source item、非法 timeline、负 tick 或 perfect-window 溢出会在 Run sequence 消费前失败。若 reservation 已成功而下游 host 意外拒绝，sequence 保持已消费，避免同一身份被第二次发布。

## 3. 完整性

新增结果同时携带 canonical config、Run reservation、host-start receipt 与 active host；`IsReady` 逐项验证：

- config ID 与 reservation binding；
- action、window 与 host 内 action 完全一致；
- definition、timing、arc 与 canonical 产品策略一致；
- receipt、host ID 与 active lifecycle 一致；
- perfect window 长度和 facing threshold 未被调用者改写。

两个成功尝试产生连续 sequence 和不同 activation/host identity；相同 Run 重置后相同输入可重建相同第一条确定性身份。

## 4. 兼容性与权威边界

- P11.5 仍是唯一 per-action weapon-guard product host；
- P11.0–P11.4 仍分别拥有 window、timing、arc、World identity 与 defense composition；
- Combat Run 只拥有 action identity sequence，不拥有装备选择、输入、时钟或格挡算法；
- source item ID 必须由既有装备／库存权威授权后传入，本阶段不伪造 authorization；
- timeline ID 与 tick 必须由既有单调时间源传入，配置只拥有窗口长度；
- 未接入 incoming Impact、生命值、耐久、资源事务或 Actor discovery；
- 未新增 GameMode、PlayerController、Tick、timer、异步任务或 RNG 路径。

## 5. 修改范围

实现与门禁共 7 个文件、946 行新增、0 行删除：

- `Source/demo_map/demo_mapShanmenWeaponGuardProductAuthority.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductAuthority.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductAuthorityTests.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinator.h`
- `Source/demo_map/demo_mapCombatRunCoordinator.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

加入本 Report/Log 后 exact stage 为 9 个文件。长期未跟踪的 0.0.9B Prompt、Report 与其它历史资料未修改、未暂存、未提交。

## 6. 测试覆盖

新增 focused tests：

1. `CanonicalConfig`：canonical 字段、标签、ID 与稳定重建；
2. `ReservationFences`：未就绪 Run、非法 config/item 与 sequence 围栏；
3. `SequentialReservation`：连续 sequence、不同 identity 与 frozen action；
4. `RunReset`：EndRun/Reset 后 sequence 与确定性身份复位；
5. `TimingFences`：非法 timeline、负 tick、溢出均不消费 sequence；
6. `StartComposition`：config/reservation/window/timing/arc/host 完整绑定；
7. `DistinctAttempts`：连续尝试得到不同 activation 和 host identity。

最终 Automation 证据：

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardProductAuthority` | 7 | 0 | 1 | `9E8E1EA4247344010B8C9AF9666A4F08C474699FB876F97C20516A8D40CC4807` |
| `Shanmen.0_0_10.Product.WeaponGuardProductHost` | 6 | 0 | 1 | `AB908A7043FA722BBF8A0D86987E278EDF3398B8FA277644EA556F7C339503C4` |
| `Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator` | 5 | 0 | 1 | `92EEE776EE8C89B061266C3F948FD38D5521BF381B1DFA3F3151921009FF8E0D` |
| `Shanmen.0_0_10.Product.WeaponGuardWorldAdapter` | 5 | 0 | 1 | `7C0008CF96BA76F10B13A66D100BB04A5CDA1B21679B95DCB69929E4EB319F90` |
| `Shanmen.0_0_10.CombatRuntime.WeaponGuardArc` | 6 | 0 | 1 | `552934CBC2101AFD06A44142CD01CAAC0BB5E4A984B5EFBB8BEA5BC0EF8EA6D9` |
| `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard` | 6 | 0 | 1 | `6F64414F1D28D907605E0FC35D9BAE7B9D07D65EB2D26E5508822F50F21A2CE7` |
| `Shanmen.0_0_10.CombatRuntime.WeaponGuard` | 12 | 0 | 1 | `9BE615BEB3AB724580C5CE7E969BFAD3541165B3386F127FFDEB5D0B2EBB4F20` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | 1 | `71F1535EA49311FB097545714DBF63CE08351C15214711A90BF42A638886807D` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | 1 | `907177794CE9B238E46A225BF24A557FF49639D7D03B85A987777824A49EC9A1` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 17 | 0 | 1 | `7243578A1529ECBFCAD9D93E1C14464E3A0BF53D62212F2DC221DEEA4AF45913` |
| `Shanmen.0_0_10.Product.FormationInfluenceConsumerWorldResolution` | 1 | 0 | 1 | `EB14691F43C2546AADF438F297996369A7EE49BF7F40C6F347B0BCD5590DA6CB` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 1 | `6D150F1644917348896A116B23E96F5AB3511F4A21ED9E6049BBABAEF306AEA0` |
| `demo_map.V3.Attributes` | 4 | 0 | 1 | `7B8B66212C1E8D536DBED1A91C7399E12CEB2B30349E77B84B87D3F1FBCE6B6D` |
| `Shanmen.0_0_10` | 496 | 0 | 1 | `A137F51B0BAB5F8EBE504407D76C11182CB7156AA7737AD353870EDF37BB747F` |

合计 585 Success / 0 Fail；该合计包含 focused 与全量之间的重复覆盖。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=95
SELF_TEST: PASS 150/150
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=14 Logs=14
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`89E1914A0F356FB52196C9B600D5C774F6702F8F86B955FD45EB47A748E0DDCA`；
- self-test SHA-256：`37699B28A9DF6D349E568EAA7E4A6ECF52B0595C4D7947D8123A02F05F4785F6`；
- 新 authority rule 与 coordinator rule 合并要求 14 组日志；
- 边界扫描未发现 item subsystem、GameMode、PlayerController、GetWorld、SpawnActor、ApplyDamage、input、Tick/timer 或 RNG 调用。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development | Succeeded | 61 / 239.03s | 0 | `A01183E991926CE70B80DE3406170961311AED2ED1EE97C769616EB25147812E` |
| Game Development | Succeeded | 60 / 198.79s | 0 | `E02A09F39C9374FD1CB9355217BF6EDFDE23B148C38DCB343D7E00CB77600B4B` |

- `UnrealEditor-demo_map.dll`：12,688,896 bytes，SHA-256 `88FADEF4A5AF25EF059D7940E11EB7816AF050C42DDD6EBFADE635E300D52DF5`；
- `demo_map.exe`：354,236,928 bytes，SHA-256 `97DD75C24AB63F61B9076DD4BE85E1EEBDA6CD2CE01A7436BA37797D8CC64F54`。

## 9. 真实异常

- 本阶段实现后的 focused、依赖组、全量 Automation、Editor 构建与 Game 构建均首次通过；没有源码失败、测试失败、重试构建、Windows commit-memory/pagefile 错误或 Win64 SDK 错误。
- 最终文档复核时，首次从 Windows PowerShell 向 `pwsh -File` 转发 changed-path 数组被展平，validator 以 positional-parameter error 退出 1；改用 PowerShell 7 进程内原生数组调用后得到既定 `PASS Changed=7 Rules=2 Required=14 Logs=14`。未修改 mapping 或 validator 语义。
- UE SDK 检查仍会打印与本目标无关的非 Win64 平台 metadata invalid；Win64 状态有效，所有目标命令原生退出码均为 0。
- LF/CRLF 提示仅是 Git 工作树换行策略提示，`git diff --check` 无错误。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段实现、代码审查、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或 F 阶段产品回归。

下一阶段应由现有 command/equipment authority 授权 exact weapon item，由既有 monotonic timeline 提供 sample，再调用本 authority 创建唯一 host；incoming Impact 只借用 active host 组合 defense。不得把装备授权、输入、时间源、guard policy 或 Impact resolver 搬进 Combat Run coordinator。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-6-weapon-guard-product-authority>
