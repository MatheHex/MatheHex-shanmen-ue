# Dev.D.UE.0.0.10.P27.30.r0 Development Log

## 1. 阶段与基线

- 日期：2026-09-15 UTC。
- 实现基线：P27.29 `58e696483119137cd4ed3b267b615af812cc7f39`。
- 提交父基线：`b3d7c026c513d8b3e03a56e47674569eb06f880d`；其相对前一提交仅修改总体 Report/Log。
- 分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 本轮接续既有五文件修复与验证进程，没有重复开发或重启全根。
- 既有未跟踪用户文件 103 个；交接前记录其路径与原字节 SHA-256，用于提交后保留核对。

## 2. 精确变更

1. `Source/demo_map/demo_mapGameMode.cpp`：Timeline/World 身份预检、成功逻辑结束结果保留、剩余清理函数、重入拒绝、待清理时现有 Tick/激活门、orphan 保留。
2. `Source/demo_map/demo_mapGameMode.h`：私有 optional、私有重入 guard、私有清理函数与两个测试 friend。
3. `Source/demo_map/demo_mapShanmenControlledWeaponWorldLifecycle.cpp`：只有 Destroy 接受后才关闭原有碰撞/表现。
4. `Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp`：增加两个注册测试、三个非零故障场景。
5. `Scripts/ShanmenRegressionMap.json`：M01GameMode 要求整个 ControlledWeaponWorldLifecycle 组。
6. `Docs/Report/Dev.D.UE.0.0.10.P27.30.r0_report.md`。
7. 本 Development Log。

五个实现/映射文件合计 291 行新增、71 行删除。没有引擎修改、网络功能、Windows 系统设置修改或新的数据权威。

## 3. 实施与证据顺序

1. 读取 P27.29 对 World/Timeline/orphan 的三个待审计点，确认旧清理顺序可能丢失已完成前缀和残留 owner。
2. 使用瞬态测试 World 的真实 Actor 销毁拒绝、外来 Timeline 和非空 orphan 构造失败。先增加测试/头文件脚手架，生产 .cpp 仍为旧实现，完成 RedProof 编译。
3. 旧实现跑新测试，0 Success / 2 Fail，保留原始日志；原生退出 0 不用于掩盖 Automation 失败。
4. 最小修复后增量 Editor 编译成功；World 生命周期全组 3/3 通过，物品权威快照无变化。
5. 映射自检 504/504，通过差异检查；五文件 SHA 锁定后启动唯一隐藏串行验证。
6. 01:36:00.128 UTC 验证开始；先 Editor/Game，再五个旧组，最后完整 Shanmen 根。
7. 02:10 UTC heartbeat 仅检查存活进程、增长日志和源码哈希，未重启或改动测试源码；当时 1,009 项只是中间结果。
8. 02:43:43.905 UTC 验证进度记为 COMPLETED。交接轮独立重新读取全部最终组原日志、终止计数、退出码与 SHA，确认 1,418/0 和 123/0。
9. 完成 Report/Log 后，以最终七文件清单运行覆盖门，精确暂存、提交、推送并核对远端。

## 4. 失败与恢复语义

飞剑夹具先从真实物品 Authority 整备并启动 Run，启动原有御器 Host、生成 Actor、Launch 并推进非零时间，保存非空 FlightPresentation。故障不是通过返回固定 false 的假函数替代真实 World 行为。

Destroy 拒绝场景将瞬态 Actor 设为 ROLE_SimulatedProxy，触发 UE GameWorld 的非权威销毁拒绝；恢复其原 role 后，仅一个销毁回调发生，回调内嵌套 Release 被拒绝。Timeline 场景恢复同一夹具的原始 Timeline。两次故障期间均检查准确身份、非零 tick、Actor 和表现；最后 AuthorityBefore/After 相等。

orphan 测试只证明非空 Timeline 没有成功结束证明时的两次保留拒绝，不扩张为“所有 orphan 组合均经测试”。成功 optional 是现有结束结果的同进程保留，不是磁盘 checkpoint，也不是整函数回滚。完整旧 RunReleased 前缀汇总未在恢复后重建，恢复完成另有 RunRetirementCompleted。

## 5. 原始日志索引

