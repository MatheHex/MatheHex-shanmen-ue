# 山门 Demo2.0 人工验收并行开发 Development Log

日期：2026-10-05。基线：`22ab7ed8d2efa16aca7e0578393d3fddca477e57`。分支：`agent/0.0.10-p27-28-formation-scatter-gamemode-composition`。本次为规则／说明与源码只读核对，不是新功能编码阶段；结论见 [Report](../Report/Demo20.ManualReview.r0_report.md)。

## 1. 用户边界

用户要求实时战斗可请人工审查并给注意事项，整体不暂停；问题版本的相应功能暂不叠加开发，独立工作继续。人工提问返回“开启对应版本游戏”，只授权启动／交付游戏，不授权代理战斗。

当前 Goal 增加并行规则，保留全部十五项验收、同一权威和失败关闭。README／状态链接人工治疗说明；当前药物和战斗参数不改。收到人工正常反馈后解除等待造成的用药扩展暂缓，独立来源接线继续。旧自动化不恢复，Goal 不标暂停或完成。

## 2. 保护和源码核对

进入时仅四份受保护 tracked 文档修改及历史 untracked。`Saved/Automation/Demo20/Protect.ps1 -Verify` 对 113 项通过。仅新建／修改本次六份文档；不纳入用户文件、Source、地图、生成器、物品档、旧失败原件或截图。

读取 AuthorityService 的 ReadGeneratedSource／AcceptGeneratedSourceDurable、Repository 的来源接纳／同锁视图、Product Subsystem、旧 M01／P8 Adapter、RewardGenerator 和 Demo20 Catalog／Loadout。确认：产品缺读取／接纳包装；来源接纳保存计划不插入可领取 item graph；旧 GenerateWithData 查询旧 ItemDefinitions。下一步需正式新产品注册与确定来源，不能假成功增加数量。

## 3. 旧窗口关闭证据

最初 PID 58300 仍在；用户要求启动对应游戏后，只读核对发现进程已退出。旧日志 `Saved/FoundationRuns/Demo20.S01/Play/20261005T092828655Z-a2fe6237/UnrealEditor.log` 有 09:56:01.935Z Game engine shut down，09:56:02.532Z Exiting，09:56:02.542Z Log file closed。

闭合日志 SHA-256：`D504FB179C1EDA9B5219A0C4111B4A1C77AFE14E79C1882021B1DECFD7C1CCEF`。旧 run-state.json 仍为 STARTED、completed_utc／exit_code 为 null；非阻塞启动没有捕获最终原生码，不能记录为 native 0。未改写旧 state 或 Medicine 历史报告。

## 4. 新人工窗口

命令：

```powershell
./Scripts/Start-Demo20.ps1 -Action Play -ProfileName ManualMedicine_20261005 -Width 1280 -Height 720 -TestMoney 1000000
```

启动器外层 0，仅代表进程启动成功。新 PID 49540；证据目录 `Saved/FoundationRuns/Demo20.S01/Play/20261005T095740690Z-f1903ca8`。09:57:52.196Z 正式 GameMode，09:57:52.214Z 正式地图 BringWorld，09:57:52.240Z PROFILE Ready=1 Native=1 Generation=1，09:57:52.241Z DEMO20_READY Actors=57，09:57:52.882Z VIEWPORT Pixels=1280x720。不把 Ready 当治疗通过。

Computer Use 首次激活失败 `failed to activate captured window`；按技能刷新窗口绑定一次，只读截图未可靠显示游戏。未继续输入、未以直接 PowerShell UI Automation 兜底。已告知人工 Alt+Tab；代理未出发、战斗、用药、关闭或重启此窗口。没有新增“治疗成功”截图。

人工用药后已退出，10:01:48.314Z Exiting，10:01:48.324Z Log file closed，核对时 UE／UE-Cmd 均不存在。闭合 `UnrealEditor.log` SHA-256：`1CE71325CB418B0FF780076A3B8EBF6CB6D23DCD98DB4486A96F7D9073624E59`。非阻塞 run-state 仍为 STARTED／exit_code=null，原生最终退出码仍未知，不改原件为 0。

真实用药 Run `D9CD87DDB59ED5C17806B2D8E3EA1E22` 于 09:59:50.686Z 出发。10:00:29.529Z MEDICINE_CONFIRM Success=1、WorldGeneration=48、Seq=14、HP=100；同刻 MEDICINE_USED Before=93.84 After=100 Carry=7 Secure=0。10:00:31.467Z 满生命拒绝。当前 93.84 + 35 夹紧为 100，实际加 6.16；不是低血量 +35 的新真实证据。

只读原生物品文件 `Saved/Demo20/User/Saved/Demo20/ManualMedicine_20261005/ShanmenItems/Authority/165C5C9C689E2502A733AFF4489C62CC.json`：FinalizePreparedRunQuantityIntent 为成功 Committed、Amount=1、resourceBefore=8、resourceAfter=7、AuthorityRevision=23，reservationIds 首项匹配该 Run。文件 SHA-256 `25814E3B6CD063DF89A1BDBEB51EF70A3E4F56DA6AF261C882E1CCF86865E303`，SaveGeneration=22；人工作用后的档案保留，不编辑、不提交。

日志另于 10:01:01.212Z 记录该 Run 撤离 Success=1、ItemGeneration=21，10:01:05.802Z 新 Run 开始／Generation=22。与用户治疗反馈一起保存为当次流程旁证，不据此宣称新来源搜集、死亡或重启检查通过。

## 5. 本次验证范围

六份 Docs 均属于 ShanmenRegressionMap 的忽略路径，生产改动为 0。未执行本次 Editor／Game 构建或 UE-Cmd 自动化；不把历史测试／构建计为本次 M3 通过。Medicine 基线自动化结论仍见原 Report／Log。

提交前核对 `git diff --check`、六份文档的相对链接、既有保护 113 项和明确暂存清单。本次不含新源码，所以没有源码首次失败或自动化结束记录；窗口激活失败与原生未知退出如实保留。已闭合日志与只读物品文件摘要独立验证，不把 JSON 序列化长度当运行内存。

## 6. 后续

用户在本次启动交付后反馈“丹药正常使用”。进一步读取当次闭合日志和精确 Run 数量回执，普通治疗及只扣一颗的量化结果对齐。没有新增成功截图、安全格、冷却连按或重启结果，不将这些项目补写为通过。治疗可按正式来源与领取依赖继续扩展，不改当前参数。

人工记录非满生命普通／安全格治疗、动作与重复键、同档重启后库存和生命；出现不确定保存保留档案，不删档或换药。文档给出步骤、风险、预期和简短反馈格式。

开发继续独立 M3 来源路径。尚未实现源计划生成、领取或 M3–M5；完整阶段目标不缩减。人工游戏运行期间不抢窗口、不替换其 DLL；当前不可运行的构建／真实验收明确列待执行，不伪造完成。
