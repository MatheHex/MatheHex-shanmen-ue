# 山门 Demo2.0 确定性随机遭遇 Development Log

日期 2026-10-05，进入基线 `c652cad826664ba7e7eb0216743fe27c52f9c53b`，沿用原分支。范围见 [Report](../Report/Demo20.M3.Encounters.r0_report.md)，人工步骤见 [遭遇审查](../Demo20/ManualEncounterReview.md)。完整 Goal 保持活动，旧自动任务保持暂停。

## 1. 工作区与范围

先读取完整 Goal、最新 Report／Log、Git 和运行进程、Session、来源、世界记录、产品接线、构建／回归脚本与映射。进入无 UE 或 dotnet；本轮只后台双构建和隔离 UnrealEditor-Cmd 自动化，不开 Editor UI／PIE／Standalone、不输入／截图、不操作人工档。四份 tracked 用户修改、历史 untracked 和原存档保留，113 项保护核对。

本轮十三 Source：新增 Encounters h／cpp 与 EncounterTests；Session h／cpp、World h、Expedition、WorldCheckpoint h／cpp、Sources、ExpeditionTests、LootTests、MedicineTests。五文档为本 Log、Report、ManualEncounterReview、README、ExpeditionStatus；共十八文件。不改地图／Content／Config／启动器／旧 writer／物品 schema，也不恢复自动任务。

## 2. 固定输入

新局两个普通槽位独立选择近战或远程，精英固定，三处初始位置各四候选。冻结 r1／r2 内容身份，新版保留旧局，确认的生命／位置／预警不重置；保存禁止同一 Run 换版本。内存 EncounterRevision 由原 ContentId 解码，没有新编码字段或数量权威。六个来源角色和生成内容不变，遗物玩家标题按区域，类型不决定另一份来源。

首轮十三源码 manifest `876ACCD4822993E74ADA0A6DDFFB12EDDBFF6531B66F29810970937E4C6DA36B` 对应 001 构建与首轮失败。仅修测试读取的缺省 AuthorityRevision，加入合法非零 revision／sequence／pity；生产检查、规则、门槛不变。最终原字节 SHA 如下，按表顺序 `Path + TAB + SHA256 + LF`、UTF-8 无 BOM 得到 manifest `CE8050DE72D4E25289B0F01B65603BC7DF71E8B76B4E542469F3F79DCF78B5B3`。该哈希在 002 构建后、最终回归前计算并在提交前复核；其后不改 Source。

| Source 路径 | SHA-256 |
| --- | --- |
| Source/demo_map/Demo20/ShanmenDemo20Encounters.cpp | `38E94D932CB1CC555835BCE726380B4285208B48964AA7183AB5EC7B96D8B533` |
| Source/demo_map/Demo20/ShanmenDemo20Encounters.h | `FA538D0A4D36AA74BB1951E65FF41D0AF68DA0DCBD133666C7955C6AE396A30C` |
| Source/demo_map/Demo20/ShanmenDemo20EncounterTests.cpp | `CC43B653A2FDA0F706DA3E0CAF2E28493B298F9EFD5511AA9FCDD22ADC1B98C9` |
| Source/demo_map/Demo20/ShanmenDemo20Expedition.cpp | `1F100965047A13A66F90B7BAEAEA64E20219FC3819CF830DE3300AEBE8899B60` |
| Source/demo_map/Demo20/ShanmenDemo20ExpeditionTests.cpp | `5E6013F5F3214ACBC2BF26051596C7B3F566FDC6D973C6E6F40CF5A06C473786` |
| Source/demo_map/Demo20/ShanmenDemo20LootTests.cpp | `98AA919B2FA3281165733DB77494A24850394195DDC60B1C6E7C7581546EE949` |
| Source/demo_map/Demo20/ShanmenDemo20MedicineTests.cpp | `96CCB7334D0D21973B9D8FB9285D4945E78541E3D415C00E2299ADC44357786A` |
| Source/demo_map/Demo20/ShanmenDemo20Session.cpp | `E77771AB6CD56C99BDB81DCED9485F4946BD53AD28A543A928F6EFC58797E82A` |
| Source/demo_map/Demo20/ShanmenDemo20Session.h | `291FF9FC64C37CDE7225B498E5155A8BD6019461636B72DB7548A53ECC13EEB0` |
| Source/demo_map/Demo20/ShanmenDemo20Sources.cpp | `10CCF04BDD5EA653DAF74466CCE5ECF0C6E23452ED3A62D87DDF6F130C1A5341` |
| Source/demo_map/Demo20/ShanmenDemo20World.h | `5430462D99B5B188B35298F1E78AF76DBCA54B46EA1B1A4482FD2A8563AD73FA` |
| Source/demo_map/Demo20/ShanmenDemo20WorldCheckpoint.cpp | `EA4EBAD1C08C19E4F7D66513612D8027FE7D1C7E180D26744B3F6CE6E8B53AB2` |
| Source/demo_map/Demo20/ShanmenDemo20WorldCheckpoint.h | `2C9F427D7A3B4812D6C112D6F13CAB613D2CCC8845130808D7B5825B843B71F6` |

