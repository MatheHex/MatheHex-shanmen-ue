# 山门 Demo2.0 持久结算明细 Development Log

日期 2026-10-05，进入基线 `dbd68cc337625aa9e70125dc0b9c8d46eb2ffa0c`。范围见 [Report](../Report/Demo20.M4.Settlement.r0_report.md)，人工步骤见 [结算审查](../Demo20/ManualSettlementReview.md)。完整 Goal 活动，旧自动任务暂停。

## 1. 工作区与范围

先读取完整 Goal、最新 Report／Log、Git／进程、物品生命周期和持久端口、实际网格、产品 UI／恢复以及构建和回归映射。进入没有 UE 或 dotnet；本轮仅隐藏双构建、隔离 UnrealEditor-Cmd，不操作窗口、不启动人工档、不截图。四份用户 tracked 修改、历史 untracked／存档按 113 项核对，不改变保护 manifest。

本轮同一 Item authority 增加 RunReport 审计，现有 UI 只读展示与最近结算入口；不改变物品实际余额、来源生成、治疗或战斗数值。Item Schema 6 → 7 是历史事实扩展，世界编码不改。不接第二库存、不修改暂停自动化、地图、Content、Config 或输入。

## 2. 固定输入

首版十六源码 manifest `64F6F3CA0F4640C3D73843B2CA6EFDEAC345112FA0DC6C68DFB1A5C7D8B41743` 对应 001 双构建。代码审查发现重启后只到整备、无法查看结算的入口缺口；完成首轮构建后补只读最近结算入口，限定场景遗留为本局容器、排除其他 scope 并行消费，并保留旧合法部分返还契约，再固定候选重新构建和回归。

002 十七源码 manifest `45A3D0151B5AE68F32C6C7849507AED6764A4A7C9EA66418E72CC86A02808411` 对应首次回归的 80／2；仅修新增测试误认安全格初始物与补给预期，生产不改。最终十七源码原字节 SHA 按下表顺序 `Path + TAB + SHA256 + LF`、UTF-8 无 BOM，manifest `AFCEAB7AF5DF017FDE1254F6CA3FBC34B17C9E50B8D7CFD79E7619C11A169F8A`。003 构建对应此输入，回归前采集，提交前复核，其后不改 Source。五阶段文档另计，不将用户四文档包含在阶段中。

| Source 路径 | SHA-256 |
| --- | --- |
| Source/demo_map/Demo20/ShanmenDemo20Expedition.cpp | `1C81E812D7A38DADAA32784770D655F1A8250F5ED44ED6A18032B7B75649BA7E` |
| Source/demo_map/Demo20/ShanmenDemo20LootTests.cpp | `D8CFA442517E536D8251E9F58F45C4CDFB56E314C96793B4C3ACFA4FD925CB64` |
| Source/demo_map/Demo20/ShanmenDemo20Settlement.cpp | `AAA1F736BEE7EC175D34DF7441A7B9026D67ADE4D8DE8ACC25312BEE3F2E8BD2` |
| Source/demo_map/Demo20/ShanmenDemo20Settlement.h | `102F8C349281ECA11D97CA0858473BF9490079E629948F7FF11D3FA17D7E8F2B` |
| Source/demo_map/Demo20/ShanmenDemo20Widget.cpp | `BBBE1EBCAB1127284FD75BD8C338C6AF8AAA8BD08FFFE11D0703E7D1D9A6EC37` |
| Source/demo_map/Demo20/ShanmenDemo20Widget.h | `73258C6EFF9D33EAEB3D4174E7A104CA75D0F564D8FED5AE284E1C6348AA35D8` |
| Source/demo_map/Demo20/ShanmenDemo20World.h | `B3D0634D8827EA50EE7AECFB9F700FDA640D05D7DBA5E552412F0B174EAC1653` |
| Source/ShanmenItems/Private/ShanmenItemPersistence.cpp | `05952D3F5A17BE23201C8E3EE416E3B57196F40FE42C260C60646EBF7D2976F9` |
| Source/ShanmenItems/Private/ShanmenItemRepository.cpp | `ABC9692ADAE3AB58695BBAC98718C0E98EB4A44D8A11390ED24B1918C70F45C0` |
| Source/ShanmenItems/Private/ShanmenItemRepositoryGrid.cpp | `0B52D08DE1879CFD4C84027E656AFF6C563B0C6289813C02D1DF796A61B4B5A5` |
| Source/ShanmenItems/Private/ShanmenItemRunReport.cpp | `26C8DD7A635BDFD41C3F19059181A8112996266BE1088D31F5DF639F037941F8` |
| Source/ShanmenItems/Private/ShanmenItemTypes.cpp | `2A6BE1F9C0DE46B51B569B61A5926603CA43CC12596DF5EC655BA1E6B104B047` |
| Source/ShanmenItems/Private/Tests/ShanmenItemGridTests.cpp | `5B54EBF89D2196C30FBB2514479064E3D67A3F5F7A73B1B7988687E86D9881D6` |
| Source/ShanmenItems/Private/Tests/ShanmenItemPersistenceTests.cpp | `2A945FBEC32EB2ED4BD94DB25D9B8226E7B33EC355FF1C28F0A6E262AA5D016B` |
| Source/ShanmenItems/Public/ShanmenItemPersistence.h | `CC282252EA0742028ED2680EE523687227997CBBD0A54D5A9237254774D169A0` |
| Source/ShanmenItems/Public/ShanmenItemRepository.h | `CA2F5F404747ACF80922AFC7098EDEC978C9F4FC6AB842DC32CD79DC336D58A1` |
| Source/ShanmenItems/Public/ShanmenItemTypes.h | `D59A7B1F3DA4CD997D777AC26ABCFBC5695F12EEA20E6CF420888B065B0FD14B` |

