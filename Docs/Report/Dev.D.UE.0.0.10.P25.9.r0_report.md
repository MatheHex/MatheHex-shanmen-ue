# Dev.D.UE.0.0.10.P25.9.r0 Report

## 1. 结论

P25.9 已让灵力护盾的 World 光域连续反映权威剩余寿命。点光范围从激活时的 `220`
随固定 Combat Run 时间线收缩到截止前的安全下限 `120`；半程精确为 `170`。达到
Deadline 时，即使 Product Session 尚未执行关闭观察，表现适配器也会立即隐藏护盾，
避免一帧陈旧可见状态。

P25.7 的容量颜色、P25.8 的容量罩体尺寸保持独立：容量只控制青/琥珀提示与罩体缩放，
剩余时间只控制光域半径。本轮没有新增墙钟、Timer、时间副本、Actor、组件或业务权威。

## 2. 玩家可观察行为

- 激活起点：护盾点光范围 `220`；
- 90 tick 生命周期半程（tick 45）：点光范围 `170`；
- 越接近 Deadline，光域越紧凑，但保持不低于 `120`；
- Deadline 样本：能量罩与点光立即隐藏，光域复位为 `220`，等待下一次合法激活；
- 低容量仍显示琥珀色和更高强度，时间收缩不覆盖容量告警；
- 容量罩体缩放、吸收量、SpiritEnergy 成本、伤害、输入与生命周期规则均未改变。

## 3. 固定时间线投影

World Presentation 每帧只消费 GameMode 现有固定时间线的一份冻结样本：

```text
remaining = clamp((deadlineTick - currentTick)
                  / (deadlineTick - startTick), 0, 1)
lightRadius = lerp(120, 220, remaining)
```

仅当 Sample 有效、TimelineId 与 Session Schedule 一致、`currentTick >= startTick` 且
`currentTick < deadlineTick` 时允许显示。样本为空、无效、来自其它时间线或早于起点时，
适配器复位外观并失败关闭；达到 Deadline 属于合法到期投影，隐藏但不擅自修改 Session。

## 4. 权威与集成边界

- PlayerController 从 `GetCombatRunFixedTimeline()` 捕获一次冻结 Sample，再与唯一 Product
  Session 一起交给既有无状态适配器；
- 适配器没有成员字段，不保存 current/start/deadline、半径或历史 Sample；
- 只读取 Schedule、容量和 Sample Getter，不调用 Session 的 Try/Commit/Observe/Reset；
- 不读取 `UWorld`、墙钟、帧号或随机数，不建立第二条时间线；
- Deadline 到达时只关闭视觉，不替代 GameMode 对 Product Session 的权威关闭；
- OnUnPossess 继续使用空 Session/Sample 同步隐藏，避免 Pawn 所有权泄漏。

## 5. 聚焦自动化证明

精确组 `Shanmen.0_0_10.Product.SpiritShieldWorldPresentation`：

| Test | Result |
|---|---|
| `GeometryFailClosed` | Success |
| `ActivationAndRelease` | Success |
| `DeadlineClosure` | Success |
| `LifetimeRadius` | Success |
| `CapacityGeometry` | Success |
| `CapacityTone` | Success |
| `CapacityDepletion` | Success |

新增 `LifetimeRadius` 证明：起点 `220`、半程 `170`；外来 TimelineId 隐藏且返回失败；
恢复权威样本后重新显示；Deadline 时 Session 仍为 Active 的情况下，视觉先行隐藏而不改写
Session。最终 7 Success / 0 Fail / Queue Empty；日志 SHA-256：
`06699D2BD668B2FD2E95F3C69EAE67E38A47C31FC4022844D39D330732789D2D`。

## 6. 改动文件驱动回归

加入 Report/Log 后共 8 个提交路径；6 个实现、测试与映射路径命中 2 条规则，文档为忽略
路径。PlayerController 的交叉输入面与 World Presentation 的新时间线依赖共同要求 30 个
精确组：

| 范围 | Groups | Success | Fail |
|---|---:|---:|---:|
| World / Shield / Timeline | 4 | 24 | 0 |
| `InputRestore` + `V2RangedCompatibility` | 2 | 123 | 0 |
| 其它 Controller 输入与武器交互链 | 24 | 185 | 0 |
| **合计** | **30** | **332** | **0** |

覆盖门最终结果：`PASS Changed=8 Rules=2 Required=30 Logs=30`；证据 SHA-256：
`5346ACE9EF0320697BF50ADAF421C28D16EDB411FF86BDDC6AF1D189DC34CB8C`。World Presentation 映射已明确加入
`Shanmen.0_0_10.Product.CombatRunFixedTimeline`，映射器自检 451/451 PASS；SHA-256：
`B57B2D40AF9919FC707608DBD5B14F5BE11635C928D15A2C37E954681424FAA7`。

## 7. 构建与静态检查

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| `demo_mapEditor Win64 Development` | Succeeded | 0 | `FC3945534AE799C0852DE04E84D8177A9B246E8CE299661C8EE39E63345AF437` |
| `demo_map Win64 Development` | Succeeded | 0 | `9769FD3376701E4C3194F49D01070706B556324042BD975498CF6C4C81BFEBCE` |

- 首次 Editor 编译一次通过；日志 SHA-256：
  `209F7B3B7C62966AB37E2ED1A02AF1FC6E719EB72FC2550F9D91C33572D49694`；
- `git diff --check`：PASS；Regression Map：schema 1 / 249 rules；
- 适配器对 World/Timer/RNG、Session 写操作、Timeline 非 Getter 操作：0 命中；
- 最终项目相关 Unreal 进程：0；
- `UnrealEditor-demo_map.dll`：19196928 bytes，SHA-256
  `40233B81948DB3CF7363563B055A993B0EB60A02150AF3AFFB6FE68732B82881`；
- `demo_map.exe`：359891968 bytes，SHA-256
  `3179FA3BA59679D465F66C0F4571DC426092B1A7951DF75A56AB441ADBA033B9`。

## 8. 首错与修复

首次 Editor 编译、聚焦组、29 个补充回归组及最终双目标构建均一次通过，没有产品或源码
失败。覆盖门在只提供聚焦日志的预检中按设计列出其余 29 个必跑组；补齐全部独立日志后
通过，这不是测试失败，也没有隐藏或覆盖任何首错证据。

## 9. P/F 边界

P 阶段已证明：时间线身份校验、起点/半程半径、截止先行隐藏、容量表现正交、控制器交叉
回归、覆盖门、静态检查及 Editor/Game 双目标构建成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package，因此不声明光域变化的最终画面层次或实机可辨识度已经目视验收。

## 10. GitHub 交接

基线提交：`557b530f6c405b39bfb94ea7590b45124303e54b`（P25.8）。
分支：`agent/0.0.10-p25-9-spirit-shield-deadline-visual`。只提交本阶段 6 个实现/测试/映射
文件、本 Report 与本 Development Log；全部其余未跟踪文件保持未暂存，
`Saved/Codex/P25.9` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-9-spirit-shield-deadline-visual>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-9-spirit-shield-deadline-visual/Docs/Report/Dev.D.UE.0.0.10.P25.9.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-9-spirit-shield-deadline-visual/Docs/Log/Dev.D.UE.0.0.10.P25.9.r0_log.md>
