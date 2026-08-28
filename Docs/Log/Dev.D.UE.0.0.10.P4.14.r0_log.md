# Dev.D.UE.0.0.10.P4.14.r0 Development Log

## 目标

在 P4.13 完成玩家 projectile canonical migration 后，先关闭剩余产品伤害入口审计，再把“改了哪个文件就必须跑哪组测试”实现为可执行、可复核、缺证据即失败的门禁，并接入现有十二小时只读架构复核。

## 基线

- 基线分支：`agent/0.0.10-p4-13-player-projectile-product`；
- 基线提交：`55c94e188d140a2661b42eda560f3e13c70a6ae3`；
- 当前分支：`agent/0.0.10-p4-14-regression-coverage-gate`；
- P4.13 自动化：coordinator `16/16`、0.0.10 `112/112`、EnemySkill `44/44`、V2 `22/22`；
- P4.13 Editor/Game 构建均成功。

## 剩余伤害入口审计

### 产品角色类

- `demo_mapPlayerController.cpp`：M01 BasicSword 在 legacy loop 前原子 return；
- `demo_mapEnemyCharacter.cpp`：M01 ordinary melee 在 legacy call 前执行 product 并 return；
- `demo_mapHeavyEnemyCharacter.cpp`：M01 heavy 与 legacy 位于互斥 if/else；
- `demo_mapM01BossCharacter.cpp`：M01 boss 与 legacy 位于互斥 if/else；
- `demo_mapSkillComponent.cpp`：两个 shape legacy call 只在 `!bUseM01ProductPath`；
- `demo_mapSkillProjectile.cpp`：唯一 shared legacy call 只在 `!bUsedCanonicalProduct`；
- Ranged/Boss projectile canonical 分支已在 shared projectile 中先于 compatibility writer。

### 非产品入口

- `demo_mapGameMode.cpp` 中剩余调用位于 automation/visible acceptance driver；
- `demo_mapV3ProgressionManager.cpp` 中剩余调用位于 automation scenario driver；
- TrainingTarget、强制 defeat、settlement 等调用用于构造测试场景，不是运行时伤害权威。

结论：未发现待迁移的 M01 产品 `ApplyDamage`，不应为了消灭文本匹配而改写 compatibility 或 automation driver。

## 门禁设计

### 输入

```powershell
Scripts/Test-ShanmenRegressionCoverage.ps1 `
  -BaseRef <base> `
  -HeadRef <head> `
  -AutomationLogPath <explicit logs>
```

确定性测试可用 `-ChangedPath` 代替 Git range。映射默认读取 `Scripts/ShanmenRegressionMap.json`。

### 路径处理

1. `git diff --name-only --diff-filter=ACMRTUXB` 获取新增、复制、修改、重命名、类型变化与未合并相关路径；
2. 统一 `\`/`/`；
3. 一个路径可以命中多条规则，required groups 取并集；
4. Docs/Scripts 等显式 ignore；
5. 未映射生产路径直接抛出 `unmapped changed paths`。

### 证据处理

1. 只读取调用者明确提供的日志，不扫描目录猜测“最新”；
2. 每份日志要求恰好一个 RunTests 命令；
3. 至少一个 Success、零 Fail；
4. queue completion 必须匹配带测试计数的实际完成行；
5. fatal/unhandled/ensure 必须为零；
6. 每份日志计算 SHA-256；
7. 父组可覆盖子组，子组不能覆盖父组；
8. 所有 required groups 有健康证据后才输出 PASS。

## 实现过程与首次失败

1. 新建 JSON schema v1 与 14 条路径规则。
2. 新建 validator，完成 Git diff、路径分类、规则并集、日志解析、健康检查与 coverage 计算。
3. 新建临时 fixture self-test。
4. 首次 self-test 在解析阶段失败：多行 mapping validation `if` 的 `-or` 续行方式不合法。
5. 第一次修补只加括号但仍把 `-or` 放在行首，parser error 仍存在；随后改为单行条件，解析通过。
6. 解析通过后的首次功能测试失败：`Shanmen.0_0_10` 没有覆盖 `Shanmen.0_0_10.CombatCore`。
7. 根因是多行 `return Equals(...) -or StartsWith(...)` 的 PowerShell 求值方式；改为显式两个 `if`。
8. 加强 queue 规则，避免命令行 `-TestExit="Automation Test Queue Empty"` 文本被误认为真实完成行。
9. 增加 RunTests command count 校验，防止一份混合日志伪装单组证据。
10. 最终 self-test `7/7`。
11. 使用 P4.13 Git range 和四份真实日志完成 acceptance。
12. 更新十二小时复核 automation `0-0-10`。

首次失败均来自新脚本自身，没有改动或重跑 UE 产品代码。最终日志单独生成并计算 SHA；早期交互式 parser output 没有伪装为文件日志。

## 最终自测

```powershell
Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1
```

结果：

- overlapping rules union and broad suite coverage：PASS；
- docs and scripts require no product log：PASS；
- missing mapped group fails closed：PASS；
- failed test evidence is rejected：PASS；
- queue completion is required：PASS；
- unknown production path is unmapped：PASS；
- narrow child suite does not satisfy required parent suite：PASS；
- 汇总：`7/7 PASS`；
- 进程退出码：`0`。

日志：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.14.r0_regression_gate_selftest_final.log`；
- SHA-256：`823CB159B2725C8C57C55965D35BD848B7679AADAA1FE08A0DA8210F9B9559B0`。

