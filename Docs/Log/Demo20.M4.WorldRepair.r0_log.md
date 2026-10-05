# 山门 Demo2.0 原局精确修复 Development Log

## 状态与基线

- `CODED_NATIVE_PASSED_MANUAL_PENDING`；开发起始基线与当时远程为 `4c0025698e499aa06c151dcfa60d3d1cf58644e8`。最终与背包增量联合固定十三源码完成验证，见末节。
- 分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 已读完整 Goal、最新结算 Report／Log、阶段状态、Git、启动脚本、世界／药品／物品恢复与 UI 接线。
- 仓库文档技能用于证据分级与既有体例；不创建云端 Page。
- 保护清单 113 项复核通过；四份 tracked 用户修改和既有 untracked／真实档不动。没有恢复自动任务，没有窗口操作。

## 编码与意图

世界存盘增加 `.head` 最新候选／前驱凭证与 `.replica` 同字节副本。显式修复只采用最新凭证匹配的主记录／副本／临时文件，不用 `.bak`；覆盖前保留损坏原件，失败不发布状态。GameMode 按当前物品 Run 或正常预检重新校验目标；Widget 复用既有卡片与按钮。未增加数量或库存权威。

新增世界原生用例五项、药品组合一项；既有终局用例增加损坏后恢复同一终局。最初八源码 Game-only 阶段没有运行测试；最终原生成功结果见末节，不以测试源代码存在当成功证据。

## 历史八源码候选（不是最终输入）

8 个源码路径，manifest 按路径 ordinal 排序，逐行 `path<TAB>SHA256`，LF 连接、无末尾 LF、UTF-8 SHA-256：
`71C44E86C83BC7D7352A203C93AFD3F726BC28171BE2E0AC74A3306ABD713CCD`。

| 路径（相对于仓库） | SHA-256 |
| --- | --- |
| Source/demo_map/Demo20/ShanmenDemo20Expedition.cpp | E902CB4BC7D2EF1FE2E5DD2216510362517D8581F33F6E54D74C078DFAC1F666 |
| Source/demo_map/Demo20/ShanmenDemo20ExpeditionTests.cpp | E0B113898AA19C1C0A8EB1340F658F29A5FB81A20AE1A55B8955197537261954 |
| Source/demo_map/Demo20/ShanmenDemo20MedicineTests.cpp | A8DF47DE66E5228EAF4B86C332427A474774C2056CE714BC01F3850681B0557D |
| Source/demo_map/Demo20/ShanmenDemo20Widget.cpp | D3CFDB7E4220DD872A090755045F08CC129A034B12D36567EC8A5A823E8D751B |
| Source/demo_map/Demo20/ShanmenDemo20World.h | BBD4B6A3D22704FB08E7F5127D622E029911915318FBA512501FA0EE0DC55B27 |
| Source/demo_map/Demo20/ShanmenDemo20WorldCheckpoint.cpp | AC7139440EBE925E65950D8E8BEFEC10F6BCBC7076F8A1CBCBC703A4A385435A |
| Source/demo_map/Demo20/ShanmenDemo20WorldCheckpoint.h | C98B2EDF38B0CCD99787882BB7D0CAD5192C174EFC5F0BBB7BF5FD504E678BE5 |
| Source/demo_map/Demo20/ShanmenDemo20WorldRecoveryTests.cpp | 30F157E8035E4AEF1CCEA5B19BC25F828D5D65BBE28FF61EA0D4DC6681391B48 |

此 manifest 是最初八源码候选，不是最终已验收证明。最终新增五个背包源码且补零字节边界；两个世界源码 SHA 已改变，不能沿用旧值。

## 历史八源码 Game 构建

使用现有 `Invoke-ShanmenBuild -Target Game`，隐藏进程，不启动产品：

- 入口 `Build.bat demo_map Win64 Development -Project=<本仓库工程> -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=2 -NoUBA`。
- UTC 2026-10-05 16:44:36.538 → 16:45:30.021；native 0、外层进程 0；UBT `Result: Succeeded`、53.16 秒。
- 证据目录 `Saved/FoundationRuns/Demo20.M4.WorldRepair/Build-Game/20261005T164436508Z-dc50b184`；保留 `run-state.json`、`stdout.log`、`stderr.log`。
- stdout SHA-256 `B2F1EE5B6C8861A02349AD238F6590900F0B66F7F6276982B974037280126782`。
- stderr 为空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。
- Game-only 构建前后 `Binaries/Win64/UnrealEditor-demo_map.dll` SHA 均为 `47EF842723E05C5B725CC5076519825C49CE4D9A521E9DDE354B534548A121C4`，没有改写正在全根回归的 Editor 模块。

