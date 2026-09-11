# Dev.D.UE.0.0.10.P26.0.r0 Development Log

## 1. 目标

- 为 0.0.10 防具方向建立类型化伤害通道抗性元数据；
- 把 exact 装备实例确定性投影到现有 CombatCore 防御层；
- 在正式抗性清单与数值未讨论完成前保持现有物品行为不变；
- 补齐改动文件驱动回归，运行精确测试矩阵与双目标构建；
- 生成 Report/Log 并独立推送本阶段分支。

## 2. 基线与范围

- 基线：`ec6ed2521f0bec742e2938e32a339be65dca3c95`（P25.9）；
- 分支：`agent/0.0.10-p26-0-armor-resistance-projection`；
- 起始 tracked tree clean；103 个既有未跟踪用户条目保持未暂存；
- 修改 item type/catalog validation，新增纯投影与自动化测试，更新回归映射及自检；
- 不修改现有物品内容值、存档 Schema、库存权威、CombatCore Resolver、GameMode、
  PlayerController、UI、World、地图或资产。

## 3. 设计决策

人工作战规划已明确防具应承载元素/类型抗性、异常抗性与小型长期防护，但正式槽位、抗性
清单和数值仍待讨论。本轮只冻结“如何表达和进入结算”，不冻结内容平衡：

- 新增 `Fdemo_mapItemDamageResistance { DamageTag, ResistanceFraction }`；
- 新增 opt-in `Edemo_mapItemGameplaySemantic::DamageResistance`，追加在末尾；
- 元数据与语义必须成对，且只允许唯一 ArmorSlot 防具；
- Tag 必须是 `Shanmen.Damage` 严格子标签，比例为有限 `(0,1)`；
- 禁止父子 Tag 重叠，避免层级匹配造成双重减伤；
- 所有当前 canonical 物品保持空元数据，不改变产品行为。

## 4. 投影实现

新增：

1. `demo_mapShanmenArmorResistanceProjection.h`
2. `demo_mapShanmenArmorResistanceProjection.cpp`
3. `demo_mapShanmenArmorResistanceProjectionTests.cpp`

纯投影要求有效 ArmorItemInstanceId、TargetEntityId、基础 Defense 和 `TargetLiving`。元数据
先按 Tag 字符串排序，再以 namespace
`Shanmen.Product.ArmorResistance.Layer.r1` 和目标/装备/定义/Tag 生成 LayerId。每层记录
exact SourceInstanceId，使用现有 Resistance 顺序、`ReduceFraction`、`DefenseArmor`、
RequiredDamageTags 和 RequiredTargetTags。

无授权内容返回 `NotApplicable` 并保留基础快照。非法输入、非法定义或重复 LayerId 返回
失败状态；构造始终在快照副本上完成，失败时不泄漏部分层。

## 5. 聚焦自动化

`Shanmen.0_0_10.Product.ArmorResistanceProjection` 共 4 项：

- `CatalogAndNoOp`：目录仍合法，现有道袍无调参，基础 Shield 原样保留；
- `DamageChannels`：Physical Slash 25%、Spirit 40%、Mental bypass 与守恒；
- `DeterminismAndComposition`：稳定身份、来源隔离、Shield → Resistance = 67.5、冲突拒绝；
- `FailClosed`：身份/语义/元数据/槽位/根 Tag/满减免/NaN/层级重叠全部拒绝。

结果：4 Success / 0 Fail / Queue Empty；SHA-256：
`44EC9D357439D6D354DAB5F3839E25B7457E964A040B0FC9FC4F9D97A933BF32`。

## 6. 改动映射回归

9 个精确组均独立运行：

| Exact group | Success | Log SHA-256 |
|---|---:|---|
| `Shanmen.0_0_10.Product.ArmorResistanceProjection` | 4 | `44EC9D35...A933BF32` |
| `Shanmen.0_0_10.Items` | 77 | `8DDD993B...DF674D3` |
| `Shanmen.0_0_10.CombatCore` | 9 | `558C2111...E8699C64F` |
| `demo_map.ItemUseAndArmor` | 46 | `852F23F8...B480615B` |
| `demo_map.P4.Hotbar` | 7 | `38288495...E9F2969` |
| `demo_map.ItemEconomySchema` | 24 | `91FA2C6A...8F68BD11` |
| `demo_map.Profile` | 211 | `71A7B590...153BF9129` |
| `demo_map.CodeB` | 60 | `E4A9788C...25721D68` |
| `demo_map.V3.WorldInteraction` | 4 | `BA2186E4...EF1BE426` |
| **合计** | **442** | **全部原始日志保留** |

