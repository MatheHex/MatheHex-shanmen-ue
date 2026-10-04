# 山门 Demo2.0 M1 持久整备与储物装备 Development Log

日期：2026-10-04。基础提交：`8792c2806cb12ca8b176047daf591992a1dbfa4c`。完整探索 Goal 保持活动；本轮只完成持久整备增量，M1 尚缺基础补给和正式出发。

## 1. 实现顺序

1. 读取完整用户 Goal、当前分支／工作区、Grid Report/Log、Demo20 与 Items 持久契约；核验 113 项保护文件／真实存档。
2. 原生新档使用显式 NativeProfile 来源与既有原子发布协议。创建、坏档拒绝、来源隔离、重开不补发通过同一 AuthorityService／Subsystem；不读 Code A／Code B，不新建钱包后端。
3. 建立 13 项固定目录与 deterministic Owner、整备作用域、容器／初始物品身份；默认一次性测试灵石 1,000,000，启动参数仅用于首次创建。
4. Grid.StorageDefinitions 保存两种普通背包和一种安全格规格；装备命令原子替换旧装备并协调布局，保持 X/Y、数量和身份，缩容失败整体拒绝。
5. schema 6 显式要求四字段 Grid；schema 5 原 wire 摘要验证后补空 StorageDefinitions；新增非零 N−1 兼容测试，旧 schema 1–4 校验保留。
6. 新整备页只读快照，真实拖放／旋转／拆分／合并／便捷转移／装备提交既有 durable port；失败提示不掩饰未保存。
7. 增加五项 Preparation 测试、Items 迁移测试、Demo20 → Items 映射及两个负向映射断言。最终映射自测 557/557，原生 0。
8. 实际 Standalone 验收发现 UI 焦点和文案问题后修正；attempt-006 最终代码候选固定，再运行双分辨率产品与八组改动驱动回归。

## 2. 失败记录

首次 Items 原件：`Saved/FoundationRuns/Demo20.M1.Preparation.First/Automation-Shanmen.0_0_10.Items/20261004T172939208Z-f24a9799/UnrealEditor.log`。SHA-256 `796262539B5C33A576DCF85F1D6BB8A229531C6992C01169D7FA8502505CACA1`。

结果 115 Success／4 Fail，队列 119，UE 原生 0，验证脚本外层 1；失败为 DurableReopenRollback、MoveRotationReplay、SecureEquipmentAndScope、SplitPartialAndFullMerge。原因是影子 Repository 的候选 State 已赋值但 bInitialized 为 false，CaptureSnapshot 为空；两处改为已初始化。保留既有断言，不通过改预期吞掉旧回归。修正后 Recheck 八组 202/0，随后最终 UI 候选再次全跑。

attempt-004 Editor 原生 6、外层 1，Game 未运行：viewport 诊断误放在 Character.Tick，LastViewportPixels 成员属于 GameMode，C2065 未声明。改至 GameMode.Tick，仅尺寸变化时记录，不每帧写日志；attempt-005 和 006 双目标通过。

首次可见界面中 Tab 没有关背包，Esc 可以返回；修正活动页面焦点与 NativeOnPreviewKeyDown 对输入注册表背包键／Esc 的处理。修正后真实 Tab 开关通过。入口换行、假生命 0/100、旋转尺寸描述与笼统非法放置文案一并修正并实际观察。此前用户停止 Computer Use 的那轮没有冒充验收，本轮在新授权后继续真实操作。

## 3. 构建原件与 SHA-256

