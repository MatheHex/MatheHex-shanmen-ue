# Dev.D.UE.0.0.10.P27.31.r0 Development Log

## 1. 基线与范围

- 日期：2026-09-15 至 2026-09-16 UTC。
- 产品基线 P27.30：`225762c1c4cc23e17df0bddd6c03e8101119bec9`。
- 提交父基线：`306f228a0f79dee9e18b21ec0e39c04b4722ceaf`；中间提交只修改总体 Report/Log。
- 分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。
- 既有未跟踪用户文件 103 个，以执行器保存的路径和 SHA-256 逐个核验，不暂存、不改写。
- 本次交接续用先前已完成的修复与验证，没有在回归完成后再改源码或重启测试。

## 2. 精确变更

1. `Source/ShanmenItems/Public/ShanmenItemRepository.h`：声明纯 `IsExactFinalizedRunReplay`。
2. `Source/ShanmenItems/Private/ShanmenItemRepository.cpp`：复用既有 Fingerprint 核验成功终局操作、ActiveRun 和请求内容。
3. `Source/demo_map/demo_mapShanmenRunLifecycleAdapter.cpp`：从 released reservation/原物品重建规范化上下文；首次结算与终局重放共用输入校验；仅精确匹配返回原回执，移除过早成功与未使用 helper。
4. `Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp`：新增一个注册测试，114 行，涵盖三种结局、非零数量、获物元数据、重复项、等价顺序与重启。
5. `Docs/Report/Dev.D.UE.0.0.10.P27.31.r0_report.md`。
6. 本 Development Log。

四源码合计 219 行新增、37 行删除。没有新增 schema、身份命名空间、物品权威、引擎依赖或玩法数据。

## 3. 实施与原始失败

1. 发现适配器按 RunId 命中成功终局就提前 NoChange；Repository 已有请求指纹但未参与此分支。
2. 在原生产实现上新增终局重放测试，完成 RedProof Editor 构建；测试得到 0 Success / 1 Fail，队列正常结束。原生 0 不能将其改称通过。
3. 实现精确重放比较。原请求 ID 和完整指纹共同约束权威载荷，私有 source 结构不作为有效整备回执对外发布。
4. 2026-09-16 清理未用 MakeContext，并在同一注册测试补等价 secured 顺序的即时重放断言。原失败日志证明最初缺口；最终专项证明最终测试和实现，不说两次测试源码逐字一致。
5. Final cleaned Editor 后，RunLifecycle 全组 6/6；测试真实启动和绑定物品服务，不用全新空状态冒充保持证明。
6. 锁定 1,131 个 tracked Source/Config/Scripts 输入和 103 个原用户文件哈希，串行完成 Editor/Game 和整个旧 demo_map 根，再运行整个新根。

反例在每种结局下将 Summary 与其 RuntimeSnapshot 的原因一致改为另一个原因；撤离额外将 PillOne 的 3 个改为 2 个、获物改合法 Jackpot 元数据、复制一个 secured identity。每种负例连续两次检查失败、无成功 durable result、完整 Document（含代次和历史）不变。重启后验证三种精确重放和再次拒绝冲突原因。

## 4. 中断、预检和恢复记录

