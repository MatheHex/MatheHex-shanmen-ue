# Dev.D.UE.0.0.10.P27.25.r0 Report

## 1. 结论

P27.25 已在 P27.24 `FormationScatterWorldPublicationSession` 外建立唯一的调用方命令所有者 `FormationScatterWorldPublicationCommandHost`。每个 Host 只绑定一个 Session、P27.22 Handoff Evidence、P27.21 Deployment 与具体 Actor 类；调用方只能提交不可变的 `Publish`、`Cancel` 或 `End` 命令，不能直接碰 Publication Ledger、World Adapter、资源 Authority 或 Deployment 真值。

同一命令精确重放直接返回 Host 保存的成功回执，不再访问 World；同一 Command ID 改写负载、同类操作换 Command ID、`Cancel`/`End` 终态改写、外来 Binding 和错误 World 均失败关闭。一次进程内所有权转移会完整移动 Session 与命令回执并使旧 Host 失效；进程后重建则通过确定性 Host ID 与 World 中既有 Deployment 标签接管同一批 Actor，不重复生成。

专项测试 4/4，完整 `Shanmen.0_0_10` 根组 1,404/1,404，映射自检 502/502，改动驱动的 20 个证据组由专项日志与完整根组覆盖，Editor 与 Game 双目标构建均成功，最终覆盖门通过。

## 2. 阶段问题与范围

P27.24 已经统一一批阵眼的 World 发布、恢复、重放和终止清理，但产品调用方仍可直接持有并操作 Session；多个调用者之间没有命令身份、回执所有权或旧 Host 退出顺序。P27.25 闭合以下边界：

- Host 身份必须确定性绑定 Session、Handoff、Deployment 与 Actor 类；
- 一个 Host 对象只能拥有一个活动 Session；
- 调用方意图必须先冻结为显式 Command ID、Binding 与操作类型；
- 精确成功重放不得再次访问或修改 World；
- 可恢复的部分发布/清理失败只能由原命令继续；
- 同一操作不得被另一个 Command ID 改写；
- `Cancel` 与 `End` 共享一条不可改写的终态身份；
- 进程内 Owner 转移必须使旧 Host 立即失效并保留完整回执；
- 进程后重建必须派生同一 Host 身份，并由 P27.24/P27.23/P27.18 既有链接管带标签 Actor；
- 本阶段不增加全局 Registry、玩家输入、调度器、自动重试、持久化命令日志或第二套 World/资源权威。

## 3. 确定性 Binding 与不可变命令

新增 `Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding`。Host ID 使用命名空间 `demo_map.Formation.ScatterWorldPublicationCommandHost.r1`，由以下规范部分确定性派生：

1. P27.24 Session ID；
2. P27.22 Handoff Evidence ID；
3. P27.21 Deployment ID；
4. Actor Class Path。

`IsValid()` 会重新派生 Host ID 并逐项核对，不能用任意 GUID 冒充绑定。Session 新增只读 `GetActorClassPath()`，Host 因而不复制 Actor 类权威。

`Fdemo_mapShanmenFormationScatterWorldPublicationCommand` 只可通过 `TryCapturePublish`、`TryCaptureCancel` 或 `TryCaptureEnd` 生成；成功捕获后同时冻结显式 Command ID、完整 Binding 与操作。Host 在调用 Session 前先验证命令和自身 Binding 精确一致。

## 4. 命令账本、重放与恢复

Host 保存最多一条 Publish 记录和一条 Terminal 记录。每条耐久记录同时保存冻结命令与最新有效结果，并要求 Command ID、操作类型和 Binding 一致。

- 首次成功映射为 `Applied` 并提交 Host 状态；
- 精确成功命令再次提交映射为 `Replayed`，返回已存回执且不访问 World；
- P27.24 返回可恢复的部分发布或 `TeardownRecoveryRequired` 时，保存原命令及拒绝回执；
- 只有同一命令可以再次进入 Session，成功后映射为 `Recovered`；
- 不可恢复的 Session 拒绝不污染 Host 账本；
- 同 Command ID 不同负载返回 `CommandIdConflict`；
- 同操作不同 Command ID，或 `Cancel`/`End` 互换，返回 `OperationIdentityConflict`。

所有实际发布、Actor 标签接管、冲突检测与终止销毁仍沿用 P27.24 Session → P27.23 Publisher → 既有 World Adapter。Host 不生成、销毁或直接改写 Actor。

## 5. Owner 转移与重建

`TryTakeover` 只接受一个有效旧 Host 和一个完全空的新 Host。成功时完整移动 Binding、Session、Publication Ledger、Completion/Teardown Evidence 与命令记录，再把旧 Host 重置为空；自转移、目标已有状态或源 Host 无效都会失败关闭。

这是进程内唯一 Owner 转移，不是复制。旧调用方在转移后无法继续提交命令，新 Host 对既有 Publish/Terminal 命令仍返回相同重放事实。

进程后重建不伪造持久化内存：用同一 Handoff 与 Actor 类重新 `TryOpen` 会得到相同 Host ID，但命令账本为空。首次 Publish 仍委托 P27.24 Session；下层 Adapter 通过 Deployment/Placement 标签接管 World 中既有 Actor，所以 Actor 指针集合和数量保持不变。随后 Terminal 命令沿同一权威路径清理该批 Actor。

## 6. P27.25 专项证明

四条真实无头瞬态 World/Actor 自动化覆盖：

