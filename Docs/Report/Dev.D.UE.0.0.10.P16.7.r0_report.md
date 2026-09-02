# Dev.D.UE.0.0.10.P16.7.r0 Report

## 1. 结论

P16.7 已为 Meridian Shock condition treatment 建立严格、可编码、可重放的 write-ahead recovery intent 值契约。该 intent 在 item durable prepare 与 expected condition revision 已知后、condition mutation 发生前捕获，固定同一次治疗必须使用的 Run、target、timeline sample、item、condition 与 revision 身份。

本轮刻意没有直接修改 P16.5 proof store 的 schema 1，也没有把 intent 接入 P16.6 Product Route。原因是现有磁盘中可能已经存在 schema-1 proof；若在没有 N-1 migration 的情况下直接改写 store 格式，会把有效旧证据误判为损坏。P16.7 因此只冻结可迁移的 intent contract，为下一阶段的 store schema 2 与 durable promotion 打基础，不虚称已关闭 condition 内存 mutation 与 proof durable publish 之间的 crash window。

最终结果：

```text
Recovery intent focused:     2 Success / 0 Fail
Treatment flow focused:     25 Success / 0 Fail
Mapped legacy regressions: 348 Success / 0 Fail
Shanmen.0_0_10 full:       738 Success / 0 Fail
Regression coverage:       PASS (Changed=5 / Rules=1 / Required=10 / Logs=7)
Regression gate self-test: PASS 273/273
Editor + Game Development: PASS / native status 0
```

## 2. Intent 身份与状态边界

`Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent` 保存以下完整身份：

- schema version 与 deterministic IntentId；
- TreatmentId、RunId、TargetEntityId；
- canonical fixed TimelineId、TimelineSampleId 与 TreatmentTick；
- ItemInstanceId 与唯一允许的 Meridian Stabilizing Pill L1 definition；
- Meridian Shock condition definition；
- ExpectedConditionRevision。

IntentId 由以上全部 canonical parts 通过现有 deterministic ID 工厂派生。相同输入得到相同 intent；任一 Run、target、sample、item、tick 或 revision 改变都会进入不同身份。

该值明确不是 condition treatment proof，不能证明 condition 已发生变化，也不能单独授权 item commit。库存与 item ledger 仍仅由 ShanmenItems 管理；condition 状态与已提交 proof 仍由 condition authority/store 管理。

## 3. 严格编码与恢复契约

schema 1 使用固定 13-part pipe encoding，并要求：

- exact magic 与 exact schema `1`；
- GUID 全部使用 canonical digits 格式；
- tick 与 revision 必须是 canonical decimal，拒绝 `064` 等非规范表示；
- item/condition definition 必须是当前唯一已知组合；
- TimelineId 必须由 RunId 的 fixed timeline 规则派生；
- TimelineSampleId 必须与 TimelineId、TreatmentTick 重新计算后的 sample 完全一致；
- TreatmentId 与 IntentId 必须重新派生并精确相等；
- revision 必须大于 0 且小于 `MAX_int64`，确保后续 mutation 可递增。

`TryRestore` 只从通过完整校验的 intent 重建 exact treatment intent 与 timeline sample。future schema、字段缺失/多余、integrity ID 篡改、sample 篡改、非 canonical timeline、跨 Run sample 与不可递增 revision 全部失败关闭。

## 4. 新增测试

新增 2 个 Automation tests：

- `RecoveryIntent.CodecRoundTrip`：验证同源 deterministic capture、稳定编码、严格解码以及 exact treatment/timeline 恢复；
- `RecoveryIntent.IdentityFence`：验证 future schema、篡改 ID、篡改 sample、非 canonical integer、额外字段、跨 Run sample、非 canonical Run timeline 与 revision exhaustion 均被拒绝。

单独 intent tests 为 2/0；完整 Meridian Shock treatment flow 从 P16.6 的 23 增至 25，最终 25/0；全套 `Shanmen.0_0_10` 从 736 增至 738，最终 738/0。

## 5. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Recovery intent focused | 2 | 0 | `B8D1052F596D08FC2000F3B5E662C5EF19BDF58F3846F1F1392C77ABE3C67514` |
| Treatment flow focused | 25 | 0 | `14EFB54F8BE4A966BD2640FBB18B66F225860A5D1C4FD866AB750D4791CE5A90` |
| `Shanmen.0_0_10` full | 738 | 0 | `A85D9AA7D4973F0643B68F1DB7A0889FF1D75FA9C8F3D687E216E3CF5F4B2B7B` |
| CodeB | 60 | 0 | `509D6088DE50AE6D355CB5CB8120BD7FEB06B4938A1D53594DAE3DD2A70291D4` |
| ItemEconomySchema | 24 | 0 | `F1BCC09478ECEDC8E83D924858980D1C93DBC490AD361EBE87D219476B884E99` |
| ItemUseAndArmor | 46 | 0 | `B3B0A5AE2DA882880144B69DC6704994B74DD1B1CC96AB28E2A9AC7321619E96` |
| P4 Hotbar | 7 | 0 | `28C81DA6D2D0AEF8745D892091DC85F1E3FFEB6F47E555AE0DB140335FAF3391` |
| Profile | 211 | 0 | `4CE2BF01A83C119B4CFBAE59C6491953C2749CE90EE259734BFF4F51C39E5842` |

