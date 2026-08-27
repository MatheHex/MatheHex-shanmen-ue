# Dev.D.UE.0.0.10.P0.0.r0 开发日志

## 身份

- 任务：`Dev.D.UE.0.0.10.P0.0.r0`
- 分支：`agent/0.0.10-p0-combat-contracts`
- 起点：`af27b71`（`agent/f1-0-r1-baseline`）
- 收口时间：`2026-08-27T02:08:19.3576249Z`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：Unreal Engine `5.8`

## 决策记录

1. 读取同项目“代码结构”任务最近三轮结论。
2. 采用“单仓库、多 Runtime 模块、0.0.10 架构优先、旧代码可迁移但不承担兼容义务”的基线。
3. 首轮只建立 `ShanmenCore` 和 `ShanmenCombatCore`，不让旧 `demo_map` 依赖新模块，避免未完成迁移时双写。
4. 物理查询与命中语义分离；本轮仅冻结候选、Impact、幂等和防御数学。
5. GAS 留在后续编排模块；CombatCore 不依赖 ASC、Ability、Effect、Actor 或 World。
6. 自目标是否可用属于目标策略，不属于几何候选结构校验；最终复核时移除了候选层的自目标禁止规则。

## 开发记录

### 分支与模块

- 从 `af27b71` 创建 `agent/0.0.10-p0-combat-contracts`。
- 在 `.uproject` 与 Editor/Game Target 中登记两个新 Runtime 模块。
- 创建 `ShanmenCore`：内容戳、操作上下文、确定性 GUID。
- 创建 `ShanmenCombatCore`：动作／检测器枚举、快照、候选、Impact 请求与结果、ID Factory、Impact Ledger、防御 Resolver。
- 创建 5 项 Editor Automation 测试。

### 首次验证

- Editor Development：退出码 `0`，19 actions，`Result: Succeeded`。
- CombatCore Automation：退出码 `0`，5/5 Success。
- Game Development：退出码 `0`，10 actions，`Result: Succeeded`。

### 代码复核修正

- 发现 `FShanmenHitCandidate::IsValid` 不应把 `SourceEntityId == TargetEntityId` 判为结构非法。
- 移除该限制并在 `DeterministicIdentity` 测试中加入自目标候选断言。
- 该修正不允许自目标自动命中；后续 `HitValidator` 仍按动作／阵营／规则决定合法性。

### 最终验证

- Editor Development：退出码 `0`，5 actions，`Result: Succeeded`，11.93 秒。
- CombatCore Automation：退出码 `0`，找到 5 项，5/5 Success，队列正常清空。
- Game Development：退出码 `0`，4 actions，`Result: Succeeded`，19.86 秒。
- 自动化日志 SHA-256：`3004CA2BB8E11A4670F0B6F1BAFCCF0CB28B3D98A7ECA29089B0AF21D3DA2C20`。
- `git diff --check`：退出码 `0`。
- 新模块边界扫描：没有旧 `demo_map`、Actor、World、直接伤害或随机调用。

## 未执行与原因

- 未将新 CombatCore 接入玩家、敌人或旧技能：P0 只冻结底层契约。
- 未实现资源 Reserve/Commit：归入 P1 的 `ShanmenItems` 边界。
- 未启动产品、PIE、Standalone、UI、Smoke、Cook 或 Package：本轮不包含真实产品行为变更。
- 未暂存或提交工作区原有未跟踪文件。

## 交付清单

- `demo_map.uproject`
- `Source/demo_map.Target.cs`
- `Source/demo_mapEditor.Target.cs`
- `Source/ShanmenCore/ShanmenCore.Build.cs`
- `Source/ShanmenCore/Public/ShanmenCore.h`
- `Source/ShanmenCore/Public/ShanmenCoreTypes.h`
- `Source/ShanmenCore/Public/ShanmenDeterministicId.h`
- `Source/ShanmenCore/Private/ShanmenCore.cpp`
- `Source/ShanmenCore/Private/ShanmenDeterministicId.cpp`
- `Source/ShanmenCombatCore/ShanmenCombatCore.Build.cs`
- `Source/ShanmenCombatCore/Public/ShanmenCombatCore.h`
- `Source/ShanmenCombatCore/Public/ShanmenCombatTypes.h`
- `Source/ShanmenCombatCore/Public/ShanmenCombatResolver.h`
- `Source/ShanmenCombatCore/Private/ShanmenCombatCore.cpp`
- `Source/ShanmenCombatCore/Private/ShanmenCombatResolver.cpp`
- `Source/ShanmenCombatCore/Private/Tests/ShanmenCombatCoreTests.cpp`
- `Docs/Report/Dev.D.UE.0.0.10.P0.0.r0_report.md`
- `Docs/Log/Dev.D.UE.0.0.10.P0.0.r0_log.md`