1. `PublishReplayAndCommandIdentity`：首次 Publish 生成 2 个具体 `ACharacter`；精确重放仍为 2 个且回执不变；Command ID 和操作身份改写失败；物品 Authority 快照不变。
2. `TakeoverPreservesReceiptsAndTerminalRoute`：进程内转移后旧 Host 无效，新 Host 保留 Host ID 与 Publish 回执；`End` 清理 2 个 Actor，精确终态重放稳定。
3. `ReconstructionAdoptsTaggedWorldBatch`：进程后重建派生同一 Host ID，重新 Publish 接管原 Actor 指针集合且数量仍为 2；`Cancel` 清理后为 0。
4. `BindingWorldAndTerminalConflicts`：无效命令、外来 Actor 类 Binding、错误 World 与 `End` 后 `Cancel` 均失败关闭；最终 Host 只保存一条 Publish 和一条 Terminal 记录。

专项结果：4 Success、0 Fail、0 Fatal/Unhandled/Ensure、原生退出 0；日志 SHA-256 `0A1BE0AC717C6736BF1BACB8EB0AE4D5D54D2A25156FEF7E3322615D9F811130`。

## 7. 自动化与回归证据

| 范围 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationScatterWorldPublicationCommandHost | 4 | 0 | 0A1BE0AC717C6736BF1BACB8EB0AE4D5D54D2A25156FEF7E3322615D9F811130 |
| Shanmen.0_0_10 完整根组 | 1,404 | 0 | 9BB9463D583EB19D6FE4F5093B800554D388223491B46BFA46AB3FA739140FED |

完整根组从 2026-09-13 20:06:53.471 UTC 运行至 21:12:41.589 UTC，持续 1 小时 5 分 48.118 秒；原生退出 0。联网可用性探测超时仅作为既有测试事件 Warning 记录；Fatal、Unhandled Exception 与 Ensure condition failed 为 0。

完整 `Shanmen.0_0_10` 根日志覆盖映射要求的全部 20 个组，因此不重复执行同一根组下的 18 份子集日志。专项日志独立证明本阶段新增四项；覆盖脚本仍逐项验证命令根、成功事件、失败事件、终止标记与致命信号。

## 8. 回归映射、构建与静态边界

`ShanmenRegressionMap.json` 新增 `FormationScatterWorldPublicationCommandHost` 规则，要求 Command Host、P27.24 Session、P27.23 Publication、P27.22 Handoff、World Delivery、Deployment/Resource 链、Mastery/Material Adapter、Formation Session、Items、WorldGameplay、Formation Mastery/Deployment、CombatCore 与完整根组。

- 映射自检：`SELF_TEST: PASS 502/502`，SHA-256 `80BF271AA260914D12ABD5BB22D62045CECC48A344AD36038034B1B13A953429`；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=8 Rules=5 Required=20 Logs=2`，SHA-256 `CEF370880C8D0EF941274EF45A1C134E44C51202F8298FD50E28D42642839C11`；
- Editor：0 actions，Result Succeeded，原生退出 0，总计 1.44 秒；日志 SHA-256 `AA1BB89364F587BBDB5C7A00147665E4B144E822470E16E8AC2F6C51CFCCDA0B`；
- `UnrealEditor-demo_map.dll`：20,146,688 bytes，SHA-256 `5FB58F71F77271BC1E5B17735C846D40D420DD6C31FC6C17F8F143C731455289`；
- Game：5 actions，Result Succeeded，原生退出 0；UBA 21.96 秒，总计 23.97 秒；日志 SHA-256 `A266DF9DA48BABE78A85D5E6034620F2A896DB3BF45D32FDE96B0EDED49C5FDF`；
- `demo_map.exe`：360,699,904 bytes，SHA-256 `4FA5049CB2243F9794FFCF94A2D728EB31C35A20621F5FCD961D4A4737BCC846`。

新 Host 头/实现共 779 行、723 个非空行。静态扫描未发现 Inventory、Profile、SaveGame、Tick/Timer、异步、随机 GUID、RNG、直接 Spawn/Destroy Actor 或 Actor transform/owner 变更；Regression Map JSON、`git diff --check` 与最终暂存差异检查全部通过。

## 9. P/F 边界

P 阶段完成：P27.22 Handoff → P27.24 Session → P27.25 不可变命令 Host → World 发布/重放/Owner 转移/重建接管 → `Cancelled`/`Ended` 终态路由 → 稳定命令与 Session 回执。真实无头 World 中验证了 2 个 Actor 的首次生成、无增长重放/接管和终态移除。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实玩家输入、截图、Smoke、Cook 或 Package；没有修改正式地图或内容资产；没有把 Host 接入唯一 Run 生命周期组合槽或玩家可触发路由；没有声明正式游戏流程已经可见或可玩。

## 10. 下一阶段与 GitHub

建议 P27.26 建立 Formation Scatter 的 Run-lifecycle Composition/Route：一个 Run 只持有一个 P27.25 Host 槽，以已提交 Deployment/Handoff 装载 Host，把上层生命周期事件翻译成显式 Publish/Cancel/End Command，并在 Owner 转移时只允许新组合根继续路由。继续禁止第二套 Actor、资源、Deployment 或命令真值；物理玩家输入仍留在后续 F 阶段。

基线提交：`96a01571626b41105c35388794e05bf8b3f02b83`（P27.24）。分支：`agent/0.0.10-p27-25-formation-scatter-world-publication-command-host`。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-25-formation-scatter-world-publication-command-host>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-25-formation-scatter-world-publication-command-host/Docs/Report/Dev.D.UE.0.0.10.P27.25.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-25-formation-scatter-world-publication-command-host/Docs/Log/Dev.D.UE.0.0.10.P27.25.r0_log.md>
