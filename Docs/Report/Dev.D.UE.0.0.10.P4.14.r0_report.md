# Dev.D.UE.0.0.10.P4.14.r0 Report

## 1. 结论

P4.14 完成，结论为 **PASS**。

本轮没有继续增加伤害管线。P4.13 后的 `ApplyDamage` 审计确认：M01 玩家、普通近战敌人、重型敌人、远程敌人、Boss、GroundCircle、SelfSector 与 Straight Projectile 均已有 canonical route ownership；角色类中保留的调用只属于非 M01 compatibility，GameMode/V3 中其余调用属于 automation driver。未发现新的 M01 产品写入缺口。

根据该结论，本轮实现了可执行的“改动路径 → 必跑测试组 → 真实日志证据”门禁，消除按任务主题手工挑选回归组的漏洞。门禁正负向自测 `7/7` 通过，并用 P4.13 的 10 个真实改动文件与四份 UE Automation 日志复核通过。

## 2. 交付内容

- `Scripts/ShanmenRegressionMap.json`
  - 版本化路径映射表；
  - 当前包含 14 条显式规则；
  - 区分生产路径与 Docs/Scripts 等非产品路径。
- `Scripts/Test-ShanmenRegressionCoverage.ps1`
  - 从 Git ref 范围或显式 changed paths 读取改动；
  - 汇总所有重叠规则要求的测试组；
  - 解析显式提供的 UE Automation 日志；
  - 校验测试组、结果、queue completion、fatal/ensure 与 SHA-256；
  - 缺映射、缺测试组或坏日志时 fail-closed。
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
  - 用临时 fixture 覆盖 7 个正负向场景；
  - fixture 结束后自动清理。

## 3. 映射规则

当前显式覆盖：

- `Source/ShanmenCombatCore/*` → `Shanmen.0_0_10.CombatCore`；
- `Source/ShanmenCombatRuntime/*` → `Shanmen.0_0_10.CombatRuntime`；
- `Source/ShanmenWorldGameplay/*` → `Shanmen.0_0_10.WorldGameplay`；
- `Source/ShanmenItems/*` → `Shanmen.0_0_10.Items`；
- CombatRunCoordinator → coordinator 定向组；
- PlayerVitality → PlayerVitality 定向组 + 0.0.10 全量；
- GameMode → 0.0.10 全量；
- PlayerController → 0.0.10 全量 + V2RangedCompatibility；
- Skill/Enemy combat → 0.0.10 全量 + EnemySkillFramework + V2RangedCompatibility；
- CombatTargeting/CombatTypes/SectorGeometry → 同上；
- Profile authority → `demo_map.Profile` + `demo_map.ItemEconomySchema`；
- CodeB → `demo_map.CodeB` + 0.0.10 Items；
- item product adapters → 0.0.10 Items；
- `DefaultGameplayTags.ini` → 0.0.10 全量。

规则允许重叠并取并集。例如 P4.13 同时修改 coordinator、GameMode 与 SkillProjectile，所以最终要求四组，而不是任选一组。

## 4. fail-closed 证据契约

### 改动路径

