# Dev.D.UE.0.0.10.P6.5.r0 Report

## 1. 结论

P6.5 已完成受控武器的多实例 Run Host，结论为 **PASS**。

`Fdemo_mapShanmenControlledWeaponRunHost` 现在以 exact `ItemInstanceId` 管理多把 P6.4 物理飞剑。每把飞剑继续独占自己的 Controller、Actor、Session、command sequence、接触窗口、impact ledger 与终态；宿主只提供 Run 绑定、唯一性栅栏、稳定批处理顺序和统一中断，不引入新的物品、伤害或生命权威。

本轮没有冻结正式按键、飞剑 Actor class、spawn、编队、数量上限、默认速度或内容资产。

## 2. 功能性

- 首把飞剑通过既有 P6.4 `TryStart` 建立 Run / source 绑定；后续飞剑必须匹配同一 ready Coordinator、Run、player entity 与已注册 source Actor；
- 每个 `ItemInstanceId`、`ActivationId`、physical weapon Actor 和 collision root 在宿主内均唯一；任何重复绑定在写入前失败关闭；
- `GetOrderedItemInstanceIds` 使用稳定 GUID digits 顺序，不依赖 `TMap` 迭代、附加顺序或对象地址；
- Launch、Redirect、contact window、sweep / overlap、Recall 与单项 Interrupt 均按 exact item 路由，不合并不同飞剑的状态；
- 批量 movement 先验证所有 Directed 飞剑的时间片，再按稳定 item 顺序调用 P6.4 swept movement；过大时间片在任一 Actor 移动前整体拒绝；
- 世界移动属于可观测物理副作用，合法批次若个别 Actor 被世界阻挡，则逐项 receipt 保留真实结果，不伪造事务回滚；
- sweep / overlap 在宿主层再次核验 Coordinator、Run、player entity 与注册 source Actor，然后仍只经 P6.3 Adapter 和 Coordinator 提交 canonical vitality；
- `TryInterruptAll` 在宿主副本上按稳定顺序中断全部 active item，全部成功才提交宿主状态；已完成的 item 不被重复中断；
- terminal item 可独立移除，最后一项移除后清空宿主 Run 绑定。

## 3. 完整性

新增 3 个产品级自动化：

1. `StableIdentityAndOrdering`：反序附加两把飞剑，验证稳定 item 顺序、独立方向、批量 swept movement 以及 oversized step 零位移；
2. `IndependentContactAndLifecycle`：两把飞剑分别通过 sweep / overlap 命中同一目标，验证不同 item / impact identity、canonical vitality 合计、单项重复回调幂等、单项完成与剩余项统一中断；
3. `AttachFencesAndAtomicInterrupt`：验证 item、Actor / collision root、activation 三类重复绑定拒绝，失败后原宿主不变，以及统一中断的原子提交和 terminal retirement。

全量 `Shanmen.0_0_10` 从 P6.4 的 `143` 个增加到 `146` 个，最终 `146/146` Success。

## 4. 兼容性

- 未修改 P6.1 Adapter、P6.2 Session、P6.3 World Adapter 或 P6.4 Controller 的既有生产源码；
- 未修改 CombatCore、CombatRuntime、WorldGameplay、Items、Profile schema、存档格式、物品定义或输入映射；
- 未新增 Actor class、spawn 路径、PlayerController 分支、inventory 副本或 vitality 写入路径；
- 新宿主不调用 `ApplyDamage` / `TakeDamage`，不直接修改库存或资源；
- Run Host 只组合已验证的单飞剑产品边界，不把 legacy projectile 当作受控飞剑；
- 宿主销毁或 Run 结束时，外层产品 owner 仍须显式调用统一 Interrupt / terminal retirement；本轮不臆造该 owner 的生命周期位置。

## 5. 修改范围

- `Source/demo_map/demo_mapShanmenControlledWeaponRunHost.h/.cpp`；
- `Source/demo_map/demo_mapShanmenControlledWeaponRunHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Log。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未修改、未暂存、未提交。

## 6. 自动化与静态检查

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 3 | 0 | 0 | `89A63080A4E011A8DF01B73210FD35C49374076863221473E924B5BBC75D35E0` |
| `Shanmen.0_0_10.Product.ControlledWeaponController` | 3 | 0 | 0 | `52AA89850B65663A19DC86FC0DCCC90290C3EF80256A439A4B3B73318DCFC005` |
| `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `4F0628F11423CD83F852952BA638EA7C29AB8890E58F0EB61D8BE2F55F7D4B82` |
| `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `A54DD52EC44368605DBC95A11DC7E215156032F6F0FE3806B597D066B1A450F3` |
| `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `AF7BEFA22BCA9EE60725773AEA6268C8046096D6CADFB465659B17EC3FDC79FB` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `753FE56F6A73F4A2CD72C5915BB4A1935C4F80EF02472D80FF35CAFB85C04660` |
| `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `A45387B2ED7612CE88F313527217EEF9D6D4CC9EF311933C12D59A016D89E3ED` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `26E01BA553B74B71B8A0AEA52069155704DBB294B37FA2F2FF23668C647A6D38` |
| `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `C294D1484C8C38766A81C1FDA3900EA82359FD0D3BFD0C4D87E0DBCE807934D4` |
| `Shanmen.0_0_10` | 146 | 0 | 0 | `785D9E2AFE490E356620EB44C3E6981F5B14235CD272B50912B72BC32F5C615A` |

- 十条最终日志均有唯一 RunTests command、queue-empty、fail 0，Fatal / unhandled / ensure 0；
- changed-file gate：`PASS Changed=7 Rules=1 Required=9 Logs=10`；
- regression coverage self-test：`10/10 PASS`；
- regression JSON parse：PASS；
- 新宿主生产文件边界扫描：`ApplyDamage` / `TakeDamage` / `SpawnActor` / input / inventory / resource / RNG / `UWorld` 命中 `0`；
- `git diff --check` 与最终 `git diff --cached --check`：native exit `0`。

## 7. 首次验证与修正

首次 Editor integration build 为 `5/5` actions、`Result: Succeeded`、native exit `0`、`11.35s`。

首次 focused 日志 `p65_run_host_first.log` 为 `3/3` Success、`0` Fail、queue empty、native exit `0`、SHA-256 `DA6B233876EC1DBE2C5AE60AF108EFAA6E1D8E67251AB64044382DF7EC01E054`。

首次编译前静态复审发现 sweep 路由曾以“空 Prepared”作为只验证 Coordinator 的哨兵。该做法未进入任何构建；已改为独立 `CoordinatorMatches`，统一核验 Run、player entity 与注册 source Actor，并由 attach 的 `BindingMatches` 额外验证 Prepared。没有测试或编译失败。

## 8. 编译

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `5/5` actions；
- `Result: Succeeded`；
- native exit `0`；
- `11.35s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `4/4` actions；
- `Result: Succeeded`；
- native exit `0`；
- `22.66s`；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 9. P/F 边界

本轮只执行 P 阶段源码开发、静态审查、`-NullRHI` 无头 Automation 与 Editor/Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-5-controlled-weapon-run-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-5-controlled-weapon-run-host/Docs/Report/Dev.D.UE.0.0.10.P6.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-5-controlled-weapon-run-host/Docs/Log/Dev.D.UE.0.0.10.P6.5.r0_log.md>
