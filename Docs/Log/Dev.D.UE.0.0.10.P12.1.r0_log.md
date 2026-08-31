# Dev.D.UE.0.0.10.P12.1.r0 Development Log

## 1. 目标

把既有 WeaponGuard 专名 30 Hz 时钟抽成唯一 Run 产品时间线，并让 P12.0 SwordRhythm core 能消费真实、完整闭合的 BasicSword 产品执行事实；不预先选择产品时序数值，不接伤害增益。

## 2. 实现

- 新增 `Fdemo_mapShanmenCombatRunFixedTimeline` 与 immutable sample；TimelineId 绑定 Run，SampleId 绑定 timeline + tick；
- 删除 `Fdemo_mapShanmenWeaponGuardFixedTimeline`，GameMode 只推进通用时钟；
- WeaponGuard 通过显式 adapter 继续获得同一个 TimelineId/tick；
- BasicSword execution result 新增 frozen Action，`IsExecuted()` 校验 Error、Action 与 ActivationId 一致；
- 新增 `Fdemo_mapShanmenSwordRhythmProductHost`，只接受同 Run、canonical timeline、正常闭合的 BasicSword result；
- Host 先在 chain 副本上计算再提交，exact replay 幂等，所有拒绝保持原子；
- 不把测试 timing window 写入 GameMode，不把 chain count 接入 damage。

## 3. Automation

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.CombatRunFixedTimeline` | 5 | 0 | `1FFD783DDBBDEE1BAEC60150486613198FBFEA21265152596F8716ABF6C4D024` |
| `Shanmen.0_0_10.Product.SwordRhythmProductHost` | 2 | 0 | `6904CBB6DE9CDF1C385DD5F9D65DC5FED556BDCE1D56121E0FC4EFF1F24BD83D` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 17 | 0 | `581361F36F46BD0E96091493EB302C8E4CBE311652C256025A45E38FA4C26C6B` |
| `Shanmen.0_0_10.CombatRuntime.SwordRhythm` | 6 | 0 | `01C2D1C26740A690B6BB78B3DA294FBEC0904551AE736F4A0FD3E680CB24A590` |
| `Shanmen.0_0_10.CombatRuntime.BasicSword` | 4 | 0 | `87DB81D114040391C8C6E6076C43538C76A70EE4E79C7DC0A9B96C70CDE267C9` |
| `Shanmen.0_0_10.CombatRuntime.ActionLifecycle` | 1 | 0 | `2AB7C527F4BDFCCA518E99AEC6F68E9124D33843BEFA1FF00E262F3621B10D76` |
| `Shanmen.0_0_10` | 549 | 0 | `3750905BB1C5F66E0B754E3F000474FE5BFB85152C77A38F2198B00E211BC060` |
| `demo_map.V3.Attributes` | 4 | 0 | `A799FA13C539AA4E91D48DE5168F287EF028384F9127554E1C99AE3CF01B58EB` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `6B155383D5FD50CEB79A89AE4816E5852D9884CFB0992BA43F6C581C7BFB2382` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `4F8726E5721364DA822E86A376652A249499368B03970E7ABDCFA7F064DAE8AC` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `42EAF304FEA6572E0295BE4FC08714C594076AC55AC78754E1298BEBB0E0DD8E` |

原始日志合计 `700 Success / 0 Fail`；按 test identity 去重为 `665`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=16 Rules=4 Required=43 Logs=11
SELF_TEST: PASS 170/170
BOUNDARY_SCAN: PASS ProductionLines=529 ForbiddenHits=0
git diff --check: PASS (native exit 0)
```

Coverage SHA-256：`D9F70AFFB279BFAE34EFCD2F622AA1F5ED8532BC9A3BA348FD5C483F8E3EE66D`。Self-test SHA-256：`E05A99B08A35898B39CECB6D114B9330E3ECCEC70F0960A714EA96C89818491B`。

## 5. 构建

- 首次 Editor：传递 include 被移除后暴露 guard sample type 缺少显式 include，`C3646/C2059/C2238`；确认后中止，launcher exit `1`，日志 SHA `652AC687F298B906B9A8013417B030F6AF8A009EFBBBF60220D2594117045F68`；
- 修复：GameMode 显式包含 `demo_mapShanmenWeaponGuardInputAdapter.h`；
- 完整 Editor：56 actions / 153.60s / exit `0`；
- 完整 Game：71 actions / 215.98s / exit `0`；
- 最终 Editor：4 actions / 10.01s / exit `0` / SHA `DE53D723EC1DD4BAC2F9B2D80612E0932C24156979A1E285D9FC5426ED284FE0`；
- 最终 Game：3 actions / 11.23s / exit `0` / SHA `3138B16E09444D464BE9E160840794463910A747860516912BC4771B3BE2568D`。

## 6. 流程异常

- Windows PowerShell 5 无法解析 validator 的 PowerShell 7 pipeline continuation；切换至 `pwsh 7.6.4` 后 `170/170`；
- 一次跨进程 hashtable 参数封装失败；改为同一 pwsh 进程直接 splat 后 gate 通过；
- 两项均为证据命令调用错误，未计作产品测试失败。

## 7. 修改与兼容性

代码、测试与门禁共 16 个路径，`1,076` additions / `450` deletions。旧 guard clock 三个文件删除，但 WeaponGuard 行为通过通用 sample adapter 保持；timeline identity 是 Run-local 瞬态值，不涉及存档迁移。

长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料保持未跟踪、未暂存、未提交。

## 8. 边界

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。未修改伤害、Impact、Vitality、inventory、schema、资源事务、GAS 或动画。P12.2 再以正式内容配置安装 Host 到 GameMode，并继续保持 chain count 与伤害解耦。
