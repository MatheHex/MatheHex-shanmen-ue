# Dev.D.UE.0.0.10.P8.28.r0 Report

## 1. 结论

P8.28 已增加 component-bound consumer application coordinator，把 P8.26 registry 与 P8.27 native attribute adapter 收进一个明确的 caller-driven 事务边界。命令先在 registry 候选副本执行；只有既有属性权威返回有效 acknowledgement 后，候选 registry 与 immutable transaction receipt 才一次提交。native rejection 会丢弃候选，因此同一 deterministic command 可直接重试，不需要 pending 镜像表或第二套 modifier authority。

本阶段结论为 **PASS**：coordinator 专项 `4/4`、attribute adapter `4/4`、consumer registry `4/4`、consumer projection `3/3`、legacy attributes `4/4`、完整 influence 链 `71/71`、`Shanmen.0_0_10` 全量 `320/320`、changed-file regression gate、自检 `88/88`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 冻结 subject/component 绑定

- `TryCreate` 只接受有效 run/content/subject 与非空 `Udemo_mapAttributeComponent`；
- coordinator ID 由 registry ID 与 subject entity ID 确定性派生；
- attribute component 以 weak pointer 绑定，不接管 UObject 生命周期；组件失效时命令返回 `ComponentUnavailable`，不推进 registry；
- 每条命令必须指向绑定 subject，target mismatch 与 malformed command 均 fail closed；
- `MatchesBinding` 可让上层在交付前核验 exact subject/component pair。

### 2.2 候选 registry 与 native acknowledgement 原子提交

- `Execute` 先复制 coordinator，并只在候选 registry 上执行 P8.26 command；
- registry rejection 直接返回，原 coordinator 与 native component 均不变；
- registry 接受后，把 immutable application result 交给 P8.27 stateless adapter；
- native acceptance 后才生成 transaction receipt、记录 completed transaction，并把候选整体提交到 coordinator；
- native rejection 会丢弃候选，原 registry 保持可重试状态；不保存 pending command、镜像 active table 或额外 attribute state。

### 2.3 immutable replay fence

- 每个已提交 command 保存其原 command 与成功 transaction result；
- 同 command 重放返回 `TransactionReplayed` 与原 transaction receipt，不再次调用 registry/native；
- 因此 Apply 已完成、后来 Remove 也完成后，再重放旧 Apply 不会复活已删除 modifier；
- command ID 与不同 evidence 碰撞时返回 `CommitRejected`；
- consistency 检查交叉核验 coordinator history、registry completed history、receipt、subject、operation 与唯一 ID。

### 2.4 retry、补偿与 teardown

- same-handle different-spec 的 native Apply/Remove rejection 不提交候选 registry；外部冲突清理后，同一 command 可直接成功重试；
- native desired state 已满足时，`ApplyReplayed`／`RemoveReplayed` 同样可完成事务，且明确记录 `bComponentMutated=false`；
- 候选在 native 实际 mutation 后若出现最终一致性拒绝，只对 `bComponentMutated=true` 的结果执行 exact inverse compensation；native replay 没有发生 mutation，因此绝不执行破坏性的反向操作；
- `IsDrained` 只在 coordinator/registry 一致且 active application 为零时成立，为上层 teardown 提供窄门禁；它不把 unrelated native modifiers 纳入本 coordinator 的权威范围。

## 3. 完整性与安全边界

coordinator 不拥有 World、Actor、GAS、计时器、异步任务、RNG、存档、inventory 或 native modifier collection。唯一 active application authority 仍是 P8.26 registry，唯一属性权威仍是 `Udemo_mapAttributeComponent`；coordinator history 只保存已交付 command 的 immutable transaction evidence。

新增 coordinator 生产文件静态扫描：`UWorld/AActor = 0`、`GameplayAbility/GameplayEffect/AbilitySystem = 0`、`RNG = 0`、`SaveGame/ProfileRepository = 0`、`Tick/while = 0`、`TMap = 0`、`TArray<Fdemo_mapActiveModifier> = 0`。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerApplicationCoordinator.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerApplicationCoordinator.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProjectionTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

coordinator header／implementation 分别为 `144`／`400` 行；新增四个 coordinator Automation case。文档加入前，本阶段产品与流程文件新增 `838` 行、删除 `0` 行。长期未跟踪的用户与 0.0.9B 工件未被修改或纳入提交。

