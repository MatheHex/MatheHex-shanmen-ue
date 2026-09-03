# Dev.D.UE.0.0.10.P18.7.r0 Report

## 1. 结论

P18.7 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P18.6 的 Run-scoped Sword Qi 事件身份与首次有效 origin/aim 样本冻结成同一个不可变逻辑命令请求。显式重放只接受 owner 生成的冻结请求，公开 API 不再接收新的空间采样函数，因此产品拒绝后不能把同一事件重采样成另一轨迹。

```text
Sword Qi Command Event Owner exact:    5 Success / 0 Fail
Shanmen.0_0_10 full:                 782 Success / 0 Fail
Required legacy groups:              443 Success / 0 Fail
Regression coverage:                  PASS (Changed=5 / Rules=2 / Required=55 / Logs=10)
Regression gate self-test:             PASS 287/287
Boundary scan:                         PASS (Files=2 / Matches=0)
Game + Editor Development:             PASS / native status 0
```

没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件或真实输入，也没有执行截图、Smoke、Cook 或 Package。结论只覆盖无头代码、自动化、构建与静态边界，不宣称键位、表现、手感或人工验收通过。

## 2. 不可变冻结请求

新增 `Fdemo_mapShanmenSwordQiCommandRequest`，私有持有：

- P18.6 的不可变 `Fdemo_mapShanmenSwordQiCommandEvent`；
- P18.5 的规范化 `Fdemo_mapShanmenSwordQiInputSample`。

请求只提供 const getter，只有 `Fdemo_mapShanmenSwordQiCommandEventOwner` 可以构造。`IsValid` 同时要求事件身份可由 `RunId + EventSequence` 重派生，且 origin/aim 为有限、非零、已规范化样本。结果的 `CanReplay` 现在要求已提交且请求有效，不再仅凭事件 ID 成立。

## 3. 提交边界修正

P18.6 在产品 route 被调用时才提交事件；P18.7 把边界前移到“首次有效空间样本已捕获”：

```text
pre-sample gate rejected / invalid sample -> 不提交，不推进 sequence
valid sample captured                  -> 冻结 request，提交并推进 sequence
product accepted or rejected           -> request 不再改变
```

因此 gameplay blocked、route unavailable 与无效空间样本仍可在环境恢复后复用同一候选序号；但只要形成过有效逻辑命令，即使 intent capture 或产品 route 随后拒绝，也必须通过该冻结请求显式重放，不能自动复用身份或换轨迹。

若回调宣称调用过产品但没有提供一致的事件、IntentId 或有效样本，owner 会保守消耗该 identity 并返回 `OwnerPostconditionFailed`，避免潜在副作用后重用身份。

## 4. 无重采样重放

`TryReplay` 从接收 `Event + RouteInput(EventId)` 改为接收：

```text
FrozenRequest + RouteFrozenInput(EventId, FrozenSample)
```

它在调用前验证 request、active Run 与 committed sequence；跨 Run、未来序号或无效请求均失败关闭。回调返回后再次核对事件身份、IntentId 与实际使用的 Sample 是否匹配冻结请求。重放不推进 sequence，也没有内部自动重试循环。

`Ademo_mapGameMode::ReplaySwordQiStartCommand` 相应删除 `SampleOrigin`/`SampleAimDirection` 参数，只把冻结样本交给既有 `RouteSwordQiStartInput`。P18.5 adapter、P18.4 controller 与唯一 `RouteSwordQiIntent` 产品入口保持不变。

## 5. 聚焦测试

在 P18.6 四条测试基础上新增 `FrozenBeforeProductRoute`，并收紧既有 replay 断言：

1. `FrozenBeforeProductRoute` 模拟有效样本已取得、产品尚未调用即拒绝，验证 request 已提交、可重放且 sequence 只推进一次；
2. 重放 callback 只能收到冻结样本，原始 source sampler 计数保持不变；
3. 同一请求跨 Run owner 在 callback 前被拒绝；
4. `AppliedReplay` 验证产品 authority 漂移后复用冻结事件、样本、item、AttackPower 与 CommandId；
5. `BusyRetry` 验证 HostBusy 后显式重放无需再调用外部采样器，并继续复用 P18.4 已冻结 command。

