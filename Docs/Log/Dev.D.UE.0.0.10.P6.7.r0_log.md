# Dev.D.UE.0.0.10.P6.7.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.7.r0`；
- 基线提交：`fffef73cd487bf963f9f3c8d8e4b3108be378fc1`（P6.6）；
- 分支：`agent/0.0.10-p6-7-controlled-weapon-command-router`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

P6.6 已将多实例 Host 接入真实 GameMode Run 生命周期，但未来 presentation / input 若直接调用 Host，仍须自行维护每把 exact item 的 command sequence、批量顺序、幂等与失败回滚。这会把确定性控制状态泄漏到 PlayerController，并使同一次玩家意图可能发生部分提交。

P6.7 因此增加输入设备无关的 Run Command Router：调用方只提供稳定意图 identity、exact Run、exact target item 集合与 Launch / Redirect / Recall payload。Router 负责规范化、序号派生、稳定原子批处理和重放 ledger；不提前冻结键位、目标选择、Actor、飞剑数量或 presentation。

## 实现记录

### Frozen intent

`TryCapture` 拒绝无效 intent / Run ID、空目标、无效或重复 item ID，并将目标按 GUID digits 排序。Launch / Redirect 要求有限非零方向并在边界处归一化；Recall 只接受零方向。

捕获后的对象只暴露只读 getter。`Matches` 比较 exact identity、kind、排序后目标与归一化后的精确方向值，使重放语义与底层 CommandId 的 canonical bit 语义一致。

### Stable atomic routing

`TryRoute` 先校验 ready Coordinator、Intent、Host、Router 及 exact Run / source identity，再处理 replay。它预检全部目标均已绑定，然后复制 Host；每个目标的下一 sequence 从现有 Execution 读取，按稳定 item 顺序调用 P6.5 Host。

任何目标拒绝、收据失效或 staged Host 失效时，Host 副本与临时 entries 被丢弃。只有全部目标成功后，Router 在副本 ledger 中记录原 Applied 结果，随后 Host 与 Router 一次提交。

同一 IntentId 的精确 payload 返回原收据并标记 Replayed，不推进任何 item sequence；同 ID 不同 payload 返回 `IntentIdConflict`。Router ledger 只属于一个 Run，空 Router 不允许残留 RunId。

### GameMode ownership and teardown

GameMode 新增唯一 `ControlledWeaponRunCommandRouter` 成员与 `RouteControlledWeaponIntent` 入口。新 Run 激活同时要求 Host 与 Router 均为空。

Preparation deactivation / EndPlay 的既有 P6.6 release helper 在成功 teardown 后重置 Router；孤立 Router、生命周期失败与 emergency reset 分支也会记录并显式清理，防止旧 intent 跨 Run 命中。

### Regression routing

新增 `ControlledWeaponRunCommandRouter` 路径规则，强制 Router、RunHost、Controller、WorldDelivery、Session、Adapter、Coordinator、Items、WorldGameplay 与 CombatRuntime。GameMode 既有规则继续要求完整 `Shanmen.0_0_10`；最终并集为 11 个 required group。

Self-test 新增 Router full-suite pass 与 coordinator-only fail-closed 两条，结果从 `12/12` 增至 `14/14`。

## 自动化覆盖

- 逆序 exact target 输入的稳定规范顺序；
- 两 item 的 Launch / Redirect / Recall sequence 0 / 1 / 2；
- Recall command、Recovery、Completed 收据与 terminal 状态；
- 等价归一化方向 replay 不推进序号；
- 同 IntentId 冲突 payload 失败关闭；
- 第二目标拒绝后的第一目标 staged mutation 回滚；
- 未绑定目标在 mutation 前拒绝；
- 重复 target capture 拒绝；
- Intent / Host / Coordinator Run mismatch；
- Router 跨 Run fence 与显式 Reset；
- GameMode teardown 的 Router reset 接线；
- 完整 0.0.10 既有契约回归。

## 首次验证与复审

首次 Editor integration build：`24/24` actions，`Result: Succeeded`，native exit `0`，`112.88s`。

首次 focused 日志 `p67_command_router_first.log`：`4/4` Success、Fail `0`、queue empty、native exit `0`、SHA-256 `84C745B8A779EE2D986D1FA6F837BD9E84FF16DEE6798D8CA4F1DC9B7EA50F39`。

契约复审将 tolerant direction comparison 收紧为捕获后的 exact vector equality，避免两个 canonical payload 被当作同一 replay。修正后 Editor 增量构建 `4/4`、`Result: Succeeded`、native exit `0`、`5.71s`；focused 复跑 `4/4`、SHA-256 `CCD9D6D6BE14680AFC64809D35B733C577AF61E7C4ED14900BD77FEDDD99ABCC`。

