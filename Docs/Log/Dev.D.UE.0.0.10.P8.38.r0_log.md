# Dev.D.UE.0.0.10.P8.38.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.38.r0`；
- 基线：`636dc3c8ec92a9e8e90889655f90265edcb6b89b`（P8.37）；
- 分支：`agent/0.0.10-p8-38-combat-run-entity-alias-binding`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

在不公开可变 registry、不允许调用方注入 EntityId、不扫描 Actor/组件的前提下，让生产 `Fdemo_mapCombatRunCoordinator` 为 caller 已持有的 AttributeComponent capability 建立显式 entity alias，从而把 P8.37 的 formation consumer World resolution 接到真实 Run 权威。

## 决策记录

### EntityId 只能从已注册 source 推导

API 接受 registered object/body 与 alias object/body，但不接受 EntityId。事务使用当前 Run registry 解析 source，只有解析成功才可能绑定 alias。

### registry 继续保持 const public view

没有增加 mutable getter。Coordinator 只新增一个语义窄化、可审计的 alias 命令，防止 caller 绕过 Run、body 与冲突规则任意修改 registry。

### copy-on-success 防止半事务

绑定、冲突检查与 alias 复核全部发生在 registry 副本；只有 `Bound` 且 receipt 自检成功才提交。`AlreadyBound` 是只读成功，所有失败都保留原 registry。

### receipt 不保存 UObject pointer

结果只记录 UObject UniqueId 与 pointer-free identity evidence。两个输入 pointer 均为 caller-owned、调用期借用，不进入 Coordinator 新状态或 receipt。

### 回归范围按被改权威扩展

CombatRunCoordinator 现在能改变 World registry 中供 formation/attributes 消费的 alias，因此其映射不能只跑自身 focused group。规则明确要求 formation resolver、WorldGameplay、legacy Attributes 与全量 0.0.10。

## 执行序列

1. 审查 production caller，确认现有非测试代码尚未调用 formation consumer delivery。
2. 定位真实缺口：CombatRunCoordinator 拥有 Run registry，但只有 const getter，caller 无法显式登记已持有的 AttributeComponent alias。
3. 定义 alias status 与 pointer-free self-validating result。
4. 实现 source-derived EntityId、prepared registry、alias re-resolution 与 commit-on-success。
5. 新增 focused Automation，覆盖拒绝、generic/exact body、幂等、冲突、formation resolver 与 Run teardown。
6. 扩展 CombatRunCoordinator regression map 为五组证据，并新增一正一反自检；自检由 `96/96` 增至 `98/98`。
7. Editor 候选构建 `49` actions 成功，Coordinator 候选用例 `17/17`。
8. 生成五份最终 Automation 日志，全部 queue-empty、Fail `0`、exit `0`。
9. 执行 implementation changed-file gate、事务局部静态扫描与 `git diff --check`。
10. Editor 最终 0 actions 成功；Game 最终 48 actions 成功。
11. 生成 Report/Log，执行 exact-stage、staged gate、commit 与 push。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `CombatRunCoordinator-final.log` | 17 | 0 | 0 | `E462875F882EE46AC50D9C7F7A1C03E9280D1AA01D976B6C7E3007CA22F72AAB` |
| `ConsumerWorldResolution-final.log` | 1 | 0 | 0 | `AB8D31201A35182E719F81A069D449F574E622F7B739D5DC45D983A3410E6637` |
| `WorldGameplay-final.log` | 10 | 0 | 0 | `7267D64C5AC52A843D489D54C542876D0DD716BBB187D712701BF17F83549E32` |
| `Attributes-final.log` | 4 | 0 | 0 | `1C5392763306BB724F7F74B2995A758FDE3871E62E3103D7E8CDD126FD4FE6C5` |
| `Shanmen-full-final.log` | 336 | 0 | 0 | `FC763B83C5A0BE726BBE335DFA63DB090886D13A6862024251FA86DCEC4DBDE4` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=68
SELF_TEST: PASS 98/98
REGRESSION_COVERAGE (implementation): PASS Changed=5 Rules=1 Required=5 Logs=5
REGRESSION_COVERAGE (exact staged): PASS Changed=7 Rules=1 Required=5 Logs=5
exact stage: PASS Files=7
git diff --check / git diff --cached --check: PASS
```

- mapping SHA-256：`DE61BD48F18F4A4E3F70FD1C7B7F73EEEE0DA7FFF8BF66656F873CFB4F187F97`；
- self-test SHA-256：`8892802D8BAD35100CF3AE4C4A2601C061867475249E8154FDD8ADDEA85A996A`；
- 新事务块的 `FindComponent`、Actor iteration、subsystem lookup、Tick/while、TMap、UWorld、AActor 与 RNG 扫描均为 `0`；
- 新 receipt 的 raw UObject pointer member 为 `0`。

## 构建证据

- Editor candidate：`49 actions / 180.63s / exit 0`，log SHA-256 `303C2EFECA35FB8C790CEB544C4DF6C999DAC2B2DD1F2122069F1F486842FDB4`；
- Editor final：`up to date / 0 actions / 1.11s / exit 0`，log SHA-256 `0F2E3507B96D84E57A50CF043894FC218C0D2142F3376158CC4D1C9AB7AA12E1`；
- Game final：`48 actions / 151.83s / exit 0`，log SHA-256 `672426FB1F45E3E9C985CC451E33386C913106DC0B80EDC141D27B74DB7ECD7B`；
- `UnrealEditor-demo_map.dll`：`12123136` bytes，SHA-256 `917BC3A8C688FC2C56C50401890B38EED9F80FF76E6BFA7E569A5EC25137F276`；
- `demo_map.exe`：`353161216` bytes，SHA-256 `FB342855689993E71A14B1B884D6C14E1AB1119F8A2800CC83483A6B72819226`。

## 命令包装修正

- 最终回归首次 launcher 包含预删除目标日志步骤，被本机执行策略在 UE 启动前拒绝；删除该非必要步骤后五组验证一次通过。
- 首次局部静态扫描的 PowerShell `foreach | Format-Table` 包装产生空管道解析错误；将 rows 先赋值再输出后通过。
- 两次均未执行失败的产品验证、未修改产品状态，也不属于源码、Automation 或构建失败。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