Fixture 使用 unattended GamePreview world、NullRHI 和真实 ItemAuthority、AttributeComponent、CombatRunCoordinator、P18.4 Product Controller 与 P18.5 Input Adapter。

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi Command Event Owner exact | 5 | 0 | `DDCFBCD916615A8B014259A2938D9363D17D26D944F1A0376D8E1146535EAA9C` |
| `Shanmen.0_0_10` full | 782 | 0 | `7F2DE6B2C26CABCD9640346E718818C18EE6B30AC218834466A80A48F7075307` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `D32B3443945AAE94A1CE385E76377131691AA2B8D32337514BA5F6C1858A0079` |
| `demo_map.Profile` | 211 | 0 | `8EEA8AB32DA3941F1609B6A03BB3C9B993786D3AA6BF4C0BEB0C15670AE8B6C6` |
| `demo_map.CodeB` | 60 | 0 | `CBAFE0CBFCA6F393C3832C40A0FE4E7534DD1E588E86FDD1F796F2316C51ABE9` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `C83E469D71B226512DF99DE2321A26FF28CFFE283C87C5A500F471B8CA65D923` |
| `demo_map.P4.Hotbar` | 7 | 0 | `575936027EC487033C6EA013C2F06D94C2F9887C8C4D3708BF68B1B2936145C5` |
| `demo_map.V3` | 29 | 0 | `14E9F625F12933930B21BC18F515A81147632548D7ECE9F50FF0BEF430B6D156` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `574A1DF202956048BB7812E35CEFA0EE4A13AA499D69A0C4030CB4BF58D85828` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `9AD6FD5C21A06EC74F691D48134037680AA424EEA8D3B5346F2E85243FE1658A` |

每份采用日志均为 Fail=0、Fatal=0，并含一个原生 `TEST COMPLETE / EXIT CODE 0`。完整套件首末 Success 为 `2026.09.03 03:32:05.289 -> 04:06:45.269 UTC`，约 34m39.980s；相对 P18.6 的 781 条增加 1。

## 7. 改动驱动回归与静态门禁

本轮复用 P18.6 已落地的 GameMode 与 `SwordQiCommandEventOwner` 路径映射；实际 5 个改动文件触发 2 条规则、55 个必跑组：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=55 Logs=10
SELF_TEST: PASS 287/287
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage SHA：`8E1BBE591019DC3A1F58403C0BBACF938BF2EBBA12C903E006DA3204584F36CF`；
- self-test SHA：`41D356DF8529E4075B30222EF4AD4AE041E1511EF5E4778AFE89D5CB2BEE26B2`；
- boundary SHA：`0D37EA04FB41FF5D5784CA3F2E447A2B54605F170FD31EE541661C2681B4C572`；
- diff-check SHA：`547436D81BD9D3FCECD61E5223E114ABA690765645EF62894DBD389D8B013FE0`。

边界扫描确认生产 owner 仍不依赖 GameplayStatics/ApplyDamage/TakeDamage、旧 SkillProjectile、UWorld/AActor、RNG、库存写事务、物品/属性读取、物理输入绑定、声音或 Niagara。

## 8. 构建与产物

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded / native 0 | 27 / 152.26s | `EC815F8D09403C9DF901590C01FF5EA15FD7839725DB093F02636B618E4CE797` |
| Game Development final | Succeeded / native 0 | 26 / 124.04s | `3CAAAB10BD940250B7AFE2E261138FC7E63DE8C9EBF802D398AA6E7B2F374174` |
| Editor Development final | Succeeded / native 0 / up to date | 0 / 1.43s | `EAE37894A702F5D760BEF7F8326D447B2F7D5A6A19E64701D8AFAE56E3CD0F93` |

最终产物：

- `demo_map.exe`：356,433,408 bytes，SHA-256 `3806EF7AD100EC5905D6244B41F3C5D8E57588FECA012288C863EE453CF13CF5`；
- `UnrealEditor-demo_map.dll`：15,116,800 bytes，SHA-256 `6C383E1218510E5E40FC7BD508344584FACEF964C60DD171E3CEBB6A157903BD`。

## 9. 提交与 P/F 边界

基线提交为 `2b1ba2d2011ae75c0d5643a0e870cf5464999b2f`。本轮提交边界为 5 个修改源码、本 Report 与本 Development Log，共 7 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；`Saved/Codex/P18.7` raw logs 不进入 Git。

未修改 Content、地图、资产、配置、Windows、UE Engine 或存档 schema，也未改变装备、库存、属性、伤害或生命权威。真实 Enhanced Input、键位、动画、声音、特效、手感和产品运行继续属于明确授权后的 F 阶段。

## 10. 下一阶段与 GitHub

P18.8 建议增加一个 Run-scoped、容量为 1 的 pending retry slot：仅在冻结请求得到明确可重试产品拒绝时保存该请求，由上层显式 retry 或 cancel；不自动循环、不重新采样，也不复制 P18.4 的产品 payload authority。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-7-sword-qi-frozen-command-request>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-7-sword-qi-frozen-command-request/Docs/Report/Dev.D.UE.0.0.10.P18.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-7-sword-qi-frozen-command-request/Docs/Log/Dev.D.UE.0.0.10.P18.7.r0_log.md>