首次 full 日志 `p67_full_first.log`：`153/153` Success、Fail `0`、queue empty、native exit `0`、SHA-256 `BBB30562804DC86BD0B4D8975D5622F32DCD9036CF5B7C7BC203E34D3EF66F47`。

没有源码、自动化或编译失败。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p67_command_router_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunCommandRouter` | 4 | 0 | 0 | `322AB9133571D3B269532DDAB4DF4F552F2EE23AF09E35CB6E184DE7BD074F76` |
| `p67_lifecycle_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunLifecycle` | 3 | 0 | 0 | `7FF21311D2B4E2F172EA476A16DCBC0D42E0A7660A7FA4835AA7D66CF02D3C58` |
| `p67_run_host_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponRunHost` | 3 | 0 | 0 | `DEAEB8CF1B70D57DB820386368023408E25201A46C221E57863A7862A04E507A` |
| `p67_controller_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponController` | 3 | 0 | 0 | `522A6C5FC15EF143343293BDA1160ACDEC7F70E26E820273A8A8FACD594E8F1B` |
| `p67_world_delivery_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery` | 3 | 0 | 0 | `A3003A0FF34E3AAEC9B3E4040AA17C8C0661A7EFDFF3421682AC5BB2036562EF` |
| `p67_session_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponSession` | 4 | 0 | 0 | `640A0492A70477134197B3B7F293D049D91CCD5922864C53F07BA5DD32C7F05E` |
| `p67_adapter_final.log` | `Shanmen.0_0_10.Product.ControlledWeaponAdapter` | 4 | 0 | 0 | `666BBA4B92430198EA76EC7E03C880EA0AAC4FC5E7528674D0228248CEBFDA52` |
| `p67_coordinator_final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `75F794E3BE1BE03EBB78DB3FF6587CB60B8132F71DD2B58AD7D9BBE5CE292BCF` |
| `p67_items_final.log` | `Shanmen.0_0_10.Items` | 69 | 0 | 0 | `33E13FBC827D4804C7618F9ECD8F5C7E90DAFFB4B16B306D24BE011809E369F1` |
| `p67_world_final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `3F2D0FBA3A2CFB68667C186DBE4A6E65B052F10F8FF948A1C385654C9CCC1924` |
| `p67_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `560F77BF4B24A08EAC9BFB2C4DB7843D111DC2BB228FD76E99339D4186E968F9` |
| `p67_full_final.log` | `Shanmen.0_0_10` | 153 | 0 | 0 | `736C5840867FABF45DC3F0B021552009B25EC7B4B0FFD47417E46B0A13BCCACA` |

十二条日志均存在一个实际 `Cmd: Automation RunTests`、一个 `tests performed` queue-empty、至少一个 Success、Fail `0`、原生退出 `0`，且 Fatal / unhandled / ensure 为 `0`。全量唯一计数 `153/153`。

## Changed-file gate

加入 Report / Log 前的生产与脚本路径检查：

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=11 Logs=12
```

Required groups 为 Router、RunHost、Controller、WorldDelivery、Session、Adapter、Coordinator、Items、WorldGameplay、CombatRuntime 与完整 `Shanmen.0_0_10`；Lifecycle 作为 GameMode teardown reset 的额外聚焦证据。12 份最终日志全部由 gate 读取并验证 SHA。

加入本 Report / Log 后的最终 staged gate：

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=11 Logs=12
```

## 构建

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 首次集成：`24/24` actions，`Result: Succeeded`，native exit `0`，`112.88s`；
- exact replay 修正后增量：`4/4` actions，`Result: Succeeded`，native exit `0`，`5.71s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `23/23` actions；
- `Result: Succeeded`；
- native exit `0`；
- `97.17s`；
- 仅生成 `Binaries/Win64/demo_map.exe`，未启动。

## 静态与兼容性

- regression JSON parse：PASS；
- regression coverage self-test：`14/14 PASS`；
- Router 生产文件中的 spawn / damage / inventory transaction / RNG / input binding：`0`；
- 最终源文件早于已测试 Editor DLL 与 Game executable；
- `git diff --check` 与最终 `git diff --cached --check`：native exit `0`；
- Profile schema、存档、item definition、GameplayTags 与 input mapping 未改；
- 长期未跟踪历史文件未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续建议

P6.8 可在策划冻结 presentation / input 语义后，让薄输入层创建稳定 IntentId、选择 exact item 集合并调用 GameMode Router；输入层仍不应持有 per-item sequence、Host 副本、库存、Run 或 impact authority。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-7-controlled-weapon-command-router/Docs/Report/Dev.D.UE.0.0.10.P6.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-7-controlled-weapon-command-router/Docs/Log/Dev.D.UE.0.0.10.P6.7.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-7-controlled-weapon-command-router>
