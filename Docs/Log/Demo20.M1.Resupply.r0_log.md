# 山门 Demo2.0 M1 有限基础补给 Development Log

日期：2026-10-04。基础提交：`f9b0df1bd6535ad7ad3834abb7da58ff65f75bd0`。本轮为 M1 补给增量；正式 Run 出发未接通，完整探索 Goal 保持活动。结论与边界见 [Report](../Report/Demo20.M1.Resupply.r0_report.md)。

## 1. 开工与实现

读取完整用户 Goal、当前分支／Git 状态、Preparation Report/Log、Demo20 和 Items 的来源／网格／持久服务／生命周期端口。保护清单 113 项核验不变；不恢复旧自动化，不修改已有四份 tracked 用户文档或 104 项历史 untracked 文件。

新增 BasicSupply 固定政策请求与 Repository 事务，经既有 Subsystem／AuthorityService 原子存盘。死亡、Owner、作用域、内容、CAS、待预留和后续 Run 检查先行；全 Owner 替代库存防刷；最多四行，一次死亡一次非空发放；确定性申请与物品 ID 保证重试／重启不再发。

FinalizePreparedRun 同候选协调已丢失储物装备的容量。快照加载验证补给关联与身份，沿用既有回执和 schema 6，无新次数文件。新增七项隔离测试；整备按钮读取固定目录申请、仅确认持久结果后报告成功。

## 2. 首次失败原件

首次构建 `Saved/Automation/Demo20.M1.Resupply/Build/attempt-001/demo_mapEditor-Win64-Development.stdout.log`，SHA-256 `F832F895E6A1FD854DFE3ACA254CDC2E7244850282DE2FA5FC5FBEEE081DF93A`。原生 6，外层 1，Game 未运行；C3861 Refresh 未声明，改为已有 RefreshProjection。结果 JSON SHA-256 `612D5D4EF1D03B14078BD66ACBA76CDABB80D3DBBD7FD5408BA0705D50287FDF`。

首次自动化 `Saved/FoundationRuns/Demo20.M1.Resupply.Initial/Automation-Shanmen.Demo20/20261004T183942238Z-e2bb4d33/UnrealEditor.log`，SHA-256 `0F8C9E10BBF3EAD7611DD1AB8B8537A4ABC3CB83B5B7B8F12E886C984A4D6255`。

13 Success／1 Fail，队列 14，UE 原生 0、验证外层 1。SupplyRejectsNonDeathAndTamper 期望 GridPolicyViolation，生产先按 immutable MaxStack 拒绝为 InvalidRequest；只修正精确错误断言，不放宽失败关闭，不取消完整快照不变和合法重试检查。初次回归与 Game 构建并行导致 UE 启动等待，最终回归改在构建完成后运行。

run-state.json 的 SUCCEEDED 只代表启动器拿到原生 0，不是测试断言通过。首次失败原件和 stderr／JSON 全部保留，不将外层失败解释为成功。

## 3. 构建证据

