# Dev.D.UE.0.0.10.P27.31.r0 Report

## 1. 结论

P27.31 闭合 Run 结算重放的身份与内容一致性缺口：同一 Run 的成功终局记录不再自动证明任意后续结算请求成功。只有规范化后的请求身份与既有持久指纹一致，才返回原成功回执；内容冲突只读拒绝，不增加拒绝记录、不重发奖励、不改存档代次。

新测试在原实现上得到 0 Success / 1 Fail。修复后 RunLifecycle 专项 6/6、完整旧 demo_map 根 1,330/1,330、完整 Shanmen.0_0_10 根 1,419/1,419 通过，最终 Editor/Game 原生退出均为 0，映射自检 504/504。完整新根来自一次独立重跑，未拼接中断日志的结果。

本阶段是底层契约修复，不代表整体框架冻结、真实 UI 或游戏体验验收。

## 2. 基线与问题

- 产品基线：P27.30 / `225762c1c4cc23e17df0bddd6c03e8101119bec9`。
- 本阶段提交父基线：`306f228a0f79dee9e18b21ec0e39c04b4722ceaf`；中间两次提交只更新总体 Report/Log。
- 分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 日期：2026-09-15 至 2026-09-16 UTC。

原 `FinalizeSettlement` 按 RunId 找到成功 FinalizePreparedRun 记录便提前返回 NoChange，跳过本次 secured items、结局与获物元数据的校验。Repository 本身已有完整请求指纹，但适配器的提前返回绕过了它。失败复现包括一致改写后的结局、撤离原物品数量、合法奖励元数据和重复 secured identity，不以格式错误替代真实冲突。

证据证明的是错误接受冲突重放，不能据此断言发生过真实玩家丢档或重复发奖。

## 3. 最小实现

1. Repository 公开纯验证函数 `IsExactFinalizedRunReplay`，重用既有 Fingerprint；核对有效请求、有效持久记录、成功操作类型、ActiveRun、RequestId 与完整指纹。
2. 已结束 Run 通过终局回执保留的预留身份，重建原始 owner、scope、整备 batch 与物品行。要求 Released、当前绑定 owner 一致、原物品不重复且 scope 一致；证据不足则拒绝。
3. 私有 `FFinalizeLoadoutSource` 仅承载规范化所需身份/原始行，不伪造仍可部署的整备回执，不要求终局物品仍处于局内或原数量。
4. 首次结算与重放共用 secured items 校验、原物品排序和获物排序路径，再构造原有请求。
5. 精确重放返回原回执；冲突不进入 Durable 命令写路径。已有终局指针属于本地 Snapshot，该分支不重取 Snapshot 使其失效。
6. 保留 `demo_map.ShanmenRun.Finalize.r3` 与 `Shanmen.Items.Command.FinalizePreparedRun.r3` 身份命名空间；没有 schema 迁移、第二权威或新序列化协议。
7. 移除已不使用的本地 MakeContext 辅助函数。

四个源码/测试文件合计 219 行新增、37 行删除，其中新注册测试为 114 行；未更改玩法数值、物品内容或 UI。

## 4. 证明范围

新增一个注册测试 `Shanmen.0_0_10.Items.RunLifecycle.TerminalReplayIdentity`，分别经过真实整备、Run 启动、独立获物和 Extraction / Death / Abandon 结算。

| 场景 | 断言 |
|---|---|
| 同样结算内容、secured 顺序调整 | 即时重放返回 NoChange 和原成功回执，完整 Authority Document 不变 |
| 同 Run，改成另一种一致的终局原因 | 连续两次拒绝，不携带成功 durable result，Document/代次/ledger 不变 |
| 撤离原物品数量从 3 改为 2 | 连续两次拒绝，非零原状态完全保留 |
| 撤离获物改为合法 Jackpot 元数据 | 连续两次拒绝，原记录不变 |
| 撤离 secured 列表增加重复实例 | 连续两次拒绝，不绕过结构校验 |
| 关闭并重新绑定持久 Authority | 三种结局均可精确重放原回执，完整 Document 不变；冲突结局再次连续两次拒绝 |

顺序调整正例在同一新测试中增加，不另算注册用例。数量/元数据/重复实例的负例在撤离路径覆盖，重启后的专门负例是结局冲突；不夸大为所有组合都独立验证。

## 5. 兼容与限制

- 首次终局仍使用既有 Durable FinalizePreparedRun 权威路径；重放不新增事务或拒绝流水。
- 重放依赖已有 released reservations、原物品身份/墓碑和持久指纹；本阶段没有实现历史裁剪后重放或跨内容版本迁移。
- 保证的是规范化后的权威请求一致，不是整份 UI/Runtime 展示结构逐字节一致；与权威请求无关的展示字段不是新的持久协议。
- 不新增存档恢复格式，不改 P27.30 同 GameMode 结束续清理的边界，也不宣称整个 Run 是跨 World 原子事务。
- 本轮只复核并交接已验证实现，没有在完整回归结束后追加源码变更。

## 6. 原始测试结果

