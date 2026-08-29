# Dev.D.UE.0.0.10.P8.3.r0 Report

## 1. 结论

P8.3 已建立 Formation world-delivery seam，结论为 **PASS**。

新增 `Fdemo_mapShanmenFormationWorldAdapter`，只把 P8.2 已发布的 immutable anchor audit 投影成 world placement intent、transient Actor handle 与 deterministic receipt。World 层不拥有材料、deployment、action 或持久状态；未提交阵眼无法创建 Actor。

最终 focused `4/4`、0.0.10 full `223/223`、changed-file gate、39/39 映射自测、Editor Development 与 Game Development 均通过。没有启动 Editor UI、PIE、Standalone 或产品可执行文件。

## 2. 唯一 placement 输入与身份

`BuildPlacementIntent` 必须同时观察：

- 有效 P8.2 product session；
- exact `Fdemo_mapShanmenFormationAnchorAudit`；
- P8.0 中同一 anchor 的 committed progress；
- RunId、OwnerId、DeploymentId、AnchorInstanceId；
- AttemptId、FulfillmentId、deployment commit receipt；
- authority revision 与 content stamp。

上述证据生成 `PlacementId`。Actor class 不属于 PlacementId，因此调用方不能通过换类为同一阵眼制造第二个 placement identity；class 只进入 placement receipt，用于检测 class drift。

World location 直接使用 P8.0 frozen anchor geometry，不读取 Actor、UI、输入或内容侧临时状态。

## 3. 投放、重放与重建

首次 `TryPlaceCommittedAnchor` 使用 `AlwaysSpawn` 创建 transient Actor，并写入 deterministic placement/deployment tags。只有 class、location、tags 与 receipt 全部满足后置条件，Adapter 才保存弱引用记录；后置条件失败时立即移除候选 Actor。

同 Adapter 的 exact replay 返回同一 Actor 与 receipt，不重复 spawn。进程内 Adapter 丢失后，新 Adapter 会扫描 exact placement tag：

- 恰好一个 Actor 且 class/location/deployment tag 完全一致：`Adopted`；
- 没有 Actor：允许首次投放；
- 多个 Actor：`DuplicatePlacementActors`，不接管、不新增；
- class 或 location/tag 漂移：失败关闭。

若 Adapter 已记录 Actor、但 Actor 外部消失且 World 中没有可验证 tag，则返回 `ActorUnavailable`，不会静默复制。

## 4. 冲突与失败边界

以下路径均在 spawn 前失败：

- invalid 或 terminal Session；
- 未提交 anchor；
- 无效 intent、World 或 concrete Actor class；
- Adapter 跨 World / Deployment 复用；
- 同 PlacementId 改用另一 Actor class；
- 重复 placement tags；
- 已清理 identity 复活。

Adapter 不调用 legacy inventory、ShanmenItems authority、`ApplyDamage`、RNG、Tick 或 timer；也不修改 P8.0 deployment 与 P8.2 session。

## 5. Terminal teardown 与恢复

只有 P8.2 Session 进入 `Cancelled` 或 `Ended` 后，`TryTeardownTerminal` 才能移除该 DeploymentId 的 Actors。未终态调用返回 `TerminalRequired`，保持 Actor 不变。

清理既覆盖本 Adapter 的弱引用，也扫描 deployment tag，因此新 Adapter 可在进程重建后完成 teardown。部分清理会保留已绑定的 recovery state、累计已移除数量，并要求 exact replay；全部清理后发布 deterministic teardown receipt。重复调用返回同一 receipt，不再执行第二轮销毁。

## 6. 自动化证据

