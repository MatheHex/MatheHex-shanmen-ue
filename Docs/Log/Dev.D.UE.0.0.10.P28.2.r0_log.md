# Dev.D.UE.0.0.10.P28.2.r0 Development Log

## 1. 基线与范围

- 日期：2026-09-16 UTC；heartbeat codex-10 20:39:57.469Z。
- 入场 HEAD `f89605577b6e07e30c3e87fd949b89144297bf57`；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 最新产品 P28.1，最新总体报告为已完成的文档交付。本轮按监控继续 P 阶段收尾，不继续编写同内容总体报告。
- 完整读取 P 基线、当前冻结索引、P28.1 Report/Log，核对 tracked/index 干净、无运行中 UnrealEditor-Cmd/UBT。保存 1,577 个产品输入和 103 个未跟踪用户文件的路径/字节 SHA-256。
- 仅修复已有携入身份与持久数量契约的正常入口缺口；不新建权威、持久 schema、玩法或 UI。

## 2. 审计选择与根因

1. 检查 Runtime SettleRunItems 和 Shanmen FinalizePreparedRun。后者及既有单元测试明确要求撤离不能静默丢失 DeploymentLock 装备；没有把该约束擅改成“缺失即销毁”。其与实际丢弃路径的边界保留为 FZ-1 待审计项。
2. 发现 AddDefinition/PickupWorld 仅按定义及奖励元数据预检和合并；携入 ID 已绑定原始预留量，但新获物可增加该 ID 数量。TagAffectedForActiveRun 又将受影响的原 ID 标为新获来源。
3. 正常 Subsystem.AddDefinition → 原物三颗变四颗 → RunLifecycleAdapter.UsePreparedRunInventoryItem 的 Runtime CAS 数量与 durable 预留不同 → 拒绝；实际 RequestSettlement 产生的原样摘要也超过可归还原物量。
4. 玩家拖拽、容器两个方向也有同类合并点。只修 AddDefinition 会留下其它入口，所以五处增量写入及相应容量预检复用同一携入身份排斥条件。
5. Runtime Authority 的三个新增参数只借用既有 DeployedItemIds，不保存指针、不创建长期副本；源/目标任意一方属于携入 ID 就不合并。检索到这三个核心写 API 的生产调用均来自 Subsystem；不将此局部检索夸大为所有物品入口已审完。核心旧无上下文调用仍兼容。
6. 容器不能合法承载某携入来源时继续保持原不变量拒绝，本轮不扩充存箱规则。新获但未整备物品的消费仍受 ItemNotPrepared 约束，未恢复旧消费旁路。

## 3. 先失败再修复

- 先只增加 AcquiredStackIdentity 测试。Red Editor 4 actions、39.59s、native 0。
- 既有隔离测试根由 NewPreparationAdapterRoot 生成于 Saved/Automation 下。真实 seed/cutover/整备/启动，携入三颗丹，正常获得一颗同类丹，生命设为 1。
- 原始 0/1，三条断言失败：原物数量/来源与新获身份分离、持久使用、未经编辑的撤离交接。native 0 仍失败。没有覆盖/修饰原日志。
- 再修复三个生产文件，新增另外三个注册测试。最终测试文件与 red 阶段不逐字相同，red 只证明最初那个用例。
- Fixed Editor 84 actions、250.11s、native 0。随后锁定 1,131 个 Source/Config/Scripts 输入，再运行专项、完整 Items、完整旧根与 Game。
- 新 World 用例复用既有 GamePreview 夹具，在内存中建立碰撞地面及 Pawn/Controller，用真实 CreateWorldItem/PickupWorldItem/DropInventoryItem 接口验证；无物理输入、正式地图改动或可见编辑器。

## 4. 自动化原件

路径相对 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`，Saved 原件保留本地，不上传整目录。

| 组 | 原日志 | Success / Fail | SHA-256 |
|---|---|---|---|
| RedProof | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0/RedProof/20260916T204351648Z-c8c058dd/UnrealEditor.log` | 0 / 1 | `D501E4D6E76DC4FD4BB96666145FB25853BEF608EE4E426B5241F4BA271F7059` |
| RunLifecycleFocused | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0/RunLifecycleFocused/20260916T205232797Z-5017211c/UnrealEditor.log` | 10 / 0 | `16EF9F8B8BE99658CA6CF4B06142FB2760082A6293297EC3BB21C7B8116F2030` |
| ItemsFullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0/ItemsFullRoot/20260916T205309354Z-783a8928/UnrealEditor.log` | 84 / 0 | `898B2FDD2D2174BF0299E5695358DE571434FCAE48433B474953FBC74490C921` |
| LegacyFullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0/LegacyFullRoot/20260916T205334895Z-03207bde/UnrealEditor.log` | 1330 / 0 | `8A5A38D68075D55FF8F49E4898A14872D5B5201628BEA593A7B42F95596E9D8B` |

各组 native 0；精确队列分别为 1、10、84、1330。RedProof 仍判失败，后三组成功。10 项包含在 Items 84 项内，不重复计独立总数。四组均有既有 13 条启动 Condition failed 文本；Fatal error / Ensure condition failed / Unhandled Exception 匹配为 0，不声称日志零错误。

修复后验证串行执行，参数为 Unattended / NullRHI / NoSound / NoCompile，使用忽略路径 `Saved/Automation/P28.2/validate.ps1` 调用既有 Foundation 进程跟踪；它联合检查原生码、Result、队列和崩溃指标，完整根要求精确 84/1330 项。测试辅助脚本及原日志不作为新增产品交付文件。

## 5. 构建

| 构建 | 原目录 | 原生码 | stdout SHA-256 |
|---|---|---:|---|
| Red Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0.RedProof/BuildEditor/20260916T204244573Z-36bd2fa8` | 0 | `D94C4B6357986F5FCE8E20E6EFF9DCA198463C74E2E412C46FEB657027BD3829` |
| Fixed Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0.Fixed/BuildEditor/20260916T204809357Z-9b9f60f2` | 0 | `EE99E36E169D1657B365282D163F73BF683EAD7F92B78224D920D8ED166AEDBE` |
| Game | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.2.r0.Validation/BuildGame/20260916T205430917Z-fbf9f625` | 0 | `422B4B36E0BF8BC552263B06A9052816150FFD6A176159C0DFB2FAB6281650F3` |

