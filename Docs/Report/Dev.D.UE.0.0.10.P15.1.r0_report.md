# Dev.D.UE.0.0.10.P15.1.r0 Report

## 1. 结论

P15.1 完成 legacy regression contract convergence。两个仍冻结在旧 Schema、旧装备槽和旧容量语义上的历史套件，已从基线 `67 Success / 13 Fail` 恢复为 `80/80 Success`；0.0.10 全量回归为 `708/708 Success`。

本轮没有回退当前产品契约。修复对象是测试 fixture、测试断言、跨进程 FullSystemLoop 自动化检查和“改动文件 → 必跑组”映射：

- Profile 默认与迁移断言改为读取 `CurrentSchemaVersion`；
- future-Schema fixture 从当前版本动态构造 `Current + 1`，且仍要求精确替换一次；
- `WindTalisman` 按当前定义进入 `SpatialRing` 槽，不再伪装成普通 Accessory；
- 背包容量从旧 `6/16/20` 收敛到当前 `6/42/42`，运行时 fixture 使用 resolver 返回值；
- 装备槽数量和顺序改为对照 canonical slot registry；
- 尸体装备区容量改为对照 `CorpseEquipmentCapacity`；
- 跨进程 reload 钩子不再要求已废止的 Schema 3；
- FullSystemLoop 与 SearchContainer 测试文件现在各自要求完整同名测试组证据。

正常产品路径、Schema 版本、序列化格式、物品定义、容量定义和存档 authority 均未修改。

## 2. 基线复现

修复前按原样运行两个历史组，均完成队列并原生退出 `0`，但日志里的测试结果明确包含失败。由此再次证明：不能只看进程退出码，必须同时检查 `Result={Fail}`。

| Group | Success | Fail | Terminal | Log SHA-256 |
|---|---:|---:|---:|---|
| `demo_map.FullSystemLoop` | 39 | 11 | 1 | `04F199DD8DC39039759C6037A7A7383146521D5CB7B6ABF2A45CE61821B5658E` |
| `demo_map.SearchContainer` | 28 | 2 | 1 | `01DB8BC54F8DC89FA86E8293404608B8D2521898D3582D712C883C056B5C294F` |

13 个失败均由历史 fixture 漂移造成，不是当前产品回归：

1. Schema 3/4 常量与 `4 -> 5` future token 已落后于当前 Schema 7；
2. `WindTalisman` 已从 Accessory 迁至 SpatialRing，旧 starter layout 因槽位错误被当前 validator 正确拒绝；
3. 背包容量已从 16/20 统一到 42，旧 runtime plan 的 20 与已提交计划不一致；
4. canonical equipment slots 已从 4 增至 5，尸体 Equipment section 也同步为 5；
5. V3 cross-process reload hook 仍要求 Schema 3，若进入对应阶段会产生确定性误报。

## 3. 实现

### 3.1 Schema fixture 去常量化

`MakeFutureSchema` 现在根据 `Fdemo_mapPersistentProfile::CurrentSchemaVersion` 构造当前 token 与 future token。若存档文本格式漂移、当前 token 不存在或重复出现，精确替换计数不等于 1，fixture 会 fail closed。

fresh profile、Schema 2 promotion、默认 Profile economy 和 protected-scope 测试均通过 Profile 实例验证其 `SchemaVersion == CurrentSchemaVersion`，不再使用过期字面量，也不使用 `CurrentSchemaVersion == CurrentSchemaVersion` 这类恒真断言。

### 3.2 当前装备与容量契约

FullSystemLoop starter layout 将 `WindTalisman` 写入 `SpatialRingItemInstanceId`。BeginRun、Death、Backpack GUID、RunInventory GUID 和 Hotbar GUID fixture 因此重新满足当前 validator，而没有削弱 validator。

容量测试继续保留明确产品数值 `base 6 / backpack L1 42 / backpack L2 42`，同时要求三个 resolver 均成功。运行时 materialization 与 Preparation view 不再复制容量常量，而是读取 canonical resolver；Preparation view 还逐项验证 canonical equipment slot 顺序。

SearchContainer 的尸体装备容量读取 `CorpseEquipmentCapacity`，从而与五槽 catalog 保持单一来源。

### 3.3 跨进程检查

`RunFullSystemLoopAutomation` reload 共通条件改为要求 `FirstLoad.Profile.SchemaVersion == CurrentSchemaVersion`。该路径只在显式 `FullSystemLoopAutomation` 参数下生效；普通产品运行没有行为变化。

本阶段遵守 P/F 边界，没有启动该跨进程产品自动化。此行由 Editor/Game 编译以及相邻 Profile、CodeB、V2 compatibility 无头回归覆盖；不宣称已完成 F 阶段跨进程产品验收。

## 4. 回归覆盖门禁

新增两个精确映射：

```text
Source/demo_map/demo_mapFullSystemLoopTests.cpp
  -> demo_map.FullSystemLoop

Source/demo_map/demo_mapSearchContainerTests.cpp
  -> demo_map.SearchContainer
```

