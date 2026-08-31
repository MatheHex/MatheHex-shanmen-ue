# Dev.D.UE.0.0.10.P11.10.r0 Report

## 1. 结论

P11.10 已完成并通过 P 阶段门禁。

本阶段补齐 P11.8/P11.9 之间缺失的 Shipping 生命周期所有权：新增唯一 `WeaponGuardProductSession`，由既有 `Ademo_mapGameMode` 持久持有成功启动的 active Host。按键／输入层以后只需产生 start/release intent，不再可能把 active Host 作为临时返回值丢弃，也不保存战斗状态。

最终结果：

- Session 只允许一个 active guard Host；
- 第一次成功 start 持久保存完整 P11.8 route proof；
- active 期间重复 start 返回 typed `AlreadyActive`，不再次消费 Run sequence；
- 普通 release 严格执行 `Active -> Recovery -> Completed` 后才清空所有权；
- Run teardown 严格执行 `Active -> Interrupted` 后才清空所有权；
- 所有终止操作先在值副本上完成，失败不会半写真实 Session；
- GameMode 的新 Run 激活会拒绝遗留 guard Session；
- GameMode 在 Combat Run Coordinator 释放前中断 guard Host；
- 新增 6 个 focused tests，0.0.10 全量达到 516/516；
- 最终健康 Automation 日志共记录 598 次 Success、0 次 Fail；
- regression mapping 99 rules，自检 158/158；
- changed-file gate：`PASS Changed=7 Rules=2 Required=32 Logs=4`；
- `git diff --check`、JSON 解析与精确生产边界扫描通过；
- Editor Development 与 Game Development 最终单并发构建均成功，原生退出码均为 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 唯一活动所有者

`Fdemo_mapShanmenWeaponGuardProductSession` 持有一个成功的 `Fdemo_mapShanmenWeaponGuardProductRouteResult`。只有 nested item authorization、Run reservation 与 Product Host 全部 ready 时，Session 才进入 active 状态。

固定启动链为：

```text
existing ItemAuthority + active CombatRunCoordinator + caller timeline
  -> P11.8 WeaponGuardProductRoute
  -> exact equipped-item authorization
  -> canonical product reservation / Host start
  -> Session acquires the sole active Host
```

Session 不接受 caller-selected item ID。装备来源仍完全由 P11.7/P11.8 从既有 `WeaponSlot` 权威解析。

### 2.2 重复启动关闭

当 Session 已持有 active Host 时，后续 `TryStart`：

- 返回 `AlreadyActive`；
- 返回现有 Host/item identity 作为诊断证据；
- 不调用 P11.8；
- 不读取新装备作为替代所有者；
- 不消费新的 guard activation sequence；
- 不重置 timeline 或 Host。

因此按键重复、键盘 repeat 或未来重复路由不能创建并行 guard action。

### 2.3 普通释放事务

`TryRelease` 在 `ActiveRoute` 的值副本上依次调用：

```text
TryEnterRecovery()
  -> TryComplete()
  -> verify terminal proof
  -> clear real Session
```

任一步拒绝时，真实 Session 保持原 active 状态，便于诊断和重试；不会出现真实 Host 已进入 Recovery、Session 却仍声称 Active 的半写状态。

### 2.4 Run teardown 事务

`TryInterruptAndReset` 同样先复制 route，再执行 `TryInterrupt`。只有 terminal interruption proof 有效时才清空真实 Session。空 Session 的重复 teardown 是 typed、成功的 no-op。

GameMode 的 `ReleaseCombatProductRun` 在释放 Combat Run Coordinator 前执行该操作。失败会 fail closed 并保留 Run，防止 coordinator 消失后留下孤立 guard Host。

## 3. 完整性

启动结果显式区分：

- `Started`；
- `AlreadyActive`；
- `ItemAuthorityUnavailable`；
- `RouteRejected`；
- `StateDesynchronized`。

终止结果显式区分：

- `NoActiveHost`；
- `Completed`；
- `Interrupted`；
- `SessionInvalid`；
- `RecoveryRejected`；
- `CompletionRejected`；
- `InterruptRejected`；
- `StateDesynchronized`。

每个成功结果保留 exact Host ID 与 Host transition receipts。装备在 guard 激活后被替换时，`IsCurrentAuthorization` 会准确变为 false，但已启动 Host 的 frozen item identity 不被改写，且仍可有序释放。

## 4. 兼容性与权威边界

- ItemAuthority 继续拥有当前装备真值；
- P11.8 继续是唯一装备授权到产品启动 route；
- Combat Run Coordinator 继续拥有 Run/activation identity 与 sequence；
- Product Host 继续拥有 action/window/timing/arc/defense lifecycle；
- Product Session 只拥有一个 active route/Host 的持续生命周期；
- GameMode 只持有 Session 并提供 start/release/teardown 入口；
- P11.9 input adapter 仍保持无状态；
- 本阶段不绑定物理 key，不选择 clock/tick 单位，不调用 Tick/Timer；
- 未增加 Actor、World、PlayerController、inventory mutation、ApplyDamage、RNG 或 Impact 权威；
- 未创建第二套 item、Run、input registry 或 action lifecycle。

## 5. 修改范围

实现与门禁共 7 个文件、911 行新增、2 行删除：

