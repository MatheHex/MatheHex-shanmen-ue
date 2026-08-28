# Dev.D.UE.0.0.10.P1.14.r0 Development Log

## 目标

关闭 P1.13 留下的最后一段产品 Code B audit 依赖：由 ShanmenItems durable receipt 直接提供 Run-start correlation，使 start、world confirmation、recovery 与 rollback 不需要重新打开或写入旧 warehouse 文档。

## 审查结果

P1.13 的资源权威已经是 ShanmenItems，但产品协调器仍携带整份 `FCodeBLoadoutSelection`：

- framework 在 Start 前刷新 Code B warehouse 并捕获 selection；
- M01 adapter 用 selection 取得 Owner；
- coordinator 用 revision / digest 做 RuntimeReady audit；
- world confirmation 尝试调用 Code B P6 writer；
- technical rollback 再读取一次 Code B active session。

authority cutover 已禁止这些产品写入，因此旧调用最多只产生 rejected audit，并不能提供新的真值。真正需要的是 Owner、ActiveRun 与冻结 prepared identities，且这些都已存在于 atomic Start receipt、reservation ledger 与 prepared receipt 中。

## 实现过程

### 1. 新 correlation contract

新增 `demo_mapShanmenRunCorrelation`，将 prepared / lifecycle identity、authority revisions、equipment、RunInventory、Hotbar 与完整 ordered line metadata 规范化为 deterministic ID。

`IsValid()` 额外检查：

- prepared item 全部有效且不重复；
- equipment 必须属于 prepared set；
- RunInventory 必须属于 prepared set 且不重复；
- Hotbar 非空项必须属于 RunInventory；
- lifecycle revision 不早于 prepared revision。

### 2. 单一恢复扫描

Preparation adapter 暴露只读 active receipt inspection；lifecycle adapter 基于它构建 correlation。原只返回 GUID 的 recovery probe 改为完整 probe 的投影，删除重复 ledger scan。

### 3. Coordinator / Runtime / GameMode

- Start API 删除 selection 参数；
- M01 preflight 从 presentation 读取 Owner，随后只相信 authority Start 返回的 correlation；
- runtime receipt 与 diagnostic 记录完整 correlation；
- RuntimeReady 要求 receipt correlation 与 attempt correlation 完全相等；
- GameMode prepare 再读 authority 并冻结 correlation；
- world confirmation 重新读取并比较，不调用 legacy writer；
- technical rollback 验证 same RunId recovery，不读取 Code B session；
- retained recoverable Run 在 AtSect audit 中为合法状态。

### 4. Compatibility writer fences

旧 Code B reader / fixtures 未删除。V3 activation 和 terminal observer 在 Shanmen lifecycle 下先做 UI cleanup / audit log 后返回，不到达旧 writer。legacy startup mode 保持原行为。

## 自动化过程

### 首次定向启动

使用绝对 `-log` 的首次命令只进入 UE 5.8 all-platform SDK validation；Win64 SDK VALID，但本机 LinuxArm64 / VisionOS SDK metadata 缺失，进程在测试前退出 1。

改为已构建 Editor 的 `-NoCompile -AbsLog` 命令后测试实际运行。

### 首次实际执行

- `AtomicStartAndTerminal`：Success；
- `RuntimeFailureRecovery`：Fail；
- 原因：测试将 active correlation probe 放在 finalization 之后；此时按契约必须不存在 active correlation。

移动 probe 到第二次 recovery materialize 之后、terminal 之前。

### 最终执行

- ProductFlow：2/2 Success；
- 全量 `Shanmen.0_0_10`：66/66 Success；
- queue empty；
- 两份最终日志 SHA-256 分别为：
  - `94EB892B43C30E5DEAAA33FAEDBD2C78CC7C3E9987B08758F7C4D918772C92CA`；
  - `E14ED6FFA43875CE70B7DE84BC16E2CE61BCDFBC891371A54EB16E491B8C07BD`。

## 构建过程

Editor 初次完整编译新增 source 后执行 49/49 actions，成功。测试修正与结构强化后增量编译均成功。Game 初次完整编译 48/48 actions，最终增量 3/3 actions，成功。全部使用单并发、NoUBA、NoHotReload，最终原生退出码均为 0。

## 最终不变量

1. ShanmenItems 是产品 item graph、prepared loadout 与 ActiveRun 的唯一 durable authority。
2. Correlation 是 receipt 的可重建投影，不是第二份持久化状态。
3. Runtime restart / technical rollback 不改变 CorrelationId。
4. World confirmation 不写 Code B。
5. Product terminal 不写 Code B。
6. Finalize 后 active correlation 消失。
7. Legacy Code B 代码仅保留兼容与历史自动化用途。

## 边界

未运行 Editor UI、PIE、Standalone、真实输入、截图、Smoke、Cook 或 Package；未触碰 unrelated 未跟踪文件。
