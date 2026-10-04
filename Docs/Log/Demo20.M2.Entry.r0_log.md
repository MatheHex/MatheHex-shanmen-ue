# 山门 Demo2.0 M2 正式探索入口 Development Log

日期：2026-10-04。基线：`4ae5f06b7519cb1cd8e5eb1b4c709d70f339d62c`。分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。本轮是正式入口增量，完整探索 Goal 保持活动，旧自动任务暂停。最终 009 候选双目标构建、FinalRegression04 六组及双分辨率恢复／锁定页复核均通过，交付范围见 [Report](../Report/Demo20.M2.Entry.r0_report.md)。

## 1. 实现与保护

读取完整用户 Goal、最新 Loadout Report／Log、分支／Git 状态、当前物品权威、Session、Controller、Widget 和原生启动脚本。113 项保护核验保持；四份用户 tracked 文档修改、104 项原有 untracked 文件与真实存档不提交。界面仅使用 EntryAcceptance_20261004 隔离档；自动化使用独立目录。

新青岚关从空地图生成，正式 GameMode／PlayerStart 独立，不复制旧 M01。运行时固定三分区、三类敌人和占位几何；Session 冻结装备属性，仍通过 BasicSword／CombatCore／VitalityAuthority 结算。正式出发世界先确认后调用既有 StartLoadoutDurable；终局世界决定先确认后调用既有 FinalizePreparedRunDurable。UI 没有库存写入或伤害公式。

世界 checkpoint 保存相同 Run／Seed、生命／revision／序号、位置、预警计时与终局，不含物品数量；物品 schema 6 未改变。固定内容戳、最大 1,024 字节、长度与完整性摘要、精确旧字节 CAS、临时 flush／校验／备份／替换／读回；替换后不确定精确重试只接纳一代。损坏主档拒绝，不回退旧备份复活敌人。当前世界档实长 333 字节，不作为运行内存证据。

## 2. 首次失败原件

首次构建 `Saved/Automation/Demo20.M2.Entry/Build/attempt-001/demo_mapEditor-Win64-Development.stdout.log`，UE 原生 6，21.266 秒，外层 1，Game 因 fail-fast 未运行。SHA-256 `4D511B58BB0C4D803D744F7D514CCCCFD813DF97184770E89D240085A612A1BF`；同名 result.json `75E1ECE701F6BB3E7ED8CBC029182804C47A480F3EDBF732B95477D025F8EA82`。GetPawn 的 TObjectPtr 不能按五处 auto* 推导，改为显式 APawn 指针，没有改变业务规则。

首次回归前缀 `Saved/FoundationRuns/Demo20.M2.Entry.FirstRegression/Automation-Shanmen.Demo20/20261004T203555527Z-d85a1bdd/`。26 Success／2 Fail，Queue Empty 28，UE 原生 0、验证外层 1，其他组未跑。UnrealEditor.log SHA-256 `7462D18C99EDA260218FB059FC168EDDF859F14495ECA356DB8A0036AC9BA7E3`；run-state.json `D04F2874DD960D02A9E041ECD74AEF9FA2B6BF4189665E9F33FCAE771B6AC910`。SUCCEEDED 只表示原生退出 0，不证明测试通过。

失败为新增护甲层缺必要 LayerTags，导致 Resolve 拒绝，非满生命断言失败。修正生产层 DefenseArmor 标签，保持 96.04／83.72／84.16／87.04 等非零生命期望，没有弱化断言或抹掉失败日志。

实际恢复首次背景近地视角错误，原件 `Saved/Automation/Demo20.M2.Entry/UI/restore-camera-first-1920x1080.png` SHA-256 `B0A491A814705F7693096C85FD4C3F00532CF0070975AEFBF83C8D2F0BED6920`。005 的 BeginPlay 初始化未解决，006 的 Controller 暂停视图刷新也未解决。007 允许暂停视图更新并增加诊断，仍失败，但记录正确的缓存位置 X=-740.277 Y=50.998 Z=1554.393、俯角 -60 度。

读取本机 UE 5.8 PlayerController.cpp 的 GetPlayerViewPoint 后定位到时间零判据：默认只有 GetCameraCacheTime()>0 才读摄像机，否则退回 ViewTarget 位置。恢复立即暂停时世界时间为 0，缓存已更新仍被当作未初始化。008 仅在 Demo20 正式模式、已显式更新缓存、暂停且缓存时间为零时覆盖视点读取；不提前推进世界或敌人。009 的真实双分辨率均正确俯视，CacheTime=0.000 时 PlayerView 与摄像机位置／旋转一致，生命和位置不变。

