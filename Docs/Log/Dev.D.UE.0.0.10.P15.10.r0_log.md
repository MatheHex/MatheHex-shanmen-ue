# Dev.D.UE.0.0.10.P15.10.r0 Development Log

## 1. 目标

独立审计并收敛 legacy `demo_map` 父组最后一项失败 `demo_map.V3.Lifecycle.E.AtomicInventoryRollback`，同时为其源文件建立精确改动—回归映射。

## 2. 基线与根因

- 定向 baseline：`0 Success / 1 Fail / 1 Total`，SHA `FD3B955EEC4868CA64455BBC012E898ADD45D0E90CB0C4A02042C03B9B50D96F`；
- 失败断言：`fill`、`rejected`、`unchanged`；
- fixture 要求 12 个非堆叠 TrainingBlade 成功进入定义驱动的固定六格库存；
- 正式 `AddDefinition` 正确地在写入前整批拒绝 12 件请求；
- 随后空库存中的一件 TrainingVest 合法成功，导致旧断言级联失败；
- 现行 `GridInventory.08.FullSixRejectsAtomically` 已以六格容量独立证明正式原子拒绝路径；
- 判定为陈旧 fixture，production authority 无需修改。

## 3. 实现

- fixture 从 `GetInventoryCapacity()` 读取当前容量；
- 使用 TrainingBlade 精确填满全部单元格；
- 超容量添加 TrainingVest 必须返回 `InventoryFull`；
- 拒绝后验证库存身份、实例数量与 Authority Revision 不变；
- 新增 `demo_mapRunLifecycleTests.cpp -> demo_map.V3.Lifecycle` exact mapping；
- 新增映射成功覆盖与无关 full evidence 失败关闭自检；
- production C++ 零修改。

## 4. 验证

```text
Atomic baseline:          0/1
V3 Lifecycle after:      15/0
legacy demo_map after:   1330/0
Shanmen.0_0_10 full:      712/0
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 268/268
JSON_PARSE: PASS Rules=159
GIT_DIFF_CHECK: PASS
```

| Evidence | Result | SHA-256 |
|---|---|---|
| Atomic baseline | 0 Success / 1 Fail | `FD3B955EEC4868CA64455BBC012E898ADD45D0E90CB0C4A02042C03B9B50D96F` |
| V3 Lifecycle after | 15 Success / 0 Fail | `F2C28972C2B863F3A366B28F46AED669389DD611F1443299E3D86315B590FEC0` |
| legacy after | 1330 Success / 0 Fail | `65709CDA2F10ABD53699CACB894337E41ABF0A1B40963BA1DDD32DB322A9FC77` |
| full 0.0.10 | 712 Success / 0 Fail | `8D12D5D094B70D9D9B7CC090F7B59C169634317E9FAE20478BDCC7E87FCEED69` |

流程日志 SHA：gate `221B503D237007157A059A8F24A5A6C63D3CDE8EAA835CCD9303A2F78CA9A18C`；self-test `B56BBC8808B3CF4CB93F6EE72BC1A062060F697CCF435FB2D013B03420E5454E`；JSON parse `3ABE40598FB966648462AF31327C8449C10EAAEAF8B3E172E478359B2B01D8F2`；diff check `6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`。

## 5. 构建

- initial Editor：4 actions / 20.52s / status 0 / SHA `1BE88923D31E0617E9CA83FA6D67FAF3DB7747FC73332616C65FB48FA56446E9`；
- final Game：3 actions / 27.59s / status 0 / SHA `73E8EACB267F1ED2F933239BB4A4F978EAF43CD64130D5549553FBB1CE99743D`；
- final Editor：up to date / 0 actions / 1.09s / status 0 / SHA `0D458C2791288C9295C5656D1EA9EA0F729F57F0BA634799248211E013926C95`；
- Game EXE：355,744,768 bytes / SHA `D5F6DB782A6B33651E6C7B7CC4BDD428915BEAA5DED1702D1FFD41C658C903EE`；
- Editor DLL：14,330,880 bytes / SHA `02CC69008CFEF2CC9CD3EF23ABFC27C767D55514F967929CD57ECBCE78A1FDEB`。

## 6. 范围

legacy 父组当前为 `1330/0`，无剩余失败。本轮仅 P 阶段 fixture、回归门禁、NullRHI Automation、静态检查与 Development build；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户文件未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-10-atomic-inventory-fixture-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-10-atomic-inventory-fixture-regression/Docs/Report/Dev.D.UE.0.0.10.P15.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-10-atomic-inventory-fixture-regression/Docs/Log/Dev.D.UE.0.0.10.P15.10.r0_log.md>
