# 山门 Demo2.0 M2 携带丹药治疗 Development Log

日期：2026-10-04 至 2026-10-05。基线：`fa91f97953958ac29c1992fdf036c6ad5194b95a`。分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。已编码和自动化通过；实际非满生命治疗未通过验收。交付范围与限制见 [Report](../Report/Demo20.M2.Medicine.r0_report.md)，完整探索 Goal 保持活动，旧自动任务暂停。

## 1. 实现与保护

读取完整 Goal、Entry Report／Log、Git 状态、唯一物品权威、正式 Run、世界 checkpoint、Session、输入与 HUD。保护核验 113 项通过；四份用户 tracked 文档修改、104 项原有 untracked 文件及真实存档不提交。仅使用 MedicineAcceptance_20261005 隔离界面档及原生自动化独立目录。

普通携带通过 PreparePreparedRunQuantityIntentDurable／FinalizePreparedRunQuantityIntentDurable，安全格通过同一 AuthorityService 的 ReserveDurable／CommitDurable；产品协调器没有库存实现。生命端口是既有 CommitLedger 的 revision 校验包装，不清空 Impact 账本。快捷栏第一键默认 1，一次最多 35 生命、上限 100、恢复 0.6 秒，普通优先安全其次，仓库不可用。

先保存固定世界意图，再扣药，再确认治疗后世界，最后发布 Session／HUD。意图绑定 Run、序号、物品、预期数量／版本和来源；失败保留同一请求暂停，不换药或提前治疗。恢复先读唯一 Run 身份，完成原意图后才严格读余额；安全格最后药品只允许精确已提交 reservation 授权墓碑恢复。物品 schema 6 未改；世界无意图原 333 字节，待确认可选尾部增加 33 字节，确认后回到 333，未知尾部拒绝。

## 2. 首次失败和修正

首次 Editor 构建原生 6、外层 1、383.163 秒，Game 因 fail-fast 未运行。路径 `Saved/Automation/Demo20.M2.Medicine/Build/attempt-001/demo_mapEditor-Win64-Development.stdout.log`，SHA-256 `494DD63F9DAC3AE14E519507C83FAA70686432F6B9C56ED3426F86458B0158DD`；同名 result.json `F2B82C21A039B24C8860B900186643E50AE22C47DF8BE67491CD30BF5327B641`。错误为新增测试复制不可复制的 AuthorityService，夹具用 TUniquePtr 在 Restart 时重建原生服务，生产语义与非零基准断言不改弱。

003 停在 bundled dotnet 启动，原生 -532462766（0xE0434352），外层 1、19,871.122 秒，无源码编译诊断。004 同位置停滞，核对 UBT 命令与本任务 PID 33848 后仅终止该 dotnet；先前 Stop-Process 未成功，最终 CIM Terminate 返回 0，004 原生 1／外层 1、191.039 秒，Game 未运行。两者不能计为编译通过，也不能从它们推出业务源码缺陷；005 正常权限成功不能单独证明权限是唯一根因。没有终止正在运行的 UE。

## 3. 双目标构建原件

命令 `Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M2.Medicine -AttemptId attempt-00N`。001／003／004 外层 1、Game 未运行，002／005 双目标原生和外层均 0。统一路径前缀 `Saved/Automation/Demo20.M2.Medicine/Build/`，表中 SHA-256 对 stdout.log：

