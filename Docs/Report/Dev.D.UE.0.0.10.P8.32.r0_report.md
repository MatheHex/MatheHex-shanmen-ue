# Dev.D.UE.0.0.10.P8.32.r0 Report

## 1. 结论

P8.32 已在 P8.31 exact command/product identity fence 之上增加显式 formation influence consumer product runtime。调用方必须逐次提交 exact `ProductHost`、subject、`Udemo_mapAttributeComponent` 与已经冻结的 Apply/Remove command；runtime 只负责按顺序调用现有 product bridge，并在产品 teardown 前要求所有 native application 已显式 Remove。

本阶段结论为 **PASS**：runtime 专项 `4/4`、product bridge `5/5`、ProductHost `6/6`、FormationSession `4/4`、command host/coordinator/adapter/registry 各 `4/4`、projection `3/3`、legacy attributes `4/4`、完整 influence 链 `84/84`、`Shanmen.0_0_10` 全量 `333/333`、changed-file regression gate、自检 `94/94`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 deterministic runtime open

- `TryOpen` 只接受有效且未终止的 caller-provided `ProductHost`；
- 内部复用一个 P8.31 product bridge，不创建第二个 command、binding、transaction 或 native modifier 权威；
- `RuntimeId` 由 bridge identity 通过固定命名空间确定性派生；相同 ProductHost 重开得到相同 identity；
- runtime 不保存 ProductHost、World、Actor 或 component 指针。

### 2.2 explicit activation

`TryActivate` 依次执行以下 fail-closed preflight：

1. runtime 与 ProductHost 有效；
2. ProductHost 与 runtime 冻结的产品身份完全一致且尚未 terminal；
3. command 有效且 operation 为 `Apply`；
4. command lease 的 run/owner/source/deployment/content 通过 P8.31 exact product fence；
5. caller subject 与 command frozen subject 完全一致；
6. subject/component 通过 append-only bridge binding；
7. exact Apply 进入现有 P8.29→P8.26→attribute component 链。

首次 Apply 返回 `Activated`；同一 command 的 immutable replay 返回 `ActivationReplayed`，不重复创建 binding、transaction 或 native modifier。

### 2.3 explicit deactivation and teardown

- `TryDeactivate` 只接受 exact `Remove` command；Apply、malformed、foreign product 与 unbound subject 均失败关闭；
- 首次 Remove 返回 `Deactivated`，重复 Remove 返回 `DeactivationReplayed`，不会复活或重复移除 modifier；
- `CheckTeardownReady` 在 active application 存在时返回 `ActiveApplicationsRemain`；
- 只有 bridge drained 后才返回 `TeardownReady`；runtime 不自动 Remove、不推测目标、不重放命令。

### 2.4 evidence preservation

runtime result 保留 product binding 与 route 的嵌套结果，因此上层可以区分 ProductHost、command identity、subject、binding、route 与 teardown 错误。它只增加生命周期编排状态，不吞掉 P8.24–31 的原始诊断。

## 3. 完整性与安全边界

ProductHost/Session 仍是 formation 产品生命周期权威；P8.31 bridge 仍是 exact product identity 与 route 栅栏；P8.29 command host 仍是 subject binding/command 聚合器；P8.28 coordinator 仍是 transaction/replay 权威；P8.26 registry 仍是 active application 权威；`Udemo_mapAttributeComponent` 仍是 native modifier 权威。

新 runtime 不做 component discovery、command synthesis、magnitude resampling、自动重试、自动 Remove、调度、持久化或 World 操作。生产 header/cpp 静态扫描结果：`UWorld/AActor = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`RNG = 0`、`SaveGame/ProfileRepository = 0`、`Tick/while = 0`、`GetSubsystem/FindComponent/TActorIterator = 0`、`Retry/PendingCommands/ActiveModifiers = 0`、`TMap = 0`。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductRuntime.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductRuntime.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductRuntimeTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

runtime header/implementation/tests 分别为 `104/319/578` 行。回归映射由 `66` 条增至 `67` 条，并把 runtime 作为 bridge、ProductHost/session、consumer chain 与 exact attribute mutation 的反向依赖；自测由 `92` 项增至 `94` 项。长期未跟踪的用户与 0.0.9B 工件未修改、未 stage。

## 5. 自动化验证

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P8.32-FormationInfluenceConsumerProductRuntime-final.log` | `FormationInfluenceConsumerProductRuntime` | 4 | 0 | 0 | `3637252E13C518C8490335F0F70ADFF4830DE4EAC24AE8B4B755CCB866CE4C2F` |
| `P8.32-FormationInfluenceConsumerProductBridge-final.log` | `FormationInfluenceConsumerProductBridge` | 5 | 0 | 0 | `CBD6CDC405337C09B07BA18336A8D7CAE165BFD6969EE9B3CF147A8328659330` |
| `P8.32-FormationProductHost-final.log` | `FormationProductHost` | 6 | 0 | 0 | `80386DA39D9ECD162300C5A7274BA1C627123E2C1A5E5D18122E51ED6D28399C` |
| `P8.32-FormationSession-final.log` | `FormationSession` | 4 | 0 | 0 | `9660FB2655525A81E30E8E089017FFACB101C554FF9DBE63B5B998CFDD883103` |
| `P8.32-FormationInfluenceConsumerCommandHost-final.log` | `FormationInfluenceConsumerCommandHost` | 4 | 0 | 0 | `1D3A93CACE25F7A192F94E6A48EB10D6AAC39F77E1CF2797FEDA6CB5903B1BEC` |
| `P8.32-FormationInfluenceConsumerApplicationCoordinator-final.log` | `FormationInfluenceConsumerApplicationCoordinator` | 4 | 0 | 0 | `56D3350FE592CBFFBEFF7B053BBE9BDFF5D2FC8410A75878BB6937B7BFE433CC` |
| `P8.32-FormationInfluenceConsumerAttributeAdapter-final.log` | `FormationInfluenceConsumerAttributeAdapter` | 4 | 0 | 0 | `C706615D076D5AFDED372DBAEA69F26074C154B82127CCCD7F50196F60AE8E8B` |
| `P8.32-FormationInfluenceConsumerRegistry-final.log` | `FormationInfluenceConsumerRegistry` | 4 | 0 | 0 | `AC95CBF757B8711BE4E0F0498DB3F3879C7CAD419C87D0CE474FAED869EF3855` |
| `P8.32-FormationInfluenceConsumerProjection-final.log` | `FormationInfluenceConsumerProjection` | 3 | 0 | 0 | `C4C19248B411207809513073AB923EDD60535D5E07637D72CFC30869C3415E2B` |
| `P8.32-Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `FE036CB4B977AAA640EC3C3108EB196B728827651AF442F3015C1D583625EADD` |
| `P8.32-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 84 | 0 | 0 | `D1DF559CD783B475EA30D58A4A7E97F8B9CA68271E466DEDB0BB48417D13674C` |
| `P8.32-Shanmen-full-final.log` | `Shanmen.0_0_10` | 333 | 0 | 0 | `03A023C32DFE0E845B4FA383AA9E189341473BF13C29685F6AD9D4FE7A1AA12B` |

