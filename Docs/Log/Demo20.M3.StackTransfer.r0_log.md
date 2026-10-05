# 山门 Demo2.0 跨来源普通物品堆叠 Development Log

日期 2026-10-05；进入基线 `9471be48c8ae44b11db463e22b943ef7bf5e2b42`，沿用原分支。范围与限制见 [Report](../Report/Demo20.M3.StackTransfer.r0_report.md)。完整 Goal 保持活动，旧自动任务保持暂停。

## 1. 工作区与权限

读取完整 Goal、Git 状态、最新地面 Report／Log、阶段状态、格子事务、生成计划、原携带接管、治疗、终局、UI 与验证脚本。进入时无 UnrealEditor／UnrealEditor-Cmd／dotnet，不终止或接管用户进程。用户“丹药正常使用”按已确认旧基线主路径保留，本轮不代操作战斗或推定混合药通过。

本轮不启动 Editor UI、PIE、Standalone 或产品 exe，无窗口输入与截图。后台双构建与隔离原生无头回归使用已有脚本／跟踪进程。四份用户 tracked 修改、历史 untracked 与原有存档不纳入阶段提交；113 项保护核对。

## 2. 实现输入与哈希

八 Source：新增 StackTransfer h／cpp；RepositoryGrid、Repository、Types、GridTests、Demo20 InventoryWidget、LootTests。五文档为本 Log、Report、ManualStackTransferReview、README、ExpeditionStatus，共十三文件。没有改 Config、Content、地图、启动脚本、旧 writer 或真实档。

普通同类仅来源角色不同可合并；特殊价值／词条仍精确相等。实例出生元数据和生成计划不改写；新合并／拆分回执记录两个参与身份及完整性见证。ItemInstance 为唯一当前数量；没有逐单位批次余额。旧回执空参与字段保持原样，不猜测重建。

下表为最终 Source 原字节 SHA-256。按表内顺序拼接 `Path + TAB + SHA256 + LF` 后以 UTF-8 无 BOM 计算的 manifest SHA 为 `669D245CFDED41AD2FD69F7264D95C7E343F431C4B587D6BCFE44F4FF63B8EEF`。001 构建后没有 Source 变更，映射回归期间记录并在最终校验／提交前复核；不称其为提前落盘的构建前 manifest。

| Source 路径 | SHA-256 |
| --- | --- |
| Source/demo_map/Demo20/ShanmenDemo20InventoryWidget.cpp | `862EF95260715D88155736854AA3647BFA24437B96BF56501244E70A79E6832A` |
| Source/demo_map/Demo20/ShanmenDemo20LootTests.cpp | `2FCC1C2DD922BE69AE3D23B21C620809D1C26CFB7040CF74FF7EA1993B94AE50` |
| Source/ShanmenItems/Private/ShanmenItemRepository.cpp | `30992F314C60E66D873695ED84096F7040A83AF38FC8BB0424BE2DD0CB7DEA44` |
| Source/ShanmenItems/Private/ShanmenItemRepositoryGrid.cpp | `0AC0B1C9CC89FE6D11CC99D645893E870714AB141463CA46B703C610A87094EF` |
| Source/ShanmenItems/Private/ShanmenItemStackTransfer.cpp | `FE72FCFADCEC8C1FF4FE13A747FF891E65260D37485EDAACFFAEF733E7606984` |
| Source/ShanmenItems/Private/ShanmenItemTypes.cpp | `04F49199CC8763FE8BAE04D3CC865CCF51CCCEA7D42143FD93731A9BC8B773F2` |
| Source/ShanmenItems/Private/Tests/ShanmenItemGridTests.cpp | `2DDB441F8C14A2789D459BCB79D21C8FDD3669F5E1EF6EAFAF5E539A620C9A03` |
| Source/ShanmenItems/Public/ShanmenItemStackTransfer.h | `3D3F067186A3C60CE82AFF859FE2331742322CC13CA829644439B15316CA685A` |

## 3. 双目标构建

执行 `./Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M3.StackTransfer -AttemptId attempt-001`。原件目录 `Saved/Automation/Demo20.M3.StackTransfer/Build/attempt-001`，Editor／Game 顺序执行、后台隐藏、NoUBA、WaitMutex、并行数 2。Editor UTC 14:20:54.579 → 14:21:14.841，Game 14:21:14.866 → 14:21:42.874；原生退出码和外层脚本退出码均 0，Result Succeeded。本轮首次即成功，没有失败重试日志被丢弃。

| 原件 | SHA-256 |
| --- | --- |
| demo_mapEditor-Win64-Development.stdout.log | `EFF651EF3C7E6E595B3F61DB80216D9D0FA8A853DFABA0507E59C4B4B470A797` |
| demo_mapEditor-Win64-Development.stderr.log | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` |
| demo_mapEditor-Win64-Development.result.json | `97637A09B091C7616C70F9504DEA95D54605F5784AE3A5C32C31EAB64FA1726D` |
| demo_map-Win64-Development.stdout.log | `8165FEF93FA8FB4CCA116BBF967FB52F23083F771D7F0D1248ED6259DC749CDC` |
| demo_map-Win64-Development.stderr.log | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` |
| demo_map-Win64-Development.result.json | `B9B228743027351E2ED3AA90C85A81E8077B736E9296615FA5FDDE0E7BD39798` |

## 4. 改动映射与补充回归