- 默认读取 `git diff --name-only --diff-filter=ACMRTUXB <BaseRef>...<HeadRef>`；
- 也可用 `-ChangedPath` 注入确定性 fixture；
- Windows `\` 与 Git `/` 路径统一规范化；
- Source、Config、Content、Plugins、uproject/uplugin 中没有显式规则的路径直接失败；
- Docs、Scripts、`.github` 与仓库说明文件不强制产品测试。

### 日志

每份证据日志必须满足：

1. 恰好一个 `Cmd: Automation RunTests <group>`；
2. 至少一个 `Result={Success}`；
3. `Result={Fail}` 为 `0`；
4. 存在真实 `Automation Test Queue Empty <N> tests performed` 完成行，不能只依赖命令行里的 `TestExit` 文本；
5. fatal、unhandled exception、ensure marker 为 `0`；
6. 计算并输出 SHA-256。

### 父子测试组

执行父组可以覆盖其子组，例如 `Shanmen.0_0_10` 可覆盖 `Shanmen.0_0_10.CombatCore`。反向不成立：只执行 coordinator 子组不能满足 GameMode 所要求的 0.0.10 全量。

## 5. 自测

最终自测 `7/7 PASS`：

1. 重叠规则取并集，父组可覆盖子组；
2. Docs/Scripts-only 改动不要求产品日志；
3. 缺少映射测试组时失败；
4. 含失败测试的日志被拒绝；
5. 缺 queue completion 的日志被拒绝；
6. 未映射生产路径失败；
7. 子组不能冒充父组。

最终日志：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.14.r0_regression_gate_selftest_final.log`；
- SHA-256：`823CB159B2725C8C57C55965D35BD848B7679AADAA1FE08A0DA8210F9B9559B0`。

## 6. 真实证据复核

使用范围：

- BaseRef：`5b87ac7dacbfa3bc4d34a2979e89bfeea16ef897`；
- HeadRef：`55c94e188d140a2661b42eda560f3e13c70a6ae3`。

结果：

- changed files：`10`；
- matched rules：`3`；
- required groups：`4`；
- evidence logs：`4`；
- 结论：`REGRESSION_COVERAGE: PASS`。

被验证的四组：

- `Shanmen.0_0_10.Product.CombatRunCoordinator`：`16 Success`；
- `Shanmen.0_0_10`：`112 Success`；
- `demo_map.EnemySkillFramework`：`44 Success`；
- `demo_map.V2RangedCompatibility`：`22 Success`。

最终门禁日志：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.14.r0_p4_13_evidence_gate_final.log`；
- SHA-256：`3D497DD07F15081EB21654AAFBECC2455DA0C1F0EDC052D7A68A7BD82DF1023D`。

## 7. 初次失败与修正

开发期间保留并如实记录了两类脚本缺陷：

1. PowerShell 多行 `if` 中把 `-or` 放在新行开头，产生 parser error；改为单行完整条件。
2. 父组覆盖 helper 使用多行 `return <expr> -or <expr>`，实际把合法父组判为缺失；改成两个显式 `if` 和最终 `return $false`。

这些是门禁脚本自身的开发错误，不是 UE 源码、工具链或环境失败。修正后所有正负向场景和 P4.13 真实证据均通过。

## 8. 十二小时复核集成

现有 automation `0-0-10`（“山门0.0.10项目十二小时复核”）已更新：

- 自动定位近期最新生产改动提交；
- 取第一父提交与该提交作为 BaseRef/HeadRef；
- 从同提交的开发 Log 提取明确列出的 Saved/Logs；
- 调用本门禁并把 PASS、缺映射、缺组、坏日志或无法验证写入交叉复核报告；
- 不重跑测试、不修改项目源代码或 Git 状态。

若最新提交只改 Docs/Scripts，复核会向前寻找最近的生产改动提交，不会用过程提交掩盖产品回归缺口。

## 9. 修改范围与静态检查

本轮只修改：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverage.ps1`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Log。

`git diff --check`：退出码 `0`。未修改 Source、Config、Content、Plugin 或 UE 资产；未纳入长期未跟踪文件。

## 10. 构建与 P/F 边界

本轮属于流程门禁开发，实际 changed paths 只有 Scripts/Docs。依据本轮刚建立的同一映射纪律，不重复执行 Editor/Game 构建或 UE Automation；否则会重新制造“按习惯全跑、但不解释改动覆盖”的冗余。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 11. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-14-regression-coverage-gate/Docs/Report/Dev.D.UE.0.0.10.P4.14.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-14-regression-coverage-gate/Docs/Log/Dev.D.UE.0.0.10.P4.14.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-14-regression-coverage-gate>
