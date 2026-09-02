# Dev.D.UE.0.0.10.P16.2.r0 Development Log

## 1. 目标

把 P16.1 Meridian Shock `prepare -> treat -> commit` 唯一路由安装到正式 hotbar 输入、active Run Session 与产品生命周期；UI 只提交输入，任何未完成 commit 必须在 Run authority 拆除前恢复。

## 2. 实现

- 新增 treatment ProductSession，按 RequestId 保存不可变 command，提供冲突门禁、精确 replay、稳定 pending recovery 与 guarded teardown；
- 新增 ProductLifecycle，绑定 ready ShanmenItems authority、完整 active Run correlation 与同 Run condition authority；
- 新增 InputAdapter，从冻结 hotbar、authority snapshot、当前 Definition 与 treatment semantic 判定输入所有权；
- 非治疗物品在 timeline/ordinal/RequestId 生成前放行；治疗语义一旦命中，所有结果均被处理，不进入通用回血；
- RequestId 确定性包含 correlation、Run、item、slot、authority revision、timeline sample 与单调 ordinal；
- PlayerController 顺序固定为 gameplay gate、thrown weapon、Meridian Shock treatment、generic quick slot；
- GameMode 在 condition authority 启动后绑定 treatment lifecycle，在 Run release 最前执行 commit-only 恢复。

## 3. 新增测试

```text
Session.HotbarOwnership
Lifecycle.TeardownRecovery
Input.DeterministicIdentity
Session.BindingFence
```

关键证明：普通消耗品放行且不采样 timeline；治疗物品数量 `3 -> 2` 并清除 Meridian Shock；inactive 再次输入被 treatment 链拒绝且权威 snapshot 不变；post-treatment interruption 在 teardown 中只 commit、不 cancel；相同 RunId 不能掩盖 foreign full correlation。

## 4. 最终验证

| Group | Result | SHA-256 |
|---|---|---|
| Meridian Shock focused | 11/0 | `4BD8B4EC5F8DD6E5A18C811F5DA999AD2AF52891F3F909CAA9A8AF0BDCB691A7` |
| Shanmen.0_0_10 full | 724/0 | `4766651559E14502F5915896DDB69212BB6B07CD9E9E8CC2E684CC8055358905` |
| ItemEconomySchema | 24/0 | `6577AC4093E132083699C4515B1392DE498A7D9EC6FE31680401524AFDD157F6` |
| Profile | 211/0 | `D86EBE454850F445CEFC17A1CB01BBC60CA73B34796C05BEFF375699063AD690` |
| CodeB | 60/0 | `1B5E92B60437EE16ECD8DAECE76085094EFC48483CB0C9B53A6B656E1C643670` |
| ItemUseAndArmor | 46/0 | `A242F5FA2ED15CA10302D023F7292F7BB23094119BCA8C82E393367F12F72AED` |
| P4.Hotbar | 7/0 | `340778519D013940D51505A7C3A60EE5ED5D6E211CA63E2F7B7F3F495B393AA8` |
| InputRestore | 101/0 | `316A5B6D09E86DCE80C04C21FCE606A5FC1179A95673EC3BA0AF2B18EACC3B75` |
| V3.Attributes | 4/0 | `D9BEE8BD328875D4BBF8AB1DAA0C7357A05349F3E9D68C8706146B93175BDE9C` |
| EnemySkillFramework | 44/0 | `F6EFAFAA4F5D5A6922A94A4DE54C360A1B5C891E7D11FF32A0876A5EBD1541CE` |
| V2RangedCompatibility | 22/0 | `343004EDCF2D9F3E784D88B15100B1634834C60915A0A81A83D27A39F6DD614A` |

改动映射回归合计 519/0。全量用时约 28m05.61s，queue-empty 终止标记 1，原生退出码 0，Fatal/Unhandled/Ensure 0。

流程结果：

```text
REGRESSION_COVERAGE: PASS Changed=13 Rules=4 Required=54 Logs=10
SELF_TEST: PASS 271/271
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS Files=6
GIT_DIFF_CHECK: PASS
```

流程 SHA：gate `4DBFD38148D5302E7CBC54686DD4CC569D37389695EF73DEC05C52CED52E9D7E`；self-test `A34FB07154B570E39D7736BF27B33745DA4F6517015445ADCBA8ED7CA4004E6A`；JSON `DA06834B2330D35329656BF228F7DB0E98064DAFD695940C400B96E4EFBC1E83`；boundary `6B2E1A69CAAFBCAEE47132A5A2718A46A41C1B98450E4D0D3983A3FC727402BD`。

## 5. UE 5.8 环境处理

两次全量尝试分别在 579/0 与 554/0 主动停止，原因是 UE HomeScreen `generate_204` 后台探测造成测试调度 large-delta；两份日志均无终止标记，不计为通过证据。

经 UE 5.8 源码定位后，最终测试仅在该进程使用 `-ForceDPCVars=HomeScreen.EnableHomeScreen=0`。probe 1/0、最终 full 724/0，网络探测 0；没有修改 Windows、Engine 或用户配置。

中断日志 SHA：`7AC2CE3B7ED4AE2F851F2B5FA7D158B1A7154B5D60DFE5B941B9F363FD2DFBA7`、`B969781A56F5B84063477CB02D6BC8817CCF23B8E751DCA7C2DD58A907CDF9F9`。

## 6. 构建

- Game Development：Succeeded / 30 actions / 129.02s / status 0 / log SHA `766B93B977BF80A28FA924E22A796AE23D05C655123C53A93A854892DD263697`；
- Editor Development：Succeeded, up to date / 0 actions / 0.99s / status 0 / log SHA `A376433754336706CF784184EDFB00C83F4DA65C38E618DEFDB1DAA42DF4ABFF`；
- Game EXE：355,881,984 bytes / SHA `4CF99F45E7828B4E2CE20B6A53BB7EE0D28D6CE4AA3CD44233851B844D322DBA`；
- Editor DLL：14,497,280 bytes / SHA `08C24517AE06596CBB08A5DBACF348D340FACAF4DC3303E85B25464ADFD6A429`。

## 7. 范围

本轮仅 P 阶段 C++ 产品接线、NullRHI Automation、流程检查与 Development builds。未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

Session journal 只提供当前进程内 recovery；不宣称进程崩溃持久化。raw logs 仅本地保存，长期未跟踪用户资料未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-2-meridian-shock-treatment-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-2-meridian-shock-treatment-session/Docs/Report/Dev.D.UE.0.0.10.P16.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-2-meridian-shock-treatment-session/Docs/Log/Dev.D.UE.0.0.10.P16.2.r0_log.md>