执行 Scripts/RunF0DevelopmentBuilds.ps1，TaskId Demo20.M1.Resupply，AttemptId attempt-001／002／003。路径相对于工程根 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`，下表文件前缀 `Saved/Automation/Demo20.M1.Resupply/Build/`。哈希对应 stdout.log；同目录 .result.json 和 stderr.log 保留。

| 尝试／目标 | 原生退出 | stdout 路径 | SHA-256 |
| --- | --- | --- | --- |
| 001 Editor 首次源码失败 | 6 | `attempt-001/demo_mapEditor-Win64-Development.stdout.log` | `F832F895E6A1FD854DFE3ACA254CDC2E7244850282DE2FA5FC5FBEEE081DF93A` |
| 002 Editor | 0 | `attempt-002/demo_mapEditor-Win64-Development.stdout.log` | `759F39A9CAAEAEA52800B4190EF37DB821293405079A38A78469B3B3C1712745` |
| 002 Game | 0 | `attempt-002/demo_map-Win64-Development.stdout.log` | `8D3851D5682AD9E121AD77498743D4AF453596B3F9350C89311FEC30BD896782` |
| 003 最终 Editor | 0 | `attempt-003/demo_mapEditor-Win64-Development.stdout.log` | `A16980AA75F1BEFED88AE61A5D8AD51F521744DADDE10E0699A04A51D70B2EF3` |
| 003 最终 Game | 0 | `attempt-003/demo_map-Win64-Development.stdout.log` | `D954763494191BA17100501830DF92A0CD43A4575D5A35D3B809D21E7BB13FE2` |

最终 Editor：18:48:52.291Z → 18:48:58.774Z，6.483 秒。Game：18:48:58.793Z → 18:49:11.313Z，12.520 秒。双目标原生 0、外层 0。最终 JSON 哈希：Editor `242BAAFF604D5DA0695B016BA1DD634BEE05B24DC780AD39C489E4A9BD57CD4D`；Game `76CF5E3C9A1155D869126E628DE67861780CA5357830E67CC825FF77E66E5E8B`。两份 stderr 均为空。

attempt-002 Game 472.532 秒包括全量重编，不拿此时长当游戏运行性能。最终代码候选包含第七项 Subsystem 测试；attempt-002 的证据不替代最终候选。

## 4. 最终改动驱动回归

执行 `Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M1.Resupply.Final`，外层退出 0。209 Success／0 Fail，八组原生退出均为 0，各自有 Automation Test Queue Empty；逐项复核原件计数、队列、run-state.json 原生码与 SHA-256。

路径前缀 `Saved/FoundationRuns/Demo20.M1.Resupply.Final/`，每目录同时保留 run-state.json。

| 组／Success-Fail／原生退出 | UnrealEditor.log 路径 | SHA-256 |
| --- | --- | --- |
| Demo20／15-0／0 | `Automation-Shanmen.Demo20/20261004T185006586Z-9ad4ecfe/UnrealEditor.log` | `9FBE8F8588DAF9FF201E50D95A611756C5C86310F145C4D3FDE3C95B8B3E90BC` |
| Items／119-0／0 | `Automation-Shanmen.0_0_10.Items/20261004T185042179Z-54b19a1f/UnrealEditor.log` | `98D63D2CFC1EBEA9C7326D1A975F173447D5434568810598282F759452CB68FE` |
| ItemUseAndArmor／46-0／0 | `Automation-demo_map.ItemUseAndArmor/20261004T185134107Z-db2f46ff/UnrealEditor.log` | `1A89EB54D7C1E7900F1A9574E7656052C5A3E93DDE61DA56A2E47863777D2B96` |
| Hotbar／7-0／0 | `Automation-demo_map.P4.Hotbar/20261004T185209621Z-10b8e4a5/UnrealEditor.log` | `6BA984F19F9492E77DD81CB6B1D25276A0F524EE2033B479F93AEF396B9AE44A` |
| CombatCore／9-0／0 | `Automation-Shanmen.0_0_10.CombatCore/20261004T185245081Z-08b286cc/UnrealEditor.log` | `E296822B47120F69FC9D2BD84EF31F80DE603EA3C778649BE6702FC263353703` |
| BasicSword／4-0／0 | `Automation-Shanmen.0_0_10.CombatRuntime.BasicSword/20261004T185320540Z-2c863c7a/UnrealEditor.log` | `F719C343B8F1D4AF0373864A94D86D384B34CC6AF59E6BE7D0E109227E2CE30A` |
| VitalityAuthority／5-0／0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityAuthority/20261004T185351814Z-7878dd6d/UnrealEditor.log` | `00970AA4917167D1D764DBF0DAFCA62FE90C72B17566E0521FCAF64B9A9AFFD9` |
| VitalityLedger／4-0／0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityLedger/20261004T185423210Z-25388622/UnrealEditor.log` | `4F89A029E63444B113B5A8CECF5EEE6A6AFEC08B141F5BE5BE1E4AB8B919B6C3` |

覆盖门 `PASS Changed=16 Rules=3 Required=8 Logs=8`。七项新测试在 Shanmen.Demo20.Preparation 下，已有八项 Demo20 测试仍通过。旧 Items／物品使用／Hotbar／战斗组按改动路径执行，没有按主题省略。未宣称完整新旧测试根验收。

映射自测 `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1` 557/557、原生 0；原件 `Saved/Automation/Demo20.M1.Resupply/RegressionMapSelfTest.log`，SHA-256 `D7FD012D6300D759C1E67EC3096F14C5C6F467897CF4533C59EBA9DC84AE1F7D`。

## 5. 实际界面与隔离存档

使用正常启动器 `Scripts/Start-Demo20.ps1 -Action Play -Width 1280 -Height 720 -ProfileName SupplyAcceptance_20261004`，随后同档改为 Width 1920／Height 1080 重启。没有修改文件构造死亡或通过控制台发物。

| Play 目录后缀 | 操作与观测 | UnrealEditor.log SHA-256 |
| --- | --- | --- |
| `20261004T185639234Z-63b9a7b2` | 720p 新档；打开整备后三次点击补给，明确拒绝；Tab 关闭／重开；无存盘变化 | `B27779F736E58E0452F06A74EFADB246A7FC59F7FBA72DF4EB370807E4E3D8C6` |
| `20261004T185927956Z-81a6fd25` | 1080p 同档重启第 1 代；无初始化重发；补给拒绝可见；Esc 返回 | `D1B3508F30AF28779F7EC04F48512684CC2518BB9DFA1EB9871FBF5C7076C460` |

前缀 `Saved/FoundationRuns/Demo20.S01/Play/`。两实例正常 Alt+F4 关闭，有 LogExit: Exiting。启动器为非阻塞启动，run-state.json 的原生退出为空，不能写成实际 UI 进程退出 0。收尾时无剩余 UnrealEditor／UnrealEditor-Cmd 测试进程。

物品文档实际路径 `Saved/Demo20/User/Saved/Demo20/SupplyAcceptance_20261004/ShanmenItems/Authority/165C5C9C689E2502A733AFF4489C62CC.json`。首次初始化后、重复点击后、面板重开后及 1080p 重启／拒绝后 SHA-256 均为 `0F5D500A2C0C8B84327B075049E7F72E163441B5D34B9307D4C47B69CF717FB1`；SaveGeneration=1、SchemaVersion=6、Currency.Test quantity=1,000,000。拒绝路径没有伪报领取或改代数。

原始窗口截图保存在 Docs/Demo20/Evidence/M1.Resupply，未合成、裁切或修改。视口日志分别 Pixels=1280x720 WidgetScale=0.666／Pixels=1920x1080 WidgetScale=1.000；桌面 DPI 截图尺寸分别 1922×1128／2882×1670。

| 截图 | SHA-256 |
| --- | --- |
| [no-death-1280x720.png](../Demo20/Evidence/M1.Resupply/no-death-1280x720.png) | `730BBA08258E826C07ECBF8EF77C3888CC6DB378BDE4E5405A189047165E12FD` |
| [no-death-1920x1080.png](../Demo20/Evidence/M1.Resupply/no-death-1920x1080.png) | `1AA0865333C97B69E0A51760D2F971FA364C6D652D71008464D077D4FD3C8169` |

## 6. 边界与交付检查

新增补给核心 header／cpp 未扫描到 demo_map、UWorld、AActor、ApplyDamage、FRandomStream 或 FMath::Rand；ShanmenItems.Build.cs 仍无 Engine 依赖。保持 schema 6，不改旧 Profile 存档格式，不新增库存／钱包后端。候选快照复制和历史检查仍有成本，未声称运行内存优化完成。

113 项保护核验不变；本轮精确名单为 16 个 Source 文件、4 份 Demo20 说明、Report／Log 和两张原生截图。提交前核验 diff --check、文档链接、证据哈希和暂存名单；Saved 原始证据留本地，不夹带用户文件，不用 git add -A。

待验收：正式键鼠死亡／补给／再出发、PIE、真实存盘失败与恢复反馈、部分堆叠界面、三分区探索、治疗、随机来源搜集、原局恢复、帧耗时与多轮对象释放。下一步正式 Run 出发和携带筛选；完整 Goal 不标完成，旧自动任务继续暂停。