流程自检新增两个正例与两个反例：完整同名组可以覆盖；无关的 `Shanmen.0_0_10` 全量不能替代 legacy 组。既有 unified-input 正例同步加入完整 FullSystemLoop 证据。结果：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=4 Required=15 Logs=9
SELF_TEST: PASS 246/246
```

15 个由改动路径推导出的必跑组均有健康日志；父组 `Shanmen.0_0_10` 合法覆盖其四个子组，父组 `demo_map.FullSystemLoop` 合法覆盖 `.41` 与 `.47`。

## 5. 修改范围

- `demo_mapFullSystemLoopTests.cpp`：动态 Schema、正确 SpatialRing fixture、当前容量与装备槽契约、去除陈旧测试名；
- `demo_mapSearchContainerTests.cpp`：当前 corpse equipment 容量与动态 Profile Schema；
- `demo_mapV3ProgressionManager.cpp`：跨进程 reload 使用 current Schema；
- `ShanmenRegressionMap.json`：增加两个 legacy suite 精确映射；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：增加四个正反覆盖自检并更新 unified-input 证据；
- Report/Log 之前 5 个源码与流程文件净变更 `+121 / -27`。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `demo_map.FullSystemLoop` | 50 | 0 | 0 | `00B4D56AF251F642F51B767231059A266AACCEEA3ACA12B098DB1791D8D96218` |
| `demo_map.SearchContainer` | 30 | 0 | 0 | `3FB7CFFFA67115E8DFCC7ECF277AED0D6C0E53FC3097190DD3E26F56FEBF9DC1` |
| `demo_map.CodeB` | 60 | 0 | 0 | `57393AAB0D11FC67370468BE65968CF8B06EF33928C5C271431757776891B8B4` |
| `demo_map.InputRestore.32` | 1 | 0 | 0 | `CDDB70151F801F049E6B64BECD3E2D3322FF0CD218B51E1AC36F465FFE86DBAD` |
| `demo_map.P5RuntimeInterface.06` | 1 | 0 | 0 | `4780E3B38CC7FF9E2F1F9118F4A43244EB10E898E5F82A36AE0A3DFC65A53228` |
| `demo_map.P7Integration` | 9 | 0 | 0 | `0BBB88617CD6FFF7A9D7B7C900019D65B81AE3B6369977DB7B34B3A3F44E7785` |
| `demo_map.Profile` | 211 | 0 | 0 | `07BA3F6C7256D5EA046F82B90B42CF08F6638FE212C712EE7F6D729F33F7618E` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `F1E2D43423DCAA03DC66175985DE73B8FB95279877DCC85A4225C28DF4323D6B` |
| `Shanmen.0_0_10` | 708 | 0 | 0 | `C1C14A6C66498E97569B6D29C2F318BC4F5166258FC3161A17DCEF18BBF5821B` |

八份聚焦日志合计 `384` Success、0 Fail；全量为 `708/708`；九份通过日志合计 `1,092` Success、0 Fail，组间有预期重叠。每份日志均有 terminal completion evidence，原生退出 `0`，Fatal/Unhandled/Ensure 为 `0`。

全量首末 Success 时间为 `13:27:14.366 -> 13:54:53.542`，约 `27m39.18s`。慢段来自测试自身对外部 connectivity probe 的 timeout/retry contract；`https://www.google.com/generate_204` 的 warning 没有转化为测试失败或 AutomationController Error。

## 7. 静态门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=4 Required=15 Logs=9
SELF_TEST: PASS 246/246
JSON_PARSE: PASS Rules=143
LEGACY_CONTRACT_SCAN: PASS
git diff --check: PASS (native exit 0)
```

目标文件中不再出现本轮识别的旧 Schema 3/4、runtime capacity 20、four-slot 或旧测试名 token。没有新增 Schema、序列化字段、repository、subsystem、Actor、World 查询、RNG、timer、damage 或存档写入口。

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Exit | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded | 6 / 28.30s | 0 | `5970DD801F5DB01B9C5E2996C97D9B2C7B458E85AA6173981E78006FBC375F3E` |
| Game final | Succeeded | 5 / 36.36s | 0 | `0BCD4568F25DB873A835A36DE4F7AE3100CC96217F9DDA3A8EC3387B104C773B` |
| Editor final | Succeeded, up to date | 0 / 0.96s | 0 | `770D03DB1611770ECE30FBAC26141E2CF125EDEA94E21ED7F5FF29A19E3659A3` |

最终产物：

- `UnrealEditor-demo_map.dll`：14,280,704 bytes，SHA-256 `C21E837FA8453EB47F82EB43CBF39FFE67190D8C93CE58A424AEF920882EBB64`；
- `demo_map.exe`：355,706,880 bytes，SHA-256 `3400D56633B8AC0445FF911638614EE27F95290772E9C68446C86A18FDD612DA`。

## 9. 异常与修复记录

两份 baseline 日志有意保留首次失败，共 13 个 `Result={Fail}`；它们用于证明旧 fixture 漂移及修复前后差异，不作为通过证据。

新增 FullSystemLoop 精确映射后的第一次流程自检正确 fail closed：既有 unified-input 正例只提供 `.41` 与 `.47`，缺少完整 `demo_map.FullSystemLoop`。修复方式是补充父组证据，而不是删除规则或降低要求；最终为 `246/246`。

本阶段没有源码编译失败、最终 Automation 失败、构建重试、Windows commit-memory/pagefile 故障或用户介入。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段测试契约修复、显式 automation-only 检查更新、NullRHI 无头 Automation、静态审查和 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

P15.1 已把“旧测试自己漂移”从 13 个噪声失败收敛为可维护契约，并让这两个历史测试文件永久进入改动驱动回归门禁。下一阶段应继续按实际风险选择工作，不再为旧命名或已被 canonical registry 取代的常量复制新断言。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-1-legacy-regression-contract-convergence>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-1-legacy-regression-contract-convergence/Docs/Report/Dev.D.UE.0.0.10.P15.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-1-legacy-regression-contract-convergence/Docs/Log/Dev.D.UE.0.0.10.P15.1.r0_log.md>