## 3. 双目标构建

命令 `Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M2.Entry -AttemptId attempt-00N`。001 外层 1、Game 未跑；002–009 各次双目标原生 0、外层 0。005／006／007 虽编译成功但恢复画面仍错误，不能作为最终可见候选。表中 SHA 对 stdout.log，同目录 stderr／result.json 保留。

统一前缀 `Saved/Automation/Demo20.M2.Entry/Build/`：

| 尝试／目标 | 原生／秒 | stdout 相对前缀路径 | SHA-256 |
| --- | --- | --- | --- |
| 001 Editor | 6／21.266 | `attempt-001/demo_mapEditor-Win64-Development.stdout.log` | `4D511B58BB0C4D803D744F7D514CCCCFD813DF97184770E89D240085A612A1BF` |
| 002 Editor | 0／6.764 | `attempt-002/demo_mapEditor-Win64-Development.stdout.log` | `3B2EB56790EF39D1F785056EBA6C7C5D2F7084BBD8AC7033483D7A42E08BEE30` |
| 002 Game | 0／27.560 | `attempt-002/demo_map-Win64-Development.stdout.log` | `BD2714CE0F253E3205F4D0EFD99130870B607DEEA23892487ACA0296FBB205CE` |
| 003 Editor | 0／6.337 | `attempt-003/demo_mapEditor-Win64-Development.stdout.log` | `5A2ACE5318AA6134138EDFA348E2101A08031408268A46F04FA34118FD764C49` |
| 003 Game | 0／12.028 | `attempt-003/demo_map-Win64-Development.stdout.log` | `134D9BD48F17F7640D9B3B66610DC05AB5C8493B7DA0F032441EE9FE8924902D` |
| 004 Editor | 0／21.658 | `attempt-004/demo_mapEditor-Win64-Development.stdout.log` | `C0C453F5DEBC114656470ACD6E0D525CC302805040C309242714F5D8AD2375EA` |
| 004 Game | 0／26.968 | `attempt-004/demo_map-Win64-Development.stdout.log` | `B8B79BA335FAAE18D1395FE2BE307E61986AE0ED3DD664AF706455FFE91C7771` |
| 005 Editor | 0／36.725 | `attempt-005/demo_mapEditor-Win64-Development.stdout.log` | `ED986E674092B34A7C84C90A6032E1E290EE6960F9972F6598D66BFA6A932BF2` |
| 005 Game | 0／48.620 | `attempt-005/demo_map-Win64-Development.stdout.log` | `033D36143FC67A24B6212C37137BD13DAADDEC5B7E64F0DAFC9852E0F62466E1` |
| 006 Editor | 0／17.531 | `attempt-006/demo_mapEditor-Win64-Development.stdout.log` | `D220A8C835680D73AEB44AE633013018CF400AB6CE4152C89166FD4C4BAB5F52` |
| 006 Game | 0／29.373 | `attempt-006/demo_map-Win64-Development.stdout.log` | `15D24F5515F1EA832F7D3D91E6E019E57E611FC9AF9FB23C80B93BE4AABFF1B8` |
| 007 Editor | 0／33.102 | `attempt-007/demo_mapEditor-Win64-Development.stdout.log` | `53DE1F64EB11593F5BACBEFA46556DC742AE2F839F4ED0401F5BDD41DF050B1C` |
| 007 Game | 0／33.307 | `attempt-007/demo_map-Win64-Development.stdout.log` | `4B32E5C430FF1923271CA9180D59AAF178A7C1B57651DB6DC0CE314A61D36E21` |
| 008 Editor | 0／34.647 | `attempt-008/demo_mapEditor-Win64-Development.stdout.log` | `7D3407CF78F5D10D0AC23A80E91286B3B6DF15F55D630CB887F0825CD827D321` |
| 008 Game | 0／39.736 | `attempt-008/demo_map-Win64-Development.stdout.log` | `10F2DA59C6064603D8066C12A505A228D3CABF43BE364F7AEFACF0B976E2EE54` |
| 009 Editor | 0／16.368 | `attempt-009/demo_mapEditor-Win64-Development.stdout.log` | `83F4F8362E8300FA9F4E88A5839028812E5ACE640E51E6C491D6E0CD8BADA184` |
| 009 Game | 0／22.610 | `attempt-009/demo_map-Win64-Development.stdout.log` | `59E7D64210563F938B90A40F1015678C1CD3DC9546CC82619CD693B64BA1AB3E` |