- 首次预检 runner 42824 在读取 Git 引用的中文未跟踪路径时失败，尚未启动产品构建/测试。保留 `Saved/Automation/P27.31/runner.preflight-path-quoting-failure.stderr.log`、对应 stdout 和 `validation.preflight-path-quoting-failure.lock`。只修正本地忽略 helper 为 `git -c core.quotepath=false ls-files`，未更改用户文件。
- 原串行 runner 32932 于 2026-09-16 03:21 UTC 启动，构建和旧根完成；新根子进程 9644 的原日志止于 04:27:57 UTC，1,165 项成功、无最终队列或 run-state。旧 `validation-progress.json` 已标记 INTERRUPTED，原生退出未知。
- 12:14 UTC 检查原进程已不存在，主机最后启动时间为 11:46:31.5 UTC。只能证明后来重启，不能断言该重启是较早日志停止的精确原因，也不把日志终止当作已证明源码崩溃。
- 重启后的执行器为 `Saved/Automation/P27.31/resume_validation_after_interruption.ps1`，runner 2296、子进程 40496，独立 `validation-resume.lock`、`runner-resumed.stdout.log`、`runner-resumed.stderr.log`。旧证据/锁全部保留。
- 新执行器先复核输入哈希、旧根哈希/队列/原生退出和已完成双目标构建，再只重跑整个 Shanmen 根；未重跑旧根、未合并原中断计数、未并发验证。
- 13:31 UTC 新根也经过原中断位置；当时进程存活、日志仍增长，不因部分计数相同就判第二次中断。
- 完整重跑进程 12:17:19.4885261 至 13:53:49.8745562 UTC；执行器经映射/输入核验后，于 13:54:09.7551373 UTC 记为 COMPLETED。
- 交接轮重新读取原始最终日志与状态、计算哈希；1,419/0 和 1,330/0 均来自实际队列完成。当前无遗留验证进程。

## 5. 测试日志索引

路径相对项目 `C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B`，Saved 原件保留在本地，不上传整份日志目录。