路径相对于工程根 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`。构建目录前缀为 `Saved/Automation/Demo20.M1.Preparation/Build/`；每项 .result.json 与 stderr.log 亦保留。表内哈希为对应 stdout.log，不是外层终端输出。

| 尝试／目标 | 原生退出 | stdout 文件 | SHA-256 |
| --- | --- | --- | --- |
| 001 Editor | 0 | `attempt-001/demo_mapEditor-Win64-Development.stdout.log` | `7E7F5BFCCA81292553F9301DB51750C4D77AB55DD1E3EA02F9035EAED02C3C51` |
| 001 Game | 0 | `attempt-001/demo_map-Win64-Development.stdout.log` | `6FE18132C3500F9159153ABFE2ECBFD11116FDD2D99B7B38F9A43A69DBDDE9E0` |
| 002 Editor | 0 | `attempt-002/demo_mapEditor-Win64-Development.stdout.log` | `62187098D7446975E073BFD87E1D4006B624B66BC1F8BB516A65E356672225EA` |
| 002 Game | 0 | `attempt-002/demo_map-Win64-Development.stdout.log` | `CCA6D0D5FBDD3AB9847893EA57AADF4A61AA2672810AA86ADC88F26A26DA4E4D` |
| 003 Editor | 0 | `attempt-003/demo_mapEditor-Win64-Development.stdout.log` | `200FABB1B1E3842A4B85F12B5D63A4E9554E4038ABF1DF100E953D84E4F34DB4` |
| 003 Game | 0 | `attempt-003/demo_map-Win64-Development.stdout.log` | `556429F84CECD0F7987E540DCFA22BBC4A6DD93C0E46224CF74CA81532AF5907` |
| 004 Editor 首次源码失败 | 6 | `attempt-004/demo_mapEditor-Win64-Development.stdout.log` | `3FFB0B0A7539E5FEF7069706200B3C04166B93619F13155CBC9BC7F91DD41932` |
| 005 Editor | 0 | `attempt-005/demo_mapEditor-Win64-Development.stdout.log` | `1758FA74907604BAB9FFC31DD1B67EB27BD428C87711FB72E7B7E15270CBC6DE` |
| 005 Game | 0 | `attempt-005/demo_map-Win64-Development.stdout.log` | `2BC4F98312DCF90474D9F3077152E170B0BF7434D7920D36CE18D95428129A82` |
| 006 最终 Editor | 0 | `attempt-006/demo_mapEditor-Win64-Development.stdout.log` | `83D423F0CE0A0A04DE220F2755E88059E7C2C9532B555E81C6DD447A3B7E19E9` |
| 006 最终 Game | 0 | `attempt-006/demo_map-Win64-Development.stdout.log` | `631E7AC77EA803DC459AD418B15789572186D3060DD9D08A4F2B93E85AFBA31C` |

最终 Editor：17:56:55.834Z → 17:57:03.718Z，7.883 秒；Game：17:57:03.735Z → 17:57:17.786Z，14.051 秒。JSON 字段名 started_utc 的实际字符串带 -04:00 偏移，上述时间已转换为 UTC，没有把本地小时当作 UTC。

## 4. 最终自动化原件

执行 `Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M1.Preparation.Final`，外层退出 0；202 Success／0 Fail，各组原生 0、Automation Test Queue Empty 记录齐全。以下路径相对于 `Saved/FoundationRuns/Demo20.M1.Preparation.Final/`，每目录含 run-state.json 原生退出记录。

| 组／Success-Fail／原生退出 | UnrealEditor.log 路径 | SHA-256 |
| --- | --- | --- |
| Demo20／8-0／0 | `Automation-Shanmen.Demo20/20261004T180615562Z-c99e1bde/UnrealEditor.log` | `6EB936A44F4DB68C5D54C64423E694D0017834EB4620B9481E691B8A1EB4B4BB` |
| Items／119-0／0 | `Automation-Shanmen.0_0_10.Items/20261004T180651114Z-5aff8436/UnrealEditor.log` | `B83162BF76C3EDB873E809F5749DD045B1C1D1B04BFAB0D70E3C8BBA53AB8472` |
| ItemUseAndArmor／46-0／0 | `Automation-demo_map.ItemUseAndArmor/20261004T180746669Z-8f01dd06/UnrealEditor.log` | `BCAF64CD9EFD7326C6F578113F5C8B5946320F82326989889F973FD33F3904D4` |
| Hotbar／7-0／0 | `Automation-demo_map.P4.Hotbar/20261004T180822286Z-8f47702a/UnrealEditor.log` | `E6F626F3D77F9018DB98CD04E0ED27139F4E8BCD9F42CBD8B940D5A41068098F` |
| CombatCore／9-0／0 | `Automation-Shanmen.0_0_10.CombatCore/20261004T180857784Z-d2b9b4c4/UnrealEditor.log` | `3CB479D1D287759F7B574A7D72BCFBC2675AAAECA9993211FC17ABB759ECF12A` |
| BasicSword／4-0／0 | `Automation-Shanmen.0_0_10.CombatRuntime.BasicSword/20261004T180933277Z-5dc01f13/UnrealEditor.log` | `1E274122D0DA65CD01536FECC2A0EDC5668E6C8C44E077BE659DFF71DFB00F9F` |
| VitalityAuthority／5-0／0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityAuthority/20261004T181004599Z-be0a4ac4/UnrealEditor.log` | `38D7D1F44BC46A7581B70991255B91D2A20032EA1A3C5BBE64AC7BA07DDC5915` |
| VitalityLedger／4-0／0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityLedger/20261004T181036068Z-93b14656/UnrealEditor.log` | `28B1C9E7B161BACF05B54D73F6B15F2977D4C43B734A0E037D15065A97DB3E65` |

覆盖门 `PASS Changed=20 Rules=3 Required=8 Logs=8`。改动生产路径都由三个既有规则覆盖，完整运行其八组并通过覆盖门；没有按战斗／UI 主题省掉旧物品回归。

先前修复后的 Recheck 八组 202/0 是 attempt-003 证据，不替代最终 attempt-006。其日志保留在 `Saved/FoundationRuns/Demo20.M1.Preparation.Recheck/`。

映射自测原件 `Saved/Automation/Demo20.M1.Preparation/regression-map-final-selftest.log`，SHA-256 `D7FD012D6300D759C1E67EC3096F14C5C6F467897CF4533C59EBA9DC84AE1F7D`，557/557，外层原生 0。覆盖规则没有因本轮新产品主题删掉旧 Items／ItemUse／Hotbar。

## 5. 实际 Standalone 操作与截图

隔离档名 `PrepAcceptance_20261004`，物品实际目录 `Saved/Demo20/User/Saved/Demo20/PrepAcceptance_20261004/ShanmenItems/Authority`。只通过正常界面操作，没有控制台发物或手改存档。

| Play 证据目录后缀 | 操作与观测 | UnrealEditor.log SHA-256 |
| --- | --- | --- |
| `20261004T173932733Z-5f09e9d4` | 新档第 1 代；移动／旋转／12→6+6→12；8×5 大包，灵草放大包专属位置；换小包拒绝仍第 7 代；发现 Tab 焦点缺陷 | `7575969917254E486E70FEF1F3EF1096581BF6509C6781EF83C324E760171D5D` |
| `20261004T175353810Z-a2f75acd` | attempt-005，720p 第 7 代重开；测试配置 999,999,999 未重发，钱包 1,000,000；整理后换 6×4、重剑、皮甲，第 11 代；非法安全格放置拒绝；Tab 关闭 | `290588F2420F291076C58209D1DCBA6AFE5E45CC5A58722DF4DF95F652C333E7` |
| `20261004T175805539Z-592dd34e` | attempt-006，1080p 第 11 代重开；灵草 12 便捷转移到仓库，第 12 代；安全格拒绝武器仍第 12 代；具体文案和旋转尺寸正确；Tab 返回 | `D0FB8C5BE2CF0435CBD103E18F6206E9536C939911DA98F77832429C5E168EB9` |
| `20261004T180501295Z-27f23414` | attempt-006，720p 第 12 代重开；仓库灵草／装备／安全格／钱包保持；安全格拒绝不写盘；Esc 返回 | `C7B0F25C0A04961EE396183FBDBA52F18595B95BE0274C5F72DF6D3AA66CE688` |

Play 前缀 `Saved/FoundationRuns/Demo20.S01/Play/`，每目录内为 UnrealEditor.log。以上窗口均正常 Alt+F4 关闭且日志有 LogExit: Exiting；非阻塞启动器的 run-state.json 留在 STARTED，exit_code=null，不能据此伪写产品进程原生退出 0。无头自动化和构建的原生码另行核验。

最后两个产品日志的 DEMO20_VIEWPORT 分别确认 Pixels=1920x1080／1280x720，WidgetScale=1.000／0.666。桌面截图受 Windows DPI150% 与窗口边框影响，分别为 2882×1670／1922×1128，不将截图像素冒充游戏视口。

提交的真实截图：[缩容拒绝](../Demo20/Evidence/M1.Preparation/storage-shrink-refused.png)、[第 7 代 720p 重开](../Demo20/Evidence/M1.Preparation/reopen-1280x720.png)、[最终 1080p 策略拒绝](../Demo20/Evidence/M1.Preparation/secure-policy-1920x1080.png)、[最终 720p 第 12 代重开](../Demo20/Evidence/M1.Preparation/final-reopen-1280x720.png)。截图为原生窗口捕获，未合成或修改。

## 6. 边界与交付核验

本轮修改的 Grid／RepositoryGrid／Persistence／AuthorityService 生产文件未扫描到 demo_map、UWorld、AActor、ApplyDamage 或 RNG 引用；ShanmenItems.Build.cs 不依赖 Engine。容量和预览不需要 World／Actor。

保护文件 113 项核验不变，用户已有四个 tracked 修改和历史 untracked 不暂存。git diff --check 通过；本轮修改的四个 PowerShell 脚本语法解析通过，文档相对链接存在，Log 中 25 个证据 SHA-256 与对应原件逐项匹配。精确暂存名单提交前核验；Saved 原件不纳入 Git，不声称 GitHub 已包含全部运行日志。

未执行本增量 PIE、完整探索战斗／治疗／随机掉落／终局恢复、部分堆叠 UI、真实持键失焦／捕获取消、产品保存失败提示、运行帧耗时／内存／多轮释放。理论位集、序列化大小、单次进程内存读数不能替代 M5 性能验收。

下阶段继续有限基础补给和正式 Run 出发，不重置初始化档来免除死亡全损后的补给要求。完整 Goal 不缩减、不标完成，旧自动化继续暂停。
