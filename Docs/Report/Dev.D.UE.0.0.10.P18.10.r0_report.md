# Dev.D.UE.0.0.10.P18.10.r0 Report

## 1. 结论

P18.10 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P18.9 的 Sword Qi 只读可用性投影接入一个设备无关、乐观并发式决策入口。调用方必须携带刚读取的 `ProjectionId` 与单一 `Issue / Retry / Cancel` 决策；router 在任何输入或产品回调前重新投影，先拒绝 stale identity，再拒绝与当前枚举状态不兼容的操作，最后才委托 P18.8 的唯一 command-event owner。

```text
Sword Qi availability router exact:   4 Success / 0 Fail
Shanmen.0_0_10 full:                 788 Success / 0 Fail
Required legacy groups:              443 Success / 0 Fail
Regression coverage:                  PASS (Changed=5 / Rules=2 / Required=56 / Logs=10)
Regression gate self-test:             PASS 289/289
Boundary scan:                         PASS (Files=2 / Matches=0)
Game + Editor Development:             PASS / native status 0
```

没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件或真实输入；没有执行截图、Smoke、Cook 或 Package。本结论只覆盖 C++ 契约、无头自动化、静态门禁与 Development 构建，不宣称最终键位、UI、表现或手感通过。

## 2. 不可变决策合同

新增 `Fdemo_mapShanmenSwordQiAvailabilityCommand`。它只有两个 private 字段：

- expected `ProjectionId`；
- `Issue / Retry / Cancel` 单一枚举。

默认对象、无效 GUID 和未知枚举均失败关闭。命令不携带 origin、aim、冻结 request、物品、属性、产品 payload 或可修改 owner 引用，因此不能形成第二份产品权威。

## 3. Admission 顺序

`Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute` 固定按以下顺序执行：

1. 验证不可变命令；
2. 从当前 owner 重新生成 P18.9 投影；
3. 比较 current 与 expected `ProjectionId`；
4. 从投影枚举派生该操作是否可用；
5. 委托既有 `TryIssue / TryRetryPending / TryCancelPending`；
6. 再次投影并验证状态转移。

stale 检查优先于 capability 检查。过期命令和错误状态命令均不会调用 fresh-input 或 frozen-input callback，也不会消费事件序号、空间样本或产品路由。

## 4. 三种路由语义

| Decision | 当前允许状态 | 唯一委托 | 外部采样 |
|---|---|---|---|
| `Issue` | `IssueReady` | `Owner.TryIssue` | 允许一次 fresh sample |
| `Retry` | `PendingRetry` | `Owner.TryRetryPending` | 禁止；只接收 owner-frozen sample |
| `Cancel` | `PendingRetry` | `Owner.TryCancelPending` | 禁止；两个 route callback 均不触发 |

外层 `Dispatched` 只表示 availability precondition 通过且 owner 被调用，不等同于产品接受。真实产品结果继续保存在嵌套 `CommandEvent.Input.Product`，因此 `GameplayBlocked`、`HostBusy` 或其他产品拒绝不会被包装成成功。

## 5. 状态转移证明

router 对 owner 调用后的投影执行结构后置检查：

- issue 在 pre-route 被拒时，投影身份与序号必须保持；
- issue 一旦提交 event，next sequence 必须精确增加 1；
- `HostBusy` 必须进入 `PendingRetry`；
- pending retry 再次 `HostBusy` 时必须保持同一投影身份；
- retry 得到非 Busy 的已路由结果后必须释放 pending slot；
- cancel 必须返回有效 cancellation proof，保持 next sequence，并产生新的 `IssueReady` 投影身份；
- 任一操作都不能改变 Run identity。

owner 拒绝与投影后置条件失败拥有独立状态；诊断文字来自真实失败层，不会被一次成功的普通投影说明覆盖。

## 6. GameMode 组合入口

新增 `Ademo_mapGameMode::RouteSwordQiAvailabilityCommand`。该入口不缓存 projection、不保存命令、不新增 ledger，只把：

- `Issue` 接回既有 `RouteSwordQiStartInput` 的 fresh sampler；
- `Retry` 接回同一路由，但 origin/aim 只来自 owner-frozen sample；
- `Cancel` 接回 owner cancellation。

现有 item、attribute、arbitration、Run Host、world delivery 与 damage authority均未复制或改写。P18.10 没有增加按键、输入映射、Widget、Tick 或自动重试。

## 7. 精确与全量自动化

新增 4 条 exact 用例：

1. `AdmissionFences`：默认/无效/未知命令失败关闭；inactive、错误 kind 与 stale identity 在 callback 前拒绝；
2. `IssueTransition`：pre-route 拒绝保持 identity；`HostBusy` 进入 pending；旧 issue 决策随后 stale；
3. `RetryTransition`：pending 禁止 issue；重复显式 Busy retry 保持 identity；非 Busy 路由释放 pending；旧 retry 随后 stale；
4. `CancelTransition`：取消不调用任何输入 callback，返回冻结 request proof，旧 cancel 不能重放。

