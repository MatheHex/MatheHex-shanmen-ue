# Dev.D.UE.0.0.10.P20.29.r0 Log

## 阶段

- 任务：P20.29 thrown-weapon Ballistic Arc deterministic preview sampler；
- 基线：`9761198876f8ed8f3ee8344786f0fd3d63c0a3b5`；
- 分支：`agent/0.0.10-p20-29-thrown-weapon-arc-preview-sampler`；
- 边界：P 阶段，无 Editor UI/PIE/Standalone/产品启动/真实输入/可见轨迹/截图/Smoke/Cook/Package。

## 实现记录

1. 新增不可变 `FShanmenThrownWeaponArcPreview` 与纯值 `FShanmenThrownWeaponArcPreviewSampler`。
2. 只接受有效 Arc plan；segment count 严格限制为 `[2, 128]`。
3. 输出严格包含 `SegmentCount + 1` 个位置，中间点按飞行时间均匀采样。
4. 复用唯一 `Plan.TrySamplePosition()`，未复制 Arc 公式。
5. 首尾点直接写入 plan origin 与 requested target，避免端点浮点漂移。
6. apex、planned landing 与 flight time 从 plan 复制；Preview ID 由 plan ID 与 segment count 确定性派生。
7. `IsValid()` 重算身份与全部期望采样点；失败时先清空复用输出。
8. 未加入 World、trace、collision、Actor、Component、Widget、renderer、timer、tick、输入或库存权威。
9. 新增 6 项纯值自动化，完整 0.0.10 总数 `985→991`。
10. changed-file map 新增 Arc preview 规则；正反 self-test `351→353`。
11. 103 个历史无关未跟踪文件保持未暂存、未修改、未删除。

## 有界证据修复

首次聚焦自动化为 `5/1`；失败仅来自 `HeightVariants` 对理论 apex Z `500/250` 使用精确 double 相等比较。生产 sampler、plan 公式与实际 landing 均正确。

保留失败日志后，只把该测试断言改为 `FMath::IsNearlyEqual(..., 1.0e-6)`；未改生产代码。after-fix Editor 构建成功，最终聚焦组 `6/0`。失败日志 SHA-256：`389A25098EB00B157EA80A933A757A40EC86DD5915EEF4157A35BA4DFEF0B69D`。

## 最终自动化证据

| Log | Success/Fail | SHA-256 |
|---|---:|---|
| `arc_preview_final.log` | `6/0` | `DB50A3FCF7564F8DD6F5D19D7B112C3676551B11FE7D6DB0A95FC35BB32725B8` |
| `arc_planner_final.log` | `10/0` | `1A1C4294E689F6B458282DCF790A3DADB522D587C384952F54CDEFE52D8E147E` |
| `thrown_weapon_runtime_final.log` | `20/0` | `3C3600B894B9CF68C5F98D0440EB068F9D4C4244E54CD1FE55F64C90F04730FB` |
| `combat_core_final.log` | `9/0` | `76AD81D010A45A4EE58F0163F1A084714303673C092FAFEBA10DFFEF2B1BB644` |
| `full_0_0_10_final.log` | `991/0` | `1F4EC8DEDC071F6C2BE336FABF1C5AF2969C77C69C1B93A6740CB20A6D665663` |

最终证据合计 `1036/0`。每份日志均有唯一 RunTests、精确成功数、零失败、唯一成功终止标记与零 fatal/unhandled/ensure；日志审计 SHA-256 `078C9486A0A40A5FF44CA75D27BDFADB07CF9583E9809662A964DB2B57D81910`。

## 流程与静态证据

- regression self-test：`353/353`；SHA-256 `D81B3A22F0EB6C6547AB92BBE19BE1938AEFBD6B8D7C90DA3978ADAD57ED6D78`；
- static audit：`34/34`；SHA-256 `DFA8CAE3077E309C990AFF1A79A79C4EB48755DF76CE0582946E1FB6A7DDC875`；
- changed-file gate：`PASS Changed=5 Rules=2 Required=5 Logs=5`；SHA-256 `EC6F4D90B05FEC2B215BC5590010FA720CE6DD4286C8A95724BF52F203FD8605`；
- `git diff --check`：PASS；SHA-256 `99947C1C3C0A821424EB0D5E35D1EC38CFCB65426ABDB29BF8518DD08F82CF55`。

## 构建与产物

- first Editor：7 actions / native 0 / 10.19 秒；SHA-256 `B9A1759F49CF6AC4752A6ACC989E5758C5A96B02D279BE6061D3434AB28275F4`；
- after-fix Editor：4 actions / native 0 / 4.87 秒；SHA-256 `D25901172B45BB61DB0A8393C97BFBC1DF66A8AC7491E68D39DBF179AE1CDFBC`；
- final Editor：up to date / native 0 / 0.92 秒；SHA-256 `5BD43FD848609CD9206ABAC2EC0D4D60BCF4853422E23D459DAE26B39AEFDB92`；
- final Game：4 actions / native 0 / 21.57 秒；SHA-256 `37E9D4007177A74468A254C4CB48F9B3B70CAF84E7898859B23D38256FF75471`；
- Editor artifact：2,039,808 bytes，SHA-256 `216F5603D6D25082322E424AE20974C9040E0B0C0EC2B31EDA7AED6512637A35`；
- Game artifact：357,709,312 bytes，SHA-256 `8A159AFC3B2BE469720EAC74E12C54F0A6EC0D2DBA5569FA72947D9DF4EE6D86`。

## P/F

PASS：纯值 Arc preview、均匀时间采样、segment 边界、精确端点、元数据、确定性身份、自验证、失败关闭，以及完整 Arc/直线投掷/Core 回归。

未验证：World trace/collision/occlusion、真实地形落点、可见轨迹/HUD、真实输入、Editor UI/PIE/Standalone、projectile 对齐、库存/投掷/Impact、截图、Smoke、Cook 或 Package。

## 下一步

P20.30：增加 device/UI-neutral Arc preview composition，单次读取现有 Arc choice 与 source basis，复用现有 planner 和 P20.29 sampler，并返回 typed unavailable reason；仍不消耗库存、不启动投掷、不查询 World、不 trace、不渲染。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p20-29-thrown-weapon-arc-preview-sampler>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-29-thrown-weapon-arc-preview-sampler/Docs/Report/Dev.D.UE.0.0.10.P20.29.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p20-29-thrown-weapon-arc-preview-sampler/Docs/Log/Dev.D.UE.0.0.10.P20.29.r0_log.md>
