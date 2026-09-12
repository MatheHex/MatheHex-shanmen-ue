# Dev.D.UE.0.0.10.P27.15.r0 Report

## 1. 结论

P27.15 已在 P27.14 的纯函数阵法熟练度策略之上建立单向、只读、失败关闭的产品权威适配器 `Fdemo_mapShanmenFormationMasteryAuthorityAdapter`。

项目当前没有可验证的阵法熟练度存档字段或既有产品权威，因此本轮没有向 Profile/schema、角色、输入或阵法控制器塞入临时真值。适配器通过一次调用方提供的权威读取，把玩家、权威修订、内容版本和熟练度档位冻结为可审计证据，再投影成既有 `FShanmenFormationMasteryPolicy`。无效前置条件零读取；线路不可用、畸形证据、跨玩家、跨内容或伪造档位均失败关闭。

专项 4/4、既有 FormationMastery 2/2、CombatCore 9/9、完整 `Shanmen.0_0_10` 1,361/1,361、Editor/Game 两目标构建、映射自测与改动驱动覆盖门全部通过。

## 2. 阶段问题与范围

P27.14 已定义三档熟练度与操作能力矩阵，但刻意没有定义熟练度从哪里读取。代码审查确认当前 Profile、存档与运行时源码中没有阵法熟练度字段；直接新增字段或让适配器自己维护等级，会在产品决策前制造第二权威和迁移债务。

本轮采用项目已有“知识权威只读投影”模式，只建立以下边界：

- 调用方必须指定有效玩家与当前内容戳；
- 现有进度/存档权威通过只读回调提供一次捕获；
- 捕获必须含同一玩家、非负权威修订、有效内容戳和正式熟练度档位；
- 成功结果只携带不可变读取证据与 P27.14 能力策略；
- 适配器不持有、修改或迁移熟练度数据。

本轮不选择最终存档字段、不制定升级条件、不接物理输入、不执行投材或散材行为，也不创建 UI、World Actor 或重试循环。

## 3. 权威读取证据

`Fdemo_mapShanmenFormationMasteryAuthorityCapture` 是调用方权威返回的临时传输结构，包含：

1. `OwnerId`：熟练度所属玩家；
2. `AuthorityRevision`：权威数据修订，必须不小于 0；
3. `Content`：版本与摘要组成的内容戳；
4. `MasteryTier`：P27.14 的正式档位。

`Fdemo_mapShanmenFormationMasteryAuthorityRead::TryCapture()` 先清空输出，再验证全部字段并创建不可变 `FShanmenFormationMasteryPolicy`。任何失败都不会残留上一份有效证据。

读取 ID 使用固定命名空间 `demo_map.Formation.MasteryAuthorityRead.r1`，由玩家、权威修订、规范化内容版本、内容摘要和熟练度档位确定性派生。相同权威快照可重放为同一 ID；修订或档位变化会得到不同 ID。

## 4. 单次投影与失败关闭

`Project()` 的调用语义为：

- 内容戳无效：`ContentInvalid`，权威读取次数 0；
- 玩家无效：`OwnerInvalid`，权威读取次数 0；
- 回调不可用：`AuthorityUnavailable`，读取次数严格为 1；
- 捕获身份、修订、内容或档位畸形：`AuthorityReadInvalid`；
- 捕获属于其他玩家：`OwnerMismatch`；
- 捕获内容与当前内容不一致：`ContentMismatch`；
- 全部边界通过：`Projected`。

每个非前置拒绝路径最多调用权威一次。适配器没有内部重试、缓存、回退等级或默认放行；调用方必须用下一份明确权威证据重新发起投影。

## 5. 产品能力证明

成功投影会携带一份有效 P27.14 策略，而不是复制熟练度判断。专项测试以 `Intermediate` 权威证据证明：

- 允许 `ProximityFill`；
- 允许 `RemoteThrow`；
- 拒绝 `ScatterFormation`。

