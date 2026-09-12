# Dev.D.UE.0.0.10.P27.14.r0 Development Log

## 1. 目标

- 把人工规划中的阵法熟练度三档转成单一、可测试的运行时契约；
- 让熟练度改变可用操作，而不是只改变伤害、距离或速度数值；
- 保证高级档位保留低级操作；
- 对默认、无效和伪造枚举值失败关闭；
- 不提前绑定熟练度权威、物理输入、World 行为、耗材数值或表现；
- 完成改动驱动回归、双目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`c3b889877423741a28602582af4c11ac3d9eee85`（P27.13）；
- 分支：`agent/0.0.10-p27-14-formation-mastery-contract`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不修改 P27.12/P27.13 产品路由、阵图、材料事务、Profile/schema、输入资产、键位、UI、PlayerController、GameMode、地图或内容资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 3. 新增运行时类型

新增：

- `Source/ShanmenCombatRuntime/Public/ShanmenFormationMastery.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenFormationMastery.cpp`；
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenFormationMasteryTests.cpp`。

正式档位为 `Beginner`、`Intermediate`、`Master`；正式操作为 `ProximityFill`、`RemoteThrow`、`ScatterFormation`。两类枚举均保留 `Invalid` 作为默认失败关闭状态。

## 4. 策略语义

`FShanmenFormationMasteryPolicy` 是不可变 Blueprint 可见值对象。成功创建后只暴露档位和能力查询，不暴露可写字段。

能力矩阵为：

- Beginner：只允许近身填充；
- Intermediate：允许近身填充和远程投材；
- Master：允许全部三种操作；
- 每档的 `GetHighestUnlockedDeliveryMode()` 分别返回近身、远投、散材；
- 所有无效或伪造值均拒绝。

实现使用显式 `switch`/白名单，不把枚举序号当作权限等级，不携带任何平衡数值。

## 5. 失败关闭与测试

`TryCreate()` 首先把输出重置为默认无效策略，再验证档位。这保证用一个既有有效变量接收失败结果时，不会残留旧权限。

两个专项测试覆盖：

1. `Contract`：默认状态、三个正式档位、伪造档位、失败后清空、无效/伪造操作；
2. `CapabilityMatrix`：完整 3×3 权限矩阵、向上保留关系、每档最高操作与 `Invalid` 拒绝。

## 6. 自动化结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `9A51026286B4B20A685E7C41CD7BEE4EB2452B7EE38AD8CCF39410887C0FCA67` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `981E5D1EB50B7081BFD8A724A95090F6D33CD2BA0FCBFF88A36C62010D625AED` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `1539AF0AECA99F7108639722852C72A1FD68B3E7D192D74AFF7C875DAAD30D4A` |
| `Shanmen.0_0_10` | 1,357 | 0 | `18768854DF2EAB2D5DE95AC4BD0AB9152BF5A2F1DA28ABA0CB424C6B2C1C1813` |

所有最终日志均具备原生退出 0、UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、0 Fail 与 0 Fatal/Unhandled/Ensure。专项测试执行 0.034 秒，FormationDeployment 0.101 秒，CombatCore 0.185 秒；完整根组执行 4,027.680 秒。

## 7. 回归映射

- `Scripts/ShanmenRegressionMap.json` 新增 `FormationMasteryPolicy` 规则；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1` 新增专项 fixture、正向映射和“专项不可替代部署/核心/广域/完整证明”的负向测试；
- JSON 解析：PASS；
- 正反自测：`480/480` PASS；
- 覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=5 Logs=4`；
- `git diff --check`：PASS。

## 8. 构建证据

Editor：

- 8 actions；`SUCCEEDED`；原生退出 0；44.80 秒；
- run-state SHA-256：`5F23AD04CB15AF5914B548706B3C0164BB2BF4E0DCD2E5C6A7CCEA49247FE0A4`；
- stdout SHA-256：`67E33D484D3A0ECF864737090576BDD6EA30C3530055E9B0001DB3EE14AF0243`；
- `UnrealEditor-ShanmenCombatRuntime.dll`：2,057,216 bytes；SHA-256 `DFC994331B4FF0A195E73FCEAEE2D4FDB4588009C380706181A0AD66A65831DA`。

Game：

- 5 actions；`SUCCEEDED`；原生退出 0；33.37 秒；
- run-state SHA-256：`6103ABABC8853FF8FB5926975BC2FD42861F63D738283BFC933586E0017100FB`；
- stdout SHA-256：`07FF811BC3FEBCF8BD3C8DD40D1D0090BC3578875520314CB4D34A90CE7B8119`；
- `demo_map.exe`：360,284,672 bytes；SHA-256 `4AEA6F5D61187FD8AA3F3E7EA8CB149127EF30F6C11EF14DA6F76EB37370D453`。

两次 stderr 均为 0 bytes，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

## 9. 静态边界与 P/F

新增生产代码 118 行非空行，测试 104 行非空行。静态扫描未发现 `demo_map`、`UWorld`、`AActor`、随机 GUID/RNG、InputAction、PlayerController、GameMode、Timer 或 Async 依赖。

P 阶段完成原生编译、无头自动化、边界扫描与证据校验。F 阶段未启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. 精确提交清单

1. `Source/ShanmenCombatRuntime/Public/ShanmenFormationMastery.h`
2. `Source/ShanmenCombatRuntime/Private/ShanmenFormationMastery.cpp`
3. `Source/ShanmenCombatRuntime/Private/Tests/ShanmenFormationMasteryTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.14.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.14.r0_log.md`

`Saved/Codex/P27.14` 与 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.14.r0` 不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-14-formation-mastery-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-14-formation-mastery-contract/Docs/Report/Dev.D.UE.0.0.10.P27.14.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-14-formation-mastery-contract/Docs/Log/Dev.D.UE.0.0.10.P27.14.r0_log.md>
