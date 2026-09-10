# Dev.D.UE.0.0.10.P25.5.r0 Report

## 1. 结论

P25.5 已把灵力护盾的权威受击结算转成短时、可读的玩家反馈。
当一次敌方攻击首次真实扣除护盾容量时，MainHUD 会显示本次吸收量与剩余容量；
容量耗尽时改为明确的 `DEPLETED` 提示。

反馈只接受现有 Impact 提交结果中的权威容量回执，不重新计算伤害、不保存第二份容量，
也不把自由格式 `Diagnostic` 文本显示给玩家。精确重放、较早防御层已拦截、
原子提交拒绝和无效证明均不会打开反馈窗口。

## 2. 玩家可观察行为

- 部分吸收：`SPIRIT SHIELD · ABSORBED 12 · 18 LEFT`，青色；
- 容量耗尽：`SPIRIT SHIELD · ABSORBED 30 · DEPLETED`，琥珀色；
- 显示时间：1.25 秒；
- 位置：MainHUD 中央 `Y=206`，与 P25.4 激活反馈的 `Y=166` 分离；
- 精确重放不会重复提示；
- 闪避、完美格挡等较早防御层已结束本次 Impact 时不会伪造护盾吸收；
- 下游提交失败或证明无效时不会显示成功反馈。

## 3. 权威回执边界

新增 `Fdemo_mapShanmenSpiritShieldImpactFeedbackPresentation`，唯一输入为
`Fdemo_mapShanmenSpiritShieldImpactCommitResult`。
投影先要求 `DidConsumeCapacity()`，再读取有效
`FShanmenSpiritShieldCapacityCommitReceipt` 的：

- `GetCommittedCapacity()`；
- `GetCapacityAfter()`；
- `IsDepleted()`。

因此展示值与 P25.2 的容量权威提交完全一致。
投影不读取 Request、预估伤害、当前 HUD 缓存或产品诊断字符串；复用输出会先清空，
无效输入失败关闭。

## 4. 六条敌方攻击接线

`Ademo_mapGameMode::PublishSpiritShieldImpactFeedback()` 在既有敌方攻击结果完成后，
只向受击玩家自己的 `Ademo_mapPlayerController` 发布首次容量提交结果。
已覆盖六条现有入口：

1. 基础近战；
2. 近战冲刺接触；
3. 远程投射物命中；
4. 重型扇区攻击；
5. Boss 形状攻击；
6. Boss 齐射投射物命中。

发布门同时要求攻击已执行、护盾已被检查且容量确实发生提交。
PlayerController 仅冻结该结构化结果和 1.25 秒展示截止时间；护盾会话仍是唯一状态权威。

## 5. MainHUD 展示

MainHUD 在 P25.4 激活反馈后绘制本提示。部分吸收使用青色面板，容量耗尽使用琥珀色面板；
文本由纯投影固定生成。HUD 不提交 Impact、不推进战斗时间线、不扣除容量，
也不依赖第二个 Widget、Actor、Component 或 Subsystem。

## 6. 自动化证明

本轮关键三组最终结果：

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `SpiritShieldHUDPresentation` | 8 | 0 | `0DE2C82577F102614D456F04D51FBEC415F79EAF0C7B6A8ADD4659A0155E35BC` |
| `SpiritShieldPhysicalInput` | 3 | 0 | `4429AD29654142D52B0DE85FAF8DA44C0725DF83A791F76D838E9A08DAA78259` |
| `SpiritShieldProductSession` | 9 | 0 | `7A576C805E45E3A47C760347938A71D84E8A9AE59942073A135BCCF25B033813` |

新增或强化的证明包括：部分吸收精确值、耗尽反馈、重放不重复、较早防御不触发、
原子拒绝不触发，以及无效 Impact 证明不能打开控制器反馈窗口。

## 7. 改动文件驱动回归

10 个改动路径命中 6 条映射规则，要求 101 个精确测试组。
本轮从 `M01GameMode` 规则删除了与这些精确组重复的宽泛父组 `Shanmen.0_0_10`；
不再用一轮 1300 项主题父套件重复代替路径映射证据。

