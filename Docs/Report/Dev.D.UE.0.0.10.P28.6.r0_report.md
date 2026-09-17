# Dev.D.UE.0.0.10.P28.6.r0 Report

## 1. 当前状态

`P28_6_VERIFIED`。最小组合修复、Editor/Game 双构建、专项6/0与5/0、完整旧根1330/0、新根1429/0和改动映射门均通过。不是整体底层冻结或 F 阶段验收。

从 P28.5 / `83f1ec142b627832fa016236907c101312b9fb2f` 继续既有 FZ-2：修复前 Manager 技术回滚将下层 Runtime 清理成功视为整个回滚成功，即使 GameMode 世界释放拒绝；下次尝试时 Flow 已离开 RunActive，不能重新提交下层回滚。本阶段先增加真实拒绝反例，再修复同一 Run 的完成确认与续接。

## 2. 验证范围

新增 `Shanmen.0_0_10.Product.ControlledWeaponWorldLifecycle.ManagerRollbackContinuation`，使用隔离 Profile、实际 cutover/Flow、真实飞剑实例、非零时间线和临时 World 的 AuthGameMode。两次释放拒绝后恢复，要求上层不得提前确认、原 Run 保留、Runtime 回滚只提交一次、世界只释放一次、持久快照不变。

测试不声称 Runtime 内容在其自身成功回滚之后仍保留。Runtime 清理已被接受与世界 owner 尚未释放是两个阶段；需要恢复的是后者，而非回滚前的物品投影。

## 3. 修复与完整证据

Red Editor 51 actions / 393.85秒 / native0。RedProof 实际0/1、队列1/native0、崩溃指标0：首轮错误确认、待清理标志丢失、恢复后仍不能释放、武器与敌人销毁次数0。Runtime只回滚一次且持久Run未变的断言未失败。完整失败原件保留。

最小组合修复已通过 Editor 构建（33 actions / 277.01秒 / native0）、Game构建（52 actions / 358.07秒 / native0）、ControlledWeaponWorldLifecycle 专项6/0、ProductFlow专项5/0及完整旧根1330/0、新根1429/0。各测试都有精确队列结束、native0和崩溃指标0；专项包含在完整新根内，不重复累计。

- Manager 的去激活返回 bool；技术回滚仅在 Runtime 前缀与世界释放均完成后返回 true。只保存单个瞬态续接记录，不缓存整份权威快照；身份/Flow/Runtime/Session/World/存储根或下层提交次数不匹配时拒绝，不丢原记录。
- 两类 Start/Activate 拒绝未完成清理；GameMode 的 Prepare 不再先擦除待恢复 correlation。
- 启动协调器的 TechnicalStartFailure 请求续做原尝试清理，成功后只到 AtSect，本次仍返回未启动，不创建新 Run、不恢复输入。
- 同一新增测试扩展为 Coordinator → M01Adapter → GameMode → Manager → Flow/World 组合，并验证绑定不匹配拒绝及上层身份保持。夹具绑定既有启动尝试，不运行正式 M01 BeginActivation，不冒充完整玩家启动验收。
- 启动协调器/M01适配器添加精确文件映射，要求完整双根。扩展后的映射自检529/529/native0；早先517项原件仍保留，不覆盖。

新根自20:30:57至22:08:43UTC连续运行约97分46秒，未拼接部分结果。运行期间保留输入锁，HTTP探测超时Warning和大delta提示不冒称为测试失败或全日志无警告。

精确13路径（8个C++文件、2个脚本/映射文件、索引、Report/Log）映射 PASS：Changed=13 / Rules=4 / Required=85 / Logs=2。使用本轮完整双根，不以主题专项代替所有改动的回归。实际原件路径/SHA见Log；包括首次RedProof失败，均保留本地，不声称原始日志已上传GitHub。

完成后复核1617个锁定产品/脚本输入与103个原未跟踪用户文件均未变，两份OverallReadiness用户修改保持原SHA。未重新运行已完成的构建或测试；仅本阶段13文件进入提交，最终暂存校验、文档链接和保护检查见Log。

## 4. 边界

只修既有回滚组合，不新增持久权威、schema、玩法或通用恢复服务。一般激活失败、终局和强制 EndPlay 仍在有限审计范围内，本阶段不宣称全部世界清理原子化。

不启动 Editor UI、PIE、Standalone、游戏程序；不改正式地图/内容，不接物理输入，不做 UI/数值/敌人行为开发、截图、Smoke、Cook 或 Package。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.6.r0_log.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
