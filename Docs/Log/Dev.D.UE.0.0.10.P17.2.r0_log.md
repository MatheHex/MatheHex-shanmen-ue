# Dev.D.UE.0.0.10.P17.2.r0 Development Log

## 1. 目标与基线

- 基线提交：`eff9c24c14bd347430941e25d4b31fdd8bf5c71b`（P17.1 Heart Mirror product lifecycle）；
- 分支：`agent/0.0.10-p17-2-heart-mirror-settlement-lifecycle`；
- 目标：验证已耗尽护心镜经过撤离、进程重启、后续 Run 再部署、死亡和再次重启后的唯一实例状态；
- 约束：只做 P 阶段；复用 generic settlement，不新增充能、修理、补给或饰品专用结算规则。

## 2. 起始审计

既有 `SpiritGuardDurableLifecycle` 已冻结通用结算语义：撤离保留当前资源并把实例返回 `Stored`；死亡把已部署实例转换为 `Destroyed` 零值墓碑；墓碑跨进程保留但不进入准备栏投影。`PrepareImpactDefense` 对已识别但资源为空的实例返回 `ResourceUnavailable`，不生成 reservation 或防御层。

P17.1 已证明真实 M01 产品攻击会把护心镜 charge 从 1 扣至 0，但终止于实例仍处于 active Run 的 `Deployed / 0` 状态。P17.2 因此只补生命周期闭环，不修改结算生产逻辑。

## 3. 新增 Automation

在 `demo_mapShanmenDefenseResourceAdapterTests.cpp` 新增：

- `Shanmen.0_0_10.Items.DefenseResourceAdapter.HeartMirrorSettlementLifecycle`

测试顺序：

1. fresh Profile 放入正式 Heart Mirror，经 cutover 和准备栏启动 Run 1；
2. 3 vitality 承受 raw 5 的致死 Impact，镜层保留 1 vitality，charge 1 -> 0；
3. `Extraction` 经 runtime settlement 与 Shanmen finalize，把同一实例变为 `Stored / 0`；
4. 断言准备投影保留 `Charges=0 / MaxCharges=1`，Presenter 输出 `CHG 0/1`；
5. 重启 authority，断言 exact ItemInstanceId 仍为 `Stored / 0`；
6. 重选同一实例，启动不同 RunId 的 Run 2，断言 `Deployed / 0`；
7. 对 Run 2 进行防御准备，断言 `ResourceUnavailable`、零 reservation、零 layer；
8. `Death` 结算，断言实例成为 `Destroyed`，Quantity/Durability/Charges 全为 0，准备投影消失；
9. 再次重启，断言零值墓碑与不可准备终态保持。

测试没有直接写资源、伪造新实例、回填默认 charge 或绕过现有 settlement API。

## 4. 编译与首次执行

新增测试后的首次 Editor 编译通过：4 actions / 30.11s。

最初一次命令使用 `-log=<absolute path>`，进程只完成 UE 平台校验，没有生成目标日志、发现测试或产生 Automation 终止标记。该次启动不计为测试成功或失败。将参数改为 `-abslog=<absolute path>` 并保留 `-stdout -FullStdOutLogOutput` 后，exact Automation 首次真实执行即为 1/0。

源码无需修正，生产逻辑零修改。

## 5. 生命周期结果

```text
Run 1 start:       exact mirror Deployed / charge 1
lethal impact:     vitality 3 -> 1 / mirror charge 1 -> 0
extraction:        exact mirror Stored / charge 0 / view CHG 0/1
restart 1:         exact mirror Stored / charge 0
Run 2 start:       distinct RunId / same mirror Deployed / charge 0
defense prepare:   ResourceUnavailable / no reservations / no layers
death:             exact mirror Destroyed / quantity 0 / durability 0 / charge 0
restart 2:         terminal tombstone preserved / absent from preparation
```

该结果证明资源当前值属于持久 ItemInstance，而不是每次 Run 从 Definition 默认值重建。

## 6. 最终测试证据

| Log | Group | Result | SHA-256 |
|---|---|---:|---|
| `P17.2_HeartMirrorSettlementLifecycle.log` | exact lifecycle | 1/0 | `8967F2F86D801A46D5867E2DB14A805316C8CA69D246BABE8AEC1FD34B88C040` |
| `P17.2_Items.log` | `Shanmen.0_0_10.Items` | 77/0 | `BFB936C23A3F32744C8D52847FC1A0B78CBADCA60AD1B481CE2396A588554735` |
| `P17.2_CombatCore.log` | `Shanmen.0_0_10.CombatCore` | 9/0 | `05D40B22D0D8BDADAAEECFC0BF214A9E7454819ECE50E40E8A4DBE9333E8AEAE` |
| `P17.2_Full.log` | `Shanmen.0_0_10` | 751/0 | `6311EF61B0D44F4F94160162B72817E243A0190DAEEFF4C20E4F92281D00F9DF` |

全部日志均有 native exit 0 的 Automation 终止标记；Fail、Fatal 与 Ensure 均为 0。完整套件首末 Success 时间为 `2026.09.02 18:41:41.673 -> 19:09:14.493 UTC`，约 27m32.82s。

## 7. 回归门禁

唯一修改源码路径为 `Source/demo_map/demo_mapShanmenDefenseResourceAdapterTests.cpp`。映射规则要求 `Shanmen.0_0_10.Items` 与 `Shanmen.0_0_10.CombatCore`，两份专用日志均进入校验：

```text
REGRESSION_COVERAGE: PASS Changed=1 Rules=1 Required=2 Logs=2
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatCore Evidence=P17.2_CombatCore.log
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.Items Evidence=P17.2_Items.log
SELF_TEST: PASS 273/273
GIT_DIFF_CHECK: PASS
```

`Scripts/ShanmenRegressionMap.json` 未修改，SHA-256 `00386B84259FCE2EFD5C5A856DA95048DD555251420999EAAB337221A49852A0`。

## 8. 最终构建

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

- Game Development：3 actions / 23.92s / native 0 / log SHA `190357550AEFE087CB388E594F9877B08BB5B8ECFF84BAE78C90D247BF284C46`；
- Editor Development：up to date / 0 actions / 0.96s / native 0 / log SHA `C22C3C62EC63149208403724059403A7784706EA3C73E40C9E599606A3AC2779`；
- `demo_map.exe`：356,155,904 bytes / SHA `7EDB2C5BF8CBF0837123D2C862D2F2020CE983C90EE517C72FA024090E5531F4`；
- `UnrealEditor-demo_map.dll`：14,848,000 bytes / SHA `4648E539FDF4E0FBE6A422B55842C6B80D0391D917FBD9A033673C3E60D5DA58`。

## 9. 边界与提交范围

计划提交一个 Automation 测试源码文件与本 Report/Log，共 3 个文件。生产代码、内容、资源、地图和配置均未改。没有运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

长期未跟踪的 0.0.9B Prompt、Report、旧交接资料、PDF、handoff 与用户资料保持未暂存；`Saved/Codex/P17.2` raw logs 不入 Git。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p17-2-heart-mirror-settlement-lifecycle>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-2-heart-mirror-settlement-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P17.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p17-2-heart-mirror-settlement-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P17.2.r0_log.md>
