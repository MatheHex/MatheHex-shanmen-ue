# Dev.D.UE.0.0.10.P11.7.r0 Development Log

## 目标

在既有局内 item/equipment 真值上，为 P11.6 武器格挡产品启动提供 exact equipped weapon authorization；不接受 caller-selected item，不按类别或名称猜能力，不接输入、时钟或 Impact。

## 基线

- branch：`agent/0.0.10-p11-7-weapon-guard-item-authority`；
- base：`27d5b438fdc380e08366a28abf6738b2c54cdf7f`；
- P11.6：canonical guard config、Run action reservation、唯一 product host；
- `Fdemo_mapItemAuthority`：局内 item/equipment 真值；
- CodeB catalog：definition 与 content identity 真值；
- P 阶段，不启动产品。

## 审计结论

1. P11.6 要求 exact source item，但尚无负责授权该 ID 的装备层；
2. 调用方传任意 GUID 会把 item authority 留在约定而非类型／结构中；
3. weapon category 或 display name 不能代表 guard capability；
4. 现有 `Fdemo_mapItemAuthority` 已有唯一 `WeaponSlot` occupant 与 revision；
5. 因此 adapter 必须直接读取 slot，不接受 caller item ID；
6. guard capability 必须是 catalog 的显式 typed semantic；
7. proof 必须绑定 authority revision，任何 mutation 后失效；
8. content semantic 变化必须升级 content identity，并保留旧 identity 为历史证据；
9. adapter 只能读，不得新增第二套装备或库存写路径；
10. 本阶段不接 GameMode、input、timeline、Impact、durability 或 resource transaction。

## 实现

新增：

- `Edemo_mapItemGameplaySemantic::WeaponGuard`；
- `Fdemo_mapShanmenWeaponGuardItemAuthorization`；
- `Edemo_mapShanmenWeaponGuardItemStatus`；
- `Fdemo_mapShanmenWeaponGuardItemResult`；
- `Fdemo_mapShanmenWeaponGuardItemAdapter::AuthorizeEquippedWeapon`；
- `Fdemo_mapShanmenWeaponGuardItemAdapter::IsCurrentAuthorization`。

六个 canonical equipped weapon definition 显式增加 guard semantic。内容身份升级为 `CodeB.Content.0.0.10.P11.7` / `6D01652004E386DC469CB09FF0F3A77110C53841F6AF3F66F5600C3C9B9B4179`，P7.7 保留为 known historical identity。

授权 ID 绑定 authority revision、exact item ID、definition、WeaponSlot、content version/digest 与固定 namespace。相同快照稳定重放；任意 authority mutation 使旧 evidence stale。

## 测试

新增 4 个 focused tests：

- `CanonicalCatalog`
- `ExactAuthorization`
- `EquipmentFences`
- `RevisionFence`

最终证据：

| Log | Group | Success | Fail | Terminal | SHA-256 |
|---|---|---:|---:|---:|---|
| `WeaponGuardItemAdapter.log` | `Shanmen.0_0_10.Product.WeaponGuardItemAdapter` | 4 | 0 | 1 | `671DE276C52463C6B8D7AC74F7F116F566492497C0825525FBF356ABD82F5ED1` |
| `Full.log` | `Shanmen.0_0_10` | 500 | 0 | 1 | `803B125F45A1743242667DC6E2AC713DC72BB5DBDBE54F9C5B1B55B527A81EFE` |
| `CodeB.log` | `demo_map.CodeB` | 60 | 0 | 1 | `4558C96758FE7FCC9D9F2600D7E7972A54B2E04B76E511D25A8F7EB5B7DEF11F` |
| `ItemEconomySchema.log` | `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `1D0F16ED4DFE6060B61DB75933CD6EBA90F9CEAB2A2D897C34CF833D3B590386` |
| `ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `BC151CB2ADC14A2AFF1AB98F18A78660B75C303FED04678954F652F2281F7AC5` |
| `Hotbar.log` | `demo_map.P4.Hotbar` | 7 | 0 | 1 | `7D67E5CB3F5A4E28EAA2A8ABBF6E336333670EC4C3406FD513166E10340B2275` |
| `Profile.log` | `demo_map.Profile` | 211 | 0 | 1 | `7B71A0EDDF9D1397E4141763F2AC70754F2D20654B890967018B8CBBD60F26EA` |
| `WorldInteraction.log` | `demo_map.V3.WorldInteraction` | 4 | 0 | 1 | `D6674C5EAA5BB8312FA9B1AC5B69E0F4DE76C35AAF3F2F4AC2FA7274BD1ABA3B` |

总计 855 Success / 0 Fail；包含 focused 与 full 重复覆盖。

## 回归映射

新增 `WeaponGuardItemAdapter` rule；公共 item types/catalog 变更继续命中 `CanonicalItemCatalog` 与既有 item migration 规则。

```text
REGRESSION_MAP_JSON: PASS Rules=96
SELF_TEST: PASS 152/152
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=10 Logs=8
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`9005DF4C2B1688715BF348F5A210B25C147F4F6327EA8706878619C48521A2C5`；
- self-test SHA-256：`44E02930FB358391AF4A188C467BC66C207956FFCAF7175983092D702232AF41`。

## 构建

| Build | Result | Actions / Time | Exit | SHA-256 |
|---|---|---|---:|---|
| Editor attempt 1 | Failed (`OtherCompilationError`) | 172-action graph / 530.68s | 1 | `259DD47086AD3435673D3B66EEDB369B34AD16E210E89F327DB962271FEA97EC` |
| Editor final | Succeeded | 4 / 5.84s | 0 | `5125D45FE604FEE97A1EF8001569B9F4C424FBA5BC859A318628AAE87727D36C` |
| Game | Succeeded | 171 / 432.94s | 0 | `2233C80D247ACCB3D567EAF45054364A4C36DED4E8BEBC7338915A41BD9F496A` |

产物：

- `UnrealEditor-demo_map.dll`：12,711,424 bytes，SHA-256 `C3BB41906B0FD01DED67A8C1EDD3C95980BDCEED3C50BAACA1EC2039BBBE949E`；
- `demo_map.exe`：354,253,312 bytes，SHA-256 `FC310141A25DA0DC1A73629353F41BF7AC74D97F28F9285C6C2DB1B566F4A916`。

## 真实异常

首次 Editor 构建在新测试第 64 行以 `C2065` 失败：`Test.TestTrue` 是错误拼写。该源码错误按原样保留，原生退出码 1。最小改为 `TestTrue` 后，Editor、全部 Automation 与 Game 均通过。未发生 Windows commit-memory/pagefile 错误；非 Win64 SDK metadata 提示不影响有效 Win64 目标。

## 修改统计

```text
9 files changed, 538 insertions(+), 11 deletions(-)
```

加入 Report/Log 后 exact stage 为 11 个文件。长期未跟踪资料不进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-7-weapon-guard-item-authority>
