# Dev.D.UE.0.0.10.P8.18.r0 Report

## 1. 结论

P8.18 已完成 formation influence 的纯值 execution command router。调用方提供 `RequestId + ExpectedIntentId`；Router 读取 Host-owned ledger，为该请求确定性派生 `AttemptId`，冻结为一条 P8.15 execution command，并对 exact replay、payload conflict、canonical order、Host binding 与历史 evidence recovery 提供 fail-closed 契约。

本阶段结论为 **PASS**：Router `4/4`、ProductRuntime `4/4`、LeaseExecutor `4/4`、ExecutorAdapter `4/4`、InfluenceHost `4/4`、`Shanmen.0_0_10` 完整回归 `282/282`、changed-file gate、自检 `65/65`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 确定性 request routing

- 新 request 由 caller-owned `RequestId` 与预期 canonical `IntentId` 组成；
- `AttemptId` 使用命名空间 `Shanmen.Formation.InfluenceExecutionAttempt.r1`，按 `LedgerId + RequestId + ExpectedIntentId` 确定性派生；
- 同一 request exact replay 返回完全相同的 `IntentId + AttemptId` command；
- 相同 `RequestId` 搭配不同 intent 时返回 `RequestConflict`，不记录、不执行。

### 2.2 canonical order 与 immutable binding

- 新 command 只允许指向 Host ledger 当前第一条 pending intent；
- out-of-order、空 pending、无 ledger、stale correlation 均在 Router mutation 前拒绝；
- 第一次成功 route 冻结完整 `RunCorrelation + LedgerId`；
- 后续不同 Host／ledger／correlation 返回 `BindingConflict` 或 `CorrelationMismatch`。

### 2.3 Host evidence recovery

- Router 不复制 ledger attempt receipt，只保存 request-to-command binding；
- 全新 Router 可用同一确定性 attempt identity 从 Host ledger 找回已发出的 command；
- recovery 不依赖 intent 仍为 pending，因此 pending 已推进、ledger 已 seal、Host 已 teardown 后仍可恢复；
- 恢复 command 交给既有 Runtime 时，由 P8.15 adapter replay Host evidence，不重入 concrete executor。

### 2.4 transactional state

- Router 接受 `const ProductHost&`，不能写 Host、ledger、session 或 executor；
- 新 record 先写入 Router 候选副本，完整 `IsValid()` 后才原子替换；
- invalid request、顺序错误、冲突与 collision 均保持 Router 原状态；
- `IsValid()` 重算每条 deterministic AttemptId，并检查 RequestId／AttemptId 唯一性。

## 3. 完整性与安全边界

本阶段明确未实现：

- 自动 drain、后台 retry、Tick、timer、cadence、async 或线程；
- executor／ProductRuntime ownership、Host ownership 或第二套 pending queue；
- 自动读取下一 intent 生成 request；caller 仍必须显式提供 `ExpectedIntentId`；
- Actor／World discovery、spawn、组件 mutation、GAS／GameplayEffect 或正式 Buff 数值；
- SaveGame、ProfileRepository、Router persistence、跨进程恢复或网络复制；
- UI、输入、AI 与正式阵法 content。

新 Router 生产文件对 `UWorld`、`AActor`、`UObject`、AbilitySystem／GameplayEffect、timer、async、RNG、spawn、damage 与 persistence API 均为 `0` matches。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutionRouter.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutionRouter.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