## P4.13 真实 evidence acceptance

```powershell
Scripts/Test-ShanmenRegressionCoverage.ps1 `
  -BaseRef 5b87ac7dacbfa3bc4d34a2979e89bfeea16ef897 `
  -HeadRef 55c94e188d140a2661b42eda560f3e13c70a6ae3 `
  -AutomationLogPath <P4.13 four explicit logs>
```

输出：

- `REGRESSION_COVERAGE: PASS Changed=10 Rules=3 Required=4 Logs=4`；
- coordinator evidence：`16 Success`；
- 0.0.10 full evidence：`112 Success`；
- EnemySkillFramework evidence：`44 Success`；
- V2RangedCompatibility evidence：`22 Success`；
- 四份输入日志 SHA 与 P4.13 Report 完全一致；
- 进程退出码：`0`。

日志：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.14.r0_p4_13_evidence_gate_final.log`；
- SHA-256：`3D497DD07F15081EB21654AAFBECC2455DA0C1F0EDC052D7A68A7BD82DF1023D`。

## 十二小时自动化

automation `0-0-10` 保持：

- 名称：`山门0.0.10项目十二小时复核`；
- 状态：ACTIVE；
- 周期：每 12 小时；
- 原有只读架构复核范围不变。

新增行为：查找最近生产改动提交，读取同提交开发 Log 中明确的 Saved/Logs，调用本门禁，把 PASS/FAIL/无法验证写入交叉复核报告。它不得重跑 UE 测试，也不得修改项目或 Git。

## 静态与范围检查

- `git diff --check`：退出码 `0`；
- mapping JSON：schema v1 可解析；
- PowerShell validator/self-test：最终执行退出码 `0`；
- 本轮 changed paths：Scripts/Docs only；
- Source/Config/Content/Plugins：`0` 个改动；
- 未纳入工作区长期未跟踪文件。

## 构建与 P/F 边界

本轮不修改 UE 源码、模块、配置或资产，因此根据门禁本身不要求 Editor/Game build 或 UE Automation。P4.13 已有完整最终源码验证，本轮不重复构建。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 最终不变量

1. 每个生产改动路径必须匹配至少一条显式规则。
2. 重叠规则要求全部满足，不能任选其一。
3. broad suite 只能向下覆盖 child group，不能反向冒充。
4. 目录中存在旧日志不构成证据；调用者必须明确提供日志。
5. Fail、未清空 queue、fatal、unhandled 或 ensure 日志不能覆盖 required group。
6. 未映射生产路径必须先更新版本化 mapping，不能静默回退到“跑全量大概够了”。
7. Docs/Scripts-only 改动不制造无意义 UE build。
8. 十二小时复核只读调用门禁，不重跑测试或修改 Git。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-14-regression-coverage-gate/Docs/Report/Dev.D.UE.0.0.10.P4.14.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-14-regression-coverage-gate/Docs/Log/Dev.D.UE.0.0.10.P4.14.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-14-regression-coverage-gate>
