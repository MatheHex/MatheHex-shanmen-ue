# Dev.D.UE.0.0.10.P8.37.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.37.r0`；
- 基线：`a81f344f50b4db9057e749504926c063d4fed31a`（P8.36）；
- 分支：`agent/0.0.10-p8-37-formation-consumer-world-resolution`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

让 caller 已经取得的 AttributeComponent capability 先通过现有 run-scoped World entity registry 证明其 EntityId，再生成 P8.36 pointer-free subject resolution；不让 LifecycleCommandHost 扫描 Actor、发现组件、保存 UObject pointer 或承担 World lookup。

## 决策记录

### registry 绑定组件本体，不反查 Actor

`FShanmenWorldEntityRegistry` 已允许多个对象显式别名到同一 EntityId。调用方因此可以分别绑定 Actor 与 AttributeComponent；resolver 直接查询组件对象，不需要新增 EntityId→UObject 反向表，也不会引入 `FindComponentByClass`。

### resolver 保持无状态和只读

新类型只接受 registry const reference、expected Run、component capability 与 BodyIndex。它不拥有 Host/runtime，不修改 registry，不执行 consumer command。结果冻结 Run/Entity/body 与 nested resolution，不保存 pointer。

### body 语义沿用现有 registry

generic binding 可匹配任意 body；exact-body binding 只在对应 body 下解析。resolver 不另建一套 body 规则。

### ProductHost 测试必须走真实证据链

P8.36 用例原先直接调用 `SubjectResolution::TryCreate`。P8.37 将本地与 foreign 组件都显式绑定到 registry，再通过新 resolver 产生 resolution，从而验证 World identity、delivery provenance 与 live component 三层 fence。

## 执行序列

1. 审查现有 World entity registry、formation World adapter、ProductHost 与 P8.36 LifecycleCommandHost 边界。
2. 排除把逻辑放入有状态 World placement adapter：该类包含 Actor recovery，不适合作为 consumer capability resolver。
3. 定义精确 World-resolution 状态、自校验 result 与 stateless resolver。
4. 实现 component-object registry lookup，并复用 P8.36 resolution factory。
5. 新增 focused automation，覆盖 Run、registry、component、body、binding 与 deterministic replay。
6. 把真实 LifecycleCommandHost consumer-fence 测试迁移到 registry-backed resolution。
7. 为新增路径增加 regression map 规则及一正一反两个自检，自检由 `94/94` 增至 `96/96`。
8. Editor 候选编译 `6` actions 成功；resolver、ProductHost 与 LifecycleCommandHost 候选用例均通过。
9. 生成六份最终 Automation 日志，全部 queue-empty、Fail `0`、exit `0`。
10. 执行 changed-file gate、静态扫描、`git diff --check` 与 Editor/Game 最终构建。
11. 生成 Report/Log，执行 exact-stage、staged gate、commit 与 push。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `ConsumerWorldResolution-final.log` | 1 | 0 | 0 | `FF70BE1ECE79870E32A63AAB3BBEE8FCA00D6FD75D72D87305810E6888C425DB` |
| `LifecycleCommandHost-final.log` | 5 | 0 | 0 | `47E2DBE4DBE59381CEAF411A06D06693C4CD57BD209262E0957C4B64AF8AB3CE` |
| `FormationProductHost-final.log` | 6 | 0 | 0 | `08CCF4D6738EFF91AA932C31FCB3EE428EA5500470B1122CECF3148E157586E2` |
| `WorldGameplay-final.log` | 10 | 0 | 0 | `AAE108A8F597C070395BD479BF1101E1910755574A7B20B1D890F45704B134B7` |
| `Attributes-final.log` | 4 | 0 | 0 | `B33635EE65DA06175384A583C2FFC2BB509AF444B1F15B629E798032910F0350` |
| `Shanmen-full-final.log` | 335 | 0 | 0 | `5E55CF1A2DDFACCD26E5A8F28ED83DFF663B2221AD393150DA5A1223BC096A76` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=68
SELF_TEST: PASS 96/96
REGRESSION_COVERAGE (implementation): PASS Changed=6 Rules=2 Required=36 Logs=6
REGRESSION_COVERAGE (exact staged): PASS Changed=8 Rules=2 Required=36 Logs=6
exact stage: PASS Files=8
git diff --check / git diff --cached --check: PASS
```

- mapping SHA-256：`D72396E917793A06AAE2B40451DEBB21C1E78CCC6564A49A12FA0044030D7C2D`；
- self-test SHA-256：`BF4CADC87E3C1D5FD0C1B938975813921BB55F2C9CC243549F09A49875AE6C4A`；
- `GetSubsystem`、`FindComponent`、Actor iteration、Tick/while、raw object-pointer member、GAS、RNG、SaveGame/ProfileRepository 扫描均为 `0`。

## 构建证据

- Editor final：`up to date / 0 actions / 0.89s / exit 0`，log SHA-256 `45E0867D85C8A3C33E4682BA06AE2A12B7E4851F2FE71D08B622855B4285F9E7`；
- Game final：`5 actions / 27.82s / exit 0`，log SHA-256 `DA6DC0AD2723B7634B73C06BF4BAF717CEE93581144A405BD70EB899FAD6ED99`；
- `UnrealEditor-demo_map.dll`：`12112896` bytes，SHA-256 `DF7D5DD69766DFF480988AAD2E3CEB0B7AAA011D312CA4AE542C3DFE67D736E5`；
- `demo_map.exe`：`353152512` bytes，SHA-256 `F873F1E044D95C3553965A66F6ADE93B3A16629E54BC290AF93F7CBE2A3A7136`。

## 命令包装修正

- 首次 self-test 使用 Windows PowerShell 5.1，因不支持项目 PowerShell 7 的行首管道语法而在解析阶段退出；改用已配置 `pwsh` 后 `96/96`。
- 首次 gate 通过 `pwsh -File` 跨进程传数组，数组被展平而触发 positional parameter 错误；在当前 PowerShell 7 进程直接调用脚本后 gate 通过。
- 两次均未启动 UE 验证、未修改产品代码，也不属于源码或 Automation 失败。

## P/F 边界

只执行 P 阶段源码开发、静态审查、无头 Automation、changed-file regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。
