# Dev.D.UE.0.0.10.P8.36.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.36.r0`；
- 基线：`3baa7937ac0dc1320159cce90b141f4c9cc20eb9`（P8.35）；
- 分支：`agent/0.0.10-p8-36-formation-consumer-delivery-application`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

让现有 formation 产品 composition root 直接消费 P8.35 command delivery，并把 caller 已解析的 subject/component、Host durable receipt 与 native runtime receipt 组成自校验工件；不新增 controller，不自动发现组件，不保存 UObject pointer。

## 决策记录

### resolution 冻结 identity，不冻结 pointer

组件 pointer 只能作为一次 Activate 调用的瞬时 capability。resolution 保存 subject id、组件进程内 unique id 与确定性 resolution id；Host 在 mutation 前用 live pointer 重新匹配，result 中不复制 pointer。

### application 必须重新绑定本 Host receipt

一份结构有效 delivery 不能单独授权 mutation。Activate/Deactivate 都按 `LifecycleCommandId` 从本 Host 读取 durable receipt，再验证 successful Apply intent、subject 与 authoritative lease。parallel Host 和篡改 id 因此 fail-closed。

### preparation 与 mutation 保持分离

P8.35 的 `TryPrepareConsumerCommands(...) const` 仍是纯只读。P8.36 新增具名 Activate/Deactivate delivery 调用，但不把二者合并成隐式事务，也不调度、循环或重试。

### 保留低层 API

旧 command-level Activate/Deactivate 仍用于独立测试与组合；真实 ProductHost 用例迁移到 delivery-level 入口，避免调用方自行拆包后绕过 provenance/resolution 证据。

## 执行序列

1. 复审 P8.35 真实调用点，确认 delivery 仍被拆成裸 command 后调用低层 API。
2. 确认 LifecycleCommandHost 已是 Router + consumer runtime 的现有 composition root，否决新 controller。
3. 定义 pointer-free subject resolution、application status 与自校验 result。
4. 为 delivery 增加 exact value `Matches(...)`。
5. 抽出 delivery/source receipt 一致性校验，供 preparation result 与 application result 共用。
6. 实现显式 delivery Activate：验证 Host receipt、resolution、live component 后调用既有 runtime。
7. 实现显式 delivery Deactivate：验证 Host receipt 后调用既有 runtime，保留完成后 replay。
8. 迁移真实 ProductHost 测试，覆盖 foreign Host、invalid resolution、subject/component mismatch、tampered receipt、Activate/Deactivate replay。
9. 首次专项候选因命令带显式 `Quit` 缺 queue-empty；移除 `Quit` 后重跑，未发生测试失败。
10. 代码复审补充 Activate replay 数量不变断言，并重新生成全部最终 Automation 证据。
11. 运行 regression map JSON、自检 `94/94`、changed-file gate、静态扫描、`git diff --check` 与 Editor/Game 最终构建。
12. 生成 Report/Log，执行 exact-stage、staged gate、commit 与 push。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `FormationInfluenceLifecycleCommandHost-final.log` | 5 | 0 | 0 | `579EC5ED03F27A9C4C5F87A032F3A5952B350B5A3839690CF0EC650AE22E890A` |
| `FormationProductHost-final.log` | 6 | 0 | 0 | `A58B42C2A2BEA3BD22E9B43EECB1BE447F032C36A38C4AA463D82BF4080F137D` |
| `FormationInfluence-final.log` | 85 | 0 | 0 | `F45EBABDC26AB38C8242CDD481934BD1086FCC91EF7E7AEFBE9CF28349D96EDD` |
| `Attributes-final.log` | 4 | 0 | 0 | `F5ECA2983389E3D72FA7D26A904501EF8846584A8FECBEBD06A7B1E68C94D8C3` |
| `Shanmen-full-final.log` | 334 | 0 | 0 | `35E123E85860F141ACFF992BD4BF0A1A6D614D97D5CACC75D6AF5856EBD67325` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=67
SELF_TEST: PASS 94/94
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=35 Logs=5
git diff --check: PASS
git diff --cached --check: PASS
```

- mapping SHA-256：`1FC61E3CA0586BC39F637980557D329F33D3B19AEECD0E1E43F93E67CC780B9A`；
- self-test SHA-256：`F11AE76FBDF73AFAD67469D68B0DF47748D4114ADC04F79577328A0A7C3C2A98`；
- 自动发现、后台循环、raw object-pointer member、Actor/World、GAS、RNG、SaveGame/ProfileRepository 扫描均为 `0`。

## 构建证据

- Editor final：`up to date / 0 actions / 0.92s / exit 0`，log SHA-256 `B5794E536DB7F9029345FAFC6D13782FFC8FFC5198BF192225F94C559CBB7D07`；
- Game final：`4 actions / 25.00s / exit 0`，log SHA-256 `67DF44FA919E9E4D40BFEE1E7107598CC4CA3E65D522AF077AF20576B1A617CF`；
- `UnrealEditor-demo_map.dll`：`12106240` bytes，SHA-256 `2CF8A407D4C1DED3BAFCD9B3C530FD25E06CB44D8D55FCE0DA7C81022A382586`；
- `demo_map.exe`：`353145856` bytes，SHA-256 `87ECE6EF9AD59232D8E662925AB477A67EA527851D0D81EF071627F654C6F5E5`。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
