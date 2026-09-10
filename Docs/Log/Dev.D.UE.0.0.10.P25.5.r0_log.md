# Dev.D.UE.0.0.10.P25.5.r0 Development Log

## 1. 目标

- 把灵力护盾首次真实容量提交转为短时玩家反馈；
- 显示精确吸收量和剩余容量，并区分容量耗尽；
- 精确重放、较早防御、无效证明和提交拒绝不得重复或伪造反馈；
- 复用现有 PlayerController 与 MainHUD，不建立第二套权威或 UI 子系统；
- 按改动文件执行精确回归、双目标构建并推送 Report 与 Log。

## 2. 基线与范围

- 基线：`1963601eeaa9431778f162026279d4c76ab9f307`（P25.4）；
- 分支：`agent/0.0.10-p25-5-spirit-shield-impact-feedback`；
- 起始 tracked tree clean；
- 用户原有 103 个未跟踪文件全程保持未暂存；
- 不修改护盾激活成本、初始容量、Deadline、防御顺序、伤害公式或共享资源权威；
- 不修改地图、Widget、材质、输入资产或存档 Schema。

## 3. 纯 Impact 展示投影

在 `demo_mapShanmenSpiritShieldHUDPresentation` 中新增：

- `Edemo_mapShanmenSpiritShieldImpactFeedbackKind`；
- `Fdemo_mapShanmenSpiritShieldImpactFeedbackPresentation`；
- `TryProject()`、`IsValid()` 与确定性 `Matches()`。

`TryProject()` 先清空输出，只接受 `DidConsumeCapacity()` 的提交结果。
随后验证权威容量回执，并从回执读取已提交容量、提交后容量和耗尽状态。
部分吸收固定输出 `SPIRIT SHIELD · ABSORBED <N> · <N> LEFT`；
耗尽固定输出 `SPIRIT SHIELD · ABSORBED <N> · DEPLETED`。

投影不读取自由格式 `Diagnostic`，不访问 World、Actor、墙钟或 RNG，
不根据预估伤害反推容量。无效输入会清空可复用输出并失败关闭。

## 4. PlayerController 生命周期

新增 `TryPresentSpiritShieldImpactFeedback()`：只有结果已真实扣除容量且 World 可用时，
才保存这份不可变提交结果，并把展示截止设为当前 World 时间后 1.25 秒。

`IsSpiritShieldImpactFeedbackActive()` 同时要求保存结果仍代表容量提交且未超过截止时间。
控制器不保存第二个容量余额、不更新护盾会话，也不生成自己的 Impact 身份。

物理输入测试补充无效 Impact 栅栏，证明无效证明不能打开该窗口。

## 5. GameMode 攻击发布

新增私有辅助 `PublishSpiritShieldImpactFeedback()`。它只在以下条件全部成立时发布：

1. 目标 Pawn 存在；
2. 攻击结果已执行；
3. 本次攻击检查过灵力护盾；
4. 权威提交结果确实扣除了容量；
5. 目标控制器是项目 PlayerController。

辅助函数接入基础近战、近战冲刺、远程投射物、重型扇区、Boss 形状攻击和
Boss 齐射投射物六条现有入口。每条入口继续使用原来的攻击执行结果，
没有新增伤害解析或容量提交路径。

## 6. MainHUD 接线

MainHUD 在 P25.4 激活反馈下方新增一个 500×36 的短反馈面板：

- 部分吸收：青色，`Y=206`；
- 容量耗尽：琥珀色，`Y=206`；
- P25.4 激活反馈保持 `Y=166`。

HUD 每帧只读取控制器保存的结构化提交结果并调用纯投影。
投影失败时不绘制；HUD 本身不修改战斗或护盾状态。

## 7. 产品测试变更

`SpiritShieldProductSession` 从 8 项扩展为 9 项，并强化既有测试：

- 首次部分扣除 12，精确显示剩余 18；
- 新增容量耗尽测试：吸收 30、最终伤害 10、剩余 0，显示 `DEPLETED`；
- 精确重放返回既有结果但不能再次投影反馈；
- 下游原子提交拒绝不能产生反馈；
- 较早的 `PreventAll` 防御层生效时，护盾不扣容量且不能产生反馈。

最终关键组：HUD 8/8、Physical Input 3/3、Product Session 9/9，合计 20/20。
对应最终日志 SHA-256：

- HUD：`0DE2C82577F102614D456F04D51FBEC415F79EAF0C7B6A8ADD4659A0155E35BC`；
- Physical Input：`4429AD29654142D52B0DE85FAF8DA44C0725DF83A791F76D838E9A08DAA78259`；
- Product Session：`7A576C805E45E3A47C760347938A71D84E8A9AE59942073A135BCCF25B033813`。

