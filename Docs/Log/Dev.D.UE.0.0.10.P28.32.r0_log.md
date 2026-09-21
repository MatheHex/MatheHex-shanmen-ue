# Dev.D.UE.0.0.10.P28.32.r0 Development Log

## 1. 基线与范围

2026-09-21 UTC；分支 `agent/0.0.10-p27-28-formation-scatter-gamemode-composition`；父提交 `0b73632b64110e7304a4990379c0c8ec36673741`。开始读取 P28.31 Report/Log、Git状态和P阶段基线；P28.31 Published证据复核通过。本轮阶段结论及未完成项见 [Report](../Report/Dev.D.UE.0.0.10.P28.32.r0_report.md)。

核对 Repository/AuthorityService/AuthorityDocument 和本机 UE 5.8 JSON 转换实现后，先实现新来源的无损载荷编解码，避免 uint64/int64 经浮点数损坏。代码不接文件系统或产品；现有 authority schema1/2 完全不变。仅新增 Codec.h/.cpp，扩展已有 GeneratedSourceTests.cpp；另外更新 Report/Log/冻结索引，共6提交路径。运行辅助脚本与原始输出留在被忽略的 Saved 中，不扩展仓库过程脚本。

## 2. 输入固定

原件 `Saved/Automation/P28.32/validation-inputs-Codec.json`，SHA `F60BE9F1917EECDE7FA51A44BE50FFAE5B6ED51C4B0048B05DAC8883942AA26B`。

1622产品/验证输入；前阶段1620中只改来源测试文件，新增2个Codec文件，其余1619原输入未变。103个原未跟踪文件逐文件比SHA保持。以下原有用户修改不编辑、不暂存：

- `Docs/Report/Dev.D.UE.0.0.10.OverallReadiness.r0_report.md`：`3B39BBBEBB1C77D08EAE30CDEF36ECFB9141F9A54C142167D2B8A9C2D305D2D9`。
- `Docs/Log/Dev.D.UE.0.0.10.OverallReadiness.r0_log.md`：`A1BF74E1284ADFA35E670E9BE9FE8733407C9D04BE7B7F5DE5B8F2C11CFE9C26`。

## 3. Editor / Game 构建

任务 `Dev.D.UE.0.0.10.P28.32.r0.Codec`，通过现有 `Invoke-Shanmen.ps1 -Action BuildBoth` 执行 Win64 Development。Editor实际5 actions、Game实际4 actions；都编译Codec与测试文件并链接，原生退出码0。不是外层超时、up-to-date或产品运行证据。

Editor：22:35:15.053—22:35:24.057 UTC。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/BuildEditor/20260921T223515014Z-81149878/stdout.log`，SHA `B3BE323D244EED17FB2B7289A959FFB90BCF60F332FEF15A49930015FFDEA1FD`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/BuildEditor/20260921T223515014Z-81149878/run-state.json`，SHA `00F5F95BE8AAF00350BA8F9CAB34EACF918C9B6719F8AFD7B2678E6D57465A8C`。

Game：22:35:24.088—22:35:40.175 UTC。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/BuildGame/20260921T223524086Z-968637c2/stdout.log`，SHA `3922CF93A3991B3EE7BFF79B31281CF2CEB3788E6A35E34CA512198ECAAD0380`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/BuildGame/20260921T223524086Z-968637c2/run-state.json`，SHA `703ECED268EEC307B59C2FA5158FD57183D1DFA8AB130B5ACB64D4DE57C914C4`。

## 4. 无头自动化

统一使用 UnrealEditor-Cmd，`-Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile`、指定测试组与 `-TestExit=Automation Test Queue Empty`。三个独立进程均 SUCCEEDED/native0，完成结果数量、唯一名称数量和唯一结束队列数量精确相等；无 Result=Fail/Fatal/Ensure/Unhandled/Assertion。

来源专项：`Shanmen.0_0_10.Items.GeneratedSource`，9 Success / 0 Fail / queue9。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/GeneratedSourceFocused/20260921T223540567Z-dba5fc0d/UnrealEditor.log`，SHA `0311EB6BD2B16CE962273D7D597F5F3DF079B7A91E47F6DC184039D2E2C8116B`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/GeneratedSourceFocused/20260921T223540567Z-dba5fc0d/run-state.json`，SHA `84A264835D2BF3D8240EDFAE72D0B39DD717A15859066F1C79D64D6C9A65C747`。

