# Dev.D.UE.0.0.10.P12.10.r0 Development Log

## 1. 目标

把 P12.9 pure symbolic evaluator 接入唯一 SwordRhythm ProductSession：由产品 config 安装 canonical policy，accepted BasicSword 原子生成 immutable evaluation receipt，并向既有 presentation seam 只读投影 symbolic identities/tokens；不定义数值或应用 gameplay effect。

## 2. 实现

- product config 升级为 `0.0.10.P12.10` / r2，拥有 PreciseSwordLink、PerfectWeaponGuard、SpiritEvasion 三来源 canonical policy；
- ConfigId 纳入 PolicyId，`IsValid()` 重建并核对 canonical policy；
- accepted BasicSword 在候选 Session 中依次形成 rhythm、binding、input、evaluation 与 presentation，全部有效后才原子提交；
- Session 只读保存/暴露完整 evaluation receipt；teardown 与既有 rhythm/presentation 状态一起清除；
- presentation read model 只投影 EvaluationReceiptId、EvaluationPolicyId 与 ordered EffectDefinitionIds；
- 真实 lifecycle 验证 Guard+Evasion 两 token、合法空 receipt、replay 稳定、下一拍 PreciseFlow 与 teardown；
- regression map 增加 source-aware route/evaluator/run-host/read-model/event 要求；self-test 增加正例与缺证据反例；
- 未新增 magnitude、multiplier、strength、damage、attribute、World、Actor、RNG、timer 或效果应用逻辑。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `2AB09F06668A64B8A40E96498E10D95CE2A192E224F0C36ADAE482C7EC649998` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `01019067C7D40C3DDB8E1102B631D505B93C0EAC017D19049C324F922A3F4D06` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `1F3FDC86B474B8F16DD7C4E2DEA963D8F0CB3BEC9137A1CFD03343D7FCECDE0B` |
| `SwordRhythmPresentationEvent` | 2 | 0 | 0 | `BFD52ED9B42A97851EA6538E6F42A456BB173113B34B9121965FC4D924648A32` |
| `SwordRhythmEvaluation` | 4 | 0 | 0 | `0C8597C79AB85EB1ADFDC3755777F5808756C6B1EB1A34476D6A48D410D61966` |
| `Shanmen.0_0_10` | 566 | 0 | 0 | `3E7CDBD36C909F3F4A79503A7E26600A28C26AF3397FE97C7BE57E5CE8F2D6B1` |

所有选定阶段均为 native exit `0`、Fail `0`、terminal marker 有效。Presentation 前缀包含 Event 子组；全量唯一用例 `566/566`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=16 Logs=6
SELF_TEST: PASS 182/182
PRODUCT_BOUNDARY_SCAN: PASS
ENGINE_BOUNDARY_SCAN: PASS
AUTHORITY_SCAN: PASS GameMode unchanged
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：30 actions / 134.17s / exit `0`；
- Editor final：0 actions / 0.92s / exit `0`；
- Game final：27 actions / 112.67s / exit `0`；
- Editor DLL：13,083,136 bytes / SHA `0E1930AD90A26B3AB0B9D5A81C65C1FCCA9AE7B1CCB9CB31875B0E80C36151F0`；
- Game EXE：354,760,704 bytes / SHA `EC77300575A3CED9C4C548A42CC28AA14DCEC066467EF4B043D6BEA0DFA19861`；
- 无源码构建失败或 Windows commit-memory/page-file 错误。

## 6. 修改、兼容性与边界

Report/Log 前 8 个代码/流程文件净变更 `+458 / -41`。既有 GameMode、动作、资源、damage、attribute、inventory 与 GAS 权威未改；唯一 Session 继续作为运行期聚合权威。长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-10-sword-rhythm-evaluation-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-10-sword-rhythm-evaluation-product-route/Docs/Report/Dev.D.UE.0.0.10.P12.10.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-10-sword-rhythm-evaluation-product-route/Docs/Log/Dev.D.UE.0.0.10.P12.10.r0_log.md>