执行 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M3.StackTransfer.FinalRegression`。八 Source 映射六组，UTC 14:22:10.732 → 14:26:05.624，共 218 Success／0 Fail；脚本外层 0，`REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=6 Logs=6`。

另外用同一 Foundation 的 NewEvidence／TrackedProcess 端口顺序运行旧物品与快捷栏，TaskId `Demo20.M3.StackTransfer.LegacyRegression`，UTC 14:27:30.527 → 14:28:41.771，共 53／0、外层 0。它们是补充组，不冒充由这八路径新增的映射要求。两 Task 共八日志、271 次 Success、271 个去重路径、0 Fail。

前六行原件路径为 `Saved/FoundationRuns/Demo20.M3.StackTransfer.FinalRegression/Automation-<Group>/<Run>/UnrealEditor.log`；后两行为 `Saved/FoundationRuns/Demo20.M3.StackTransfer.LegacyRegression/Automation-<Group>/<Run>/UnrealEditor.log`。各目录保留 `run-state.json`，全部 state SUCCEEDED、native 0、目标 Cmd 1、Queue 完成记录 1、Fatal／Unhandled／Ensure 0。

| Group | Success／Fail | Run | UnrealEditor.log SHA-256 |
| --- | --- | --- | --- |
| Shanmen.Demo20 | 74／0 | 20261005T142210697Z-4693d846 | `E171EE153CB1108BF5E84A224B09ECDFDA54FB6A6FE1CFDF07D9A4AEBFA7E3F4` |
| Shanmen.0_0_10.Items | 121／0 | 20261005T142307059Z-f2db0a32 | `F5D4309CDA78042E14AB64DE325F230D35C150E3BF276C2B15788A4EE59F65D8` |
| Shanmen.0_0_10.CombatCore | 9／0 | 20261005T142359162Z-bb52c208 | `8CE2ACC6EAFC9E08F115FF2F4216DA9302E9B6C44D1EB0858DA5E141DBE2C813` |
| Shanmen.0_0_10.CombatRuntime.BasicSword | 4／0 | 20261005T142431045Z-53f747ae | `3576D6A3551978EA4D3F501E81B43DA4A7F28C369D5C755F7586E6A1210976EB` |
| Shanmen.0_0_10.CombatRuntime.VitalityAuthority | 6／0 | 20261005T142502587Z-072bd4a0 | `06B65CA7C790310E94848EF01EDE4A1E8598C812EE1630730D6E6B19BD18D21F` |
| Shanmen.0_0_10.CombatRuntime.VitalityLedger | 4／0 | 20261005T142534145Z-8706a411 | `79525ED15DC85EF3F3C4800E30BC55CB8A32B42E4EE04C67A568B319649ACD82` |
| demo_map.ItemUseAndArmor | 46／0 | 20261005T142730500Z-61ab67e0 | `605242519DB254CA08EF25C8B4C692BA19979A54CDAFAABA488C6DBC7645E950` |
| demo_map.P4.Hotbar | 7／0 | 20261005T142806338Z-028d7e5e | `860339B51909BA3527E6A4BFE9BA092B2A223A98C1BC33CADFE686F4AFF40989` |

每组目标 Cmd 前仍有十三条 Engine 启动 `Condition failed`，不隐去或当作零 Error。目标用例无失败，本轮不改 Engine 或放宽门。没有崩溃、超时或退出码冒充成功；本轮不代替最终全部新旧测试根验收。

## 5. 新增用例与检查

新增 Demo20 五项 StackTransfer：Enemy／Chest／整备实际部分合并和 32 颗非零守恒；反向合并、拆入安全格、地面往返及死亡／撤离；merge／split 替换前后持久失败与精确重试；参与身份、见证、平衡字段、fingerprint 和出生元数据不一致拒绝；混合十颗药生命存盘失败后恢复只扣一颗。两项 Items 用例验证普通／特殊兼容与旧空回执原样重放。既有各组全部继续执行，没有仅挑新增用例。

边界扫描新 StackTransfer h／cpp 和 RepositoryGrid 未发现 demo_map／Engine include、UWorld／AActor 或随机调用。ShanmenItems.Build.cs 保持 Core／CoreUObject／GameplayTags／ShanmenCore、Json／JsonUtilities；既有 OpenSSL third-party helper 不作为 Engine 模块依赖。Source 与阶段文档 `git diff --check` 退出 0，五文档 67 个本地链接有效，113 项保护及固定 Source 哈希复核通过。八日志合并再次校验 `PASS Changed=8 Rules=2 Required=6 Logs=8`；不额外宣称八个必跑组。

## 6. 实际验收与保留限制

本轮零新窗口、输入和截图；真实拖放、余量提示、混合药、安全格、恢复、双分辨率和焦点检查列在 [ManualStackTransferReview](../Demo20/ManualStackTransferReview.md)，待人工，不阻塞独立开发。没有测真实帧耗时、进程内存或多轮对象释放，不把文件大小／GUID 数量当作内存实测。

现有回执多三项 GUID 和见证计算，仍有历史及候选复制成本。出生字段不是逐单位来源追踪；历史旧回执没有目标信息，本轮保留兼容，不伪造精细来源。既有文档 digest 与正式来源授权没有被见证取代；没有新增经济价值结算或随机词条。

## 7. 交付与目标状态

阶段文件为八 Source 加五文档，精确暂存、提交、非强制推送原分支后核对远程提交 SHA。原件保留在 Saved，不混入 Git；四份用户改动与历史 untracked 留原地，旧自动任务仍暂停。完整 Goal 保持活动，随机敌人组合、内容量、结算明细、安全恢复、完整实际循环和 M5 最终验收仍有工作，不能标完成。
