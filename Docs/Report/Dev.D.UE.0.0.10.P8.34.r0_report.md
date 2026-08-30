# Dev.D.UE.0.0.10.P8.34.r0 Report

## 1. 结论

P8.34 已在既有 `Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost` 内收紧 formation influence authoritative lease 与 native consumer application 的因果顺序，没有新增第二套产品控制器。

本阶段结论为 **PASS**：consumer CommandHost `4/4`、consumer runtime `4/4`、consumer registry `4/4`、LifecycleCommandHost `5/5`、ProductHost `6/6`、legacy Attributes `4/4`、FormationInfluence `85/85`、`Shanmen.0_0_10` 全量 `334/334`；changed-file regression gate、映射 JSON、自检 `94/94`、静态边界、`git diff --check`、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 lease provenance gate

- 新 consumer Apply/Remove 必须由同一个 lifecycle CommandHost 的内部 lease executor 提供 active authoritative lease；
- command projection 中的 `LeaseId`、`ApplyIntentId`、lease key 与 evaluation receipt 必须和 authoritative snapshot 完全一致；
- 未绑定的平行 CommandHost 返回 `LeaseAuthorityUnavailable`，不存在 active lease 返回 `LeaseNotActive`，证据不一致返回 `LeaseIdentityMismatch`；
- product runtime result 显式携带 `bLeaseAuthorityChecked` 与 `AuthoritativeLease`，调用方不再只能从成功状态反推 lease 依据。

### 2.2 causal-order fence

单个 lease 的有效顺序固定为：

1. authoritative lease Apply；
2. 显式 native consumer Apply；
3. 显式 native consumer Remove；
4. authoritative lease Remove。

LifecycleCommandHost 在提交新的 authoritative Remove 前，按 exact `LeaseId` 查询其内部 consumer runtime。仍有 native application 时返回 `ConsumerDeactivateRequired`，不执行 coordinator/router、不提交 durable lifecycle receipt，也不推进 ProductHost intent。native consumer 显式 Remove 后，同一 authoritative Remove 才可继续。

### 2.3 durable replay 与只读查询

- consumer CommandHost 可按完整 command 读取已完成 transaction，读取同时校验 receipt 确实匹配该 command；
- exact 已完成 consumer command 在 authoritative lease 删除后仍可只读 replay，不再次要求 active lease，也不重复修改 attribute；
- lifecycle durable command replay 先于新 Remove gate，既有 receipt 不会被当前 mutable application 数量重判；
- registry 与 consumer CommandHost 提供 exact lease active-application count，非法状态或非法 LeaseId 返回 `INDEX_NONE`，不会把未知状态误判为零。

## 3. 权威与兼容性

- ProductHost/Session 仍是 formation 产品生命周期权威；
- lifecycle Router 内部 execution runtime/lease executor 仍是 authoritative lease 权威；
- consumer runtime/bridge/command host 仍只管理显式 native application lifecycle；
- `Udemo_mapAttributeComponent` 仍是 native modifier 权威；
- caller 继续显式提供 ProductHost、World、subject、AttributeComponent 与 frozen command；新代码不发现组件、不自动清理、不循环、不调度、不后台重试、不持久化，也不保存 engine-object pointer。

既有 lower-level registry、coordinator、router API 保留用于组合和独立测试；caller-facing lifecycle CommandHost 增加严格 gate，因此没有复制 ProductHost 或新增总控制器。

## 4. 修改范围

更新生产代码：

- `demo_mapShanmenFormationInfluenceConsumerCommandHost.h/.cpp`；
- `demo_mapShanmenFormationInfluenceConsumerProductRuntime.h`；
- `demo_mapShanmenFormationInfluenceConsumerRegistry.h/.cpp`；
- `demo_mapShanmenFormationInfluenceLifecycleCommandHost.h/.cpp`；
- `demo_mapShanmenFormationInfluenceLifecycleCoordinator.h`。

更新验证：

- `demo_mapShanmenFormationInfluenceConsumerProjectionTests.cpp`；
- `demo_mapShanmenFormationProductHostTests.cpp`；
- 本 Report 与同名 Development Log。

regression map 已覆盖上述路径，本阶段无需扩展规则，仍为 67 条。长期未跟踪的 0.0.9B Prompt/Report 与用户文件未修改、未 stage。

