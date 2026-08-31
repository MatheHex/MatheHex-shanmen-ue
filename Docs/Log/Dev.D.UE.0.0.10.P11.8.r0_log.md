# Dev.D.UE.0.0.10.P11.8.r0 Development Log

## 目标

把 P11.7 当前装备授权与 P11.6 canonical product start 组合成不暴露 item ID 的唯一同步 route；不接 GameMode/input，不拥有 clock、Run、host 或 Impact。

## 基线

- branch：`agent/0.0.10-p11-8-weapon-guard-product-route`；
- base：`0dd6f2d74757328584b9ae178d2f61437ce2b996`；
- P11.6：product config、Run reservation、active host；
- P11.7：exact equipped-item authorization；
- P 阶段，不启动产品。

## 审计结论

1. P11.7 已能证明 current WeaponSlot exact item；
2. P11.6 仍公开接受 source item GUID；
3. 真实产品入口必须消除 caller-selected GUID；
4. 最小解是无状态 route，不是新的 inventory/controller/lifecycle；
5. route 必须保留 nested authorization 与 product proofs；
6. exact item 必须贯穿 reservation、action 与 host；
7. authorization 在 product start 前后都应 current；
8. rejected preflight 不得消费 Run sequence；
9. caller 继续拥有 timeline sample；
10. 本阶段不接 input、Actor、Impact 或 durability。

## 实现

新增：

- `Edemo_mapShanmenWeaponGuardProductRouteStatus`；
- `Fdemo_mapShanmenWeaponGuardProductRouteResult`；
- `Fdemo_mapShanmenWeaponGuardProductRoute::TryStart`。

route API 只接 item authority、Combat Run coordinator、timeline ID 与 start tick。它不接 item ID。结果要求 authorization、reservation、action 和 host 四层 exact item identity 相同。

## 测试

新增 5 个 focused tests：

- `ItemFences`
- `ExactBinding`
- `TimelineFences`
- `RunFence`
- `Replacement`

最终证据：

| Log | Group | Success | Fail | Terminal | SHA-256 |
|---|---|---:|---:|---:|---|
| `WeaponGuardProductRoute.log` | `Shanmen.0_0_10.Product.WeaponGuardProductRoute` | 5 | 0 | 1 | `017D63A2148B666955493FE8D261A61F23726E9938782EC947B9533435AE4961` |
| `Full.log` | `Shanmen.0_0_10` | 505 | 0 | 1 | `A2209E722BF330F14CC94A7AF7EE12D71B924664046D40F7679571FEC94CFD45` |
| `ItemEconomySchema.log` | `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `8A954AA43C27C109E5A8791BE02CF2FED649378107ACF458814ED1AB3A7CD302` |
| `ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `1AEF39A824CF6D1ADCB5F5B9F67D749855E46ABFF4647915F8B695C910C4F1F9` |
| `Hotbar.log` | `demo_map.P4.Hotbar` | 7 | 0 | 1 | `8D5F4938C9DD966B41C41C596377B2DE2629B8FE8D1D23971FC7B10CE1F09389` |

总计 586 Success / 0 Fail；包含 focused 与 full 重复覆盖。

## 回归映射

新增 `WeaponGuardProductRoute` rule，要求完整 item authorization、product authority、product host、Combat Run、Items 与 legacy item/Hotbar evidence。

```text
REGRESSION_MAP_JSON: PASS Rules=97
SELF_TEST: PASS 154/154
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=5
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`8E7728668CE81DC6A72D7ABDBC1B78E50E39FE81CF1F8994446D6F3CF4F987EE`；
- self-test SHA-256：`E876FE87DF9953E64F1C1DA06547FFBA991337F1B6206260FEA919C9CB359450`。

## 构建

| Build | Result | Actions / Time | Exit | SHA-256 |
|---|---|---|---:|---|
| Editor | Succeeded | 5 / 29.20s | 0 | `52D264886A67CB989CF42A3691EF63770FF17DBD8BFABDF93FB2A63C7E39E0EC` |
| Game | Succeeded | 4 / 23.85s | 0 | `DE4ABFEC0F1157A61F66464951EFDA18EDDA5D39BE06A532051DBC08EE7DDAFD` |

产物：

- `UnrealEditor-demo_map.dll`：12,733,440 bytes，SHA-256 `8EAD9D2C395971DC33AD3760A7D1E35171A030078C961A735422441F3B6F0448`；
- `demo_map.exe`：354,271,232 bytes，SHA-256 `6F5A523E5E54F41BE6F20CF8A1A7D9D66D6B45889A6F8A118B2DEA46DE9C9E82`。

## 真实异常

本阶段所有实现后检查、Automation 和构建均首次通过。未发生源码失败、测试失败、重试或内存环境故障。非 Win64 SDK metadata 提示不影响有效 Win64 目标。

## 修改统计

```text
5 files changed, 506 insertions(+)
```

加入 Report/Log 后 exact stage 为 7 个文件。长期未跟踪资料不进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-8-weapon-guard-product-route>
