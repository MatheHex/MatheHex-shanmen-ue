# Dev.D.UE.0.0.10.P1.4.r0 开发日志

## 身份

- 任务：`Dev.D.UE.0.0.10.P1.4.r0`
- 分支：`agent/0.0.10-p1-4-product-lifecycle-binding`
- 起点：`d9914c2c4a7dc1d714c2b676a10bd4794d5a18f2`
- 收口时间：`2026-08-27T22:11:37.3105388Z`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：Unreal Engine `5.8`

## 开工审查

1. P1.3 已建立线程串行、持久化确认、回滚与歧义重开的纯 C++ authority service，但产品没有唯一 UObject 生命周期所有者。
2. Code A 的完整 Profile 只存在于 Session Coordinator；公开 Session Snapshot 不足以执行 P1.1 的逐字段来源核验。
3. Code B 的完整 committed record 由 P5 Warehouse Store 持有；现有公开接口只有 UI projection 与 loadout selection，不提供只读迁移副本。
4. 真实 0.0.9B Framework 在宗门启动时会打开/补建 Code B，局内 P6—P73 路径仍可能继续写 Code B。
5. 因此“现在自动迁移并继续运行旧产品”会形成新旧双真值。P1.4 必须先建立唯一 owner 和显式门，不能为了表面接入而提前切换。

## 设计决策

### owner 与惰性

- 使用 `UGameInstanceSubsystem`，而不是 Code A ItemSubsystem、Profile Coordinator、Code B Warehouse Service 或 Framework Actor。
- GameInstance subsystem 是产品生命周期内唯一对象，跨地图存在；它不把新 authority 嵌入任何旧真值系统。
- `Initialize` 零磁盘 I/O。存储根和 OwnerId 只在显式绑定时冻结。

### existing-first

- `BindExisting` 只读新文档；Missing 只进入 Waiting。
- `BindFromStableLegacy` 先调用 `StartExisting`，只有 Missing 后才检查 legacy。
- Ready、Recovered 或 AlreadyReady 路径不读取 legacy，即使参数损坏也采用 durable authority。
- Recovery 状态禁止 legacy fallback。

### 稳定来源

- Code A 必须是当前 schema、Repository-valid、ReadyForPreparation、无 ActiveRun。
- Code B 必须是 current committed out-of-raid record、无 active P6 session、无局内容器残留。
- 三方 OwnerId 必须一致。
- Profile/Code B 适配器只复制值，不公开写者。

### product cutover 边界

- Subsystem 提供四个 durable 命令，未来新产品调用不得直接组合 Repository/Store。
- 本轮不在 Framework/GameMode 中调用迁移，不修改旧 Run 写路径。
- 下一阶段先做写者清单与 Adapter，再接唯一 cutover coordinator。

## 实现记录

### 新公开类型

- `Edemo_mapShanmenItemAuthorityLifecycleState`
- `Edemo_mapShanmenItemAuthorityBindStatus`
- `Fdemo_mapShanmenItemAuthorityBindResult`
- `Udemo_mapShanmenItemAuthoritySubsystem`

### 绑定结果

- Existing：Opened / Recovered / AlreadyReady。
- First upgrade：CreatedFromLegacy。
- 可修正等待：WaitingForStableLegacy / LegacyNotStable / LegacyRejected。
- 身份防护：BindingMismatch / InvalidRequest。
- 失败关闭：PersistenceFailure / RecoveryRequired。
- `bLegacyInputsRead` 直接证明旧参数是否在 Missing 后被读取。

### 只读 source adapters

- Coordinator 和 Profile Session 增加完整 Profile 值复制。
- Warehouse Service 增加完整 Code B Record 值复制。
- 两者都在自身稳定状态失败关闭。

### durable facade

- Reserve、Commit、Cancel、ReleaseDeployment 都由 Subsystem 转发给唯一 service。
- 非 Ready 或非 Game Thread 不进入 service。
- Service Recovery 会同步到产品 owner 状态。

## 自动化记录

新增 7 项：

1. `LazyGameInstanceOwner`
2. `StableLegacyBinding`
3. `ExistingWinsBeforeLegacyRead`
4. `ActiveRunAndIdentityGates`
5. `BindingAndRecoveryFailClosed`
6. `DurableCommandsAndRestart`
7. `StableSourceCaptureAdapters`

验证覆盖：

- GameInstance 唯一实例与构造零 I/O。
- Missing probe 不读 legacy、不写 authority。
- Profile/Code B 稳定来源发布 generation 1，旧输入逐字段不变。
- malformed legacy 不影响已有 authority。
- 活动 Run、Owner mismatch、改绑和损坏文档失败关闭。
- Reserve/Commit 通过产品 owner 持久化并跨 GameInstance 重启精确重放。
- Session 与 Warehouse 只读捕获边界。

## 首次失败、自查与修正

- Editor 首编：33/33 actions，退出码 `0`。
- P1.4 专项首轮：7/7 Success，退出码 `0`。
- 完整回归首轮：44/44 Success，退出码 `0`。
- Game 构建：30/30 actions，退出码 `0`。
- 本轮没有源码、测试或环境导致的首次失败，不需要修复循环。

## 验证记录

### Editor

- 33/33 actions，`Result: Succeeded`。
- 原生退出码 `0`，总时间 133.02 秒。

### Automation

- 专项：7/7 Success、0 Fail。
- 完整：44/44 Success、0 Fail。
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.4.r0_automation.log`。
- 最终 SHA-256：`7F7F2EA97E6119EB77F8901EBCB0EF63F9215C9E8C66BFD68E7559625D2637BF`。
- 专项 SHA-256：`0D04137014C964EB29E82CD74E5EB8D2D2596BBD8A188D17628D319166916B5C`。

### Game

- 30/30 actions，`Result: Succeeded`。
- 原生退出码 `0`，总时间 109.99 秒。

### 静态

- `git diff --check`：退出码 `0`。
- 新 owner 中 World/Actor/UI/GAS/随机数：0 匹配。
- 新 owner 中旧 Profile/Code B 写 API：0 匹配。
- 产品自动迁移调用：0；仅定义与自动化调用。

## 交付边界

- 不自动切换当前产品 authority，不双写旧系统。
- 不启动 Editor UI、PIE、Standalone 或产品可执行文件。
- 不执行真实输入、截图、Smoke、Cook 或 Package。
- 不上传 Saved 自动化日志。
- 只暂存本轮 9 个源码文件与 Report/Log；工作区其它未跟踪文件保持原状。

## 下一步

P1.5 应执行产品 cutover adapters：列出所有 0.0.9B Code A/Code B 写者，先令迁移后的旧来源不可再变化；把新的仓库、携带、消耗、拾取与结算资源事务改经 `Udemo_mapShanmenItemAuthoritySubsystem`；完成端到端无头验证后，才在唯一协调点调用 `BindFromStableLegacy`。
