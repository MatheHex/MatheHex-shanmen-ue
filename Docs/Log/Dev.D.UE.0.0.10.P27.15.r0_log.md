# Dev.D.UE.0.0.10.P27.15.r0 Development Log

## 1. 目标

- 在 P27.14 熟练度能力策略与未来真实进度/存档来源之间建立单向只读边界；
- 不在来源未确定时新增 Profile/schema 字段或第二套运行时真值；
- 将玩家、权威修订、内容版本与熟练度档位冻结为确定性证据；
- 严格一次读取，所有无效、外来、过期或伪造证据失败关闭；
- 完成改动驱动回归、双目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`6cf88ead5023c99cae0f73b75afa9c01c67e07dd`（P27.14）；
- 分支：`agent/0.0.10-p27-15-formation-mastery-authority-adapter`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 源码搜索未发现 Formation Mastery/proficiency 的既有存档字段或产品权威；
- 参考 `demo_mapShanmenFormationKnowledgeAuthorityAdapter` 的一次只读权威投影模式；
- 不修改 Profile/schema、角色属性、阵法产品路由、输入、UI、地图、内容资产或升级规则；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 3. 新增文件与类型

新增：

- `Source/demo_map/demo_mapShanmenFormationMasteryAuthorityAdapter.h`；
- `Source/demo_map/demo_mapShanmenFormationMasteryAuthorityAdapter.cpp`；
- `Source/demo_map/demo_mapShanmenFormationMasteryAuthorityAdapterTests.cpp`。

主要类型：

1. `Fdemo_mapShanmenFormationMasteryAuthorityCapture`：调用方权威的临时输出；
2. `Fdemo_mapShanmenFormationMasteryAuthorityRead`：验证后不可变、身份绑定的读取证据；
3. `Fdemo_mapShanmenFormationMasteryProjectionResult`：保存状态、诊断、请求上下文、读取次数和读取证据；
4. `Fdemo_mapShanmenFormationMasteryAuthorityAdapter`：执行一次权威读取并投影 P27.14 策略。

## 4. 身份与确定性

有效读取必须同时具备有效玩家、非负权威修订、有效内容戳和正式熟练度档位。`TryCapture()` 先重置输出；验证或策略创建失败时不会泄漏旧证据。

`ReadId` 由固定命名空间、玩家 GUID、权威修订、规范化内容版本、内容摘要与熟练度档位派生。完全相同的权威读取得到相同 ID；修订变化或档位变化得到不同 ID。

## 5. 单次读取与状态机

`Project()` 在内容或玩家前置条件无效时直接拒绝，`AuthorityReadCount == 0`。其它路径严格调用回调一次，`AuthorityReadCount == 1`，不内部重试。

结果状态为：`ContentInvalid`、`OwnerInvalid`、`AuthorityUnavailable`、`AuthorityReadInvalid`、`OwnerMismatch`、`ContentMismatch` 或 `Projected`。默认 `Invalid` 结果不合法；每种失败状态都必须具有与其读取边界一致的可验证证据。

成功投影复用 P27.14 的 `FShanmenFormationMasteryPolicy`。本适配器不复制能力矩阵、不保存等级、不推断回退等级，也不执行升级。

## 6. 专项自动化

新增四条测试：

1. `DeterministicSingleReadProjection`：每次投影恰好一次读取、完全重放同一 ID，并携带 Intermediate 能力；
2. `PreflightAndAvailabilityFences`：无效内容/玩家零读取，不可用与畸形权威一次读取后失败关闭；
3. `OwnerContentAndTierFences`：外来玩家、过期内容与伪造档位均拒绝；
4. `RevisionAndTierIdentity`：修订与档位进入 ID，失败重捕获清空旧读取。

## 7. 自动化结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `EFAD942E58200DEA2500A84EFBE41C596A21A4F5094950478C7D93311573B15A` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `040054AAD6253A137EF68A87E60F27A04E222319667B93AF88E0845130B129D3` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `CBE27A6E1BA330010931CFFAF77FFD0B2A7C2A6A2E4EE32543F557D3D2169FBC` |
| `Shanmen.0_0_10` | 1,361 | 0 | `48C71FBC10518CE8B694178D089F8E8D71B0B476E72A3BF30E09981927971A81` |

所有日志均具备原生退出 0、UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、0 Fail 与 0 Fatal/Unhandled/Ensure。专项执行时间分别约 0.107、0.064 与 0.188 秒；完整根组执行 4,739.897 秒。

## 8. 回归映射

- `Scripts/ShanmenRegressionMap.json` 新增 `FormationMasteryAuthorityAdapter` 规则；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1` 新增专项 fixture、正向映射及“专项不可替代策略/核心/完整证明”的负向测试；
- JSON 解析：PASS；
- 正反自测：`482/482` PASS；
- 覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=4`；
- `git diff --check`：PASS。

四份日志对应全部必跑组，覆盖工具记录的 SHA-256 与本日志第 7 节一致。

## 9. 构建、静态边界与 P/F

Editor：

- 7 actions；`SUCCEEDED`；原生退出 0；29.81 秒；
- run-state SHA-256：`470330F4D654C15B6D452DD8921418E5096CCCCD92ABB7E87350585B3E98F1AB`；
- stdout SHA-256：`8B5F6BEE2289F599BD79F2A79B7C0F487DE7B201EE790539F0376A790DA525F9`；
- `UnrealEditor-demo_map.dll`：19,674,624 bytes；SHA-256 `7D7D463599800EB7E41B48F7E9EC499CB2A6AD3B4CE69A54AAA3B5AC93522E99`。

Game：

- 4 actions；`SUCCEEDED`；原生退出 0；43.98 秒；
- run-state SHA-256：`1AD1166F8616801BE465FE96976A8E0AC15F6631FA22C0B5F5A3CF02B37207D2`；
- stdout SHA-256：`3C1114A92D0B98428FA93CCFAB0049536F3FA08C955AA9BD49C73179F19983A4`；
- `demo_map.exe`：360,301,056 bytes；SHA-256 `2CE96F7A43B1A10E1F603B7E1E2E9197739783E5B8C4F42638C2D9B13D4D4643`。

两次 stderr 均为 0 bytes，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增生产代码 303 行非空行，测试 287 行非空行。生产适配器未引入 World、Actor、控制器、GameMode、Profile/SaveGame、物品/库存、输入、计时器、异步或 RNG 依赖。

P 阶段完成编译、无头自动化、静态边界与证据审计。F 阶段未启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationMasteryAuthorityAdapter.h`
2. `Source/demo_map/demo_mapShanmenFormationMasteryAuthorityAdapter.cpp`
3. `Source/demo_map/demo_mapShanmenFormationMasteryAuthorityAdapterTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.15.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.15.r0_log.md`

`Saved/Codex/P27.15` 与 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.15.r0` 不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-15-formation-mastery-authority-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-15-formation-mastery-authority-adapter/Docs/Report/Dev.D.UE.0.0.10.P27.15.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-15-formation-mastery-authority-adapter/Docs/Log/Dev.D.UE.0.0.10.P27.15.r0_log.md>
