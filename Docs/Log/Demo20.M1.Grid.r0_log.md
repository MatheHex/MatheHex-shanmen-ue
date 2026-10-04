# 山门 Demo2.0 M1：二维格子事务基础 Development Log

日期：2026-10-04。基础提交：`714003d0dcd66aaea390856bf79f36a2ff327c4c`。本轮是 M1 内部基础增量，完整探索 Goal 保持活动。

## 1. 实现记录

1. 完整读取用户 Goal 与当前 Demo20/物品权威/持久化，确认二维占格缺口。完整 Goal 保存为 Docs/Prompt/Demo20.Expedition.goal.md（242 行，与附件逐行内容无差异），当前基线覆盖历史切片的范围限制。
2. 新增 ShanmenItemGrid 几何/策略，沿用 ItemInstance/Container 数量、Owner、作用域、唯一锚点；没有第二份库存。
3. 新增 Repository EditGrid：CAS、确定性请求指纹、移动/旋转、Split、部分/全量 Merge、审计墓碑、候选验证后发布。拒绝无变更。
4. 接入既有 AuthorityService 原子持久事务及 GameInstance Subsystem 端口；保存不确定/失败仍走既有恢复或回滚语义。
5. 快照 Grid 持久化进入 schema 5；保持 schema 1–4 的原摘要校验及兼容规范化。schema-4 初始摘要允许精确原来源匹配，不重导入库存。
6. 将 Grid 回执接入既有不变量检查；几何验证改为有界临时占格位集，不在大历史上逐物品两两扫描。
7. 新增 7 项 Grid 测试（非零 30 单位材料库存），并更新旧 schema 1–3 夹具的历史字段排除。未弱化旧期望值。
8. 新增文件映射回归脚本，以实际改动路径选择既有规则并提交原覆盖门；测试根、回归规则及通过条件未放宽。

## 2. 首次失败与修正

- attempt-001：新回执误访问不存在的 RunId/OwnerId 字段。确认源码编译错误后停止仍在编译其他文件的本轮 UBT 子树；记录的原生退出是 1，不伪记为标准源码退出 6。
- attempt-002：新测试将 EAutomationTestFlags 保存为 uint32，构造不接受；原生 6。
- attempt-003：测试全局常量 A/B 遮蔽 UE 模板参数；改为明确的 MaterialStackAId/MaterialStackBId，原生 6。
- attempt-004：沙箱内 dotnet 在源码编译前异常退出 -532462766。stdout 没有源码编译诊断，stderr 为空；没有崩溃堆栈，不声称已确定根因。批准执行范围后 attempt-005 两目标通过。
- Test-Demo20Grid.ps1 首次为 PowerShell 多行 if 表达式解析错误，外层 1，UE 未启动；拆出完成记录布尔值，解析检查通过。
- 首轮 UE 模块加载等待 Build.bat 文件锁；进程树确认 ValidatePlatforms 子进程在等待同时进行的 Game 编译。待编译释放后正常进入测试。
- 首轮 Items：117 Success / 1 Fail，队列 118，原生 0，外层验证 1。失败在新 schema-4 测试代数预期：实际文件已是 schema 5 / generation 2，数量 12,18,1。规范化需要从原创建代数 +1；修正测试并增加 bDiskStateChanged 与精确 Authority 比较，未改生产保存契约。
- attempt-006：最终候选 Editor/Game 原生 0。重新运行全部三组，最终 171/0，外层 0。

## 3. 构建原件与 SHA-256

以下路径相对于工程根 C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B。对应 .result.json 与 stderr.log 亦本地保留；表内为 stdout 原件的 SHA-256。

| 尝试 / 目标 / 原生退出 | 本地 stdout | SHA-256 |
| --- | --- | --- |
| attempt-001 / demo_mapEditor / 1 | `Saved/Automation/Demo20.M1.Grid/Build/attempt-001/demo_mapEditor-Win64-Development.stdout.log` | `B36ABC10C2BF46AEE5DB0CEC39B1BC19DBA687AAD677FE3C4844848FF81B1189` |
| attempt-002 / demo_mapEditor / 6 | `Saved/Automation/Demo20.M1.Grid/Build/attempt-002/demo_mapEditor-Win64-Development.stdout.log` | `3FD8D91AEBF9126688CD669994916F3751D36B34782859696943ADA58F6D14A3` |
| attempt-003 / demo_mapEditor / 6 | `Saved/Automation/Demo20.M1.Grid/Build/attempt-003/demo_mapEditor-Win64-Development.stdout.log` | `DBC80B4828AFA43464D9E0B6D43CBF7782A6913354AEA0B2464E7176E2D1BD4E` |
| attempt-004 / demo_mapEditor / -532462766 | `Saved/Automation/Demo20.M1.Grid/Build/attempt-004/demo_mapEditor-Win64-Development.stdout.log` | `241D79AE6954131BE4FA887E6CF8C3302CEE8F7255F736F355F5718BE7D90883` |
| attempt-005 / demo_map / 0 | `Saved/Automation/Demo20.M1.Grid/Build/attempt-005/demo_map-Win64-Development.stdout.log` | `ACDB426291A19176682F06BDB112ED5680752F5E4883383150AA7A3915945FAF` |
| attempt-005 / demo_mapEditor / 0 | `Saved/Automation/Demo20.M1.Grid/Build/attempt-005/demo_mapEditor-Win64-Development.stdout.log` | `29EAA40A093609BB14AAC8AAB71A89AADB6A4CDD63AA0DFA32A02A4A3CD0B4ED` |
| attempt-006 / demo_map / 0 | `Saved/Automation/Demo20.M1.Grid/Build/attempt-006/demo_map-Win64-Development.stdout.log` | `9E25EBF01B5F8F5CFE476A2629F0C599802E832795E6E63EA816EB2CFF8A456E` |
| attempt-006 / demo_mapEditor / 0 | `Saved/Automation/Demo20.M1.Grid/Build/attempt-006/demo_mapEditor-Win64-Development.stdout.log` | `5ABF26C22CDE1E33C5084FDB87D343B90482821D050E5C1611BA68335BDEA16C` |