| 尝试与目标 | 原生码／秒 | stdout 路径 | SHA-256 |
| --- | --- | --- | --- |
| 001 Editor | 6／383.163 | `attempt-001/demo_mapEditor-Win64-Development.stdout.log` | `494DD63F9DAC3AE14E519507C83FAA70686432F6B9C56ED3426F86458B0158DD` |
| 002 Editor | 0／22.271 | `attempt-002/demo_mapEditor-Win64-Development.stdout.log` | `276DE58C54F4E56784116A548CE1832CCD2DA7DFC4636A197D406F22EA8219AF` |
| 002 Game | 0／403.956 | `attempt-002/demo_map-Win64-Development.stdout.log` | `5781EC41975250370FBBB364C386FD4D130E048AE2EDB90AF44109E666042B47` |
| 003 Editor | -532462766／19871.122 | `attempt-003/demo_mapEditor-Win64-Development.stdout.log` | `241D79AE6954131BE4FA887E6CF8C3302CEE8F7255F736F355F5718BE7D90883` |
| 004 Editor | 1／191.039，中止 | `attempt-004/demo_mapEditor-Win64-Development.stdout.log` | `241D79AE6954131BE4FA887E6CF8C3302CEE8F7255F736F355F5718BE7D90883` |
| 005 Editor | 0／99.226 | `attempt-005/demo_mapEditor-Win64-Development.stdout.log` | `F51A8A63E8A10BAB6E96010DFED3854806F3268C84866E24BF2E2AC526B4B90B` |
| 005 Game | 0／25.854 | `attempt-005/demo_map-Win64-Development.stdout.log` | `5663898AB20FD150A59CC23F89BCFF485DDF3562804E65508ECBFBBDE4BF91AE` |

002 发生在库存页文字最后修改前，仅作历史。003 22:34:04.618Z → 次日 04:05:15.740Z，其 result.json SHA-256 `115BA5268178ABA14F3F718B2C411318D38D6393922C8B1073B9FF0644453B8D`。004 09:16:42.536Z → 09:19:53.575Z，result.json `42AFD2C4159E4F268026B1B08B622083711D9ABEE63A6C9AB465B39557B15EB2`。

最终 005 Editor 09:18:46.135Z → 09:20:25.361Z，99.226 秒包含 Build.bat mutex 等待，UBT 内部 31.44 秒；Game 09:20:25.381Z → 09:20:51.235Z。result.json SHA-256：Editor `9D54B4D607E109CFFEBF54E7020D1840FD3545EEF74B006A101B6B08096C73D5`，Game `39B137D293E5B40B1CA55668F3661E334E14CD71E6DD2FDC4AEE472BAC17C541`。stderr 均空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。构建耗时不代表游戏帧性能。

## 4. 改动驱动最终回归

最终命令 `Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M2.Medicine.FinalRegression`，外层 0。映射门 `PASS Changed=19 Rules=2 Required=7 Logs=7`；七组原生 0、全部结束队列与成功数相同。325 次 Success、0 Fail，按结果 Path 去重 311；CombatRuntime 全组和三个子组有 14 次重复覆盖，不算独立新增测试。

统一日志前缀 `Saved/FoundationRuns/Demo20.M2.Medicine.FinalRegression/`，下表对 UnrealEditor.log：

