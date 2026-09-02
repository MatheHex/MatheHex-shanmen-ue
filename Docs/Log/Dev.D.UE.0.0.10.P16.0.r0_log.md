# Dev.D.UE.0.0.10.P16.0.r0 Development Log

## 1. 目标

建立 0.0.10 第一条条件专用治疗链：`一阶定脉丹 -> Meridian Shock exact revision -> durable item Quantity`，复用既有条件权威与 ShanmenItems authority，不新增库存真值。

## 2. 实现

- 新增 `MeridianShockTreatment` 产品语义；
- 新增唯一规范物品 `Prototype.Item.Consumable.MeridianStabilizingPill.Level1` / `一阶定脉丹`；
- Catalog 40 -> 41，内容身份更新为 `CodeB.Content.0.0.10.P16.0` / `9B789DB381BB934F772326A5217094F19C654D5AF51D5623EDB0A108DBA4CC7B`；
- P11.7 身份继续作为已知历史证据；
- 条件组件新增确定性 Treatment Intent、Receipt、Result、重放 journal 与严格修订门禁；
- 成功治疗移除速度 modifier、置 inactive、修订 `N -> N+1`，同一 TreatmentId 重放不二次写入；
- adapter 先持久化准备局内 Quantity，精确 Receipt 才可提交消耗；
- 普通丹药、错误 Run/物品/修订/Receipt、过期 snapshot、冲突 pending intent 全部失败关闭；
- 治疗前取消保持物品与条件；取消必须携带同一实时 active condition revision；
- 治疗后旧准备证据不能撤销物品消耗；
- 使用既有 `PreparePreparedRunQuantityIntent` / `FinalizePreparedRunQuantityIntent`，没有 legacy inventory 双写。

## 3. 测试演进

新增 5 个 Automation 用例：

```text
CanonicalCatalog
TreatmentAuthority
CommitAndReplay
PreTreatmentCancel
FailClosed
```

收尾审计追加“治疗后旧凭证不得取消”的反例断言。定向 treatment 组仍为 4 个测试节点；`TreatmentAuthority` 位于 CombatCondition 组。

首次 Profile 回归：`210/1`。失败是 `CatalogExactImmutableOrderAndPrices` 的精确价格 fixture 未加入新增买价 45；只更新 fixture，最终 Profile `211/0`。首次失败日志保留，SHA `716EE4E119E323412DFC5C2DE1DA989E2AA9C44AE973281D3870FD58E0173A8C`。

## 4. 最终验证

| Group | Result | SHA-256 |
|---|---|---|
| MeridianShockTreatment | 4/0 | `E92E101B7D53209BF99B37543D2C481B7F0B12FD05E96E9FD59EE72942B2871F` |
| CombatCondition.MeridianShock | 19/0 | `5D0A75B552CAA62F2C10B29EF0130841EE48B001FBC2C2C37C5F776573848494` |
| ShanmenItems | 72/0 | `BD4840B2236A2E8B4A1A4D4EF0F81548F01A1949C4A014DFF50F6515384E9A9E` |
| CodeB | 60/0 | `57DB2F4B7ED6778E8F90BA04A4354ABBD02A5A279F52225C1DEB76391F808E1D` |
| ItemEconomySchema | 24/0 | `E567475743C524CA79B17051E70405FCE2AA75A472DBA420A7B6146998A4AD35` |
| ItemUseAndArmor | 46/0 | `C2901CF39B5B17920E28A2A830E1DDF3AB405A2FCBDA876B228E558C15DA3A61` |
| P4.Hotbar | 7/0 | `7EDD75603229C79BCC49CD11EFABA95E55BF45AD3C9992E72120FB85FA39BEAB` |
| Profile final | 211/0 | `4C59A21C75C795C1AF0B751BBE7CC0EC280FE48C4A7A0A6BA8CFB526EC9AC82F` |
| V2RangedCompatibility | 22/0 | `CA83615035A4E62073617769409D11996A4DA97B85002C2A16584A9D12ED4658` |
| V3.Attributes | 4/0 | `179ECE5A8E5DCEF173140243B22730C846A9E147C76396C9A18BEF2CEA607453` |
| V3.WorldInteraction | 4/0 | `815A6FBBD42F126553D9596650A30A8522C482380D113932006FA61B9116FCC4` |
| Shanmen.0_0_10 full | 717/0 | `83AB21E901C7647BAE9F6A9CEDDE4D611F3A35867491C2EA5BA3D4441486D933` |

全量用时约 `29m17.07s`，queue-empty `717 tests performed` 精确出现一次，Fatal/Unhandled/Ensure 为 0。

流程结果：

```text
REGRESSION_COVERAGE: PASS Changed=19 Rules=8 Required=19 Logs=12
SELF_TEST: PASS 270/270
JSON_PARSE: PASS
GIT_DIFF_CHECK: PASS
BOUNDARY_SCAN: PASS
```

流程 SHA：gate `3362DC1FD424DB96FE59AFD3B131A665E56BC52B368AF1E14A5E8D22A65FB371`；self-test `44B3437B88198790A05644C7D71C2B201CA5F134D7B478918C0CEAFC6592FF84`；JSON `48C32879BFB681137257F2FA697C74FED7A350541D953E584A70FE8C7B067345`；diff `6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`；boundary `987868770230B1BD9D2D2CDE3B236144DCE4A338FAD398B3A815B24D6A93222A`。

## 5. 构建

- Editor cancellation guard：5 actions / 28.27s / status 0 / SHA `D465B14E9E8ACC03DBDC6AA4C9A79EED1A8DAF0C9AC548364CECFB98F7E9D1C6`；
- Game final：4 actions / 24.84s / status 0 / SHA `076E487973446581116CF91A2D0867F3528C4C2BCD71EF1E2D6EC2BCFFD70C82`；
- Editor final：0 actions / 0.92s / status 0 / SHA `05B4963BEB2C251505B92ECB2524542FF3496E82CFCC528A3FFE7ABC3F6F8210`；
- Game EXE：355,798,528 bytes / SHA `0C74A36D2804C2B90878D101AED2882363C1FFE540B33B731B6853AC18473FE3`；
- Editor DLL：14,399,488 bytes / SHA `E160C82DF24DA6D42E1BCE9E9F6D60BAFCBFC78D4B13C298314E63145EC7D9AA`。

## 6. 范围

本轮仅 P 阶段 C++ 契约、NullRHI Automation、流程检查和 Development build。没有 UI、输入产品路由、PIE、Standalone、产品 exe 运行、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户资料未修改或提交。

下一阶段由唯一产品 coordinator 安装 `prepare -> treat -> commit` 顺序和中断恢复，再让正式热栏/输入路由调用；UI 不得直接写条件或库存。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-0-meridian-shock-treatment-contract>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-0-meridian-shock-treatment-contract/Docs/Report/Dev.D.UE.0.0.10.P16.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-0-meridian-shock-treatment-contract/Docs/Log/Dev.D.UE.0.0.10.P16.0.r0_log.md>
