# Dev.D.UE.0.0.10.P1.5.r0 开发日志

## 身份

- 任务：`Dev.D.UE.0.0.10.P1.5.r0`
- 分支：`agent/0.0.10-p1-5-cutover-fence`
- 起点：`dcfaa9286446247307f1d70d3e91beafc48a39f8`
- 收口时间：`2026-08-27T23:15:31.7024064Z`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：Unreal Engine `5.8`

## 开工审查

1. P1.4 已建立 GameInstance 唯一 authority owner、existing-first 打开、稳定 Profile/Code B 只读捕获和 durable 命令，但没有退休旧持久化写者。
2. Code B 的全部持久化写最终收敛到 `FCodeBOutOfRaidProfileStore::SaveRecord`。
3. Code A/Profile 的准备、仓库、开局、结算、商店和交易事务最终收敛到 `Fdemo_mapProfileRepository::SaveProfile`。
4. 若逐个修补 UI/Actor 调用者，容易遗漏写路径；若在旧写者仍开放时自动迁移，会形成双真值。
5. 因此本轮选择“新 authority 文件即 durable marker + 两个中央提交点围栏 + 显式 cutover coordinator”，不自动接产品启动。

## 设计决策

### marker 不另存状态

- 使用 Owner 专属 authority primary/backup 的存在性作为退休标记。
- 不新增独立 `bMigrated` 文件，避免 authority 已提交但标记未写、或标记已写但 authority 未提交的双文件事务裂缝。
- primary/backup 任一存在都关闭旧写者；损坏只改变新 authority 生命周期为 Recovery，不重新授权 legacy。

### Profile 允许非物品进度

- Profile 仍承载灵石、宗门升级等非物品数据，不能在物品切换后整份冻结。
- 围栏比较旧文档中全部物品承载字段，只允许其余字段变化。
- 比较基线优先有效 primary，再退到有效 backup；两者都无有效 Owner 匹配数据时拒绝。

### Code B 全写关闭

- Code B record 本质上是旧物品权威，切换后不存在需要继续写入的非物品例外。
- 在唯一 `SaveRecord` 前拒绝可覆盖所有当前 Code B commit API，并保证没有临时文件 I/O。

### existing-first coordinator

- `BindExisting` 总是第一步。
- 仅 Missing/Waiting 才读取 Profile 与 Code B。
- 读取后再次调用 P1.4 existing-first 绑定，处理检查与发布之间的竞争。
- 发布成功后重新探测 durable fence；围栏未关闭不能报告 Ready。

## 实现记录

### 新文件

- `demo_mapShanmenLegacyItemWriteFence.h/.cpp`
- `demo_mapShanmenItemCutover.h/.cpp`
- `demo_mapShanmenItemCutoverTests.cpp`

### 旧系统中央入口

- Profile Repository 增加退休物品投影比较与 valid-baseline gate。
- Code B Store 在 `SaveRecord` 最前方增加退休 gate。
- Warehouse presentation 动态投影只读状态。
- Profile Session 暴露动态围栏状态，并在退休后不再补建 legacy ShopStock。

### 未做事项

- 没有在 Framework、GameMode、Widget 或 Actor 中自动调用 cutover。
- 没有把局内物品操作改接新 authority。
- 没有双写新旧文档。
- 没有删除、迁移或重写现有用户数据。

## 自动化记录

### 新增测试

1. `StablePublishRetiresLegacyWriters`
2. `MissingSourceDoesNotRetire`
3. `CorruptAuthorityStillRetiresLegacy`
4. `RestartSourcesAreReadOnly`

### 修正循环

#### 第 1 轮

- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.5.r0_automation.first.log`
- SHA-256：`5FAE5B4F352F3B8CB7602E7F498F9DCABE2BA597CA29EDD283C7835D5B0E6414`
- 结果：3 Success、1 Fail。
- 失败：`StablePublishRetiresLegacyWriters` 的测试物品变化使用超上限 stack count，被既有 Profile validation 先拒绝，断言要求的 `retired` 诊断未出现。
- 判断：测试夹具无效，不是围栏生产逻辑失败。

#### 第 2 轮

- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.5.r0_automation.fixed.log`
- SHA-256：`64EF4B26AF2EBB0336B860CF87F24950BCC3D6FA4238D952F1C7E5A4F4DD8D77`
- 结果：2 Success、2 Fail。
- 失败：新档的 `WarehouseLayout` 尚未初始化，仓库换位夹具无法构造；测试在前置条件处明确失败。
- 判断：第二个夹具假设错误，生产逻辑未进入失败路径。

#### 第 3 轮

- 修正：从新档必有的 `TrainingBlade` 取得合法 ItemInstanceId，写入 Weapon preparation slot，构造可通过既有 Profile validation 的真实物品字段变化。
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.5.r0_automation.corrected.log`
- SHA-256：`1838F8DDCF08BC9C25DD53A32FC275DBA1372F4877518AEFA55AD058C59DFD24`
- 结果：`4/4 Success`、0 Fail、队列正常清空。

### 完整回归

- 目标：`Automation RunTests Shanmen.0_0_10`
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.5.r0_automation.log`
- SHA-256：`40387A5E6B9151B1450E9FE4A302ECD79AA4C58AE0A49168405ED70C10025206`
- 结果：`48/48 Success`、0 Fail、`Automation Test Queue Empty`。
- 组成：既有 44 项 + P1.5 新增 4 项。
- 进程原生退出码：0；测试真值以日志中的逐项 Result 和队列汇总为准。

## 构建记录

### Editor

- 命令：`Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`
- 首次生产代码构建：35/35 actions，`Result: Succeeded`，退出码 0，147.37 秒。
- 最终测试夹具增量构建：4/4 actions，`Result: Succeeded`，退出码 0，5.34 秒。

### Game

- 命令：`Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`
- 结果：34/34 actions，`Result: Succeeded`，退出码 0，122.05 秒。

## 静态审查

- `git diff --check`：退出码 0。
- 新 Cutover/Fence 中 World/Actor/UI/GAS/随机数 API：0 匹配。
- `SaveRecord`：一个定义、内部 commit 收敛调用。
- `SaveProfile`：Profile 事务统一入口，围栏在任何写盘前执行。
- Cutover coordinator 产品调用：0；目前仅定义与测试。
- 既有工作区中的无关未跟踪 Prompt、Report、PDF 与自动化交接文档不纳入本次暂存。

## P/F 边界

- 完成代码、静态审查、无头自动化、Editor/Game Development 构建。
- 未启动 Editor UI、PIE、Standalone、产品 exe。
- 未做真实输入、截图、Smoke、Cook、Package 或大规模产品回归。
- 原始 `Saved/Logs` 不提交；通过本 Log 固化摘要与 SHA-256。

## 后续入口

P1.6 从产品 Adapter 开始：先把宗门仓库与携带准备改为只调用 `Udemo_mapShanmenItemAuthoritySubsystem`，再覆盖局内消耗、拾取/掉落与结算；产品写路径全部脱离旧 authority 后，才把 `Fdemo_mapShanmenItemCutoverCoordinator::Execute` 接到唯一启动点。
