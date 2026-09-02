# Dev.D.UE.0.0.10.P18.0.r0 Development Log

## 1. 目标与基线

- 基线提交：`2dcfd84737ff2502fed8536ee707d6728e295861`（P17.2 Heart Mirror settlement lifecycle）；
- 分支：`agent/0.0.10-p18-0-sword-qi-contract`；
- 目标：建立第一条由精确剑实例驱动的纯运行时剑气契约；
- 约束：仅做 P 阶段，不创建世界 Actor、输入、表现、碰撞查询或第二套伤害/生命/库存权威。

## 2. 起始审计与设计决定

现有 CombatRuntime 已具备 Action phase、不可变 action snapshot、detector emission session、Projectile candidate、统一 Impact identity、幂等 ledger、标签伤害与有序防御 resolver。人工战斗规划明确剑可从近战延伸剑气，但要求手感优先且数值后置。

因此本轮复用上述基础，只补剑气自己的 definition、offense snapshot、launch identity、生命周期与 candidate-to-impact 编排。伤害标签保持内容可选，不提前把剑气冻结为 Physical 或 Spirit；fixture 使用 Spirit 只为验证标签防御链。

## 3. 新增源码

新增：

- `Source/ShanmenCombatRuntime/Public/ShanmenSwordQiExecution.h`；
- `Source/ShanmenCombatRuntime/Private/ShanmenSwordQiExecution.cpp`；
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenSwordQiExecutionTests.cpp`。

主要结构：

- `FShanmenSwordQiDefinition`：捕获 canonical action、detector/formula、伤害参数、速度、距离和标签策略；
- `FShanmenSwordQiOffenseSnapshot`：冻结 activation 时的 AttackPower；
- `FShanmenSwordQiLaunchReceipt`：证明精确 Sword Item、原点、规范方向、速度和距离；
- `FShanmenSwordQiImpactReceipt`：包装通用 request/result，并要求 accepted、ID 一致及伤害守恒；
- `FShanmenSwordQiExecution`：管理 Ready、InFlight、Dissipated，复用 emission session 与 Impact ledger。

## 4. 确定性与失败关闭

Launch ID 命名空间为 `Shanmen.SwordQi.Launch.r1`。canonical parts 包括 Run、Owner、Activation、Source Entity、Source Item、Action Definition、内容版本/摘要、原点、方向、速度和距离。方向先安全归一，正负零规范化后再做 bit-level 编码。

执行失败关闭以下输入：无 Sword Item、错误 Action ID、非 Damage 树标签、非正速度/距离、负/非有限 AttackPower、Startup 发射、零方向、自目标、缺 Living tag、错误 detector/kind、重复 candidate、开放 sample 时消散、终态后重新发射或重新开放接触。

等价 launch 重放返回同一 receipt；冲突 launch 清空 caller output。动作中断可与开放 sample 竞争，`EndForActionTermination` 负责关闭 sample 并将状态收敛为 Dissipated。

## 5. 新增 Automation

新增四条：

1. `Shanmen.0_0_10.CombatRuntime.SwordQi.DefinitionAndSwordBoundary`；
2. `Shanmen.0_0_10.CombatRuntime.SwordQi.LaunchLifecycle`；
3. `Shanmen.0_0_10.CombatRuntime.SwordQi.ImpactPolicyAndOrdinals`；
4. `Shanmen.0_0_10.CombatRuntime.SwordQi.DefenseReplayAndTermination`。

关键 fixture：`BaseDamage=10`、`AttackPower=40`、`Coefficient=0.5`，raw damage 为 30；Spirit shield 吸收 7，final damage 为 23。两个等价 execution 重现 Launch/Impact identity；改变 activation sequence 则得到不同 Launch ID。

## 6. 编译与执行

新增源码后的首次 Editor build 通过：UHT 生成 3 个文件，8 actions，executor 45.15s，总耗时 49.91s，日志 SHA `24427FEA6EF82535189D2C53A7B83681DAEF6FBF424F650EE41BBC96B32DC4CA`。

exact Sword Qi Automation 首次真实运行即为 4/0，没有产生源码修正轮。随后运行整个 CombatRuntime 映射组与 0.0.10 全量套件：

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `test_sword_qi_exact.log` | Sword Qi exact | 4/0 | `BB0C5E1D62B8A637214BAB1A8E184DBD69C2E0CB703BC510A6E62034EA437511` |
| `test_combat_runtime_mapped.log` | `Shanmen.0_0_10.CombatRuntime` | 126/0 | `485393C83B3A419F5BFBC572A8843E3115970652B4A8C7AEAC1B7B1CCBF40506` |
| `test_full_0_0_10.log` | `Shanmen.0_0_10` | 755/0 | `E3EF30196ED3A17284D39E8D95CB3D1D9B89075613C0A0F5407BD251B285F0F3` |

所有最终日志均有 native test exit 0；Fail 与 Fatal 为 0。full 首末 Success 为 `2026.09.02 19:30:11.760 -> 19:58:45.185 UTC`，约 28m33.425s。

## 7. 回归与静态门禁

三个修改源码路径都匹配 `Source/ShanmenCombatRuntime/**`，要求一组 `Shanmen.0_0_10.CombatRuntime` 证据：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=test_combat_runtime_mapped.log
SELF_TEST: PASS 273/273
BOUNDARY_SCAN: PASS / 0 matches
GIT_DIFF_CHECK: PASS
```

- coverage log SHA：`C03AD5DB4C3BDDB2B724CF90BC5EA7292570E2908EC98A3ED817D0F6AE463860`；
- self-test log SHA：`7511F8052BECC706C0B779BC65250B2565B736026144521AAAFDD8607E98865A`；
- regression map SHA：`00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。

边界扫描对 `GetWorld|SpawnActor|ApplyDamage|TakeDamage|Commit|Consume|Tick|Timer|FMath::Rand|FRandomStream` 无命中。

## 8. 最终构建

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

- Game Development：5 actions / executor 37.75s / total 43.60s / native 0 / log SHA `E938B3CFE67489C7D775C683D5B6E6CDC7A9F5ECEB0E8D0AD79680E59559B40B`；
- Editor Development：up to date / 0 actions / total 1.09s / native 0 / log SHA `993278A7CDDF9BCDCD01BFBCC769B9F7F2F65FEC6BCE27B9748B640AFC71091A`；
- `demo_map.exe`：356,204,544 bytes / SHA `41B8F6054A8236116D9D1977DBF297120DA2D85D2B090F6E8B99C61F12D53B5A`；
- `UnrealEditor-demo_map.dll`：14,848,000 bytes / SHA `0EBC93380817B435F92B8AA54466F12EEB6CFCADAAE478299A82B4499878E921`。

构建只验证编译与链接，没有启动产品。

## 9. 边界与提交范围

计划提交 3 个新增源码文件与本 Report/Log，共 5 个文件。没有修改既有生产源码、Content、地图、资源、配置、系统权限或用户设置。未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料保持未暂存；`Saved/Codex/P18.0` raw logs 不入 Git。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-0-sword-qi-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-0-sword-qi-contract/Docs/Report/Dev.D.UE.0.0.10.P18.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-0-sword-qi-contract/Docs/Log/Dev.D.UE.0.0.10.P18.0.r0_log.md>
