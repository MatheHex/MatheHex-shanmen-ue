# Dev.D.UE.0.0.10.P11.11.r0 Development Log

## 目标

为 P11.9/P11.10 的 WeaponGuard 产品链选择唯一 30 Hz Run-owned timeline，并接入既有统一输入注册表与 PlayerController 的实际 press/release binding；输入层只产生 typed intent，不拥有装备、Host、窗口或伤害状态。

## 基线

- branch：`agent/0.0.10-p11-11-weapon-guard-physical-input`；
- base：`f2a2b3ce306cf4c9e0dab448a68553faf615c577`；
- P11.10：唯一 active Product Session 与 ordered release/interrupt；
- P11.9：无状态 press adapter；
- P11.8：唯一 item-authorization-to-product route；
- P11.0-P11.6：timing、arc、defense、Host 与 Combat Run reservation；
- P 阶段，不启动产品。

## 审计结论

1. WeaponGuard 已有完整产品链，但没有 Shipping 物理 key；
2. 既有 input registry 有 22 个动作，需作为唯一键位真值扩展，不能另建输入系统；
3. WeaponGuard 是 hold action，必须同时绑定 press 与 release；
4. press 可以受 gameplay/UI gate 约束；
5. release 不能受 gate 约束，否则按住后打开 UI 会产生 stuck guard；
6. P11.9 要求 caller 提供 opaque timeline sample，但项目尚无 canonical guard clock；
7. timeline 应归 Run/GameMode，而非 PlayerController、adapter 或 Host；
8. timeline 只输出整数 tick，不读取 wall clock、frame counter 或 Timer；
9. 既有 perfect window 为 5 tick，因此需固定 tick rate才能形成稳定产品语义；
10. 选择 30 Hz 后，perfect window 为约 166.667 ms；
11. timeline identity 必须由 Run identity 确定性派生；
12. Version 3 input 文件必须保留旧覆盖并自动补新动作。

## 实现

新增 `Fdemo_mapShanmenWeaponGuardFixedTimeline`：

- canonical rate：30 Hz；
- deterministic ID：Run ID + rate；
- `TryBegin` / `TryAdvance` / `TryCapture` / `TryEnd` / `Reset`；
- monotonic whole ticks 与 sub-tick carry；
- invalid delta、overflow、Run mismatch 原子失败。

Input registry：

- 新增 `WeaponGuard`；
- 默认 `RightMouseButton`；
- `bRequiresReleasedEvent=true`；
- exact count 22 -> 23；
- serialization Version 3 -> 4；
- generic migration 保留旧覆盖并处理默认键冲突。

Input adapter：

- 新增 typed release status/result；
- release 委托 exactly once；
- release 无 gameplay/UI lock gate。

GameMode / PlayerController：

- Run begin/end 持有 timeline；
- GameMode Tick 推进 timeline；
- press 捕获 opaque sample 后启动 Session；
- release 结束 Session；
- live remap 同时重建 press/release binding；
- automation-only invocation/result 证据。

## 测试

新增 5 个 FixedTimeline tests：

- `IdentityLifecycle`
- `FixedRatePartition`
- `SubTickCarry`
- `InvalidDeltaAtomic`
- `OpaqueSample`

新增 6 个 PhysicalInput tests：

- `RegistryDefault`
- `VersionThreeMigration`
- `MigrationConflict`
- `PressReleaseBinding`
- `LiveRemap`
- `InputLockSafeRelease`

InputAdapter 增加 `ReleaseDelegation`，并同步更新所有 22 -> 23 exact-registry fixtures。

最终健康证据：

