# Dev.D.UE.0.0.10.P0.1.r0 开发日志

## 身份

- 任务：`Dev.D.UE.0.0.10.P0.1.r0`
- 分支：`agent/0.0.10-p0-combat-contracts`
- 起点：`b7724b1`（P0.0）
- 收口时间：`2026-08-27T02:38:04.8066516Z`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：Unreal Engine `5.8`

## 输入复核

逐项核对复审 A--G，并读取现有 Code A／Code B 边界：

- `Udemo_mapItemSubsystem` 仍包含局内物品与快捷使用路径。
- Code B 包含持久物品图、局外仓库、Run 容器和结算路径。
- `demo_mapGameMode.cpp` 的现有桥仍明确包含 audit-only 语义。
- `demo_mapProfileRepository.cpp::IsExactLegacySourceFor` 的 legacy 范围仍为 `1..5`，Schema 6 -> 7 缺陷属实。

据此撤销 P0.0 中“P1 只以单向 Adapter 对接 Code B”的计划，改为 `ShanmenItems` 新单一权威和双来源迁移。

## 实现记录

1. 把五个固定防御字段改为通用 `FShanmenDefenseLayer` 数组。
2. 新增确定性 Order/LayerId 排序和四种纯函数操作。
3. 新增 DamagePacket、DamageTags、TargetVitality 与致死拦截。
4. 新增 native Gameplay Tags 与 Damage/Source/Target 条件过滤。
5. 将结果统一为总 PreventedDamage 加逐层触发 receipt，建立守恒函数。
6. Request 重新派生 ImpactId；Ledger 改为接收完整 Request。
7. 用 Capture -> 私有只读 Snapshot 替代可任意改写的动作快照。
8. 移除 CombatCore 的 BasePower；攻击公式层负责输出 DamagePacket。
9. 要求资源提交的防御层必须携带有效 SourceInstanceId；重复 LayerId 失败关闭。
10. 新增 0.0.10 物品唯一权威 ADR。

## 编译与测试过程

### Editor 构建

- 初次完整重构构建：退出码 `0`，7 actions，`Result: Succeeded`，35.01 秒。
- 补充提交来源与重复层校验后的最终构建：退出码 `0`，5 actions，`Result: Succeeded`，7.06 秒。

### 自动化启动修正

1. 首次使用绝对 `-log` 路径时只完成平台 SDK 预检，未生成目标测试日志；退出码虽为 `0`，但没有测试证据，因此不计成功。
2. 第二次把双引号作为参数正文传入，UE 收到 `-ExecCmds=""...""`；目标命令未正确收口，进程被有界终止，退出码 `1`。这是命令封装错误，不是源码或测试失败。
3. 最终使用单参数、无字面双引号的 PowerShell 传参。UE 收到正确的 `-ExecCmds="Automation RunTests Shanmen.0_0_10.CombatCore"`，原生退出码 `0`。

### 最终自动化

- 测试发现／完成：9 项。
- `Result={Success}`：9。
- `Result={Fail}`：0。
- 队列：`Automation Test Queue Empty 9 tests performed`。
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P0.1.r0_automation.log`。
- SHA-256：`4A4E7D1B6D7CE3E5238DEE0017E1ACAD20B0DB1A3BD9FC3203A03B3102E62B96`。
- UE 启动阶段的 13 条内置 `Condition failed` 与 P0.0 基线数量相同，发生在目标测试启动前；目标测试全部通过。

### Game 构建

- `demo_map Win64 Development -MaxParallelActions=1 -NoUBA`
- 原生退出码：`0`
- actions：6
- 结果：`Succeeded`
- 总时间：26.93 秒

### 静态证据

- `git diff --check`：退出码 `0`。
- 新模块禁止依赖／调用扫描：0 匹配。
- CombatCore `BasePower`：0 引用。
- 没有启动产品、Editor UI、PIE、Standalone、Smoke、Cook 或 Package。

## 交付清单

- `Source/ShanmenCombatCore/Public/ShanmenCombatTypes.h`
- `Source/ShanmenCombatCore/Public/ShanmenCombatResolver.h`
- `Source/ShanmenCombatCore/Public/ShanmenCombatTags.h`
- `Source/ShanmenCombatCore/Private/ShanmenCombatResolver.cpp`
- `Source/ShanmenCombatCore/Private/ShanmenCombatTags.cpp`
- `Source/ShanmenCombatCore/Private/Tests/ShanmenCombatCoreTests.cpp`
- `Docs/Architecture/Dev.D.UE.0.0.10_ItemAuthority_ADR.md`
- `Docs/Report/Dev.D.UE.0.0.10.P0.1.r0_report.md`
- `Docs/Log/Dev.D.UE.0.0.10.P0.1.r0_log.md`

未暂存工作区原有未跟踪文件。