以下路径相对于 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`，仅本地 Saved 保留；各原字节 SHA-256 与计数见 Report 第 6 节。

| 标签 | 原日志路径 |
|---|---|
| RedProof | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0/RedProof/20260915T004433357Z-154dd9ea/UnrealEditor.log` |
| WorldRetirementFocused | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0/WorldRetirementFocused/20260915T004731942Z-0df609d5/UnrealEditor.log` |
| LegacyAttributes | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0/LegacyAttributes/20260915T013820889Z-51361fa0/UnrealEditor.log` |
| LegacyEnemySkill | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0/LegacyEnemySkill/20260915T013837133Z-21c07c92/UnrealEditor.log` |
| LegacyV2Ranged | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0/LegacyV2Ranged/20260915T013857582Z-c1d985bc/UnrealEditor.log` |
| LegacyItemArmor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0/LegacyItemArmor/20260915T013913757Z-a148cbfa/UnrealEditor.log` |
| LegacyHotbar | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0/LegacyHotbar/20260915T013934225Z-7386b672/UnrealEditor.log` |
| FullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0/FullRoot/20260915T013951356Z-73132995/UnrealEditor.log` |

最终五个旧组依次为 4、44、22、46、7 个 Success，总计 123，Fail 均为 0；完整根 1,418 Success、0 Fail。各终止句的 performed 数与 Success 数相等，native 0、Fatal/Ensure/Unhandled 0。专项 3 项包含在根组中，不重复累计。

完整根进程 01:39:51.357 至 02:43:43.011 UTC；测试主体 01:40:05.815 至 02:43:40.986 UTC。全根 SHA-256：`F402AF7DE96A5AFE92464D571E1426DBBD22C46E06FE667658E2E3E69F9C724C`。

启动阶段选定测试尚未开始时有 13 条 LogAutomationTest Condition failed 文本，与 P27.29 专项原日志相同。本阶段没有掩盖这些文本，也不作“整个日志零 Error”的错误表述；通过依据是选定测试的结果、终止、原生退出及崩溃指标共同核对。

## 6. 构建与检查器

- Red Editor：`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0.RedProof/BuildEditor/20260915T004139132Z-304cba56/`；native 0，37 actions，135.30s。
- Fixed Editor：`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0.Fixed/BuildEditor/20260915T004624803Z-8343e890/`；native 0，5 actions，14.58s。
- 最终 Editor：`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0.Validation/BuildEditor/20260915T013600213Z-0c518b43/stdout.log`；native 0，Succeeded，0.88s；SHA-256 `5BC9382A911FC01ED745222F3A0173BD65CC28C7AB35A832E5E56A8B43B010B5`。
- 最终 Game：`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.30.r0.Validation/BuildGame/20260915T013601407Z-f6c4fb26/stdout.log`；native 0，Succeeded，139.03s；SHA-256 `FAC9FD874ADD58ECACFBB8F7B2C5F924397AFBB13B50A328A161089CBA749C58`。
- 映射自检：`Saved/Automation/P27.30/P27.30_regression_selftest.log`，PASS 504/504；SHA-256 `507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`。早期调用未独立保存 LASTEXITCODE，不虚构该值。
- 初始五文件覆盖：`Saved/Automation/P27.30/P27.30_regression_coverage.log`，PASS Changed=5 Rules=3 Required=88 Logs=6；SHA-256 `D98D9CF619E38CFA925C3A7F57202AC4743CE9ACF046FA299EA8DC13A9E45778`。

## 7. 锁定源码与执行器

| 路径 | 验证前后原字节 SHA-256 |
|---|---|
| Source/demo_map/demo_mapGameMode.cpp | `5FFFA729BDB4F275F16E5BE90309827BA3BCF71A8F6382400019B9785CD81008` |
| Source/demo_map/demo_mapGameMode.h | `2CC565792302054D1DDE4EDE6DD3A3C37CD4C4403259F4EEA666208640AF3652` |
| Source/demo_map/demo_mapShanmenControlledWeaponWorldLifecycle.cpp | `05ABA685FA375FB6490F2C2BB4713414526EB0927D0374D3F58082099F9435CE` |
| Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp | `A26861CEF545902C6ED49811A03B4A129DEECDF1934AA24E7502AE33B3CD6CFF` |
| Scripts/ShanmenRegressionMap.json | `ABB6F9DFB993C6C72620FC32F5C6D17F45C2EB9EB8BB23B751612FA0511CD3FB` |

执行器为本地 `Saved/Automation/P27.30/run_validation_suite.ps1`，唯一实际 runner PID 34276，全根子进程 PID 33744。原子进度 JSON 与独占锁保留，未删锁盲重跑。启动前探测首次误匹配了自身 probe，因此在实际启动前拒绝；只读确认后排除自身 PID，未产生重叠验证。

现有 tracked-process helper 在 `Start-Process -Wait` 返回后写 run-state，故运行中的子进程可能尚无该文件。轮询时用准确 command line、父子 PID 与增长原日志确认；文件未出现不是测试失败，最终仍强制核验原生退出。这项观察没有扩张为本轮执行器改造。

复跑只可在没有现存验证进程时使用既有 BuildBoth 入口，MaxParallelActions=1、UseUba；无头参数为 -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile，每进程一个明确 RunTests 组、独立绝对日志及队列清空退出条件。不要运行已完成的带锁脚本来覆盖证据。

## 8. 最终范围检查

最终覆盖门针对第 2 节七文件，沿用本阶段六份完成日志：`REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=88 Logs=6`。

- 原日志：`Saved/Automation/P27.30/P27.30_regression_coverage_final_scope.log`。
- SHA-256：`3B1312041B97D6B31C71DF369241D0D25E1EE26286C4BE4B2572DB68F5C894DC`。
- 该 PowerShell 脚本以正常返回和 PASS 标记表示成功，不显式设置 LASTEXITCODE；交接检查包装第一次把未设置值误判为失败。源码确认后重新核对同一完整覆盖日志及哈希，通过依据未改变，未伪造 native 0，也没有重跑或覆盖测试证据。
- 五个实现/映射文件哈希仍与最终验证一致；两份新文档的 5 个本地链接目标存在，差异卫生检查通过。文档不作为新产品测试。
- 提交范围精确限定七文件；既有 103 个未跟踪文件的路径和内容哈希独立核对保留，不纳入提交。

## 9. P/F 边界与下一步

没有实际游戏性、物理输入、UI、资产、地图、Editor UI、PIE、Standalone、产品运行、截图、Smoke、Cook 或 Package。无头瞬态 Actor/World 用于底层证明，不代表正式游戏交互完成。

下一步是冻结前契约/入口/生命周期总审计，按可验证缺口决定是否还需最小修复。不凭本次局部成功宣布整个框架冻结，也不无边界增加恢复层。最终冻结与 F 阶段债务清单需要独立 Report/基线。

- [Report](../Report/Dev.D.UE.0.0.10.P27.30.r0_report.md)
- [总体就绪度报告](../Report/Dev.D.UE.0.0.10.OverallReadiness.r0_report.md)
