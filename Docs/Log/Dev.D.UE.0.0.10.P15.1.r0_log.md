# Dev.D.UE.0.0.10.P15.1.r0 Development Log

## 1. 目标

收敛仍冻结在旧 Schema、旧装备槽和旧容量语义上的 FullSystemLoop/SearchContainer regression fixtures；修正显式跨进程自动化钩子的旧 Schema 检查，并把两个历史测试文件纳入精确的改动驱动回归门禁。

## 2. 基线与实现

- 修复前 FullSystemLoop：`39 Success / 11 Fail`，日志 SHA `04F199DD8DC39039759C6037A7A7383146521D5CB7B6ABF2A45CE61821B5658E`；
- 修复前 SearchContainer：`28 Success / 2 Fail`，日志 SHA `01DB8BC54F8DC89FA86E8293404608B8D2521898D3582D712C883C056B5C294F`；
- 失败均为 fixture 漂移：Schema 3/4、future `4 -> 5`、Accessory 位置的 WindTalisman、容量 16/20、四装备槽；
- Schema 断言与 future token 改为基于 `CurrentSchemaVersion`；
- WindTalisman 改写到 SpatialRing layout；
- runtime 与 view 容量读取 canonical resolver，明确当前 `6/42/42` contract；
- equipment slot 顺序读取 canonical registry，corpse equipment 容量读取 prototype constant；
- FullSystemLoop automation reload hook 改为 current Schema；
- 新增 FullSystemLoop/SearchContainer 精确回归映射及四个正反自检；
- 正常产品路径、Schema、序列化、catalog 和 authority 未变化。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| FullSystemLoop | 50 | 0 | 0 | `00B4D56AF251F642F51B767231059A266AACCEEA3ACA12B098DB1791D8D96218` |
| SearchContainer | 30 | 0 | 0 | `3FB7CFFFA67115E8DFCC7ECF277AED0D6C0E53FC3097190DD3E26F56FEBF9DC1` |
| CodeB | 60 | 0 | 0 | `57393AAB0D11FC67370468BE65968CF8B06EF33928C5C271431757776891B8B4` |
| InputRestore.32 | 1 | 0 | 0 | `CDDB70151F801F049E6B64BECD3E2D3322FF0CD218B51E1AC36F465FFE86DBAD` |
| P5RuntimeInterface.06 | 1 | 0 | 0 | `4780E3B38CC7FF9E2F1F9118F4A43244EB10E898E5F82A36AE0A3DFC65A53228` |
| P7Integration | 9 | 0 | 0 | `0BBB88617CD6FFF7A9D7B7C900019D65B81AE3B6369977DB7B34B3A3F44E7785` |
| Profile | 211 | 0 | 0 | `07BA3F6C7256D5EA046F82B90B42CF08F6638FE212C712EE7F6D729F33F7618E` |
| V2RangedCompatibility | 22 | 0 | 0 | `F1E2D43423DCAA03DC66175985DE73B8FB95279877DCC85A4225C28DF4323D6B` |
| full 0.0.10 | 708 | 0 | 0 | `C1C14A6C66498E97569B6D29C2F318BC4F5166258FC3161A17DCEF18BBF5821B` |

聚焦 `384` Success；全量 `708/708`；九份通过日志合计 `1,092` Success、0 Fail，组间有预期重叠。全部 terminal complete、native exit `0`，Fatal/Unhandled/Ensure 为 `0`。全量约 `27m39.18s`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=4 Required=15 Logs=9
SELF_TEST: PASS 246/246
JSON_PARSE: PASS Rules=143
LEGACY_CONTRACT_SCAN: PASS
git diff --check: PASS (native exit 0)
```

第一次 self-test 在新增精确映射后因旧 unified-input fixture 缺完整 FullSystemLoop 证据而 fail closed；补父组 evidence 后通过，没有放宽规则。

## 5. 构建

- Editor initial：6 actions / 28.30s / exit `0` / log SHA `5970DD801F5DB01B9C5E2996C97D9B2C7B458E85AA6173981E78006FBC375F3E`；
- Game final：5 actions / 36.36s / exit `0` / log SHA `0BCD4568F25DB873A835A36DE4F7AE3100CC96217F9DDA3A8EC3387B104C773B`；
- Editor final：up to date / 0 actions / 0.96s / exit `0` / log SHA `770D03DB1611770ECE30FBAC26141E2CF125EDEA94E21ED7F5FF29A19E3659A3`；
- Editor DLL：14,280,704 bytes / SHA `C21E837FA8453EB47F82EB43CBF39FFE67190D8C93CE58A424AEF920882EBB64`；
- Game EXE：355,706,880 bytes / SHA `3400D56633B8AC0445FF911638614EE27F95290772E9C68446C86A18FDD612DA`。

## 6. 范围与边界

Report/Log 前 5 个源码与流程文件净变更 `+121/-27`。长期未跟踪用户文件未修改或提交；raw logs 仅本地保存。

本轮仅 P 阶段，未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。显式 cross-process FullSystemLoop hook 已编译但未做 F 阶段产品验收。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-1-legacy-regression-contract-convergence>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-1-legacy-regression-contract-convergence/Docs/Report/Dev.D.UE.0.0.10.P15.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-1-legacy-regression-contract-convergence/Docs/Log/Dev.D.UE.0.0.10.P15.1.r0_log.md>
