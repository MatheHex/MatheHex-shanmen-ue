# Dev.D.UE.0.0.10.P27.26.r0 Report

## 1. 结论

P27.26 已在 P27.25 `FormationScatterWorldPublicationCommandHost` 外建立唯一的 Run 绑定路由 `FormationScatterWorldPublicationRunRoute`。路由只能从一个已经提交的 P27.22 Handoff Evidence 与其内部 P27.19 Resource Plan 所记录的精确 `ActiveRunId` 打开；上层只能提交 `Publish`、`Cancel` 或 `End` 生命周期事件，不能自带 Command ID、绕过 Host、改写资源/Deployment 真值或直接操作 Actor。

Route ID 由 Run ID 与 P27.25 Host ID 确定性派生；每种事件的 Command ID 由 Route ID 与事件名确定性派生。相同事件的精确成功重放由 Host 直接返回既有回执，测试已用 `nullptr` World 证明不会再次进入 World；外来 Run、无效事件、错误 World 与终态原因改写全部失败关闭。进程内接管会整体移动 Route 与 Host 并使旧 Owner 失效；进程后按相同 Handoff 重建会派生相同身份，并由下层既有标签权威接管同一批 Actor。

专项测试 4/4，完整 `Shanmen.0_0_10` 根组 1,408/1,408，映射自检 504/504，改动驱动的 21 个证据组由专项日志与完整根组覆盖，Editor 与 Game 双目标构建均成功，最终覆盖门通过。

## 2. 阶段问题与范围

P27.25 已让调用方只能通过不可变命令 Host 操作 P27.24 Publication Session，但调用方仍要自行持有 Command ID 并理解 Host 命令协议；Host 本身也没有把 Handoff 内的资源 Run 与上层 Run 生命周期绑定。P27.26 闭合以下边界：

- 一个 Route 只能绑定一个 P27.25 Host 和一个已经写入资源计划的 Run；
- 传入 Run 必须等于 `Handoff -> Deployment -> Resource Evidence -> Plan -> ActiveRunId`；
- Route 与 Publish/Cancel/End Command 身份必须确定性生成，调用方不能注入任意 GUID；
- 上层事件只能翻译为 P27.25 不可变命令，所有 World 副作用仍由 Host/Session/Publisher/World Adapter 完成；
- 外来 Run 与无效事件必须在 Host 发生任何状态变化前被拒绝；
- 精确成功重放不得要求 World，也不得重复生成或清理 Actor；
- 进程内 Owner 转移必须使旧 Route 失效并完整保留 Host 回执；
- 进程后重建必须派生相同 Route/Command 身份并接管既有标签批次；
- 本阶段不增加全局 Route Registry、玩家输入、轮询、调度器、后台重试、持久化路由日志或第二套 World/资源权威。

## 3. Run Binding 与确定性身份

新增 `Edemo_mapShanmenFormationScatterWorldPublicationRunEvent`，只接受 `Publish`、`Cancel` 与 `End`。`TryOpen` 先验证 Handoff 的完整证据链，再要求调用方 Run ID 精确等于嵌套 Resource Plan 的 `ActiveRunId`；之后只通过 P27.25 `CommandHost::TryOpen` 建立唯一 Host。

Route ID 使用命名空间 `demo_map.Formation.ScatterWorldPublicationRunRoute.r1`，由以下规范部分确定性派生：

1. `ActiveRunId`；
2. P27.25 Host ID。

Command ID 使用命名空间 `demo_map.Formation.ScatterWorldPublicationRunCommand.r1`，由 Route ID 和事件名 `Publish`、`Cancel` 或 `End` 派生。`IsValid()` 会持续重新派生 Route ID，并再次检查 Host 内 Handoff 的资源计划仍绑定同一 Run，因此任意 GUID 或外来 Run 无法伪装成有效路由。

## 4. 事件翻译、重放与失败关闭

`TryRoute` 固定按以下顺序执行：

