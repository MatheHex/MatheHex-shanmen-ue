# Dev.D.UE.0.0.10.P15.9.r0 Report

## 1. 结论

P15.9 完成三项同源 legacy integration fixture 的有界收敛：P6 ProductLoop 的 starter layout 重新遵守当前五装备角色契约；P6 SectRouteClosure 与 P1 WidgetSmoke 不再把已经退役的 Sect 内置 Warehouse page 当成正式外部仓库入口的页面状态。

定向结果：

```text
demo_map.P6.Integration:     0 Success / 2 Fail -> 2 Success / 0 Fail
demo_map.P1.SectNavigation:  P15.8 legacy 中 1 Fail -> 2 Success / 0 Fail
```

legacy `demo_map` 父组从 P15.8 的 `1326 Success / 4 Fail / 1330 Total` 收敛为 `1329 Success / 1 Fail / 1330 Total`，精确消除本阶段三项失败。`Shanmen.0_0_10` 全量保持 `712 Success / 0 Fail / 712 Total`。

本阶段 production C++ 零修改；没有恢复旧 Warehouse authority、旧内置页面写路径或第二库存系统。

## 2. 根因

### ProductLoopPersistence

fixture 的 `BuildStarterLayout()` 把 `WindTalisman` 写入 `AccessoryItemInstanceId`。当前 Definition 明确把 WindTalisman 定义为 SpatialRing，并只兼容 `SpatialRingSlot`。Preparation commit 保留了这份旧布局，随后 BeginRun 的正式 slot compatibility gate 正确拒绝，因此没有产生 ActiveRun。

同仓库现行 FullSystemLoop fixture 已使用 `SpatialRingItemInstanceId`；P6 Integration 仍停留在迁移前写法。

### SectRouteClosure 与 WidgetSmoke

P23 已把正式宗门 Warehouse 入口切换到共享 Code B Profile Host：Sect widget 调用 `OpenCodeBOutOfRaidInventory()`，外部 workspace 负责显示仓库；Sect navigation 本身不再切换到旧内置 `Warehouse` page。

两份旧 fixture 都以 `LifecycleManager == nullptr` 构造 widget，却仍期待：

```text
Teleport -> retired internal Warehouse page -> Teleport
```

当前 fail-closed 行为是：保持已选 Teleport page，显示不可用诊断，不伪造外部 workspace 已打开；随后普通 Sect 返回动作回 Home。旧断言因此与正式路由和自身 null-manager 前提同时冲突。

## 3. 实现

### P6 Product Loop

`BuildStarterLayout()` 现在把 WindTalisman 的稳定 ItemInstanceId 写入 `SpatialRingItemInstanceId`。BeginRun 继续通过正式 Definition/slot compatibility gate，测试没有放宽产品校验。

### Sect 路由 fixtures

P1 与 P6 两份 fixture 统一验证：

- external warehouse entry 不伪造旧内部 Warehouse page；
- null-manager 路径保持 TeleportArray；
- 拒绝路径提供非空可见诊断；
- 随后的普通 Sect 返回动作回到 Home；
- Home→legacy Warehouse 的隔离 fallback 与 Town route 既有断言仍保留。

这只修正测试对页面所有权的认识，没有修改正式 widget、manager 或 Code B workspace。

## 4. 回归覆盖门禁

为此前未精确映射的两个 fixture 新增规则：

```text
Source/demo_map/demo_mapSectNavigationTests.cpp
  -> demo_map.P1.SectNavigation

Source/demo_map/demo_mapP6IntegrationClosureTests.cpp
  -> demo_map.P6.Integration
```

