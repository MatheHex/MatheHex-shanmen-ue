# Dev.D.UE.0.0.10.P27.0.r0 Development Log

## 1. 目标

- 为既有 FormationProductHost 建立受物品 ActiveRun 与 CombatRun 双重约束的产品准备入口；
- 冻结阵法投放所需身份、内容版本、阵图与一次采样位姿；
- 保证预检失败不消耗身份，准备过程不写库存；
- 把新增入口纳入改动文件驱动回归；
- 完成 Report/Log GitHub 交接。

## 2. 基线与范围

- 基线：`d18ab44664906c6b6e176ac5a910a84ba38480c2`（P26.5）；
- 分支：`agent/0.0.10-p27-0-formation-product-authority`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不改阵法定义、材料数量、影响数值、物品 schema、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. Formation Product Authority 实现

新增 `demo_mapShanmenFormationProductAuthority.h/.cpp`：

1. `Fdemo_mapPlayerFormationActionReservation` 只允许 CombatRunCoordinator 写入序号与 Action；
2. Reservation 重算确定性 ActivationId，并要求 canonical formation action 与精确 Source.Player 标签；
3. `Fdemo_mapShanmenFormationDeploymentCommand` 冻结 Run correlation、reservation、diagram、origin 与
   normalized forward；
4. `PrepareDeployment()` 先验证外部几何，再读取真实 item authority ActiveRun 与 snapshot；
5. 只有 item Owner/revision 与同一 CombatRun 全部一致，才允许预留 Action；
6. 该层不持有 UObject、不扫描 World、不放置 Actor，也不执行库存事务。

## 4. CombatRunCoordinator 改动

新增 `TryReservePlayerFormationAction()` 和独立的
`NextPlayerFormationActivationSequence`：

- `RunId` 取 exact durable ActiveRun；
- `OwnerId` 取 durable item/profile owner；
- `SourceEntityId` 取 CombatRun 玩家实体；
- `ActionDefinitionId` 取 canonical formation action；
- `Content` 取当前 item-authority snapshot；
- 每次成功预留后序号单调递增；
- `TryEndRun()` 与 `Reset()` 均把序号恢复为 1。

## 5. 产品级测试

在既有 FormationProductHost 测试文件中新增
`FormationProductAuthority.AuthorityRunAndHostStart`：

- 三种预检失败与不消耗序号；
- durable Owner 与 combat SourceEntity 的双身份；
- authority content/revision 冻结；
- direction normalization；
- prepared command 直接启动既有 ProductHost；
- 连续命令使用不同确定性 ID；
- item authority snapshot 前后相等；
- CombatRun 结束后序号复位。

## 6. 自动化结果与首错保留

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationProductAuthority` | 1 | 0 | `148D44B57315B4D752D59F29BFE6110E928995486000F2D44946C98C8ABB0A4B` |
| `Shanmen.0_0_10` | 1,322 | 0 | `977F2A245242C4BE0EDA56D3673C6D84877B3CBB20C80C03DECC9000AF540F29` |
| `demo_map.V3.Attributes` | 4 | 0 | `612AAA801FC5A5EFC123BF3BAFC2230FB1E53675CDAE24234BE671AE9297CFE0` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `640BBAAB36DA908FD3B6531CEA6B7BE693CADA9CCB182DC08D027E5E48B3DF8C` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `DFC38023896727BA519246A0EA9002A0889D56107BE55F04B0CDD5C1750D9D49` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `A67C38F1C1DDD3E81A8C32D2652782F8A2C671FC573C2E6029AFEF9DBDFF1399` |

每份最终日志均有至少一个 Success、0 Fail、0 Fatal、终端队列完成标记和进程退出码 0。

首次 `ItemUseAndArmor` 运行返回码为 0，但只记录 45 Success，最后一项 `TerminalCleanup` 没有完成
记录，终端队列标记也缺失。覆盖门正确判为 unhealthy evidence。首错日志保存在
`Saved/Codex/P27.0/ItemUseAndArmor_first_truncated.log`，SHA-256
`9DA3969E03D2134233B3EE8E323D5B2C231080DEB291B023AFA12AF2A2D146B9`；独立重跑得到完整 46/46。

## 7. 改动驱动覆盖

新增 `FormationProductAuthority` 映射规则，要求 focused authority、CombatRun、FormationHost/Session、
material adapter、Items、FormationDeployment、CombatCore 与完整 0.0.10 证据。协调器和 Host 测试的
既有规则再补充旧战斗兼容组。

最终门禁：`REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=50 Logs=6`。

- 覆盖日志 SHA-256：
  `948ECB6E82E9CE0D3E7E7E6529E75BDC1AFF79C61AFDE7DC3F0CA177FCD5FFB4`；
- 映射器自测：`459/459` PASS；SHA-256
  `37554C33817CB707356992239A8C2C8A0D7F24F8BF0050EBEF9C29FB5F7A7CF2`。

## 8. 构建与静态检查

| Target | Result | Duration | SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 557.92s | `2AB370AF1016889C3D7808D6AFD5A3034209DAF05E86DBD99C8F878C4EF26A4B` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 0.98s | `E012FA6B31FD184B816AA7359031370F354B459BE1E7D44101F69BC5055D762B` |

最终二进制：

- `demo_map.exe`：360,004,096 bytes；SHA-256
  `667E13B45B91ED1EA4B4869B9D6D8D6110AFD5D51A933BCA1BBB7402B35CCFDC`；
- `UnrealEditor-demo_map.dll`：19,324,416 bytes；SHA-256
  `71DD073C8F06A629A78A83A5BF781D5C8B84107A4A62AF65BCE0F0F098593B91`。

`git diff --check`、regression map JSON 解析与新增运行时代码边界扫描均 PASS。边界扫描未发现
World/Actor 创建或扫描、RNG、`ApplyDamage`。

## 9. 未执行项与后续边界

没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
因此本轮只声明 P 阶段权威命令与 Host 兼容成立，不声明阵法已从玩家输入进入可见世界。后续若接
产品调用点，必须继续复用这一唯一 authority command，不得另造 Run、Owner、内容版本或库存路径。

原始证据位于 `Saved/Codex/P27.0`，不进入 Git。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationProductAuthority.h`
2. `Source/demo_map/demo_mapShanmenFormationProductAuthority.cpp`
3. `Source/demo_map/demo_mapCombatRunCoordinator.h`
4. `Source/demo_map/demo_mapCombatRunCoordinator.cpp`
5. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
6. `Scripts/ShanmenRegressionMap.json`
7. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
8. `Docs/Report/Dev.D.UE.0.0.10.P27.0.r0_report.md`
9. `Docs/Log/Dev.D.UE.0.0.10.P27.0.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-0-formation-product-authority>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-0-formation-product-authority/Docs/Report/Dev.D.UE.0.0.10.P27.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-0-formation-product-authority/Docs/Log/Dev.D.UE.0.0.10.P27.0.r0_log.md>
