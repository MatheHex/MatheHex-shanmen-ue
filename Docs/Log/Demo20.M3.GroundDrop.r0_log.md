# 山门 Demo2.0 地面丢弃与拾回 Development Log

日期 2026-10-05；进入基线 `7bf21072b677236c153ef42549d2069cc6d5fa99`，原分支。实现范围见 [Report](../Report/Demo20.M3.GroundDrop.r0_report.md)。完整 Goal 保持活动，旧自动任务保持暂停。

## 1. 工作区和权限

读取完整 Goal、Git 状态与最新 Report／Log、来源／Run／物品权威、治疗和终局、UI、构建／映射脚本。Get-CimInstance 受限返回拒绝访问；随后只读 Get-Process 核对无 UnrealEditor／UnrealEditor-Cmd／dotnet，未终止或接管用户进程。上一轮已提交且远程一致，本轮基于当前代码继续，不重做已完成接管。

实时战斗交人工；本轮无 Editor UI、PIE、Standalone、产品 exe、代理输入或截图。113 项用户材料／原有存档保护核对，四份用户 tracked 修改和历史 untracked 保留。没有改真实档、自动任务、Config、Content、地图、脚本或旧库存 writer。

## 2. 实现输入

十九 Source：Items 的 Grid h、Types h／cpp、Repository h／cpp、RepositoryGrid cpp、RunGrid cpp、AuthorityService h／cpp、新 GroundDrop cpp；Demo20 的 World h／cpp、Expedition cpp、SourceWorld cpp、InventoryWidget cpp、LootTests cpp、新 GroundWorld cpp；既有 Subsystem h／cpp。五文档为 README、ExpeditionStatus、ManualGroundDropReview、Report、本 Log，共二十四文件。

整堆真实实例移动到派生地面容器，没有 caller 数量或第二库存；Owner／Scope／Run／revision、稳定请求和持久位置在同一回执绑定。所有数量与格子操作复用原权威，拾回不重新生成；地面内容终局丢失，拾回按实际普通／安全位置。新增 operation 尾部追加，Schema 6 和世界字段不变。

产品在脚下确认地面、正式持久命令成功后呈现无碰撞行囊；现有 UI 工具栏丢整堆，现有交互键打开真实地面格子。无 Tick 快照新增，终局移除 Actor 引用并销毁。普通药治疗参数未改。

最终输入清单如下（文件实际字节 SHA-256；不含用户文档和验证原件）。按表内顺序拼接 `Path + TAB + SHA256 + LF`、UTF-8 无 BOM 后计算的 manifest SHA 为 `85EB854FFBF8C01D46A676500A3B3606ECE278ACE9123935A3A356F11F372030`；最终构建前冻结，回归后和提交前复核。

| Source 路径 | SHA-256 |
| --- | --- |
| Source/demo_map/demo_mapShanmenItemAuthoritySubsystem.cpp | `56FB4BFDAB7EA98244BEA215F1C98A7C93088276F84D62DDDE92D67352F5BC58` |
| Source/demo_map/demo_mapShanmenItemAuthoritySubsystem.h | `959AB3F495CE69CF8934A7756B8A58D07C43CE95E246A0FCDF17D47E089C8561` |
| Source/demo_map/Demo20/ShanmenDemo20Expedition.cpp | `BB5698F00DF60FC24DF0F8186D977748B1E0855B630375350773E6BFBE564620` |
| Source/demo_map/Demo20/ShanmenDemo20GroundWorld.cpp | `537C24928C84E94AF26AC9BF7ED2D34210BA7DC8E9478528FA8E01F5A2E5CA31` |
| Source/demo_map/Demo20/ShanmenDemo20InventoryWidget.cpp | `D2728304CBCEA197E2186C36826A3C7919D0B0AEFC1A3BB7C102CD812525B1B6` |
| Source/demo_map/Demo20/ShanmenDemo20LootTests.cpp | `AA7FB5C915ADB495A5E6D875193517A49A4BA4D1BC57E22D164338FDC0ADA065` |
| Source/demo_map/Demo20/ShanmenDemo20SourceWorld.cpp | `7AD8C9C4A19CC6679D6099AF7EA39505D82EAA697EC8C15081322D60ED5DAE3E` |
| Source/demo_map/Demo20/ShanmenDemo20World.cpp | `52D790197F9C1C0153E2BC545DDA6D77E59EFEF57B15016494A45F94AD8B001A` |
| Source/demo_map/Demo20/ShanmenDemo20World.h | `8C25A04D9F92A1F846AE3D31763F4379B55B7EB1153623231D56CB412B7E0D6B` |
| Source/ShanmenItems/Private/ShanmenItemAuthorityService.cpp | `019AEFD9EC2BE1C754B9F05625D3C164F7F71D8E2965B415C95239012CC0981E` |
| Source/ShanmenItems/Private/ShanmenItemGroundDrop.cpp | `5D8810ECFD23EA4FE3301422F33FEF85C83C4B27EA841CF281D5C302F7A7AED3` |
| Source/ShanmenItems/Private/ShanmenItemRepository.cpp | `6983044C752B72A866F288694C3EABF14465F003BF7340D98E6D339300FEDF5E` |
| Source/ShanmenItems/Private/ShanmenItemRepositoryGrid.cpp | `AE28BB114D8AF1D1801BB1E2CB40728FADC0778DC6C21D3FC066CE1CF428DAE9` |
| Source/ShanmenItems/Private/ShanmenItemRunGrid.cpp | `54AB3F2F9A587F9C9C808B78C57D6C099D0732F5BADA4B7F198329DBB65E2A48` |
| Source/ShanmenItems/Private/ShanmenItemTypes.cpp | `C592A988954436024DC41E85FE90612D8A011FCF132722D1E1E6E9047244C162` |
| Source/ShanmenItems/Public/ShanmenItemAuthorityService.h | `666C5F299B89D685756AEE0C51BEEAFE6268A81BD5728AA5685EABA6FC92877F` |
| Source/ShanmenItems/Public/ShanmenItemGrid.h | `564A5680F1FAA75E57E7BB888722564C56171551188F7AB14D8BD7915081201F` |
| Source/ShanmenItems/Public/ShanmenItemRepository.h | `CCEAE9A924AC616FF545437A71105D7E79B39AD72C1C10F8699A5360FD87F3CC` |
| Source/ShanmenItems/Public/ShanmenItemTypes.h | `21408DD00480A821C4D4CA7B4EFB5AAB330E7B112D5D493FF6BD15D3913A54FC` |