1. 验证 Route 自身及嵌套 Host/Session/Handoff/Plan；
2. 验证调用方 `ExpectedRunId`；
3. 验证上层事件；
4. 派生该事件唯一 Command ID；
5. 通过 P27.25 `TryCapturePublish`、`TryCaptureCancel` 或 `TryCaptureEnd` 捕获不可变命令；
6. 只调用 Host 的 `TrySubmit`，并将 Host 的 `Applied`、`Recovered` 或 `Replayed` 结果映射为 Route 回执。

Route 不保存 `UWorld*`；World 只作为一次调用的借用参数下传。P27.25 Host 已保存成功命令回执，所以相同 Route 事件重放可传入 `nullptr`，直接返回相同 Route ID、Command ID 与 Host/Session 证据，不再访问 World。

外来 Run 返回 `RunMismatch`，无效事件返回 `EventInvalid`；两者都发生在命令捕获与 Host 调用之前。错误 World 由下层返回 `WorldConflict` 并映射为 `HostRejected`，不会提交终态，原命令随后可在正确 World 重试。`End` 成功后再提交 `Cancel` 会沿 P27.25 Terminal 身份槽返回 `OperationIdentityConflict`，不会改写既有终态。

## 5. Owner 转移与进程后重建

`TryTakeover` 只允许有效旧 Route 移入完全空的新 Route。它先复制 Route/Run 身份，再调用 P27.25 Host 的唯一 `TryTakeover` 移动完整 Session、Publication Ledger、Completion/Teardown Evidence 与命令记录，最后清空旧 Route。自转移、源无效或目标已有状态均失败关闭。

进程后重建不伪造内存账本：使用同一 Handoff 与 Actor 类重新 `TryOpen`，会得到相同 Route ID、Host ID 和各事件 Command ID，但新 Host 的内存记录从空开始。首次 Publish 仍进入下层 Session；P27.23/P27.18 既有发布链按 Deployment/Placement 标签接管 World 中原有 Actor，因此 Actor 指针集合和数量不变。随后 Cancel/End 仍沿唯一权威路径清理该批 Actor。

## 6. P27.26 专项证明

四条真实无头瞬态 World/Actor 自动化覆盖：

1. `PublishReplayAndRunBinding`：Route 绑定嵌套 `ActiveRunId`；首次 Publish 生成 2 个具体 `ACharacter`；以 `nullptr` World 精确重放仍返回相同 Route/Command/Completion 证据且 Actor 数不增长；物品 Authority 快照不变。
2. `TakeoverPreservesOwnerAndEndRoute`：进程内转移后旧 Route 为空且无效，新 Route 保留 Route/Host ID 与 Publish 回执；End 清理 2 个 Actor，`nullptr` World 终态重放稳定。
3. `ReconstructionAdoptsAndCancelsBatch`：进程后重建派生相同 Route 与 Publish Command ID，重新 Publish 接管原 Actor 指针集合且数量仍为 2；Cancel 后数量为 0。
4. `RunWorldAndTerminalConflicts`：外来嵌套 Run 无法打开 Route；外来 Expected Run 与无效事件在 Host 变更前被拒；错误 World 的 End 可在正确 World 重试；End 后 Cancel 终态改写失败关闭。

专项从 2026-09-13 22:00:07.472 UTC 运行至 22:00:22.807 UTC，持续 15.335 秒；结果 4 Success、0 Fail、0 Fatal/Unhandled/Ensure、原生退出 0。日志 SHA-256 `A6AE19AAC99CC11C2A8CF0F29ADB0BE139295B41FE1BD2DD01BD438746C1C5B3`。

## 7. 自动化与回归证据

| 范围 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationScatterWorldPublicationRunRoute | 4 | 0 | A6AE19AAC99CC11C2A8CF0F29ADB0BE139295B41FE1BD2DD01BD438746C1C5B3 |
| Shanmen.0_0_10 完整根组 | 1,408 | 0 | 4F5E7CEC16CA5C35F636E7435A6B1B2206C36FB84E1084D62CEC572780C5D1F5 |

完整根组从 2026-09-13 22:01:07.742 UTC 运行至 23:13:14.445 UTC，持续 1 小时 12 分 6.703 秒；原生退出 0。联网可用性探测与运行中的默认音频设备交换只作为既有 Warning/Display 事件记录；测试继续推进并完成，Fail、Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