流程自检为每条规则各增加一正一反：对应 exact suite 可以覆盖；无关 `Shanmen.0_0_10` full evidence 不能替代。最终证据：

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=2 Logs=2
SELF_TEST: PASS 266/266
JSON_PARSE: PASS Rules=158
git diff --check: PASS
```

流程日志 SHA：gate `F802D95873DDDE2A06D53B15082DE42FF3A49433FAAFC0AAA4077DF642FCCCCC`；self-test `1651F73DE463C0E21FD8F3B3B61B98719940E84658AD71E5FD7371DE842881C2`；JSON parse `22E3C2784AB6C74B73419A315E2C096E72187E41B2A4B511FD5A4D1B0AAAB449`。

## 5. Automation 证据

| Evidence | Success | Fail | Total | SHA-256 |
|---|---:|---:|---:|---|
| P6 Integration baseline | 0 | 2 | 2 | `D27507F1B1308DA6289AB40D97D28DDF834F073769794BB2E91E08B3A82ACB8F` |
| P6 Integration after | 2 | 0 | 2 | `75BC2EACF8F1CD4CCE2B81FD14E5AACD99F6967EC7DA8CAD32CB296F8CFBFB4D` |
| P1 Sect Navigation after | 2 | 0 | 2 | `A38FFE486C7F005E485D4914FD122E2D023D77140D45D93EC452FF19218BAC26` |
| legacy `demo_map` after | 1329 | 1 | 1330 | `BDB8D983CFF291DCDBC5F4E86797A8E7FCAFFB2DCDCAD897BBEE5FE6E9C6283F` |
| `Shanmen.0_0_10` full | 712 | 0 | 712 | `D8B9D4236AFD56681A671B9EA722760DF4F9F619AB32E133A0796E790A8C1EF3` |

五份本阶段 Automation 日志均有 queue-empty terminal evidence；Fatal error、Unhandled Exception 与 Ensure condition failed 均为 0。P1 的失败基线保存在 P15.8 legacy 日志 `0A44835AE3380FA7918E49EA8FD0E593F3DBC26B55FAFAC64639817409A3B7D4` 中。

0.0.10 全量首末 Success 时间为 `23:28:45.859 -> 00:00:13.443 UTC`，约 `31m27.58s`，随后出现 `712 tests performed` queue-empty terminal evidence。

## 6. 构建证据

考虑主机当前提交内存较高，本轮构建从一开始即采用 `-NoUBA -MaxParallelActions=1`，避免无意义并行重试。

| Target | Result | Actions / UBT total time | Status | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 5 / 20.61s | 0 | `416DE9C8F507550C1BA7825A516088BF5857567FFB3181F2A906D44821287D68` |
| Game final | Succeeded | 4 / 26.02s | 0 | `11B1017B62F2B233408F3D94C3E6E8472A8CBEF7B88E8102033F219B8D46E116` |
| Editor final | Succeeded, up to date | 0 / 1.04s | 0 | `1CB00F7711A5F6DD3002BC8B3D697BBF0A8D19050E953A2B19BE6455C727D412` |

最终产物：

- `demo_map.exe`：355,744,256 bytes，SHA-256 `73D6F059880495F29907EEC1FED27A08DC2C2468990527F1202EB95BAE034497`；
- `UnrealEditor-demo_map.dll`：14,330,368 bytes，SHA-256 `795A66A631193BFC95E9F14CCEB8BFB812ACC1603DEE7ADECCA34F868F61A544`。

## 7. 修改范围

- `demo_mapP6IntegrationClosureTests.cpp`：修正 WindTalisman role，并按当前外部 workspace 路由验证 fail-closed 页面状态；
- `demo_mapSectNavigationTests.cpp`：同步当前外部 Warehouse 页面所有权；
- `ShanmenRegressionMap.json`：新增 P1 SectNavigation 与 P6 Integration exact mappings；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增两组 exact mapping 正反自检；
- Report/Log；
- production C++ 零修改，raw logs 仅本地保存；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 8. P/F 边界

本 Report 仅包含 P 阶段 legacy test contract 修复、回归门禁、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

外部 Code B workspace 的真实打开、关闭与输入交互不由本轮 null-manager fixture 冒充验证，仍属于 F 阶段真实产品验证。

## 9. 剩余失败与下一步

legacy 父组只剩一项：

- `demo_map.V3.Lifecycle.E.AtomicInventoryRollback`。

下一阶段可对该最后失败做独立根因审计；若属于陈旧 fixture，则有界收敛并重新证明 legacy 父组 1330/0；若指向真实原子回滚缺陷，则保留失败并修正式事务边界。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-9-p6-integration-contract-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-9-p6-integration-contract-regression/Docs/Report/Dev.D.UE.0.0.10.P15.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-9-p6-integration-contract-regression/Docs/Log/Dev.D.UE.0.0.10.P15.9.r0_log.md>