## 4. 自动化原件与 SHA-256

第一次 Items 失败原件：`Saved/FoundationRuns/Demo20.M1.Grid/Automation-Shanmen.0_0_10.Items/20261004T162050247Z-91c322cd/UnrealEditor.log`，SHA-256 `E32DCB27257B31019DE3DE2ECA3DA0EF1094F4F86DB302CD6F3EABCDAE69305F`。117/1、队列 118、原生 0；不能把 run-state 的进程 SUCCEEDED 当作测试通过。

最终三组均有 Automation Test Queue Empty 记录，run-state 原生退出 0；回归脚本外层退出 0：

| 最终组 / Success-Fail / 原生退出 | 本地 UnrealEditor.log | SHA-256 |
| --- | --- | --- |
| demo_map.ItemUseAndArmor，46/0，原生 0 | `Saved/FoundationRuns/Demo20.M1.Grid.Final/Automation-demo_map.ItemUseAndArmor/20261004T162358024Z-fcb7f1fd/UnrealEditor.log` | `7C741B0160CF1CEC2CE4FC7698230BD7F500F0C3128AE872697D480D1C88F16A` |
| demo_map.P4.Hotbar，7/0，原生 0 | `Saved/FoundationRuns/Demo20.M1.Grid.Final/Automation-demo_map.P4.Hotbar/20261004T162443662Z-c40d0769/UnrealEditor.log` | `6E7BD972F5814558284775364AAF3F49C18BCA046F45B4FDC4F48D8FB9272FDA` |
| Shanmen.0_0_10.Items，118/0，原生 0 | `Saved/FoundationRuns/Demo20.M1.Grid.Final/Automation-Shanmen.0_0_10.Items/20261004T162524129Z-23836498/UnrealEditor.log` | `A999D35EF76AF8C8C0413F0EF1DD3A5AD0E88B6355276B867A057F8CC386A7B6` |

新增 Grid 测试 7 项全部 Success：GeometryAndPreview、MoveRotationReplay、SplitPartialAndFullMerge、SecureEquipmentAndScope、ReservationsAndLegacyBoundary、DurableReopenRollback、Schema4NonzeroMigration。

## 5. 覆盖与保护

- 改动覆盖：`PASS Changed=15 Rules=2 Required=3 Logs=3`。ShanmenItems 路径要求 Items，Subsystem 路径同时要求旧 ItemUseAndArmor 与 P4.Hotbar。
- 原映射自测：555/555；本轮终端输出观察到原生 0，没有改写规则。
- git diff --check：通过。CRLF 提示是行尾提醒，不是差异检查失败。
- 新网格生产文件无 demo_map/UWorld/AActor/ApplyDamage/RNG 引用；ShanmenItems.Build.cs 无 Engine 依赖。全模块字符串扫描有一处已有注释提及 demo_map，不把注释伪报成代码依赖。
- 113 项已有文件/存档哈希核验不变；用户历史未跟踪文档和原有未提交总体报告/索引/边界修改不纳入提交。
- 原始 Saved 日志留本地；GitHub 提交代码、报告与索引，不宣称上传了全部原件。

## 6. 实际验收与下一步

本轮未启动可见 UE/PIE/Standalone；没有背包拖放画面或真实键鼠证据，也没有双分辨率、运行帧耗时、内存或多轮释放数据。无头事务和编译不代替这些验收。

M1 仍需隔离原生新档、10–15 项目录、一次性测试货币、仓库/装备/背包 UI、两种储物容量与缩容保护、基础补给和正式出发。下一增量优先产品接线；M2–M5 的随机搜集、治疗、终局与恢复全部保留，完整 Goal 未缩减。
