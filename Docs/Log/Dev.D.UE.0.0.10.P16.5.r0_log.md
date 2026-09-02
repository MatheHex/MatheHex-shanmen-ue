# Dev.D.UE.0.0.10.P16.5.r0 Development Log

## 1. 目标

为 P16.4 的 Meridian Shock treatment recovery proof 建立唯一、版本化、可校验、原子发布的 condition-domain durable store；不得复用陈旧 Profile 写入路径，不得复制 ShanmenItems 库存真值，也不在本轮提前改 Product Route。

## 2. 实现

- 新增 schema 1 RecoveryDocument，绑定 deterministic DocumentId、OwnerId、RunId、generation、UTC timestamps、ProofSetId 与 canonical proof array；
- 路径按 `ProjectSavedDir/ShanmenCombatConditions/MeridianShockTreatment/<OwnerId>/<RunId>.json` 分区；空 Root 和无效 identity 先于文件操作拒绝；
- parser 要求精确字段集、当前 schema、规范 proof 顺序、唯一 proof identity、同 Run 与重算 ProofSetId；最多 256 条；
- `RecordProof` 支持首次 publish、精确 replay 零写、TreatmentId 冲突 fence、generation 单步更新；
- `ForgetProof` 幂等删除并保留空的 audit-safe 文档；
- write path 使用 temp full flush、byte/document read-back、当前 generation 比对、verified backup、同卷替换与 postcommit read-back；
- load path 支持 corrupt-primary quarantine 与 verified-backup recovery；future schema、read failure、无有效 backup 均不 reset、不 downgrade、不误报 absent；
- 增加 save/load/mutation status、diagnostic 与测试专用 failure-stage injection；
- regression mapping 纳入 `RecoveryStore`。

## 3. 新增测试

```text
RecoveryStore.RoundTripReplayForget
RecoveryStore.FailureIsolationRetry
RecoveryStore.BackupRecoveryFutureSchema
RecoveryStore.IdentityConflictFence
```

关键断言：

- generation 1/2/3、proof canonical order、reload 与 forget 正确；
- exact replay 不写盘，同 TreatmentId 不同 proof 失败关闭；
- 原子替换前失败不改变 primary，相同请求重试只提交一次；
- corrupt primary 被保留后由 verified backup 恢复，generation 不变；
- primary 或 backup 的 future schema 均拒绝 downgrade/reset；
- proof 不能跨 Owner/Run 分区，空 Root 不能退化到进程目录。

## 4. 自查与最终验证

首次 focused 3/1 暴露的是测试证据读取顺序错误：成功重试后才读取 primary，却用于验证失败前状态。修正测试顺序后通过，失败日志保留，SHA `2C6AD8F14223ABE727A6ABFC82B29B8BBF57F5471B65CF2729149F682B4DFAAA`。

第一次 full 在 579/0 时因空 Root 路径风险主动终止；修复并补断言，非终态日志 SHA `84048CA7B02DB7059F78D19E1A024E89E1FCC23789A029D98BE3080C061E6612`。随后 734/0 后又发现 backup-only future schema 误报 Missing，修复并补覆盖；前一 full 日志 SHA `EBF0EC26EE396C1788536B9F3DCA76A4C4F5F415C2597CE45C82E485536907B8`，不作为最终证据。

最终证据：

| Group | Result | SHA-256 |
|---|---|---|
| Recovery Store focused | 4/0 | `A83E9C9240F8AC237B7FC73EC1A90CA8DCD30BAA9B273A1E456A39BE8CDB094B` |
| Shanmen.0_0_10 full | 734/0 | `3DF1261F9DC159A0C9D046523B95243A177BDD9C84BA3195F0D0998030CBCEBD` |
| CodeB | 60/0 | `B6812ADA84D4BB1A067C50ED3B44ED099D04B370AF215DA0FAC446C08EE353E5` |
| ItemEconomySchema | 24/0 | `53718F256C5523531545AF7CDB36C5B0DA3A35E075B4AA332E5A9F695013B3AC` |
| ItemUseAndArmor | 46/0 | `0357C8D2AAE5434928F5AAC079821C876818D7323F310E9884F277BDCEA9CF00` |
| P4.Hotbar | 7/0 | `00CE982027A1893A75F40B9D67964876D75EB1A5A16169369CEA0A1D127AFE95` |
| Profile | 211/0 | `A29B7BB2DC8A8581525AB913FBEB50EFBE4FB438C9D55748121AE4D79FDE59DA` |

mapped legacy 合计 348/0。最终日志全部 Fail/Fatal/Unhandled/Ensure/network=0，并包含 native terminal-success。

```text
REGRESSION_COVERAGE: PASS Changed=4 Rules=1 Required=10 Logs=7
SELF_TEST: PASS 271/271
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS HITS=0
GIT_DIFF_CHECK: PASS
```

gate output SHA `905A0670A22938A427583566E3955245308F72C3F677217840EA00E548FE90E2`；self-test output SHA `A34FB07154B570E39D7736BF27B33745DA4F6517015445ADCBA8ED7CA4004E6A`；mapping SHA `62E7F055919C0F1514AAFBCF83227BE57968F3087CB1687F9C8AA76F6FA786C7`。

## 5. 构建

- Game Development：Succeeded / 4 actions / 24.92s / native status 0 / log SHA `77F3522ADBD630A4FB93FBCB846DFDD8F875F63CFAD10A72C0BC7455F0404D53`；
- Editor Development final check：Succeeded / target up to date / 1.00s / native status 0 / log SHA `0193C32D0165DEE11FE8A8AED33913A74E645D0B76132DA74D3FD9C3FD25104F`；
- Game EXE：355,998,720 bytes / SHA `238599C20A3DDAECFE93FDA809AA9BE9E69488869FF65453A2FF25556C32B5C4`；
- Editor DLL：14,641,664 bytes / SHA `9E7DAB5E76BEC384D541C39B688B94CF75BCFA29604966B76431544C9398F47C`。

## 6. 边界

生产 store 对旧 Profile、旧 ItemSubsystem、World/Actor、UI、输入、timer 与 RNG 的静态扫描为 0。它尚未自动接入 Product 生命周期，也不提供跨进程锁；P16.6 应串行连接“condition mutation 后持久化 proof -> item commit -> 完成后 forget”的完整恢复链。

仅执行 P 阶段 C++、真实磁盘 Automation、NullRHI full/legacy regressions、静态检查与 Development builds；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户资料未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-5-meridian-shock-proof-store>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-5-meridian-shock-proof-store/Docs/Report/Dev.D.UE.0.0.10.P16.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-5-meridian-shock-proof-store/Docs/Log/Dev.D.UE.0.0.10.P16.5.r0_log.md>
