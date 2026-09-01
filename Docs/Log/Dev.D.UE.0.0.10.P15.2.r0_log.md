# Dev.D.UE.0.0.10.P15.2.r0 Development Log

## 1. 目标

从完整 legacy `demo_map` 基线按失败根因聚类，只修复一个边界清晰的高价值簇；保持安全边界 fail closed，并为修改文件增加精确的自动回归映射。

## 2. 基线与选择

- 完整 `demo_map`：`1246 Success / 83 Fail / 1329 Total`，queue empty，native exit `0`，SHA `1506C1E0006D476870032A802CAB80BE2F6BA1EDE7FE9083D199AC5C9DF1DA48`；
- 失败聚类：RootBoundary 27、RewardJackpot 12、RewardAffix 10、RewardBossSource 10、RareExtremeValue 9、其余 15；
- 选择最大 RootBoundary 簇，因为 26 项共享同一确定根因：当前 `Dev.D.UE.0.0.9B` 项目根遗漏于精确 allowlist；
- 未处理其它 reward、input、P6 或 lifecycle 失败。

## 3. 实现

- `bProjectRootLeafMatch` 精确加入 `Dev.D.UE.0.0.9B`；
- 保留 exact equality，不引入 prefix/wildcard；
- 新增 case 47，验证当前根通过且 `Dev.D.UE.0.0.9B.suffix` 失败；
- 新增 AutomationRootBoundary 精确回归映射；
- 新增 focused evidence 正例与 unrelated evidence 反例；
- 4 个代码/流程文件净变更 `+53/-1`。

## 4. 验证

| Evidence | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| RootBoundary isolated | 47 | 0 | 0 | `723B4872F16C178F0FC93B89FE39495F1B0AB8B624D5476FB995537DBC3FA129` |
| legacy after, same baseline command | 1273 | 57 | 0 | `292A0200BA471C4EA51307CB01147F2D7430BF2E618F9C3716DA8F0CD534808B` |
| full 0.0.10 | 712 | 0 | 0 | `5B028AA752EAE01D8FD69BF02905BEC8C8F1CC2C887616DDD1D70483FBB25DB6` |

同命令 legacy 对比净减少 26 Fail；新增一个 contract 后总数从 1329 增至 1330。无 `-UserDir` 的父组仍保留 case 26 环境失败，正确隔离参数下 focused 为 47/47。

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 250/250
JSON_PARSE: PASS Rules=145
ROOT_ALLOWLIST_SCAN: PASS
git diff --check: PASS
```

## 5. 构建

- Game：4 actions / 27.82s / exit `0` / log SHA `B3BE953FBD1CDACBB6EDA74B146D6A04D906DFB6271A76E83545AB9E63DF1475`；
- Editor：up to date / 0 actions / 1.11s / exit `0` / log SHA `AAF76AD28C7A2D1F6D63D1D3C2F699B6D0EFF253616C811B06630D231EF7C3F0`；
- Game EXE：355,738,624 bytes / SHA `B4F60074A6F5F7FD1E1256536860AF42FC05474C6E57550CC1199775B1B95BB4`；
- Editor DLL：14,323,712 bytes / SHA `0D40AC7C08A379F959048E06E152401541B1B61E361FD2E079F640CC57D3DA23`。

## 6. 审查与异常

- 不带 `-UserDir` 的第一次 focused 为 46/47；改用匹配历史任务 identity 的隔离 UserDir 后 47/47，没有降低断言。
- Windows PowerShell 5 不支持 validator 使用的 PowerShell 7 pipeline 语法；最终在既有 PowerShell 7 runtime 复跑为 250/250、exit 0。
- legacy after 的 57 Fail 被保留并明确分类；未把父组 native exit 0 误写成全绿。

## 7. 范围

仅 P 阶段 automation boundary、contract 与流程门禁；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户文件未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-2-legacy-regression-convergence>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-2-legacy-regression-convergence/Docs/Report/Dev.D.UE.0.0.10.P15.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-2-legacy-regression-convergence/Docs/Log/Dev.D.UE.0.0.10.P15.2.r0_log.md>
