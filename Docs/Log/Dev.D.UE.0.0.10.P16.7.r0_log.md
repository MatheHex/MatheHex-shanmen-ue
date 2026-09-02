# Dev.D.UE.0.0.10.P16.7.r0 Development Log

## 1. 目标

在不破坏 P16.5 schema-1 durable proof store 兼容性的前提下，先冻结 Meridian Shock treatment 的 portable write-ahead recovery intent 值契约；让下一阶段能够安全增加 schema-2 durable intent 与 N-1 migration，而不是把旧 proof 当作损坏数据。

## 2. 实现

- 新增 `Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent`；
- 在 item prepare 与 expected condition revision 已知后、condition mutation 前，从 exact treatment intent 与 fixed timeline sample 捕获 deterministic intent；
- 固定 schema、IntentId、TreatmentId、Run/target、timeline/sample、item/condition、tick 与 expected revision；
- 使用 13-part strict canonical codec，拒绝 future schema、额外字段、非 canonical integer 与 identity tampering；
- 重新派生 timeline、sample、TreatmentId 与 IntentId，所有不一致均失败关闭；
- 拒绝跨 Run sample、非 canonical Run timeline 与 `MAX_int64` revision；
- `TryRestore` 仅从完整有效 intent 恢复 exact treatment intent 与 timeline sample；
- 明确 intent 不是 treatment proof，不复制库存或 condition 真值，不单独授权 item commit。

## 3. 测试与回归映射

新增：

```text
Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryIntent.CodecRoundTrip
Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryIntent.IdentityFence
```

Regression mapping 的 ProductFlow source regex 扩展为识别 `RecoveryIntent`，self-test 增加一组应命中 fixture 与一组不得借用无关 full evidence 的 fixture。self-test 从 271 增至 273，最终 273/273。

## 4. 最终验证

| Group | Result | SHA-256 |
|---|---|---|
| Recovery intent focused | 2/0 | `B8D1052F596D08FC2000F3B5E662C5EF19BDF58F3846F1F1392C77ABE3C67514` |
| Meridian treatment focused | 25/0 | `14EFB54F8BE4A966BD2640FBB18B66F225860A5D1C4FD866AB750D4791CE5A90` |
| Shanmen.0_0_10 full | 738/0 | `A85D9AA7D4973F0643B68F1DB7A0889FF1D75FA9C8F3D687E216E3CF5F4B2B7B` |
| CodeB | 60/0 | `509D6088DE50AE6D355CB5CB8120BD7FEB06B4938A1D53594DAE3DD2A70291D4` |
| ItemEconomySchema | 24/0 | `F1BCC09478ECEDC8E83D924858980D1C93DBC490AD361EBE87D219476B884E99` |
| ItemUseAndArmor | 46/0 | `B3B0A5AE2DA882880144B69DC6704994B74DD1B1CC96AB28E2A9AC7321619E96` |
| P4.Hotbar | 7/0 | `28C81DA6D2D0AEF8745D892091DC85F1E3FFEB6F47E555AE0DB140335FAF3391` |
| Profile | 211/0 | `4CE2BF01A83C119B4CFBAE59C6491953C2749CE90EE259734BFF4F51C39E5842` |

mapped legacy 合计 348/0；最终 Automation 日志 Fail/Fatal/Ensure 均为 0，并包含 native terminal-success evidence。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 273/273
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS HITS=0
CONTRACT_SCAN: PASS
GIT_DIFF_CHECK: PASS
```

gate output SHA `03B839A1CB153800B8ECACB1FD0E742307BB0BF6EE7D28E6F4B7FC0597707E2F`；self-test output SHA `7511F8052BECC706C0B779BC65250B2565B736026144521AAAFDD8607E98865A`；mapping SHA `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。

## 5. 构建

- Game Development：Succeeded / 4 actions / 24.09s / native status 0 / log SHA `25EEE26EDD376D546964662EB2D3757FA7503195A6EA85A4218A9D7462DC75CB`；
- Editor Development：Succeeded / up to date / 1.01s / native status 0 / log SHA `619797D51781558A62C3A474FB7021387DE385E137F8E06C53F6B5CEE0434A0F`；
- Game EXE：356,030,464 bytes / SHA `8B847566B572146E8236B727624ED1F36210B4FB6C79257E520C3AD57CE1F14E`；
- Editor DLL：14,685,184 bytes / SHA `831E2DE552CC014E6DD32E7FA5DAB8FE0EF33B50C8DECF44C843BD4EB9B1F275`。

## 6. 边界与后续

本轮没有修改 durable proof store 或 Product Route，所以 intent 尚未写盘，P16.6 的 condition-mutation/proof-publish crash window 仍未关闭。P16.8 应先实现 proof store schema 1 -> 2 的 N-1 migration、prepared-intent storage 与 intent-to-proof atomic document promotion；后续再把 lifecycle 顺序接入，不跳过迁移边界。

仅执行 P 阶段 C++、NullRHI Automation、静态检查与 Development builds；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户资料未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-7-meridian-shock-recovery-intent>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-7-meridian-shock-recovery-intent/Docs/Report/Dev.D.UE.0.0.10.P16.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-7-meridian-shock-recovery-intent/Docs/Log/Dev.D.UE.0.0.10.P16.7.r0_log.md>
