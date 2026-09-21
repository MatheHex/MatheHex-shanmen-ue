# Dev.D.UE.0.0.10.P28.27.r0 Development Log

状态：`P28_27_AUDIT_PASS`。本阶段为有界可达性审计与文档分类，源码零改动；不是整体冻结或 F 验收。发布基线由包含本文的 Git 提交追溯。

## 1. 基线与保护

2026-09-21T15:58:47.666Z heartbeat；分支 agent/0.0.10-p27-28-formation-scatter-gamemode-composition；HEAD `57fb7bdef9c4f40fcf84f6f40f10dc4097678cca`。入口读取 P28.26 Report/Log、P 阶段基线、有限清单与 Git 状态。15:58:59.7446230UTC 的 P28.26 Published 核验通过：1617 输入、103 原用户文件、两份保护文档、12 原件 SHA、126 链接、7 个健康执行、2760 独立完整根用例、暂存 0。没有重复启动 P28.26 验证。

保护文档 OverallReadiness Report SHA `3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`、Log SHA `A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。原 103 文件精确路径/SHA 与 1617 输入沿用 Saved/Automation/P28.26/validation-inputs.json；本轮在验证前后核对，未更改该锁或受保护文件。

## 2. 静态审计链

核对 demo_mapProfileStartupMode.h/.cpp 的默认输入、Select、UsesProfilePreparation/UsesLegacyRuntimeBegin；Manager.h 的私有默认状态；Manager.cpp 的 ReadNonShippingStartupFlags、IsLegacyAutomationRequested、Initialize、全部 ProfileStartupMode 赋值与 Flow.Reset、RequestSettlementAndReload、CompleteDurableProfileSettlementWorld、RetryPendingProfileSettlement、ReloadAfterSettlement 与 EndPlay。

基线源码定位：Manager.cpp:663 的旧模式标志来自显式旧自动化标志 OR 集合；1419–1436 的读取与输入复制受非 Shipping 条件保护；1437 唯一模式赋值、1440 创建 Flow。正常 Profile 分支在旧 BeginWorld/BeginRun 前返回。1718/1833/2102/2180/2220/2914 的 Reset 都属于非 Shipping 自动化初始化失败并立即返回 false；9515 是 EndPlay 的 Reset。源码保持不变，行号对应本阶段基线。

9197 的 RequestSettlementAndReload 根据模式及 Flow 选择终局；Profile 持久成功调用已有完成端口，失败/待重试也从 Profile 分支返回。只有非 Profile 尾部 9295 设置旧重载计时器，9379 的 ReloadAfterSettlement 才调用 OpenLevel。未执行该计时器或加载地图；静态分类不当作完整 Manager 初始化及终局动态验证。

正常产品结论排除已进入 EndPlay、自动化初始化失败后被外部错误继续调用、夹具直接篡改私有状态等条件。选择器本身仍接受 LegacyAutomation 输入；Shipping 的 Manager 标志来源只是静态预处理审阅，本轮没有 Shipping 构建。旧终局 DestroyRuntimeContainers 的返回值仍被忽略，未宣称该分支已经修复。强制 EndPlay 的清理/回调次序继续单独留在 FZ-2。

## 3. 本轮执行证据

本地独立任务 Dev.D.UE.0.0.10.P28.27.r0.Audit；忽略目录中的 run.ps1 检查 HEAD 与旧输入锁，执行 Invoke-Shanmen BuildBoth，再启动唯一 UnrealEditor-Cmd 无头专项。仅运行既有 demo_map.ProfileNormalStartup，不修改或增加用例，不使用旧 UI/Smoke 启动标志。

Editor Development：16:01:39.3403442–16:01:42.3668818UTC，原生 0，Target is up to date、零编译动作，构建工具报告 2.69 秒。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.27.r0.Audit/BuildEditor/20260921T160139293Z-8cc37475/stdout.log`，SHA `BA5542FBD1053AD60AD597DA6111E58AE54D59E8B2D62AD8341F313893059F93`。