| 组／Success-Fail／Queue | 路径 | SHA-256 |
| --- | --- | --- |
| Demo20／34-0／34 | `Automation-Shanmen.Demo20/20261005T092121107Z-64c33ce2/UnrealEditor.log` | `D6F666E3FA755686FE91E723831499EBAB0A747C5FC39079CD0BC2179BEDE586` |
| Items／119-0／119 | `Automation-Shanmen.0_0_10.Items/20261005T092226823Z-bf4d6693/UnrealEditor.log` | `A7596C64E0D24667B3CFAA91988E4B000455A47D5F897E598B653D100DFB30B1` |
| CombatCore／9-0／9 | `Automation-Shanmen.0_0_10.CombatCore/20261005T092333653Z-b8d15b5a/UnrealEditor.log` | `95EBCFEBA1A5B341147ED76B7F0223369F60E469410C89B1698C733033C0F07B` |
| CombatRuntime／149-0／149 | `Automation-Shanmen.0_0_10.CombatRuntime/20261005T092419134Z-ed79bae7/UnrealEditor.log` | `58C4593BC48E6FCB35F4CC5A2D2C04F3E5F17AA94609152B7A49CF4E46D49C4C` |
| BasicSword／4-0／4 | `Automation-Shanmen.0_0_10.CombatRuntime.BasicSword/20261005T092519730Z-705357f7/UnrealEditor.log` | `D6C9C7F5D9893BA2D4864365329172E9E26C2FBB48E694485311DE84EEFB7C0D` |
| VitalityAuthority／6-0／6 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityAuthority/20261005T092601070Z-46c086f5/UnrealEditor.log` | `2D57B9FEC2CF9B25814A45B9CF28A4DED9C35B335DF5193A288E69AB0DC9C798` |
| VitalityLedger／4-0／4 | `Automation-Shanmen.0_0_10.CombatRuntime.VitalityLedger/20261005T092642261Z-2f2ab8df/UnrealEditor.log` | `31C29C03AFE363EA4011C232683D31C5ECD967CD29AAAE05D7551B1BDE883435` |

同目录 run-state.json 均为 SUCCEEDED／exit_code=0，只代表进程，不单独证明断言通过。对应 SHA-256 为：Demo20 `8D4195148B9EE1298864B0E1235BF3ACBBCF861990A7210D9E2D059222F0D553`；Items `E353766361F393BA99AA98F63AA9046A4F1817F967029D9BE51F58F9551FF7DB`；CombatCore `4A0C9E00CA4FEC2AA62E9C5EB1F6AC3D1845638FD51EC86C68D1D68F9DCFD94D`；CombatRuntime `BB3F56864236C18AFA68B7EEDC51855416BF1BCC6DEA72945726A911BA2B8372`；BasicSword `1F38E25F263789A6C6971ECD6788111320FDBF176F3766440D91C97BD7F33292`；VitalityAuthority `357A391A884A00C5E79802E49FF7711A09AD6BAD377131F721EA376F0542E74A`；VitalityLedger `35D22689FFFDCA0FC24E788DFC1D8F6EAB4EBF8D24CA48415BB91B8721573D6A`。

首次回归在 002 下也为七组 325/0，路径 `Saved/FoundationRuns/Demo20.M2.Medicine.FirstRegression/` 独立保留；没有测试失败原件被取消。它发生在最终库存页文字修改前，不作为最终候选证明。本轮未修改旧 ProfileRepository 或 Items 生产文件；CombatRuntime 生产端口修改要求运行 Runtime 全组，不按治疗主题裁剪。

## 5. 新增测试和边界

六项 Demo20 测试：MedicineRejectsAndProjection、MedicineOrdinaryNativeExactlyOnce、MedicineSecureDepletedRestartAndDeath、MedicineFailureRecoveryMatrix、MedicineLegacy333ByteCompatibility、MedicineWorldAcceptanceAndSameProcessRetry。另增 VitalityAuthority.ExternalMutationRevisionAndLedger。

普通真实攻击基准 36.64 → 71.64（35 治疗），8 → 7，revision／序号各一；冷却后第二次合法用药到 100、6 剩余，满生命不改账本。普通／安全格各覆盖命令前拒绝、原生 AtomicReplace 失败、预备后提交前中断、世界替换后读回不确定；未确认时不发布 HP，原生重建服务／读档后只扣一颗、只恢复 35。最后安全格药品耗尽父容器清空后恢复精确回执，死亡不返还消耗、仓库与安全装备保持，重复终局不增代次。

333 → 366 → 重启 → 确认 → 333，未知尾部失败关闭；世界意图接纳前失败不改物品，接纳不确定／治疗写入不确定的同进程精确重试不加额外代次。生产边界扫描 Core／Items 无 UWorld、AActor、ApplyDamage、FRandomStream、FMath::Rand；demo_map 字样有一条既有 ItemTypes 注释，不能声称全文零匹配。未改这些模块生产依赖。

## 6. 实际 Standalone 部分证据

启动参数 `Scripts/Start-Demo20.ps1 -Action Play -ProfileName MedicineAcceptance_20261005 -Width 1280 -Height 720`。005 候选实际视口日志 Pixels=1280x720／WidgetScale=0.666；桌面 PNG 为 1922×1128，来自 Windows DPI 和窗口边框，文件名表示游戏视口，不是图片尺寸。

活动日志路径 `Saved/FoundationRuns/Demo20.S01/Play/20261005T092828655Z-a2fe6237/UnrealEditor.log`。09:28:41 Ready 原生物品第 1 代；09:29:31.937 正式 Run `D203213CF091FECE02A78DD6EB7130B7`、Seed 13575504546739965157、第 2 代。该 Run ID 可与另一个隔离新档相同，因为相同 Owner／初始内容／版本派生相同；档根隔离，没有共享物品文件。

09:29:39.651 MEDICINE_REJECT HP=100.00，画面显示“生命已满，没有消耗回春丹”，普通 8／安全格 0 未变。之后实际闪身进入石径，09:31:13.674 HP=52.5、Seq=3；用药尝试未在死亡前完成，09:31:22.671 HP=0、Seq=7、Phase=3，09:31:22.695 Death 终局 Success=1／第 3 代。没有 MEDICINE_USED 或成功治疗 CONFIRM，不计非满生命治疗通过，也不据工具时序推断生产缺陷。

用户用 Esc 停止电脑操作；下一轮明确选择“暂不操作窗口，继续代码与文档”。此后未发 UI 命令、未关闭／重启游戏、未再次运行构建或 UE-Cmd。游戏 PID 58300 仍在，活动日志尚无 LogExit，不能写实际运行原生 0，也不能给其活动内容当最终 SHA-256。后续允许真实验收后再另建有关闭记录的证据，不覆盖此次未完成尝试。

三张已落盘原生图片均实际打开复核；前缀 `Docs/Demo20/Evidence/M2.Medicine/`：

| 截图与用途 | SHA-256 |
| --- | --- |
| [preparation-1280x720.png](../Demo20/Evidence/M2.Medicine/preparation-1280x720.png)，正常有限新档整备 | `FA265488424F1250EF2EEC2A08C313460A66F83E3CADBE31BEFDC3B2150276CC` |
| [full-health-rejected-1280x720.png](../Demo20/Evidence/M2.Medicine/full-health-rejected-1280x720.png)，满血拒绝与数量 | `05065787C2C592EEA6AEC539A3AB4914F8D68E8741C7864AAA593B97EA6C7C7F` |
| [first-attempt-death-1280x720.png](../Demo20/Evidence/M2.Medicine/first-attempt-death-1280x720.png)，未完成治疗尝试与已确认死亡 | `A9C89040722A76EA68D8FC8F45830AD7E540FE3E72A0385D5DECE82959AC3C56` |

## 7. 提交检查和未完成项

交付清单为 19 Source、四份 Demo20 文档、两份 Report／Log、三张 PNG，共 28 个明确路径；不夹带四份用户 tracked 或历史 untracked。首次失败／最终构建与回归原件、隔离存档留在本地 Saved。提交前保护 113 项、git diff --check、36 个相对文档链接及 31 个 SHA 引用核验通过；暂存路径须完全等于本轮清单。最后只改文档，没有新增源码候选覆盖 005 构建。

待补真实用药验收：正常有限物资下受伤后普通用药，安全格用药，冷却／格挡／重复键拒绝，治疗后同档重启不返还，保存失败与恢复页，720p／1080p 和 PIE。完整 Goal 还需随机敌人／宝箱／掉落、来源接纳、搜索、领取、新丹药、局内背包、丢弃拾回及组合恢复。真实帧耗时、运行内存和多轮对象释放没有实测，333／366 字节不是其证据。

接下来继续代码与文档侧的随机来源／搜集接线，保持同一权威；直到获准才恢复实际窗口验收。当前切片不标完整 M2–M5 或 Goal 完成，旧定时任务不恢复。
