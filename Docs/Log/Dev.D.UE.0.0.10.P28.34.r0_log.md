# Dev.D.UE.0.0.10.P28.34.r0 Development Log

## 1. 基线和范围

- UTC 2026-09-22；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 父提交 `3a3536c2837ccb3763f9289f3c668666cf8a96c2`，origin 为 MatheHex/MatheHex-shanmen-ue。
- 开始时复核 P28.33 Report/Log、P/F 基线与 Published 验证：1623 输入、103 用户未跟踪、2 份用户已修改文档、索引为空；上一阶段证据核对通过。
- 本轮只改库存奖励元数据存档和 schema 兼容；Report 见 [P28.34](../Report/Dev.D.UE.0.0.10.P28.34.r0_report.md)。没有源内容资产、地图、输入或 UI 变化。

## 2. 真实失败和修正顺序

1. **Red**：仅加入大整数存档回归，Editor 4 actions、原生 0；`ExactMetadataRed` 0 Success / 1 Fail，2 个发布断言失败，正常结束队列 1，原生 0。该 0 不是测试通过。测试当时源码 SHA 为 `5BE50FC5010B381FAE01A936289292EF6494F8CE5C8A451F8B2A729268B3F323`。
2. **ExactMetadata**：实现 schema 4、共用 metadata codec、schema 3 旧摘要兼容和三个注册测试。Editor 233 actions、Game 230 actions，原生均 0。构建期间修正两处残留版本诊断/注释，旧输入锁与当前两个文件不符；外层门退出 1，未开始测试，不作为最终证据。
3. **Final**：重新固定输入；Editor 230 actions、Game 16 actions、原生均 0；存档专项 10 Success / 2 Fail，队列 12、原生 0。Schema2NonzeroMigration 与 Schema3LiveHistoryMigration 的夹具摘要不一致，外层门退出 1，阻止后续测试。
4. **Diagnose**：临时输出独立反射 JSON 与旧夹具 JSON；Editor 4 actions、原生 0，单测 0/1、队列 1、原生 0。确认差异为夹具外层 `rewardMetadata` 被替换成 `RewardMetadata`；旧规范序列化摘要对字节敏感。仅修正两处夹具键拼写并移除临时诊断，没有放宽生产校验。
5. **Verified**：重新锁定最终代码后双构建和回归；前述失败/中间目录及输入锁均保留，未覆盖或挑选中间成功用例抵消失败。

## 3. 最终验证

Verified Editor 实际 4 actions、Game 实际 3 actions，原生均 0；PersistenceFocused 12/0、ItemsRegression 103/0、LegacyFullRoot 1330/0，结束队列分别 12/103/1330，原生均 0，唯一测试名计数一致。独立成功 1433，专项 12 是 Items 子集。三份无头日志各有 13 条既存启动 Condition failed 输出，不宣称零 Error；注册 Fail/Fatal/Ensure/Unhandled/Assertion 为 0。

额外 ShanmenFullRoot 仍在执行，**不计作已通过**，最近完整根仍为 P28.29 的历史证据。此次公共持久头变更后主动加跑全根；核查旧日志得知此前一次需约 112 分 41 秒，不中止或拼接当前长测。改动映射本轮强制 Items 组，已完整通过，因此本阶段按已完成的局部契约交付，扩展全根另行接续验收。

命令入口使用现有 `Scripts/Invoke-Shanmen.ps1 -Action BuildBoth -MaxParallelActions 4`。无头测试通过 `Shanmen.Foundation.psm1` 的受追踪进程入口启动 `UnrealEditor-Cmd`，参数为 `-Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile`、对应 Automation RunTests 组和队列为空退出；从未启动产品 exe 或 Editor UI。Game 输出 exe 仅为编译产物。

最终输入锁 1623 项；与 P28.33 相比恰好 5 个原源码变更、1618 项未变，没有新增源码。每组之前/之后或交付核对检查文件 SHA，最终所有输入保持。映射按八个实际交付路径推导，强制 `Shanmen.0_0_10.Items`，真实整组日志覆盖；覆盖器自检 537/537。codec/来源 Repository 边界扫描无 demo_map、UWorld、AActor、随机身份或 RNG，模块构建依赖未添加 Engine。

## 4. 精确改动范围

- `Source/ShanmenItems/Public/ShanmenItemGeneratedSourceCodec.h` 与对应 Private 实现：公开同一 metadata 布局的无损读写入口。
- `Source/ShanmenItems/Public/ShanmenItemPersistence.h` 与对应 Private 实现：schema 4、自定义 metadata 编解码、schema 1/2/3 原摘要验证和首次迁移来源匹配。
- `Source/ShanmenItems/Private/Tests/ShanmenItemPersistenceTests.cpp`：三个新增注册测试；旧 schema 2 夹具恢复真实字段格式，重复键测试用当前版本常量。
- `Docs/Report/Dev.D.UE.0.0.10.P28.34.r0_report.md`、本文、`Docs/Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md`。

