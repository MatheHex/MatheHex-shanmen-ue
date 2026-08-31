# Dev.D.UE.0.0.10.P11.7.r0 Report

## 1. 结论

P11.7 已完成并通过 P 阶段门禁。

本阶段补齐 P11.6 明确保留的装备授权缺口：不让调用方提供或猜测武器实例，而是直接读取既有 `Fdemo_mapItemAuthority` 的 `WeaponSlot`，要求 canonical definition 显式拥有 `WeaponGuard` 商品语义，再签发绑定 authority revision 的不可变授权证据。

最终结果：

- 六个 canonical 装备武器获得显式 `WeaponGuard` 语义；
- 物品内容身份升级为 `CodeB.Content.0.0.10.P11.7`，P7.7 身份保留为已知历史证据；
- 新增只读 equipped-weapon authorization adapter，不接受 caller-selected item ID；
- 新增 4 个 focused tests，0.0.10 全量达到 500/500；
- 8 组最终 Automation 日志共记录 855 次 Success、0 次 Fail；
- regression mapping 96 rules，自检 152/152；
- changed-file gate：`PASS Changed=9 Rules=3 Required=10 Logs=8`；
- `git diff --check` 与边界扫描通过；
- Editor 首次构建发现测试源码拼写错误并以原生退出码 1 失败，修复后 Editor 与 Game 构建均成功、原生退出码 0；
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 2. 功能性

### 2.1 显式商品语义与内容身份

`Edemo_mapItemGameplaySemantic` 新增 `WeaponGuard`。该语义只赋予以下六个 canonical definition：

- `TrainingBlade`；
- `HeavyPracticeBlade`；
- `WeaponLevel1`；
- `WeaponLevel2`；
- `WeaponLevel3`；
- `WeaponLevel4`。

catalog validation 要求 `WeaponGuard` definition 必须同时满足：

- category 为 Weapon；
- max stack 为 1；
- equipment slot 为 `WeaponSlot`；
- compatible slots 精确等于 `[WeaponSlot]`；
- 不得同时拥有 `ThrownWeapon` 语义；
- 全 catalog 精确存在 6 个 guard-capable definition。

因此防具、投掷物、名称相似物品或仅因位于武器类别中的未来物品都不能隐式获得格挡资格。

内容身份升级为：

```text
Version: CodeB.Content.0.0.10.P11.7
Digest:  6D01652004E386DC469CB09FF0F3A77110C53841F6AF3F66F5600C3C9B9B4179
Parent:  6C30E84A05386A7986A2344DB8247961E75F0FE2179F41927A0C7DF950F45A00
```

P7.7、P5.4 与更早身份仍由 `IsKnownContentIdentity` 接受；本阶段没有改写持久化 `DefinitionId` 或 schema。

### 2.2 既有装备真值上的只读授权

`Fdemo_mapShanmenWeaponGuardItemAdapter::AuthorizeEquippedWeapon` 固定执行：

```text
validate existing Runtime item authority
  -> read exact WeaponSlot occupant
  -> resolve exact item instance
  -> verify Equipped / local owner / equipment container / WeaponSlot / quantity 1
  -> resolve canonical definition
  -> require explicit WeaponGuard semantics and exact slot compatibility
  -> issue immutable revision-bound authorization
```

API 不接收 item instance ID，因此库存中已知武器、旧缓存 ID 或任意 GUID 都不能绕过装备权威。适配器只读，不执行 equip、unequip、库存迁移或第二份状态写入。

### 2.3 确定性授权与失效栅栏

授权证据绑定：

- authority revision；
- exact source item instance ID；
- definition ID；
- canonical `WeaponSlot`；
- content version 与 digest；
- 固定 identity namespace `demo_map.Sword.WeaponGuard.ItemAuthorization.r1`。

相同 authority 快照可重建相同授权；任意 equip、unequip、替换或其它 authority mutation 都推进 revision，使旧证据无法通过 `IsCurrentAuthorization`。装备替换还会同时改变 exact item ID 与 authorization ID。

## 3. 完整性

授权结果使用显式状态区分：

- `Authorized`；
- `AuthorityInvalid`；
- `WeaponNotEquipped`；
- `ItemUnavailable`；
- `EquipmentStateMismatch`；
- `DefinitionUnavailable`；
- `DefinitionNotGuardCapable`。

`Authorization::IsValid` 不只检查 GUID：它重新验证 current content identity、canonical definition、显式语义、slot contract，并重算 deterministic authorization ID。结果状态和授权内容必须同时有效，才能被视为授权成功。

## 4. 兼容性与权威边界

- `Fdemo_mapItemAuthority` 继续是局内 item/equipment 唯一真值；
- CodeB catalog 继续是 definition 与内容身份真值；
- P11.6 继续拥有 guard product config、Run action reservation 与 product-host 组合；
- P11.7 只证明“当前 exact equipped item 可以发起格挡”，不拥有输入、时钟、窗口、方向、Impact 或伤害数学；
- 未按 category、display name、价格或属性猜测格挡能力；
- 未直接修改库存、装备、耐久、资源事务或持久化存档；
- 未接入 GameMode、PlayerController、Actor discovery、Tick、timer、RNG 或 incoming Impact。

## 5. 修改范围

实现与门禁共 9 个文件、538 行新增、11 行删除：

