# Dev.D.UE.0.0.10.P4.4.r0 Development Log

## 目标

把 P4.3 的玩家专用 World Registry 提升为共享 Run 战斗身份面；用 M01 authored spawn identity 登记全部敌人；选择现有近战敌人 Actor 作为第一种真实敌人 vitality host，并证明玩家 BasicSword resolved receipt 可 exactly-once 地提交到该产品真值。

## 基线

- 分支：`agent/0.0.10-p4-4-m01-enemy-vitality`；
- 基线提交：`5cc4b165bf3820c37adb72da174ad35622d7bd5e`（P4.3）；
- 基线全量：`Shanmen.0_0_10` 98/98；
- M01 authored enemy 数：14；
- 已存在身份来源：每条 `Fdemo_mapM01EnemyDefinition::SpawnMarkerId` 唯一；
- 已存在产品近战宿主：`Ademo_mapEnemyCharacter`，原生命为两个 `int32` 字段。

## 审计与决策

### 1. 不使用随机 LootSourceId 作为战斗身份

`Ademo_mapEnemyCharacter` 的 `LootSourceId = FGuid::NewGuid()` 已服务尸体与奖励投影。它不是 authored spawn identity，并且无法跨回放重建，因此继续保留在奖励边界，绝不进入 combat EntityId 派生。

M01 已有唯一 `SpawnMarkerId`，最终采用：

```text
EntityId = MakeEntityId(AuthorityRunId, Definition.SpawnMarkerId, 0)
```

这也避免使用 Actor 地址、对象名和生成循环顺序。

### 2. 共享协调器需要改名

若继续让 `Fdemo_mapPlayerCombatCoordinator` 持有全部敌人，类型名会错误表达所有权并在后续 P4.5 扩大误导。P4.4 在结构仍小时将其改为 `Fdemo_mapCombatRunCoordinator`，保留玩家路径的全部契约，同时新增 M01 登记与敌人交付门。

### 3. 全部敌人先有身份，生命迁移按宿主类型分批

M01 14 个 Actor 全部进入共享 Registry，保证 P4.5 的 world hit 候选统一解析。生命迁移只覆盖 `Ademo_mapEnemyCharacter`，即 5 个 StandardSkirmisher 与 2 个 EliteStalker。远程、重甲、Boss 只登记 identity，不在同一轮触碰其死亡、技能与表现链。

### 4. 旧伤害不能绕过 revision

近战 Actor 仍被既有技能和投射物通过 `TakeDamage` 命中。删除旧入口会破坏兼容性，直接保留字段写入又会让 canonical ledger desynchronize。最终保留旧取整与返回值语义，但把状态变化路由到 `TryCommitExternalMutation`；canonical Impact 则使用同一产品字段上的 `Commit`。

## 实现过程

1. 建立 P4.4 分支并审计 M01 生成循环、identity component、14 条 definitions、近战死亡链与 GameMode Run 生命周期。
2. 新建 `Fdemo_mapCombatRunCoordinator`，迁移 P4.3 玩家绑定与交付能力。
3. 增加 `MakeM01EnemyEntityId`、M01 actor/root alias 登记、archetype/class 校验、冲突拒绝与 Run 级释放。
4. GameMode 在 M01 内容生成完成后登记全部 14 个 Actor；失败时撤销已建立的 combat Run。
5. 将近战 Actor 的生命真值改为 float vitality，并接入 external mutation、snapshot、commit、revision、receipt replay 与 exact-ID release。
6. 增加敌人 BasicSword 产品交付门，并要求当前玩家 source、当前 Run 与 authored target 全部一致。
7. 将 P4.3 两条测试迁移到共享命名，新增 M01 authored identity 与 BasicSword delivery 两条测试。
8. 首轮验证后加强 identity 测试：以 14 个真实对应 Actor class 全量登记，并在 coordinator 中加入 authored archetype／class mismatch fail-closed 校验。

## 验证时间线

1. 初始 `git diff --check`：退出码 0。
2. Editor 首次完整构建：25/25 actions，`Result: Succeeded`，退出码 0。
3. 共享 coordinator 定向初跑：4/4 Success，0 Fail，queue empty，退出码 0。
4. 全量初跑：100/100 Success，0 Fail，queue empty，退出码 0。
5. Game 首次完整构建：24/24 actions，`Result: Succeeded`，退出码 0。
6. 复审增加 14 个真实 class 全量 identity 登记与 class mismatch fail-closed。
7. Editor 最终增量构建：6/6 actions，`Result: Succeeded`，退出码 0。
8. 共享 coordinator 最终定向：4/4 Success，0 Fail，queue empty，退出码 0。
9. 全量最终：100/100 Success，0 Fail，queue empty，退出码 0。
10. Game 最终增量构建：5/5 actions，`Result: Succeeded`，退出码 0。
11. 最终 `git diff --check` 与身份禁用 API 扫描通过。

本轮没有编译失败、测试失败、Windows commit memory／页面文件错误或其它需要重试的环境错误。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次：25/25，退出码 0；
- 最终：6/6，退出码 0。

### CombatRunCoordinator 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- found／performed：4；
- result：4 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.4.r0_combat_run_automation_final.log`；
- SHA-256：`F8A8FC6CE2C12015302AA94B9ABE39425691EED985152C069C775827F606D9CC`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- found／performed：100；
- result：100 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.4.r0_full_automation_final.log`；
- SHA-256：`B86708A37858556C4318DCA0796A4E7343D5FFE9DEC291C4593AE9A455A9D4A2`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次：24/24，退出码 0；
- 最终：5/5，退出码 0。

## 最终不变量

1. 玩家和 M01 敌人共享一个 authority Run Registry。
2. M01 EntityId 只由 authority Run、authored SpawnMarkerId 和固定 ordinal 派生。
3. authored archetype 必须匹配实际产品 Actor class。
4. 同一 authored spawn 在一个 Run 中只能属于一个 Actor；同 Actor 重复登记幂等。
5. 近战 Actor 的产品 float 字段是唯一生命真值；ledger 只保存 fingerprint、revision 与 receipt。
6. legacy damage 与 canonical Impact 都必须推进同一 ledger revision，不能双写。
7. canonical first commit 扣血并反馈一次；replay 不扣血、不反馈；旧 Run receipt 在新 Run 失败关闭。
8. Run 结束以 exact EntityId 释放所有已迁移 host，随后清空 Registry。
9. 随机 LootSourceId 只留在 reward/corpse 边界，不是 combat identity。

## 诊断与边界

- 两份最终 Automation 日志包含 UE 5.8 测试发现前的既有启动诊断；目标测试全部成功，目标测试期间没有 controller error、fatal、assertion 或 handled ensure。
- Win64 SDK 为 `VALID 10.0.22621.0`；非目标 LinuxArm64／VisionOS metadata 警告未描述为源码失败。
- `git diff --check`：原生退出码 0。
- 身份路径禁用 API 扫描全部为 0 匹配。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-4-m01-enemy-vitality/Docs/Report/Dev.D.UE.0.0.10.P4.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-4-m01-enemy-vitality/Docs/Log/Dev.D.UE.0.0.10.P4.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-4-m01-enemy-vitality>
