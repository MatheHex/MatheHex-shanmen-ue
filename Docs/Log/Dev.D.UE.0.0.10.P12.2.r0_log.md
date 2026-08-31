# Dev.D.UE.0.0.10.P12.2.r0 Development Log

## 1. 目标

建立第一版可版本化的太极剑 rhythm timing config，把 P12.1 Host 安装到真实 Combat Run 与 BasicSword 产品路径，同时继续保持节奏结果与 damage 解耦。

## 2. 实现

- 新增 `Fdemo_mapShanmenSwordRhythmProductConfig`：`0.0.10.P12.2`、digest r1、30 Hz、`[8,13)` ticks；
- ConfigId 绑定版本、digest、style/action、rule、窗口与 tick rate；
- 新增 Run-local `Fdemo_mapShanmenSwordRhythmProductSession`，持有 config、Host 与 latest immutable receipt；
- begin/end 按同一 RunId 精确管理，观察采用副本提交，exact replay 幂等；
- GameMode 在 Run begin 安装 Session，在真实 BasicSword `IsExecuted()` 后捕获同一 timeline sample；
- Run release 校验 Session/Coordinator identity 并记录 observation count；
- chain count/band 不写入伤害、攻速、资源或其它数值。

## 3. Automation

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 2 | 0 | `B7A64105B0932B0DC1919354EA4DCA785A9E1E5DE9074D676A498AE2C4DD141D` |
| `Shanmen.0_0_10` | 551 | 0 | `7766FF0AF91D9F195AD032DDF5FB24CC24582A20E02A622BC9006716910C6DCA` |
| `demo_map.V3.Attributes` | 4 | 0 | `7079E2EBCAB1679756A11C1898AB81AE23460C10F40EA5C864B429D5C82EB2A1` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `C4F935C6077190503B579C2B857B9174F670C5D500BF96A73F2368F9675665FE` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `B68021208E9E7D94B98FC5A2E567ECB689FA9A6065A22E1794DA6E2AB632562E` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `FCBE9F125509D4D25DF6D68C303B066AF9E649CF62EAB2B2E9B27B63B9BAC77B` |

原始日志 `669 Success / 0 Fail`；focused 2 已包含于 full 551，按 test identity 去重为 `667`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=43 Logs=6
SELF_TEST: PASS 172/172
BOUNDARY_SCAN: PASS ProductionLines=350 ForbiddenHits=0
git diff --check: PASS (native exit 0)
```

Coverage 日志 SHA-256：`5EBDE6668C1AF6861A54C4698CFB1A0DC560477A09384D631FBE7F2D602F1B60`。Self-test 日志 SHA-256：`FEA3C9D5EAD56572BB41CDE74F68BBE23D3A26203E612FE7E3B372FAFA69232C`。

## 5. 构建

- 首次 Editor：24 actions / 118.55s / exit `0`；
- 首次 Game：23 actions / 106.57s / exit `0`；
- 最终 Editor：up to date / 0 actions / 0.88s / exit `0` / log SHA `4704F0C28FF77589BD958E8D4B107242838D2D8F1742E15D0210B7730FFE997B`；
- 最终 Game：up to date / 0 actions / 0.91s / exit `0` / log SHA `CFF4BCB4985E7C4C8D34EA26426971912F4B137933DFCA034731E772B0B2CD21`；
- 两目标首次均成功，无源码或环境失败。

## 6. 修改与兼容性

不含文档共 7 个路径，`770 additions / 6 deletions`。没有修改 Impact、Vitality、inventory、schema、资源事务、GAS、动画或输入；新 Session 不依赖 World/Actor/timer/RNG/damage API。长期未跟踪的 0.0.9B 资料保持未暂存。

## 7. 边界

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。下一阶段只读投影 receipt 给表现／状态层，仍不把 chain count 接入 damage。