完整 `Shanmen.0_0_10` 根日志覆盖映射要求的全部 21 个组，专项日志再独立证明本阶段新增四项。覆盖脚本逐一验证每份证据的命令根、成功事件、失败事件、终止标记与致命信号，且不会用专项组替代其依赖链或完整根组。

## 8. 回归映射、构建与静态边界

`ShanmenRegressionMap.json` 新增 `FormationScatterWorldPublicationRunRoute` 规则，覆盖新 Route 文件和共享测试宿主；它要求 Route、P27.25 Host、P27.24 Session、Publication、Handoff、World Delivery、Deployment/Resource 链、Mastery/Material Adapter、Formation Session、Items、WorldGameplay、Formation Mastery/Deployment、CombatCore 与完整根组，共 21 个唯一组。

- 映射自检：`SELF_TEST: PASS 504/504`，SHA-256 `507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=7 Rules=6 Required=21 Logs=2`，SHA-256 `C26949AAB8AD0B913E5F116318029CEE45E5F734B8A5CFA702263EC5A900A52E`；
- Editor：0 actions，Result Succeeded，原生退出 0，总计 2.38 秒；日志 SHA-256 `D9A210D7066E3E3C0E2DCFDDBB1DA2AD3BD49DE198D03A03F1317F91B0BDED7B`；
- `UnrealEditor-demo_map.dll`：20,178,432 bytes，SHA-256 `BDB79E0ECF436B2F35257EF49DE69B2B1FB1F6DCD70FA2D0493775E4A1FEBC55`；
- Game：4 actions，Result Succeeded，原生退出 0；UBA 39.32 秒，总计 42.06 秒；日志 SHA-256 `03DC762BF830F8F14DE3959F80A757F37DD7A090553F00EA70CD5724FAA24139`；
- `demo_map.exe`：360,724,480 bytes，SHA-256 `3790EEB48D6889455AC6FA02F8B6C83ACE81E9575233AD39BA3D8C83EBD5C06B`。

新 Route 头/实现共 422 行、387 个非空行。静态扫描未发现 Inventory、Profile、SaveGame、Tick/Timer、异步、随机 GUID、RNG、Actor 枚举、直接 Spawn/Destroy Actor 或 Actor transform/owner 改写；`UWorld*` 只存在于调用参数，不是 Route 字段。Regression Map JSON、`git diff --check` 与最终暂存差异检查全部通过。

## 9. P/F 边界

P 阶段完成：已提交资源计划的 Run → P27.22 Handoff → P27.25 Command Host → P27.26 确定性 Run Route → Publish/Replay/Recover → Owner 转移或进程后重建接管 → Cancel/End → 稳定 Route、Command 与 Session 回执。真实无头瞬态 World 中验证了 2 个 Actor 的生成、无增长重放/接管和终态移除。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实玩家输入、截图、Smoke、Cook 或 Package；没有修改正式地图或内容资产；没有把 Route 接入既有 `FormationRunLifecycle` 唯一组合槽或 `GameMode` 产品调用路径；没有声明正式游戏流程已经可见或可玩。

## 10. 下一阶段与 GitHub

建议 P27.27 将 P27.26 Route 作为 `Fdemo_mapShanmenFormationRunLifecycle` 内唯一、可选的 scatter-publication 槽：只从已经提交的 Handoff 装载；由显式产品生命周期调用 Publish；在 formation 产品 teardown 与共享 Combat Run 释放之前先路由 Cancel/End 并保存可重放检查点；Owner 转移后只允许新生命周期根继续路由。继续禁止物理输入、全局 Registry、第二套 Actor/资源/Deployment/命令真值与后台自动重试。

基线提交：`e152ad194b293437f6915708ee1c44e3cdbd4a7b`（P27.25）。分支：`agent/0.0.10-p27-26-formation-scatter-world-publication-run-route`。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-26-formation-scatter-world-publication-run-route>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-26-formation-scatter-world-publication-run-route/Docs/Report/Dev.D.UE.0.0.10.P27.26.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-26-formation-scatter-world-publication-run-route/Docs/Log/Dev.D.UE.0.0.10.P27.26.r0_log.md>