最终日志均为 one command、one queue-empty、Fail `0`、fatal/assert/ensure `0`，进程原生退出码为 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationWorldDelivery` / `FormationWorldDelivery.log` | 4 | 0 | `E4E405008782B0CAD79D26F2A0D10C4AC9AAA7E61825F0819F496413AF7D46A2` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 223 | 0 | `85B073889C4ACCB0E4C00EC75A21274D9358B60465CAD1E28DDAB6F78CF0037A` |

四项 focused 测试覆盖：

1. committed intent、首次 spawn 与 same-instance exact replay；
2. Adapter reconstruction adoption、class conflict、duplicate-tag conflict；
3. uncommitted/no-world/no-class/terminal placement fail-closed；
4. 非终态 teardown fence、两阵眼 Ended、跨 Adapter teardown 与 receipt replay。

完整 suite 从 P8.2 的 `219` 增至 `223`，此前测试全部继续通过。

### 首次失败证据

首次 focused 日志为 `1 Success / 3 Fail`，SHA-256 `675E06794ACFE4FED5AAB3787323EFF999B0AAA22E3A224845AAE8601F521DF8`。基础 `AActor` 没有 RootComponent，无法保持请求位置，生产后置条件正确回滚 Actor；fixture 改为带根组件的 `ACharacter`。

第二、第三轮分别为 `3 Success / 1 Fail`，SHA-256 `DA18C9DCC896E1996EFFDCDE02294D2E9DC7766ACB5C7B455B8793F038968F2D` 与 `33B94069AB8822827BF3B49F4BCB668B0A23CBC0F06AC6F2436586C2AEFA5D91`。剩余断言揭示 `IsPlacementSuccess()` 把 immutable operation success 与 weak Actor 当前存活混在一起；最终改为由 status/intent/receipt 判定历史成功，弱引用只表示当前可达性。三轮失败的进程退出码也为 `0`，因此本阶段始终以 `Test Completed Result` 与 queue-empty 为成功标准，没有把进程码伪报为测试成功。

## 7. 改动—回归与静态门禁

新增 `FormationWorldDelivery` 映射，要求 focused world delivery、P8.2 FormationSession、ShanmenWorldGameplay 与 P8.0 FormationDeployment。

- regression map JSON：PASS，`39` rules；
- mapping self-test：`39/39 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=2`；
- coverage SHA-256：`F9F06AB72C3636A79528AC8BE20FAFB2BEC58B2C432111D358A86A11BFE46888`；
- self-test SHA-256：`035490D8337FD35B95B0BB63A08304731F8370A3A4C8013EC7B49844D06EA76A`；
- boundary scan 未发现 item authority、legacy item subsystem、`ApplyDamage`、RNG、Tick 或 timer；no-match 退出码 `1` 为预期；
- tracked diff 与三个新增 source 的 no-index whitespace check：PASS；
- 最终 staged `git diff --check`：PASS。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| 构建 | Result | Native exit | 时间 | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | Succeeded | 0 | 15.77s | `B84D867FCD3716C2A6004366D4BDBA6DAECB972D0CE7A74D62644A03202DD8BC` |
| Editor final | Succeeded | 0 | 11.05s | `D70723DC0C30868894D896ADF2B77DD27FAF8C43B723418B2A1AF4F40E5D551C` |
| Game final | Succeeded | 0 | 14.55s | `12E3E3C9A2DE9600D7D2113DA9E1B3B6FEA4BB55A34208BD7A0C302E34E8F4A7` |

所有源码、UHT、compile 与 link 均一次通过；没有 commit-memory、环境或源码构建失败。自动化 fixture 与语义审查修正后，最终 Editor、focused、full、gate 和 Game 证据均基于最终源码。

- Editor module：`10939904` bytes，UTC `2026-08-29T16:36:11.3039496Z`；
- Game executable：`352174080` bytes，UTC `2026-08-29T16:37:37.3383985Z`。

## 9. 修改范围、兼容性与 P/F 边界

新增：

- `demo_mapShanmenFormationWorldAdapter.h/.cpp`；
- `demo_mapShanmenFormationWorldAdapterTests.cpp`。

更新 regression map、自测、本 Report 与同名 Development Log。未修改 Build.cs、GameplayTags、Content、schema、ShanmenItems、P8.0/P8.1/P8.2、GameMode、输入、UI 或旧产品链。长期未跟踪的用户与 0.0.9B 工件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一阶段与 GitHub

P8.4 建议建立 Formation product host/controller：把 P8.2 material/deployment commit 与 P8.3 placement 的调用顺序收束为 forward-only command，显式处理“材料已提交、spawn 待重试”和 terminal teardown；仍不引入 UI、正式阵图内容或阵法效果。完成该恢复闭环后，再进入区域效果 provider 与正式交互。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-3-formation-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P8.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-3-formation-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P8.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-3-formation-world-delivery>
