# Dev.D.UE.0.0.10.P27.29.r0 Development Log

## 1. 阶段与基线

- 时间：2026-09-14 UTC；父提交 `b1cef4a39a8ccd6dc09821f1045520a3f6894b53`。
- 分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 目标：闭合 GameMode 剑法 owner 拒绝释放后的证据保留；不增加实际游戏性。
- 入场状态：四个本阶段实现/映射文件处于未提交验证状态；此前保存的原始失败、成功专项和验证进程被继续使用，没有重复启动全根。
- 既有未跟踪用户文件：103；交接前记录路径及 SHA-256，用于提交后检查未被修改或纳入提交。
- 参照 P27.28 Report/Log 的 P/F 边界；总体报告提交没有更改产品实现。

## 2. 变更文件

1. `Source/demo_map/demo_mapGameMode.cpp`：先检查逻辑 owner，再结束表现 owner；无效/跨 Run/表现结束失败均保留对应状态并返回，不继续拆共享 Run。
2. `Source/demo_map/demo_mapGameMode.h`：仅测试 friend。
3. `Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationRunControllerTests.cpp`：新增 GameModeReleaseRecovery，两个非空场景。
4. `Scripts/ShanmenRegressionMap.json`：为 M01GameMode 增加完整 SwordRhythm 控制器组。
5. `Docs/Report/Dev.D.UE.0.0.10.P27.29.r0_report.md`。
6. 本 Development Log。

四个实现/映射文件合计 154 行新增、22 行删除；不包含这两份文档。没有添加 runtime 字段、Authority、ID 工厂、Timer、输入绑定或内容资产。

## 3. 实施与验证顺序

1. 底层收尾审计发现旧 GameMode 对 SwordRhythm 拒绝执行 Reset，且继续共享 Run 清理。
2. 先增加测试与测试友元，编译 Editor；生产 GameMode 仍为旧实现时运行新单测，复现真实断言失败。
3. 调整生产释放顺序与拒绝分支，再增量编译 Editor，运行完整 SwordRhythmEffectCuePresentationRunController 组，6/6 通过。
4. 回归映射自检通过；锁定四文件源码哈希，隐藏运行串行验证脚本。
5. Editor 与 Game 构建成功后运行四个旧组，再运行完整 Shanmen.0_0_10 根；脚本同时检查结果计数、原生退出和终止标记。
6. 23:20 UTC heartbeat 检查时全根仍正常进行，只保存续接记录，未重启、改动验证源码或提交部分成功。
7. 23:51:13.560 UTC 验证脚本记为 COMPLETED；23:56 UTC heartbeat 重新读取原始日志、run-state 和源码哈希，确认实际 1,416/0 及 116/0。
8. 对最终六文件清单运行覆盖门，补齐 Report/Log，精确提交后核对远端。文档不作为新的产品测试结果。

## 4. 非零失败与成功证明

复现阶段执行新测试、旧生产二进制，结果 0 Success / 1 Fail。失败包含原回执丢失、待处理 Handoff 与队列不保留、第二次跨 Run 结束意外成功、校正原 owner 后无法按预期结束。原始失败日志没有删除或以最后一次成功覆盖。

夹具同时建立 Current/Foreign 两个有真实剑法历史的 Run，且每个都有非零 observation、视觉/音频待处理命令与队列。两次拒绝后逐项核对身份和数量。校正使用夹具原有的正确 Session/Presentation 快照，而不是随机新 Run 或空状态。成功路径之后再次结束仍返回成功。

复现进程原生退出为 0，run-state 的 SUCCEEDED 因而只代表进程退出；Automation Result={Fail} 决定这次测试失败。后续门禁没有将此文件列为成功证据。预期 Error 只登记两次精确的身份拒绝，未全局屏蔽错误。

## 5. 结果与原始日志

所有相对路径以项目根 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B` 为起点；原始文件保留在本地 Saved 目录，不声称它们已随 GitHub 文档上传。

| 证据标签 | 本地原始日志路径 |
|---|---|
| RedProof | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0/RedProof/20260914T223757986Z-8ad2d9cd/UnrealEditor.log` |
| SwordRhythmController | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0/SwordRhythmController/20260914T224035305Z-fb3662ec/UnrealEditor.log` |
| LegacyAttributes | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0/LegacyAttributes/20260914T224603946Z-4ac52cdb/UnrealEditor.log` |
| LegacyEnemySkill | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0/LegacyEnemySkill/20260914T224620242Z-7664ee54/UnrealEditor.log` |
| LegacyV2Ranged | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0/LegacyV2Ranged/20260914T224640709Z-be61318e/UnrealEditor.log` |
| LegacyItemArmor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0/LegacyItemArmor/20260914T224701182Z-539bc9ea/UnrealEditor.log` |
| FullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0/FullRoot/20260914T224721635Z-6308f466/UnrealEditor.log` |

| 范围 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| 旧实现复现（预期失败） | 0 | 1 | `2CCAF1E6B2D9F2CA5FE0E72968912B8F2C8E8C491A93446F1F5200D4309B7504` |
| SwordRhythm 控制器全组 | 6 | 0 | `011186149D462D1BE0CBA8615F6ACDE0A2D92A8F53E0D061A228E7AA2EA8719B` |
| demo_map.V3.Attributes | 4 | 0 | `E2FE0F981267D58C1082A36F4A34DBA8B375FF65AC1A985F55FB2E457934E1DE` |
| demo_map.EnemySkillFramework | 44 | 0 | `818F2E9ECD336233A3C321153D0EA4BA9E34FEDBD9A49D8F53A0BDE88C326425` |
| demo_map.V2RangedCompatibility | 22 | 0 | `DA05F4C4EE0481597C4AF9A0F6B1E5A6923EF3F99667E992E8CE40DBC58B3FAE` |
| demo_map.ItemUseAndArmor | 46 | 0 | `67A6FCC2FC2DC7DBB3ACDEBE5952192652D856131E670F70C5C7BA68E1D1AAE2` |
| Shanmen.0_0_10 全根 | 1416 | 0 | `87CB5E8499AFF448ACB38C296AFC54341A200311B21BC8623845A0CAE486B9F5` |