## 3. 双目标构建

原件 `Saved/Automation/Demo20.M4.Settlement/Build/<Attempt>`，执行 `./Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M4.Settlement -AttemptId attempt-001`，之后同脚本 002／003。隐藏、顺序 Editor／Game、WaitMutex／NoUBA／并行 2。三轮各目标 native 0、外层 0，无构建失败；不覆盖尝试目录。表中 stem 后缀 stdout.log／stderr.log／result.json。

001 Editor UTC 15:31:20.431 → 15:39:50.215，509.783 秒；Game 15:39:50.241 → 15:48:02.786，492.545 秒。共享结构触发 362／359 action，不用构建耗时推断游戏帧率。002 Editor UTC 15:51:03.270 → 15:51:34.610，31.341 秒；Game 15:51:34.630 → 15:52:08.000，33.370 秒，15／12 action。003 最终 Editor UTC 15:55:50.203 → 15:56:05.096，14.893 秒；Game 15:56:05.116 → 15:56:22.959，17.843 秒。

| Attempt／Target | stdout SHA-256 | stderr SHA-256 | result SHA-256 |
| --- | --- | --- | --- |
| 001 demo_map | `55AA48443675377357FC8F3852E2DEC3E75E74B2229178796A73FF37FF180A8A` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `A196BDA40DE8D37E52A2F0B97FEB546E7F8A4365CAA4A5A5BF95F0A76E5CA90A` |
| 001 demo_mapEditor | `ACB19F0174A988AEABF2411F2167FA7B2E9F37196B77EE2C03F6042F0B1D2725` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `0FEF8BEF82EA0124709318096B61AD071D4174A11E6D508FB2F9AFF1CECB7D7B` |
| 002 demo_map | `66A1F7D030107FE5E1AB38766E444580FAC147DB32BC48A409EF57629F3DE211` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `E88AFDD25B396D41A09ACFBD9B7E29356B0EDC6F25D87ED97EC2A72952787DF8` |
| 002 demo_mapEditor | `C0E0ECEAB1521AED1B69802D5C86945BBAE1DE57BF7ADFBF0CE040E6BDE3DC76` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `60A566D08F06D0809F637F77B810D627E0791C42F75C0F551B078DD4DE659658` |
| 003 demo_map | `37325605AAFDA480FEBA70BFCEFEFDCDC628C37A61CFD632D5A95182BB765097` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `03C0F0946FA8EAA9DA379B796732CDD86F5762CEEE0BC1A0EF9A1241195E07D0` |
| 003 demo_mapEditor | `C088BC5F6B443215EFDE7F31BAB3795EA5852187C2CC85EE9BBB3595D00FA378` | `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855` | `B02ABD0934A79AF42600FDE072349D4D365CC2D65D77FD66F4980825B07D4BAE` |

## 4. 验证与首次失败

本轮新增四项非零真实领取／消费／安全格／混堆／地面／死亡／撤离／原生重启、失败注入、闭合历史不可重写、旧部分返还／下局历史选择，以及 Schema 6 原生非零活动局兼容。旧局不补造统计；下局从成功携带起点记录。

首次 `Test-Demo20Grid.ps1 -TaskId Demo20.M4.Settlement.FinalRegression` 为 Demo20 80 Success／2 Fail、Queue 82 performed；UTC 15:52:44.253 → 15:53:54.870，native 0／run-state SUCCEEDED 只表示进程正常退出，回归脚本因 Fail 外层 1，余下五组未跑，不计映射通过。原件 `Saved/FoundationRuns/Demo20.M4.Settlement.FinalRegression/Automation-Shanmen.Demo20/20261005T155244215Z-62ce121f/UnrealEditor.log` SHA `967203FC793CB9CE35FBD32F43D3FC28658CC9BE13EC97CAB7FF28464BC32033`，同目录 run-state 和日志尾部保留。

失败为 `Settlement.LegacyPartialReturnAndNextRunHistory` 和 `Settlement.MixedPartialConsumedTerminalAndRestart`：新测试误把初始安全格的一枚灵玉当作两颗丹药，导致带回／保留期望多二；仍有四颗安全药和仓库替代装备时补给按既有规则应拒绝 BasicSupplyNotNeeded，不应发放。只修测试输入／预期，另断言真实灵玉保留，并在药全部损失的实际死亡用例证明有限两药补给不改历史；不删除断言、不修改目录、药数或补给规则。003 重新双构建后跑全部映射组。

