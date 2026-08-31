# Dev.D.UE.0.0.10.P12.8.r0 Development Log

## 1. 目标

把现有 WeaponGuard final defense receipt 与 SpiritEvasion active projection 接入 P12.7 的唯一 SwordRhythm ProductSession，并为每个 accepted BasicSword 生成独立 evaluator 可消费的 immutable input；路由层不定义数值或修改伤害。

## 2. 实现

- ProductHost/Component 新增 SpiritEvasion active projection 的只读出口；
- 成功 SpiritEvasion start 使用既有 fixed Run timeline 登记 contribution；
- 六类敌方攻击仅在 final execution 成功且 timing 为 Perfect 时登记 WeaponGuard contribution；
- BasicSword live path 返回 rhythm receipt、optional binding 与 evaluator input；
- evaluator input 以 receipt identity 派生 deterministic ID，并交叉验证 observation、Run、Owner、timeline；
- 无 binding 时仍有合法 evaluator input，exact replay 保持同一 InputId；
- Session 原子维护 latest evaluator input，teardown 同步清理；
- 不新增 ledger、clock、Component authority、damage path 或 numeric policy。

## 3. Automation

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `SwordRhythmProductSession` | 2 | 0 | `26B2C13E4257369997B01BAC4B033AE6625FE4BF6718CBFCA825E49065FF8DE0` |
| `SpiritEvasionProductHost` | 7 | 0 | `838ABC46CD875623EB9849863A36FD00A2FA4882B2F70C09F2DBFB25A540DE3A` |
| `SpiritEvasionComponent` | 7 | 0 | `ADE59EBF1CDD42DB6EC796B900B985CCD3B6B05F6307905D96D63F7ECEE4FEFD` |
| `SpiritEvasionProductRoute` | 7 | 0 | `78E139ABD6CA1AA5AD95A84E34BD438CF126841E84740280601BFFD52AE85E43` |
| `WeaponGuardImpactRoute` | 3 | 0 | `57703AD3534E4D4B40A4D9C8504E68273F6DD543C7A994CB48BF68B309D9F658` |
| `Shanmen.0_0_10` | 562 | 0 | `5977BAD42EEC2E6B4C9B5FEB829A60A1CC7C139E06231B5F63A5838AC3EE3BC4` |
| `V3.Attributes` | 4 | 0 | `BE3F81FACD3BF85B9F06EFFA4D1F98CD9A050A8A4DFE6310BEF8264E8A71AA33` |
| `EnemySkillFramework` | 44 | 0 | `2F60845174F231BEBA203D142B2046EDEC47BA58085E1FB8DED128EBBC337050` |
| `V2RangedCompatibility` | 22 | 0 | `71977924BDB8725FF33CC7FA2975A9076DB35BB6E8F09E56C9E11C8A41EE267F` |
| `ItemUseAndArmor` | 46 | 0 | `B07DFF9219372AB981F8ECCDDD450158C1BDD095334920724657B95873D43F55` |

正式日志原始合计 `704 Success / 0 Fail`；全量唯一用例 `562`。全部最终进程原生退出 `0`，选定测试阶段错误标记 `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=14 Rules=6 Required=48 Logs=10
SELF_TEST: PASS 182/182
WEAPON_GUARD_LIVE_CALLS: PASS 6/6
BOUNDARY_SCAN: PASS
EVALUATOR_POLICY_SCAN: PASS identity and evidence only
git diff --check: PASS (native exit 0)
```

## 5. 构建与修正

- Editor initial：50 attempted / 176.07s / exit `1`；测试对 `FShanmenActionTransitionReceipt` 调用了不存在的 `GetAction()`；
- 修正：从真实 Component-owned Host 的 ActionRuntime 读取 action；
- Editor corrected：4 actions / 5.66s / exit `0`；
- Editor final：0 actions / 0.92s / exit `0`；
- Game final：47 actions / 158.71s / exit `0`；
- Editor product DLL：13,069,312 bytes / SHA `504094F66F80A0966F07F6B3A9C83465A464C64C2621B5AF99CC82D594AAC62C`；
- Game EXE：354,692,096 bytes / SHA `1784B4886DE0ECCCB9BDF2DFDF62C70806E05A05D957A7520293145568431E94`；
- 首次失败属于测试源码编译错误；最终无源码或环境构建失败。

## 6. 修改、兼容性与边界

Report/Log 前 14 个代码/流程文件净变更 `+427 / -8`。旧 BasicSword Session overload 保留；SpiritEvasion 与 WeaponGuard 原有产品权威不变。未修改 Impact、Vitality、inventory、schema、GAS、输入、动画或 damage policy；长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-8-sword-rhythm-live-contribution-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-8-sword-rhythm-live-contribution-route/Docs/Report/Dev.D.UE.0.0.10.P12.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-8-sword-rhythm-live-contribution-route/Docs/Log/Dev.D.UE.0.0.10.P12.8.r0_log.md>