生产合计 123 新增/15 删除；测试 170 新增/1 删除。五源码加三文档共八路径精确暂存。辅助运行脚本与原始运行目录留在 Saved，不上传原始日志；GitHub 上的本 Development Log 提供原件位置和 SHA，不宣称原件已在 GitHub。

## 5. 用户文件保护与发布核对

103 个原未跟踪文件逐一按上一输入锁的路径与 SHA 保持，不重新归类为本轮文件。下列用户修改保持原样，明确排除提交：

- `Docs/Report/Dev.D.UE.0.0.10.OverallReadiness.r0_report.md`：`3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`。
- `Docs/Log/Dev.D.UE.0.0.10.OverallReadiness.r0_log.md`：`A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。

发布门核对八路径集合、父提交、分支、原始 SHA、完整结果/队列/原生状态、改动映射、文档链接及工作区/暂存 diff --check；提交后再次核对 commit 文件集合和用户文件，普通非强制 push，远端分支 SHA 必须等于本地 HEAD。最终提交身份由包含本文件的 Git commit 给出，不在自身内容嵌入自引用 SHA。

## 6. 原始证据索引

- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Diagnose/BuildEditor/20260922T004340114Z-298e85f7/run-state.json`，SHA `5F4E244937F689BE7984BCCE7AA8E1C0B4CBF98153E849C9E973F10789637ECB`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Diagnose/BuildEditor/20260922T004340114Z-298e85f7/stdout.log`，SHA `BBA0DDB95A23EDA14B5ABFA4A5EA6D51E42565838BCBEE845A2CF5EA642CCBEA`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Diagnose/Schema2Diagnosis/20260922T004345478Z-91a72301/run-state.json`，SHA `DBFCE04BD2E70D14EE8826623BFCECBC7AD234AC39FC397FD0A0A0F0C0D87121`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Diagnose/Schema2Diagnosis/20260922T004345478Z-91a72301/UnrealEditor.log`，SHA `634B8A24812105BF0A8FE560904BBB00613ED06F4BED0812BC92DC4EECDA70A2`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.ExactMetadata/BuildEditor/20260922T003312649Z-5958c821/run-state.json`，SHA `AB502426D3DD200220C5A62226A75CB7C488781911931CD8224AE7A40BABF3B9`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.ExactMetadata/BuildEditor/20260922T003312649Z-5958c821/stdout.log`，SHA `3D473D1C9E26D47B38504D387CE371660CFE0DB7A99452D9C446D521920E2E62`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.ExactMetadata/BuildGame/20260922T003600072Z-caead802/run-state.json`，SHA `F448B856348791AFAEA06B69D05C49403EBEAE313ACCD11ABFB4F71565263F35`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.ExactMetadata/BuildGame/20260922T003600072Z-caead802/stdout.log`，SHA `A4F3BF74030B2D2BF9209816A8221BA3A8AABBA70E324BBC5C7325F3CD26D777`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Final/BuildEditor/20260922T003904093Z-f08e088a/run-state.json`，SHA `12D0A8797A5E661C35D01121C5B8D648929F8EFC3C426AA8FC4C83CC951D2C4E`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Final/BuildEditor/20260922T003904093Z-f08e088a/stdout.log`，SHA `16126D3467C651F0130310B78BBF40265E1BDAF42D87C24C734FC480729F496D`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Final/BuildGame/20260922T004147495Z-ab6690a3/run-state.json`，SHA `C093636E31312AA97526E2542BB6FB2B1927BB561A6AD8A14A955910D95CCA5C`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Final/BuildGame/20260922T004147495Z-ab6690a3/stdout.log`，SHA `4367555AAE0FED2C5CF470C3B18AD2F15DDAD75A9EF57DA7C6C7C22DDB4A215F`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Final/PersistenceFocused/20260922T004210477Z-5bda204d/run-state.json`，SHA `4437B9ACF207BD0860679CB3F4F50A53335A628FE29AB541C6E9C471EFA62831`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Final/PersistenceFocused/20260922T004210477Z-5bda204d/UnrealEditor.log`，SHA `3BE79DECE766C37B5DC3870C6EC5D1AC78913807DA72F0FE7D29CF56B068C029`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Red/BuildEditor/20260922T002813368Z-2ed8f532/run-state.json`，SHA `651D8D2AA05D97B650EBD138CA56BCBED906DA23FD137B6ED5FC1E815099B3AB`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Red/BuildEditor/20260922T002813368Z-2ed8f532/stdout.log`，SHA `63BC40B75A4E0082E888EB1FB180F1ECD04B07992FC513A34668140CFC29216E`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Red/ExactMetadataRed/20260922T002831772Z-9d97e427/run-state.json`，SHA `82D0474D44644565D8DADCADB7C75DD61C1400518591C662A29CCA48E68E87E6`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Red/ExactMetadataRed/20260922T002831772Z-9d97e427/UnrealEditor.log`，SHA `8BA7109E9C67623CAB611B3DFFB9162F3E13FADF4A77800CDA49AF634681428F`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/BuildEditor/20260922T004527591Z-f36bcb70/run-state.json`，SHA `DB8EAC8E8BFDDCD1B108511197A4C77B850F23C9809CBB55B02317171F46842F`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/BuildEditor/20260922T004527591Z-f36bcb70/stdout.log`，SHA `E3F5DDFC01692D9315D489ADC109FE150B103BA72CA7C2BDB47010AB6FD11768`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/BuildGame/20260922T004532888Z-c5704ca5/run-state.json`，SHA `631CEDC69E2FEDE4CBBF84439015537C6F7E3E20C9F7B70F554486CEDF9A026F`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/BuildGame/20260922T004532888Z-c5704ca5/stdout.log`，SHA `5904ACE629F931D0F0830A83F80B0F4C1E6CEC5A1F06BB3260DBEC2EC141E793`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/ItemsRegression/20260922T004606004Z-78d8d540/run-state.json`，SHA `B93A274750405D7DCDB29558EFD9E8ED9B07D66B6D905850D0306E01237E4A48`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/ItemsRegression/20260922T004606004Z-78d8d540/UnrealEditor.log`，SHA `522F9A012036020D562024C6D9B9E219245B2CC2B3D70387D21B4D5B4132FC04`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/LegacyFullRoot/20260922T004636743Z-957a8899/run-state.json`，SHA `F911683C01C03154DC4D511C25DB1982CD1E42C2783309FBF03F23B74CF71617`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/LegacyFullRoot/20260922T004636743Z-957a8899/UnrealEditor.log`，SHA `BA127CFF9CE2B97F7DCBD64134FF2F366FA4CE12A04DE66BCDF1DE5FF440F774`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/PersistenceFocused/20260922T004545348Z-b9cc2d95/run-state.json`，SHA `CA73DAE28553D1D9E8B78E84E89619CABF87173B51515C979F58E4C3F12D9A2B`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/PersistenceFocused/20260922T004545348Z-b9cc2d95/UnrealEditor.log`，SHA `BD35572544BDE4D2D7289602E5493677D13BBA2D01FF55B7774DEC6584079787`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.34/validation-inputs-ExactMetadata.json`，SHA `E8F9ED4ECFCEB5B16D9AAD2E3AF3E1D849BE26D92F2BCCBF24A8A22CFC38BE59`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.34/validation-inputs-Final.json`，SHA `B2D5EB153C0020B669D38DA6426CD8144F18408C3FE96B679E31F31633614ECF`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.34/validation-inputs-Verified.json`，SHA `A495AADEA4E512204119D3AD67407F574AC80D0FE51F157CD8C0D6F3E68B7156`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.34/regression-coverage.log`，SHA `4171B06C68D30FE30A3079C0D739B7BAC2EEF96D9D84F25F852FDCAA559B2065`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.34/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。

## 7. 边界与后续

本轮闭合 P28.33 已记录的库存奖励大整数格式缺口，保持同一权威文档。FZ-1 产品来源授权/目录接纳/物化/获物和消费终局组合尚未闭合，FZ-2/3 仍开放；已完成的局部契约回归不能替代这些功能契约的完成证明。内存峰值、吞吐和整个 World 跨进程恢复不在本次成功声明内。按 [P 阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md) 继续底层工作，不进入 F、不暂停为“已完成”。

## 8. 后台完整根交接（优先于新增源码）

- 唯一运行：`Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.34.r0.Verified/ShanmenFullRoot/20260922T004752811Z-20fbf70c/`，开始 UTC 00:47:52。当前进程 PID 51080，命令为 UnrealEditor-Cmd 的 `Automation RunTests Shanmen`；只按 PID 加完整命令行/日志路径共同辨识，不凭 PID 操作其他进程。
- 当前外层执行会话 18881；可取回完成输出，或读取该目录完成后的 run-state.json 与 UnrealEditor.log。执行脚本还会在进程结束后检查最终输入锁。不得重复启动、不拼接重试日志、不把中途成功计数作通过。
- 最终必须核对原生 0、精确唯一 1448 Success、0 Fail、唯一队列 1448、无 Fatal/Ensure/Unhandled/Assertion，并复核 Verified 输入锁。通过后新旧两根独立合计应为 2778，103 Items 和12专项均为子集；这是**待验条件，不是当前结果**。
- 完成前只允许证据整理，不更改固定输入中的源码/配置/脚本。完成后另行报告结果与最终 SHA，再继续下一最小结构缺口；该长测不应使自动化越过 P/F 边界。