Items整组：`Shanmen.0_0_10.Items`，94 Success / 0 Fail / queue94。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/ItemsRegression/20260921T223601307Z-40e223ed/UnrealEditor.log`，SHA `4E82D69C001D99CC1DCF2FD7199253762B459D6A644B982EF65B7C94EDCF994B`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/ItemsRegression/20260921T223601307Z-40e223ed/run-state.json`，SHA `B468A73463BF810B958CDF544658D231CC348A52BC4D3E146B012D4DB6B26D03`。

旧系统全根：`demo_map`，1330 Success / 0 Fail / queue1330，约22:37:47 UTC结束。原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/LegacyFullRoot/20260921T223631979Z-ad9fc01f/UnrealEditor.log`，SHA `81E60966FF6AF21D4FA0D93FC19D81FABCEF277BAEE034AA48105F33F44B9961`。

原件 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P28.32.r0.Codec/LegacyFullRoot/20260921T223631979Z-ad9fc01f/run-state.json`，SHA `5B143164C85E36D143A42AA474B579822E4342474F012FD6A604EB1B54593A60`。

独立成功94+1330=1424；专项9项是Items子集，不再相加。首次构建/测试均通过，没有失败日志被新成功覆盖。Items日志启动时的13条 `LogAutomationTest: Error: Condition failed` 与P28.31同组13条一致；不将日志说成完全无Error。Shanmen全根本轮未跑，P28.29完整根为历史证据而非本轮验收。

## 5. 新测试与测量

新增4注册测试：`Codec.LosslessRoundTrip`、`Codec.StrictRejection`、`Codec.BoundedCollections`、`Codec.RequiredFieldMatrix`。完整极值往返后计划、来源/容器/有序物品身份、候选回执一致；9专项包括此前5项。原53种载荷变化也逐项走codec：合法变更保留，非法变更拒绝。另有43种损坏变体、80字段的删除/null共160次检查，不计为新增注册用例。

`BoundedCollections` 日志实测 `UTF8Bytes=701135`：1024条普通堆叠、无稀有/词缀奖励的代表性计划。JSON实际序列化/解析后字段完整一致；超限1025条、65标签、17词缀被拒绝。该数值不是内存峰值或最坏大小；接口之外的原始文本字节限制/重复键处理、DOM内存和全Run历史仍需持久接入时处理。

## 6. 改动驱动覆盖与提交检查

原件 `Saved/Automation/P28.32/regression-coverage.log`，SHA `F6D35665E8B3D7866E78AF60532BA97309B2A717931985F3FD903A48A406258A`。六路径：`Changed=6 Rules=1 Required=1 Logs=1`，三个源码均命中Items组，由本轮94成功原件覆盖。额外旧系统全根不是映射必跑组的替代。

原件 `Saved/Automation/P28.32/regression-selftest.log`，SHA `BE9B0BCBA9F3A9FFB23E260B9062A6E050841E81AF98C4829F82AAFAC36009EF`。本轮重新运行 `SELF_TEST: PASS 537/537`；确定性输出SHA与前轮相同不表示复用未运行。

阶段检查核对13份原件SHA、输入锁、所有相对文档链接、精确6路径暂存/提交范围、103用户文件及2保护文件；扫描Codec不依赖demo_map/UWorld/AActor/RNG/随机GUID；`git diff --check`及暂存检查通过。提交后核对HEAD父提交、远端分支SHA和剩余用户工作区。

## 7. 后续边界

无新存档格式上线、无实际来源接收/目录授权/Run cursor写入、无跨进程重放或产品调用；依旧FZ-1冻结阻塞，FZ-2仍开放。下一步将完整来源与序号/保底接入同一 AuthorityDocument，旧schema先验证原摘要再迁移，复用现有失败回滚/不确定重开机制；随后再接生成读取与获物/消费/终局链。禁止旁路旧写入隔离和AcquiredItemMismatch。没有进入 F 或启动任何被禁止的产品/编辑器UI验证。
