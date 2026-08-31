# Dev.D.UE.0.0.10.P11.8.r0 Report

## 1. 结论

P11.8 已完成并通过 P 阶段门禁。

本阶段把 P11.7 的 exact equipped-item authorization 与 P11.6 的 canonical product start 组合为一个同步、无状态的 `WeaponGuardProductRoute`。新入口不接受 item ID；它只允许当前 `WeaponSlot` 真值签发的授权进入 Run reservation 和 active host。

最终结果：

- 新增唯一 authorization-to-product route；
- exact item identity 贯穿 authorization、reservation、action 与 host；
- route 前后复查 item authority revision/current evidence；
- 新增 5 个 focused tests，0.0.10 全量达到 505/505；
- 5 组最终 Automation 日志共记录 586 次 Success、0 次 Fail；
- regression mapping 97 rules，自检 154/154；
- changed-file gate：`PASS Changed=5 Rules=1 Required=10 Logs=5`；
- `git diff --check` 与边界扫描通过；
- Editor Development 与 Game Development 单并发构建均首次成功，原生退出码均为 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 唯一产品启动入口

`Fdemo_mapShanmenWeaponGuardProductRoute::TryStart` 的公开输入只有：

- existing `Fdemo_mapItemAuthority`；
- existing `Fdemo_mapCombatRunCoordinator`；
- caller-owned monotonic timeline ID；
- caller-owned active-start tick。

API 没有 source item 参数。固定流程为：

```text
existing item authority
  -> authorize current exact WeaponSlot occupant (P11.7)
  -> verify authorization is current
  -> pass authorized exact item to canonical product start (P11.6)
  -> verify authorization remains current
  -> verify exact item across reservation + action + host
  -> return one complete route proof
```

因此任意 GUID、库存中未装备武器、旧换装缓存或按类别／名称选择的物品都无法进入 P11.6。

### 2.2 完整路由证据

`Fdemo_mapShanmenWeaponGuardProductRouteResult` 同时保留：

- P11.7 `ItemAuthorization`；
- P11.6 `ProductStart`；
- route status 与 diagnostic。

`IsReady` 要求 nested proofs 全部有效，并确认同一个 exact source item ID 同时存在于：

- item authorization；
- Run reservation；
- frozen combat action；
- active product host action runtime。

任何层丢失或替换 identity 都会 fail closed。

### 2.3 失败分类与消费边界

route 显式区分：

- `Ready`；
- `ItemAuthorizationRejected`；
- `ProductStartRejected`；
- `ItemAuthorizationStale`；
- `BindingRejected`。

空装备在 P11.7 层关闭；非法 timeline 与未就绪 Run 在 P11.6 层关闭。上述 preflight 失败不会消费 weapon-guard activation sequence。若 product start 已成功后才发现 authorization stale，已分配的 action/host proof 保留在结果中且 sequence 不复用。

## 3. 完整性

route 在调用 P11.6 前后都核对：

- authorization self-valid；
- authority revision 未变化；
- authorization 对当前 authority 仍为 current；
- P11.6 result complete；
- authorized item 与 reservation/action/host exact match。

route 本身不缓存 authority、host 或 timeline，不提供第二个 lifecycle，也不修改 item authority。相同装备快照的 authorization 稳定重放；每次 accepted route 仍由 Combat Run 分配新的单调 activation identity。

## 4. 兼容性与权威边界

- `Fdemo_mapItemAuthority` 继续拥有 item/equipment 真值与 revision；
- P11.7 继续唯一拥有 equipped-item authorization；
- P11.6 继续拥有 canonical config、Run reservation 与 active host；
- caller 继续拥有 timeline sample；
- route 只组合证据，不拥有 input、clock、Run、host、inventory、durability、Actor 或 Impact；
- 未改 GameMode、PlayerController、input mapping、Tick、timer、RNG 或 incoming damage 路径；
- 未引入第二套 item ID 选择、产品配置或 host lifecycle。

