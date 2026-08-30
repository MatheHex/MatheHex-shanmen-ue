# Dev.D.UE.0.0.10.P8.33.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.33.r0`；
- 基线：`6e720e6a807d0c485a1f0f1ea8e503e4957fa733`（P8.32）；
- 分支：`agent/0.0.10-p8-33-formation-influence-consumer-product-lifecycle-integration`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

把 P8.32 consumer product runtime 接入现有 lifecycle command host，使 native applications 未显式 Remove 时无法 seal/end，同时保留 caller ownership、旧 lower-level API、durable receipt 与 exact forward recovery。

## 决策记录

### 不接受 caller 提供任意 runtime 副本

P8.32 runtime identity 可确定性重建，但两个相同 identity 的值对象可以拥有不同 active-application 状态。如果终止 API 接受 caller 任意传入 runtime，调用方可以用一个空副本绕过真实 active modifier。因此唯一安全集成点是现有 caller-facing `LifecycleCommandHost`：它在 open 时内部创建并持有唯一 runtime，Apply/Remove/End 全部走同一值状态。

### readiness 先于 seal

consumer readiness 在 `TrySealInfluence` 之前执行。失败时 coordinator/router 均不提交状态，也不生成 durable End receipt；caller 可以显式 Remove 后用原 End command 重试。

### terminal 只读恢复

World teardown 失败发生在 Host 已 seal/session terminal 之后。runtime readiness 因此允许 exact terminal ProductHost 做只读 drained 检查，但 mutation API 继续通过原 `CheckProductHost` 拒绝 terminal。

### recovery mode sticky

第一次 End receipt 若记录了 consumer check，forward recovery 必须继续携带 runtime；无 consumer 的旧路径不能恢复该 receipt。完成后 replay 不重入。

## 执行序列

1. 复审 ProductHost、P8.20 coordinator、P8.21 router、P8.22 command host 与 P8.32 runtime 的 teardown/recovery 顺序。
2. 否决“caller 传 runtime 引用”的初始方案，确定 command host 内部唯一 runtime 所有权。
3. 扩展 lifecycle result/status，加入 nested consumer teardown evidence。
4. 为 coordinator/router 增加 guarded terminal 路径，并保持旧 lower-level API。
5. 在 command host 增加显式 Activate/Deactivate，并让所有 Submit 使用内部 runtime。
6. 调整 runtime readiness，使 terminal ProductHost 仅可进行只读 recovery check。
7. 增加真实 lease→projection→attribute modifier→terminal fence→Remove→World recovery 测试。
8. 扩展 Router recovery 测试，证明 guarded receipt 不能降级为无 consumer recovery。
9. 更新 regression map 的 lifecycle/consumer/attribute 反向依赖。
10. 第一次 self-test 因正例 fixture 缺少 attributes evidence 退出 `1`；修正 fixture 后 `94/94`。
11. 跑专项、父级与全量 Automation；全部通过。
12. 运行 changed-file gate、静态扫描、`git diff --check` 与 Editor/Game 最终构建。
13. 生成 Report/Log，准备 exact-stage、commit 与 push。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `P8.33-FormationInfluenceLifecycleCommandHost-final.log` | 5 | 0 | 0 | `0967ABBA5CA4694A8DEE59B9F359E1FDB3DE8EE88385C5A7CFA274B931B3DC3B` |
| `P8.33-FormationInfluenceLifecycleCommandRouter-final.log` | 4 | 0 | 0 | `6095E36C8BADFE3EA1DAD18BFC0498E2D58F01B889E3CE8867C471A03EE467C6` |
| `P8.33-FormationInfluenceConsumerProductRuntime-final.log` | 4 | 0 | 0 | `873EB5F3EA42ADF0A4BD67FF0F292ECD76E1F05DE531D73D3C8A489C5A86BA40` |
| `P8.33-FormationProductHost-final.log` | 6 | 0 | 0 | `FBDD6761D95C4DB89BCF2154F67B63570781AA42319FE5C2C88025117D63F3AE` |
| `P8.33-Attributes-final.log` | 4 | 0 | 0 | `F015E362C160E106F1977BE99E739B099CBCB2ACA0749A1371C3AD5956D7041E` |
| `P8.33-FormationInfluence-final.log` | 85 | 0 | 0 | `79DAA77FDF1DD2AA89014923E42912097DE5B17169668C852CFD56630F526D1D` |
| `P8.33-Shanmen-full-final.log` | 334 | 0 | 0 | `EE17BF3BDB1302A1031658D23B4768AC8B36B4E4F8B1107CE654643BA2C2C1A4` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=67
SELF_TEST: PASS 94/94
REGRESSION_COVERAGE: PASS Changed=11 Rules=5 Required=36 Logs=7
git diff --check: PASS
```

- mapping SHA-256：`1FC61E3CA0586BC39F637980557D329F33D3B19AEECD0E1E43F93E67CC780B9A`；
- self-test SHA-256：`F11AE76FBDF73AFAD67469D68B0DF47748D4114ADC04F79577328A0A7C3C2A98`；
- 自动发现、后台循环、raw object-pointer member、GAS、RNG、SaveGame/ProfileRepository 扫描均为 `0`。

## 构建证据

- Editor final：`9 actions / 18.72s / exit 0`，log SHA-256 `3C20F2C9326C7A7F9BD91F672C9194979B4A9884B0BD831640638D1E29BF4979`；
- Game final：`8 actions / 34.05s / exit 0`，log SHA-256 `048633D7BF98E2416A162BB3B40F3E26E913A41C78CEEBD53D5D91FA49A3C999`；
- `UnrealEditor-demo_map.dll`：`12069376` bytes，SHA-256 `92E3FDAE921AF99A836F04BC668EB3726E32FBAFE33E9D9DB587B0F0BC9A0D66`；
- `demo_map.exe`：`353118208` bytes，SHA-256 `408539DBEB9E76B8D0D9890C6336CD1734B3B44C497A76A500A775906A093B25`。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
