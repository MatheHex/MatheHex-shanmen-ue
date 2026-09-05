# Dev.D.UE.0.0.10.P20.29.r0 Report

## 1. 结论

P20.29 已在既有 `FShanmenThrownWeaponArcPlan` 之上增加纯值、不可变、确定性的 Ballistic Arc preview sampler。它按固定且有界的 segment 数均匀采样飞行时间，复用 plan 的唯一轨迹采样公式，不复制抛物线数学；首点与末点分别精确固定为 origin 与 requested target。

成功结果携带 plan、segment count、`SegmentCount + 1` 个 world-space position、apex、planned landing、flight time 与确定性 Preview ID。结果可自行重建采样点并验证身份；无效 plan、越界 segment 或采样失败均失败关闭并清空复用输出。

新增聚焦自动化 `6/0`，完整 0.0.10 由 `985` 增至 `991/0`。按改动文件映射运行 5 份最终自动化证据，合计 `1036/0`；Editor 与 Game Development 构建均成功。

本轮没有查询 World、trace 或 collision，没有创建 Actor、Component、Widget、renderer、timer 或 mutable runtime authority；也没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，没有执行真实输入、可见轨迹验收、截图、Smoke、Cook 或 Package。

## 2. 基线与分支

- 基线提交：`9761198876f8ed8f3ee8344786f0fd3d63c0a3b5`（P20.28）；
- 分支：`agent/0.0.10-p20-29-thrown-weapon-arc-preview-sampler`；
- 引擎：Unreal Engine 5.8；
- 平台：Win64 Development。

## 3. Preview 契约

`FShanmenThrownWeaponArcPreviewSampler::TrySample()` 只接收：

1. 一份已经有效的 `FShanmenThrownWeaponArcPlan`；
2. 一个 `[2, 128]` 闭区间内的 segment count；
3. 一份可复用的输出值。

成功时输出点数严格为 `SegmentCount + 1`。中间点使用：

`ElapsedSeconds = FlightTimeSeconds * Index / SegmentCount`

然后调用既有 `Plan.TrySamplePosition()`。Index `0` 与 `SegmentCount` 不依赖浮点近似，分别直接写入 plan 的 origin 与 requested target。

## 4. 身份、元数据与自验证

Preview ID 使用命名空间 `Shanmen.ThrownWeapon.ArcPreview.r1`，由 plan ID 与 segment count 规范派生。相同 plan 与分辨率可逐值重放；改变 segment count 会改变 ID 与采样点数，但不会改变 planned landing。

`IsValid()` 会检查：

- plan、Preview ID 与 segment 边界；
- 点数、有限数值、精确首尾点；
- apex、planned landing 与 flight time 元数据；
- 由 plan ID 和 segment count 重算出的 Preview ID；
- 每个采样点与 plan 在对应时间上的重建结果。

因此调用方不能把任意点列伪装成有效 preview，也不能让复用输出在失败后残留旧值。

## 5. 权威边界

本阶段只建立空间预览数据，不建立显示或物理权威：

- 不读取 World，不做 line trace、sweep、overlap 或 collision；
- 不判断遮挡、可达性、地形落点或命中；
- 不创建 Actor、Component、Widget 或渲染资源；
- 不拥有 tick、timer、缓存、输入或 choice revision；
- 不消耗库存、不启动投掷、不产生 Impact。

轨迹数学继续由 P20.25/P20.26 的有效 Arc plan 独占；P20.29 只是有界采样视图。

## 6. 新增自动化覆盖

`Shanmen.0_0_10.CombatRuntime.ThrownWeaponPreview.Arc` 新增 6 项无头自动化：

1. `CanonicalEndpoints`：元数据、点数与精确首尾点；
2. `UniformSamples`：逐点对照既有 plan 的均匀时间采样；
3. `DeterministicIdentity`：相同输入精确重放，分辨率参与身份；
4. `HeightVariants`：终点高于或低于起点时仍保留正确 apex 与 landing；
5. `SegmentFences`：`2` 与 `128` 可用，边界外失败关闭；
6. `InvalidPlanFailsClosed`：无效 plan 清空全部复用输出字段。

测试只验证纯值契约，没有把无头结果描述为可见轨迹或真实物理验收。

## 7. 改动文件回归映射与有界修复

新增 `ThrownWeaponArcPreview` changed-file 规则，要求同时提供：

- `Shanmen.0_0_10.CombatRuntime.ThrownWeaponPreview.Arc`；
- `Shanmen.0_0_10.CombatRuntime.ThrownWeaponArc`；
- `Shanmen.0_0_10.CombatRuntime.ThrownWeapon`；
- `Shanmen.0_0_10.CombatCore`；
- broad `Shanmen.0_0_10.CombatRuntime`（由完整 0.0.10 证据覆盖）。

正反映射 self-test 由 `351` 增至 `353/353`。最终 changed-file gate 对 5 个生产、测试与流程路径求并集：`Changed=5 Rules=2 Required=5 Logs=5`，全部具备健康证据。

第一次聚焦自动化得到 `5/1`：`HeightVariants` 使用精确 double 相等比较预期 apex Z `500/250`，而生产值仅有浮点舍入差。保存首次失败日志后，只把测试断言改为 `1.0e-6` 容差；没有修改生产公式或放宽生产契约。重编译后聚焦组为 `6/0`。首次失败日志 SHA-256 为 `389A25098EB00B157EA80A933A757A40EC86DD5915EEF4157A35BA4DFEF0B69D`。