007 Editor 21:21:53.071Z → 21:22:26.174Z；Game 21:22:26.199Z → 21:22:59.505Z。result.json SHA-256：Editor `DE65EAC5751FE634A762A13B77672D4020AF44E728F8F33111D88C0DB6DC9F5C`，Game `04224BB2DE0C15CF58E91A74511B468835B1A090BF865D7F1312F8220DD53FC0`；stderr 均空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。构建耗时不是游戏帧性能。

摄像机候选 008 Editor 21:32:16.173Z → 21:32:50.820Z；Game 21:32:50.845Z → 21:33:30.581Z。result.json SHA-256：Editor `62A8FD3B64E19184BE6D8DD68686F9FDB4C0FC27AF0C02883D9C7FA166CA24C5`，Game `99FAE17FB256F985BF2A6153264F4254C4AF59588008BA8C1A47583B143D50D8`；stderr 均空，SHA 同上。

最终界面候选 009 Editor 21:37:12.742Z → 21:37:29.110Z；Game 21:37:29.129Z → 21:37:51.739Z。result.json SHA-256：Editor `D6717A4C111B32EC3372515A607C8E680C15D71B483689A87FB173854E7759CC`，Game `83B70B7FD54E40494FB67EB13C88A41AB83A98390ADD297BB0CFE8217908D098`；stderr 均空，SHA 同上。修正保留原局的“尚未出发／续局尚在接通／未装备行囊”误导提示；锁定状态仅在快照刷新时由 InspectActive 读取并缓存，不在 Paint 遍历回执、不增加数量权威。

## 4. 回归与地图

Regression02 为 169 Success／0 Fail、六组原生 0、外层 0，覆盖 `PASS Changed=9 Rules=1 Required=6 Logs=6`，发生在地图和最终界面修正前，原件保留 `Saved/FoundationRuns/Demo20.M2.Entry.Regression02/`。

FinalRegression03 对应 004，169 Success／0 Fail、六组原生 0、外层 0，各 Test Queue Empty 为 28／119／9／4／5／4，覆盖 `PASS Changed=12 Rules=1 Required=6 Logs=6`。路径前缀 `Saved/FoundationRuns/Demo20.M2.Entry.FinalRegression03/`，表为 UnrealEditor.log：

| 组／Success-Fail | 日志路径 | SHA-256 |
| --- | --- | --- |
| Demo20／28-0 | `Automation-Shanmen.Demo20/20261004T210048796Z-ec50c00f/UnrealEditor.log` | `B81B700A5EA6816B127D0F0AE667FCAEB54DF253DBBB0A0308A789814551DFF0` |
| Items／119-0 | `Automation-Shanmen.0_0_10.Items/20261004T210129441Z-0033f9ff/UnrealEditor.log` | `58AAC83DCF211DB58BC6246E1695BB09A450A81487B27FEA531D9A34DC4512E3` |
| CombatCore／9-0 | `Automation-Shanmen.0_0_10.CombatCore/20261004T210224969Z-97bee60f/UnrealEditor.log` | `15719C1371B1292D3C66164EECE79F63C527628F9AA390D590B63925F3FAAC2C` |
| BasicSword／4-0 | `Automation-Shanmen.0_0_10.CombatRuntime.BasicSword/20261004T210300427Z-280a2622/UnrealEditor.log` | `1DC18C2B5D90F8E4CDB885D943906AD553C13950E8CB758C805515FF47CE2854` |
| VitalityAuthority／5-0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityAuthority/20261004T210331831Z-57143809/UnrealEditor.log` | `20DAADB267B5D85D9FA29066F1BD73BAE29E88152A3F0B9BABBD833C4A46B933` |
| VitalityLedger／4-0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityLedger/20261004T210407291Z-c825d58f/UnrealEditor.log` | `DF341733E3801C3129A31B17CCF1C319B12A5FAC75BD4447E72E1E7A8B293BC9` |

最终 009 候选重新运行 `Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M2.Entry.FinalRegression04`，169 Success／0 Fail，六组原生 0、外层 0，各 Test Queue Empty 为 28／119／9／4／5／4，覆盖 `PASS Changed=13 Rules=1 Required=6 Logs=6`。路径前缀 `Saved/FoundationRuns/Demo20.M2.Entry.FinalRegression04/`，表为 UnrealEditor.log：

