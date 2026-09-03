# Dev.D.UE.0.0.10.P18.5.r0 Report

## 1. 结论

P18.5 在 P 阶段边界内完成，结论为 **PASS**。

本轮为 P18.4 的 Run-scoped Sword Qi Product Controller 增加薄型命令适配层。适配器只接收调用方拥有的稳定输入事件 ID，并在所有可用性门禁通过后采样一次 origin/aim，再派生确定性 IntentId，调用唯一 `Ademo_mapGameMode::RouteSwordQiIntent` 产品入口。它不拥有真实按键、不读取装备或属性、不修改库存、不创建第二套动作/伤害/生命权威。

```text
Sword Qi Input Adapter exact:          4 Success / 0 Fail
Shanmen.0_0_10 full:                 777 Success / 0 Fail
Required legacy groups:              443 Success / 0 Fail
Regression coverage:                  PASS (Changed=7 / Rules=2 / Required=54 / Logs=10)
Regression gate self-test:             PASS 285/285
Boundary scan:                         PASS (Files=2 / Matches=0)
Game + Editor Development:             PASS / native status 0
```

本轮没有绑定键盘、鼠标、手柄或 Input Action，没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。因此证明的是未来输入事件到现有剑气产品控制器的确定性代码闭环，不宣称真实输入手感、表现或人工验收通过。

## 2. 输入事件与确定性身份

`Fdemo_mapShanmenSwordQiInputAdapter` 为无状态适配器。调用方提供：

- 当前 gameplay surface 是否允许输入；
- 唯一产品路由是否可用；
- active RunId；
- 调用方拥有的稳定 InputEventId；
- origin 与 aim 的一次采样函数；
- 唯一产品 route callback。

IntentId 使用命名空间 `demo_map.SwordQi.InputIntent.r1`，由 `RunId + InputEventId` 确定性派生。相同 Run/事件永远得到相同 IntentId；事件或 Run 任一变化都会得到不同身份；无效身份在空间采样前失败关闭。

轨迹不参与 IntentId 派生是有意设计：同一事件 ID 若被错误地复用为另一 origin/aim，不会伪装成新事件，而会抵达 P18.4 Controller 并以 `IntentIdConflict` 明确拒绝。

## 3. 一次采样与门禁顺序

适配顺序固定为：

```text
gameplay gate
  -> unique product route availability
  -> active Run identity
  -> stable input-event identity
  -> sample origin/aim exactly once
  -> capture immutable Sword Qi intent
  -> invoke sole product route exactly once
```

Gameplay blocked、产品路由不可用、Run 无效或 EventId 无效时，采样与产品 route 均为 0 次。空间样本无效时只采样一次，产品 route 为 0 次。有效方向被规范化；结果校验使用容差相等，避免对二次归一化后的浮点逐位相等产生错误依赖。

## 4. GameMode 唯一入口

`Ademo_mapGameMode::RouteSwordQiStartInput` 只负责组合现有状态：

- 确认 Sword Qi Controller active 且 invariants 有效；
- 确认 Controller 与 CombatRunCoordinator 指向同一 Run；
- 确认玩家、物品 authority 与属性 authority 可用；
- 延迟执行调用方的 origin/aim 采样；
- 把捕获后的 intent 委托给既有 `RouteSwordQiIntent`。

装备 exact item、final AttackPower、Run sequence、动作仲裁、载体创建、命中与 terminal 生命周期仍完全属于 P18.4/P18.3 的既有产品链。输入层没有复制这些职责。

## 5. 重放、冲突与忙态

- 首次有效事件：采样一次、路由一次，并由 P18.4 冻结 exact item、AttackPower 与 launch command；
- exact replay：相同 Run/EventId/轨迹得到相同 IntentId，复用冻结命令，不重新读取装备或属性，不重复 reserve sequence；
- identity conflict：相同 Run/EventId 搭配另一轨迹，以 `IntentIdConflict` 失败关闭；
- Host busy：第二事件已冻结但 route 返回 `HostBusy`，上一发 terminal retirement 后，以相同事件与轨迹显式重试即可启动同一 command；
- authority drift：捕获后的装备与属性变化不能改写已冻结 command。

适配器没有内部自动重试循环；是否再次提交同一事件由上层命令所有者明确决定。

## 6. Automation 证据

