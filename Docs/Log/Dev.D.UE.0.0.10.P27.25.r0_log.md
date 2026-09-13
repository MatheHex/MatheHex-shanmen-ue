# Dev.D.UE.0.0.10.P27.25.r0 Development Log

## 1. 目标

- 在 P27.24 Publication Session 外建立唯一调用方命令所有者；
- 确定性绑定 Host、Session、Handoff、Deployment 与 Actor 类；
- 将 Publish、Cancel、End 冻结成带显式 Command ID 的不可变命令；
- 保存可审计命令回执并让精确成功重放不再访问 World；
- 对 Command ID、操作身份、Binding、World 和终态改写失败关闭；
- 支持可恢复 Session 拒绝由原命令继续，不允许换身份重试；
- 支持进程内唯一 Owner 转移，完整保留 Session 与命令回执并使旧 Host 失效；
- 支持进程后由同一 Host 身份重建并接管 World 标签标识的既有 Actor；
- 保持所有 Actor 变更只经 P27.24/P27.23/既有 World Adapter；
- 通过真实无头 World 自动化、改动驱动回归、双目标构建、Report/Log 和 GitHub 推送完成交接。

## 2. 基线与边界

- 基线：`96a01571626b41105c35388794e05bf8b3f02b83`（P27.24）；
- 分支：`agent/0.0.10-p27-25-formation-scatter-world-publication-command-host`；
- 起始 tracked tree clean；
- 103 个既有未跟踪用户文件保持原样且不暂存；
- 输入为完整 P27.22 Handoff Evidence、一个具体 Actor 类、一个显式 Command ID 和调用时 World；
- 一个 Host 只拥有一个 P27.24 Session，不保存裸 World 指针；
- 源资源、Deployment 与 World Actor 的既有权威不可复制、回滚或重提交；
- 允许 UnrealEditor-Cmd 无头瞬态 World/Actor 测试；
- 禁止 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 和 Package；
- 不修改地图、内容资产、Profile/schema、物品 Authority、Deployment 或 P27.24 发布/清理算法。

## 3. 新增和修改文件

新增生产文件：

1. `Source/demo_map/demo_mapShanmenFormationScatterWorldPublicationCommandHost.h`
2. `Source/demo_map/demo_mapShanmenFormationScatterWorldPublicationCommandHost.cpp`

修改：