## 3. 双目标构建

执行 `./Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M3.Encounters -AttemptId attempt-001`，001 Editor UTC 14:51:16.094 → 14:52:10.733，54.639 秒；Game 14:52:10.771 → 14:53:11.959，61.187 秒，两 native 0、外层 0。首轮测试失败后只修测试，再运行同脚本 `-AttemptId attempt-002`；Editor UTC 14:57:17.219 → 14:57:32.185，14.966 秒；Game 14:57:32.210 → 14:57:55.274，23.064 秒，native／外层均 0。两轮无构建失败，没有覆盖尝试目录。

原件在 `Saved/Automation/Demo20.M3.Encounters/Build/<Attempt>`，后台隐藏、顺序 Editor／Game、WaitMutex／NoUBA／并行 2。表中 stem 后缀为 stdout.log／stderr.log／result.json。

| Attempt／Target | stdout SHA-256 | stderr SHA-256 | result SHA-256 |
| --- | --- | --- | --- |
| 001 demo_mapEditor | `BBF7BCC8F42D3F5741591BC543498646DF7772E1A9FF9C1D21DE8557D8F27548` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `1CFCD7AE2C0100194657338051C3AE4436D10685C021166A061274D7ED03EE75` |
| 001 demo_map | `91C04C0F23DD9A34518AFBCAD8AD1D11D71D7ADD24122043145DF0D55B109817` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `90E9242C22005826CABDA1D6535585CA2A74CB089A0102A7380448936B40E5CB` |
| 002 demo_mapEditor | `1B2FDBCA2081682F02A1DC1301C7ADCB3C2655878E5EF8CE070558CD2AC7FEE0` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `FA530119AAFF338938F7797E5CFC4F6CDF6AAE8BB0B9D7959AA6D1019ABBDE88` |
| 002 demo_map | `E0A0A8A8BF8CF3B457A750DFC01DDAD01A4AE114D1497F2D69A923BA1AB18148` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `C5B577247F22667963413CB625DC20BF4CF574269BA02E860BB5EFAB6C759BE5` |

## 4. 首次失败保留

`./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M3.Encounters.FinalRegression` 首组 Demo20 UTC 14:54:43.669 → 14:56:00.562 为 77 Success／1 Fail，Queue 78 performed。native 0、run-state SUCCEEDED 仅表示进程退出；脚本因 Fail 外层退出 1，没有继续余下五组，也没有记映射通过。

失败路径 `Shanmen.Demo20.Encounters.SeededTypesPositionsAndIsolatedChannels`，断言 Real loot generator before unrelated channel；来源 Read 的 AuthorityRevision 默认 INDEX_NONE，被 Scope 正确拒绝。原件 `Saved/FoundationRuns/Demo20.M3.Encounters.FinalRegression/Automation-Shanmen.Demo20/20261005T145443641Z-6beb84dd/UnrealEditor.log` SHA `811429A4CA5EA25D440E0885BC7CCC21300A2C9124D3BA93063B62C4A616CF0A`；同目录 run-state.json 和全部尾部保留。仅修测试输入，无生产放宽。

## 5. 最终映射回归

