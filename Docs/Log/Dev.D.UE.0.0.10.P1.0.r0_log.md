# Dev.D.UE.0.0.10.P1.0.r0 开发日志

## 身份

- 任务：`Dev.D.UE.0.0.10.P1.0.r0`
- 分支：`agent/0.0.10-p1-item-transactions`
- 起点：`6762aeb`（P0.1）
- 收口时间：`2026-08-27T18:42:52.2934198Z`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：Unreal Engine `5.8`

## 规划输入

1. 读取 0.0.10 人工完善战斗规划，确认御器、暗器、阵法、主动防御和灵器都必须使用真实物品。
2. 读取同项目“代码结构”最新结论，冻结 `Validate -> Reserve -> Commit Point -> Commit/Cancel -> Receipt`。
3. 读取 P0.1 物品唯一权威 ADR，确认 Code A／Code B 只作为迁移来源，P1 不做在线双写 Adapter。
4. 从 `6762aeb` 创建独立分支 `agent/0.0.10-p1-item-transactions`。

## 实现记录

### 模块

- 新增 Runtime 模块 `ShanmenItems`。
- 在 uproject、Editor Target 与 Game Target 登记模块。
- 依赖仅为 Core、CoreUObject、GameplayTags、ShanmenCore。

### 数据与事务

- 新增 Definition、Container、ItemInstance、Reservation、ProcessedRequest、AuthoritySnapshot。
- 新增 Quantity、DeploymentLock、Durability、Charges 四类资源。
- 新增 Reserve、Commit、Cancel、ReleaseDeployment。
- 新增 deterministic reservation/fingerprint/receipt 身份。
- Snapshot 保存成功／失败 ledger，支持重启重放。
- 完整消耗保留 Depleted tombstone 并清除容器槽位。

### 自查修正

1. 初版测试 8/8 通过后，发现失败 Receipt 已进入持久 ledger，但 AuthorityRevision 未推进。修正为 ledger 变化也推进 AuthorityRevision，避免同 revision 对应不同 Snapshot。
2. 收紧 DeploymentLock 双向排他，避免部署前与 Durability/Charges 预留并存导致终态冲突。
3. 发现已部署飞剑仍必须接受命中耐久提交；修正为 Deployed 状态允许 Durability/Charges，但继续拒绝 Quantity 和再次 DeploymentLock。
4. 加强 Snapshot 验证：终态 Reservation 必须存在对应 Commit/Cancel/Release 成功 Receipt，Receipt 的资源、数量、阶段与 Reservation 必须一致。
5. 补充无效 reload 原子保留、Cancel-after-Commit、陈旧 Revision、错误 Capability 和 Depleted 重载断言。

## 验证记录

### Editor 构建

- 首次模块完整构建：退出码 `0`，13 actions，`Result: Succeeded`，44.73 秒。
- 事务自查修正后构建：退出码 `0`，6 actions，`Result: Succeeded`，10.10 秒。
- 最终测试增强后构建：退出码 `0`，4 actions，`Result: Succeeded`，10.78 秒。

### Automation

- 初版 Items 定向执行：8/8 Success。
- 自查修正后 Items 定向执行：8/8 Success。
- 最终联合执行 `Shanmen.0_0_10`：17/17 Success，0 Fail，队列正常清空。
- 最终原生退出码：`0`。
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.0.r0_automation.log`。
- SHA-256：`2D8487589CFE660E1FEA6241AD8DEEF5578963CECC99F4E9BB3C21D3B206A424`。
- 目标测试前的 13 条 UE 内置 Condition failed 与 P0 基线一致，不属于 Shanmen 测试。

### Game 构建

- 最终 `demo_map Win64 Development -MaxParallelActions=1 -NoUBA`。
- 退出码 `0`，3 actions，`Result: Succeeded`，19.65 秒。

### 静态证据

- `git diff --check`：退出码 `0`。
- ShanmenItems 禁止依赖／调用扫描：0 匹配。
- 未启动产品、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 交付清单

- `demo_map.uproject`
- `Source/demo_map.Target.cs`
- `Source/demo_mapEditor.Target.cs`
- `Source/ShanmenItems/ShanmenItems.Build.cs`
- `Source/ShanmenItems/Public/ShanmenItems.h`
- `Source/ShanmenItems/Public/ShanmenItemTags.h`
- `Source/ShanmenItems/Public/ShanmenItemTypes.h`
- `Source/ShanmenItems/Public/ShanmenItemRepository.h`
- `Source/ShanmenItems/Private/ShanmenItems.cpp`
- `Source/ShanmenItems/Private/ShanmenItemTags.cpp`
- `Source/ShanmenItems/Private/ShanmenItemTypes.cpp`
- `Source/ShanmenItems/Private/ShanmenItemRepository.cpp`
- `Source/ShanmenItems/Private/Tests/ShanmenItemsTests.cpp`
- `Docs/Architecture/Dev.D.UE.0.0.10_ItemTransactions_ADR.md`
- `Docs/Report/Dev.D.UE.0.0.10.P1.0.r0_report.md`
- `Docs/Log/Dev.D.UE.0.0.10.P1.0.r0_log.md`

未暂存工作区原有未跟踪文件。