3. `Source/demo_map/demo_mapShanmenFormationScatterWorldPublicationSession.h`
4. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`
5. `Scripts/ShanmenRegressionMap.json`
6. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

交接：

7. `Docs/Report/Dev.D.UE.0.0.10.P27.25.r0_report.md`
8. `Docs/Log/Dev.D.UE.0.0.10.P27.25.r0_log.md`

新 Host 头/实现共 779 行、723 个非空生产代码行；Session 只增加 5 行只读 getter；共享测试宿主新增 333 行；流程映射和自检新增 63 行。

## 4. 实现过程

### 4.1 Binding 与命令捕获

Host ID 使用命名空间 `demo_map.Formation.ScatterWorldPublicationCommandHost.r1`，由 Session ID、Handoff Evidence ID、Deployment ID 和 Actor Class Path 规范派生。Binding 验证会重新派生并核对全部字段。

Session 暴露只读 Actor Class Path；Host 不复制 Actor 类判断。命令只能经 `TryCapturePublish`、`TryCaptureCancel`、`TryCaptureEnd` 创建，冻结显式 Command ID、完整 Binding 与操作类型。

### 4.2 Host 状态与命令记录

`TryOpen` 先通过既有 P27.24 `TryStart` 建立 Session，再从 Session/Handoff 派生 Binding。Host 有效性持续核对 Session 与 Binding 一致、Command ID 唯一、最多一条 Publish 和一条 Terminal、Session 终态与 Terminal 命令一致，以及 Publish 记录与 Session 发布状态一致。

每次提交先在 Host 副本上执行；只有 Session 成功或明确可恢复拒绝形成有效耐久记录后才整体提交，避免半写 Host。不可恢复拒绝只返回结果，不污染命令记录。

### 4.3 重放、冲突与恢复

同一成功命令精确重放直接返回存储回执，标记 `Replayed`，不再调用 Session。相同 Command ID 但 Binding/Operation 不同返回 `CommandIdConflict`；已有同类操作但 Command ID 不同返回 `OperationIdentityConflict`；`Cancel` 与 `End` 被视为同一个 Terminal 身份槽。

Publish 只有在 Session 已保存规范部分 Ledger 且返回 `PublicationRejected` 时可恢复。Terminal 只有在下层明确返回 `TeardownRecoveryRequired` 时可恢复。恢复仍使用原 Command ID，成功更新原记录并对调用方返回 `Recovered`；另一轮仍可恢复的拒绝只前进同一记录。

### 4.4 Owner 转移与重建

`TryTakeover` 要求源 Host 有效、目标 Host 完全空且不是同一对象；成功后整体移动 Binding、Session 与 Records，再重置旧 Host。目标不是第二份 Host 真值，旧 Host 立即无效。

进程后重建通过同一 Handoff/Actor 类得到相同 Host ID。此时内存 Records 合理为空；Publish 进入原 P27.24 Session，再由下层 World Adapter 根据 Deployment/Placement 标签接管既有 Actor。因此重建不依赖伪造持久化账本，也不会生成第二批 Actor。

## 5. 专项测试与自查

新增四条专项：

1. Publish 首次应用、精确重放、Command ID 冲突与操作身份冲突；
2. Owner 转移保留 Host/回执、旧 Host 失效、End 路由与终态重放；
3. 进程后重建派生同一 Host ID、接管既有 Actor 指针集合、Cancel 清理；
4. 无效命令、外来 Binding、错误 World 和 Terminal 身份冲突。

所有测试使用真实无头瞬态 `UWorld`，发布具体 `ACharacter`；Binding 冲突另用 `APawn` 类证明 Actor Class Path 会改变 Host 身份。测试核对 Actor 数/指针集合、Host 记录数、Session 状态、Completion/Teardown 回执和物品 Authority 快照。

专项结果：4 Success、0 Fail、0 Fatal/Unhandled/Ensure、原生退出 0；日志 SHA-256 `0A1BE0AC717C6736BF1BACB8EB0AE4D5D54D2A25156FEF7E3322615D9F811130`。

## 6. 改动驱动回归

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationScatterWorldPublicationCommandHost | 4 | 0 | 0A1BE0AC717C6736BF1BACB8EB0AE4D5D54D2A25156FEF7E3322615D9F811130 |
| Shanmen.0_0_10 full | 1,404 | 0 | 9BB9463D583EB19D6FE4F5093B800554D388223491B46BFA46AB3FA739140FED |

完整根组的命令根 `Shanmen.0_0_10` 覆盖映射要求的全部 20 个唯一组，包括 Command Host、Session、Publication、Handoff、World Delivery、Deployment/Resource、Mastery/Material、Formation Session、Items、WorldGameplay、Formation Mastery/Deployment 与 CombatCore。专项日志保留新增四项的精确证据；不再为同一根组重复生成 18 份子集日志。

完整根组首项于 2026-09-13 20:06:53.471 UTC 开始，末项于 21:12:41.589 UTC 完成，持续 1 小时 5 分 48.118 秒；1,404 项成功，0 项失败，原生退出 0，Fatal/Unhandled/Ensure 信号 0。

## 7. 回归映射

新增 `FormationScatterWorldPublicationCommandHost` 路径规则，覆盖新 Host 文件和共享测试宿主。规则要求 20 个唯一组，且显式包含完整 `Shanmen.0_0_10` 根组。

Self-test 新增：

- 正向夹具：完整 20 组证据可覆盖 Command Host 文件；
- 负向夹具：只有 Command Host 专项不能替代 Session、Publication、Handoff、资源、World 与完整根组。

结果：

- `SELF_TEST: PASS 502/502`；
- Self-test SHA-256：`80BF271AA260914D12ABD5BB22D62045CECC48A344AD36038034B1B13A953429`；
- `REGRESSION_COVERAGE: PASS Changed=8 Rules=5 Required=20 Logs=2`；
- Coverage SHA-256：`CEF370880C8D0EF941274EF45A1C134E44C51202F8298FD50E28D42642839C11`。

## 8. 构建、产物与卫生

Editor：

- 目标已是最新，0 actions；Result Succeeded，原生退出 0，总计 1.44 秒；
- 日志 SHA-256 `AA1BB89364F587BBDB5C7A00147665E4B144E822470E16E8AC2F6C51CFCCDA0B`；
- `UnrealEditor-demo_map.dll` 20,146,688 bytes；
- DLL SHA-256 `5FB58F71F77271BC1E5B17735C846D40D420DD6C31FC6C17F8F143C731455289`。

Game：

- 新源文件触发 5 actions；UBA 21.96 秒，总计 23.97 秒；Result Succeeded，原生退出 0；
- 日志 SHA-256 `A266DF9DA48BABE78A85D5E6034620F2A896DB3BF45D32FDE96B0EDED49C5FDF`；
- `demo_map.exe` 360,699,904 bytes；
- EXE SHA-256 `4FA5049CB2243F9794FFCF94A2D728EB31C35A20621F5FCD961D4A4737BCC846`。

静态与 Git 卫生：

- 新 Host 生产文件未发现 Inventory、Profile、SaveGame、Tick/Timer、异步、随机 GUID、RNG、Spawn/Destroy Actor 或 Actor transform/owner 修改；
- Actor 发布、标签接管、冲突判断与终止清理继续只在既有 Session/Publisher/World Adapter 链；
- Regression Map JSON 解析 PASS；
- `git diff --check` PASS；
- 最终暂存差异检查 PASS；
- 最终暂存精确为 8 个阶段实现、测试、映射和交接文件；
- 103 个既有未跟踪用户文件保持原样且未暂存。

## 9. P/F 边界

P 阶段完成：权威 Handoff → P27.24 Session → P27.25 Command Host → Publish/Replay/Recover → Owner 转移或进程后重建接管 → Cancel/End → 稳定命令与 Session 回执。真实无头瞬态 World 中验证了 2 个 Actor 的生成、无增长重放/接管与终态销毁。

F 阶段未执行：没有 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；没有正式地图视觉验收；没有把 Host 接入唯一 Run-lifecycle Composition/Route；没有全局 Host Registry 或持久化命令日志。

## 10. 下一阶段与 GitHub

建议 P27.26 建立一个 Run-lifecycle Composition/Route：每个 Run 只有一个 P27.25 Host 槽；上层只从已提交 Handoff 装载 Host，并将生命周期意图翻译成冻结 Command。Owner 转移后旧组合根不得继续路由。继续禁止第二套 Actor、资源、Deployment 或命令权威；物理输入延后到 F 阶段。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-25-formation-scatter-world-publication-command-host>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-25-formation-scatter-world-publication-command-host/Docs/Report/Dev.D.UE.0.0.10.P27.25.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-25-formation-scatter-world-publication-command-host/Docs/Log/Dev.D.UE.0.0.10.P27.25.r0_log.md>