## 5. 最终映射回归

003 后执行 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M4.Settlement.FinalRegression.r1`，UTC 15:56:49.457 → 16:00:49.623。十七源码匹配两规则、六必跑组，226 Success／0 Fail、226 个去重路径；native 均 0、外层 0，`REGRESSION_COVERAGE: PASS Changed=17 Rules=2 Required=6 Logs=6`。不把首轮 80 次成功混入最终数字，不按主题裁剪映射组。

原件 `Saved/FoundationRuns/Demo20.M4.Settlement.FinalRegression.r1/Automation-<Group>/<Run>/UnrealEditor.log`，同目录 run-state.json 全部 SUCCEEDED／native 0；各 Cmd 1、Queue performed 1、Fatal／Unhandled／Ensure 0。每组 Cmd 前 Engine 启动仍有十三条 Condition failed，未隐藏，不声称零 Error，不改 Engine。

| Group | Success／Fail | Run | UnrealEditor.log SHA-256 |
| --- | --- | --- | --- |
| Shanmen.Demo20 | 82／0 | 20261005T155649428Z-d0dcf163 | `A6FE8B653B76C03AD9BFCFF5C6821846208A91ABA7F6AAFBA56481469E28F345` |
| Shanmen.0_0_10.Items | 121／0 | 20261005T155745062Z-ff0dacb2 | `EEE77E65B44D421E5DF6C9CDB91591018683F3BAD9B79B36698B44988EB13B5F` |
| Shanmen.0_0_10.CombatCore | 9／0 | 20261005T155835810Z-7d237bee | `199BFDF76B1A548AE1B1009F203639E74A4234FD72B49D4DC987E9C645D396EB` |
| Shanmen.0_0_10.CombatRuntime.BasicSword | 4／0 | 20261005T155907419Z-2207c811 | `1DCCDDF4C26FD2C411D3F7FC68FB2C91CDEB0DD8E58AB18F31D417A7E5D0987D` |
| Shanmen.0_0_10.CombatRuntime.VitalityAuthority | 6／0 | 20261005T155938727Z-5f87a1f0 | `BC35D205E9222E1B4FCAFFC3165BD13F3545FB6EBD8EA0D2E1215537ACB89728` |
| Shanmen.0_0_10.CombatRuntime.VitalityLedger | 4／0 | 20261005T160014180Z-2d43056a | `FA17086E34A38DD91E737A811F24237A8718D2EDBAF6BB603653438EA0E668B8` |

共享快照与存档扩展额外运行新旧 `Shanmen`／`demo_map` 全根，使用相同固定 Source、Entry／NullRHI／NoSound 与隔离 UserDir、隐藏原生进程。此批次 `Demo20.M4.Settlement.FullRoots` 在本代码候选发布时仍运行，不是最终成功证据：`Shanmen` 发现 1549 项，最近核对的完成记录为 UTC 16:17:16.122，已见 932 Success／0 Fail，但没有 Queue performed、最终 run-state、native 或外层退出码；`demo_map` 尚未开始。原始增长中日志为 `Saved/FoundationRuns/Demo20.M4.Settlement.FullRoots/Automation-Shanmen/20261005T160157647Z-4d2c6ff6/UnrealEditor.log`，暂不发布最终 SHA。

六组映射的 226 次执行与全根存在重叠，不能累加成去重用例数。补充全根继续后台执行，不取消或删首次结果；后续闭合时另核对 native／outer／结束记录、全部失败及最终 SHA。部分深层产品恢复用例耗时较长，不修改测试输入、实现或成功判据来加速，不以此耗时推断实际 Demo 性能。此批次未结束以及本增量未实测窗口的边界均保留，不冒称完整 M5 验收。

## 6. 检查与实际验收

阶段 Source／文档 diff check 0；结算领域和展示计算 h／cpp 扫描无 demo_map 依赖、UWorld／AActor／ApplyDamage／全局 RNG／NewGuid／时钟，展示仍在既有产品模块，不另称新框架。五文档 82 个本地链接有效。交付前再次验证十七 Source 原字节与固定 manifest 完全匹配、113 项保护全部通过、进入基线未移动及 index 为空；阶段只精确暂存十七源码和五文档，不包含用户既有修改。没有新实际窗口或截图；普通丹药先前人工通过不扩展为本轮结算通过。报告增长、临时复制与校验有成本，真实帧耗时／内存／对象释放未测；限高滚动和返回／重试的可见效果交人工。

## 7. 交付与目标状态

本轮只推进持久结算明细及重启后的只读访问；发布的是已完成编码、双构建和全部文件映射回归的阶段代码候选，补充全根不计通过。完整内容量、损坏世界安全恢复、真实循环、双分辨率、PIE／Standalone 和性能仍有工作，Goal 不标完成。阶段精确提交、非强制推送原分支并核对远程；Saved 和用户文件留原地，旧自动任务不恢复，不打包或接管窗口。补充原生进程仍运行时不要另开 UE 或并发构建，先收取结果，再固定下一候选。
