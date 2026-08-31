# Dev.D.UE.0.0.10.P11.9.r0 Development Log

## 目标

在 P11.8 唯一产品 route 前建立无状态 weapon-guard input seam：先执行既有 gameplay/route gate，再对 caller-owned monotonic timeline 取样一次并委托一次；不接真实按键，不拥有 item、clock、Run、host 或 Impact。

## 基线

- branch：`agent/0.0.10-p11-9-weapon-guard-input-adapter`；
- base：`aa72de451442807f10ac72ae0867292702250339`；
- P11.8：唯一 authorization-to-product route；
- P11.7：current exact equipped weapon authorization；
- P11.6：canonical product config、Run reservation、active host；
- P 阶段，不启动产品。

## 审计结论

1. P11.8 已消除 caller-selected item ID，但仍需要输入边界；
2. 本阶段不应直接选择物理 key 或 frame/second 单位；
3. gameplay gate 与 route availability 必须在 timeline 取样前执行；
4. eligible 输入必须只取样一次，避免同一次按压出现不同 tick；
5. downstream rejection 不得由 input 层重试；
6. timeline ID/tick 必须原样到达 P11.8 host timing policy；
7. item 缺失与 exact authorization 继续由 P11.7/P11.8 处理；
8. input seam 不接受 item ID、balance、Run ID、host、retry 或 Impact；
9. 不创建新的 clock、GUID、timer、input registry 或 lifecycle；
10. 下一阶段再由统一玩家输入入口选择实际 monotonic source 并绑定 command。

## 实现

新增：

- `Fdemo_mapShanmenWeaponGuardInputTimelineSample`；
- `Edemo_mapShanmenWeaponGuardInputStatus`；
- `Fdemo_mapShanmenWeaponGuardInputResult`；
- `Fdemo_mapShanmenWeaponGuardInputAdapter::RouteStartInput`。

adapter 在 blocked / unavailable 时不执行任何 callback；eligible 时恰好调用 sample callback 一次。sample 有效后，恰好调用 P11.8 callback 一次；失败直接返回 typed result，不重试、不改值。accepted proof 复核 host timing policy 与 sample 完全一致。

## 测试

新增 5 个 focused tests：

- `Gates`
- `Timeline`
- `SingleSample`
- `ItemFence`
- `AppliedProof`

最终证据：

| Log | Group | Success | Fail | Terminal | SHA-256 |
|---|---|---:|---:|---:|---|
| `WeaponGuardInputAdapter.log` | `Shanmen.0_0_10.Product.WeaponGuardInputAdapter` | 5 | 0 | 1 | `E3926E2392EADE3C5594894C4D6F25A94556F34FF7707B54655A4EF15B708DC3` |
| `Full.log` | `Shanmen.0_0_10` | 510 | 0 | 1 | `A81DB5597E7537E92796E72D2512DE8B9D5D45F2B01A2BB2260EE8B97F201904` |
| `ItemEconomySchema.log` | `demo_map.ItemEconomySchema` | 23 | 0 | 1 | `BD38818E4B9D5C8EDDBE1221B2B72411252A513EF57DE55A9872C80ACC714311` |
| `ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 1 | `E93CF7C84CCF8E856259B11268EDC7109F99AAB25CB2F5D6403E821F762F0E4B` |
| `Hotbar.log` | `demo_map.P4.Hotbar` | 7 | 0 | 1 | `5D6DA3D426B6B12F36BCDAD1D9BCFD79D75BD1606DB83DE7A5C90F4B860CA932` |

总计 591 Success / 0 Fail；包含 focused 与 full 重复覆盖。

## 回归映射

新增 `WeaponGuardInputAdapter` rule，要求 P11.8/P11.7/product host/authority/Combat Run/Items 与 legacy item/Hotbar 完整证据。

```text
REGRESSION_MAP_JSON: PASS Rules=98
SELF_TEST: PASS 156/156
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=5
git diff --check: PASS
BOUNDARY_SCAN: PASS
```

- mapping SHA-256：`702FB2A97E1B9BEFFCBD168759057DFFC5261C6B8F6431EE746C6DC153EFAD2B`；
- self-test SHA-256：`BC594759AD044D369D7D327BF36CD50F39B74820264D9FCC869B7297FCEB4F62`。

## 构建

| Build | Result | Actions / Time | Exit | SHA-256 |
|---|---|---|---:|---|
| Editor | Succeeded | 5 / 31.76s | 0 | `32558B7C9173E7FD9613CB0D1C05A5FF36BB2E71566F7113CDAE15A7CAE7A346` |
| Game | Succeeded | 4 / 28.20s | 0 | `E91940B5CED462A8E629387CE1D7CCB3B8E9B1A848003C59DBBA5D118216C7BB` |

产物：

- `UnrealEditor-demo_map.dll`：12,754,944 bytes，SHA-256 `EA9292BE1A39E62928DAE0E404BCB5F85EB59E7D85D8362844FD5EF5B48F2CDC`；
- `demo_map.exe`：354,289,152 bytes，SHA-256 `DA103B5DD2BF056E551B7B7533A89EA561607DE37316EC0AD8CF13C8020C67D9`。

## 真实异常

产品实现、全部 Automation 与两个构建均首次通过。changed-file gate 的前两次调用分别因 PowerShell 数组参数展平及错误推测日志名失败；改为 hashtable splatting 并使用实际日志名后通过。未改产品代码、未重跑测试或构建。未发生内存环境故障。

## 修改统计

```text
5 files changed, 522 insertions(+)
```

加入 Report/Log 后 exact stage 为 7 个文件、802 行新增。长期未跟踪资料不进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-9-weapon-guard-input-adapter>
