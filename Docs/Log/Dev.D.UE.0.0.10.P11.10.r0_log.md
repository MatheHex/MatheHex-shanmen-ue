# Dev.D.UE.0.0.10.P11.10.r0 Development Log

## 目标

把 P11.8/P11.9 成功返回的 active guard Host 从临时值提升为 Shipping 期唯一持久所有权；由既有 GameMode 持有一个 Product Session，普通 release 完成 Recovery/Completed，Combat Run teardown 完成 Interrupt。本阶段不接物理输入或实际 clock。

## 基线

- branch：`agent/0.0.10-p11-10-weapon-guard-product-session`；
- base：`50978bff40c252a785a3b57e844a366398d3b2ae`；
- P11.9：无状态 input adapter；
- P11.8：唯一 item-authorization-to-product route；
- P11.7：current exact equipped weapon authorization；
- P11.6：canonical config、Run reservation、active Host；
- P 阶段，不启动产品。

## 审计结论

1. P11.8 route result 按值携带 active Host；
2. P11.9 input result 再按值包装 route result；
3. Shipping caller 若丢弃返回值，guard lifecycle 会立即失去所有者；
4. 物理输入接入前必须先建立 persistent owner；
5. owner 应位于已有 GameMode/Combat Run 生命周期，不应位于 key handler；
6. 同时只能有一个 active guard Host；
7. 重复 start 不得消费新 activation sequence；
8. 正常释放必须经过 Recovery 再 Completed；
9. Run teardown 必须在 coordinator 释放前 Interrupt；
10. 终止失败必须保留真实 Session，不允许半写；
11. item replacement 只使 authorization stale，不得改写 frozen Host；
12. fixed-rate timeline 与 physical binding 延后到 P11.11。

## 实现

新增：

- `Edemo_mapShanmenWeaponGuardSessionStartStatus/Error`；
- `Fdemo_mapShanmenWeaponGuardSessionStartResult`；
- `Edemo_mapShanmenWeaponGuardSessionTransitionStatus/Error`；
- `Fdemo_mapShanmenWeaponGuardSessionTransitionResult`；
- `Fdemo_mapShanmenWeaponGuardProductSession`。

Session 保存完整 ready route，并提供：

- `TryStart`；
- `TryRelease`；
- `TryInterruptAndReset`；
- `IsEmpty` / `HasActive`；
- `IsCurrentAuthorization`；
- active route/Host 的只读 getter。

GameMode 新增：

- `RouteWeaponGuardStartIntent`；
- `RouteWeaponGuardReleaseIntent`；
- `GetWeaponGuardProductSession`；
- stale lifecycle Run-start fence；
- ordered Run teardown interruption；
- release audit log 中的 guard interruption 结果。

## 测试

新增 6 个 focused tests：

- `StartFences`
- `Ownership`
- `ActiveFence`
- `Release`
- `Interrupt`
- `StaleItem`

最终健康证据：

| Log | Group | Success | Fail | Terminal | SHA-256 |
|---|---|---:|---:|---:|---|
| `WeaponGuardProductSession.log` | `Shanmen.0_0_10.Product.WeaponGuardProductSession` | 6 | 0 | 1 | `758F99124EA1627DF5CE079945F99F195011E8C0C79067232EBC021D020ACF97` |
| `Shanmen.0_0_10.log` | `Shanmen.0_0_10` | 516 | 0 | 1 | `5FCC95664C66400EAE85636D290F8DD44C3D931313228AEF1774657A382BC7C0` |
| `ItemEconomySchema.log` | `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `9D305CAD4FB7DDE9ACE3A4B09844F248AE4970CAE005E539A40602F10D7830CE` |
| `ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `AB3B56F817F2E775737D57D93F3DB54A4D4E4EDC3C6C294F9634AA897E84D422` |
| `P4.Hotbar.log` | `demo_map.P4.Hotbar` | 7 | 0 | 1 | `286C59335BB541D0862D3835C1709663A3928B9AD6473BE1A819CC6948EA0794` |

总计 598 Success / 0 Fail；包含 focused/full 重复覆盖。

## 回归映射

新增 `WeaponGuardProductSession` rule，并扩充 `M01GameMode` rule，使所有 Session、route、item、Host、Run 与 legacy 依赖都由 changed-file gate 推导。

```text
REGRESSION_MAP_JSON: PASS Rules=99
SELF_TEST: PASS 158/158
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=32 Logs=4
git diff --check: PASS (native exit 0)
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`0F3B71B28BC2B7489CD1E9F96F24C696E8B7138E8E3BD2D84DDD6BA71630C87D`；
- self-test SHA-256：`23EFBDAF3039775214EAF0ED75DD3F30C535B6FAF698A4461392D85E209B8797`。

## 构建

| Build | Result | Final actions / time | Exit | SHA-256 |
|---|---|---|---:|---|
| Editor | Succeeded | 4 / 11.71s | 0 | `E949CD97CAC5367D04C9E5CCF96B55FA19324068E8A5BC97975B45C4BD1CF939` |
| Game | Succeeded | 3 / 12.21s | 0 | `6D701A7F0CC85DA87E2820F0928933AFC32D25FE188F1B60121D49B8EAC230FE` |

产物：

- `UnrealEditor-demo_map.dll`：12,791,296 bytes，SHA-256 `59EF8987E07A7331D8BD9C0EB3CE68D874814CBA835970E879049E08D341EF66`；
- `demo_map.exe`：354,317,824 bytes，SHA-256 `DE34F35234A358CB3084D7A7E058FAB8A65E9227B98655C274AD32ABC5AE55C3`。

## 真实异常

1. 额外的过宽 `demo_map` 父组完成 1328 项、1231 Success / 97 Fail；进程退出 0，但日志不健康，证明不能只看进程码；
2. 首个失败族是 `AutomationRootBoundary`，当前命令进程 UserDir 为 UE Engine binaries，历史 exact-leaf/redirected-root 前置上下文不成立；
3. 失败日志保留为 `Saved/Logs/P11.10/demo_map.log`，SHA-256 `B88E0CEFBA608EA7D3FFBCD36F5A78FC95ABFE21069B238B6F1DD8D364DA32A2`；
4. 随后按 mapping 跑三组实际 legacy contract，全部成功；
5. validator 首次调用因外层 `pwsh -File` 把路径数组并成单字符串而报 unclassified；改为当前 PowerShell 进程传数组后通过；
6. 最终无产品源码失败、mapped test 失败、构建失败或内存环境错误。

## 修改统计

实现与门禁：

```text
7 files changed, 911 insertions(+), 2 deletions(-)
```

长期未跟踪资料不进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-10-weapon-guard-product-session>