| 组／Success-Fail | 日志路径 | SHA-256 |
| --- | --- | --- |
| Demo20／28-0 | `Automation-Shanmen.Demo20/20261004T214206142Z-878b99ae/UnrealEditor.log` | `75DD3982FCDE69AAB4B10EC445D1EE3D6273785ECCDE4876446BD10380F2EE7A` |
| Items／119-0 | `Automation-Shanmen.0_0_10.Items/20261004T214246779Z-59c2af59/UnrealEditor.log` | `8FB6A1CE3FDC74A4F41AE05A4D126F7D0D7FA64A6CE5664B1DE9A35024F4C1CD` |
| CombatCore／9-0 | `Automation-Shanmen.0_0_10.CombatCore/20261004T214342341Z-0d7bf334/UnrealEditor.log` | `799C4B7949E4811C5A76F31034720AE786536AE40C86A6C54F467C1FB5915047` |
| BasicSword／4-0 | `Automation-Shanmen.0_0_10.CombatRuntime.BasicSword/20261004T214417773Z-77a586c7/UnrealEditor.log` | `77606CDD133875F5B07B75DC73F44E9E41D1FE077717D24A9CA68ED25EFC2A7D` |
| VitalityAuthority／5-0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityAuthority/20261004T214449067Z-ca73d5b5/UnrealEditor.log` | `45545E8215F2EA252A9A72F43D2B9027279EBF7DEDE1C9CC7C1BB81D49B2EF0F` |
| VitalityLedger／4-0 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityLedger/20261004T214520445Z-8412f29a/UnrealEditor.log` | `3339E1E6A278EF3CDE9B222A9A19C213E4349F63BDA19AC84B865A3BE3BD6B93` |

映射使用 Scripts/ShanmenRegressionMap.json 的 Demo20StandaloneSlice：Source/demo_map/Demo20 与 Content/Demo20 要求全部六组；本轮无旧 ProfileRepository 或 Items 生产代码改动。改动驱动门不等于 M5 最终完整新旧根验收。

新增四项：CombatCheckpointAndGear、WorldCheckpointRoundTrip、WorldCheckpointFailureAndCAS、NativeEntryAndTerminalRecovery。测试真实原生世界／物品存盘、非满生命、敌人已死、序号和种子恢复、失败关闭、CAS、替换后精确重试、终局恢复及重复无额外代次；没有假成功回执。

新地图生成命令 `Scripts/Start-Demo20.ps1 -Action GenerateExpedition`，原生 0、外层 0，日志有 DEMO20_EXPEDITION_MAP_CREATED。原件 `Saved/FoundationRuns/Demo20.S01/GenerateExpedition/20261004T204230119Z-7b0cea48/UnrealEditor.log` SHA-256 `E3E2BE605240FE27C4A4DA61B47BC86FE65FB89AD68CCF9A0DE0EF09C63441B0`。地图 `Content/Demo20/Maps/L_Demo20_JadePass.umap` SHA-256 `883E194C8F2779533ECE09C4404FBA63028D4AEF4715684F2A5BAB9C793BD067`，生成器拒绝覆盖既有资产。

## 5. 实际 Standalone 与原局

正常启动参数 `Scripts/Start-Demo20.ps1 -Action Play -ProfileName EntryAcceptance_20261004 -Width 1280 -Height 720`，恢复检查改 Width 1920／Height 1080；无控制台命令、手工改档或调试发物。原生桌面截图受 Windows DPI 影响，不把截图像素当游戏视口。UI 启动器为非阻塞，最终原生码 null，不能写 UI 原生退出 0；关闭以各 LogExit: Exiting 为证。

日志前缀 `Saved/FoundationRuns/Demo20.S01/Play/`：

