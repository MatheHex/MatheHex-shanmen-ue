# Dev.D.UE.0.0.10.P15.9.r0 Development Log

## 1. 目标

收敛 P6 Integration 两项失败及其同源 P1 Sect Navigation 失败，并为两个 legacy fixture 建立精确改动—回归映射。

## 2. 基线与根因

- P6 Integration baseline：`0 Success / 2 Fail / 2 Total`，SHA `D27507F1B1308DA6289AB40D97D28DDF834F073769794BB2E91E08B3A82ACB8F`；
- P15.8 legacy baseline：`1326 Success / 4 Fail / 1330 Total`，包含 P1 WidgetSmoke 失败；
- ProductLoop fixture 把 SpatialRing Definition `WindTalisman` 写入旧 Accessory field，BeginRun 正确拒绝 slot mismatch；
- P1/P6 widget fixtures 使用 null lifecycle manager，却期待已由 P23 退役的内部 Warehouse page；
- 当前正式入口由共享 Code B Profile Host 拥有，Sect widget 在 external workspace 路径保持 Teleport page；
- 因此只修 fixture，不恢复旧仓库 authority 或产品路由。

## 3. 实现

- WindTalisman 改写入 `SpatialRingItemInstanceId`；
- P1/P6 route fixture 验证 null-manager external entry 保持 Teleport、给出可见诊断、普通返回回 Home；
- Home fallback 与 Town route 既有断言继续保留；
- 新增 `demo_mapSectNavigationTests.cpp -> demo_map.P1.SectNavigation` exact mapping；
- 新增 `demo_mapP6IntegrationClosureTests.cpp -> demo_map.P6.Integration` exact mapping；
- 每条映射新增成功覆盖与无关 full evidence 失败关闭自检；
- production C++ 零修改。

## 4. 验证

```text
P6 Integration:      0/2 -> 2/0
P1 Sect Navigation:  legacy fail -> 2/0
legacy demo_map:      1326/4 -> 1329/1, Total=1330
Shanmen.0_0_10:       712/0
REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=2 Logs=2
SELF_TEST: PASS 266/266
JSON_PARSE: PASS Rules=158
git diff --check: PASS
```

| Evidence | Result | SHA-256 |
|---|---|---|
| P6 baseline | 0 Success / 2 Fail | `D27507F1B1308DA6289AB40D97D28DDF834F073769794BB2E91E08B3A82ACB8F` |
| P6 after | 2 Success / 0 Fail | `75BC2EACF8F1CD4CCE2B81FD14E5AACD99F6967EC7DA8CAD32CB296F8CFBFB4D` |
| P1 after | 2 Success / 0 Fail | `A38FFE486C7F005E485D4914FD122E2D023D77140D45D93EC452FF19218BAC26` |
| legacy after | 1329 Success / 1 Fail | `BDB8D983CFF291DCDBC5F4E86797A8E7FCAFFB2DCDCAD897BBEE5FE6E9C6283F` |
| full 0.0.10 | 712 Success / 0 Fail | `D8B9D4236AFD56681A671B9EA722760DF4F9F619AB32E133A0796E790A8C1EF3` |

流程日志 SHA：gate `F802D95873DDDE2A06D53B15082DE42FF3A49433FAAFC0AAA4077DF642FCCCCC`；self-test `1651F73DE463C0E21FD8F3B3B61B98719940E84658AD71E5FD7371DE842881C2`；JSON parse `22E3C2784AB6C74B73419A315E2C096E72187E41B2A4B511FD5A4D1B0AAAB449`。

## 5. 构建

- initial Editor：5 actions / 20.61s / status 0 / SHA `416DE9C8F507550C1BA7825A516088BF5857567FFB3181F2A906D44821287D68`；
- final Game：4 actions / 26.02s / status 0 / SHA `11B1017B62F2B233408F3D94C3E6E8472A8CBEF7B88E8102033F219B8D46E116`；
- final Editor：up to date / 0 actions / 1.04s / status 0 / SHA `1CB00F7711A5F6DD3002BC8B3D697BBF0A8D19050E953A2B19BE6455C727D412`；
- Game EXE：355,744,256 bytes / SHA `73D6F059880495F29907EEC1FED27A08DC2C2468990527F1202EB95BAE034497`；
- Editor DLL：14,330,368 bytes / SHA `795A66A631193BFC95E9F14CCEB8BFB812ACC1603DEE7ADECCA34F868F61A544`。

## 6. 剩余失败与范围

legacy 父组仅剩 `demo_map.V3.Lifecycle.E.AtomicInventoryRollback` 一项。它未被跳过、重分类或归入本阶段。

本轮仅 P 阶段 fixture、回归门禁、NullRHI Automation、静态检查与 Development build；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户文件未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-9-p6-integration-contract-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-9-p6-integration-contract-regression/Docs/Report/Dev.D.UE.0.0.10.P15.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-9-p6-integration-contract-regression/Docs/Log/Dev.D.UE.0.0.10.P15.9.r0_log.md>
