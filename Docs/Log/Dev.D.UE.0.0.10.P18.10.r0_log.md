# Dev.D.UE.0.0.10.P18.10.r0 Development Log

## 1. 目标与基线

- 基线提交：`a56d4990d42c61b2f9c59509114dbe1e46231faf`（P18.9 availability projection）；
- 分支：`agent/0.0.10-p18-10-sword-qi-availability-command-router`；
- 目标：以 expected `ProjectionId` 保护设备无关 `Issue / Retry / Cancel`，在 callback 前拒绝 stale 或错误状态；
- 约束：不接真实键位/UI，不自动 retry，不复制 command owner、sample、item、attribute、Host、world 或 damage authority。

## 2. 实现记录

新增 `demo_mapShanmenSwordQiAvailabilityCommandRouter.h/.cpp`：

- immutable command 只保存 expected projection identity 与 command kind；
- stateless router 每次立即重读 owner projection；
- stale fence 先于 capability fence；
- Issue 只委托 `TryIssue`；
- Retry 只委托 `TryRetryPending` 并接收 frozen sample；
- Cancel 只委托 `TryCancelPending`；
- 调用后重新投影并验证 Run、sequence、pending 与 identity 转移；
- `Dispatched` 不冒充嵌套 product accepted。

## 3. GameMode 组合

`Ademo_mapGameMode::RouteSwordQiAvailabilityCommand` 把新 router 接到现有 owner 和 `RouteSwordQiStartInput`。Issue 才能触发 caller sampler；Retry 用 owner-frozen origin/aim 构造只读 sampler；Cancel 不进入任一输入 callback。

没有删除或复制既有 owner API，没有增加状态缓存、第二 ledger、Tick、输入映射或 Widget。

## 4. 自动化改动

新增 `demo_mapShanmenSwordQiAvailabilityCommandRouterTests.cpp`，包含 4 条测试：

- invalid/inactive/wrong-kind/stale admission fences；
- issue pre-route identity stability 与 HostBusy transition；
- repeated explicit retry identity stability 与 non-Busy release；
- callback-free cancel、cancellation proof 与 stale replay rejection。

全量测试从 784 增至 788。测试明确统计 fresh 与 frozen callback 次数，避免只检查状态却遗漏副作用。

## 5. 回归映射

`ShanmenRegressionMap.json` 新增 router 路径规则，并把 exact router group 加入 GameMode 与 command owner 的 dependent-consumer 覆盖。self-test 新增：

- broad full + legacy evidence 应通过；
- 只有 router exact evidence 必须因缺少 owner/product/legacy 组而失败。

最终门禁：

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=56 Logs=10
SELF_TEST: PASS 289/289
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

## 6. 测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `automation_exact.log` | router exact | 4/0 | `D008E21752F0C7EE48DE7A59718C56728D98E223479DB38A0CD36A4C5B168688` |
| `automation_shanmen_full.log` | `Shanmen.0_0_10` | 788/0 | `520A15D5B571BA4818FA4AB501C245A0579A4A074646FDD4D522A0972A3A36D0` |
| `automation_item_economy_schema.log` | legacy | 24/0 | `56EDEEFAC0E8B200FEE82750A0BD53D220EA5B4415007684B822899A5B60EBD1` |
| `automation_profile.log` | legacy | 211/0 | `70292A68810977D109653D42E04984754FF259CDEA9593F1E11B0D5ECA0917BE` |
| `automation_code_b.log` | legacy | 60/0 | `52ACF3729EDDAA222ED8983EB008DCB8735B653130EB21AB996A2C5AA343C97A` |
| `automation_item_use_and_armor.log` | legacy | 46/0 | `4FF65EE9C9ECF318B757CEFF16E96D2EE32A9B6A665E4BCCDE564961E424AA46` |
| `automation_p4_hotbar.log` | legacy | 7/0 | `171294F15C72C1969204277DD071DA976D461FED66CF9F728F993AE8E287ED21` |
| `automation_v3.log` | legacy parent | 29/0 | `0259B0389BCBBEA8610A16D57A0E70275B80810DECB52B1EA1122AD96DC4B74A` |
| `automation_enemy_skill_framework.log` | legacy | 44/0 | `88B4F0912B47F8D06E7EE14CAB1F6B51BF4705B1E8C5F49528C135BE5BE85E32` |
| `automation_v2_ranged_compatibility.log` | legacy | 22/0 | `AA3FA37212A8575FE71252C2461F0A528722696124C782D237DE03D2ECC9366B` |

全部日志内部 Fail=0，且各有原生 terminal success marker；legacy 合计 443/0。

## 7. 诊断修正记录

第一次 full 在 611/0 时由开发端主动终止。原因不是测试失败，而是并行审查发现 router 复用投影成功说明作为临时 diagnostic，可能在罕见 owner/postcondition 失败时留下错误层级的文字。

修正为 before projection、owner dispatch、after projection 各自独立承接 diagnostic；随后 Editor 增量编译、exact 4/0 和最终 full 788/0 全部重跑。第一次部分运行不计入最终证据。

## 8. 构建与产物

- Editor final-source incremental：4 actions，native 0，12.51s；
- Game final：26 actions，native 0，124.72s，log SHA `314DD230E38E2F4F803E835703A897FBC0B04C34B207A1A3A2570D11B9A27FA4`；
- Editor final：up to date，0 actions，native 0，1.03s，log SHA `D42A916A52DC45982F118035505A6D1FB32B4185142E7AF9AF48D3ED73D80066`；
- `demo_map.exe`：356,475,904 bytes，SHA `C0CAD6E83E2A0FD018301898ECF1FBD1C0C7241E7DD3669EB728D1E5F7DF4C5C`；
- `UnrealEditor-demo_map.dll`：15,168,000 bytes，SHA `6C8C0C203FFBEC0974D4D63C19C86115E8F13029ED5AEFF1EAF0CBCF4B0571AA`。

## 9. 改动与提交边界

计划精确提交：

- 新 router header/cpp/tests 3 个文件；
- GameMode header/cpp 2 个文件；
- regression map/self-test 2 个文件；
- Report/Development Log 2 个文件。

长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件不修改、不暂存。raw automation logs 只保留在 `Saved/Codex/P18.10`。

## 10. 下一步判断

P18 的 P 阶段输入前边界已闭合到 stale-safe decision router。继续添加没有消费者的 command wrapper 会增加流程与代码复杂度而不增加玩法价值，因此不建议自动创建 P18.11 包装层。

下一轮应切换到 0.0.10 规划中尚未闭合的实质 P 阶段战斗能力；真实键位、UI、运行手感和产品验收只在用户明确授权 F 阶段后执行。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-10-sword-qi-availability-command-router>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-10-sword-qi-availability-command-router/Docs/Report/Dev.D.UE.0.0.10.P18.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-10-sword-qi-availability-command-router/Docs/Log/Dev.D.UE.0.0.10.P18.10.r0_log.md>
