# Dev.D.UE.0.0.10.P26.0.r0 Report

## 1. 结论

P26.0 已建立“物品防具抗性元数据 → CombatCore canonical 防御层”的类型化、确定性、
失败关闭投影契约。它让后续火焰、毒性、灵性、物理等伤害通道抗性可以由唯一装备物品
身份进入现有 Impact 结算，而不新增第二套防御算法或装备真值。

规划仍把正式防具槽位、抗性清单和数值留待后续讨论，因此本轮没有给现有道袍或任何
canonical 物品写入抗性数值。当前目录行为保持不变；只有同时具备显式
`DamageResistance` 语义和合法元数据的防具才能产生抗性层。

## 2. 类型化物品契约

`Fdemo_mapItemDamageResistance` 为每条抗性保存一个伤害 Gameplay Tag 与减伤比例：

- Tag 必须是 `Shanmen.Damage` 的严格子标签，不能用根标签捕获全部伤害；
- 比例必须为有限值且严格位于 `(0, 1)`，被动防具不能变成完全免疫；
- 同一物品禁止重复标签，也禁止父子标签同时出现，避免一次 Impact 双重减伤；
- 抗性元数据必须与 `DamageResistance` 语义同时存在；
- 仅 `ArmorCategory`、唯一 `ArmorSlot`、`MaxStackSize == 1` 的兼容装备可以声明该语义。

新枚举值追加在既有语义末尾，保持历史枚举序号不变。现有物品定义全部保持空抗性数组，
目录验证和旧存档路径因此没有发生内容或平衡迁移。

## 3. canonical 防御层投影

`Fdemo_mapShanmenArmorResistanceProjection::TryProject()` 是纯函数。调用方必须提供：

1. 完整物品定义；
2. 精确装备实例 GUID；
3. 精确目标实体 GUID；
4. 已合法且带 `TargetLiving` 的基础防御快照。

投影按伤害标签文本排序，再为每条抗性生成稳定 LayerId。身份由目标、装备实例、定义和
伤害 Tag 共同决定；同一输入可重放，不同装备实例不会碰撞。每层使用现有
`ReduceFraction`、`FShanmenDefenseOrder::Resistance`、`DefenseArmor` 和
`RequiredDamageTags` 契约，保留 exact SourceInstanceId，并与已有 Shield、Guard、
Lethal Interception 等层在同一 Resolver 中组合。

无抗性语义和元数据时返回成功的 `NotApplicable` 并原样保留基础快照；输入、定义或层
身份冲突时整体拒绝，不返回部分投影，也不修改调用方快照。

## 4. 自动化证明

精确组 `Shanmen.0_0_10.Product.ArmorResistanceProjection`：

| Test | Result |
|---|---|
| `CatalogAndNoOp` | Success |
| `DamageChannels` | Success |
| `DeterminismAndComposition` | Success |
| `FailClosed` | Success |

关键断言包括：

- 现有 `ArmorRobeLevel1` 没有被暗中写入抗性，投影是成功无操作；
- 物理 25% 抗性把 100 点 Slash 结算为 75，灵性 40% 抗性结算为 60，未列出的 Mental
  仍为 100；三条路径均满足伤害守恒；
- 10 点 Shield 先吸收，剩余 90 再应用 25% 防具抗性，最终为 67.5；
- 相同输入稳定重放相同 LayerId，不同装备实例得到不同 LayerId；
- 缺语义、缺元数据、错误槽位、根 Tag、100% 抗性、NaN、父子 Tag 重叠和重复投影均
  失败关闭。

聚焦组最终 4 Success / 0 Fail / Queue Empty；日志 SHA-256：
`44EC9D357439D6D354DAB5F3839E25B7457E964A040B0FC9FC4F9D97A933BF32`。

## 5. 改动文件驱动回归

加入 Report/Log 后共 10 个本阶段路径；8 个实现、测试和流程路径命中 3 条规则，文档为
忽略路径。覆盖门要求并独立验证 9 个精确组：