该历史输入没有构建失败日志；不是删除首次失败。当时 Editor 测试编译尚未执行，Game 宏排除的原生用例不能据此计成功；后续联合候选首次测试失败与最终证明见末节。

## 上一候选全根：八源码编码时的历史进度

原任务 `Demo20.M4.Settlement.FullRoots` 使用 4c00256 已构建输入，exec session 69955，Editor-Cmd PID 29032。日志为：
`Saved/FoundationRuns/Demo20.M4.Settlement.FullRoots/Automation-Shanmen/20261005T160157647Z-4d2c6ff6/UnrealEditor.log`。

在 UTC 16:46:23 附近只读核验：已结束 1133 Success／0 Fail，首根发现 1549 项，尚无原生结束记录，`demo_map` 第二根尚未开始。日志增长中不生成最终 SHA，不宣称全根通过。上一阶段正式映射 226／0 不与本轮测试混算。

该任务不是环境阻塞：日志与原生测试持续进展。继续候选代码／测试／文档；Editor 链接与新原生运行等其结束后执行，不能覆盖正在使用的 DLL 或终止旧证据来取得“成功”。

## 历史接续验证与辅助诊断

一次性本地辅助 `Saved/Automation/Demo20/Complete-WorldRepair.ps1` 已启动，exec session 65972。它只等待旧两根闭合及 UE 进程退出，持续核对 4c00256／八源码 manifest／旧 Editor DLL；之后隐藏编译 Editor、调用既有 `Test-Demo20Grid.ps1` 全六组和 Game 最终构建。任何固定输入变化或验证失败立即停止本辅助，不改旧任务或真实档，不写 Git，不操作窗口。结果仍须下一次读取 native／outer／结束记录与 SHA，辅助启动不等于验证完成。

辅助首次 PowerShell 语法检查 exit 1，原因是条件换行造成 `-or` 解析错误；只连接本地辅助条件行，八源码未改。第二次 parser exit 0。工具结果 e40e99 为原始诊断；`Saved/Automation/Demo20/WorldRepairVerifierParserFirstFailure.log` 是从工具字段重建的诊断记录，不冒充 UE 原始测试日志或构建失败。没有丢弃该失败，也不将它算为游戏源码失败。

## 最终联合候选闭合与待人工

后续玩家背包增量新增五源码，原 session 65972 已由固定路径检查关闭（exit 1），不再验证八源码。联合候选全部十三源码最终输入、构建／原始日志与 SHA 以 [便捷领取 Log](Demo20.M5.QuickTransfer.r0_log.md) 为准。这里保留历史等待记录，但等待已经结束；不继续扩展世界修复。

1. 旧基线 Shanmen 1549／0；demo_map 首次 1329／1、native 0／outer 1，origin 前置环境错误。保持旧 DLL、仅改 canonical 隔离 UserDir 后 1330／0、native／outer 0；不改测试或保护门，首次原件保留。旧成功两根 2879 只证明 4c00256。
2. 联合候选首次 Demo20 90／1，三项背包测试成功，空损坏原件断言失败。Windows 文件 API 拒绝零字节读取；只补允许 Minimum=0 的有界原件读写，空 checkpoint 的普通加载仍拒绝。增加真实空原件、精确体／head、代次及重复断言；第一次日志 SHA `9D1C148DBA0AEE1BF13FDBD9028F4996851A6EBD9A021B03FCBD4A09CAA75BC0` 完整保留。
3. 最终十三路径 manifest `52D79114975DE1D860802937D50152515DAD87F60FAE05571C32755C15632260`，Editor／Game native／outer 0；全六组 235 Success／0 Fail，各队列结束记录闭合。`PASS Changed=13 Rules=1 Required=6 Logs=6`；五项世界、药品组合与终局损坏恢复通过。
4. `git diff --check` 和 113 项保护通过。精确提交十三源码＋八份阶段文档，共 21 路径，不纳入四份 tracked 用户修改／历史 untracked／真实档。提交身份用包含本文的 Git commit 追溯，不在正文自引用提交 SHA。

`git diff --check -- Source/demo_map/Demo20` 通过；仅常见 LF／CRLF 提示，不是差异检查失败。无真实窗口、PIE／Standalone、截图、帧耗时或内存证明。Goal 活动、自动任务保持暂停。