| Log | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `WeaponGuardFixedTimeline.log` | `Shanmen.0_0_10.Product.WeaponGuardFixedTimeline` | 5 | 0 | `76D32DADE6481430E5D67470EB49E1BAA3615ADAC87EA00A25CCF9314A11FC7C` |
| `WeaponGuardPhysicalInput.log` | `Shanmen.0_0_10.Product.WeaponGuardPhysicalInput` | 6 | 0 | `0A58AF8D5CB48966858032A9BBCB0439625D9922A8C47AFF57DE7D17101DEB40` |
| `WeaponGuardInputAdapter.log` | `Shanmen.0_0_10.Product.WeaponGuardInputAdapter` | 6 | 0 | `9BE627E7855DA2569D1C39FD362E10F4EBE7EC9C627B84A5A92A39B125FD43C4` |
| `SpiritEvasionPhysicalInput.log` | `Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput` | 6 | 0 | `1CCE00563B7A562677FDE8EC665630FD75C218C7DBD61178794DDCEB8C16BFB3` |
| `Shanmen.0_0_10.log` | `Shanmen.0_0_10` | 528 | 0 | `3C8903300FD3B77E1836B4A43B97F21770C83A625439DBE285C47A36BEBF5B4D` |
| `FullSystemLoop.41.log` | `demo_map.FullSystemLoop.41` | 1 | 0 | `8BC6D9163F085236BEA62FCA858C07FCA0BC15BDB1C637697723025DB2F1F0AA` |
| `FullSystemLoop.47.log` | `demo_map.FullSystemLoop.47` | 1 | 0 | `5AA0430B0AC4F9DDE27629DB8FC87210E1F28C05622BDF1B11F7F85D8B121514` |
| `P7Integration.log` | `demo_map.P7Integration` | 9 | 0 | `450E2313BF435B4E3D2CE0C49A803FE7B8771871779FA861A0562171CA0B970A` |
| `P5RuntimeInterface.06.log` | `demo_map.P5RuntimeInterface.06` | 1 | 0 | `88982A8C0DC73503FE77BC661BD97B7AE5A8190B56CA5EB9CD4AD4E0A4CC302B` |
| `InputRestore.32.log` | `demo_map.InputRestore.32` | 1 | 0 | `6E2DDFA88F964918FEDC6102AC4F783107AB928CDB1C318BCB0E0A98B2AF111F` |
| `V2RangedCompatibility.log` | `demo_map.V2RangedCompatibility` | 22 | 0 | `F3F9B595FFC1D031D715DFBF1CA6FD6EFFAD0EFF72845C9C81B4B6269C0F911C` |
| `ItemEconomySchema.log` | `demo_map.ItemEconomySchema` | 23 | 0 | `8E1657144369A611DB8F12C4E3FB8B2D8378CAD4894AC803F5BDF52C76FFD066` |
| `ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | `BF09D94C403CA454C2198895368EACED7C8BC209043AFAF30D77B36A54451E68` |
| `P4.Hotbar.log` | `demo_map.P4.Hotbar` | 7 | 0 | `58ABE0C9CDBA32CEE4445BB0D7EC88F8FADE9072A7C5C7300256D69F4315D178` |

合计 662 Success / 0 Fail；按 identity 去重 639 项。

## 回归映射

新增 FixedTimeline rule，扩充 UnifiedInput、GameMode、PlayerController 与 WeaponGuard adapter rules。

```text
REGRESSION_MAP_JSON: PASS Rules=100
SELF_TEST: PASS 160/160
REGRESSION_COVERAGE: PASS Changed=21 Rules=5 Required=43 Logs=14
git diff --check: PASS (native exit 0)
PURE_BOUNDARY_SCAN: PASS
TEST_DETERMINISM_SCAN: PASS
```

- mapping SHA-256：`42D7912BC74DC655726201CAC7A020599F580AF6C6C4807A92294CFFB1A9C5AD`；
- self-test SHA-256：`0022FA800060A23FD50D7CF3A7DCA74EEC379142290FB4F5B4BCFCB5DA85198E`。

## 构建

| Build | Result | Actions / time | Exit | SHA-256 |
|---|---|---|---:|---|
| Editor first | Failed / test API | 38/41 / 151.87s | 6 | `CF80DB1E4B9BDF6C56CF75BA461ADE650AC94BDEC54857D58E01430DCA0AA056` |
| Editor final | Succeeded | 4 / 5.28s | 0 | `654C0E8EFABC861EFE6EB84527388CFE28ED7A64E5ECA4AA6AAE35B48A4F77E6` |
| Game final | Succeeded | 40 / 147.17s | 0 | `5E53B4568DF8B4A602C8450AC3BA04EF5A7F125B6E7FAFCD7ED78D47AE5A179E` |

产物：

- `UnrealEditor-demo_map.dll`：12,841,984 bytes，SHA-256 `950F46A5D84FFA8E2C91850FB56DDF70B3150BBD25A454E4D587374EEA967FCE`；
- `demo_map.exe`：354,359,808 bytes，SHA-256 `DC2D4C4967F55D928B4390E9C08CBF1AC3BB9B01603C797134A3BA17FF8AABF0`。

## 真实异常

1. 首次 Editor 构建因新测试调用 UE 不提供的 `TNumericLimits::QuietNaN/Infinity` 而原生退出 6；
2. 改用 `std::numeric_limits` 后 Editor 最终构建成功；
3. 初始 boundary scan 的范围错误包含物理输入测试 `UWorld` fixture；拆分纯生产边界与测试确定性边界后均通过；
4. 最终无产品源码失败、Automation 失败、Fatal/Ensure 或内存环境错误；
5. Win64 SDK 有效，其它未安装平台的 metadata 提示不影响本轮目标。

## 修改统计

实现与门禁（不含本 Report/Log）：

```text
21 files changed, 1068 insertions(+), 19 deletions(-)
```

长期未跟踪资料不进入本阶段提交。

## P/F 边界

仅执行 P 阶段实现、无头 Automation、静态／路径门禁和 Editor/Game Development 构建。没有启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p11-11-weapon-guard-physical-input>
