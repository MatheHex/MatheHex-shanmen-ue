# Dev.D.UE.0.0.10.P8.30.r0 Report

## 1. 结论

P8.30 已增加 formation influence consumer product bridge。桥从既有 `Fdemo_mapShanmenFormationProductHost` 冻结 correlation、action activation/source、deployment 与 content 身份；上层每次显式提交 exact ProductHost、subject component 和 P8.25 command，桥先验证产品会话，再直接委托 P8.29 command host。桥不自动发现 component、不自动重试、不复制 binding/application/native/history 权威。

本阶段结论为 **PASS**：product bridge 专项 `4/4`、ProductHost `6/6`、FormationSession `4/4`、P8.29 host `4/4`、P8.28 coordinator `4/4`、P8.27 adapter `4/4`、P8.26 registry `4/4`、P8.25 projection `3/3`、legacy attributes `4/4`、完整 influence 链 `79/79`、`Shanmen.0_0_10` 全量 `328/328`、changed-file regression gate、自检 `92/92`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 产品身份冻结

- `TryOpen` 只接受有效、非 terminal ProductHost；
- bridge ID 由 correlation、run/owner、action activation/source、deployment 与 content 确定性派生；
- bridge 保存完整 run correlation，并交叉验证 P8.29 command host 的 run/content；
- 每次 bind/route 都重新核验 caller-owned ProductHost；同 run 之外的 host、无效 host 与 terminal host 在进入 P8.29 前拒绝；
- ProductHost 可以从 Deploying 正常推进到 Active，只要冻结身份未变，桥仍保持有效。

### 2.2 显式 component binding

- `TryBindSubject(ProductHost, SubjectEntityId, AttributeComponent)` 不搜索 Actor、World、subsystem 或 component；
- 合法 bind 与 exact replay 映射为产品级 `Bound` / `BindingReplayed`，并保留原 P8.29 binding receipt；
- subject replacement、component reuse、invalid subject 与 null component 作为完整嵌套 rejection 返回；
- drained 后 binding 仍 append-only，replacement component 不能重置历史 fence。

### 2.3 窄 route 与结果回传

- `TryRoute(ProductHost, Command)` 在产品身份校验后直接调用 P8.29 `TryRoute`；
- 首次 Apply/Remove 映射为 `Routed`；coordinator immutable replay 映射为 `TransactionReplayed`；
- unbound subject、malformed command、run/content mismatch 和 native/coordinator rejection 保留 P8.29/P8.28 原始嵌套结果；
- bridge 只转发 `bHostStateChanged`，不生成第二条 transaction、application、native acknowledgement 或 completed history；
- Apply、Remove 完成后重放旧 Apply 返回原 evidence，不复活 attribute modifier。

### 2.4 生命周期查询

- `MatchesProductHost` 提供 exact product identity 查询；
- `IsDrained` 仅在 P8.29 所有 coordinator 都无 active application 时为真；
- binding、active application 与 completed transaction count 直接读取 P8.29，不维护镜像集合；
- 产品结束前，上层可用 `IsDrained` 明确检查 consumer teardown，桥本身不抢占 ProductHost 的终止权威。

## 3. 完整性与安全边界

ProductHost/Session 仍是 formation 产品生命周期权威，P8.29 仍是多 subject binding resolver，P8.28 仍是 transaction/replay 权威，P8.26 registry 仍是 active application 权威，`Udemo_mapAttributeComponent` 仍是 native modifier 权威。bridge 只冻结产品身份并做顺序校验与委托。

新增 bridge 生产文件静态扫描：`UWorld/AActor = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`RNG = 0`、`SaveGame/ProfileRepository = 0`、`Tick/while = 0`、`GetSubsystem/FindComponent/TActorIterator = 0`、`RetryQueue/PendingCommands/ActiveModifiers = 0`、`TMap = 0`。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductBridge.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductBridge.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductBridgeTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

bridge header/implementation/tests 分别为 `133/320/558` 行；流程映射新增 `27` 行，自检新增 `20` 行。文档加入前共新增 `1058` 行、删除 `0` 行。长期未跟踪的用户与 0.0.9B 工件未修改、未 stage。

## 5. 自动化验证

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P8.30-FormationInfluenceConsumerProductBridge-final.log` | `FormationInfluenceConsumerProductBridge` | 4 | 0 | 0 | `A655A73EC8A8FA5E5E36D5843AD8D4EAE68E1A53B5992CE593BA59650DA84FA8` |
| `P8.30-FormationProductHost-final.log` | `FormationProductHost` | 6 | 0 | 0 | `E360239415D8D10674FC7404C4624FFE334F976886FCD54278A7DDD35A4EA975` |
| `P8.30-FormationSession-final.log` | `FormationSession` | 4 | 0 | 0 | `0FC3C6EE68DBB7854FF6AC9AC77B99DCAFBD19BC446FBD669EBCDC5631D2A0E3` |
| `P8.30-FormationInfluenceConsumerCommandHost-final.log` | `FormationInfluenceConsumerCommandHost` | 4 | 0 | 0 | `5A16D4C58436722210CAD47E1A4839E25CD8D25A55F38FB1739BC8B463814911` |
| `P8.30-FormationInfluenceConsumerApplicationCoordinator-final.log` | `FormationInfluenceConsumerApplicationCoordinator` | 4 | 0 | 0 | `EFD6F2AF5719E472A270194370AD4BA187AF48C386F06B5F819523D1620DE1BE` |
| `P8.30-FormationInfluenceConsumerAttributeAdapter-final.log` | `FormationInfluenceConsumerAttributeAdapter` | 4 | 0 | 0 | `62D70BF487F1C56621FE0A90F5109F9A27EBC480F0780EC4990D3F14485CD423` |
| `P8.30-FormationInfluenceConsumerRegistry-final.log` | `FormationInfluenceConsumerRegistry` | 4 | 0 | 0 | `B605C9376379ADB9477AF6DE6B0C194B039EE5508BFD6EF0B758F0F41F0BEA90` |
| `P8.30-FormationInfluenceConsumerProjection-final.log` | `FormationInfluenceConsumerProjection` | 3 | 0 | 0 | `26673BAAB5D2A676A1CD461665CA8465548DE6A9A99452967D28284FC7C59889` |
| `P8.30-Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | `4B80A0E8FD7C49447E746DD740744163EC38CE096AB960C88A3DCCF233C25270` |
| `P8.30-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 79 | 0 | 0 | `D2DE10E7DD55AEA999994EE33DD8A5141E8AECBC2D148EF5A4B258BAB6F05A58` |
| `P8.30-Shanmen-full-final.log` | `Shanmen.0_0_10` | 328 | 0 | 0 | `A920D4181FDBC7A03952A2CAAC5557E30271065A22F6C66C487BE0C0E1C70E42` |

