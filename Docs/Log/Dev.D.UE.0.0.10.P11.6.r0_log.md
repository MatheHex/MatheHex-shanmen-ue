# Dev.D.UE.0.0.10.P11.6.r0 Development Log

## 目标

冻结武器格挡产品配置，由 Combat Run 为合法尝试分配单调确定性 action identity，并把 exact source item 与 caller-owned timeline 组合进 P11.5 唯一宿主；不接管装备、输入、时钟或 incoming Impact 权威。

## 基线

- branch：`agent/0.0.10-p11-6-weapon-guard-product-authority`；
- base：`c84c5bf3b966f6024720a152a93695e7f89c83ce`；
- P11.0–P11.4：window、timing、arc、World identity、defense composition；
- P11.5：唯一 per-action weapon-guard product host；
- P 阶段，不启动产品。

## 审计结论

1. P11.5 已有宿主，但 action、definition、timing 和 arc balance 仍需调用者逐项提供；
2. 直接让输入或 incoming route 选择这些值会产生第二套产品配置权威；
3. Combat Run 已拥有玩家 action identity，因此只扩展独立 guard sequence；
4. exact source item 必须来自既有装备／库存授权，本层只验证其 identity；
5. timeline 必须来自既有单调时钟，本层只消费 sample；
6. 所有 caller 输入围栏必须先于 sequence 消费；
7. reservation 成功后的 downstream 失败不得复用 identity；
8. Run 结束／重置必须复位 guard sequence；
9. 不在本阶段接线 input、Actor、Impact、durability 或 resources；
10. 配置 ID 必须覆盖全部 balance/tag/policy 内容。

## 实现

新增：

- `Fdemo_mapShanmenWeaponGuardProductConfig`；
- `Fdemo_mapPlayerWeaponGuardActionReservation`；
- `Fdemo_mapShanmenWeaponGuardProductStartResult`；
- `Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart`；
- `Fdemo_mapCombatRunCoordinator::TryReservePlayerWeaponGuardAction`；
- 独立 `NextPlayerWeaponGuardActivationSequence`。

Canonical 产品值：25% ordinary guard、Physical required、Mental blocked、Living target、5-tick perfect window、0.5 minimum facing dot，以及三条固定 defense/perfect/arc rule。

## 测试

新增 7 个 focused tests：

- `CanonicalConfig`
- `ReservationFences`
- `SequentialReservation`
- `RunReset`
- `TimingFences`
- `StartComposition`
- `DistinctAttempts`

最终证据：