| 标签 | 原日志路径 | Success / Fail | 原字节 SHA-256 |
|---|---|---|---|
| RedProof | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0/RedProof/20260915T041521007Z-9be2e054/UnrealEditor.log` | 0 / 1 | `7356B1D24714C5328583B56B5ABEFF75D56AD4FFB443B2629186A9F0086C12AB` |
| RunLifecycleFocused | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0/RunLifecycleFocused/20260916T031730562Z-f761b323/UnrealEditor.log` | 6 / 0 | `042307C034766DEF1B8974ECE4BCD74B44799ED10EB9200A958123C1B46B70B0` |
| LegacyFullRoot | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0/LegacyFullRoot/20260916T033134796Z-0afa1a8a/UnrealEditor.log` | 1330 / 0 | `0966DDC34F0B225CDA740BB7CB8A2EC807F9E7CB211E9091817EDF9896CB7DFE` |
| FullRoot（中断，不计通过） | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0/FullRoot/20260916T033300827Z-1710891b/UnrealEditor.log` | 1165 / 0，中间值 | `CADDCB4242EF99CE9259554F76364D105240063400F589F75A253646364F05AF` |
| FullRootResumed | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0/FullRootResumed/20260916T121719423Z-e1a68c02/UnrealEditor.log` | 1419 / 0 | `90A03871AFF2B16FB8E6994D87C94311239BF414D62DF301ABB2E690727C9F6A` |

各完成组同目录有 run-state.json。专项和两个最终根均 native 0、精确队列完成、Fatal/Ensure/Unhandled 0；RedProof native 0 但实际 Fail。完整新根相对 P27.30 多一个注册测试，六项专项包含其中。

两个最终根启动阶段各 13 条既有 Condition failed 文本，另有 HTTP 超时 Warning；原字节均保留，不宣称整个日志没有错误/警告。测试主体时间：旧根 03:31:50.966–03:32:55.607 UTC；重跑新根 12:18:27.563–13:53:47.815 UTC。

## 6. 构建与映射证据

构建目录的 stdout.log 和 run-state.json 均保留。最终构建原生退出独立核对为 0/0；不将输出 binary 路径解释成启动产品。

| 构建 | 原证据目录 | 结果 |
|---|---|---|
| Red Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0.RedProof/BuildEditor/20260915T041410501Z-a11b55cb/` | native 0；4 actions，53.64s |
| First Fixed Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0.Fixed/BuildEditor/20260915T041803904Z-abe25b81/` | native 0；233 actions，894.92s |
| FixedClean Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0.FixedClean/BuildEditor/20260916T031630876Z-c6ed7e72/` | native 0；5 actions，41.70s |
| Final Editor | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0.Validation/BuildEditor/20260916T032114134Z-69884766/` | Succeeded，native 0；0 actions，0.94s |
| Final Game | `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.31.r0.Validation/BuildGame/20260916T032115394Z-44f61089/` | Succeeded，native 0；230 actions，618.76s |

- 最终 Editor stdout SHA-256：`B9F35894FFBDFFA79CACDC603DB36D2462D459DF57D93C8C4F88D05DAE641392`。
- 最终 Game stdout SHA-256：`518B8B4FA26268D986AE7EE04CD450E5577872455962C2ED3FF987D57F225BDE`。
- `Saved/Automation/P27.31/regression-selftest-resumed.log`：SELF_TEST: PASS 504/504；SHA-256 `507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`。
- `Saved/Automation/P27.31/regression-coverage-resumed.log`：PASS Changed=4 Rules=3 Required=7 Logs=2；SHA-256 `8B6E75439487573A2FD6F5AE0FF58FC5EFB28673389B87EBE52532C00B68BECF`。
- 七个要求组为 demo_map.Profile、CodeB、ItemUseAndArmor、P4.Hotbar、V2RangedCompatibility，以及 Shanmen.0_0_10 与其 Items；它们被本次两个完整根覆盖。7 是组数，不是测试数。
- 检查器按正常返回及 PASS 判断，不伪造其未单独返回的原生进程码。最终提交范围覆盖记录补充在第 8 节。

## 7. 输入保持证明

构建与测试的 1,131 个 tracked Source/Config/Scripts 输入和 103 个原用户文件，在重跑前后及交接时按原字节 SHA-256 一致。四个修改文件的最终基线为：

| 文件 | 原字节 SHA-256 |
|---|---|
| Source/ShanmenItems/Public/ShanmenItemRepository.h | `92C6160A5D6A1E4E613AD98B5AD1FC38A36C3B5023073AAB9278DC870895893A` |
| Source/ShanmenItems/Private/ShanmenItemRepository.cpp | `AC884CB4091A27A0A804534C9670940B1755F205FDA7EC12B608F36A49136EBD` |
| Source/demo_map/demo_mapShanmenRunLifecycleAdapter.cpp | `110AAE33CA63D1AF5537FDC4E6CFC519F0FCBE57493CC4E76ED5B7F9D59A695E` |
| Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp | `75AA85E23826DE6AE31867A5408306CD78FE524B24863284682ECFCA87453C7A` |

原输入清单保存在本地 `Saved/Automation/P27.31/validation-resume-progress.json`，不把含无关用户文件路径的整个清单提交到 GitHub。

## 8. 最终范围核验

最终六文件覆盖门：`REGRESSION_COVERAGE: PASS Changed=6 Rules=3 Required=7 Logs=2`。

- 原记录：`Saved/Automation/P27.31/regression-coverage-final-scope.log`；SHA-256 `9A2BCB572508D2A8FABD3278F032BDCB032EBAD7B1F5CB50ED520F20196DB611`。
- 两份文档的 5 个本地链接目标存在；差异卫生检查通过。最终精确范围是第 2 节六文件，不包含总体报告、Saved 执行器或其他材料。
- 1,131 个验证输入及 103 个原用户文件哈希均保持，原用户文件路径集合也相同。首次集合比较将相对路径和绝对路径混比而误报；统一到项目绝对路径后差异为 0，没有删除/移动任何文件，也未重跑产品测试。
- 发布前本地 HEAD、远端分支均为 `306f228a0f79dee9e18b21ec0e39c04b4722ceaf`，初始 index 为空；只精确暂存本阶段六文件。LF/CRLF 提示是仓库换行规则，不代表源码或测试失败。

## 9. P/F 边界与下一步

仅底层结算重放、只读比较与无头验证。未开发或运行物理输入、UI、正式地图/内容资产、玩法数值/手感/敌人、Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook 或 Package。

本阶段解决的是已复现的结算内容冲突；全体已批准底层契约与唯一入口/生命周期总索引仍需冻结前审计，不在本阶段宣称最终冻结，也不自动进入实际游戏性开发。

- [Report](../Report/Dev.D.UE.0.0.10.P27.31.r0_report.md)
- [上一阶段 Log](Dev.D.UE.0.0.10.P27.30.r0_log.md)
