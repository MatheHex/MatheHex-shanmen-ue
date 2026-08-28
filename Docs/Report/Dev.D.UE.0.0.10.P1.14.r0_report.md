# Dev.D.UE.0.0.10.P1.14.r0 Report

## 结论

P1.14 已完成：0.0.10 产品 Start、RuntimeReady、world-confirmed audit 与技术回滚不再依赖 Code B `FCodeBLoadoutSelection` 或 Code B active-session 写入。产品链路现在只携带从 ShanmenItems durable ledger 重建的不可变 `Fdemo_mapShanmenRunCorrelation`。

最终验证：

- ProductFlow 定向自动化：2/2 Success；
- `Shanmen.0_0_10` 完整自动化：66/66 Success；
- Editor Development：成功，原生退出码 0；
- Game Development：成功，原生退出码 0；
- `git diff --check`：原生退出码 0；
- 产品 Start 三个入口中的旧 selection / world-confirmed writer / rollback probe：0 命中。

## 功能性

### 1. Authority-native Run correlation

新增 `Fdemo_mapShanmenRunCorrelation`。它不建立第二份持久化文档，而是只从以下既有 durable evidence 重建：

- prepared request / receipt；
- lifecycle request / receipt；
- ActiveRunId、OwnerId、ScopeId；
- prepared 与 lifecycle authority revision；
- 五个装备身份；
- 按 receipt 顺序冻结的全部 prepared item；
- RunInventory 顺序与 9 格 Hotbar；
- 每条 reservation 的 definition、resource kind、amount、purpose 与来源格位。

`CorrelationId` 使用 `FShanmenDeterministicId::FromCanonicalParts` 和固定命名空间 `Shanmen.Product.RunCorrelation.r1` 生成。相同 durable ledger 在 Runtime 重启、恢复与技术回滚后得到相同 correlation；篡改顺序或任一 canonical line 会改变 identity。

结构验证拒绝无效或重复 prepared item、脱离 prepared set 的 equipment / RunInventory、以及不属于 RunInventory 的 Hotbar 绑定。

### 2. Read-only recovery probe

`Fdemo_mapShanmenPreparationAdapter::TryInspectActivePreparedLoadout` 以 snapshot 重建唯一未终结 prepared receipt 与 lifecycle receipt，不发 command、不增加 authority revision。

`Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation` 在此基础上构建完整 correlation；原 `TryFindRecoverableActiveRun` 改为该完整探针的窄 ActiveRunId 投影，避免两套恢复扫描逻辑分叉。

### 3. Product Start cutover

- 0.0.9B framework 的 Start 按钮不再刷新或捕获 Code B warehouse selection；
- M01 adapter 从只读 Profile presentation 取得预期 Owner，实际 Run 身份由 Shanmen atomic Start 返回；
- GameMode 在 Run materialize 后立即从 authority 重建 correlation，并要求 Owner / ActiveRun 完全匹配；
- coordinator 的 attempt、runtime receipt 与 start diagnostic 均携带同一 correlation；
- RuntimeReady 必须逐字段等于 attempt correlation，且继续满足 map、world、GameMode、controller、pawn 与 input readiness；
- Editor audit 接受技术失败返回 AtSect 后保留同一 durable recoverable Run，而不再误判为身份泄漏。

### 4. World confirmation 与 terminal 写入边界

world confirmation 只执行两次 identity comparison：

1. 与 GameMode 在 prepare 时冻结的 correlation 比较；
2. 与当前 authority ledger 重新构建的 correlation 比较。

比较成功只写诊断日志，不调用 Code B writer。技术回滚也不再读取 Code B active session，而是要求：已创建 Run 时 recoverable RunId 必须等于 attempt RunId；未创建 Run 时不得凭空出现 recoverable identity。

V3 兼容入口仍保留旧 Code B 实现供历史 / 自动化路径使用，但 `UsesShanmenItemLifecycle()` 的产品 activation 与 terminal observer 会在 writer 前返回，明确记录 `PostActivationNoLegacyWrite` / `PostTerminalNoLegacyWrite`。

## 完整性

自动化在既有两个 ProductFlow case 中增加以下不变量：