十二份最终 Automation 日志各有唯一 RunTests command、正式 queue-empty、selected Fail `0`、fatal/unhandled/ensure `0` 与原生退出码 `0`。

## 6. 专项覆盖与提交前审查

四个 runtime case 覆盖：

1. invalid host 拒绝、deterministic open、初始 teardown ready；
2. Apply/replay、active teardown block、Remove/replay、最终 drained；
3. invalid command、错误 operation、错误 subject 与同 scope/不同 deployment 在 bind 前失败；
4. null component 保留嵌套 binding 错误，exact command 仍可执行；
5. foreign ProductHost 无法 bind/route/check teardown；
6. 一个 native component 不得被另一 subject 复用；
7. unbound Remove 保留嵌套 route 错误；
8. rejected operation、foreign identity 与 binding conflict 均不修改 transaction/native 状态。

fixture 通过真实 ProductHost、area provider、intent、modifier evaluator、evaluation binding、lease executor 与 consumer projector 生成 frozen command；未修改私有字段、未伪造内部 ledger 或 native handle。

## 7. Changed-file regression gate

新增 runtime 路径规则要求十五组测试契约；十二份日志通过专用组与父组覆盖完整满足。bridge 规则也增加 runtime 反向依赖，避免未来修改 bridge 时遗漏 lifecycle facade。

```text
REGRESSION_MAP_JSON: PASS Rules=67
SELF_TEST: PASS 94/94
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=15 Logs=12
```

- mapping SHA-256：`A934B8BA4DB6BEDD55612973306F3FA234709E331BA9739842C0FC3CE2DCCB68`；
- self-test SHA-256：`1D058B83B572948C5AB0C223459F255DE6292D4EE0E37BD0444FC8596F3DBCA1`；
- `git diff --check`：PASS。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor final | Succeeded / 0 actions / 0.95s | 0 | `FC4F550C9877A61EB32050267CBC865D889EA0ADBB37EF8D24CBB659E47BA02B` |
| Game final | Succeeded / 4 actions / 24.33s | 0 | `8A5D626E0BCDEECA482A90644A95050253561E044AE4C7C65C7AC8BB8EEBC126` |

- `UnrealEditor-demo_map.dll`：`12048896` bytes，SHA-256 `5E5A2EC84E92CEF5BCADAB85AADEF159F0B5C8815CA4B91504B8235ACF31D835`；
- `demo_map.exe`：`353101312` bytes，SHA-256 `B282C33CAF057F5825FA4B91EC01D54A1CB7606A0DB016685875F4073A270E07`。

## 9. 真实异常与兼容性

首次 Editor 编译原生退出码为 `1`：生产 runtime 已通过编译，测试 fixture 在 `ModifierSpecification` 上误用了不存在的 `TryCreateOffensePowerAdditive`，产生 `C2039/C3861`。修为现有 `TryCreate` 契约后 Editor 编译成功；未修改产品设计。

首次 runtime Automation 进程原生退出码为 `0`，但日志只有 `2 Success / 2 Fail / queue-empty`，runner 未接受该证据。两项失败均因测试在 Remove 后才读取激活态计数；改为在 Remove 前冻结只读快照后，专项为 `4/4`，随后 influence `84/84` 与全量 `333/333`。失败日志保留为 `FormationInfluenceConsumerProductRuntime.first-fail.log`，SHA-256 `4E422AA40D2CB3C854DD48C6D60CB45F84AB7D13637C364AA848099D3088F283`。

UE 启动阶段的非 Win64 SDK 枚举警告未影响 `Win64 VALID`。未把原生退出码 `0` 当作测试成功的唯一条件；selected Fail 与 queue-empty 均由 runner 独立检查。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.33 在该 runtime 之上增加 caller-owned product lifecycle integration：由现有 ProductHost teardown 流程显式调用 runtime readiness，并由上层提供已知 component 与 frozen commands；不得让 runtime 接管发现、调度、重试或产品终止权威。