所有组 0 Fail、Queue Empty 且进程原生退出码 0。最终覆盖门：
`PASS Changed=10 Rules=3 Required=9 Logs=9`；SHA-256：
`5CA514FCBC2D69510032FA46E623BEB28FD23C483EE047596F001B8C6C547866`。

回归映射新增 `ArmorResistanceProjection` 规则，要求聚焦投影、Items、CombatCore 和旧
ItemUseAndArmor 证据；正例证明证据完整时通过，反例证明只有聚焦日志时失败关闭。映射器
自检 453/453 PASS；SHA-256：
`687E34D9D89DC76408296F0A2772336E75F9270220C4B0EE964863E17DAFB710`。

## 7. 构建结果

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `P26.0_EditorBuild_attempt-1.log` | 首次 Editor 编译 Succeeded | 0 | `5C6B9733...ACE062B46` |
| `P26.0_EditorBuild_final.log` | 最终 Editor Succeeded | 0 | `B3F61842...0448FF84` |
| `P26.0_GameBuild_final.log` | 最终 Game Succeeded | 0 | `859BF49F...6D27A4574` |

最终二进制：

- `UnrealEditor-demo_map.dll`：19227648 bytes / SHA-256
  `6A5B214F09F91AC240EA6DEE4B4D49CD721298FC506BE1BD3845286D0F3491C4`；
- `demo_map.exe`：359921664 bytes / SHA-256
  `60C30A10A570C28C8E512522679D6315CBD4753E7051CA230F83B4F43364EAD8`。

## 8. 静态边界与首错

- `git diff --check`：PASS；Regression Map schema 1 / 250 rules；
- 投影对 World、Actor/Component、Timer、RNG、伤害副作用与库存写 API：0 命中；
- 最终静态证据 SHA-256：
  `96488FE5C8D7103646A33CBD9CB5CC99C00A773F6DB2D77326FFB5E94FC1E2C3`；
- 项目相关 Unreal 进程：0。

第一次静态扫描用宽泛词 `Inventory|Reserve|Consume`，命中了注释中的边界声明和
`TArray::Reserve`，产生预期外误报。原始失败文件
`P26.0_StaticBoundary_attempt-1.txt` 已保留，SHA-256：
`F0D32DEE40D9A9CAFC5D96AD8436253A54D0886CAF859817CB1D46B34126598D`。复查改用具体
运行时类型/API 后通过。源码编译、产品测试和构建没有失败。

## 9. P/F 边界

P 阶段完成：契约、验证、确定性身份、Resolver 组合、守恒、失败关闭、9 组回归、覆盖门、
静态边界和 Editor/Game 构建。

F 阶段未启动：未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package；未声明玩家可见防具内容已经实现。

## 10. GitHub 交接

只提交以下 10 个文件：

1. `Source/demo_map/demo_mapItemTypes.h`
2. `Source/demo_map/demo_mapItemTypes.cpp`
3. `Source/demo_map/demo_mapItemDefinitions.cpp`
4. `Source/demo_map/demo_mapShanmenArmorResistanceProjection.h`
5. `Source/demo_map/demo_mapShanmenArmorResistanceProjection.cpp`
6. `Source/demo_map/demo_mapShanmenArmorResistanceProjectionTests.cpp`
7. `Scripts/ShanmenRegressionMap.json`
8. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
9. `Docs/Report/Dev.D.UE.0.0.10.P26.0.r0_report.md`
10. `Docs/Log/Dev.D.UE.0.0.10.P26.0.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-0-armor-resistance-projection>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-0-armor-resistance-projection/Docs/Report/Dev.D.UE.0.0.10.P26.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-0-armor-resistance-projection/Docs/Log/Dev.D.UE.0.0.10.P26.0.r0_log.md>
