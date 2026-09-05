# Dev.D.UE.0.0.10.P20.30.r0 Log

## 阶段

- 任务：P20.30 thrown-weapon Ballistic Arc preview composition；
- 基线：`a9204d75fb43f745546ae15a3d9676119be3b59e`；
- 分支：`agent/0.0.10-p20-30-thrown-weapon-arc-preview-composition`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/可见轨迹/截图/Smoke/Cook/Package。

## 实现记录

1. 新增不可变 `Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration`。
2. 配置只捕获既有 action、产品 Arc policy、产品发射速度上限、choice policy 与 segment count。
3. 配置 ID 由全部冻结几何输入确定性派生并在 `IsValid()` 中重算。
4. 新增纯同步 `Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose()`。
5. 有效配置只读取一次 current choice、只采样一次 source basis。
6. choice 无效时不读 basis；basis 无效时不执行 projection、planner 或 sampler。
7. 复用现有 Arc choice projector、产品 Arc envelope、Arc planner 与 P20.29 preview sampler。
8. typed status 区分配置、choice、basis、projection、plan、preview 与成功结果。
9. projection 与 plan 拒绝保留原始状态和诊断；成功结果重建 plan 与 preview 自验证。
10. 未加入库存 reserve/consume/commit、launch、World、trace、collision、Actor、Component、Widget、renderer、timer、tick 或输入设备权威。
11. 新增 7 项纯值自动化，完整 0.0.10 总数 `991→998`。
12. changed-file map 新增组合规则；正反 self-test `353→355`。
13. 当前存在的 103 个历史无关未跟踪文件保持未暂存、未修改、未删除。

## 执行结果

首轮 Editor 构建直接成功：7 actions、native exit 0、10.01 秒。首轮聚焦自动化直接得到 `7/0`；无需修复生产代码或测试。最终又独立运行聚焦组及全部依赖组，并运行完整 0.0.10。

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_preview_composition_final.log` | `7/0` | `E073C1F555CF164BF525F2647C16FA2B2BB4B3719C6C4F165132206C5EF94A66` |
| `arc_choice_projection_final.log` | `5/0` | `EB69B35FB443E0D126798BA900DD631E7B26CE07486795EAD43B1FC83D2F0D52` |
| `thrown_product_controller_final.log` | `7/0` | `D8861801950FC46BFC779976942206A4E0DBC69309A71FA784BBC1F208EE6240` |
| `arc_preview_sampler_final.log` | `6/0` | `6F0DD10D8A681D83EC6A015B9962F6E239F0232BD2A0243F3787311C904000CB` |
| `arc_planner_final.log` | `10/0` | `5F009C02F08A09CAF3988B1969ECE8E1E0C6ADACC11F9385A2E4E4C04B762A18` |
| `thrown_weapon_runtime_final.log` | `20/0` | `55ED77D19B83421F01B112E22842A998FC52545C62C230D72373A27522AC01EA` |
| `combat_core_final.log` | `9/0` | `4F4097A8B69026EEBF301172A72D0CDDEFD81912C92EBC3D0CFA7B68B946C694` |
| `full_0_0_10_final.log` | `998/0` | `3E1686E9BA20280523176918B6C67298A07B7D80347B80EB4F86E055A47EDD03` |

最终证据合计 `1062/0`。每份日志均有唯一 RunTests、精确成功数、零失败、唯一成功终止标记与零 fatal/unhandled/ensure；日志审计 SHA-256 `4409A766AD53DDB0CC71F95D6B8D3A2FD2ECE930D67B964582643D1793D8CF86`。

## 流程与静态证据

- regression self-test：`355/355`；SHA-256 `9DAC2DC232838EB54DDC666B4EA1B4EC95843940E4F96701F0D4C6FB2E8B6F1B`；
- static audit：`40/40`；SHA-256 `75A2F13DB748B8130B15865D6DE7A14B313A8042A8E164DB4BECF81CA88DCCCA`；
- changed-file gate：`PASS Changed=5 Rules=1 Required=8 Logs=8`；SHA-256 `F0CB7B1AB12B5091CDB7509E2ACF347DE1AAA63EAFA43E22E15E9A692AB6F048`；
- `git diff --check`：PASS；SHA-256 `6265212BD6B0A3AA35EE192F7AFC2095A484C044C1B4C59F429669011496F12C`。

## 构建与产物

- first Editor：7 actions / native 0 / 10.01 秒；SHA-256 `7868E56357720A3086332605AF4E3C4C7D14ED8F2B7F290EB95BDDBF50805173`；
- final Editor：up to date / native 0 / 0.97 秒；SHA-256 `6CDAC6FFF83DDC247883BCF03499826B56BB59157592156EB1BCCE6F6FA4CF16`；
- final Game：4 actions / native 0 / 22.28 秒；SHA-256 `5C3FB930C808F81D759BFED30ED1179C1E67C5BD1BFA238F48BAE2FA9D54F756`；
- Editor artifact：16,498,688 bytes，SHA-256 `6ED06162C93B60E3A190E51CBE6C0E3EF15117058FD73B595AC3A4ABBDCE387A`；
- Game artifact：357,743,104 bytes，SHA-256 `249324C2E05D04079168015EEE23EF14CBAB27E778EA82E5A6BB770121E8EE66`。

## P/F

PASS：冻结配置、单次 choice/basis 读取、既有 projector/planner/sampler 组合、typed failure、确定性身份、自验证、短路行为，以及完整直接依赖回归。

未验证：World trace/collision/occlusion、真实地形落点、可见轨迹/HUD、真实输入、Editor UI/PIE/Standalone、产品可执行文件、projectile 对齐、库存/投掷/Impact、截图、Smoke、Cook 或 Package。

## 下一步

P20.31：先定义 read-only preview identity/capture policy，在不推进真实 activation sequence、不建立 reservation、不消耗物品的前提下，从现有产品会话生成 P20.30 冻结输入；身份边界明确后再接 UI presentation。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-30-thrown-weapon-arc-preview-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-30-thrown-weapon-arc-preview-composition/Docs/Report/Dev.D.UE.0.0.10.P20.30.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-30-thrown-weapon-arc-preview-composition/Docs/Log/Dev.D.UE.0.0.10.P20.30.r0_log.md>