| Exact group | Success | Fail |
|---|---:|---:|
| `Shanmen.0_0_10.Product.ArmorResistanceProjection` | 4 | 0 |
| `Shanmen.0_0_10.Items` | 77 | 0 |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 |
| `demo_map.ItemUseAndArmor` | 46 | 0 |
| `demo_map.P4.Hotbar` | 7 | 0 |
| `demo_map.ItemEconomySchema` | 24 | 0 |
| `demo_map.Profile` | 211 | 0 |
| `demo_map.CodeB` | 60 | 0 |
| `demo_map.V3.WorldInteraction` | 4 | 0 |
| **合计** | **442** | **0** |

全部日志有唯一 RunTests 命令、至少一个原生 Queue Empty 标记且无 Fatal/Ensure。覆盖门最终
结果：`PASS Changed=10 Rules=3 Required=9 Logs=9`；证据 SHA-256：
`5CA514FCBC2D69510032FA46E623BEB28FD23C483EE047596F001B8C6C547866`。新增规则的正反例已进入映射器自检，453/453 PASS；SHA-256：
`687E34D9D89DC76408296F0A2772336E75F9270220C4B0EE964863E17DAFB710`。

## 6. 构建与静态检查

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| 首次 `demo_mapEditor Win64 Development` | Succeeded | 0 | `5C6B9733C0028CEC2CA64D6065E58924A59561D80FF6621516D1A11ACE062B46` |
| 最终 `demo_mapEditor Win64 Development` | Succeeded | 0 | `B3F61842C3795E6D0A0F6687D31F168DE6DF65AACA1312D37EA082550448FF84` |
| 最终 `demo_map Win64 Development` | Succeeded | 0 | `859BF49F8EFCF867395FD5E2C5D0461CD1FF1DC9600FDD3C3DF1AAA6D27A4574` |

- `git diff --check`：PASS；Regression Map：schema 1 / 250 rules；
- 投影对 World、Actor/Component、Timer、RNG、ApplyDamage 和物品库存写 API：0 命中；
- 最终静态证据 SHA-256：
  `96488FE5C8D7103646A33CBD9CB5CC99C00A773F6DB2D77326FFB5E94FC1E2C3`；
- 最终项目相关 Unreal 进程：0；
- `UnrealEditor-demo_map.dll`：19227648 bytes，SHA-256
  `6A5B214F09F91AC240EA6DEE4B4D49CD721298FC506BE1BD3845286D0F3491C4`；
- `demo_map.exe`：359921664 bytes，SHA-256
  `60C30A10A570C28C8E512522679D6315CBD4753E7051CA230F83B4F43364EAD8`。

## 7. 首错与修复

源码编译、9 个测试组、覆盖门及双目标构建均一次通过。第一次静态边界扫描把注释中的
`inventory/resources` 与 `TArray::Reserve` 误识别为库存写操作，因此按设计失败；该原始
证据已保留。复查规则改为具体运行时类型/API 后全部 0 命中。未为了报告美观删除首错，
也没有对产品实现进行规避性修改。

## 8. 权威与未冻结内容

- Code A 物品目录仍是抗性元数据来源；本轮不读取或修改运行中库存；
- CombatCore 仍是减伤顺序、Tag 匹配、Receipt 和守恒的唯一结算权威；
- 投影没有资源扣除、耐久消耗、网络、存档、World 或表现副作用；
- 正式防具抗性列表和数值未冻结，现有道袍玩家行为为零变化；
- 后续 P26.1 可让唯一已装备 ArmorSlot 权威调用此纯投影，但仍必须显式授权具体内容值，
  不能按名称或类别猜测抗性。

## 9. P/F 边界

P 阶段已证明：物品目录验证、Tag 通道过滤、确定性 LayerId、Shield 组合顺序、伤害守恒、
失败关闭、旧物品/迁移/存档/Code B/世界交互回归及 Editor/Game 双目标构建成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package，因此不声明防具抗性已经具有玩家可见内容或实机体验。

## 10. GitHub 交接

基线提交：`ec6ed2521f0bec742e2938e32a339be65dca3c95`（P25.9）。
分支：`agent/0.0.10-p26-0-armor-resistance-projection`。只提交本阶段 8 个实现/测试/流程
文件、本 Report 与本 Development Log；全部其余未跟踪文件保持未暂存，
`Saved/Codex/P26.0` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-0-armor-resistance-projection>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-0-armor-resistance-projection/Docs/Report/Dev.D.UE.0.0.10.P26.0.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-0-armor-resistance-projection/Docs/Log/Dev.D.UE.0.0.10.P26.0.r0_log.md>
