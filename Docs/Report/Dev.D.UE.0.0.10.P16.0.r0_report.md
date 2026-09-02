# Dev.D.UE.0.0.10.P16.0.r0 Report

## 1. 结论

P16.0 完成 0.0.10 第一条“异常状态必须由专用物品治疗”的产品契约：新增唯一规范物品 `一阶定脉丹`，并把它接入既有 Meridian Shock 条件权威与既有 ShanmenItems 局内数量权威。

本阶段形成的顺序契约是：

```text
准备同一局内物品数量
  -> 以同一条件修订生成治疗意图
  -> 条件权威清除经脉震荡并返回不可变回执
  -> 仅凭完全匹配的治疗回执提交物品消耗
```

失败或重放不会另建库存真值：治疗前取消保留物品与条件；治疗成功后只能提交，不能使用旧准备证据改走取消；相同请求与回执可以确定性重放。

最终结果：

```text
Meridian Shock treatment focus:   4 Success / 0 Fail
Combat condition focus:          19 Success / 0 Fail
ShanmenItems focus:              72 Success / 0 Fail
Shanmen.0_0_10 full:            717 Success / 0 Fail
Regression coverage:            PASS (19 / 8 / 19 / 12)
Editor + Game Development build: PASS / native status 0
```

## 2. 产品内容

新增规范定义：

- DefinitionId：`Prototype.Item.Consumable.MeridianStabilizingPill.Level1`；
- 中文名：`一阶定脉丹`；英文名：`TIER 1 MERIDIAN STABILIZING PILL`；
- 一阶消耗品，最大堆叠 20，允许热栏与购买；
- 买价 45、卖价 22、价值 22；
- 唯一 Gameplay Semantic：`MeridianShockTreatment`；
- 普通回血丹没有该语义，不能借用定脉治疗能力。

Catalog 从 40 项增至 41 项，并继续强制只有一个定义可以持有该治疗语义。内容身份更新为：

```text
Version: CodeB.Content.0.0.10.P16.0
Digest:  9B789DB381BB934F772326A5217094F19C654D5AF51D5623EDB0A108DBA4CC7B
```

P11.7 内容身份仍保留为可识别历史证据；既有 DefinitionId 不迁移、不重映射。

## 3. 条件治疗权威

`Udemo_mapShanmenCombatConditionComponent` 新增不可变 Treatment Intent、Receipt 与 Result：

- TreatmentId 由 Run、目标、固定时间线、物品实例、规范 DefinitionId、条件修订和条件 DefinitionId 确定性派生；
- Intent 只能指向 `一阶定脉丹` 与规范 Meridian Shock；
- 治疗要求 Run、目标、时间线与条件修订全部相等；
- 成功后移除唯一速度修饰器、将条件置为 inactive、条件修订只增加一次；
- 同一 TreatmentId 重放返回原始 Receipt，不重复移除或增加修订；
- 错误 Run、目标、时间线、修订、失效条件或冲突身份全部失败关闭；
- 已处理治疗进入组件不变量与清理生命周期。

## 4. 物品事务桥

新增 `Fdemo_mapShanmenMeridianShockTreatmentAdapter`，只桥接两个既有权威，不复制健康、条件、库存、时钟或输入状态。

Prepare 阶段验证：

- 当前内容身份与非陈旧 Authority Revision；
- 精确 active Run correlation 与生命周期回执；
- 物品实例确实位于该 Run 的有序局内库存；
- Definition 同时存在于 ShanmenItems snapshot 与产品 catalog；
- 仅规范治疗语义、Quantity 资源、已提交 Run reservation 可用；
- 不允许另一个未终结数量意图占用同一实例；
- 数量充足，并冻结 `ExpectedQuantityBefore`。

Finalize 阶段：

- Commit 只接受与准备 Intent 完全匹配的条件治疗 Receipt；
- Cancel 额外要求当前条件状态仍 active，且 Run、目标、时间线和条件修订仍与准备 Intent 相同；
- 因此“先治疗、后拿旧凭证取消消耗”无法生成合法请求；
- 成功提交将数量 `4 -> 3`，治疗前取消保持 `4 -> 4`；
- 持久化 prepare/finalize 使用既有 ShanmenItems 命令与回执，不双写 legacy inventory。

这不是跨两个权威的一次数据库事务。中断恢复依赖确定性 ID、持久化数量意图和条件 Receipt 重放；后续产品路由必须拥有 `prepare -> treat -> commit` 的唯一调用顺序。

## 5. 首次失败与修正

首次 `demo_map.Profile` 回归得到 `210 Success / 1 Fail`：

```text
demo_map.ProfileTrade.01.CatalogExactImmutableOrderAndPrices
demo_mapProfileTradeTests.cpp(242)
```

原因是新增 45 灵石定脉丹后，精确有序价格 fixture 未加入 `45`，而产品 catalog 与实际投影一致。只修正测试期望数组；未放宽 catalog 顺序或价格验证。最终 Profile 为 `211/0`。

首次失败日志保留，SHA-256：`716EE4E119E323412DFC5C2DE1DA989E2AA9C44AE973281D3870FD58E0173A8C`。

