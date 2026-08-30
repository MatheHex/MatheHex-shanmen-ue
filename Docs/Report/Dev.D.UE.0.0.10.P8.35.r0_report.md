# Dev.D.UE.0.0.10.P8.35.r0 Report

## 1. 结论

P8.35 已在既有 `Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost` 内建立 receipt-bound、只读、可审计的 native consumer command delivery，没有新增 controller，也没有把组件解析或 native mutation 隐藏进 lifecycle Host。

本阶段结论为 **PASS**：LifecycleCommandHost `5/5`、ProductHost `6/6`、FormationInfluence `85/85`、legacy Attributes `4/4`、`Shanmen.0_0_10` 全量 `334/334`；changed-file regression gate、映射 JSON、自检 `94/94`、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 durable receipt 绑定的 command delivery

- caller 现在只需提交本 Host 已完成的 lifecycle `CommandId` 与 frozen formation definition；
- Host 必须读取同一实例保存的 durable lifecycle receipt，并确认其为成功的 `ExecuteStep` / `Apply`；
- source receipt 的 invocation intent、subject、expected intent id 与 active authoritative lease 必须完全一致；
- 只有上述证据闭合后，才返回 projection、frozen Apply command 与 frozen Remove command。

因此产品调用方不再通过 `GetRouter().GetCoordinator()...GetExecutor()` 穿透内部层级、手工查询 lease、投影并拼装 native command。

### 2.2 只读交付，不代替执行

`TryPrepareConsumerCommands(...) const` 只读取 durable receipt 和 active lease，随后执行纯值投影与 deterministic command build：

- 不解析 subject component；
- 不调用 consumer runtime Apply/Remove；
- 不产生 binding、application 或 transaction；
- 不修改 lifecycle state；
- 不调度、不重试、不持久化 engine object。

实际 component 解析与 Apply/Remove 仍由既有显式产品层完成，P8.34 的 lease/native causal-order fence 保持不变。

### 2.3 自校验值结果

`Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery` 与 result 均可独立重验：

- lifecycle `CommandId`、subject、definition、projection 与 authoritative lease 对齐；
- Apply/Remove 共用同一 application handle，且 command id 不同、operation 正确；
- result 重新验证 source receipt 成功、operation/intent/subject 一致，而不是只信任工厂路径；
- malformed host/id/receipt、非 Apply receipt、definition mismatch、inactive lease、projection/build failure 与 impossible state 都有 fail-closed 状态。

## 3. 权威与兼容性

- ProductHost/Session 仍是 formation 产品生命周期权威；
- lifecycle Router/Coordinator/Executor 仍是 authoritative lease 权威；
- LifecycleCommandHost 只新增 caller-facing 的只读交付口，不复制 lease ledger；
- consumer runtime/bridge/command host 仍管理显式 native application lifecycle；
- `Udemo_mapAttributeComponent` 仍是 native modifier 权威。

既有 lower-level getter/API 保留给组合和独立测试；真实 ProductHost 测试已切换到新交付口。没有新增总控制器、自动发现组件、隐式提交或双写状态。

## 4. 修改范围

更新生产代码：

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.cpp`。

更新验证：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- 本 Report 与同名 Development Log。

regression map 已覆盖上述路径，本阶段无需扩展规则，仍为 67 条。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 5. Automation 证据

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationInfluenceLifecycleCommandHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost` | 5 | 0 | 0 | `8E38AB91EDD1303673D125DB2E06698CA70DFA458EDDCBB85FE3809EC9673A68` |
| `FormationProductHost-final.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 6 | 0 | 0 | `B47CFBEE18EBB3ED2955FA5B35BF78399B04E089569B4A7AECB6A2312206A54C` |
| `FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 85 | 0 | 0 | `10782A5FF7B89CA2E4855D042E1D27403AFC6B66E6E2EB6475BC742F045BE7F8` |
| `Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `FD4DC3DF89C9E320F92439DA21830EEA23AF65AAFDCC202D751280152E639EE1` |
| `Shanmen-full-final.log` | `Shanmen.0_0_10` | 334 | 0 | 0 | `AF2A99624FF0F465B8B1707827B118A2E674A1BA314B5D0DB195AF565F3296AD` |

五份最终日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## 6. 专项覆盖

真实 ProductHost 用例新增或保留以下断言：

- lifecycle Apply receipt 可签发完整 consumer delivery；
- 相同 receipt 重复签发结果稳定，且没有创建 binding/application/transaction；
- invalid `CommandId` 与未知 receipt 被拒绝；
- foreign content/definition mismatch 在 projection 阶段被拒绝；
- 非 Apply lifecycle receipt 不能签发 Apply/Remove pair；
- authoritative lease 删除后，同一旧 receipt 不能重新签发 delivery，返回 `LeaseNotActive`；
- 已签发 frozen command 仍按 P8.34 的 Apply → native Remove → authoritative Remove 顺序执行；
- terminal failure、forward recovery 与 immutable replay 行为保持不变。

## 7. Changed-file regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=67
SELF_TEST: PASS 94/94
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=35 Logs=5
```

- mapping SHA-256：`1FC61E3CA0586BC39F637980557D329F33D3B19AEECD0E1E43F93E67CC780B9A`；
- self-test SHA-256：`F11AE76FBDF73AFAD67469D68B0DF47748D4114ADC04F79577328A0A7C3C2A98`；
- `git diff --check` 与 `git diff --cached --check`：PASS。

## 8. 静态边界

两个生产 header/cpp 扫描：`GetSubsystem=0`、`FindComponent=0`、`TActorIterator=0`、`Tick=0`、`while=0`、`TMap=0`、`AActor=0`、GAS symbols `0`、RNG `0`、SaveGame/ProfileRepository `0`、raw object-pointer member `0`。

## 9. 构建与真实异常

构建命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Log SHA-256 |
|---|---|---:|---|
| Editor final | Succeeded / up to date / 0 actions / 0.90s | 0 | `8575654C58640A751A7546CB12AB7D2C7108DA98912C2B787CAF34B551EDA3E0` |
| Game final | Succeeded / 4 actions / 25.04s | 0 | `F0F858114AC029251D829F34B4C055BC5633EF3AB50CD68228A14D565C803191` |

- `UnrealEditor-demo_map.dll`：`12093440` bytes，SHA-256 `2B3AAB9C8E7D1837BD79439BFF0CE29915117A5C65FE7A1C4374EDC606280C9B`；
- `demo_map.exe`：`353137152` bytes，SHA-256 `92E61BC95612BB4F3A8DA52C3DAB3D5560F7D3DB42EF27941B29EAE7CF2353D2`。

本阶段没有源码编译失败、UE Automation 失败或内存/环境故障。

## 10. P/F 边界与下一步

本 Report 仅包含源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议 P8.36 继续沿显式 caller contract 前进：在现有产品 composition root 中消费本阶段 delivery，并把 subject/component resolution 的输入与拒绝证据做成可审计值；仍不得把 component discovery、Apply/Remove mutation 或重试藏进 lifecycle Host。
