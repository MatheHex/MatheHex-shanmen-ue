# Dev.D.UE.0.0.10.P8.34.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.34.r0`；
- 基线：`df5558edb6b9991aa6cab17ae49e375cf0d45d77`（P8.33）；
- 分支：`agent/0.0.10-p8-34-formation-influence-consumer-lease-order-gate`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

在现有 lifecycle CommandHost 内建立 authoritative lease 与 native consumer application 的 provenance/order gate，消除“authoritative lease 已删除但 native modifier 仍存活”的中间状态，同时保留显式 caller ownership、durable replay 与 lower-level composition API。

## 决策记录

### 不新增总控制器

当前 formation 的最高产品权威已经是 `Fdemo_mapShanmenFormationProductHost`，caller-facing lifecycle authority 已是 `Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost`。再包一层 run/controller 会复制 correlation、ledger 与 consumer runtime 所有权，因此 P8.34 直接增强既有 Host。

### lease 与 native application 使用同一 Host 值状态

consumer command 不能只凭结构有效就修改 native component。新命令必须与该 lifecycle Host 内部 executor 的 active lease 完全一致；构造相同 ProductHost identity 的平行 Host 没有原 Host 的 lease 状态，不能跨实例消费。

### Remove 顺序先 native、后 authoritative

P8.33 的 terminal fence 能阻止带 active modifier 的 End，但 caller 仍可先执行 authoritative Remove。P8.34 在新的 Remove lifecycle command 进入 Router 前查询 exact LeaseId application count；大于零即 fail-closed，且不生成 durable receipt。

### completed replay 不依赖当前 active lease

authoritative lease 删除后，之前已经完成的 consumer Remove 仍需支持 exact idempotent replay。Host 先读取自己内部 consumer command history；只有未完成的新命令才要求当前 active lease。

## 执行序列

1. 复审 ProductHost、lifecycle coordinator/router/CommandHost 与 consumer runtime 的实际调用层级。
2. 否决新增 formation controller，确认在既有 lifecycle CommandHost 收口。
3. 为 registry/consumer CommandHost 增加 per-lease active count。
4. 为 consumer CommandHost 增加完整 command 对应的 completed transaction 只读查询。
5. 扩展 product runtime result，记录 lease authority evidence 与三类精确拒绝状态。
6. 在 Activate/Deactivate 前校验内部 executor 的 exact active lease。
7. 在新的 authoritative Remove 前强制 native consumer 已清空；durable lifecycle replay保持优先。
8. 扩展真实 ProductHost 测试，覆盖 parallel Host、Remove fence、显式 deactivation、authoritative Remove、historical replay 与 terminal forward recovery。
9. 扩展 registry/consumer CommandHost 测试，覆盖 per-lease count 与 completed receipt 查询。
10. 运行专项、父级、legacy Attributes 与 `Shanmen.0_0_10` 全量 Automation。
11. consumer CommandHost 第一份候选日志缺正式 queue-empty，丢弃后仅重跑该组，最终 `4/4`。
12. 运行 regression map JSON、自检 `94/94`、changed-file gate、静态扫描、`git diff --check` 与 Editor/Game 最终构建。
13. 生成 Report/Log，执行 exact-stage、commit 与 push。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `Attributes-final.log` | 4 | 0 | 0 | `98273B216FDE196787BEF76F9BD4AF2CAECE9D37FFF89CC084DCD451009DF7E0` |
| `FormationInfluence-final.log` | 85 | 0 | 0 | `AC9E86E8F0AB34126EBC12051BD1786A345620C653D65609D83B0C4CC8FB20FF` |
| `FormationInfluenceConsumerCommandHost-final.log` | 4 | 0 | 0 | `42FC8CCB92C4965BC366E05A7EF13A2E2FB20A2D3C5B40071035728D5358CE21` |
| `FormationInfluenceConsumerProductRuntime-final.log` | 4 | 0 | 0 | `843D47148AECF9FB7D1D169F79BD23CED0BB1F699A50AA9CB1ECA85667C158D5` |
| `FormationInfluenceConsumerRegistry-final.log` | 4 | 0 | 0 | `B3C12043019D51F1B748295981C2156728971E0485391201D40A7FDFAE881973` |
| `FormationInfluenceLifecycleCommandHost-final.log` | 5 | 0 | 0 | `7D92C2EEF70356B451FC2AAB560DA4336C59C0C9A8C13A50C5533CC25896A905` |
| `FormationProductHost-final.log` | 6 | 0 | 0 | `F24E867C1974FF367295FCD852739AC9D9AF9C4076A23EE80BF9516B6750A978` |
| `Shanmen-full-final.log` | 334 | 0 | 0 | `A7264115B7C86C4C902FBD0278B8CF51CF17FE20A59019A2AB919523D107C9E1` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=67
SELF_TEST: PASS 94/94
REGRESSION_COVERAGE: PASS Changed=12 Rules=7 Required=36 Logs=8
git diff --check: PASS
```

- mapping SHA-256：`1FC61E3CA0586BC39F637980557D329F33D3B19AEECD0E1E43F93E67CC780B9A`；
- self-test SHA-256：`F11AE76FBDF73AFAD67469D68B0DF47748D4114ADC04F79577328A0A7C3C2A98`；
- 自动发现、后台循环、raw object-pointer member、GAS、RNG、SaveGame/ProfileRepository 扫描均为 `0`。

## 构建证据

- Editor final：`9 actions / 17.90s / exit 0`，log SHA-256 `F8DF5E2B7C099CC996AE061E70A37B71A48C9F9FAB089B7FBC843CA18FA93C42`；
- Game final：`15 actions / 48.81s / exit 0`，log SHA-256 `54BA95D3F64F9A5FA4FDAF49B92571314385F1A4E5DBB5301C288E135D433EA2`；
- `UnrealEditor-demo_map.dll`：`12079104` bytes，SHA-256 `3DC3494B3605CA643EF48DDE4F63C954B04A737008F6A6E164BAD7BA9B26D16D`；
- `demo_map.exe`：`353126400` bytes，SHA-256 `9DA70CCF05F2FBED974D6D4B09A7A5F5182E2F4224D6F61A55B0DCBEF7DAF5C3`。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