十一份最终日志各有一个 RunTests command、一个正式 queue-empty，selected Fail、fatal、unhandled 与 ensure 均为 `0`。

## 6. 专项覆盖与提交前审查

四个 product bridge case 覆盖：

1. 无效 ProductHost 不能 open；同一 ProductHost 重开得到同一 deterministic bridge ID；
2. exact bind/replay、Apply/replay/Remove、aggregate drain/count；
3. foreign ProductHost 在 bind/route 前被拒绝，不能创建 binding 或 native mutation；
4. subject replacement、component reuse、invalid subject、null component，以及 drained replacement fence；
5. unbound subject、malformed command 与 foreign run/content command 的嵌套错误可见性；
6. Apply/Remove 后旧 Apply replay 不复活 modifier、不增加 history。

测试使用纯值、有效的 run correlation/action/diagram 启动真实 ProductHost，再经 area → intent → evaluator → lease executor → P8.25 projector 生成 command；没有伪造 bridge 内部状态，也没有依赖 UI/World Actor。

## 7. Changed-file regression gate

新增 product-bridge path rule，并把 bridge 专项反向接入 ProductSession、ProductHost、projection、attribute adapter、application coordinator、command host 与 exact attribute mutation 映射。共享映射由 `65` 增至 `66` 条，自检由 `90/90` 增至 `92/92`。

```text
REGRESSION_MAP_JSON: PASS Rules=66
SELF_TEST: PASS 92/92
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=14 Logs=11
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=14 Logs=11
```

- mapping SHA-256：`E48DD688C2017B7B49C07A9EA74BF6045C87DAE737F73EC90EB407BC8E379D46`；
- self-test SHA-256：`9C1A64FE9F31B4E0153C11B2D29994ABC35B315FB089872F5CCD777730A01879`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor first attempt | Failed / `OtherCompilationError` | 6 | terminal evidence |
| Editor corrected | Succeeded / 4 actions | 0 | terminal evidence |
| Editor final | Succeeded / 0 actions / 0.92s | 0 | `08659575C02ECE3439852ABE1969FF1E0CFA61371F90F816979CC41F4C9237EC` |
| Game final | Succeeded / 4 actions / 23.97s | 0 | `3AE082A4C7EB1DF17175710E8406CF7B575EAECCEA42E0E4239724678EACE4ED` |

- `UnrealEditor-demo_map.dll`：`12003840` bytes，SHA-256 `D5B196A75811E53F11953FB72B6CAA2FBCF00AC727FA847074C3206DC4AC5E4D`；
- `demo_map.exe`：`353066496` bytes，SHA-256 `F611AF65DE7EE08AF582CEC86764E599C92375AC797BF9B0FF095BA59222A898`。

## 9. 真实异常与兼容性

首次 Editor 编译时新增测试遗漏 `ShanmenCombatResolver.h`，导致 `FShanmenCombatIdFactory` 未声明，UBT 返回 `OtherCompilationError` / 退出码 `6`。补齐测试 include 后 Editor、全部 Automation 与 Game 均通过；产品桥实现文件在首次编译中已通过。该错误是测试翻译单元 include 缺失，不是运行环境或产品契约失败。

UE 启动阶段在正式 RunTests command 前仍会输出引擎自带 UnifiedError/Automation 条件演示日志；selected tests 全部 Success，正式 queue-empty、原生退出码、fatal/unhandled/ensure 检查均通过。非 Win64 SDK 枚举警告未影响 Win64 VALID 状态。

文档加入后的第一次 gate 复查用 `P8.30-*-final.log` 选中了两份 build log，validator 按设计因其没有 RunTests/queue-empty 而 fail closed；收窄为十一份 Automation log 后 `Changed=7` 通过。中间一次 `pwsh -Command` 包装层提前展开 `$logs`，造成参数绑定失败；改用当前 PowerShell 的显式数组调用后通过。这两项均为证据选择/命令引用错误，不是产品或映射失败。

`git diff --check`：PASS。P8.25–29 wire shape、ProductHost/Session 源码、attribute component 与 legacy 产品权威均未修改。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.31 增加显式 consumer product runtime：接收已经完成的 lease/projected command 与 caller-provided component binding，按 Apply/Remove 生命周期调用 P8.30，并在 product teardown 前强制检查 bridge drained；仍不得自动发现 component、自动 retry、重采样 magnitude 或复制 P8.24–30 任一权威。
