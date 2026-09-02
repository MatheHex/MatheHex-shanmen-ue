# Dev.D.UE.0.0.10.P16.1.r0 Development Log

## 1. 目标

建立 Meridian Shock 的唯一产品事务路由，在 Game Thread 上拥有 `prepare item -> treat condition -> commit consumption` 顺序，并实现精确重放、RequestId 冲突门禁、治疗前安全取消与治疗后 commit-only 恢复。

## 2. 实现

- 新增不可变 `Fdemo_mapShanmenMeridianShockTreatmentCommand`；
- 新增 `Fdemo_mapShanmenMeridianShockTreatmentProductRoute`，精确绑定 active Run 与 condition authority；
- RequestId journal 拒绝不同 payload，精确 replay 不重复写入；
- fresh command 依次调用 P16.0 prepare、condition treatment 与 item commit；
- treatment 失败只在同一实时 active revision 下取消准备；
- treatment 成功后永久进入 commit-only 分支；
- ambiguous prepare、prepared 或 treated 未完成时禁止 route end；
- 新增 post-treatment interruption 与 item-store failure 测试钩子；
- 新增 3 个产品路径 Automation tests；
- 回归映射增加 route 对 full、治疗、条件、Items 与受影响 legacy 组的强制证据。

## 3. 集成修正

真实 GameInstance authority 首次使 focused route tests `0/3`：P16.0 adapter 把稳定 ShanmenItems authority content stamp 与可变 product catalog identity 混为同一身份。追加诊断后再次复现 `0/3`，随后改为：

- authority snapshot 比对 `ProductContentStamp()`；
- product Definition 独立比对当前 catalog；
- P16.0 fixture 使用真实 authority stamp。

没有放宽 snapshot revision、Run、Definition、语义或 Quantity 门禁。首次失败、诊断失败与最终通过日志均保留。

## 4. 产品路径测试

```text
OrderedCommitReplay
CommitOnlyRecovery
CancelAndConflict
```

- success：数量 `3 -> 2`、条件 revision `1 -> 2`、commit finalize 1、replay 零突变；
- interruption + `WriteTemp` failure：condition 已治疗、route 阻止 end、authority 重启后只恢复 commit；
- stale timeline：仅在同一实时 active revision 下 durable cancel `3 -> 3`；
- 同一 RequestId / 不同 timeline payload：写入前冲突拒绝。

## 5. 最终验证

| Group | Result | SHA-256 |
|---|---|---|
| Route focused final | 3/0 | `A845D6AEC44B90E4A7990461193141A5850A889B49840449616089B3EF31F724` |
| MeridianShockTreatment final | 7/0 | `21F5BBC66E59FB4D154FA6166612205918D90A687C6187FF19865DBAD3076CA3` |
| ItemEconomySchema | 24/0 | `E8AA2992CA1ABA2425619EA626F32735460B888DADDF3F307F7D4F3B63A4732A` |
| Profile | 211/0 | `F9819EB641610920CABC965C39485A215D5400DDDFC9CDB77FFB9CBED708BF23` |
| CodeB | 60/0 | `3AF17969F53C63F275BAE41800DF95CCBA062BB65BD50784DB5CA6E500FD347F` |
| ItemUseAndArmor | 46/0 | `3388905E4B36C8212855B84540700B38CE03B7E69A0CB1BB6290AB23BAEE1463` |
| P4.Hotbar | 7/0 | `3C4D74C618954279EF8097B560475A8DA3D7F6EC345AC862730D3D6CE852641E` |
| Shanmen.0_0_10 full | 720/0 | `5CB84D8332FCEDAC774910FB32089D5CAC76A0B746D25742037BC710DA5A358A` |

首次失败：`35DC9421BBED357FA929002ACFDE601CFA08EF4223D8D1A6C049E6520F957867`；诊断失败：`3693B2320EE28EA8413784AEE2C2730E13B008A5E3D5B2546A9208B38C7A94BB`。

流程结果：

```text
REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=10 Logs=7
SELF_TEST: PASS 271/271
JSON_PARSE: PASS
GIT_DIFF_CHECK: PASS
BOUNDARY_SCAN: PASS
```

流程 SHA：gate `7660AA75C16FCDC0FBB62E2BFFC68B1BB08839AA07F055ECEAA7D8490AC523F9`；self-test `A34FB07154B570E39D7736BF27B33745DA4F6517015445ADCBA8ED7CA4004E6A`；JSON `48C32879BFB681137257F2FA697C74FED7A350541D953E584A70FE8C7B067345`；diff `6800E9EECC22306DA1EEF24FAB91ACBEEE5282A7287C221FDB09CF61D6CE27E9`；boundary `5A69B655C3CC5E2DEF3EA05ACA4BF71E4047158A0D274603761E01D51F9B257C`。

最终 gate 首次运行暴露检查脚本不识别 UE `Automation RunTests <group>;Quit` 原生格式；已将分号后的命令从组名解析中排除，并新增一项针对性 self-test，最终 271/271。

## 6. 构建

- Game final：Succeeded / 6 actions / 30.62s / status 0 / SHA `763EDFA6EF3CEDE2EBA23BC60F9D39C648EB757F72ECACFAE4D5C4BB12EBC3D4`；
- Editor final：Succeeded, up to date / 0 actions / 0.91s / status 0 / SHA `6C86AD2192A15C9472E6C03C97EFE6D038DC3E34FF8F001141EE409E63E35965`；
- Game EXE：355,839,488 bytes / SHA `A9FE339BD4D6CC9457CBD7BB302C4B9ABD1071A5CF79526EE54B4002F9D191C9`；
- Editor DLL：14,447,616 bytes / SHA `8C54D714AB13B68811349D727C93FFFFD3A1FDF9CFD87AA8DDED6852FE43D74A`。

## 7. 范围

本轮仅 P 阶段 C++ 契约、NullRHI Automation、流程检查与 Development build。没有 UI、正式 input/hotbar 接线、PIE、Standalone、产品 exe 运行、真实输入、截图、Smoke、Cook 或 Package。

route journal 目前是运行时恢复真值；本阶段验证 authority restart/rebind，不声称 route/condition 已具备进程崩溃持久化。下一阶段接入唯一 session/lifecycle checkpoint 与正式输入入口。

raw logs 仅本地保存；长期未跟踪用户资料未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-1-meridian-shock-treatment-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-1-meridian-shock-treatment-route/Docs/Report/Dev.D.UE.0.0.10.P16.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-1-meridian-shock-treatment-route/Docs/Log/Dev.D.UE.0.0.10.P16.1.r0_log.md>