| 范围 | Success / Fail | 原字节 SHA-256 |
|---|---|---|
| RedProof，原实现失败复现 | 0 / 1 | `7356B1D24714C5328583B56B5ABEFF75D56AD4FFB443B2629186A9F0086C12AB` |
| RunLifecycleFocused | 6 / 0 | `042307C034766DEF1B8974ECE4BCD74B44799ED10EB9200A958123C1B46B70B0` |
| LegacyFullRoot：demo_map | 1330 / 0 | `0966DDC34F0B225CDA740BB7CB8A2EC807F9E7CB211E9091817EDF9896CB7DFE` |
| FullRootResumed：Shanmen.0_0_10 | 1419 / 0 | `90A03871AFF2B16FB8E6994D87C94311239BF414D62DF301ABB2E690727C9F6A` |

最终各组均有精确队列结束数、原生退出 0，Fatal/Ensure/Unhandled 记录为 0。6 个专项包含在新根中，不能再次累计。原 RedProof 进程退出也是 0，但 Automation 明确 Fail，仍是失败证据。

最终新根测试主体 2026-09-16 12:18:27.563 至 13:53:47.815 UTC，约 1 小时 35 分 20 秒。进程从 12:17:19.489 至 13:53:49.875 UTC。旧根测试主体约 64.64 秒。运行时长不能用早期纯值用例成本代替。

两个最终根启动阶段各有既有 13 条 LogAutomationTest Condition failed 文本，原日志还保留网络超时等警告；本阶段未屏蔽或删改，不声明“日志零 Error/Warning”。通过依据为选定测试结果、队列完成、原生退出和崩溃指标联合核验。

## 7. 中断与执行器恢复

原完整新根停在 1,165 Success / 0 Fail，没有队列终止及原生退出证据；原日志 SHA-256 为 `CADDCB4242EF99CE9259554F76364D105240063400F589F75A253646364F05AF`，只能标记中断，不能计通过。

后续检查确认原进程不存在且主机后来重启；未证明较早日志停止的精确原因。保留旧日志/锁后，确认 1,131 个 Source/Config/Scripts 输入及 103 个原用户文件哈希不变，复核已完成构建和旧根，使用新的独占执行器只重跑整个 Shanmen 根。未拼接中断前 1,165 项，也未并发启动第二份验证。

更早一次本地预检因 Git 非 ASCII 路径引用形式失败，发生在产品测试前；修正忽略目录内 helper 的路径枚举后继续，错误输出保留。详见 Development Log，不将其描述为产品测试失败。

## 8. 构建与改动驱动回归

- 最终 Editor：Succeeded，0 actions，0.94 秒，原生 0；stdout SHA-256 `B9F35894FFBDFFA79CACDC603DB36D2462D459DF57D93C8C4F88D05DAE641392`。
- 最终 Game：Succeeded，230 actions，618.76 秒，原生 0；stdout SHA-256 `518B8B4FA26268D986AE7EE04CD450E5577872455962C2ED3FF987D57F225BDE`。
- 两次构建在中断前已完成；重跑前后输入哈希一致，重新核验原状态/哈希后复用，不冒称中断后又重新编译。
- 映射自检 504/504 PASS；四源码覆盖门 PASS Changed=4 Rules=3 Required=7 Logs=2。
- 必跑组包括 Items、新系统根、旧 Profile、CodeB、ItemUseAndArmor、Hotbar、V2RangedCompatibility；两份完整根日志覆盖全部要求，不仅运行本次新增测试。
- 最终六文件范围覆盖门 PASS Changed=6 Rules=3 Required=7 Logs=2，SHA-256 `9A2BCB572508D2A8FABD3278F032BDCB032EBAD7B1F5CB50ED520F20196DB611`；原四文件覆盖证据保留，位置见 Log 第 8 节。

## 9. P/F 边界及下一步

仅底层源码、测试、Editor/Game 编译与 UnrealEditor-Cmd 无头自动化。未接物理输入、修改正式地图/内容资产、设计玩法数值/手感/敌人/UI，未启动 Editor UI、PIE、Standalone 或产品 exe，未做截图、Smoke、Cook、Package。

P27.31 修复已可交接。下一步是已批准契约、唯一产品入口、权威所有者和生命周期索引的冻结前总审计，区分真实结构缺口与留给 F 阶段的资产/性能/体验债务。不凭本阶段通过宣布整体冻结或增加玩法/恢复包装层；最终冻结完成后再暂停监控。

## 10. 精确交接

本阶段四个源码/测试文件与本 Report、Development Log 共六文件；103 个既有未跟踪用户文件保留，未混入提交。原始 Saved 日志留在本地，GitHub 文档提供其位置与原字节哈希，不声称日志目录已上传。

- [Development Log](../Log/Dev.D.UE.0.0.10.P27.31.r0_log.md)
- [上一阶段 P27.30](Dev.D.UE.0.0.10.P27.30.r0_report.md)
- [总体就绪度报告（其已交付基线仍为 P27.30）](Dev.D.UE.0.0.10.OverallReadiness.r0_report.md)
