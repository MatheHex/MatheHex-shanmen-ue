# Dev.D.UE.0.0.10.P8.26.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.26.r0`；
- 基线：`e54348a303021fc0c1f0904498c7f58107781828`（P8.25）；
- 分支：`agent/0.0.10-p8-26-formation-influence-consumer-registry`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

把 P8.25 纯值 Apply／Remove command 收进一个可重建、可查询、幂等且支持 teardown drain 的 application registry。必须阻止 slot 冲突和 stale Remove，不直接写 Actor／attribute component，并保证暂态失败不会永久阻断同一 deterministic command 的后续重试。

## 设计记录

### success-only history

只记录成功状态转移。失败结果没有 receipt，不进入 replay history；条件变化后可以再次执行同一 command。成功 command 则稳定返回原 receipt，不重复 Apply／Remove。

### slot and application identity

registry scope 是 run/content；活动 slot 是 lease/target attribute。application ID 纳入 registry、Apply command、handle、lease 与 attribute，避免不同运行或不同 authored projection alias。

### reconstructable state

一致性检查从 completed commands 重放状态，不信任当前 active array。每条 receipt 的历史 active count、application ID 与 command 都被复核，最终重放集合必须与 active snapshots 相等。

### stale remove semantics

同 slot 不同 handle 的未完成 Remove 返回 `StaleHandle`，不写历史。已完成旧 Remove 仅重放自己的 receipt，因此新 handle 重用同 slot 后仍安全。

## 执行序列

1. 审查 P8.25 projection/command 与现有纯值 executor 状态模式。
2. 决定只永久记录成功命令，保留 collision/missing/stale 的可重试性。
3. 新增 registry、active snapshot、success receipt、deterministic registry/application/receipt ID。
4. `IsConsistent()` 采用 completed history 重放并与 live snapshots 交叉验证。
5. 新增 replay、collision retry、stale remove、scope/missing/drain 四项专项测试。
6. 新增 changed-file mapping；projection test 文件与 registry 规则重叠，自检由 `80/80` 增至 `82/82`。
7. 首次 Editor `5 actions / 28.94s / exit 0`，专项及四层回归全部通过。
8. 审查后把 stale Remove 显式加入 collision case；最终 Editor `4 actions / 5.98s / exit 0`。
9. 重跑最终五组 Automation：`4/4`、`3/3`、`3/3`、`63/63`、`312/312`。
10. regression gate 首次经外层 `pwsh -File` 传数组时包装失败（exit `1`）；直接调用脚本后 `Changed=5 / Rules=2 / Required=7 / Logs=5` 通过。
11. Game `4 actions / 24.19s / exit 0`；Report／Log exact stage 后最终 gate 以 `Changed=7 / Rules=2 / Required=7 / Logs=5` 通过。
12. 完成静态边界、cached diff 与 exact staging 收口。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `P8.26-FormationInfluenceConsumerRegistry-final.log` | 4 | 0 | 0 | `2AAA4F27DD3CEB4F5B31620E1B7670DD257E8C9AA813D7B61141E7EF0447C2C8` |
| `P8.26-FormationInfluenceConsumerProjection-final.log` | 3 | 0 | 0 | `5E7E091DDA4545736160B96123A7B238A896C64BA02B5B900C914071769CF43E` |
| `P8.26-Attributes-final.log` | 3 | 0 | 0 | `F9E72261931CB3FB94AC3B2A4E855C6B79BDFB49EBC200F730B9B4394C1231C5` |
| `P8.26-FormationInfluence-final.log` | 63 | 0 | 0 | `0A0C4A5F5C380545AE150953F5B14DFF2F7EB67D4E0AAC21731430D5FB621130` |
| `P8.26-Shanmen-full-final.log` | 312 | 0 | 0 | `21B2AF002E818ABE91E0348B4DE9D6357A03A1F1C016730326A72846F1A3FD3B` |

全部观察到 queue-empty；fatal／unhandled／ensure 为 `0`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=61
SELF_TEST: PASS 82/82
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=7 Logs=5
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=7 Logs=5
```

- mapping SHA-256：`2093F23B708644CA7B3B60C107774AA1362D61B964174130C95F473D39C728ED`；
- self-test SHA-256：`9BC49FB1B22B90EE98481AF089EF6DC3AE26E825C2C293D8FBA064666CF9500F`。

## 静态边界

```text
UWorld/AActor/UObject = 0
GameplayAbility/GameplayEffect/AbilitySystem = 0
Timer/Async/RNG = 0
float/double = 0
Spawn/ApplyDamage/SaveGame/ProfileRepository = 0
Tick/while = 0
AddModifier/RemoveModifier/Fdemo_mapModifierSpec = 0
```

## 构建证据

- Editor initial：`5 actions / 28.94s / exit 0`，SHA-256 `AFFB3A20DB56FE6DDABB2021E77C9165FC1D80D726FFC2053BE884DBC3A50BC7`；
- Editor final：`4 actions / 5.98s / exit 0`，SHA-256 `6121109584F3B93C6FB00958D1229F57F0D6C5CB4E6A5AF5A7E3372F1569DBCC`；
- Game final：`4 actions / 24.19s / exit 0`，SHA-256 `4F8D00DAB45AA71D79FC1AD20586BDDFE6C19420F8E72CB94B3300A7484305F2`；
- `UnrealEditor-demo_map.dll`：`11870208` bytes，SHA-256 `4B40677B3C16941F073ACB90D2057E54F813CC64A4D78E3681E8AF0551037A72`；
- `demo_map.exe`：`352957440` bytes，SHA-256 `CA308580EC5C1AE23D060E13422C999D775CB8697F4F503ADD4D0FB8FC8F6316`。

## P/F 边界

仅执行源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.27 可实现窄 consumer application port／adapter，把 registry 的 exact command/handle 翻译到既有 attribute modifier seam；adapter 必须集中处理 fixed-point 转换、失败 acknowledgement 与补偿，不得成为第二套 attribute authority。