| 范围 | Groups | Success | Fail |
|---|---:|---:|---:|
| 旧 `demo_map` 回归 | 10 | 230 | 0 |
| CombatCore | 1 | 9 | 0 |
| CombatRuntime | 6 | 178 | 0 |
| Items | 1 | 77 | 0 |
| Spirit Shield 产品链 | 3 | 20 | 0 |
| 其余 0.0.10 产品与 World | 80 | 534 | 0 |
| 合计 | 101 | 1048 | 0 |

覆盖门结果为 `PASS Changed=10 Rules=6 Required=101 Logs=101`，
日志 SHA-256 为 `133DF64B978C7E7291E5EC114209DFCCAD8041B8F137B2C4E2D1BBEE61C8C1FE`。
101 份最终日志清单的组合 SHA-256 为
`C832AF6CBD31E3349C80B6070C94D5F36ED2DB6CC0DAD4B9A1D67448A0517F3B`。
映射器自检 448/448 PASS，SHA-256 为
`6328D811B01534276A7F15365654B6C6B2B8F2ED1B3AE91739D68B1390F6B7BE`。

## 8. 首错与有界修复

1. Editor 首次调用把项目路径错误展开为 `\demo_map.uproject`，在源码编译前以
   `OtherCompilationError` / 原生退出码 6 终止；改用显式绝对路径后成功。
   首错 SHA-256：`01E30AC9034AB3788FDAECE4CA9870BEA685C8F8086D2802756F379048C22829`。
2. 三个独立测试进程分别在最后一项测试已启动时提前结束：已有 6 Success、
   0 Fail/Fatal，但缺少最终成功与终止标记，因此均未计作通过。
   每组只进行一次有界重跑，最终均为 7/7 Success 且具有原生终止标记。
3. 三份首轮基础设施证据 SHA-256 分别为
   `D954162A5A101F7F1ED8EB439FF223DE029A1BDAD8A934BA0CBB3E330ACA7969`、
   `628330ADCFCB01012B3A49419DED9BB608A27444264066EA9642B4A11F476959`、
   `C480AF729A839BB8A97C051549E14789D3475E21CA0AE070523137DDD9920377`。

没有降低成功、失败、Fatal、终止标记或联网探针判据。

## 9. 构建与静态检查

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| `demo_mapEditor Win64 Development` | Succeeded | 0 | `8BA5763414E6D982A61BB22533545FFC52CF9E997935492F08D2011740677FDA` |
| `demo_map Win64 Development` | Succeeded | 0 | `511AB8F5FF1BD1820E00838975DC1E84551DCDC7552472E8A990C232F1D6FD9F` |

- `git diff --check`：PASS；
- Regression Map JSON：PASS（248 条规则）；
- 纯展示投影对 `Diagnostic`、`UWorld`、`AActor`、`GetWorld` 与 RNG：0 命中；
- 最终项目相关 Unreal 进程：0；
- `UnrealEditor-demo_map.dll`：19151872 bytes，SHA-256
  `ECA6740F5EE1A469176A7F76DEC130DA928B6A03BD96938CBBE238039548EB7B`；
- `demo_map.exe`：359853568 bytes，SHA-256
  `EAEF2C41F342EBD42C2A14230501656EE060BF1FDB5BF47DA0B1D4D2719BC5E0`。

## 10. P/F 边界与 GitHub 交接

P 阶段已证明：权威容量回执投影、部分吸收、耗尽、重放/拒绝/较早防御栅栏、
六条攻击入口、控制器生命周期、MainHUD 接线、101 组路径映射回归及双目标构建成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、
截图、Smoke、Cook 或 Package，因此不声明已目视确认布局、颜色或实机手感。

基线提交：`1963601eeaa9431778f162026279d4c76ab9f307`（P25.4）。
分支：`agent/0.0.10-p25-5-spirit-shield-impact-feedback`。
只提交本阶段 10 个实现/测试/流程文件、本 Report 与本 Development Log；
用户原有 103 个未跟踪文件保持未暂存，`Saved/Codex/P25.5` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-5-spirit-shield-impact-feedback>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-5-spirit-shield-impact-feedback/Docs/Report/Dev.D.UE.0.0.10.P25.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-5-spirit-shield-impact-feedback/Docs/Log/Dev.D.UE.0.0.10.P25.5.r0_log.md>
