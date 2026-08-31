# Dev.D.UE.0.0.10.P11.5.r0 Development Log

## 目标

建立一条武器格挡产品状态的唯一值类型宿主：一次性绑定已 commit 的 action、P11.0 window、P11.1 timing、P11.2 arc，并为每次 incoming Impact 调用 P11.4；不让大型 Run coordinator 临时制造第二套 guard 状态。

## 基线

- branch：`agent/0.0.10-p11-5-weapon-guard-product-host`；
- base：`20bd18f39aabd318a1cff50acaf8c9f3e5baf632`；
- P11.0：Active-only guard window/layer；
- P11.1：Perfect/Ordinary timing projection；
- P11.2：guard arc 与 selected layer；
- P11.3：live Actor/World/entity/transform adapter；
- P11.4：append-only defense snapshot coordinator；
- P 阶段，不启动产品。

## 审计结论

1. 现有 incoming enemy Impact route 已有 World、registry、source enemy、target player、candidate 与 resource-adjusted base defense；
2. 该 route 没有玩家 guard action/window/timeline/arc policy owner；
3. 直接在 Run coordinator 中临时构造这些状态会复制输入与生命周期权威；
4. 因此本阶段先冻结 product host，下一阶段再让 command owner 与 Impact route 借用它；
5. host 只保存 value state，不保存 World/Actor/component 指针；
6. observation fence 由 host 单调推进，nested 失败不得消耗 tick；
7. lifecycle transition 必须复用现有 action orchestrator；
8. per-impact composition 必须复用 P11.4；
9. resource、durability、Impact 与 vitality 仍留在既有权威；
10. terminal 重放必须幂等且不生成第二个 receipt。

## 实现

新增：

- `Fdemo_mapShanmenWeaponGuardProductHost`；
- start/defense/transition 三组 typed status 与 error；
- `TryStart`、`TryComposeDefense`、`TryEnterRecovery`、`TryComplete`、`TryInterrupt`；
- host namespace：`demo_map.Sword.WeaponGuard.ProductHost.r1`；
- defense receipt namespace：`demo_map.Sword.WeaponGuard.ProductHost.Defense.r1`。

宿主成员只有：

- deterministic `HostId`；
- monotonic `LastObservedTick`；
- existing action orchestrator；
- P11.0 window；
- P11.1 timing policy；
- P11.2 arc policy；
- started flag。

## 测试

新增 6 个 focused tests：

- `PerfectReplay`
- `OrdinaryOutside`
- `MonotonicAuthority`
- `Lifecycle`
- `StartFences`
- `ConflictIsolation`

最终证据：

| Log | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `P11.5-Automation-WeaponGuardProductHost.log` | 6 | 0 | 1 | `8F2AF3F6AAFD46B209F803203299D7DAA5972ECE5D9E207ED047CFFE7ED651EE` |
| `P11.5-Automation-WeaponGuardDefenseCoordinator.log` | 5 | 0 | 1 | `B8B84CB81939D10A8EB174A2B02150AE6393A0BD912E787FC1CC6336F4A8EA2B` |
| `P11.5-Automation-WeaponGuardWorldAdapter.log` | 5 | 0 | 1 | `0D62E933487F140E4BB22F00FE9DE008EAD809D3EB37CC039B34F2770CB1D694` |
| `P11.5-Automation-WeaponGuardArc.log` | 6 | 0 | 1 | `2826C6AAAAB096AE895AD2E5142123D0CAA73011111285EA9524D522C760667F` |
| `P11.5-Automation-WeaponPerfectGuard.log` | 6 | 0 | 1 | `BD7B24AD3DB9CD75EB12FFFAEF7F60766C11768B1A6E728E371788A8EF316B7E` |
| `P11.5-Automation-WeaponGuard.log` | 12 | 0 | 1 | `369B002C6F414D10A8A6BF4697AC6094C3D92EC78E07046E672020B82E5A5E55` |
| `P11.5-Automation-ActionLifecycle.log` | 1 | 0 | 1 | `60876D14C78DDAF67A573F010C6811E16E238C93B24FF8557A39BC3FA60FAA16` |
| `P11.5-Automation-CombatCore.log` | 9 | 0 | 1 | `22E37283169FA05865C3C7E9B06B1A00002726F7D7F9208503E7F47DF5EB0D7B` |
| `P11.5-Automation-WorldGameplay.log` | 10 | 0 | 1 | `3F78209F988317E138D492571F1F79BC2F9611BBEAB0CAEBBFB118D954CC812E` |
| `P11.5-Automation-Full.log` | 489 | 0 | 1 | `96D47A4A94DF8720F9B748562BA80DCD9FAC38EEF2A76B1659CD407D56C464D9` |

总计 549 Success / 0 Fail；包含 focused 与全量的重复覆盖。

## 回归映射

新增 `WeaponGuardProductHost` rule，要求：

- `Shanmen.0_0_10`
- `Shanmen.0_0_10.Product.WeaponGuardProductHost`
- `Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator`
- `Shanmen.0_0_10.Product.WeaponGuardWorldAdapter`
- `Shanmen.0_0_10.CombatRuntime.WeaponGuardArc`
- `Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard`
- `Shanmen.0_0_10.CombatRuntime.WeaponGuard`
- `Shanmen.0_0_10.CombatRuntime.ActionLifecycle`
- `Shanmen.0_0_10.CombatCore`
- `Shanmen.0_0_10.WorldGameplay`

结果：

```text
REGRESSION_MAP_JSON: PASS Rules=94
SELF_TEST: PASS 148/148
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=10
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

## 构建

| Build | Result | Actions / Time | Exit | SHA-256 |
|---|---|---|---:|---|
| Editor final | Succeeded | 4 / 5.62s | 0 | `D9D560CF3177E96940D0D14B9691C30DAAE3A9EE428CB663BC28A6015DCD2F52` |
| Game final | Succeeded | 6 / 29.71s | 0 | `826F4C3A268F642A10446998F834094A125F1276A36CF851EA8E59D7DC8C11FE` |

## 真实异常

1. 首次 focused：5 Success / 1 Fail，原生退出 0、外层证据检查退出 1。测试用 tick 10 Perfect 结果与 tick 11 Ordinary 结果构造“同 layer identity 冲突”，预期本身不成立；改为同 tick 重放后最终 6/6。失败日志 SHA-256 `6AFBF3E3CAB2D5D4BBA9351E74E37FE3AFBFC73CC9EEDD17A117023C2D588752`。
2. 首次 changed-file gate 因 Windows PowerShell 到 `pwsh -File` 的数组参数展平而退出 1；改用 PowerShell 7 literal arrays 后通过。没有修改 validator 语义。
3. 没有编译、产品逻辑、内存或 Win64 SDK 失败。

## 修改统计

```text
5 files changed, 1496 insertions(+)
```

加入 Report/Log 后 exact stage 为 7 个文件。长期未跟踪资料不进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-5-weapon-guard-product-host>
