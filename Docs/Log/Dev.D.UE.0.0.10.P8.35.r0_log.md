# Dev.D.UE.0.0.10.P8.35.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.35.r0`；
- 基线：`5dd642a87126678aaebac18ec2ff04fb24790a1b`（P8.34）；
- 分支：`agent/0.0.10-p8-35-formation-influence-consumer-command-delivery`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

消除真实 ProductHost 测试与未来 caller 为生成 native consumer command 而穿透 lifecycle Router/Coordinator/Executor 的做法，在既有 LifecycleCommandHost 上提供 receipt-bound、只读、可审计的 command delivery；组件解析与实际 mutation 继续保持显式。

## 决策记录

### 使用 durable lifecycle receipt，而不是裸 lease key

caller 已持有完成的 lifecycle `CommandId`。以它作为交付入口，可以证明 command 来自同一个 Host、确实成功执行 Apply，并把 invocation intent、subject、expected intent 与 authoritative lease 串成一条可重放证据链。

### delivery 是值工件，不是新事务

新 API 为 `const`，只返回 source receipt、active lease、projection 与 deterministic Apply/Remove command。它不会发现组件、注册 binding、应用 modifier 或提交 consumer transaction，因此不复制 consumer runtime，也不会把原子性含义扩大到 native mutation。

### result 必须自校验

成功状态不能只信任工厂曾走过某条路径。delivery/result 的 `IsValid()` / `IsSuccess()` 会重新验证 receipt operation、intent、subject、lease 与 command pair 的完整一致性，使值被复制或跨层传递后仍可 fail-closed。

### expired lease 不重新签发

durable lifecycle receipt 可回放，但它不代表 lease 永远 active。authoritative Remove 后，旧 Apply receipt 仍可审计，却不能重新签发新的 native command delivery。

## 执行序列

1. 复审 P8.34 的真实调用缺口，确认测试仍通过深层 getter 手工查询 lease、投影并构建命令。
2. 否决新增 controller 与隐式 consumer executor，选择扩展既有 LifecycleCommandHost。
3. 定义 delivery status、自校验 delivery value 与带完整 source evidence 的 result。
4. 实现按 durable lifecycle `CommandId` 的只读 `TryPrepareConsumerCommands(...) const`。
5. 校验 source receipt 为成功 Apply、intent/subject 一致、active lease 与 source evidence 完全匹配。
6. 使用既有 projector/command factory 生成 frozen Apply/Remove pair。
7. 将 ProductHost 测试 helper 从深层 getter 切换到新接口。
8. 增加 replay stability、无副作用、非法/未知 receipt、foreign definition、非 Apply receipt 与 expired lease 覆盖。
9. 补强 result 自校验后重新生成全部最终 Automation 证据。
10. 运行专项、父级、legacy Attributes 与 `Shanmen.0_0_10` 全量 Automation。
11. 运行 regression map JSON、自检 `94/94`、changed-file gate、静态扫描、`git diff --check` 与 Editor/Game 最终构建。
12. 生成 Report/Log，执行 exact-stage、staged gate、commit 与 push。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `FormationInfluenceLifecycleCommandHost-final.log` | 5 | 0 | 0 | `8E38AB91EDD1303673D125DB2E06698CA70DFA458EDDCBB85FE3809EC9673A68` |
| `FormationProductHost-final.log` | 6 | 0 | 0 | `B47CFBEE18EBB3ED2955FA5B35BF78399B04E089569B4A7AECB6A2312206A54C` |
| `FormationInfluence-final.log` | 85 | 0 | 0 | `10782A5FF7B89CA2E4855D042E1D27403AFC6B66E6E2EB6475BC742F045BE7F8` |
| `Attributes-final.log` | 4 | 0 | 0 | `FD4DC3DF89C9E320F92439DA21830EEA23AF65AAFDCC202D751280152E639EE1` |
| `Shanmen-full-final.log` | 334 | 0 | 0 | `AF2A99624FF0F465B8B1707827B118A2E674A1BA314B5D0DB195AF565F3296AD` |

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
- 自动发现、后台循环、raw object-pointer member、GAS、RNG、SaveGame/ProfileRepository 扫描均为 `0`。

## 构建证据

- Editor final：`up to date / 0 actions / 0.90s / exit 0`，log SHA-256 `8575654C58640A751A7546CB12AB7D2C7108DA98912C2B787CAF34B551EDA3E0`；
- Game final：`4 actions / 25.04s / exit 0`，log SHA-256 `F0F858114AC029251D829F34B4C055BC5633EF3AB50CD68228A14D565C803191`；
- `UnrealEditor-demo_map.dll`：`12093440` bytes，SHA-256 `2B3AAB9C8E7D1837BD79439BFF0CE29915117A5C65FE7A1C4374EDC606280C9B`；
- `demo_map.exe`：`353137152` bytes，SHA-256 `92E61BC95612BB4F3A8DA52C3DAB3D5560F7D3DB42EF27941B29EAE7CF2353D2`。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