mapped legacy 合计 348/0。Full suite 的 738 项全部成功、0 项失败，首末 Success 间约 28m01.83s。日志中的少量长 scheduler delta 均被 Automation Controller 明确忽略，对应测试最终通过，不构成失败。

## 6. 改动—回归门禁与静态边界

3 个新 intent 源文件与 2 个 regression-process 文件全部命中 `MeridianShockTreatmentProductFlow` 映射。10 个必跑组由 7 份健康日志完整覆盖：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 273/273
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS HITS=0
CONTRACT_SCAN: PASS
GIT_DIFF_CHECK: PASS
```

coverage gate output SHA 为 `03B839A1CB153800B8ECACB1FD0E742307BB0BF6EE7D28E6F4B7FC0597707E2F`；self-test output SHA 为 `7511F8052BECC706C0B779BC65250B2565B736026144521AAAFDD8607E98865A`；regression map SHA 为 `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。

静态扫描确认 intent contract 不依赖旧 ItemSubsystem/Profile、World/Actor、Spawn/ApplyDamage、UI、输入、Timer 或 RNG；contract 注释与校验明确区分 intent 与 proof，并包含 version、revision upper fence、canonical timeline 与 exact restore。

## 7. 构建证据

最终源码使用 `-WaitMutex -NoUBA -MaxParallelActions=1` 构建。Game 目标重新编译两个新实现单元并成功链接；随后 Editor 目标确认同一源码图已 up to date。两个目标原生退出码均为 0。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 4 / 24.09s | `25EEE26EDD376D546964662EB2D3757FA7503195A6EA85A4218A9D7462DC75CB` |
| Editor Development | Succeeded | 0 / 1.01s | `619797D51781558A62C3A474FB7021387DE385E137F8E06C53F6B5CEE0434A0F` |

最终产物：

- `demo_map.exe`：356,030,464 bytes，SHA-256 `8B847566B572146E8236B727624ED1F36210B4FB6C79257E520C3AD57CE1F14E`；
- `UnrealEditor-demo_map.dll`：14,685,184 bytes，SHA-256 `831E2DE552CC014E6DD32E7FA5DAB8FE0EF33B50C8DECF44C843BD4EB9B1F275`。

## 8. 修改范围

本轮新增 intent header、implementation 与 Automation tests，扩展一个 regression mapping regex，并为该映射补充正反 self-test fixture；另新增本 Report 与 Development Log，计划提交 7 个文件。

没有修改 P16.5 store schema/codec、P16.6 Product Route/Lifecycle、Profile、ShanmenItems、GameMode、PlayerController、地图、资源、存档 schema、Windows、UE Engine 或用户配置。raw logs 仅保存在本机 `Saved/Logs`。长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 9. P/F 边界

本 Report 证明 P 阶段纯值 contract、identity/codec failure fences、NullRHI focused/full/legacy regressions、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P16.7 不提供 durable storage，也不改变运行时执行顺序。因此 P16.6 已记录的极小 crash window 仍存在：condition 内存 mutation 成功、proof durable publish 尚未完成时若进程退出，当前 store 仍没有可恢复 intent。此限制不是测试失败，而是本阶段主动限制的范围。

## 10. 下一阶段

P16.8 应把现有 proof store 从 schema 1 升级为 schema 2，并提供对 schema 1 的明确 N-1 migration。schema 2 应同时保存 prepared intents 与 committed proofs，并让 intent-to-proof promotion 成为同一次 durable document mutation；损坏、future schema、重复/冲突 intent 或 proof 必须继续失败关闭。只有 durable store 完成后，后续阶段才应把 lifecycle 的 `prepare item -> record intent -> mutate condition -> promote proof -> commit item` 顺序接入产品路径。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-7-meridian-shock-recovery-intent>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-7-meridian-shock-recovery-intent/Docs/Report/Dev.D.UE.0.0.10.P16.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-7-meridian-shock-recovery-intent/Docs/Log/Dev.D.UE.0.0.10.P16.7.r0_log.md>