002 后执行 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M3.Encounters.FinalRegression.r1`，UTC 14:58:35.542 → 15:02:29.712。十三 Source 匹配一个规则、六个必跑组，全部 222 Success／0 Fail、222 个去重路径；外层 0，`REGRESSION_COVERAGE: PASS Changed=13 Rules=1 Required=6 Logs=6`。不把首轮 77 次成功混入最终 222，也不冒称最终全新旧根通过。

原件 `Saved/FoundationRuns/Demo20.M3.Encounters.FinalRegression.r1/Automation-<Group>/<Run>/UnrealEditor.log`，同目录 run-state.json 全部 SUCCEEDED／native 0。各目标 Cmd 1、Queue performed 1、Fatal／Unhandled／Ensure 0；每组 Cmd 前仍有十三条 Engine 启动 Condition failed，未隐藏、不声称零 Error、不改 Engine。

| Group | Success／Fail | Run | UnrealEditor.log SHA-256 |
| --- | --- | --- | --- |
| Shanmen.Demo20 | 78／0 | 20261005T145835513Z-f4f3b10f | `6EFA334CEDFF4439ADA67ED6663A2F12527A02732AE99CB28D6FD452955A2CD1` |
| Shanmen.0_0_10.Items | 121／0 | 20261005T145931134Z-473f9483 | `540BF916A5B81CCC3172C6357546B1B03565B496A284024D5E6B1E7D27C66154` |
| Shanmen.0_0_10.CombatCore | 9／0 | 20261005T150023191Z-d3785ce6 | `1CD88DA6FAE3864C6F2C1F36A741EADB773DB1E810A4BA8654F48C888EC49CC0` |
| Shanmen.0_0_10.CombatRuntime.BasicSword | 4／0 | 20261005T150054810Z-22f88b9f | `3C5F64C2097BF80083DBB91FA27E7D6BA436C44D22DCBC755BA43B67FD9ECEE7` |
| Shanmen.0_0_10.CombatRuntime.VitalityAuthority | 6／0 | 20261005T150126425Z-a56a75ac | `39B460934AE59F4F414662F7DCC69FDF97B33B97D2391084A43A8ADF27189AD7` |
| Shanmen.0_0_10.CombatRuntime.VitalityLedger | 4／0 | 20261005T150158077Z-090afd4e | `1C9CDBC92350F0E558964A74C15A2A280D64877BB6FC60DC30843C3E24036EAB` |

四新增用例覆盖样本类型／位置与真实生成通道隔离、独立生命／伤害／格挡算术、恢复上限及保持非零值、两版本原生编解码／死亡／位置／预警／内容锁、替换前后失败／精确重试与提前撤离。既有所有 Demo20／Items 等组继续执行；旧 Source 固定随机向量、药品／混合堆／地面／终局测试未被换成恒真基准。旧 Profile／Code B／共享随机源码未改，本轮不额外声称补跑其全根。

## 6. 检查与实际验收

生成器 h／cpp 边界扫描无全局 RNG、NewGuid、UWorld／AActor 或 Tick 依赖；它仍是 demo_map 产品模块内的内容派生，未宣称纯领域新模块。十三 Source 固定哈希复核通过；Source／阶段文档 diff check 0，五文档 74 个本地链接有效，113 项保护核对、暂存前索引为空。精确暂存与 cached diff check 再验证，原始 Saved 证据不进 Git。

本轮没有窗口、输入、截图或新实际战斗通过；人工审查独立继续，失败路径不再叠加，其他开发不等待反馈。Item Schema 6 和世界编码布局不变，空意图 333 字节不当作内存测量。三个规格各在 Session／GameMode 缓存，不增 Actor，仍有字符串派生／候选复制成本；真实帧耗时、内存、对象释放未测。

## 7. 交付与目标状态

十八阶段文件精确提交、非强制推送原分支并核对远程 SHA；保护用户 tracked／untracked，不操作真实档、不恢复自动任务。当前只闭合最小种子遭遇，三只敌人不等于 10–15 分钟内容量；内容扩充、结算明细、安全恢复、完整真实循环、PIE／Standalone、双分辨率、性能和 M5 全根回归仍有工作，Goal 不标完成。
