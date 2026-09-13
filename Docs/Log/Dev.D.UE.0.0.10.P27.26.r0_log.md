# Dev.D.UE.0.0.10.P27.26.r0 Development Log

## 1. 目标

- 在 P27.25 Command Host 外建立唯一 Run 绑定的 scatter World-publication 路由；
- 只允许 Handoff 内 Resource Plan 已记录的精确 `ActiveRunId` 打开 Route；
- 确定性派生 Route ID 与 Publish/Cancel/End Command ID，禁止调用方注入任意 GUID；
- 将上层 Run 生命周期事件翻译为 P27.25 不可变命令，不允许绕过 Host；
- 让精确成功重放不再访问 World，并保留稳定 Route/Command/Session 回执；
- 对外来 Run、无效事件、错误 World 和终态原因改写失败关闭；
- 支持进程内唯一 Owner 转移，完整保留 Host 状态并使旧 Route 失效；
- 支持进程后由同一身份重建并接管 World 标签标识的既有 Actor 批次；
- 保持所有 Actor 变更只经 P27.25/P27.24/P27.23/既有 World Adapter；
- 通过真实无头 World 自动化、改动驱动回归、双目标构建、Report/Log 和 GitHub 推送完成交接。

## 2. 基线与边界

- 基线：`e152ad194b293437f6915708ee1c44e3cdbd4a7b`（P27.25）；
- 分支：`agent/0.0.10-p27-26-formation-scatter-world-publication-run-route`；
- 起始 tracked tree clean；
- 103 个既有未跟踪用户文件保持原样且不暂存；
- 输入为精确 Run ID、完整 P27.22 Handoff Evidence、一个具体 Actor 类，以及调用时借用的 World；
- Route 只拥有一个 P27.25 Host，不保存裸 World 指针；
- 源资源、Deployment 与 World Actor 的既有权威不可复制、回滚或重提交；
- 允许 UnrealEditor-Cmd 无头瞬态 World/Actor 测试；
- 禁止 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 和 Package；
- 不修改地图、内容资产、Profile/schema、物品 Authority、Deployment、P27.25 Host 或下层发布/清理算法。

## 3. 新增和修改文件

新增生产文件：

1. `Source/demo_map/demo_mapShanmenFormationScatterWorldPublicationRunRoute.h`
2. `Source/demo_map/demo_mapShanmenFormationScatterWorldPublicationRunRoute.cpp`

修改：

3. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

交接：

6. `Docs/Report/Dev.D.UE.0.0.10.P27.26.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.26.r0_log.md`

新 Route 头/实现共 422 行、387 个非空生产代码行；共享测试宿主新增 278 行；流程映射和自检新增 65 行。

## 4. 实现过程

### 4.1 Run 绑定与 Route 身份

`TryOpen` 首先清空输出并验证 Run、Handoff 与嵌套 Resource Plan。只有传入 Run ID 精确等于 `Handoff -> Deployment Evidence -> Resource Evidence -> Plan -> ActiveRunId` 时，才通过 P27.25 `CommandHost::TryOpen` 创建唯一 Host。

Route ID 使用命名空间 `demo_map.Formation.ScatterWorldPublicationRunRoute.r1`，由 Run ID 和 Host ID 规范派生。Route 有效性持续重新派生该身份，并核对嵌套 Plan 的 `ActiveRunId`，防止 Route 与 Host 或资源 Run 漂移。

### 4.2 事件与命令身份

上层事件只有 `Publish`、`Cancel` 和 `End`。每个事件的 Command ID 使用命名空间 `demo_map.Formation.ScatterWorldPublicationRunCommand.r1`，由 Route ID 与规范事件名派生；调用方不再持有或生成 Command ID。

事件分别通过 P27.25 `TryCapturePublish`、`TryCaptureCancel`、`TryCaptureEnd` 转为不可变命令。Route 只调用 Host 的 `TrySubmit`，并将 Host 的成功状态映射为 `Applied`、`Recovered` 或 `Replayed`；其余有效拒绝映射为 `HostRejected`，同时保留完整嵌套 Command/Session 诊断。

### 4.3 重放、冲突与 World 边界

Route 先验证自身、Expected Run 与事件，再捕获命令和调用 Host；因此外来 Run 与无效事件不会触及 Host。World 只作为一次调用参数下传，Route 不保留 World 指针，也不枚举 Actor。

P27.25 Host 对成功命令保存耐久进程内回执。相同事件的精确重放可传入 `nullptr` World，仍返回相同 Route/Command/Session 证据；这从调用侧证明重放没有进入 World。错误 World 的终态请求不会提交 Host 状态，之后同一确定性命令可在正确 World 继续。End 与 Cancel 共用 P27.25 Terminal 身份约束，成功后不能互相改写。

### 4.4 Owner 转移与重建

`TryTakeover` 要求源 Route 有效、目标 Route 完全为空且不是同一对象。它通过 P27.25 Host 的 `TryTakeover` 整体移动 Session、Publication Ledger、Completion/Teardown Evidence 与命令记录，再使旧 Route 变为空状态。

进程后重建从同一 Handoff/Actor 类派生相同 Route、Host 与 Command 身份。内存命令记录合理为空，首次 Publish 委托下层按 Deployment/Placement 标签接管既有 Actor；没有生成第二批 Actor。随后 Terminal 事件沿同一 Authority 路径清理该批次。

## 5. 专项测试与自查

新增四条专项：

1. Publish 首次应用、`nullptr` World 精确重放、Run 绑定与物品 Authority 不变；
2. Owner 转移保留 Route/Host/回执、旧 Route 失效、End 路由与终态重放；
3. 进程后重建派生相同身份、接管既有 Actor 指针集合、Cancel 清理；
4. 外来嵌套 Run、外来 Expected Run、无效事件、错误 World 与终态原因改写围栏。