## 8. 自动化结果

| Log | Group | Success/Fail | SHA-256 |
|---|---|---:|---|
| `arc_preview_final.log` | `CombatRuntime.ThrownWeaponPreview.Arc` | `6/0` | `DB50A3FCF7564F8DD6F5D19D7B112C3676551B11FE7D6DB0A95FC35BB32725B8` |
| `arc_planner_final.log` | `CombatRuntime.ThrownWeaponArc` | `10/0` | `1A1C4294E689F6B458282DCF790A3DADB522D587C384952F54CDEFE52D8E147E` |
| `thrown_weapon_runtime_final.log` | `CombatRuntime.ThrownWeapon` | `20/0` | `3C3600B894B9CF68C5F98D0440EB068F9D4C4244E54CD1FE55F64C90F04730FB` |
| `combat_core_final.log` | `CombatCore` | `9/0` | `76AD81D010A45A4EE58F0163F1A084714303673C092FAFEBA10DFFEF2B1BB644` |
| `full_0_0_10_final.log` | `Shanmen.0_0_10` | `991/0` | `1F4EC8DEDC071F6C2BE336FABF1C5AF2969C77C69C1B93A6740CB20A6D665663` |

最终证据合计 `1036/0`。日志审计逐份核对唯一 RunTests 命令、精确成功数、零失败、唯一 UE 5.8 原生成功终止标记及零 fatal/unhandled/ensure；审计日志 SHA-256 为 `078C9486A0A40A5FF44CA75D27BDFADB07CF9583E9809662A964DB2B57D81910`。

## 9. 静态、流程、构建与产物

- regression self-test：`353/353`，SHA-256 `D81B3A22F0EB6C6547AB92BBE19BE1938AEFBD6B8D7C90DA3978ADAD57ED6D78`；
- static audit：`34/34`，验证有界 segment、唯一 plan sampler 调用、精确端点、确定性身份、元数据、自验证、六项测试及无 World/Actor/trace/random authority，SHA-256 `DFA8CAE3077E309C990AFF1A79A79C4EB48755DF76CE0582946E1FB6A7DDC875`；
- changed-file gate：`PASS Changed=5 Rules=2 Required=5 Logs=5`，SHA-256 `EC6F4D90B05FEC2B215BC5590010FA720CE6DD4286C8A95724BF52F203FD8605`；
- `git diff --check`：PASS，SHA-256 `99947C1C3C0A821424EB0D5E35D1EC38CFCB65426ABDB29BF8518DD08F82CF55`；
- first Editor：7 actions / native 0 / 10.19 秒，SHA-256 `B9A1759F49CF6AC4752A6ACC989E5758C5A96B02D279BE6061D3434AB28275F4`；
- after-fix Editor：4 actions / native 0 / 4.87 秒，SHA-256 `D25901172B45BB61DB0A8393C97BFBC1DF66A8AC7491E68D39DBF179AE1CDFBC`；
- final Editor：up to date / native 0 / 0.92 秒，SHA-256 `5BD43FD848609CD9206ABAC2EC0D4D60BCF4853422E23D459DAE26B39AEFDB92`；
- final Game：4 actions / native 0 / 21.57 秒，SHA-256 `37E9D4007177A74468A254C4CB48F9B3B70CAF84E7898859B23D38256FF75471`。

产物：

- `Binaries/Win64/UnrealEditor-ShanmenCombatRuntime.dll`：2,039,808 bytes，SHA-256 `216F5603D6D25082322E424AE20974C9040E0B0C0EC2B31EDA7AED6512637A35`；
- `Binaries/Win64/demo_map.exe`：357,709,312 bytes，SHA-256 `8A159AFC3B2BE469720EAC74E12C54F0A6EC0D2DBA5569FA72947D9DF4EE6D86`。

## 10. P/F 边界、下一步与 GitHub

PASS 范围：有效 Arc plan 的纯值确定性采样、segment 上下界、精确端点、apex/landing/flight 元数据、身份重放、无效输入失败关闭、完整 0.0.10 与受影响 Arc/直线投掷/Core 回归。

未验证：World trace/collision/occlusion、真实地形落点、可见轨迹/HUD marker、真实设备输入、Editor UI/PIE/Standalone、产品可执行文件、真实 projectile 对齐、投掷/库存/Impact、截图、Smoke、Cook 或 Package。Development 构建与无头自动化不能描述为可见产品验收。

建议 P20.30 增加 device/UI-neutral Arc preview composition：单次读取现有 Arc choice 与 source basis，构造既有 Arc plan，并投影 P20.29 preview；失败时返回 typed unavailable reason。该阶段仍不消耗库存、不启动投掷、不查询 World、不 trace、不渲染。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-29-thrown-weapon-arc-preview-sampler>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-29-thrown-weapon-arc-preview-sampler/Docs/Report/Dev.D.UE.0.0.10.P20.29.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-29-thrown-weapon-arc-preview-sampler/Docs/Log/Dev.D.UE.0.0.10.P20.29.r0_log.md>