Game Development：16:01:42.3847959–16:01:43.6619500UTC，原生 0，Target is up to date、零编译动作，构建工具报告 1.03 秒。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.27.r0.Audit/BuildGame/20260921T160142382Z-26cf5372/stdout.log`，SHA `C8CE5FBCD8A68882388ED46E76F61A7CBCCD45FDB2C774D68D178E3EEA8F01C5`。没有运行生成的游戏程序。

StartupFocused：16:01:44.0977387–16:02:29.5448635UTC，17 成功/0 失败、17 个独立名字、精确队列 17、原生 0、Fatal/Ensure/Unhandled 指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.27.r0.Audit/StartupFocused/20260921T160144087Z-ffb1d029/UnrealEditor.log`，SHA `7B6D7243E173675372467938C7B9B436E0A78624B453C408ECFCBECEE425CC3B`。exec46500 外层 0；16:02:30.0262515UTC 后置输入锁检查通过。既有 .01/.02 覆盖默认 V3 和显式旧模式选择；其他用例仍为隔离 Flow/逻辑接口测试，不冒充物理输入、实际页面或地图验证。

精确三份文档的回归分类实际执行 PASS Changed=3 Rules=0 Required=0 Logs=0。原件 `Saved/Automation/P28.27/regression-coverage.log`，SHA `92637BA9A2DD35B80B09B9D8046527FA74138760EFFB9D7069542C3A7D50A845`。零必跑组来自文档分类，不是产品回归豁免；本轮专项证据独立登记。映射与检查器未改动，没有宣称重跑其 537 项自检。

## 4. 明确复用的完整根证据

以下两份均为 **P28.26 Final**，不是 P28.27 新跑；1617 产品/验证输入一致，本轮只有文档改动。17 项专项属于旧根子集，不与完整根相加。

P28.26 旧根：2026-09-21T13:18:30.6573583UTC 完成，1330 成功/0 失败、精确队列 1330、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/LegacyFullRoot/20260921T131715213Z-f5b5d741/UnrealEditor.log`，SHA `306CE55AFDABD29E890EB351F093EA5CE46BC4048D72C7429335C1FB590A140F`。

P28.26 新根：2026-09-21T14:48:50.0391969UTC 完成，1430 成功/0 失败、精确队列 1430、原生 0、崩溃指标 0。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.26.r0.Final/ShanmenFullRoot/20260921T131830947Z-45d20b29/UnrealEditor.log`，SHA `8A0640A07E8785FE2C7F3A287B97A45E396718E2350742C5B89820C7EC68A19B`。两根共 2760 个独立成功用例，仍不是 FZ-1/2 关闭后的最终冻结验证。

## 5. 精确交接

仅更新有限冻结索引和新增本 Report/Log 三路径；生产/测试/配置/资产均零改动，无产品 Red 或修复声明。本轮保留旧自动化兼容分支，不新增通用恢复服务。发布核验范围包括输入与用户文件哈希、原件六项 SHA、相对链接、三个本轮健康执行、两套复用完整根、精确路径集合与 git diff --check。原始日志仅保留本地，GitHub 上发布的是本开发日志及其可核验索引。

16:10:21.5022488UTC 发布前核验实际通过：1617 输入、103 原用户文件、两份保护文档、六份原件 SHA、136 个相对链接、三个本轮健康执行及两份健康复用根；17 个本轮专项均属于复用的 2760 独立完整根用例。暂存 0、原 103 加新 Report/Log 共 105 未跟踪路径精确匹配，差异空白检查通过，无残留 UE-Cmd。Git 的 LF/CRLF 提示不等于空白错误；用户两份修改未纳入阶段范围。

FZ-1/2 仍开放；强制 EndPlay 与其余入口继续审计。本轮不暂停自动化，不进入玩法、UI 表现或 F 阶段。

- [Report](../Report/Dev.D.UE.0.0.10.P28.27.r0_report.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