## 5. Automation 证据

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `98273B216FDE196787BEF76F9BD4AF2CAECE9D37FFF89CC084DCD451009DF7E0` |
| `FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 85 | 0 | 0 | `AC9E86E8F0AB34126EBC12051BD1786A345620C653D65609D83B0C4CC8FB20FF` |
| `FormationInfluenceConsumerCommandHost-final.log` | `FormationInfluenceConsumerCommandHost` | 4 | 0 | 0 | `42FC8CCB92C4965BC366E05A7EF13A2E2FB20A2D3C5B40071035728D5358CE21` |
| `FormationInfluenceConsumerProductRuntime-final.log` | `FormationInfluenceConsumerProductRuntime` | 4 | 0 | 0 | `843D47148AECF9FB7D1D169F79BD23CED0BB1F699A50AA9CB1ECA85667C158D5` |
| `FormationInfluenceConsumerRegistry-final.log` | `FormationInfluenceConsumerRegistry` | 4 | 0 | 0 | `B3C12043019D51F1B748295981C2156728971E0485391201D40A7FDFAE881973` |
| `FormationInfluenceLifecycleCommandHost-final.log` | `FormationInfluenceLifecycleCommandHost` | 5 | 0 | 0 | `7D92C2EEF70356B451FC2AAB560DA4336C59C0C9A8C13A50C5533CC25896A905` |
| `FormationProductHost-final.log` | `FormationProductHost` | 6 | 0 | 0 | `F24E867C1974FF367295FCD852739AC9D9AF9C4076A23EE80BF9516B6750A978` |
| `Shanmen-full-final.log` | `Shanmen.0_0_10` | 334 | 0 | 0 | `A7264115B7C86C4C902FBD0278B8CF51CF17FE20A59019A2AB919523D107C9E1` |

八份最终日志各有唯一 RunTests、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。consumer CommandHost 的第一次候选日志虽为 `4/4` 且进程退出 `0`，但缺正式 queue-empty；该证据被丢弃，仅重跑这一组并以表中 SHA 的最终日志为准。

## 6. 专项覆盖

真实产品链用例覆盖：

- 同一 ProductHost 打开 exact 与 parallel lifecycle CommandHost；
- exact Host 执行 Apply intent，生成 active authoritative lease；
- parallel Host 无法消费 exact Host 内部 lease；
- exact Host 将 frozen projection Apply 到真实 `Udemo_mapAttributeComponent`；
- native modifier 活跃时 authoritative Remove 返回 `ConsumerDeactivateRequired`，且无 durable receipt、ProductHost intent 未推进；
- native Remove 后 authoritative Remove 成功；
- lease 已删除后 exact native Remove 只读 replay；
- drained consumer 继续通过 null-World terminal failure、durable forward recovery 与完成后 immutable replay。

底层专项同时覆盖 per-lease active count、非法 LeaseId fail-closed、已完成 Apply/Remove transaction 的 exact receipt 查询。

## 7. Changed-file regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=67
SELF_TEST: PASS 94/94
REGRESSION_COVERAGE: PASS Changed=12 Rules=7 Required=36 Logs=8
```

- mapping SHA-256：`1FC61E3CA0586BC39F637980557D329F33D3B19AEECD0E1E43F93E67CC780B9A`；
- self-test SHA-256：`F11AE76FBDF73AFAD67469D68B0DF47748D4114ADC04F79577328A0A7C3C2A98`；
- `git diff --check`：PASS。

## 8. 静态边界

八个生产 header/cpp 扫描：`GetSubsystem=0`、`FindComponent=0`、`TActorIterator=0`、`Tick=0`、`while=0`、`TMap=0`、`AActor=0`、GAS symbols `0`、RNG `0`、SaveGame/ProfileRepository `0`；五个相关 header 的 raw object-pointer member 为 `0`。

## 9. 构建与真实异常

构建命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Log SHA-256 |
|---|---|---:|---|
| Editor final | Succeeded / 9 actions / 17.90s | 0 | `F8DF5E2B7C099CC996AE061E70A37B71A48C9F9FAB089B7FBC843CA18FA93C42` |
| Game final | Succeeded / 15 actions / 48.81s | 0 | `54BA95D3F64F9A5FA4FDAF49B92571314385F1A4E5DBB5301C288E135D433EA2` |

- `UnrealEditor-demo_map.dll`：`12079104` bytes，SHA-256 `3DC3494B3605CA643EF48DDE4F63C954B04A737008F6A6E164BAD7BA9B26D16D`；
- `demo_map.exe`：`353126400` bytes，SHA-256 `9DA70CCF05F2FBED974D6D4B09A7A5F5182E2F4224D6F61A55B0DCBEF7DAF5C3`。

本阶段没有源码编译失败、UE Automation 失败或内存/环境故障。唯一被拒绝的候选证据是缺少 queue-empty 的日志终止完整性问题，重跑后已闭环。

## 10. P/F 边界与下一步

本 Report 仅包含源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

建议 P8.35 在不复制权威的前提下继续收口 caller contract：对现有 formation 产品调用链增加一个显式、可审计的 projection-command 交付点，让 subject/component 解析留在既有产品层，lease/native 顺序仍由本阶段的 lifecycle CommandHost 统一门禁。
