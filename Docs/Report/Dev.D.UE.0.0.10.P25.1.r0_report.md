# Dev.D.UE.0.0.10.P25.1.r0 Report

## 1. 结论

P25.1 已把 P25.0 的共享 Run 级 `SpiritEnergy` 账本接成第一条可操作的灵力护盾产品链：玩家按下可重映射的 `H` 后，经统一动作仲裁与 Combat Run 身份发放，原子支付 20 点灵力并建立 30 点护盾容量；护盾在 30 Hz 固定时间线的第 90 tick（3 秒）自动关闭。

这不是第二套灵力或第二套护盾。产品会话直接拥有既有 `FShanmenSpiritShieldSession`，支付复用 Divine Sense Product Controller 的唯一资源权威，Run 结束时先释放护盾再关闭共享灵力 Host。

## 2. 玩家行为

- 新增输入动作 `SpiritShield`，默认键为 `H`，中文标签“灵力护盾”；
- 一次按键只路由一次 `PlayerController -> GameMode -> ProductSession`；
- 有效 Run 中首次激活：灵力 `100 -> 80`、容量 `30`、期限 `90 ticks`；
- 护盾活动期间重复按键失败关闭，不二次扣费、不消耗新的护盾激活序号；
- 灵力不足时，资源余额、共享账本和护盾状态全部保持原状；
- 到期后自动关闭，可再次激活并使用下一确定性身份；
- Run teardown 会精确关闭并清空仍活动的护盾所有权。

## 3. 产品会话与原子性

新增 `Fdemo_mapShanmenSpiritShieldProductSession` 作为唯一产品所有者。激活过程先在候选副本中完成：

1. 验证活动 Combat Run 与固定时间线；
2. 经 `PlayerActionArbitration` 授权瞬时护盾动作；
3. 从 Run 单调序号生成 Action、Transaction 与 Command 身份；
4. 捕获固定的护盾定义、灵力成本和 90-tick Schedule；
5. 在共享 SpiritEnergy Controller 副本中执行 Shield `Begin + Commit`；
6. 同时验证资源 Receipt、护盾 Session 与产品 Session；
7. 只有全部成立才一并发布资源 Controller 与护盾产品状态。

因此任何身份、资源、Schedule 或 Session 错误都不会留下半扣费或半激活状态。

## 4. Combat Run 与生命周期

Combat Run Coordinator 新增独立的 `NextPlayerSpiritShieldActivationSequence`，Run 结束和 Reset 都回到 1。护盾 Action 固定使用玩家 Source、规范内容版本 `0.0.10.P25.1` 与规则 `Defense.Spell.SpiritShield.Basic01`。

护盾激活是瞬时动作，不长期占用玩家互斥动作槽；持续防御由 Shield Session 自身持有。GameMode Tick 只在当前固定 tick 到达 Deadline 时观察一次期限并关闭，未新增 Timer、Actor 或第二时间源。

## 5. 输入版本迁移

统一输入注册表由 32 项增至 33 项，配置输出版本由 10 升为 11。

首次实现曾选择 `Left Shift`，完整回归立即暴露它违反既有“修饰键保留”规则，并使旧迁移/重映射测试得到不完整绑定表。修复没有放宽校验，而是改用无冲突的 `H`，并新增真实 N-1 迁移：

- 版本 10 未包含护盾且 `H` 空闲时，护盾迁移到 `H`；
- 若用户已把既有动作覆盖到 `H`，保留原覆盖并将护盾分配到释放出的 `G`；
- 迁移后 33 个动作、键合法性与唯一性全部重新验证。

## 6. 自动化证明

新增产品会话 5 项测试，覆盖规范策略、共享账本激活、余额不足原子回滚、固定期限、重复激活、再激活与 Owner 释放。

新增物理输入 3 项测试，覆盖 33 项精确注册表、版本 10 到 11 迁移，以及真实 `H` Press 只派发一次。所有直接受影响的旧物理输入组也逐组复测。

正式 `Shanmen.0_0_10` 全量结果：1287 Success / 0 Fail，原生退出码 0；日志 SHA-256 为 `6FE9BFDCBEE56A1D540CABC4D3D140D658E23208AC4F6AFEBABA96278510ACEE`。

完整回归使用命令行只读覆盖 `HomeScreen.EnableHomeScreen=0`，避免 UE 5.8 无界面 Editor Home Panel 对 `generate_204` 的无关联网重试；没有修改引擎文件、Windows 权限或防火墙，测试选择与断言保持不变。

## 7. 改动文件驱动回归

本轮把新增护盾产品会话和物理输入文件加入 `ShanmenRegressionMap.json`，并给覆盖器自检补正反例。改动文件要求的 `Shanmen.*` 组由完整 `Shanmen.0_0_10` 覆盖，旧 `demo_map.*` 依赖按精确组单独执行。

覆盖门禁：`PASS Changed=31 Rules=11 Required=95 Logs=9`。

覆盖器自检：446/446 PASS。

## 8. 构建与首错证据

首次 Editor 构建失败于两处 `UE_LOG`：严重级别使用了条件表达式，而 Unreal 宏要求编译期常量。修复为固定 `Log` 后 Editor 目标成功；原始失败日志保持不变。

首次广覆盖测试发现 12 个旧输入回归，全部由 `Left Shift` 与既有保留键契约冲突引起。该轮在 743 项后停止，完整失败详情原样保留；修复为 `H`、提升配置版本并补 N-1 迁移后，从头执行正式全量，不修改旧断言。

随后旧 `demo_map.FullSystemLoop` 精确复测发现 3 个测试夹具仍把 `H` 假设为空闲重映射键。此时 `H` 已是护盾的规范默认键，重复键拒绝属于正确行为。修复只把测试夹具的任意空闲键改为 `K`，没有修改生产绑定或放宽唯一性校验；失败日志保留，重新编译后该组 50/50 通过。

最终构建：

- `demo_mapEditor Win64 Development`：Succeeded，原生退出码 0；
- `demo_map Win64 Development`：Succeeded，原生退出码 0。

## 9. P/F 边界与下一步

PASS：物理输入注册与派发、动作仲裁、Run 身份、共享灵力原子支付、活动期幂等、余额不足回滚、90-tick 到期、再激活和 Run 释放均已有无头证明；文件驱动回归、双目标构建和静态门禁完成。

未声明：护盾已拦截敌方伤害。P25.1 只完成“玩家启动、支付、持有、到期”闭环；尚未把活动容量投影到每类敌方 Impact，也没有 HUD 容量呈现。P25.2 应在 ProductSession 内提供受击提交入口，并接入所有敌方 Impact 路径，禁止外部直接改写底层容量。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. GitHub 交接

基线提交：`1ea933ef3ab237e05e03df2c65edacf40036d6ca`。

工作分支：`agent/0.0.10-p25-1-spirit-shield-activation`。

本阶段只提交精确实现/测试/回归映射文件、本 Report 与本 Development Log；用户原有 103 个未跟踪文件保持未暂存，`Saved/Codex/P25.1` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-1-spirit-shield-activation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-1-spirit-shield-activation/Docs/Report/Dev.D.UE.0.0.10.P25.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-1-spirit-shield-activation/Docs/Log/Dev.D.UE.0.0.10.P25.1.r0_log.md>