| Log | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `P11.6-Automation-WeaponGuardProductAuthority.log` | 7 | 0 | 1 | `9E8E1EA4247344010B8C9AF9666A4F08C474699FB876F97C20516A8D40CC4807` |
| `P11.6-Automation-Shanmen-0-0-10-Product-WeaponGuardProductHost.log` | 6 | 0 | 1 | `AB908A7043FA722BBF8A0D86987E278EDF3398B8FA277644EA556F7C339503C4` |
| `P11.6-Automation-Shanmen-0-0-10-Product-WeaponGuardDefenseCoordinator.log` | 5 | 0 | 1 | `92EEE776EE8C89B061266C3F948FD38D5521BF381B1DFA3F3151921009FF8E0D` |
| `P11.6-Automation-Shanmen-0-0-10-Product-WeaponGuardWorldAdapter.log` | 5 | 0 | 1 | `7C0008CF96BA76F10B13A66D100BB04A5CDA1B21679B95DCB69929E4EB319F90` |
| `P11.6-Automation-Shanmen-0-0-10-CombatRuntime-WeaponGuardArc.log` | 6 | 0 | 1 | `552934CBC2101AFD06A44142CD01CAAC0BB5E4A984B5EFBB8BEA5BC0EF8EA6D9` |
| `P11.6-Automation-Shanmen-0-0-10-CombatRuntime-WeaponPerfectGuard.log` | 6 | 0 | 1 | `6F64414F1D28D907605E0FC35D9BAE7B9D07D65EB2D26E5508822F50F21A2CE7` |
| `P11.6-Automation-Shanmen-0-0-10-CombatRuntime-WeaponGuard.log` | 12 | 0 | 1 | `9BE615BEB3AB724580C5CE7E969BFAD3541165B3386F127FFDEB5D0B2EBB4F20` |
| `P11.6-Automation-Shanmen-0-0-10-CombatRuntime-ActionLifecycle.log` | 1 | 0 | 1 | `71F1535EA49311FB097545714DBF63CE08351C15214711A90BF42A638886807D` |
| `P11.6-Automation-Shanmen-0-0-10-CombatCore.log` | 9 | 0 | 1 | `907177794CE9B238E46A225BF24A557FF49639D7D03B85A987777824A49EC9A1` |
| `P11.6-Automation-Shanmen-0-0-10-Product-CombatRunCoordinator.log` | 17 | 0 | 1 | `7243578A1529ECBFCAD9D93E1C14464E3A0BF53D62212F2DC221DEEA4AF45913` |
| `P11.6-Automation-Shanmen-0-0-10-Product-FormationInfluenceConsumerWorldResolution.log` | 1 | 0 | 1 | `EB14691F43C2546AADF438F297996369A7EE49BF7F40C6F347B0BCD5590DA6CB` |
| `P11.6-Automation-Shanmen-0-0-10-WorldGameplay.log` | 10 | 0 | 1 | `6D150F1644917348896A116B23E96F5AB3511F4A21ED9E6049BBABAEF306AEA0` |
| `P11.6-Automation-demo-map-V3-Attributes.log` | 4 | 0 | 1 | `7B8B66212C1E8D536DBED1A91C7399E12CEB2B30349E77B84B87D3F1FBCE6B6D` |
| `P11.6-Automation-Shanmen-0-0-10.log` | 496 | 0 | 1 | `A137F51B0BAB5F8EBE504407D76C11182CB7156AA7737AD353870EDF37BB747F` |

总计 585 Success / 0 Fail；包含 focused 与全量的重复覆盖。

## 回归映射

新增 product-authority rule，并扩展 coordinator rule，最终要求 14 个测试组。

```text
REGRESSION_MAP_JSON: PASS Rules=95
SELF_TEST: PASS 150/150
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=14 Logs=14
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`89E1914A0F356FB52196C9B600D5C774F6702F8F86B955FD45EB47A748E0DDCA`；
- self-test SHA-256：`37699B28A9DF6D349E568EAA7E4A6ECF52B0595C4D7947D8123A02F05F4785F6`。

## 构建

| Build | Result | Actions / Time | Exit | SHA-256 |
|---|---|---|---:|---|
| Editor Development | Succeeded | 61 / 239.03s | 0 | `A01183E991926CE70B80DE3406170961311AED2ED1EE97C769616EB25147812E` |
| Game Development | Succeeded | 60 / 198.79s | 0 | `E02A09F39C9374FD1CB9355217BF6EDFDE23B148C38DCB343D7E00CB77600B4B` |

产物：

- `UnrealEditor-demo_map.dll`：12,688,896 bytes，SHA-256 `88FADEF4A5AF25EF059D7940E11EB7816AF050C42DDD6EBFADE635E300D52DF5`；
- `demo_map.exe`：354,236,928 bytes，SHA-256 `97DD75C24AB63F61B9076DD4BE85E1EEBDA6CD2CE01A7436BA37797D8CC64F54`。

## 真实异常

本阶段没有 focused/full 测试失败、源码编译失败、环境内存失败或构建重试。最终文档复核的首次 validator 调用因 Windows PowerShell 向 `pwsh -File` 展平数组而以 positional-parameter error 退出 1；改用 PowerShell 7 进程内原生数组后通过，未改变门禁语义。非 Win64 SDK metadata 提示不影响有效 Win64 目标；两个构建和全部测试命令原生退出码均为 0。

## 修改统计

```text
7 files changed, 946 insertions(+)
```

加入 Report/Log 后 exact stage 为 9 个文件。长期未跟踪资料不进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-6-weapon-guard-product-authority>