- `Source/demo_map/demo_mapItemTypes.h`
- `Source/demo_map/demo_mapItemDefinitions.cpp`
- `Source/demo_map/demo_mapItemTests.cpp`
- `Source/demo_map/demo_mapShanmenItemMigrationTests.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardItemAdapter.h`
- `Source/demo_map/demo_mapShanmenWeaponGuardItemAdapter.cpp`
- `Source/demo_map/demo_mapShanmenWeaponGuardItemAdapterTests.cpp`
- `Scripts/ShanmenRegressionMap.json`
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

加入本 Report/Log 后 exact stage 为 11 个文件。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 文档与其它资料未修改、未暂存、未提交。

## 6. 测试覆盖

新增 focused tests：

1. `CanonicalCatalog`：六个精确 definition、负例语义与新旧内容身份；
2. `ExactAuthorization`：exact equipped instance、stable replay 与 current evidence；
3. `EquipmentFences`：空槽、仅库存、卸装均 fail closed；
4. `RevisionFence`：装备替换改变 evidence，旧授权立即失效。

最终 Automation 证据：

| Group | Success | Fail | Terminal | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.WeaponGuardItemAdapter` | 4 | 0 | 1 | `671DE276C52463C6B8D7AC74F7F116F566492497C0825525FBF356ABD82F5ED1` |
| `Shanmen.0_0_10` | 500 | 0 | 1 | `803B125F45A1743242667DC6E2AC713DC72BB5DBDBE54F9C5B1B55B527A81EFE` |
| `demo_map.CodeB` | 60 | 0 | 1 | `4558C96758FE7FCC9D9F2600D7E7972A54B2E04B76E511D25A8F7EB5B7DEF11F` |
| `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `1D0F16ED4DFE6060B61DB75933CD6EBA90F9CEAB2A2D897C34CF833D3B590386` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `BC151CB2ADC14A2AFF1AB98F18A78660B75C303FED04678954F652F2281F7AC5` |
| `demo_map.P4.Hotbar` | 7 | 0 | 1 | `7D67E5CB3F5A4E28EAA2A8ABBF6E336333670EC4C3406FD513166E10340B2275` |
| `demo_map.Profile` | 211 | 0 | 1 | `7B71A0EDDF9D1397E4141763F2AC70754F2D20654B890967018B8CBBD60F26EA` |
| `demo_map.V3.WorldInteraction` | 4 | 0 | 1 | `D6674C5EAA5BB8312FA9B1AC5B69E0F4DE76C35AAF3F2F4AC2FA7274BD1ABA3B` |

合计 855 Success / 0 Fail；该合计包含 focused 与 `Shanmen.0_0_10` 全量之间的重复覆盖。

## 7. 静态与回归门禁

```text
REGRESSION_MAP_JSON: PASS Rules=96
SELF_TEST: PASS 152/152
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=10 Logs=8
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`9005DF4C2B1688715BF348F5A210B25C147F4F6327EA8706878619C48521A2C5`；
- self-test SHA-256：`44E02930FB358391AF4A188C467BC66C207956FFCAF7175983092D702232AF41`；
- changed-file gate 从公共 catalog 变更自动推导出 Profile、CodeB、schema、armor/use、Hotbar 与 WorldInteraction 旧系统回归；
- 新 adapter rule 还要求 full、Items、P11.6 product authority 与本轮 focused evidence；
- 边界扫描未发现 World、Actor、GameMode、PlayerController、input、Tick/timer、ApplyDamage 或 RNG 调用。

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target / attempt | Result | Actions / Time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor Development attempt 1 | Failed (`OtherCompilationError`) | 172-action graph / 530.68s | 1 | `259DD47086AD3435673D3B66EEDB369B34AD16E210E89F327DB962271FEA97EC` |
| Editor Development final | Succeeded | 4 / 5.84s | 0 | `5125D45FE604FEE97A1EF8001569B9F4C424FBA5BC859A318628AAE87727D36C` |
| Game Development | Succeeded | 171 / 432.94s | 0 | `2233C80D247ACCB3D567EAF45054364A4C36DED4E8BEBC7338915A41BD9F496A` |

产物：

- `UnrealEditor-demo_map.dll`：12,711,424 bytes，SHA-256 `C3BB41906B0FD01DED67A8C1EDD3C95980BDCEED3C50BAACA1EC2039BBBE949E`；
- `demo_map.exe`：354,253,312 bytes，SHA-256 `FC310141A25DA0DC1A73629353F41BF7AC74D97F28F9285C6C2DB1B566F4A916`。

## 9. 真实异常

- 首次 Editor 构建在 `demo_mapShanmenWeaponGuardItemAdapterTests.cpp:64` 报 `C2065`：错误写成 `Test.TestTrue(...)`，而该测试类应直接调用 `TestTrue(...)`。这是测试源码错误，不是内存、SDK 或构建环境故障；原生退出码为 1。
- 进行单行最小修复后，Editor 重建 4 个动作并成功；随后所有 Automation 与 Game 构建均通过。
- UE SDK 检查仍打印与本目标无关的非 Win64 platform metadata invalid；Win64 SDK `10.0.22621.0` 有效，最终目标命令原生退出码均为 0。
- LF/CRLF 提示仅是 Git 工作树换行策略提示，`git diff --check` 无错误。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段实现、代码审查、无头 Automation、静态／路径门禁以及 Editor/Game Development 构建。未执行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或 F 阶段产品回归。

下一阶段可由既有 input/command route 在 guard start 时调用本 adapter，取得 current authorization 的 exact source item ID，再交给 P11.6 `PrepareStart`。组合完成前必须再次验证 authorization current；不得把装备状态复制进 Combat Run，也不得让 P11.6 接受未授权的 caller item ID。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-7-weapon-guard-item-authority>