## 3. 双目标构建

使用 `./Scripts/RunF0DevelopmentBuilds.ps1 -TaskId Demo20.M3.GroundDrop -AttemptId attempt-001`；已知默认受限环境 .NET 曾停滞，本轮直接请求编译权限，隐藏执行、不启动窗口。首次 Editor／Game 均原生 0。首次 Editor 完成后，代码复核补上地面投影失败同步暂停世界／刷新提示，以及读取失败停止交互后续来源路径；因此首次日志不作为最终输入证明。

最终十九源码冻结后同命令运行 `-AttemptId attempt-002`。两次外层脚本均 0，四目标均 `Result: Succeeded`、原生 0。002 Editor 等待已知 001 Game 的 Build.bat 锁，164.009 秒含等待，native 自报执行 10.35 秒；不把锁等待当编译失败、超时或源码错误。原件根为 `Saved/Automation/Demo20.M3.GroundDrop/Build/<attempt>`，每目标 `<target>-Win64-Development.stdout.log`、`.stderr.log`、`.result.json` 保留。

| Attempt／Target | 开始 UTC → 结束 UTC | 原生码 | stdout SHA-256 |
| --- | --- | ---: | --- |
| 001 Editor | 13:36:47 → 13:44:53 | 0 | `299073DFD3BD9CA0C8F962ACDEA92E074418A17335A30CCB6301D1649DF18362` |
| 001 Game | 13:44:53 → 13:52:48 | 0 | `0B2F04585B94DB84CA33C47FC648A50D61326B64E48FE508BB90002C17D03045` |
| 002 Editor（最终） | 13:50:14 → 13:52:58 | 0 | `C77059484EA139B2E27CF368C9D4E041D3682E36AD0F91C311DA80FB2DE3BBEB` |
| 002 Game（最终） | 13:52:58 → 13:53:11 | 0 | `3EEE7C25A48D80EEA6F9E0B557647673AEC8C8ECD97A6AFCB8D51352B0788090` |

四 stderr 均 0 字节，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。001 Editor／Game result.json SHA 依次 `9D7DBE770988926E2A8BFCB296764356CD54B16B2CF4F714F3002D52194E5C65`／`A6DAEB235CA3708FBC24C322E30E49FB29CC403ED35F5DB890935305B250B924`；002 依次 `A1601CD2E767D067773C118CDCFD527BCBCB977E4933DA29DEA05933F9AB1D4C`／`9184BC6BE116B62E3E0A68ADE9122275D6FA42AF5275A70C06A3C7A3801EEA15`。本轮没有失败构建原件被覆盖或丢弃。

## 4. 改动驱动回归

固定最终十九源码后使用 `./Scripts/Test-Demo20Grid.ps1 -TaskId Demo20.M3.GroundDrop.FinalRegression`。2026-10-05 UTC 13:53:41 → 13:58:53，外层脚本 0；八组 264 Success／0 Fail、264 个去重路径，每组 native 0、SUCCEEDED、RunTests 命令 1、结束记录 1、Fatal／Unhandled／Ensure 0。映射三规则 Demo20StandaloneSlice／Items／ItemProductAdapters，`REGRESSION_COVERAGE: PASS Changed=19 Rules=3 Required=8 Logs=8`。

