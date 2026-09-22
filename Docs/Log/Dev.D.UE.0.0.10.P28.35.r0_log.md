# Dev.D.UE.0.0.10.P28.35.r0 Development Log

## 1. 基线与范围

- UTC 2026-09-22；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 父提交 `e4fa4da792fd3f808237a73b244d65db18b2da27`；开始读取最新 P28.34.r1 Report/Log、P 阶段基线、冻结索引和 Git 状态，并重跑前阶段 Published 验证通过。前一完整根已结束，没有重复轮询或重启。
- 初始暂存为空，103 个既有未跟踪文件、2 份用户修改文档；没有其他产品源码修改。
- 增量依据是 P28.30 已确认来源闭合顺序：建立 fresh 产品候选转换，不切换 Manager 或生成物持久路由。成果见 [Report](../Report/Dev.D.UE.0.0.10.P28.35.r0_report.md)。

## 2. 实施

新增 GeneratedSourceAdapter 的 h/cpp 和 3 个注册测试；新增共享 DefinitionAdapter h/cpp；MetadataAdapter h/cpp 增加 PlannedStack 入口，原 persistent/runtime 转换仍用原定义 ID；Migration.cpp 改用共享定义映射并保留原 Code A/B 内容/堆叠交叉核对。8 个 Source 路径之外，没有生产或测试源码改动。

关键实现条件：只解析已登记 M01/P8 槽位，用原 Planner 和原 BuildContainerSeed；完整元数据和规范化布局共同进入候选。所有失败只丢弃局部候选，公开输出为空。fresh API 明确要求提交调用方先查持久历史；当前还没有 Manager 调用此新入口。

回归映射新增 `GeneratedSourceProductBoundary`，与既有 `ItemProductAdapters` 取并集：Items、两种武器内容、旧来源规划/全图分布、ItemUseAndArmor、P4.Hotbar，共 7 个必跑组。自检新增 4 路径 × 3 正/负证据变体，共 12 项，从 537 增至 549。

## 3. 本轮原生执行

唯一输入锁 Initial，1628 项：Source 1097、Content 485、Scripts 40、Config 5、uproject 1。构建前和各测试结束后逐项核对 SHA；新增源码、实际脚本也包含在锁内。验证过程中未修改锁定输入。

使用 `Scripts/Invoke-Shanmen.ps1 -Action BuildBoth`，Development Win64、MaxParallelActions=4。Editor UTC 03:41:28.834—03:41:47.048，12 actions；Game 03:41:47.068—03:42:10.476，9 actions；均 SUCCEEDED / 原生 0，是实际编译而非 up-to-date。没有运行构建出的游戏程序。

后续五次独立 UnrealEditor-Cmd 都使用 `-Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile`，单个 `Automation RunTests` 组，队列为空退出；各有独立目录和 run-state：

| Action / 组 | 唯一 Success / Fail / 队列 | 原生 |
|---|---|---|
| AdapterFocused / Shanmen.0_0_10.Items.GeneratedSourceAdapter | 3 / 0 / 3 | 0 |
| ItemsRegression / Shanmen.0_0_10.Items | 106 / 0 / 106 | 0 |
| ThrownDefinition / Shanmen.0_0_10.Product.ThrownWeaponContent | 1 / 0 / 1 | 0 |
| ControlledDefinition / Shanmen.0_0_10.Product.ControlledWeaponContent | 1 / 0 / 1 | 0 |
| LegacyFullRoot / demo_map | 1330 / 0 / 1330 | 0 |

专项 3 个完整名称包含于 Items；另四组共 1438 个互不重复成功名称。外层执行于 UTC 03:45:10.957 回传完成，退出 0；所有进程已结束。五日志均有 13 条既存启动 Condition failed Error，无注册 Fail、Fatal、Ensure、Unhandled Exception、Assertion failed，无 HTTP request timed out。没有本轮 UE 失败或中断；不制造 RedProof，历史失败仍在原阶段目录。

## 4. 原始证据 SHA-256

原件在本地 Saved，不纳入 Git；以下完整路径和哈希由最终交付检查逐项比对，不声称 GitHub 已存放原件。

- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/BuildEditor/20260922T034128792Z-eb7a6453/stdout.log`，SHA `7218383AA2433DA6B6AFAE21A7295E7D930E32427790FC086D60A10D2D69FF25`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/BuildEditor/20260922T034128792Z-eb7a6453/run-state.json`，SHA `95C9413DF35173B5DADCEDBBF60517A0D2790F3B092EC88FAF5DFA1FBB07B68A`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/BuildGame/20260922T034147065Z-efbd2c92/stdout.log`，SHA `CE29856EF124F94E974D1F5F497F920CEA8E3B1916214CB4CE0F9785EBA4B1E2`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/BuildGame/20260922T034147065Z-efbd2c92/run-state.json`，SHA `7C30D926D3B42AA655A486E8AF444F1530EA6668ED38301F75A1F1EAC0BC4998`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/AdapterFocused/20260922T034210865Z-9f946a59/UnrealEditor.log`，SHA `E557F09702806F5B2F9166326D5795C26447259A32F1A52D8580632F1F3418ED`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/AdapterFocused/20260922T034210865Z-9f946a59/run-state.json`，SHA `00C890EA200466810F270714A2D02185FC53BEEFE16D34675579F84DC8E53B27`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/ItemsRegression/20260922T034236688Z-81bf9b09/UnrealEditor.log`，SHA `1CC4AD6F4AB9AF546BEB49AED1BC6494A6A734AB4E1BA2C963C5EC3BFD605CB4`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/ItemsRegression/20260922T034236688Z-81bf9b09/run-state.json`，SHA `FCB81D9A80CE2656D9366CFDCB2758E094C5C37DF92ABD29D4B40E5826F4E672`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/ThrownDefinition/20260922T034308529Z-16945fa4/UnrealEditor.log`，SHA `FBE9F6B456FF23487C82821D02FF1C6F4C62CACE050C9ECA82CCEC6D3C294EA2`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/ThrownDefinition/20260922T034308529Z-16945fa4/run-state.json`，SHA `D18BCDEF27C9CD5612D3BB390A3F52CDD34B3FED4855E149855804D5EB119B5B`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/ControlledDefinition/20260922T034329226Z-5faf930f/UnrealEditor.log`，SHA `B102082E7F1BD01088E69392FD9CF6A419DBA4262D6931FC86A5A1A4F8F8759C`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/ControlledDefinition/20260922T034329226Z-5faf930f/run-state.json`，SHA `37DBEDD8CA64BB20A6DA618B01E394C3DE244D458806ECDF9196CD0FBCFDA01B`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/LegacyFullRoot/20260922T034349942Z-1717cf05/UnrealEditor.log`，SHA `31AF8CB0FEFE5C1497A68236E22A01D78D91DB28841F13375D284A5A7E456575`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.35.r0.Initial/LegacyFullRoot/20260922T034349942Z-1717cf05/run-state.json`，SHA `18050AE03ED0BDFF8F816C1F56302159191428BE6EA8E700BD840A7AD57E5AC0`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.35/validation-inputs-Initial.json`，SHA `6045561B8888BAAB3144984A26E9F6B4CCB815C412CEE6295CB2E14712C94ADA`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.35/coverage-Initial.log`，SHA `9953832FB01801B48928CB9CA1F288FCA11DCC50603A31FFA4816EB260EC5971`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.35/regression-selftest-Initial.log`，SHA `27C66F1B346956EB9E16A6CC798E763AEA6EEE484FA7986FDE75814BFB50B6C7`。
- 原件 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Automation/P28.35/boundary-Initial.log`，SHA `15D10A4A3353C1774BE886AB5ACC8C926316AD50BFDAC31B4A7034C9676135FA`。

## 5. 回归与静态检查

对本阶段 13 个明确路径执行回归检查，结果 Changed=13、Rules=2、Required=7、Logs=4；输入完成的 Items、两武器和旧根四日志，没有用专项替代整个 Items。549/549 映射自检通过，覆盖漏旧规划组或漏武器组必须拒绝的负例。

边界扫描最初按字面量 demo_map 命中已有 ShanmenItemTypes.h:185 的说明性注释，外层诊断退出1；不是源码依赖错误，未修改该注释。随后明确检查 demo_map include、World 类型、ApplyDamage、直接 RNG：核心三模块零命中，Build.cs 无 Engine 模块依赖。新两个产品 Adapter 没有直接 World/存取盘/随机 GUID/RNG 调用；仍调用既有随机规划器，不声称产品整体无 RNG。更正扫描说明写入原件。

交付前核对全部锁定路径集合和 SHA、用户文件、18 条证据哈希、文档相对链接、工作区/暂存 diff --check 和精确提交范围。原始 UE 日志不清洗，保存脚本/检查脚本位于本地 Saved。完整新根未重跑；前阶段完整根只能作为历史，不复用为当前全根通过。

## 6. 用户工作保护与提交范围

仅本轮 8 Source + 2 Scripts + 3 Docs。原 103 个用户未跟踪文件的路径/内容保持，不暂存。两份原用户修改也保持并排除：

- OverallReadiness Report：`3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`。
- OverallReadiness Log：`A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。

按显式路径暂存，不使用全量 add；普通非强制推送，发布后核对远端分支与本地 HEAD。最终提交身份由 Git 提供，不把自引用提交 SHA 写入本文。

## 7. 交接

下一步仍是同一权威内的合法新定义目录接纳、来源内容授权及 durable 当前状态查询，然后产品恢复/物化及消费/终局组合；本候选构造器不能越过这些尚未落地的门。FZ-1/2 开放，FZ-3 不执行，不暂停为“已完成”。内存只是临时 Plan/Seed/Candidate 的静态线性形态，没有峰值或吞吐实测。

严格保持 [P 阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：未接物理输入/玩法/UI，未改正式地图或内容，未运行 Editor UI、PIE、Standalone、游戏程序、截图、Smoke、Cook、Package。