| 目录后缀／候选 | 观测 | UnrealEditor.log SHA-256 |
| --- | --- | --- |
| `20261004T204322822Z-ec8f13cb`／003 | 720p 新档撤离、死亡、安全保留、有限补给、替代装备和再次出发、暂停返回／继续 | `847F67CF6B595584D8853A74DCCFAF913AB48277F90B4538D0AA30F7550F76A3` |
| `20261004T210456825Z-ea80cfa0`／004 | 720p 同 Run 继续，闪身到石径后受伤 35.2，Esc 保存暂停 | `093E664EB3C8143240240E81C34F5C46FF1AB5790FC4E193532FD19A5821F2B2` |
| `20261004T211106398Z-42c415e8`／004 | 1080p 恢复相同受伤原局，发现暂停背景错误；保持暂停正常关闭 | `07341EEBC6DB338293C173D3B9E750110248144D582F200968214775996AE820` |
| `20261004T211432160Z-b4ad3e10`／005 | 同档恢复值保持，首次摄像机修正仍失败 | `99A3700A1BDA4BDB5CB3BDF55A25CE6FA0450014D72A8D1997946F05D6F194D9` |
| `20261004T211855076Z-701b57be`／006 | 同档恢复值保持，暂停更新缓存仍未改变近地背景 | `4A9C0C5AEA9B12F24700C1A7AE3E71D25E15ED7A3BE531D9965E77DD48ACF7D4` |
| `20261004T212333216Z-f3efa7a0`／007 | 缓存俯视位置正确，实际背景仍错误；未推进世界，正常关闭 | `ECADD3808472A4654155CFF196F40CCFAD97CE378E09C0ED9A28C3437D32283A` |
| `20261004T213354714Z-c6b3dc8c`／008 | 1080p 初始暂停正确俯视；同一 HP／位置／Run，保存返回入口及锁定页 | `22226AECC376F70D08EB7DEB74C25311F65749ED427216D7EC59CE5BBA76F291` |
| `20261004T213823894Z-451a1ebd`／009 | 720p 原局暂停俯视正确；保存返回入口、锁定提示、尝试旋转仓库物品被拒绝 | `4B6F610E75272D31AC52D5A76C774D606216B588DB88ED35F0C597D7C35D90C0` |
| `20261004T214043460Z-74df2659`／009 | 1080p 同一受伤原局；暂停、入口与锁定背包布局可见；正常关闭 | `869FCC58E5809089414B494FECCC7AB0E2DA97F6664593F259AA260644C5A0A0` |

第一局 Run `D203213CF091FECE02A78DD6EB7130B7`，Seed 13575504546739965157，启动第 2 代，20:44:12 撤离第 3 代（约 13.2 秒，0 击杀）。第二局 `A4A48D8FB80C672B48CD574D7B9B6CC4`，Seed 2776476014842574004，第 4 代；接敌／预警／死亡，20:46:45.942 终局第 5 代。普通青锋剑／道袍／小行囊／原丹药为损失墓碑；护命匣 1、玉简 1、仓库灵草 12／矿石 6／残卷 1／替代装备和钱包 1,000,000 保持。

20:47:52.208 基础补给仅一行 2 丹药，第 6 代，已有仓库替代装备不重发。真实穿戴玄铁剑／皮甲／扩容行囊分别第 7／8／9 代，携丹药第 10 代，实际网格 8×5。20:55:27.500 重复补给重放，仍第 10 代，无货币／物品增加。

第三局 `BA7EEEA8CE21AB554CD9D74543B53F27`，Seed 15685435928769418195，20:55:44.825 第 11 代。003 暂停保存第 15 代世界，返回入口第 16 代，继续原 Run，第 17 代正常退出。004 恢复后真实闪身至 X=109.723 Y=50.998 Z=82.150，受到 5 次皮甲减免攻击，生命 35.2、Seq=5；21:10:24.132 暂停世界第 310 代，21:10:46 正常退出保存第 311 代。

1080p 21:11:35.860 恢复第 311 代、相同 Run／Seed／生命／位置；005、006 也保持原局。物品档始终 schema 6、第 11 代、revision 22，SHA-256 `25E45D858B393B84AD1829B05E3B80867A5BDB76F8978BFB8D432682DF6DAC66`，没有新预留或重发。当前没有真实治疗、新来源、实际击杀恢复或持续按住 W／右键证明。

009 的 720p 21:38:34.996 恢复世界第 316 代，21:39:40.390 保存返回入口第 317 代；HP=35.2、Seq=5、相同位置，等待暂停期间未推进战斗。21:40:18.559 旋转仓库灵草拒绝，Success=0 Status=3 Error=23 Generation=11，原数量 12／位置及物品 SHA 不变。面板说明原局锁定；具体操作反馈仍泛化为容器／装备条件，错误映射待完善。

009 的 1080p 21:40:54.196 恢复第 317 代，21:41:33.860 返回入口保存第 318 代，相同非满生命／序号／位置；输入未恢复、未推进一帧战斗。运行日志明确视口 Pixels=1280x720／WidgetScale=0.666 与 Pixels=1920x1080／1.000，暂停页、入口与锁定背包说明无主要遮挡。仅证明这些表面的当前可见状态，不冒充全部界面拖放、PIE、持续输入或完整战斗验收。