所有测试使用真实无头瞬态 `UWorld`，发布具体 `ACharacter`；核对 Actor 数量/指针集合、Route/Host/Command 身份、Host 记录数、Session 状态、Completion/Teardown 回执与资源 Authority 快照。

专项从 2026-09-13 22:00:07.472 UTC 至 22:00:22.807 UTC，持续 15.335 秒；4 Success、0 Fail、0 Fatal/Unhandled/Ensure、原生退出 0；日志 SHA-256 `A6AE19AAC99CC11C2A8CF0F29ADB0BE139295B41FE1BD2DD01BD438746C1C5B3`。

静态边界扫描覆盖新头/实现，未发现 Inventory、Profile、SaveGame、Tick/Timer、异步、随机 GUID、RNG、Actor 枚举、直接 Spawn/Destroy Actor 或 Actor transform/owner 改写。`UWorld*` 只存在于方法参数，没有成为持久字段。

## 6. 改动驱动回归

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationScatterWorldPublicationRunRoute | 4 | 0 | A6AE19AAC99CC11C2A8CF0F29ADB0BE139295B41FE1BD2DD01BD438746C1C5B3 |
| Shanmen.0_0_10 full | 1,408 | 0 | 4F5E7CEC16CA5C35F636E7435A6B1B2206C36FB84E1084D62CEC572780C5D1F5 |

完整根组的命令根 `Shanmen.0_0_10` 覆盖映射要求的全部 21 个唯一组，包括 Route、Command Host、Session、Publication、Handoff、World Delivery、Deployment/Resource、Mastery/Material、Formation Session、Items、WorldGameplay、Formation Mastery/Deployment 与 CombatCore。专项日志保留新增四项的精确证据；不为同一根组重复生成 19 份子集日志。

完整根组首项于 2026-09-13 22:01:07.742 UTC 开始，末项于 23:13:14.445 UTC 完成，持续 1 小时 12 分 6.703 秒；1,408 项成功，0 项失败，原生退出 0，Fatal/Unhandled/Ensure 信号 0。联网探测超时和运行中的默认音频设备交换只产生非失败日志，设备随后恢复且测试正常完成。

## 7. 回归映射

新增 `FormationScatterWorldPublicationRunRoute` 路径规则，同时覆盖两个新 Route 文件与共享测试宿主。规则要求 21 个唯一组，并显式包含完整 `Shanmen.0_0_10` 根组。

Self-test 新增：

- 正向夹具：完整 21 组证据可覆盖 Route 文件；
- 负向夹具：只有 Route 专项不能替代 Host、Session、Publication、Handoff、资源、World 与完整根组。

结果：

- `SELF_TEST: PASS 504/504`；
- Self-test SHA-256：`507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`；
- `REGRESSION_COVERAGE: PASS Changed=7 Rules=6 Required=21 Logs=2`；
- Coverage SHA-256：`C26949AAB8AD0B913E5F116318029CEE45E5F734B8A5CFA702263EC5A900A52E`。

## 8. 构建、产物与卫生

Editor：

- 目标已是最新，0 actions；Result Succeeded，原生退出 0，总计 2.38 秒；
- 日志 SHA-256 `D9A210D7066E3E3C0E2DCFDDBB1DA2AD3BD49DE198D03A03F1317F91B0BDED7B`；
- `UnrealEditor-demo_map.dll` 20,178,432 bytes；
- DLL SHA-256 `BDB79E0ECF436B2F35257EF49DE69B2B1FB1F6DCD70FA2D0493775E4A1FEBC55`。

Game：

- 新源文件触发 4 actions；UBA 39.32 秒，总计 42.06 秒；Result Succeeded，原生退出 0；
- 日志 SHA-256 `03DC762BF830F8F14DE3959F80A757F37DD7A090553F00EA70CD5724FAA24139`；
- `demo_map.exe` 360,724,480 bytes；
- EXE SHA-256 `3790EEB48D6889455AC6FA02F8B6C83ACE81E9575233AD39BA3D8C83EBD5C06B`。

卫生：

- Regression Map JSON 解析 PASS；
- `git diff --check` PASS；
- 最终暂存差异检查 PASS；
- 最终暂存精确为 7 个阶段实现、测试、映射和交接文件；
- 103 个既有未跟踪用户文件保持原样且未暂存。

## 9. P/F 边界

P 阶段完成：权威资源 Run → Handoff → P27.25 Host → P27.26 Run Route → Publish/Replay/Recover → Owner 转移或进程后重建接管 → Cancel/End → 稳定 Route/Command/Session 回执。真实无头瞬态 World 中验证了 2 个 Actor 的生成、无增长重放/接管与终态销毁。

F 阶段未执行：没有 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；没有正式地图视觉验收；没有把 Route 接入 `FormationRunLifecycle` 或 `GameMode`；没有全局 Route Registry 或持久化路由日志。

## 10. 下一阶段与 GitHub

建议 P27.27 在既有 `Fdemo_mapShanmenFormationRunLifecycle` 内增加唯一可选 scatter-publication Route 槽：从已提交 Handoff 装载，在显式产品生命周期调用中路由 Publish，并在 formation 产品 teardown 和共享 Combat Run 释放之前完成可重放的 Cancel/End。Owner 转移后旧生命周期根不得继续路由。物理输入仍留在 F 阶段。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-26-formation-scatter-world-publication-run-route>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-26-formation-scatter-world-publication-run-route/Docs/Report/Dev.D.UE.0.0.10.P27.26.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-26-formation-scatter-world-publication-run-route/Docs/Log/Dev.D.UE.0.0.10.P27.26.r0_log.md>
