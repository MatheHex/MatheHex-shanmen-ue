# Dev.D.UE.0.0.10.P16.3.r0 Development Log

## 1. 目标

在不复制库存权威、不新增 checkpoint 系统的前提下，使 Meridian Shock treatment route 能在 transient Product Session / command journal 丢失后，从 ShanmenItems durable ledger 恢复一个未 finalize 的 prepare；没有独立 condition proof 时必须零写入失败关闭。

## 2. 实现

- condition component 新增 exact processed-treatment receipt 的只读查询；
- treatment purpose 提升为 adapter 的稳定公开 discriminator；
- adapter 可从 durable prepare receipt 与独立 treatment intent 重建原请求，并要求 durable API 返回逐字段相等的 `Replayed` receipt；
- route 扫描 active Run 下尚未 finalize 的 treatment prepare，0 个为 no-op、1 个进入恢复、多个因歧义失败关闭；
- 已有 treatment receipt 时只 commit；仍 active 的同 revision condition 时先 treat 再 commit；两种证明都没有时不 commit、不 cancel；
- Session 在新 hotbar 请求与 Run teardown 前先恢复 durable ledger，再按 GUID 稳定顺序处理 runtime pending commands；
- 没有新增库存副本、事务信封、tick、timer、world actor 或 UI 路径。

## 3. 新增测试

```text
Recovery.ActiveCondition
Recovery.CommitOnlyProof
Recovery.MissingProofFence
```

关键断言：

- active-condition 恢复：`recovered=1 / commit=1 / cancel=0`；
- runtime-receipt 恢复：`recovered=1 / commit=1 / cancel=0`，不重复 treatment；
- missing-proof fence：`recovered=0 / commit=0 / cancel=0 / authority snapshot unchanged`；
- 三种情况下 reconstructed Session 均不伪造 captured hotbar request。

## 4. 最终验证

| Group | Result | SHA-256 |
|---|---|---|
| Meridian Shock focused | 14/0 | `01B14A1FC87B4F07AD097EAD7BCE8F9B0A5CBADA7FCCB5E5EA82D3F58A3B1434` |
| Shanmen.0_0_10 full | 727/0 | `A8DBBD4B570698866AC4DEE2AD4A0CF87AC9698572B715202D8D9E49B7C5CAE5` |
| CodeB | 60/0 | `0A32D7779CEF17C8E3E3B9851FA58776C38EDB0FCD5CB7B04C88D48BEF613113` |
| ItemEconomySchema | 24/0 | `1B42842493194B62AD48DBE7012AD16BD9063F22E6B976CCD51A2921EFAD81AD` |
| ItemUseAndArmor | 46/0 | `52FC46C549D856D3202E3250E5543698838C64CC8681017FCCDFBE3AA0AA07CD` |
| P4.Hotbar | 7/0 | `3F379A4180179FAF0CD760D06D5FDE9CF364D51ADC07B0DC6D27C692C76F3685` |
| Profile | 211/0 | `80342B152FAD819357549AE8D450CDD5F255D1E33D8D146287D820F90FE4502F` |
| V3.Attributes | 4/0 | `5C0372AC17D33EEDCD78431AD5DA6CEA51B248D555397E19044578F49F343B76` |

mapped legacy 合计 352/0。full 用时约 27m48.79s，queue-empty 标记 1，原生退出码 0，Fatal/Unhandled/Ensure 0，网络探测 0。

流程结果：

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=16 Logs=7
SELF_TEST: PASS 271/271
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS Files=8 Patterns=12
GIT_DIFF_CHECK: PASS
```

流程 SHA：gate `F08FF5DD79288D963B9C1D4217C1AE8D9B0EDCE1866ABD96DED3C023B2E98B4C`；self-test `633D910E13342A4270666EB4F949163E3E91F28713E333758CF51D1333BC0EF6`；mapping `DFE18FDD15197821D47AB1DAA6FC6D8C98BA56702E58FB2C6D009626558AF2A7`。

## 5. 环境诊断

首个 full 命令因 PowerShell 将未整体引用的 DPC 参数拆开，在 578/0 时主动停止；终止标记 0、`generate_204` 记录 63，SHA `CFBBC7EBD320C123961BEC424076D169948005E028DEEDED8B64B89758EC5D2F`，不计通过。

最终 full 使用单一参数 `-ForceDPCVars=HomeScreen.EnableHomeScreen=0`，完成 727/0 且网络探测为 0。没有修改 Windows、Engine 或用户配置。

## 6. 构建

- Game Development：Succeeded / 33 actions / 145.12s / native status 0 / log SHA `6A2FDC4F2872477459CBC655EFF59A5D951BA4DF71EC35A2AE56CAFA8EE06E16`；
- Editor Development：Succeeded / 30 actions / 115.44s / native status 0 / log SHA `11874FC016722947DF9DD2962D9364FDE69017483625A2EC249293CCC4BE3270`；
- Game EXE：355,909,120 bytes / SHA `7D05E9CC7024E0F645D1C2078521CECABCC3612C393CFB04E6E373D838EA2C68`；
- Editor DLL：14,526,464 bytes / SHA `73C03BC5779A8921686BB727C25FE579C15928908EA60A0DC393A7CBA7C9BD39`。

## 7. 范围

本轮仅 P 阶段 C++ ledger reconciliation、NullRHI Automation、流程检查与 Development builds。未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

本阶段恢复的是 durable item prepare 与仍可验证的 runtime condition proof 之间的事务，不是完整 process-crash recovery。缺少 condition proof 时保留 pending prepare 并失败关闭。

raw logs 仅本地保存；长期未跟踪用户资料未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-3-meridian-shock-ledger-recovery>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-3-meridian-shock-ledger-recovery/Docs/Report/Dev.D.UE.0.0.10.P16.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-3-meridian-shock-ledger-recovery/Docs/Log/Dev.D.UE.0.0.10.P16.3.r0_log.md>