## 5. 自动化验证

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---|---|
| `P8.28-FormationInfluenceConsumerApplicationCoordinator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerApplicationCoordinator` | 4 | 0 | 0 | observed | `B756E0CB68255EA5359CE9BD8D4A26CB1111AE33274B766637BA791B31B5D28F` |
| `P8.28-FormationInfluenceConsumerAttributeAdapter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerAttributeAdapter` | 4 | 0 | 0 | observed | `8B12F583F496D46C0F3D756E7757EE2ED3A488034DC54E4976FEFD09FC5FBBDA` |
| `P8.28-FormationInfluenceConsumerRegistry-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerRegistry` | 4 | 0 | 0 | observed | `EE7B9B28D61F880916FFF00168466E05CF13C0E486096BCB4B43988E4AC883BD` |
| `P8.28-FormationInfluenceConsumerProjection-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection` | 3 | 0 | 0 | observed | `11587E304064E17CC0A3F6C66540A3E148A89D79B2A3CF8008788A047A1AFD3D` |
| `P8.28-Attributes-final.log` | `demo_map.V3.Attributes` | 4 | 0 | 0 | observed | `F18FC1ADF7FDB833CAE065D4A2697A0366B4CABD532BE4E91E889E3B4C86E3D1` |
| `P8.28-FormationInfluence-final.log` | `Shanmen.0_0_10.Product.FormationInfluence` | 71 | 0 | 0 | observed | `3A9FCDB4DB44BA63838094DCC068AFF6FCC58371B0B6C9EF94FD4FB713F2983B` |
| `P8.28-Shanmen-full-final.log` | `Shanmen.0_0_10` | 320 | 0 | 0 | observed | `50B13B971C2F0AE9202FAA8A1473B41A647D250D8C48233C30F7F8E15D7B9F07` |

七份最终日志 fatal／unhandled／ensure 均为 `0`，每份都有唯一 RunTests command 与正式 queue-empty marker。

coordinator 专项覆盖：

1. Apply 与 Remove 同时推进 registry/native，Remove 后 drain；
2. native Apply/Remove handle conflict 均丢弃候选 registry，并允许同 command 重试；
3. ApplyReplayed 与 RemoveReplayed 在无 native mutation 时完成事务；
4. 已 Remove 后重放历史 Apply 不会复活 modifier；
5. completed result/receipt 可查询且 replay 不增加 history；
6. null component、wrong subject、wrong component 与 malformed command fail closed。

## 6. 首轮与提交前审查

首次 Editor 构建成功：`5 actions / 22.64s / exit 0`；首轮 coordinator `4/4`，随后七组 Automation 全绿。提交前源码审查发现一项异常路径风险：候选最终校验若失败，原补偿 helper 会对 `ApplyReplayed/RemoveReplayed` 也执行 inverse operation；这两种状态没有实际 native mutation，反向操作反而可能破坏预先存在的正确 desired state。

最终修正让补偿接收完整 native result，并仅在 `bComponentMutated=true` 时反向；无 mutation 的 replay 直接视为无需补偿。专项重试 case 同时增加 ApplyReplayed 与 RemoveReplayed 断言。修正后重新执行 Editor 编译、全部七组 Automation、regression gate 与 Game 编译，全部通过。没有源码编译失败、Automation case 失败或构建环境失败。

## 7. Changed-file regression gate

新增 coordinator path rule，并把 coordinator 纳入 projection、attribute adapter 与 exact attribute mutation 的 consumer coverage。共享 projection tests 与新增 coordinator 源码共同推导十个必跑 group。

```text
REGRESSION_MAP_JSON: PASS Rules=64
SELF_TEST: PASS 88/88
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=10 Logs=7
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=10 Logs=7
```

- mapping SHA-256：`5715E917F4049551905C0EB221240F80AB76E6B39EB540C39E2842B756DE57EC`；
- self-test SHA-256：`551A3E30044FF86FAA8EE30FCA0593772E776037BA9F7E1A80B84F34F6B38823`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial | Succeeded / 5 actions / 22.64s | 0 | `2AA63C29E0F830A9FCB6A1D75F15FC5DB110DA702AB9F85909554A5D7B36691E` |
| Editor review correction | Succeeded / 5 actions / 14.52s | 0 | `B4B9FE77710BB3CB379F4465A431B4D5D536745ECB6234FE775C04F1D9C6AC9F` |
| Editor final | Succeeded / 0 actions / 0.91s | 0 | `BD448C22AD7ECB2C4AB744CEE78B7E9B8F28B7259D799B80A04BCA257E44A901` |
| Game final | Succeeded / 4 actions / 21.79s | 0 | `0EA8E3A4E186D454C356586AE102BFCA00D34C359290FFD861531991206D7EE0` |

- `UnrealEditor-demo_map.dll`：`11931648` bytes，SHA-256 `24AF1835597BE96986C0FF417E54B921D33BD70B04C5536DEF93C475C73AA18D`；
- `demo_map.exe`：`353008640` bytes，SHA-256 `E888A9BFAF0593C1609CCA164DD3A7B51DAE96FFAB78C9C5E7E649BB00A38378`。

## 9. 兼容性与工作区保护

- P8.25 projection/command、P8.26 registry/receipt 与 P8.27 adapter/acknowledgement 的 wire shape 均未修改；
- coordinator 组合既有接口，不绕过 `Udemo_mapAttributeComponent`，也不改变 legacy modifier caller；
- native failure 不污染 registry durable history，历史 replay 不触发 native side effect；
- component weak binding 失效时 fail closed；coordinator 不延长 UObject 生命周期；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；本阶段只 exact-stage 上述七份文件。

`git diff --check` 与最终 `git diff --cached --check`：PASS。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.29 增加窄 consumer command host/resolver：从产品生命周期取得 exact subject attribute component，把 projection command 路由到本 coordinator，并将 transaction result 回送上游；host 只能管理绑定与生命周期，不得复制 registry active state、native modifiers 或 completed transaction history。