- atomic Start 后连续两次只读重建得到完全相同的 correlation；
- 两次 probe 前后 authority document 字节语义不变；
- Runtime materialization failure 后仍可读取同一 correlation；
- retry、technical rollback、再次 retry 均保留完全相同的 correlation 与 ActiveRunId；
- terminal finalize 后 active correlation 必须消失；
- retired Profile bytes 全周期不变；
- atomic Start 仍为一次 `StartPreparedRun`，没有 `CommitBatch + ClaimPreparedRun` 回退。

测试总数保持 66；本阶段强化既有 ProductFlow 用例，没有制造只验证重复表面的新 case。

## 兼容性

- 未删除旧 Code B 文档、reader、UI 或 automation fixtures；
- legacy lifecycle 在没有 Shanmen authority cutover 时仍可使用原 observer；
- `ClaimPreparedRun` 历史 receipt 仍能重建 correlation；
- Profile presentation 继续提供 Owner / transient Run UI 状态，但不再作为物品或 loadout 权威；
- Runtime、M01 world、input、terminal HUD 与已有 66 项 0.0.10 自动化全部通过。

## 修改范围

主要新增：

- `Source/demo_map/demo_mapShanmenRunCorrelation.h`
- `Source/demo_map/demo_mapShanmenRunCorrelation.cpp`

主要调整：

- Shanmen preparation / lifecycle adapter；
- Profile preparation flow 与 ProductFlow automation；
- 0.0.9B framework types、M01 runtime adapter、run coordinator、Editor audit；
- GameMode world-confirmed seam；
- V3 activation / terminal compatibility guards；
- legacy bridge 的参数改为 authority correlation。

未改动 ShanmenCore / ShanmenItems 持久化 schema，未迁移或覆盖用户现有文档。

## 验证记录

### 自动化

- 定向：`Shanmen.0_0_10.Items.ProductFlow`
  - 2 Success / 0 Fail；
  - queue empty；
  - 原生退出码 0；
  - 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.14.r0_targeted_final.log`；
  - SHA-256：`94EB892B43C30E5DEAAA33FAEDBD2C78CC7C3E9987B08758F7C4D918772C92CA`。
- 完整：`Shanmen.0_0_10`
  - 66 Success / 0 Fail；
  - queue empty；
  - 原生退出码 0；
  - 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.14.r0_automation_final.log`；
  - SHA-256：`E14ED6FFA43875CE70B7DE84BC16E2CE61BCDFBC891371A54EB16E491B8C07BD`。

### 构建

统一参数：

`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`

- `demo_mapEditor Win64 Development`：成功，原生退出码 0；首次完整 fan-out 49/49 actions，最终增量 4/4 actions；
- `demo_map Win64 Development`：成功，原生退出码 0；首次完整 fan-out 48/48 actions，最终增量 3/3 actions。

### 中间失败如实记录

1. 首次定向 automation 启动在测试执行前进入 UE 5.8 all-platform SDK validation，并因本机 LinuxArm64 / VisionOS `MainVersion` 元数据缺失返回原生退出码 1。Windows SDK 为 VALID，Editor / Game 构建均正常。改用已构建 Editor 的 `-NoCompile -AbsLog` 路径后测试正常执行；相同非目标平台警告仍可见但不阻塞 Win64 测试。
2. 首次实际 ProductFlow 执行得到 1/2。唯一失败为新增测试在 finalize 之后才读取 active correlation；产品正确返回 false。将读取移到 finalize 之前后，最终 2/2 与完整 66/66 均通过。

### 静态检查

- `git diff --check`：0；
- `demo_map0909BRunStartCoordinator.cpp`、`demo_map0909BFramework.cpp`、`demo_mapGameMode.cpp` 中：
  - `FCodeBLoadoutSelection`：0；
  - `NotifySuccessfulRunWithLoadoutSelection`：0；
  - `VerifyNoActiveRunSession`：0；
- unrelated 未跟踪文件保持原状；提交只使用显式路径暂存。

## P/F 边界

本阶段执行源码开发、静态审查、headless Unreal automation 与必要 Editor / Game 编译。未启动 Unreal Editor UI、PIE、Standalone、产品交互、截图、Smoke、Cook 或 Package。

## 下一步建议

P1 的产品物品 authority / run lifecycle / recovery / correlation 链路已闭合。下一迭代可进入 P2：强化 combat Impact 的 receipt 守恒、damage tag channel、ImpactId 自校验与不可变 action snapshot，再把纯函数契约交给 P3 编排层。