## 5. 修改范围

实现与门禁共 5 个文件、506 行新增：

- `Source/demo_map/demo_mapShanmenWeaponGuardProductRoute.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductRoute.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardProductRouteTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

加入本 Report/Log 后 exact stage 为 7 个文件。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 文档与其它资料未修改、未暂存、未提交。

## 6. 测试覆盖

新增 focused tests：

1. `ItemFences`：空装备在 product start 前拒绝，sequence 保持 1；
2. `ExactBinding`：exact item 贯穿四层 proof，route 不修改 item revision；
3. `TimelineFences`：missing/negative/overflow timeline 保留 item proof 且不消费 sequence；
4. `RunFence`：已授权装备不能伪造未就绪 Combat Run；
5. `Replacement`：换装使旧 authorization stale，新 route 绑定新实例与新 activation。

最终 Automation 证据：

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardProductRoute` | 5 | 0 | 1 | `017D63A2148B666955493FE8D261A61F23726E9938782EC947B9533435AE4961` |
| `Shanmen.0_0_10` | 505 | 0 | 1 | `A2209E722BF330F14CC94A7AF7EE12D71B924664046D40F7679571FEC94CFD45` |
| `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `8A954AA43C27C109E5A8791BE02CF2FED649378107ACF458814ED1AB3A7CD302` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `1AEF39A824CF6D1ADCB5F5B9F67D749855E46ABFF4647915F8B695C910C4F1F9` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | `8D5F4938C9DD966B41C41C596377B2DE2629B8FE8D1D23971FC7B10CE1F09389` |

合计 586 Success / 0 Fail；该合计包含 focused 与 `Shanmen.0_0_10` 全量之间的重复覆盖。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=97
SELF_TEST: PASS 154/154
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=5
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`8E7728668CE81DC6A72D7ABDBC1B78E50E39FE81CF1F8994446D6F3CF4F987EE`；
- self-test SHA-256：`E876FE87DF9953E64F1C1DA06547FFBA991337F1B6206260FEA919C9CB359450`；
- route rule 要求 full、route、item adapter、product authority、product host、Combat Run、Items 与三组 legacy item/Hotbar evidence；
- 边界扫描未发现 World、Actor、GameMode、PlayerController、input、Tick/timer、ApplyDamage 或 RNG 调用。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development | Succeeded | 5 / 29.20s | 0 | `52D264886A67CB989CF42A3691EF63770FF17DBD8BFABDF93FB2A63C7E39E0EC` |
| Game Development | Succeeded | 4 / 23.85s | 0 | `DE4ABFEC0F1157A61F66464951EFDA18EDDA5D39BE06A532051DBC08EE7DDAFD` |

产物：

- `UnrealEditor-demo_map.dll`：12,733,440 bytes，SHA-256 `8EAD9D2C395971DC33AD3760A7D1E35171A030078C961A735422441F3B6F0448`；
- `demo_map.exe`：354,271,232 bytes，SHA-256 `6F5A523E5E54F41BE6F20CF8A1A7D9D66D6B45889A6F8A118B2DEA46DE9C9E82`。

## 9. 真实异常

- 本阶段 focused、legacy、full Automation、Editor 构建与 Game 构建均首次通过；没有源码失败、测试失败、构建重试或 Windows commit-memory/pagefile 错误。
- UE SDK 检查仍打印与本目标无关的非 Win64 platform metadata invalid；Win64 SDK `10.0.22621.0` 有效，所有目标命令原生退出码均为 0。
- LF/CRLF 提示仅是 Git 工作树换行策略提示，`git diff --check` 无错误。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段实现、代码审查、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或 F 阶段产品回归。

下一阶段应审计现有玩家 command/input 与 monotonic timeline 的唯一入口，再让它调用本 route。不得让 input adapter 重新接受 item ID、选择 balance、创建第二个 host，或自行维护格挡窗口。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-8-weapon-guard-product-route>