## 8. 回归映射与完整结果

`M01GameMode` 原规则同时要求宽泛 `Shanmen.0_0_10` 父组和 101 个精确组。
父组展开为约 1300 项，会重复运行已经由改动路径推导出的组；本轮删除该重复父组，
保留所有精确路径映射。映射器自检仍为 448/448 PASS。

10 个改动路径命中 6 条规则，最终要求并独立运行 101 个组：

| Category | Groups | Success | Fail |
|---|---:|---:|---:|
| Legacy `demo_map` | 10 | 230 | 0 |
| CombatCore | 1 | 9 | 0 |
| CombatRuntime | 6 | 178 | 0 |
| Items | 1 | 77 | 0 |
| Spirit Shield Product | 3 | 20 | 0 |
| Other Product / World | 80 | 534 | 0 |
| Total | 101 | 1048 | 0 |

每份最终日志仅含一个 `RunTests` 命令、至少一个 Success、零 Fail、
零 Fatal/Unhandled/Ensure、原生成功终止标记且无联网探针。

覆盖门：`PASS Changed=10 Rules=6 Required=101 Logs=101`；
日志 SHA-256 `133DF64B978C7E7291E5EC114209DFCCAD8041B8F137B2C4E2D1BBEE61C8C1FE`。
按 `Group|Success|SHA256` 排序得到的 101 组组合清单 SHA-256：
`C832AF6CBD31E3349C80B6070C94D5F36ED2DB6CC0DAD4B9A1D67448A0517F3B`。

## 9. 首错与有界修复

| Evidence | 观察 | 修复 | SHA-256 |
|---|---|---|---|
| `P25.5_EditorBuild_attempt-1.log` | 项目路径误展开为 `\demo_map.uproject`，源码编译前退出 6 | 改用显式绝对路径 | `01E30AC9034AB3788FDAECE4CA9870BEA685C8F8086D2802756F379048C22829` |
| `...CombatRuntime_ActionResource_attempt-1.log` | 6 Success、0 Fail/Fatal，最后测试已开始但无终止标记 | 单次重跑后 7/7 | `D954162A5A101F7F1ED8EB439FF223DE029A1BDAD8A934BA0CBB3E330ACA7969` |
| `...SpiritEvasionCommandRouter_attempt-1.log` | 6 Success、0 Fail/Fatal，最后测试已开始但无终止标记 | 单次重跑后 7/7 | `628330ADCFCB01012B3A49419DED9BB608A27444264066EA9642B4A11F476959` |
| `...InteractionRequestCoordinator_attempt-1.log` | 6 Success、0 Fail/Fatal，最后测试已开始但无终止标记 | 单次重跑后 7/7 | `C480AF729A839BB8A97C051549E14789D3475E21CA0AE070523137DDD9920377` |

宽泛父组在映射修正前曾运行到 771 Success；它不是产品失败，
但因属于重复主题覆盖且没有完成，不作为最终证据。最终只采用 101 份精确组日志。

## 10. 构建、文件与交接

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `P25.5_EditorBuild_final.log` | Editor Succeeded | 0 | `8BA5763414E6D982A61BB22533545FFC52CF9E997935492F08D2011740677FDA` |
| `P25.5_GameBuild_final.log` | Game Succeeded，50 个动作 | 0 | `511AB8F5FF1BD1820E00838975DC1E84551DCDC7552472E8A990C232F1D6FD9F` |

- `git diff --check`：PASS；
- Regression Map JSON：248 条规则，PASS；
- 纯展示依赖扫描：0 命中；
- 最终项目相关 Unreal 进程：0；
- Editor DLL：19151872 bytes / `ECA6740F5EE1A469176A7F76DEC130DA928B6A03BD96938CBBE238039548EB7B`；
- Game EXE：359853568 bytes / `EAEF2C41F342EBD42C2A14230501656EE060BF1FDB5BF47DA0B1D4D2719BC5E0`。

P 阶段完成；F 阶段未启动 Unreal Editor UI、PIE、Standalone、产品 exe、
真实输入、截图、Smoke、Cook 或 Package。

精确提交文件为 10 个实现/测试/流程文件加本 Report 与本 Log。
103 个用户未跟踪文件不暂存，`Saved/Codex/P25.5` 证据仅在本地保留。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-5-spirit-shield-impact-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-5-spirit-shield-impact-feedback/Docs/Report/Dev.D.UE.0.0.10.P25.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-5-spirit-shield-impact-feedback/Docs/Log/Dev.D.UE.0.0.10.P25.5.r0_log.md>