原件根 `Saved/FoundationRuns/Demo20.M3.GroundDrop.FinalRegression/Automation-<Group>/<ID>/`；表中 SHA 对应该目录 `UnrealEditor.log`，同目录 run-state.json／stdout／stderr 保留。没有失败重跑，也未覆盖旧证据。

| Group | Success／Fail | 原件 ID | Log SHA-256 |
| --- | ---: | --- | --- |
| Shanmen.Demo20 | 69／0 | 20261005T135341116Z-83ef5122 | `064D3A5DA1ADD0751769774EC5409AD310469D50C0A95BE8C5335056D07D2F5B` |
| Shanmen.0_0_10.Items | 119／0 | 20261005T135441797Z-718af71e | `1B38C87D989CE137A7EC75A5DE240474C1AA45B00C5CADBA51F8EDA3FECE04D2` |
| demo_map.ItemUseAndArmor | 46／0 | 20261005T135532714Z-6d1e17e9 | `96D1A50604B502A2F303CFCF927A004C7DFF86D2F0A782FD19BEBEC447B8E36D` |
| demo_map.P4.Hotbar | 7／0 | 20261005T135608565Z-19a915f6 | `D6262CF84DD813A26BD26385A49C8D9B5E161627F192AC9E4AADB975724C3C74` |
| Shanmen.0_0_10.CombatCore | 9／0 | 20261005T135644064Z-a1f34a1f | `E369F4A411D3144F18AE40342A3BCAD752FBAFC0C162B9DDCE9596E565C5BB2C` |
| Shanmen.0_0_10.CombatRuntime.BasicSword | 4／0 | 20261005T135719480Z-db8bdd8c | `3626A2F79257ABE495F759CBB619096D3EB5973CE42A97431EA9896DC7B65298` |
| Shanmen.0_0_10.CombatRuntime.VitalityAuthority | 6／0 | 20261005T135751318Z-e3c2f7f1 | `1749C867A8B7348213DFC7024672AB511DB74A3879ADC49E581E1F064B049E25` |
| Shanmen.0_0_10.CombatRuntime.VitalityLedger | 4／0 | 20261005T135822949Z-40fe5183 | `DB41AE939E6CB7F0FA7EEDC850FC11CD77A0D18DE864C1D5A0AC685CCFA0B499` |

每组日志在目标 Cmd 前有十三条 `LogAutomationTest: Error: Condition failed`，均不属于目标结果；原件和上述哈希完整保留，不声称日志零 Error。只读本机 Engine `Runtime/Core/Tests/Experimental/UnifiedError/UnifiedErrorTests.cpp` 可见十三项固定英文文本比较；本机中文资源导致该现象是与既有观察一致的推断，不当作已证明的根因，未修改 Engine 或放宽映射门槛。

## 5. 测试与实际验收边界

七项新增原生用例使用隔离 native 文件，数量、位置和身份是实际权威而非内存假回执：原八颗往返、不新建 Item；安全格三颗丢出及终局损失，实际安全玉佩／仓库／钱包非零对照；目标九颗只接一颗、地面二颗保留；替换前回滚与替换后精确重读；作用域／位置／revision／回执篡改拒绝；预留用药关闭后才转移；旋转武器及下一局隔离。既有 GameInstance 用例扩展绑定端口。

原普通丹药人工反馈与旧证据保留，不计为地面拾回后的治疗、焦点、恢复与双分辨率通过。实际 [地面丢弃审查](../Demo20/ManualGroundDropReview.md) 可以稍后完成；不暂停独立代码开发，无头测试不代替画面或输入。

## 6. 成本与剩余范围

每次丢弃保持物品数不变，增加 64-slot 容器、布局和回执；空容器／规范位置名称及历史回执仍增长，单局 4096 安全上限拒绝后原物保持。UI 快照成本、真实内存、帧耗时和多局对象趋势未实测。未完成跨来源合并、随机敌人组合、目标内容量、结算明细、损坏记录修复和完整实际验收。

## 7. 收尾

最终十九源码哈希复核一致；五文档 61 个相对文件链接均存在，`git diff --check` 原生 0，113 项保护核对通过。Items 依赖／类型／RNG 扫描无 demo_map include、UWorld、AActor、FRandomStream、FMath::Rand 或 ApplyDamage；Build.cs 未增加 Engine／demo_map 模块依赖。宽文本扫描仅命中既有 Types.h 第 202 行说明“产品策略保留在 demo_map 边界”的注释，不把它误判为代码依赖，也不宣称整模块不存在该文字。源码不为修扫描改写。

交付只精确暂存二十四本阶段文件，索引集合和 cached diff 再核对；证据原件留本地 Saved，不纳入用户材料或存档。本报告所在阶段提交及远程 SHA 由交付消息给出；推送后实际核对远程一致，不将本地提交冒充已上传。完整 Goal 不标完成，旧自动任务不恢复。