收尾审计另发现取消接口缺少实时条件门禁；已将 live active condition revision 纳入 API，并增加“治疗后旧凭证不得取消”反例。该修正后的全部证据见下表。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Meridian Shock treatment | 4 | 0 | `E92E101B7D53209BF99B37543D2C481B7F0B12FD05E96E9FD59EE72942B2871F` |
| Combat condition Meridian Shock | 19 | 0 | `5D0A75B552CAA62F2C10B29EF0130841EE48B001FBC2C2C37C5F776573848494` |
| ShanmenItems | 72 | 0 | `BD4840B2236A2E8B4A1A4D4EF0F81548F01A1949C4A014DFF50F6515384E9A9E` |
| CodeB | 60 | 0 | `57DB2F4B7ED6778E8F90BA04A4354ABBD02A5A279F52225C1DEB76391F808E1D` |
| ItemEconomySchema | 24 | 0 | `E567475743C524CA79B17051E70405FCE2AA75A472DBA420A7B6146998A4AD35` |
| ItemUseAndArmor | 46 | 0 | `C2901CF39B5B17920E28A2A830E1DDF3AB405A2FCBDA876B228E558C15DA3A61` |
| P4 Hotbar | 7 | 0 | `7EDD75603229C79BCC49CD11EFABA95E55BF45AD3C9992E72120FB85FA39BEAB` |
| Profile final | 211 | 0 | `4C59A21C75C795C1AF0B751BBE7CC0EC280FE48C4A7A0A6BA8CFB526EC9AC82F` |
| V2 ranged compatibility | 22 | 0 | `CA83615035A4E62073617769409D11996A4DA97B85002C2A16584A9D12ED4658` |
| V3 attributes | 4 | 0 | `179ECE5A8E5DCEF173140243B22730C846A9E147C76396C9A18BEF2CEA607453` |
| V3 world interaction | 4 | 0 | `815A6FBBD42F126553D9596650A30A8522C482380D113932006FA61B9116FCC4` |
| `Shanmen.0_0_10` full | 717 | 0 | `83AB21E901C7647BAE9F6A9CEDDE4D611F3A35867491C2EA5BA3D4441486D933` |

全量首末 Success 时间为 `03:15:08.021 -> 03:44:25.089 UTC`，约 `29m17.07s`，随后出现唯一 `Automation Test Queue Empty 717 tests performed`。最终日志中 Fatal error、Unhandled Exception 与 Ensure condition failed 均为 0。

## 7. 流程与边界证据

```text
REGRESSION_COVERAGE: PASS Changed=19 Rules=8 Required=19 Logs=12
SELF_TEST: PASS 270/270
JSON_PARSE: PASS
GIT_DIFF_CHECK: PASS
BOUNDARY_SCAN: PASS
```

SHA-256：

- regression gate：`3362DC1FD424DB96FE59AFD3B131A665E56BC52B368AF1E14A5E8D22A65FB371`；
- self-test：`44B3437B88198790A05644C7D71C2B201CA5F134D7B478918C0CEAFC6592FF84`；
- JSON parse：`48C32879BFB681137257F2FA697C74FED7A350541D953E584A70FE8C7B067345`；
- diff check：`6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`；
- boundary scan：`987868770230B1BD9D2D2CDE3B236144DCE4A338FAD398B3A815B24D6A93222A`。

新增 adapter 精确映射要求 full、治疗、条件、Items、CodeB、ItemEconomySchema、Profile、ItemUseAndArmor 与 Hotbar 证据；其它被修改文件继续触发 V2/V3 对应回归。边界扫描确认 adapter 不依赖 World、Actor、GameMode、输入、Tick/Timer、RNG、Damage 或 Spawn。

## 8. 构建证据

本轮使用 `-NoUBA -MaxParallelActions=1` 控制主机提交内存压力。

| Target | Result | Actions / UBT time | Status | Log SHA-256 |
|---|---|---|---:|---|
| Editor cancellation guard compile | Succeeded | 5 / 28.27s | 0 | `D465B14E9E8ACC03DBDC6AA4C9A79EED1A8DAF0C9AC548364CECFB98F7E9D1C6` |
| Game final | Succeeded | 4 / 24.84s | 0 | `076E487973446581116CF91A2D0867F3528C4C2BCD71EF1E2D6EC2BCFFD70C82` |
| Editor final | Succeeded, up to date | 0 / 0.92s | 0 | `05B4963BEB2C251505B92ECB2524542FF3496E82CFCC528A3FFE7ABC3F6F8210` |

最终产物：

- `demo_map.exe`：355,798,528 bytes，SHA-256 `0C74A36D2804C2B90878D101AED2882363C1FFE540B33B731B6853AC18473FE3`；
- `UnrealEditor-demo_map.dll`：14,399,488 bytes，SHA-256 `E160C82DF24DA6D42E1BCE9E9F6D60BAFCBFC78D4B13C298314E63145EC7D9AA`。

## 9. 修改范围与 P/F 边界

修改包括：规范物品语义与 catalog、内容身份及关联精确 fixture；Meridian Shock 条件治疗权威；新 treatment adapter 与 5 个 Automation 用例；改动—回归映射与自检；Report/Log。

raw logs 仅保存在本机 `Saved/Codex/P16.0`。长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 和用户资料未修改、未暂存、未提交。

本 Report 仅包含 P 阶段 C++ 契约、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. 后续

下一阶段应增加唯一产品 coordinator/route，在 Game Thread 上拥有 `prepare -> treat -> commit` 顺序与中断恢复：只允许在实时条件仍是同一 active revision 时取消；若条件 Receipt 已存在则必须重放 commit。随后再由正式热栏/输入路由调用该 coordinator，不让 UI 直接修改条件或库存。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-0-meridian-shock-treatment-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-0-meridian-shock-treatment-contract/Docs/Report/Dev.D.UE.0.0.10.P16.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-0-meridian-shock-treatment-contract/Docs/Log/Dev.D.UE.0.0.10.P16.0.r0_log.md>