新增四条聚焦测试：`DeterministicIdentity`、`GatesBeforeSampling`、`AppliedReplayAndConflict`、`BusyRetry`。

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi Input Adapter exact | 4 | 0 | `F6AB77E49D8DCF38065A17905659AFEA5B866EA445F3926E562C91A1461477FA` |
| `Shanmen.0_0_10` full | 777 | 0 | `030FF8830E3E5FBCB3A9E5DF9CDB65F4D026CB1C93AE28BC61CF5ED4B6471443` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `D8E4DE6C35616A292D416E6C10B769C41CA7A302B674D0A574C95DCAD6BB2379` |
| `demo_map.Profile` | 211 | 0 | `0F410990AB507474469B227EC9E5B5F395111350465C29BF8ECD0ABE88026C4A` |
| `demo_map.CodeB` | 60 | 0 | `B1464D8BD764F656CB38514FD109A85557D57E7E7A4B9E18B4446019E7EE7D7C` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `9AAC574AD8EA88755D5A249D2A47CE776464630B5B3B140485CE1EDBC6A796E7` |
| `demo_map.P4.Hotbar` | 7 | 0 | `00B0F85E83627627DD4A56E9E0533687379F345CC0C1386C86A59268F6F4FE68` |
| `demo_map.V3` | 29 | 0 | `50BC640ED6537DE3B7867503F326DB3B6550BF29AC93111E800A9297ECDCEFD3` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `F3167D95FD35ADC1D604F641EBC82FE8EAF8851399F12383F12A7A5A601C478F` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `8D0BCA2CFE5D7F531552CDF4D8B8705A95E670C0B393E687699D40D71479FDD7` |

所有采用日志均包含 terminal `TEST COMPLETE` 与 exit code 0，Fail、Fatal、Unhandled 为 0。完整套件首末 Success 为 `2026.09.03 01:40:11.559 -> 02:12:02.894 UTC`，约 31m51.335s；相对 P18.4 的 773 条精确增加 4。

## 7. 静态复查修正

初版聚焦与完整套件均通过。随后源码复查发现 Result invariant 使用 exact `FVector` 相等比较；空间样本与 Intent 都会执行安全归一化，非轴向方向可能因二次浮点归一化产生非语义差异。

提交前将方向比较收紧为容差相等，并把聚焦 fixture 从轴向 `(10,0,0)` 改为非轴向 `(10,3,2)`，同时补充“无效空间样本只采样一次且不得调用产品 route”的断言。修正后重新编译，并重跑 exact、完整与全部 legacy 证据；最终结果如上。没有为测试修改产品公式或既有 authority。

## 8. 改动驱动回归与边界

Regression map 新增 `SwordQiInputAdapter` 路径规则，并把 GameMode 依赖扩展到该聚焦组。最终门禁：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=54 Logs=10
SELF_TEST: PASS 285/285
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage SHA：`94A1818183A7B21FBE1DD0E8396254380A332A449CCF988102A12FD88F043397`；
- self-test SHA：`1FCA481362D1F4B3AB04271BA030E0F7F0A964A34EF15557EE6D287F788BEB9F`；
- regression map SHA：`F4493AA772FDE3EE9494C4B3CEA672029A32A02EBBE5F67E376D3C422F5F4975`；
- boundary SHA：`3AE3D28D78DAA7B11F54480615608B79DF0FB3FC3832E782E85943E874C9DB63`。

边界扫描确认生产适配器没有直接 GameplayStatics/ApplyDamage/TakeDamage、旧 SkillProjectile、RNG、库存写事务、物品/属性读取、物理输入绑定、声音或 Niagara 调用。

## 9. 构建、产物与 P/F 边界

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded | 27 / 149.84s | `55CFB0D47D99128CEB1F731125E268CBC185424A5A87C1BAD88A64B6A6B9F3E7` |
| Editor after invariant tightening | Succeeded | 5 / 15.88s | `B45C57B00C2596C216A798FB503CBBEFE4DF0F20317B31B8189D7783265904E3` |
| Game Development final | Succeeded / native 0 | 26 / 150.48s | `9F2E7230C40D022F41747C03AFCA0D457EB6FDE13F7306CD7A2EEF58538EB4E9` |
| Editor Development final | Succeeded / native 0 / up to date | 0 / 1.16s | `9645D755F9ABDE0D6C59E50DF12A8183DB27CD0B1D4DC04FC140C8CB52DD44A1` |

最终产物：

- `demo_map.exe`：356,401,664 bytes，SHA-256 `96715F6072083C58F6BF194E0214505CBAB50E5D2F562A7B4F70A011F4B7FA6A`；
- `UnrealEditor-demo_map.dll`：15,078,912 bytes，SHA-256 `4F9DA91A767C5A373B9EE8F71878268A2AB471FA4895DEA69AB15FE977E8979B`。

本轮计划提交 3 个新增源码、4 个修改文件及本 Report/Development Log，共 9 个文件。未修改 Content、地图、资源、配置、Windows、UE Engine、存档 schema 或既有装备/库存/属性/伤害/生命权威。raw logs 只保存在本地 `Saved/Codex/P18.5`。

## 10. 下一阶段

P18.6 建议建立 Run-scoped 的逻辑命令事件所有者：只负责给一次 Sword Qi 逻辑命令分配稳定、可重放的 InputEventId，并调用本轮适配器；仍不绑定真实物理按键。正式 Enhanced Input 资产、按键选择、动画、声音、特效与手感继续留给明确授权的 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-5-sword-qi-command-adapter>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-5-sword-qi-command-adapter/Docs/Report/Dev.D.UE.0.0.10.P18.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-5-sword-qi-command-adapter/Docs/Log/Dev.D.UE.0.0.10.P18.5.r0_log.md>