Game 83 actions、250.81s，run-state 为 SUCCEEDED/native 0；于 20:58:42.0048041Z 完成。Fixed Editor 84 actions、250.11s，同为 SUCCEEDED/native 0。均使用 Invoke-Shanmen 入口、MaxParallelActions=1、NoUBA。输出 binary 路径不代表启动了产品。

## 6. 回归映射与保持检查

- 四个源码路径均命中既有 ItemProductAdapters，必跑 Shanmen.0_0_10.Items、demo_map.ItemUseAndArmor、demo_map.P4.Hotbar；选完整旧根而非仅主题专项，覆盖旧物品、容器、拖拽、整备、存档等。
- 检查器和映射未改动。自检原件 `Saved/Automation/P28.2/regression-selftest.log`：517/517，SHA-256 `96B77D69044EEA0655A097D38D0629A20A894FF21CE4352562003F17ABCE6A5F`。
- 七文件映射原件 `Saved/Automation/P28.2/regression-coverage.log`：PASS Changed=7 Rules=1 Required=3 Logs=2，SHA-256 `868C0464CD395B236ECBD499B5A4C1AE0A6B7ADBA509A0ADC6C6EF670EA82D39`；恢复交付时再按同一七路径和两份完整根日志复核。
- 测试后的中间保持核验：1,131 个验证输入原字节未变；1,577 个产品输入中仅预期四个源码变化；103 个原用户文件路径/字节未变，HEAD 未变。

## 7. 精确交付与边界

精确七文件：ItemAuthority.cpp/.h、ItemSubsystem.cpp、ShanmenPreparationAdapterTests.cpp、FoundationClosure_Index、本 Report/Log。保持 103 个既有未跟踪用户文件；不提交 Saved、资产、存档或无关总体报告。

FZ-1 只关闭本次已复现的堆叠身份缺口，剩余路由和政策接缝仍需审计；FZ-2/3 未完成。没有最终冻结，不暂停监控，不自动转入实际玩法。

未接物理输入，未改地图/内容、技能、敌人、手感、数值或 UI；未启动 Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook 或 Package。

- [Report](../Report/Dev.D.UE.0.0.10.P28.2.r0_report.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)

## 8. 恢复交付（21:37:57.997Z heartbeat）

- 恢复时 HEAD 与远端分支均为 `37959331e7481704fd930d94a2b623dafaf60a96`，index 无暂存。与本阶段最初 HEAD `f896055` 相比只增加总体 Report/Log 文档提交；未将其回退或纳入本阶段改动。
- 重新读取共享 P 基线、冻结索引和本阶段 Report/Log，审阅四个源码差异；无运行中的 UnrealEditor/UnrealEditor-Cmd/UBT，不重复启动已结束构建。
- 重新核对四份 UE 原日志：结果、精确队列、native 与 SHA-256 均与第 4 节一致；原始 RedProof 仍为 0/1。Editor/Game 原 run-state 和 stdout 哈希与第 5 节一致。此轮完成恢复核验，不冒充重新执行测试。
- 1,131 个锁定 Source/Config/Scripts 输入的 SHA-256 全部一致；103 个原用户文件路径和原字节全部一致。仅本阶段两份文档为新增未跟踪交付项；未添加其它用户内容。
- 整体状态仍为 FREEZE_AUDIT_IN_PROGRESS。下一轮继续 FZ-1 剩余入口或 FZ-2 可达边界，不能把本次身份保护视为全部框架契约已闭合。
- 恢复后的七路径映射再次 PASS（Changed=7 / Rules=1 / Required=3 / Logs=2），三份 Markdown 的 44 个本地链接目标存在，`git diff --check` 通过。提交前精确暂存七文件并检查暂存差异，不混入总体报告、Saved 或用户文件。

发布源码与验证输入的原字节哈希：

| 文件 | SHA-256 |
|---|---|
| `Source/demo_map/demo_mapItemAuthority.cpp` | `0C10E01464F3EEC54806355BFD8C9556DB94D805883810F89D9763F93260210E` |
| `Source/demo_map/demo_mapItemAuthority.h` | `8A9B37A81509D63E5D836D3786CC704BD7E26E71339D02DE770B57825DFD3059` |
| `Source/demo_map/demo_mapItemSubsystem.cpp` | `D58244E54CA30D10897960194DD2E68245D284BB553E32A3A656B05CD99232BA` |
| `Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp` | `157C49D294A63DD20B8BBBE951A48E1E75079F90CB3B250A9FD415203F97B090` |
