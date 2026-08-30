# Dev.D.UE.0.0.10.P8.27.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.27.r0`；
- 基线：`df86477004711c7b4d48f5bdc8c0604801e83db9`（P8.26）；
- 分支：`agent/0.0.10-p8-27-formation-influence-attribute-adapter`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-30`。

## 目标

把 P8.26 registry 已接受的 deterministic Apply／Remove 命令翻译到既有唯一属性权威。要求 adapter 无状态、exact handle 可重放、fixed-point 转换集中、冲突不误删、失败有结构化 acknowledgement/result，且不建立第二套 modifier table。

## 设计记录

### desired-state native seam

在 `Udemo_mapAttributeComponent` 增加 exact-handle Apply／Remove，不绕开既有 `ActiveModifiers` 与 `Recalculate()`。same handle + same spec 是 replay；same handle + different spec 是 conflict。legacy API 保持原有调用契约。

### one numeric boundary

consumer projection 在 adapter 之前保持 int64 numerator/denominator。只有 `TryBuildModifierSpec` 允许转换为 native float，并拒绝非有限、越界和非零下溢。

### stable acknowledgement

ack ID 由 immutable registry receipt 与 projection evidence 派生，不把 Applied／Replayed 的传输态写入语义 ID，因此 lost-ack replay 稳定；native status 仍单独保存供诊断。

### conflict-safe remove

提交前审查发现只按 handle Remove 会在碰撞时误删其它 spec。最终 API 要求 Remove 同时携带 expected spec；冲突只返回 `HandleConflict`，不改变 component。

## 执行序列

1. 审查 legacy `AddModifier`／`RemoveModifier`、active handle、modifier ordering 与 float 运算边界。
2. 增加 `Fdemo_mapModifierSpec::Matches`、exact mutation status 与 component convergence API。
3. 新增 stateless consumer attribute adapter、result 与 deterministic acknowledgement。
4. 新增 Apply replay、Remove replay、missing-native compensation、handle conflict、signed conversion 与 evidence fence 四项 adapter case。
5. 增加 legacy exact-handle component case，并把 V3 Items／ItemUseAndArmor 纳入改动文件回归映射。
6. regression map 增至 `63` 条，自检由 `82/82` 增至 `86/86`。
7. 首次 Editor：`186 actions / 520.11s / exit 0`；首轮八组 Automation 全绿。
8. 提交前审查发现冲突 Remove 风险；改为 expected-spec Remove，并扩展冲突专项。
9. 最终 Editor：`25 actions / 73.06s / exit 0`；重跑八组 Automation 全绿。
10. changed-file gate：产品/流程改动为 `Changed=9 / Rules=3 / Required=11 / Logs=8`；Report／Log exact-stage 后为 `Changed=11 / Rules=3 / Required=11 / Logs=8`。
11. Game：`185 actions / 457.83s / exit 0`；完成静态边界与 diff 检查。

## Automation 证据

| Log | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `P8.27-FormationInfluenceConsumerAttributeAdapter-final.log` | 4 | 0 | 0 | `DC706A357A346A19729CD564C93238A9EB187755C5E01280511E62F5CF102FFE` |
| `P8.27-Attributes-final.log` | 4 | 0 | 0 | `044FDE0F0B9D3DAD28502E8D8A66E1E1101CAAE7863B810CE0FEBEE13DAEE23E` |
| `P8.27-V3Items-final.log` | 5 | 0 | 0 | `21983E9FCB2DA37A6E4053F03C00E1350BACD70EFAB6892220232A903C7A569C` |
| `P8.27-ItemUseAndArmor-final.log` | 46 | 0 | 0 | `5944116507DB7ECCAC1EB3C014FA8070EE43748B0873BB0AA6A9E243BBEFA858` |
| `P8.27-FormationInfluenceConsumerProjection-final.log` | 3 | 0 | 0 | `085E488AE248CD2509EB4558B03B4447EC1B26F06002403AD3B14C6ECAF2471E` |
| `P8.27-FormationInfluenceConsumerRegistry-final.log` | 4 | 0 | 0 | `22CF46C10AD1F0C6BC853D487B36D9C3457206ABEC58C5490DD17576B57F5E1F` |
| `P8.27-FormationInfluence-final.log` | 67 | 0 | 0 | `34A35F2AC090FEF6A6D293EBB079CED8F7ECB45516AB8125B5EF2CE727CA9D82` |
| `P8.27-Shanmen-full-final.log` | 316 | 0 | 0 | `8869C4B73651866C1AAF96F83B793C965226E533D5BFA5F0FD0C749D92EBF436` |

全部观察到 queue-empty；fatal／unhandled／ensure 为 `0`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=63
SELF_TEST: PASS 86/86
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=11 Logs=8
REGRESSION_COVERAGE: PASS Changed=11 Rules=3 Required=11 Logs=8
```

- mapping SHA-256：`5608BC29C79651B499BFE84C3CB4212CE115D6EDB79421530334BA82A59228ED`；
- self-test SHA-256：`ACA5C684EAFA9AC3AFE56176C06DAF25039BAB78B5FA2E6E0050F84EEE3579B1`。

## 静态边界

```text
UWorld/AActor = 0
GameplayAbility/GameplayEffect/AbilitySystem = 0
Timer/Async/RNG = 0
SaveGame/ProfileRepository = 0
Tick/while = 0
TMap = 0
TArray<Fdemo_mapActiveModifier> = 0
```

exact rational 到 float 的实现只位于 adapter `TryBuildModifierSpec`；adapter 不拥有 native modifier collection。

## 构建证据

- Editor initial：`186 actions / 520.11s / exit 0`，SHA-256 `B764E6A59A945A20826931BC7A94E6898CD059D09697EBDB5011CF8B92151FA7`；
- Editor final：`25 actions / 73.06s / exit 0`，SHA-256 `52BC4494E317DA559B1C7FF606B5C41A524ABBB6D3FC04E2CD39601F133251E2`；
- Game final：`185 actions / 457.83s / exit 0`，SHA-256 `AA8DD37709C04E9B3209533DD4BD9355485DB4B23E79A324EB37F422E96AFAD1`；
- `UnrealEditor-demo_map.dll`：`11897856` bytes，SHA-256 `8893BC77D0A9D0BAB83C0E79E11B12733CD9AF6016A8401A3B97BF271C8CE88F`；
- `demo_map.exe`：`352979456` bytes，SHA-256 `7DF619AC4A7EBC4B39B07AF615FA9DD0E24463AA10A8D080CD79128D6802AED6`。

## 真实异常记录

产品源码、Editor/Game 构建与 Automation 均无失败。一次 self-test 使用 Windows PowerShell 5.1 触发 parser error；项目脚本要求 PowerShell 7，改由已有 `pwsh` 执行后 `86/86` 通过。未把该解释器错误描述为产品失败。

## P/F 边界

仅执行源码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.28 可增加 consumer application coordinator：绑定目标 component、编排 registry/native 顺序、记录 acknowledgement，并对 native rejection 与 teardown 明确补偿；不把 adapter 扩成第二套状态权威。