ProductHost、dispatch ledger、executor adapter、lease executor 与 product runtime 生产代码均未修改。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.18-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 0 | 1 | `E8D8B42045819B50D6594A14CBE0D8114E10FDB0A09632CA13EC366D5C375B7B` |
| `P8.18-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 0 | 1 | `333FB27CB80A8B64C419BCB0B86764E4998E374A67298321624CEEDF047E05F2` |
| `P8.18-FormationInfluenceLeaseExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor` | 4 | 0 | 0 | 1 | `FEF8B58A6CBBDFED9A5D1EC433A442242A2B13D6FDDEFCB0B30DAAB780B096B2` |
| `P8.18-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 0 | 1 | `03E1EC64757ABDCC0D8EEDED93C72CC1D775D4E1EFC8E1FF85F71AD2D8435B78` |
| `P8.18-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 0 | 1 | `9B5134AF9E574272AB9017BEC335F3F4CCF1023AD2EBA2CDEDA09244033F0543` |
| `P8.18-Shanmen-full-final.log` | `Shanmen.0_0_10` | 282 | 0 | 0 | 1 | `16E2BFA8E05C4627A7A7B27CB06D186D0E9E07E8A742100135BD7CD52AA5AE3D` |

六份最终日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 Router focused case：

1. `RouteExecuteReplay`：两个 canonical intent 各生成一个稳定 command；request 与 runtime exact replay 均不重复执行；
2. `RequestConflictAndOrderFence`：空 Router 的 out-of-order 不绑定；RequestId payload conflict 与提前路由下一 intent 均 fail-closed；
3. `BindingAndHistoricalRecovery`：pending 推进后全新 Router 从 Host attempt evidence 重建同一 command；foreign Host 被拒绝；
4. `RetryAndTerminalDrain`：retry request 保持 pending，新 request 生成新 attempt，Apply／Remove drain 后原 Router replay 与 sealed-ledger fresh recovery 都不重入 executor。

## 6. 首次失败与修正

Automation case 首轮即 `4/4`，最终仍为 `4/4`；产品语义没有测试失败。

开发中有两次 Editor 编译失败，均如实保留：

1. 初版 Router 重放诊断赋值多一个右括号，MSVC `C2059`，UBT `OtherCompilationError`，原生退出码 `1`；删除多余括号后 Editor `4 actions` 成功；
2. 增强 sealed recovery 测试时补丁锚点误落到较早的 adapter terminal test，产生未声明 `Router/RemoveRequest/Runtime` 的 `C2065`，原生退出码 `1`；移动测试块后 Editor `4 actions` 成功。

两次均为源码／测试编辑错误，不是内存、页面文件或构建环境故障。

## 7. Changed-file regression gate

新增 `FormationInfluenceExecutionRouter` path rule，并把 Router contract 加入 ProductHost、dispatch、executor adapter、lease executor 与 product runtime 的 required groups。最终结果：

```text
REGRESSION_MAP_JSON: PASS Rules=52
SELF_TEST: PASS 65/65
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=20 Logs=6
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=20 Logs=6
```

第一条 coverage 为 Source／Scripts gate；第二条为加入 Report／Log 后的 exact-staged gate。

- mapping SHA-256：`3F39EE76EDA9050AE524F32AE9B066F586CBB2A5FF3555D5AB3CEC606501590D`；
- self-test SHA-256：`05AAB10158FFDA90F9E5C0CD9254287CBDD953922B420F984AFFBF635C7D85AC`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Evidence |
|---|---|---:|---|
| Editor initial Router | Failed / `C2059` | 1 | console evidence |
| Editor corrected Router | Succeeded / 4 actions | 0 | console evidence |
| Editor misplaced strengthened test | Failed / `C2065` | 1 | console evidence |
| Editor corrected strengthened test | Succeeded / 4 actions / 6.88s | 0 | console evidence |
| Editor final | Succeeded / up to date / 0.89s | 0 | `37405834F1C95BDAA7E1C433C11FB4A318B0CB059F41EFC2E341BC421A4064BF` |
| Game final | Succeeded / 3 actions / 19.20s | 0 | `2802775450F679BF76754CE6156DBF3B133074E1A62A24572079F9DC5A7B193C` |

- Editor module：`11553280` bytes，SHA-256 `95BFA7E0883E25BFDCB806F50997CF4107365D253CD41DE46755100DB71A65F7`；
- Game executable：`352690176` bytes，SHA-256 `D401F956D87B3F52B02FB9D4C8F5AFE2705CE80E100FEF4115CA7FAC75952F7B`。

## 9. 兼容性与工作区保护

- Router 只依赖现有 public ProductHost／ledger／execution-command API；
- 既有 caller-owned command 与直接 adapter／runtime 调用路径保持不变；
- Host ordering、retry、acknowledgement、seal、teardown 与 executor semantic lease authority 未迁移；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 7 个文件。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.19 增加窄 single-step execution service：组合 P8.18 Router 与 P8.17 Runtime，一次显式 request 最多 route／execute 一条 intent，同时继续禁止自动 drain、后台 retry、Host ownership、Actor discovery 与 GAS 数值实现。
