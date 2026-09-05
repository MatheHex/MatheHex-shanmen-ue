# Dev.D.UE.0.0.10.P20.30.r0 Report

## 1. 结论

P20.30 已在既有投掷物 Arc choice、source basis、ballistic planner 与 P20.29 preview sampler 之上增加纯值、device/UI-neutral 的预览组合层。调用方提供一份冻结配置后，组合层严格读取一次当前 choice、严格采样一次 source basis，再把投影、规划与采样结果组合成一份自验证证据。

本轮没有建立第三套轨迹或产品权威：目标与 apex 继续由既有 Arc choice projector 决定；重力、技法档位、最大飞行时间与发射速度上限继续来自产品捕获；弹道继续由 Arc planner 决定；预览点继续由 P20.29 sampler 决定。

无效配置、choice 不可用、basis 不可用、投影拒绝、规划拒绝与预览拒绝均返回独立 typed status 和诊断；较早失败会阻止后续 live read 或几何步骤。成功结果可重算 plan 与 preview，并验证确定性配置身份。

新增聚焦自动化 `7/0`，完整 0.0.10 由 `991` 增至 `998/0`。按改动文件映射运行 8 份最终自动化证据，合计 `1062/0`；Editor 与 Game Development 构建均成功。

本轮未消耗、预留或提交库存，未启动投掷，未查询 World、trace 或 collision，未创建 Actor、Component、Widget 或 renderer；也未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，未执行真实输入、可见轨迹验收、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`a9204d75fb43f745546ae15a3d9676119be3b59e`（P20.29）；
- 分支：`agent/0.0.10-p20-30-thrown-weapon-arc-preview-composition`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. 冻结配置

`Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture()` 接收：

1. 一份有效的 Arc `FShanmenCombatActionSnapshot`；
2. 一份有效且定义为 Arc 的现有 thrown-weapon product capture；
3. 一份有效的现有 Arc choice policy；
4. 一个处于 sampler `[2, 128]` 闭区间内的 segment count。

捕获只投影预览所需字段：action、产品 Arc policy、产品定义的发射速度上限、choice policy 与 segment count。配置不持有库存、reservation、输入设备、World、Actor、renderer 或 launch 状态。

Configuration ID 使用命名空间 `demo_map.ShanmenThrownWeapon.ArcPreviewConfiguration.r1`，由 action 身份与内容版本、Arc policy、最大速度、choice policy ID 和 segment count 规范派生。`IsValid()` 会重算该身份；复用输出在失败时清空。

## 4. 单次组合流程

`Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose()` 的顺序固定为：

1. 先验证冻结配置；无效时两个 callback 均不调用；
2. 调用 `ReadCurrentChoice()` 一次；无效时不采样 basis；
3. 调用 `SampleSourceBasis()` 一次；无效时不投影或规划；
4. 使用现有 `Fdemo_mapShanmenThrownWeaponArcChoiceProjector` 投影 target 与 apex；
5. 使用现有 `FShanmenThrownWeaponArcPlanner` 建立 plan；
6. 使用现有 `FShanmenThrownWeaponArcPreviewSampler` 生成有界位置序列。

成功结果保留冻结配置、读取到的 choice、basis、projection、plan result、preview，以及两个 live-read count。该结构只是一份同步值证据，不拥有更新循环或缓存。

## 5. Typed 失败与短路

结果状态为：

- `ConfigurationRejected`：冻结配置无效，live read 为 `0/0`；
- `ChoiceUnavailable`：choice 读取一次后无效，basis 不读取；
- `BasisUnavailable`：choice 与 basis 各读取一次，几何不执行；
- `ProjectionRejected`：保留 projector 的精确状态与诊断；
- `PlanRejected`：保留 planner 的精确状态与诊断；
- `PreviewRejected`：有效 plan 无法形成 sampler 输出时失败关闭；
- `Composed`：projection、plan 与 preview 全部有效且相互匹配。

失败结果不伪装为成功预览，也不会把 choice/basis 缺失混写成弹道不可达。

## 6. 自验证与权威边界

成功结果的 `IsValid()` 会用冻结配置、basis 与 projection 重建 planner capture，重新调用纯 planner，并按 segment count 重新调用纯 sampler；只有 plan result 与 preview 均逐值匹配时才有效。

`PlanRejected` 结果也会重算 planner 结果，防止调用方把任意拒绝状态伪装成有效证据。`Matches()` 比较 typed status、诊断、live-read count、配置、choice、basis、projection、plan 与 preview。

组合层没有复制 ballistic 数学、choice 几何或 preview 采样公式；产品字段、几何投影、弹道与采样仍分别由既有唯一权威负责。

## 7. 新增自动化覆盖

`Shanmen.0_0_10.Product.ThrownWeaponArcPreviewComposition` 新增 7 项无头自动化：

1. `CanonicalComposition`：单次读取、标准 target/apex、plan-preview 绑定与端点；
2. `DeterministicReplay`：相同输入逐值重放，segment resolution 改变身份；
3. `ConfigurationFences`：action/product 匹配、segment 上下界及无效配置零读取；
4. `ChoiceUnavailable`：choice 无效时 basis callback 不调用；
5. `BasisUnavailable`：basis 无效时 projection/planner/sampler 不执行；
6. `ProjectionReasons`：非 Arc 与缺少 target 的 projector 原因原样保留；
7. `PlanReason`：发射速度不足时保留 planner `Unreachable` 结果。

首轮 Editor 编译与首轮聚焦自动化均直接成功，没有生产或测试修复轮。

## 8. 改动文件回归映射

新增 `ThrownWeaponArcPreviewComposition` changed-file 规则，要求同时提供：