- `Source/demo_map/demo_mapShanmenWeaponGuardProductSession.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductSession.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductSessionTests.cpp`
- `Source/demo_map/demo_mapGameMode.h`
- `Source/demo_map/demo_mapGameMode.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

长期未跟踪的 0.0.9B Prompt、Report、CSEMI 文档与其它资料未修改、未暂存、未提交。

## 6. 测试覆盖

新增 focused tests：

1. `StartFences`：缺 ItemAuthority／空 WeaponSlot 失败关闭且不消费 sequence；
2. `Ownership`：exact equipped item、Host 与 Run sequence 被唯一持有；
3. `ActiveFence`：重复 start 返回同一 Host 且不再次消费 sequence；
4. `Release`：严格产生 Recovery 与 Completed receipts，重复释放为 no-op；
5. `Interrupt`：Run teardown 产生 Interrupted receipt，重复 teardown 为 no-op；
6. `StaleItem`：装备替换使 authorization stale，但不改写 active Host 且可清理。

最终健康 Automation 证据：

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardProductSession` | 6 | 0 | 1 | `758F99124EA1627DF5CE079945F99F195011E8C0C79067232EBC021D020ACF97` |
| `Shanmen.0_0_10` | 516 | 0 | 1 | `5FCC95664C66400EAE85636D290F8DD44C3D931313228AEF1774657A382BC7C0` |
| `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `9D305CAD4FB7DDE9ACE3A4B09844F248AE4970CAE005E539A40602F10D7830CE` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `AB3B56F817F2E775737D57D93F3DB54A4D4E4EDC3C6C294F9634AA897E84D422` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | `286C59335BB541D0862D3835C1709663A3928B9AD6473BE1A819CC6948EA0794` |

合计 598 Success / 0 Fail；focused 6 项同时包含在 516 项 full suite 中，因此该合计包含重复覆盖。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=99
SELF_TEST: PASS 158/158
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=32 Logs=4
git diff --check: PASS (native exit 0)
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`0F3B71B28BC2B7489CD1E9F96F24C696E8B7138E8E3BD2D84DDD6BA71630C87D`；
- self-test SHA-256：`23EFBDAF3039775214EAF0ED75DD3F30C535B6FAF698A4461392D85E209B8797`；
- Session rule 要求 full、focused Session、P11.8 route、P11.7 item adapter、product authority/host、Combat Run、Items 与三组 legacy item/Hotbar evidence；
- GameMode rule 同步加入 Session 及其完整 route/authority chain；
- 精确生产扫描未发现 World、Actor、PlayerController、physical input、timer、ApplyDamage、RNG 或新 GUID 调用。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Final actions / time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development | Succeeded | 4 / 11.71s | 0 | `E949CD97CAC5367D04C9E5CCF96B55FA19324068E8A5BC97975B45C4BD1CF939` |
| Game Development | Succeeded | 3 / 12.21s | 0 | `6D701A7F0CC85DA87E2820F0928933AFC32D25FE188F1B60121D49B8EAC230FE` |

产物：

- `UnrealEditor-demo_map.dll`：12,791,296 bytes，SHA-256 `59EF8987E07A7331D8BD9C0EB3CE68D874814CBA835970E879049E08D341EF66`；
- `demo_map.exe`：354,317,824 bytes，SHA-256 `DE34F35234A358CB3084D7A7E058FAB8A65E9227B98655C274AD32ABC5AE55C3`。

## 9. 真实异常

- focused、0.0.10 full、三组 mapped legacy Automation 与两个最终构建均通过，没有产品源码失败、mapped test 失败或内存环境错误。
- 为验证“按路径选回归”与“盲目跑主题父组”的差异，首次额外运行了过宽的 `Automation RunTests demo_map`。该日志完成 1328 项，1231 Success / 97 Fail，进程退出码 0；失败从 `demo_map.AutomationRootBoundary` 开始，明确显示命令进程的 UserDir 落在 UE Engine binaries，历史 exact-leaf/redirected-root 测试前置上下文不成立，并继续影响 Reward/SearchContainer 等历史组。它不属于 mapping 要求，也不能作为健康证据。
- 该首次失败日志完整保留为 `Saved/Logs/P11.10/demo_map.log`，SHA-256 `B88E0CEFBA608EA7D3FFBCD36F5A78FC95ABFE21069B238B6F1DD8D364DA32A2`；没有删除、覆盖或把 0 进程码误报为测试成功。
- 随后严格按改动映射分别运行三组 legacy contract，23/23、46/46、7/7 全部成功，changed-file gate 通过。
- changed-file validator 第一次从外层 `pwsh -File` 传数组时被合并成单个字符串，报 unclassified；改为当前 PowerShell 进程传真实数组后原规则通过。属于命令编排错误，不是产品或映射失败。
- 完成一次无语义清理后，重新执行最终增量 Editor/Game 构建以确保日志对应最终源码；两次最终构建均首次成功。
- UE SDK 检查仍打印与本目标无关的非 Win64 platform metadata invalid；Win64 SDK `10.0.22621.0` 有效。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段实现、代码审查、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或 F 阶段产品回归。

下一阶段 P11.11 应在既有统一玩家输入入口中：

1. 选择项目唯一的 fixed-rate monotonic guard timeline（建议明确 tick rate，避免 frame-rate 依赖）；
2. 将物理 press 先送入 P11.9 input adapter；
3. 让 adapter 的 route callback 调用本阶段 GameMode Session start；
4. 将 release 送入 `RouteWeaponGuardReleaseIntent`；
5. 不把 Host、item 选择、clock 或 lifecycle 反向塞回按键处理器。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-10-weapon-guard-product-session>
