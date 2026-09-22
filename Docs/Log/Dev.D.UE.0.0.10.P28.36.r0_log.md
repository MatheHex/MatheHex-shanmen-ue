# Dev.D.UE.0.0.10.P28.36.r0 Development Log

## 1. 基线与范围

- UTC 2026-09-22；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 父提交 `91b4a666e4c49bc7b7d667b8c9b3f43217260089`。读取 P28.35 Report/Log、P阶段基线、冻结索引、Git状态；前阶段 Published 检查通过。13:51续接后重新确认同一父提交、原用户改动和空暂存，没有重复前阶段已结束测试。
- 初始103个用户未跟踪文件、2份 OverallReadiness 修改。只闭合来源同锁读取边界，不修改产品生成入口。成果见 [Report](../Report/Dev.D.UE.0.0.10.P28.36.r0_report.md)。

## 2. 实施与代码量

六个既有Source路径：GeneratedSource.h定义值结果/三态；Repository.h与AuthorityService.h声明ReadGeneratedSource；RepositoryGeneratedSource.cpp从已验证历史推导Run归属、终局、当前顺序/保底/内容并重建指定原回执；AuthorityService.cpp在原Mutex内检查Ready/Storage Owner并读取；AuthorityServiceTests.cpp新增2个注册用例并扩展原八线程同来源用例。

源码213新增/1删除，其中生产83新增、测试130新增/1删除。没有新增writer/索引/schema/持久字段/故障注入口；旧TryGet行为不变。首次SourceContent保持未绑定；查询值不是写入许可，原顺序校验和磁盘代次检查仍负责拒绝读取后的竞争。两个新测试使用非零顺序/保底和完整计划/文件字节基准，覆盖Abandon后重启、陈旧提交、保存回滚、不确定提交重开及RecoveryRequired；不是玩家流程或不同来源吞吐验证。

查询扫描现有请求/来源历史，仅返回指定的一份计划，不全量拷贝库存图。仍有随历史增长的扫描和回执验证成本；本轮未测RAM峰值/锁时间，不宣称常数时间、零分配或性能验收。

## 3. 原生执行及首次不完整证据

唯一输入锁 Initial：1628路径（Source1097、Content485、Scripts40、Config5、uproject1）；对比前阶段只有本轮6个Source内容改变。构建前、各次完整测试后及交付检查核对路径集合与SHA；重跑沿用同一锁，没有源码变更或重构建。

通过原 `Scripts/Invoke-Shanmen.ps1 -Action BuildBoth`，Development Win64、MaxParallelActions=4：Editor UTC13:55:38.900—13:58:59.726，239actions；Game13:58:59.749—14:02:27.297，236actions；均Succeeded/原生0，是实际编译。没有运行游戏程序。

四次独立UnrealEditor-Cmd使用 `-Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile` 和单组 `Automation RunTests`；`-TestExit=Automation Test Queue Empty`。原件各有唯一目录/run-state：

| 目录 / Action / 组 | 唯一Success / Fail | 结束队列 | 原生 / 证据结论 |
|---|---|---|---|
| Initial / ReadFocused / Shanmen.0_0_10.Items.GeneratedSource.Read | 2 / 0 | 2 | 0 / 通过 |
| Initial / ItemsRegression / Shanmen.0_0_10.Items | 108 / 0 | 缺失 | 0 / 不完整，不计通过 |
| Recovery1 / ItemsRegression / Shanmen.0_0_10.Items | 108 / 0 | 108 | 0 / 通过 |
| Recovery1 / LegacyFullRoot / demo_map | 1330 / 0 | 1330 | 0 / 通过 |

首轮Items在UTC14:03:18.893—14:03:49.288执行，原生退出0、run-state记为SUCCEEDED，但UnrealEditor.log停在末用例EndEvents，没有队列结束/正常退出尾段。stdout/stderr也不补足此证据；外层run.ps1证据门退出1，Initial旧根尚未启动。未发现注册测试Fail/Fatal/Ensure/Unhandled Exception/Assertion failed，缺尾段原因未确定，不能把它当成源码失败、超时或已修复故障。保留全部原件，不清洗、不覆盖、不放宽门。

Recovery1从UTC14:05:26.785开始，只重跑Items，再运行旧根，于14:07:13.813回传完整检查成功、外层退出0。两个日志各有唯一带计数的正常结束队列和正确测试组调用。独立成功1438=108+1330，ReadFocused2为Items子集；首轮108不重复计数。所有UE进程已结束。

四份测试日志各有13条既存启动 `LogAutomationTest: Error: Condition failed`，不归入注册用例Fail；除此无本轮注册Fail/Fatal/Ensure/Unhandled Exception/Assertion failed/HTTP request timed out。原日志照存，不称“零Error”。新API没有旧实现RedProof；本轮首次不完整证据亦非RedProof。

## 4. 原始证据 SHA-256

原件保存在本地Saved，不纳入Git；GitHub交付的是Report/Log及证据索引，不声称包含以下原始日志。16条路径/哈希在最终检查中逐项比对。

- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Initial/BuildEditor/20260922T135538849Z-03d1a65d/stdout.log`，SHA `59895E0085EE426CC231DA7D346415BB78A1A33EF71B1F532149EFDB3EB5437C`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Initial/BuildEditor/20260922T135538849Z-03d1a65d/run-state.json`，SHA `B9F1907C2CD579641064AFF1914438827D34CF60467DD1F4DC0859C65E55CB51`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Initial/BuildGame/20260922T135859745Z-27dc5a08/stdout.log`，SHA `2E6EA4F4D52BAB690CACB89FD3C34542D64EBD4E151FD6AC4245DC72DB4F72CA`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Initial/BuildGame/20260922T135859745Z-27dc5a08/run-state.json`，SHA `AD91AF42674159059AF6A43C0547CEE990E1342E5194B087F210D35341F1C025`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Initial/ReadFocused/20260922T140227977Z-c885e882/UnrealEditor.log`，SHA `35D22A21B09D285F52321A7F31CDE40D5BDCC20FB8F8DD64D6D70745B2FFE12B`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Initial/ReadFocused/20260922T140227977Z-c885e882/run-state.json`，SHA `2FE17146A170493802ABCEF986BE1035A35A21AB681223E4FDAE0158F1522ABC`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Initial/ItemsRegression/20260922T140318890Z-a78aa818/UnrealEditor.log`，SHA `AA41DBF177CDE4CAD29F3149BCA4A4B3E4BB1B4B71A2BA35EE6D8090E97C1DEA`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Initial/ItemsRegression/20260922T140318890Z-a78aa818/run-state.json`，SHA `DEFD223EFA4A8E76B8AD5C5F179BAEB567AB3B7B9F2D1D403CCD84D68A9191A4`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Recovery1/ItemsRegression/20260922T140526742Z-f210ae53/UnrealEditor.log`，SHA `B4C5BB4B95994959192B1EF654D8388C9E1EE39AEB675DA69486B03691D51E19`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Recovery1/ItemsRegression/20260922T140526742Z-f210ae53/run-state.json`，SHA `1F051B9F5C46AAEFF8AFA8B642F7C7C2025C86577BC0CB709349CC410CC9B261`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Recovery1/LegacyFullRoot/20260922T140557751Z-45e076be/UnrealEditor.log`，SHA `F3AA53FDD4C576E02FE64A437B212A550BE628719561EFBA62683CABE4BABE7F`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.36.r0.Recovery1/LegacyFullRoot/20260922T140557751Z-45e076be/run-state.json`，SHA `6EA5006B27FDA2EC352E7DD6AF080138CEA05A102286B7CB4EEEF05994DC7CE8`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.36/validation-inputs-Initial.json`，SHA `1E4F8239E34EFE4FD655EA83D32A4A7A47120B8D9129F005388ED1FB26A8ED4D`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.36/coverage-Initial.log`，SHA `F186EEFD72CF32710FC5BA9689741D6F0C2313880C77AC34D85792E4F643D6C1`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.36/regression-selftest-Initial.log`，SHA `27C66F1B346956EB9E16A6CC798E763AEA6EEE484FA7986FDE75814BFB50B6C7`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.36/boundary-Initial.log`，SHA `BE1B37900EEE0CCEFAF31B31737C36C47E268F95C45B8CA02D1AA25CC831C2C7`。

## 5. 回归与交付检查

本阶段9路径应用既有ShanmenItems映射，要求完整Items组；输入Recovery1的Items和旧根日志，结果 `REGRESSION_COVERAGE: PASS Changed=9 Rules=1 Required=1 Logs=2`。映射脚本及规则未改，重新执行自检549/549通过。不以专项替代完整Items，不把旧版本完整Shanmen根算成本轮结果。

核心ShanmenCore/CombatCore/Items扫描无demo_map include、UWorld/AActor、ApplyDamage、直接RNG；Build.cs无Engine/demo_map依赖。本次读取实现不新增存取盘/生成器调用。交付检查核对1628输入路径及SHA、103用户文件、2受保护文档、16证据哈希、文档相对链接、diff --check、准确9路径暂存/提交与无残留UE进程。

## 6. 用户工作保护与发布

仅提交6个Source及Report/Log/架构索引，共9路径。103个既有未跟踪文件保持原路径/内容；以下两份原有用户修改保持且不暂存：

- OverallReadiness Report：`3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`。
- OverallReadiness Log：`A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。

使用显式路径暂存与普通非强制推送；发布后检查远端分支等于本地HEAD。提交身份由Git提供，不在文件中自引用提交SHA。原始证据与执行/检查脚本保存在本地Saved。

## 7. 交接与P/F边界

本轮只闭合durable来源读取契约；FZ-1仍需同权威新定义目录接纳、首次来源内容授权、Subsystem/Manager产品接入，以及原计划恢复/物化与获物/消费/终局身份组合。FZ-2其余生命周期审计未完成；FZ-3最终双根/双构建待有限清单关闭及最终输入固定后执行。见 [冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)，不以本阶段通过暂停为“整体完成”。

遵守 [P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：没有物理输入、正式地图/内容资产、玩法数值/手感/敌人行为/UI修改，没有Editor UI、PIE、Standalone、游戏程序、截图、Smoke、Cook或Package。