- `Product.ThrownWeaponArcPreviewComposition`；
- `Product.ThrownWeaponArcChoiceProjection`；
- `Product.ThrownWeaponProductController`；
- `CombatRuntime.ThrownWeaponPreview.Arc`；
- `CombatRuntime.ThrownWeaponArc`；
- `CombatRuntime.ThrownWeapon`；
- `CombatCore`；
- broad `Shanmen.0_0_10`。

正反映射 self-test 由 `353` 增至 `355/355`。最终 changed-file gate 对 5 个生产、测试与流程路径求并集：`Changed=5 Rules=1 Required=8 Logs=8`，全部具备健康证据。

## 9. 自动化、静态、构建与产物

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_preview_composition_final.log` | `Product.ThrownWeaponArcPreviewComposition` | `7/0` | `E073C1F555CF164BF525F2647C16FA2B2BB4B3719C6C4F165132206C5EF94A66` |
| `arc_choice_projection_final.log` | `Product.ThrownWeaponArcChoiceProjection` | `5/0` | `EB69B35FB443E0D126798BA900DD631E7B26CE07486795EAD43B1FC83D2F0D52` |
| `thrown_product_controller_final.log` | `Product.ThrownWeaponProductController` | `7/0` | `D8861801950FC46BFC779976942206A4E0DBC69309A71FA784BBC1F208EE6240` |
| `arc_preview_sampler_final.log` | `CombatRuntime.ThrownWeaponPreview.Arc` | `6/0` | `6F0DD10D8A681D83EC6A015B9962F6E239F0232BD2A0243F3787311C904000CB` |
| `arc_planner_final.log` | `CombatRuntime.ThrownWeaponArc` | `10/0` | `5F009C02F08A09CAF3988B1969ECE8E1E0C6ADACC11F9385A2E4E4C04B762A18` |
| `thrown_weapon_runtime_final.log` | `CombatRuntime.ThrownWeapon` | `20/0` | `55ED77D19B83421F01B112E22842A998FC52545C62C230D72373A27522AC01EA` |
| `combat_core_final.log` | `CombatCore` | `9/0` | `4F4097A8B69026EEBF301172A72D0CDDEFD81912C92EBC3D0CFA7B68B946C694` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `998/0` | `3E1686E9BA20280523176918B6C67298A07B7D80347B80EB4F86E055A47EDD03` |

最终证据合计 `1062/0`。每份日志均有唯一 RunTests、精确成功数、零失败、唯一 UE 5.8 原生成功终止标记与零 fatal/unhandled/ensure；日志审计 SHA-256 为 `4409A766AD53DDB0CC71F95D6B8D3A2FD2ECE930D67B964582643D1793D8CF86`。

- regression self-test：`355/355`，SHA-256 `9DAC2DC232838EB54DDC666B4EA1B4EC95843940E4F96701F0D4C6FB2E8B6F1B`；
- static audit：`40/40`，SHA-256 `75A2F13DB748B8130B15865D6DE7A14B313A8042A8E164DB4BECF81CA88DCCCA`；
- changed-file gate：`PASS Changed=5 Rules=1 Required=8 Logs=8`，SHA-256 `F0CB7B1AB12B5091CDB7509E2ACF347DE1AAA63EAFA43E22E15E9A692AB6F048`；
- `git diff --check`：PASS，SHA-256 `6265212BD6B0A3AA35EE192F7AFC2095A484C044C1B4C59F429669011496F12C`；
- first Editor：7 actions / native 0 / 10.01 秒，SHA-256 `7868E56357720A3086332605AF4E3C4C7D14ED8F2B7F290EB95BDDBF50805173`；
- final Editor：up to date / native 0 / 0.97 秒，SHA-256 `6CDAC6FFF83DDC247883BCF03499826B56BB59157592156EB1BCCE6F6FA4CF16`；
- final Game：4 actions / native 0 / 22.28 秒，SHA-256 `5C3FB930C808F81D759BFED30ED1179C1E67C5BD1BFA238F48BAE2FA9D54F756`。

产物：

- `Binaries/Win64/UnrealEditor-demo_map.dll`：16,498,688 bytes，SHA-256 `6ED06162C93B60E3A190E51CBE6C0E3EF15117058FD73B595AC3A4ABBDCE387A`；
- `Binaries/Win64/demo_map.exe`：357,743,104 bytes，SHA-256 `249324C2E05D04079168015EEE23EF14CBAB27E778EA82E5A6BB770121E8EE66`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：冻结 Arc preview 配置、单次 choice/basis 读取、既有 projector/planner/sampler 组合、typed unavailable/rejection、确定性身份、自验证、短路行为，以及完整 0.0.10 与全部直接依赖回归。

未验证：World trace/collision/occlusion、真实地形落点、可见轨迹/HUD marker、真实设备输入、Editor UI/PIE/Standalone、产品可执行文件、真实 projectile 对齐、库存 reserve/consume、投掷/Impact、截图、Smoke、Cook 或 Package。Development 构建与无头自动化不能描述为可见产品验收。

建议 P20.31 先建立 read-only preview identity/capture policy：从现有产品会话取得 P20.30 所需冻结输入，但不得推进真实 activation sequence、创建 reservation 或消耗物品；明确 preview identity 与未来真实 action identity 的隔离后，再接入 UI presentation。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-30-thrown-weapon-arc-preview-composition>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-30-thrown-weapon-arc-preview-composition/Docs/Report/Dev.D.UE.0.0.10.P20.30.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-30-thrown-weapon-arc-preview-composition/Docs/Log/Dev.D.UE.0.0.10.P20.30.r0_log.md>