这说明产品边界只负责证明“谁、哪个内容版本、哪个权威修订给出了哪一档”，能力矩阵仍由唯一纯函数策略负责。适配器没有重新实现或分叉档位权限。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `EFAD942E58200DEA2500A84EFBE41C596A21A4F5094950478C7D93311573B15A` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `040054AAD6253A137EF68A87E60F27A04E222319667B93AF88E0845130B129D3` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `CBE27A6E1BA330010931CFFAF77FFD0B2A7C2A6A2E4EE32543F557D3D2169FBC` |
| `Shanmen.0_0_10` | 1,361 | 0 | `48C71FBC10518CE8B694178D089F8E8D71B0B476E72A3BF30E09981927971A81` |

四份日志均有 UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、测试进程原生退出 0、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组从 22:16:58.863 UTC 运行至 23:35:58.760 UTC，共 4,739.897 秒。

专项四条分别覆盖：确定性单次读取、前置与可用性边界、玩家/内容/档位边界，以及修订与档位身份；完整根组比 P27.14 增加且仅增加这 4 条，从 1,357 增至 1,361。

## 7. 改动驱动回归

新增 `FormationMasteryAuthorityAdapter` 映射规则。三个适配器路径必须同时具有完整根组、适配器专项、既有 FormationMastery 策略和 CombatCore 证据。

- regression map JSON：PASS；
- 映射器正反自测：`482/482` PASS；
- 负向自测证明只有适配器专项不能替代策略、核心和完整回归；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=4`；
- `git diff --check`：PASS。

## 8. 构建与静态边界

- Editor：7 actions，`SUCCEEDED`，原生退出 0，29.81 秒；stdout SHA-256 `8B5F6BEE2289F599BD79F2A79B7C0F487DE7B201EE790539F0376A790DA525F9`；
- `UnrealEditor-demo_map.dll`：19,674,624 bytes，SHA-256 `7D7D463599800EB7E41B48F7E9EC499CB2A6AD3B4CE69A54AAA3B5AC93522E99`；
- Game：4 actions，`SUCCEEDED`，原生退出 0，43.98 秒；stdout SHA-256 `3C1114A92D0B98428FA93CCFAB0049536F3FA08C955AA9BD49C73179F19983A4`；
- `demo_map.exe`：360,301,056 bytes，SHA-256 `2CE96F7A43B1A10E1F603B7E1E2E9197739783E5B8C4F42638C2D9B13D4D4643`；
- 两次 stderr 均为空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增生产代码 303 行非空行，测试 287 行非空行。生产适配器静态扫描未发现 `UWorld`、`AActor`、PlayerController、GameMode、SaveGame/Profile 类型、库存/物品子系统、InputAction、Timer、Async、随机 GUID 或 RNG 依赖。

## 9. P/F 边界与下一阶段

P 阶段已证明单次读取、零读取前置拒绝、玩家与内容隔离、伪造档位拒绝、确定性重放、修订/档位身份变化、失败捕获清空、完整回归、覆盖门及双目标构建。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。因此本轮不声明玩家存档已有熟练度，也不声明游戏中已按熟练度切换操作。

下一阶段必须由正式产品决定确认熟练度的唯一真实数据源及升级时机，然后把该权威接入此只读回调。确定前可继续开发不依赖该决定的阵法操作原子契约，但不能让 Profile、角色、控制器和适配器分别持有等级。

## 10. GitHub 交接

基线提交：`6cf88ead5023c99cae0f73b75afa9c01c67e07dd`（P27.14）。分支：`agent/0.0.10-p27-15-formation-mastery-authority-adapter`。本阶段只提交 3 个新增源/测试文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.15` 与构建证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-15-formation-mastery-authority-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-15-formation-mastery-authority-adapter/Docs/Report/Dev.D.UE.0.0.10.P27.15.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-15-formation-mastery-authority-adapter/Docs/Log/Dev.D.UE.0.0.10.P27.15.r0_log.md>
