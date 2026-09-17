# Dev.D.UE.0.0.10.P28.3.r0 Report

## 1. 状态与范围

状态：`P28_3_VERIFIED`。最小修复、专项、完整新旧根、双目标构建及改动映射核验完成；2026-09-17 01:27 heartbeat 恢复阶段交付，不重复执行已完成验证。此状态不是整个底层框架最终冻结。

本轮只修复 FZ-2 的 GameMode 返回整备边界：Run 释放拒绝时，不继续清除该 GameMode 管理的任务对象与活动标志。未改变资源权威、持久 schema、技能、输入、资产或 UI。Manager 更外层的释放/回滚顺序仍需独立核对。

## 2. 基线及真实反例

- 初始产品基线 P28.2 / `f4e34d277163eb3e6644df9a9fc8907ff05724e9`。恢复本阶段时 HEAD 为 `175a651991b93aabaf2b9c19cd23bb9ab4ba9bef`，中间仅交付总体 Report/Log；保持该文档提交。
- 最终交付恢复 HEAD 为 `a8bc3626eb3d8de0ca7a6e8b3720bbb304e3b0d8`；相对 P28.2 仅新增总体文档提交，1,617 个锁定产品/脚本输入与验证时相同，不把总体报告回退或混入本阶段。
- 先增加一项测试与测试友元，不改生产行为；Red Editor 原生 0。
- 隔离 Profile 实际 cutover、整备并启动带真实飞剑实例的 Run；在瞬态无头 World 中创建飞剑和已注册敌人，发射飞剑并推进非零时间线。
- 复用已有 P27.30 故障方法：仅在测试内将飞剑设为 SimulatedProxy，使引擎实际拒绝 Actor 销毁，不改引擎、不实现联网玩法。
- 两次调用生产 DeactivateV3MissionContentForPreparation。原 ReleaseCombatProductRun 已保留待清理结果，但外层不检查 bool，继续销毁敌人并清空任务标志。原测试 0 Success / 1 Fail，原生 0；两次保持断言及随后销毁计数断言失败，原日志保留。

## 3. 最小实现

1. DeactivateV3MissionContentForPreparation 检查 ReleaseCombatProductRun 的结果；拒绝立即返回，不执行后续任务投影清理。沿用已有下层诊断，不建立新的恢复服务。
2. 保持原 void 接口与正常成功清理流程；同一 owner 重试仍由既有 PendingCombatRunRetirement 续接已成功前缀。
3. M01 激活分支绑定 Run 失败后显式返回 false。否则新保留的活动标志可能让该分支把绑定失败当成功；这是该保护改动必须保持的调用契约，不是新增玩法。
4. 没有将持久结算、逻辑 Run 结束与 World 清理混为一次成功；此函数不替代持久 Finalize。

## 4. 已完成专项

`Shanmen.0_0_10.Product.ControlledWeaponWorldLifecycle`：4 Success / 0 Fail，队列 4、原生 0、崩溃指标 0。

新增 PreparationDeactivationRetention 验证：连续两次拒绝均保留原 Run ID、已结束前缀、飞剑 owner、非零时间线、敌人和活动标志；恢复可销毁条件后只销毁敌人一次，再次调用为空操作；完整 durable snapshot 与 Runtime RunId 不变。

测试直接覆盖 GameMode 的去激活函数及非 M01 清理分支，不是整个 Manager、正式 M01 地图或玩家流程验收。M01 绑定失败返回值补强来自直接调用点审查；不宣称本新增用例走过完整 M01 激活链。

## 5. 回归与构建

- Red Editor：37 actions，144.45 秒，SUCCEEDED/native 0。
- First Fixed Editor 与 Final Fixed Editor 已完成；最后版本 4 actions、11.02 秒、SUCCEEDED/native 0。最终专项使用最后版本。
- Game：36 actions、143.76 秒，SUCCEEDED/native 0。
- 完整旧 demo_map 根：1,330 Success / 0 Fail，队列 1,330、原生 0、崩溃指标 0。
- 完整 Shanmen.0_0_10 根：1,426 Success / 0 Fail，队列 1,426、原生 0、崩溃指标 0；于 2026-09-17 00:13:03 UTC 完成。专项 4 项属于新根子集，不重复累计；两个完整根均使用最后修复源码。
- 三个源码路径命中现有 M01GameMode / ItemProductAdapters，合并要求 81 个组。精确六文件映射通过：Changed=6 / Rules=2 / Required=81 / Logs=2，证据为上述完整新旧根，不只运行本轮四项专项。
- 映射检查器自检原件 517/517，哈希已复核且检查器/映射输入未变；1,617 个锁定输入、103 个既有用户文件原字节保持。恢复交付只复查最终证据，不冒充重新运行 UE。

## 6. 剩余边界与 P/F

FZ-2 未整体关闭。Manager.DeactivateProfileWorld 仍在调用 GameMode 之前清理部分拾取物/容器，之后调用 Items.TeardownWorld；其技术回滚、激活失败、结算和 EndPlay 调用点的顺序及结果传播需继续审计。引擎强制 EndPlay 不是同进程可恢复保留的等价场景。

FZ-1 的剩余丢弃/存箱/未整备新获物政策边界和 FZ-3 最终冻结证据也未完成。不得把局部 GameMode 保持测试当作全世界原子恢复、正式可玩或最终冻结。

只允许本阶段三个源码、冻结索引、本 Report/Log 精确交付。103 个既有未跟踪用户文件不属于提交范围；Saved 原日志仅保留本地，Log 提供路径和 SHA-256。未接物理输入、未改正式地图或资产、未启动 Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook/Package。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.3.r0_log.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