exact 4/0；完整 `Shanmen.0_0_10` 从 P18.9 的 784 增至 788，实测 788/0。完整套件首末 Success 为 `2026-09-03 07:05:43.204 -> 07:39:26.493 UTC`，约 33m43.289s。

一次 preliminary full run 在 611/0 时主动停止：并行代码审查发现成功投影的普通说明可能遮蔽 owner/postcondition 的真实错误诊断。修正后重新编译、重跑 exact，并从零完成最终 788/0；未把旧二进制的部分运行计入最终证据。

## 8. 改动驱动回归证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| availability router exact | 4 | 0 | `D008E21752F0C7EE48DE7A59718C56728D98E223479DB38A0CD36A4C5B168688` |
| `Shanmen.0_0_10` full | 788 | 0 | `520A15D5B571BA4818FA4AB501C245A0579A4A074646FDD4D522A0972A3A36D0` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `56EDEEFAC0E8B200FEE82750A0BD53D220EA5B4415007684B822899A5B60EBD1` |
| `demo_map.Profile` | 211 | 0 | `70292A68810977D109653D42E04984754FF259CDEA9593F1E11B0D5ECA0917BE` |
| `demo_map.CodeB` | 60 | 0 | `52ACF3729EDDAA222ED8983EB008DCB8735B653130EB21AB996A2C5AA343C97A` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `4FF65EE9C9ECF318B757CEFF16E96D2EE32A9B6A665E4BCCDE564961E424AA46` |
| `demo_map.P4.Hotbar` | 7 | 0 | `171294F15C72C1969204277DD071DA976D461FED66CF9F728F993AE8E287ED21` |
| `demo_map.V3` | 29 | 0 | `0259B0389BCBBEA8610A16D57A0E70275B80810DECB52B1EA1122AD96DC4B74A` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `88B4F0912B47F8D06E7EE14CAB1F6B51BF4705B1E8C5F49528C135BE5BE85E32` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `AA3FA37212A8575FE71252C2461F0A528722696124C782D237DE03D2ECC9366B` |

所有采用日志均为 0 Fail 且各含一个原生 `TEST COMPLETE / EXIT CODE 0`。映射增加 router 生产路径规则、GameMode consumer 要求和 owner dependent-consumer 要求；自检同步增加一条应通过和一条应失败场景。

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=56 Logs=10
SELF_TEST: PASS 289/289
BOUNDARY_SCAN: PASS Files=2 Matches=0
GIT_DIFF_CHECK: PASS
```

## 9. 构建、产物与边界

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development after final source fix | Succeeded / native 0 | 4 / 12.51s | final source compiled before tests |
| Game Development final | Succeeded / native 0 | 26 / 124.72s | `314DD230E38E2F4F803E835703A897FBC0B04C34B207A1A3A2570D11B9A27FA4` |
| Editor Development final | Succeeded / native 0 | 0 / 1.03s | `D42A916A52DC45982F118035505A6D1FB32B4185142E7AF9AF48D3ED73D80066` |

最终产物：

- `demo_map.exe`：356,475,904 bytes，SHA-256 `C0CAD6E83E2A0FD018301898ECF1FBD1C0C7241E7DD3669EB728D1E5F7DF4C5C`；
- `UnrealEditor-demo_map.dll`：15,168,000 bytes，SHA-256 `6C8C0C203FFBEC0974D4D63C19C86115E8F13029ED5AEFF1EAF0CBCF4B0571AA`。

router 生产 header/cpp 的边界扫描为 0 命中：无 GameplayStatics、damage、旧 projectile、World/Actor、RNG、库存事务、物品/属性读取、物理输入绑定、声音或 Niagara 权威。

## 10. 提交边界与后续

基线提交为 `a56d4990d42c61b2f9c59509114dbe1e46231faf`。本轮计划精确提交 5 个生产/测试源码、2 个覆盖规则文件、本 Report 与本 Development Log，共 9 个文件。长期未跟踪的 0.0.9B Prompt/Report、CSEMI、handoff、PDF 与用户文件保持未暂存；`Saved/Codex/P18.10` raw logs 仅本地保留。

P18 的 P 阶段 Sword Qi 输入前边界现在已有：单次采样 adapter、Run-scoped event owner、capacity-one explicit retry、只读 availability projection，以及 stale-safe decision router。不建议继续叠加无消费者的包装层。下一步应由现有 0.0.10 P 计划选择另一项未闭合的战斗能力；只有明确进入 F 阶段时，才把该入口绑定到真实按键/UI 并做手感与产品运行验证。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-10-sword-qi-availability-command-router>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-10-sword-qi-availability-command-router/Docs/Report/Dev.D.UE.0.0.10.P18.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-10-sword-qi-availability-command-router/Docs/Log/Dev.D.UE.0.0.10.P18.10.r0_log.md>
