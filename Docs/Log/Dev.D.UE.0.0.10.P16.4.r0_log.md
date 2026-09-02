# Dev.D.UE.0.0.10.P16.4.r0 Development Log

## 1. 目标

为 Meridian Shock treatment receipt 定义可移植、版本化、规范编码且可验证的 condition-domain proof，并让新 Product Lifecycle 在 proof 可用时跨越 transient condition/session 与 item-authority 重建边界完成 commit-only recovery；不复制库存权威，不提前建立未定义 owner 的持久化系统。

## 2. 实现

- 新增 schema 1 recovery proof，完整绑定 treatment、Run、target、timeline、item、definition、tick 与 revision；
- ProofId 对全部 canonical fields 做确定性派生，codec 严格拒绝未知 schema、非规范值和 integrity mismatch；
- condition authority 只允许向 freshly begun、inactive、无历史状态恢复 exact processed-treatment receipt；精确重复幂等，任何冲突失败关闭；
- Product Route / Session / Lifecycle 新增 proof view overload，旧无 proof API 保持兼容；
- route 先验证 supplied proof set，再与唯一 pending durable item prepare、Run correlation 和 runtime receipt 交叉核对；
- proof 匹配时恢复 condition history、重建 durable prepare 并只 commit item Quantity；active/non-fresh conflict 时 item snapshot 零变化；
- regression mapping 纳入 `RecoveryProof`，避免新文件绕过改动—测试覆盖门禁。

## 3. 新增测试

```text
Recovery.ProofCodec
Recovery.ProcessProof
Recovery.ProofConflictFence
```

关键断言：

- codec round trip 成功，未知 schema、非规范 tick、错误 ProofId 全部拒绝；
- condition reset/rebegin 与 item authority restart 后，decoded proof 恢复 1 条 condition history，item commit=1、cancel=0；
- 有效旧 proof 不能覆盖 active condition，item authority snapshot 前后完全相等。

## 4. 最终验证

| Group | Result | SHA-256 |
|---|---|---|
| Meridian Shock focused | 17/0 | `0E803295F057967FAD71694A67FE948FE278AFE6422DFB5F27CC50849503191C` |
| Shanmen.0_0_10 full | 730/0 | `640FBCBEA37BB05E4EF6A32879B5D0A47424D6FD0F5736B31BED2067318674B4` |
| CodeB | 60/0 | `04AF9447623E6DD49AA40359D89274ED1671FDAA61BC4BEC5ED07A4DA4F041E5` |
| ItemEconomySchema | 24/0 | `C7463F337454583677E557937A53F730C0B7202AB30EA45E145EC274C4D51F95` |
| ItemUseAndArmor | 46/0 | `8C99A7CF7A66AC604CB9AAE63A25777AAB74E0A73415D6219BB21CF079F9309C` |
| P4.Hotbar | 7/0 | `A548FBD4B18E50CCBADD938838BD06FFE2D29C4645BC2DAA205077DFCB159BD8` |
| Profile | 211/0 | `5D1683F99632D1D16E751E67927A69792380DB1051A3101D3FDAC75B160B6C96` |
| V3.Attributes | 4/0 | `82A13752405E8A1AC193B121403826E9B52D0824FA9D280881B3CF3441066E6B` |

mapped legacy 合计 352/0。全部日志 terminal marker=1，Fail/Fatal/Unhandled/Ensure/network=0。

```text
REGRESSION_COVERAGE: PASS Changed=12 Rules=2 Required=16 Logs=8
SELF_TEST: PASS 271/271
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS HITS=0
GIT_DIFF_CHECK: PASS
```

流程 SHA：gate `F08FF5DD79288D963B9C1D4217C1AE8D9B0EDCE1866ABD96DED3C023B2E98B4C`；self-test `633D910E13342A4270666EB4F949163E3E91F28713E333758CF51D1333BC0EF6`；mapping `CC6EB9EC8C74A3FFEC4F189F9857F654067412187C95E594BC961F948AADB18F`。

## 5. 构建

- 首次 Editor 编译暴露 `FString::Join` initializer-list 推导错误；改用显式字符串数组后重新编译成功；
- Game Development：Succeeded / 34 actions / 132.90s / native status 0 / log SHA `2E2C6281964C86614A32D96C957B281CDA1C2B10F57E5D4210CF9E886DA445A7`；
- Editor Development final check：Succeeded / target up to date / 1.15s / native status 0 / log SHA `51F05E55F01A8C8DAEAB018E659D487EAEA0D33D466693DAA549D2695EE5F9E1`；
- Game EXE：355,935,232 bytes / SHA `D77BAB50EA5031162DD4E9EECDB84A6419BA07DA6FAF70EDBC58DEDA2A543ED2`；
- Editor DLL：14,560,256 bytes / SHA `7B4643637EDDB66E49D184BE7AAE7D7D8467BB4397B47F35B955628610D8B56C`。

## 6. 边界

本轮没有新增 disk writer、第二库存 truth、checkpoint、World actor、UI、tick/timer 或随机源。Proof 只有由未来唯一 condition storage owner 原子持久化并重新提供时，才构成真实 process-crash recovery 的 durable evidence；当前验证证明的是“可信 proof 可用时”的恢复协议。

仅执行 P 阶段 C++、NullRHI Automation、静态检查与 Development builds；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户资料未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-4-meridian-shock-recovery-proof>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-4-meridian-shock-recovery-proof/Docs/Report/Dev.D.UE.0.0.10.P16.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-4-meridian-shock-recovery-proof/Docs/Log/Dev.D.UE.0.0.10.P16.4.r0_log.md>