原生截图前缀 Docs/Demo20/Evidence/M2.Entry。前六张来自 003，final-paused 图来自 004；restored-paused-final-1920x1080 来自 008 摄像机修正候选，文件名含 009 的四张为最终只读整备文字和恢复候选，不用旧截图冒充最终视觉效果：

| 截图 | SHA-256 |
| --- | --- |
| [formal-start-1280x720.png](../Demo20/Evidence/M2.Entry/formal-start-1280x720.png) | `A873AFACA600A218562C0F84FE6488E5CA2490049E150DB451090D38DF5EF557` |
| [extracted-1280x720.png](../Demo20/Evidence/M2.Entry/extracted-1280x720.png) | `F8AFB403DB8B5027F95287B302426F4CF2656B442A1EB1DBFB35191B305C54AC` |
| [melee-warning-1280x720.png](../Demo20/Evidence/M2.Entry/melee-warning-1280x720.png) | `8BABBAC78AC7A8BF3A7EE3B277BCA2A65F892D6766BC9867481D0D85892F93B8` |
| [death-1280x720.png](../Demo20/Evidence/M2.Entry/death-1280x720.png) | `15C0C8242A0BFECFD758BED8327CA1D30CD95A6C88FE79DEE9FCAE82939ABCA3` |
| [death-retention-1280x720.png](../Demo20/Evidence/M2.Entry/death-retention-1280x720.png) | `99DFB01EDA2FD9A1271E8EEA7E838FA0D47D1C2F68A2458637D09AD913576B3F` |
| [resume-from-entry-1280x720.png](../Demo20/Evidence/M2.Entry/resume-from-entry-1280x720.png) | `E2AA4BE42BED75D66FC3093AAE05FE742EE767C7045FF6DCCD1C42AA3C94E2E4` |
| [final-paused-1280x720.png](../Demo20/Evidence/M2.Entry/final-paused-1280x720.png) | `2698B87A8B30913CECBA8638EA1962B587FF0AD9AD1FDFB6657E69BA48650D6D` |
| [restored-paused-final-1920x1080.png](../Demo20/Evidence/M2.Entry/restored-paused-final-1920x1080.png) | `4E8A813B8F076131A6FEBC74570635D2FCA24A5382E15667C2562EB17C28A5AD` |
| [restored-paused-009-1280x720.png](../Demo20/Evidence/M2.Entry/restored-paused-009-1280x720.png) | `160B30470E0FD00806CD5B5686E129C9F6A212F6EE2E7A05E6455353705D97D9` |
| [locked-grid-009-1280x720.png](../Demo20/Evidence/M2.Entry/locked-grid-009-1280x720.png) | `0EC9F63CDD8F40748826B9B3F41570C0C8C1C14D68C93D951D0B582D55C4DDED` |
| [restored-paused-009-1920x1080.png](../Demo20/Evidence/M2.Entry/restored-paused-009-1920x1080.png) | `68889E7E700BC2F6414BC160EEAB8F2D51ACF194EFDB1A2EF244D25FA9B79EFE` |
| [locked-grid-009-1920x1080.png](../Demo20/Evidence/M2.Entry/locked-grid-009-1920x1080.png) | `0B76E89F2E4B44FB0B68F797954A83D54110EA3A3126C534ED5892FE81D63B5E` |

## 6. 范围、检查与交付

12 个 Demo20 Source、启动／生成脚本、新地图、五份文档、Report／Log 及 12 张原生截图精确交付，共 34 个文件；用户四份 tracked 和历史 untracked 不暂存。Saved 原件与隔离档留本地。提交前保护核验通过 113 项、diff --check 通过、40 个文档相对链接有效；构建／两次最终回归／实际运行表和截图的 50 个 SHA-256 均与原件一致，最终六组原生均正常结束。暂存必须与本阶段 34 个明确路径完全一致。

当前不宣称完整 M2–M5 完成：治疗、随机敌人／宝箱／掉落计划、来源接纳、搜索、局内背包、安全格操作、丢弃拾回及其恢复尚未接通；PIE、持续输入、存盘失败可见反馈、真实帧耗时、运行内存与多轮对象释放未验收。333 字节世界档不是运行内存证据；同步 checkpoint 与历史扫描仍须测量。

下一增量优先接正式非满生命治疗和 Run 携带，再接稳定来源随机计划与搜集闭环。完整 Goal 保持活动，旧定时任务保持暂停，不自动扩大成多关、商店或打包发行。