所有最终通过组原生退出为 0，终止队列标记各 1，Fatal/Unhandled Exception/Ensure 均为 0。失败复现的退出码也为 0，但 Fail=1，必须单独列为失败证据。

根组首条 Test Started：2026-09-14 22:47:36.060 UTC；末条 Test Completed：23:51:10.156 UTC。进程从 22:47:21.637 UTC 至 23:51:12.715 UTC。不能混淆测试主体与进程总耗时。最终注册测试比上一阶段 1,415 增加 1；6 项专项已包含在 1,416 根组中。

## 6. 构建与检查器

最终 Editor（native 0；up to date；0.87 s）：

- `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0.Validation/BuildEditor/20260914T224323171Z-eeb4c6fd/stdout.log`
- SHA-256：`3652C4DC11CE51091E8B75271BBD725DAC122DD170E8CB9A201D6CC52F945135`。
- 同目录 run-state.json 保存实际 Build.bat 参数、开始/结束 UTC 和退出码。

最终 Game（native 0；35 actions；159.10 s；UBA 156.62 s）：

- `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0.Validation/BuildGame/20260914T224324342Z-57fbde24/stdout.log`
- SHA-256：`9E814B06ACDE94B67C26F76A0015410AB7F123DDD289D672C1497C83EB0D38B2`。

复现编译和修复后增量编译的原始目录分别位于 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0.RedProof/BuildEditor/` 与 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.29.r0.Fixed/BuildEditor/`。最终双构建没有替代或删除这些早期证据。

映射自检：`Saved/Automation/P27.29/P27.29_regression_selftest.log`，PASS 504/504，SHA-256 `507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`。该次同进程脚本调用未独立输出 LASTEXITCODE，不为其虚构原生退出码。

四文件初始覆盖门：`Saved/Automation/P27.29/P27.29_regression_coverage.log`，PASS Changed=4 Rules=2 Required=99 Logs=5，SHA-256 `8AB56B55141BE77D19262C07C9F2AC21B8A89ACBD61582CF6DF561A54C584CFC`。

六文件最终覆盖门：`Saved/Automation/P27.29/P27.29_regression_coverage_final_scope.log`，PASS Changed=6 Rules=2 Required=99 Logs=5，SHA-256 `5BA9A4C640FC5B81D34207A4CC4B1E0C0FAB2AC777005FA4C489AEF7BAC33EEC`。99 是当前两条路径规则的唯一必跑组集合；文档不新增产品组。

## 7. 验证源码身份

以下哈希为验证时及文档交接前重新核验的工作区原始字节，包含当时换行；Git 检出换行策略变化时不应直接比较不同换行的字节哈希。

| 路径 | SHA-256 |
|---|---|
| `Source/demo_map/demo_mapGameMode.cpp` | `E041805BE62B138C8956E08E4D89AC55DA9AD8711A90430CE681A7153DC40467` |
| `Source/demo_map/demo_mapGameMode.h` | `BB2B0962678069F7628F22735016DB13FA4C8E97B2B0E3100B20E53FA2678119` |
| `Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationRunControllerTests.cpp` | `8294755EEA6E1514740C041C79DBD1EF6474E83EC1E9CE1632F5E30C7D228194` |
| `Scripts/ShanmenRegressionMap.json` | `EA351651C9B7DE738BB384E9D50CE625EA89D567FDA0D4FE63C4FD861B5B584F` |

串行执行入口：`Saved/Automation/P27.29/run_validation_suite.ps1`，进程 32180，使用独占锁和原子进度 JSON；只运行一次。每次子进程后和最终覆盖后复核四文件哈希。

可复跑操作：使用 `Scripts/Invoke-Shanmen.ps1 -Action BuildBoth -MaxParallelActions 1 -UseUba` 编译；用项目解析出的 UE 5.8 UnrealEditor-Cmd 逐组执行 `-Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile`，每进程一个 `Automation RunTests <group>`，并指定独立绝对日志路径和队列清空退出条件。不可把整个 Saved 重启脚本的锁文件删除后盲跑。

## 8. 收尾检查与后续债务

- 四文件哈希一致，映射 JSON 可解析，差异卫生检查通过；最终仅暂存第 2 节列出的六文件。
- Report/Log 使用普通文档交接，不引入 CSEMI 信封、附件冒称或额外发送确认。
- 当前修复并非整个 ReleaseCombatProductRun 的原子回滚；更早成功清理前缀未回滚。
- ControlledWeapon World 清理拒绝、Timeline 拒绝、inactive-Coordinator orphan Reset 仍是待审计代码点。必须先证明可达失败及恢复语义，不能只逐处删除 Reset。
- 以上候选未在 P27.29 修改；当前没有足够依据冻结所有底层框架或暂停自动化。

## 9. P/F 边界与交接

本阶段只允许底层源码、无头自动化及 Editor/Game 编译。未接物理输入、未更改正式地图/内容、未开发 UI 或玩法数值/手感/敌人/关卡，未运行 Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook、Package。

- [Report](../Report/Dev.D.UE.0.0.10.P27.29.r0_report.md)
- [分支](https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-28-formation-scatter-gamemode-composition)

下一阶段先对结束清理剩余分支进行有界审计，不增加玩法；若没有新的真实结构缺口，再进入最终冻结和完整基线验证。
