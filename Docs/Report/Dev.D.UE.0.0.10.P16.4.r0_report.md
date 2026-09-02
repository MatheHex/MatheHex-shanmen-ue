# Dev.D.UE.0.0.10.P16.4.r0 Report

## 1. 结论

P16.4 已为 Meridian Shock 治疗链补上**版本化、规范编码、可校验的 condition-domain treatment proof**，并把该证明接入 Product Route / Session / Lifecycle 的恢复入口。只要未来的唯一 condition 持久化 owner 提供已解码证明，新进程中的 condition authority 与 item authority 均可重建后完成 `commit only`，不需要依赖旧进程的内存 receipt。

最终结果：

```text
Meridian Shock focused:       17 Success / 0 Fail
Mapped legacy regressions:   352 Success / 0 Fail
Shanmen.0_0_10 full:         730 Success / 0 Fail
Regression coverage:         PASS (Changed=12 / Rules=2 / Required=16 / Logs=8)
Regression gate self-test:   PASS 271/271
Editor + Game Development:   PASS / native status 0
```

## 2. Proof 契约

新增 `Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof`。它只描述 condition domain 已经提交的一次精确治疗，不拥有库存、活跃 condition 或存档权威。

规范正文固定为 13 个 `|` 分隔字段：magic、schema、proof identity、treatment identity、Run/target/timeline/item identities、item/condition definition、treated tick、revision before/after。当前 schema 为 1。

证明的 `ProofId` 由全部正文内容和独立 namespace 确定性派生。解码会严格拒绝：

- 未知 schema 或字段数量错误；
- 非 `Digits` 形式 GUID；
- 非规范整数（包括前导零）；
- 非预期灵脉稳固丹或 Meridian Shock definition；
- revision 非单步递增、TreatmentId 不匹配；
- ProofId 与完整正文重算结果不一致。

因此，损坏、截断、重排或替换字段的 payload 不会进入恢复路径。

## 3. 恢复顺序与权威边界

Product Route 仍以 ShanmenItems 既有 durable processed-request ledger 为唯一库存事实，先定位当前 Run 中唯一、未 finalize 的 Meridian Shock prepare，再选择恢复证明：

```text
durable prepare + exact runtime treatment receipt
  -> reconstruct prepare -> commit only

durable prepare + decoded condition proof + fresh condition authority
  -> restore exact condition history -> reconstruct prepare -> commit only

durable prepare + still-active exact condition revision
  -> reconstruct prepare -> treat -> commit

invalid/conflicting proof or active/non-fresh condition conflict
  -> fail closed；不 commit、不 cancel、不改 item authority
```

全部 supplied proofs 会先做完整性验证。有效但属于其它 TreatmentId 的 proof 被忽略；同一 TreatmentId 的冲突 proof 被拒绝。匹配 proof 还必须与当前 Run correlation、pending item instance 和 definition 完全一致。若运行时 receipt 与 supplied proof 同时存在，两者必须逐字段一致。

Condition component 只允许把 proof 恢复进**刚完成 Begin、仍 inactive、revision/tick/journal 均为空**的 authority；已有 active condition 或任何历史状态都不能被覆盖。精确重复恢复是幂等的，冲突重复恢复失败关闭。

## 4. 没有建立第二份库存或 checkpoint

本轮没有复制 item ledger、没有新增库存字段、没有创建第二套 checkpoint，也没有加入文件 I/O。Recovery proof 是未来 condition-domain 持久化 owner 可原子保存的 canonical value；当前阶段只负责 capture、encode/decode、integrity validation、restore 与产品调用链消费。

因此，本 Report 不声称已经实现无条件零丢失崩溃恢复。若进程在 condition mutation 后、proof 被唯一持久化 owner 保存前崩溃，仍不存在可恢复证明。后续若补持久化，必须明确唯一 owner、原子提交点和 supplied-proof 来源，不能让 item ledger 猜测 condition 结果。

## 5. 新增验证

在既有 14 个 Meridian Shock treatment tests 上增加 3 个 P16.4 场景：

- `Recovery.ProofCodec`：capture/encode/decode round trip，并拒绝未知 schema、非规范 tick 和错误 integrity ID；
- `Recovery.ProcessProof`：治疗后导出 proof，重建 condition authority、重启 item authority，再由新 lifecycle 消费 decoded proof；最终 condition history 恢复且 item 只 commit 一次；
- `Recovery.ProofConflictFence`：旧的有效 treated proof 不能覆盖当前 active condition；item snapshot 前后相等，commit/cancel 均为 0。

关键日志：

```text
process-proof recovered=1 conditionRevision=2 processed=1 commit=1 cancel=0
proof codec schema=1 bytes=335 roundtrip=1 tamperFence=1
active-conflict recovered=0 active=1 unchanged=1 commit=0 cancel=0
```

## 6. Automation 证据

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Meridian Shock focused | 17 | 0 | `0E803295F057967FAD71694A67FE948FE278AFE6422DFB5F27CC50849503191C` |
| `Shanmen.0_0_10` full | 730 | 0 | `640FBCBEA37BB05E4EF6A32879B5D0A47424D6FD0F5736B31BED2067318674B4` |
| CodeB | 60 | 0 | `04AF9447623E6DD49AA40359D89274ED1671FDAA61BC4BEC5ED07A4DA4F041E5` |
| ItemEconomySchema | 24 | 0 | `C7463F337454583677E557937A53F730C0B7202AB30EA45E145EC274C4D51F95` |
| ItemUseAndArmor | 46 | 0 | `8C99A7CF7A66AC604CB9AAE63A25777AAB74E0A73415D6219BB21CF079F9309C` |
| P4 Hotbar | 7 | 0 | `A548FBD4B18E50CCBADD938838BD06FFE2D29C4645BC2DAA205077DFCB159BD8` |
| Profile | 211 | 0 | `5D1683F99632D1D16E751E67927A69792380DB1051A3101D3FDAC75B160B6C96` |
| V3 Attributes | 4 | 0 | `82A13752405E8A1AC193B121403826E9B52D0824FA9D280881B3CF3441066E6B` |

mapped legacy 合计 352/0。Full suite 的首末 Success 时间为 `08:56:27.436 -> 09:24:31.944 UTC`，约 28m04.51s。全部 8 份通过日志均有唯一 native terminal-success marker，Fatal、Unhandled、Ensure 与 `generate_204` 计数均为 0。

## 7. 改动—回归门禁与静态边界

12 个实际改动路径命中 `CombatCondition` 与 `MeridianShockTreatmentProductFlow` 两条规则。16 个必跑组全部由 8 份健康日志覆盖；RecoveryProof 新文件已加入 ProductFlow path mapping，后续修改不会绕过该门禁。

```text
REGRESSION_COVERAGE: PASS Changed=12 Rules=2 Required=16 Logs=8
SELF_TEST: PASS 271/271
JSON_PARSE: PASS Schema=1 Rules=161
BOUNDARY_SCAN: PASS HITS=0
GIT_DIFF_CHECK: PASS
```

流程文件 SHA：gate `F08FF5DD79288D963B9C1D4217C1AE8D9B0EDCE1866ABD96DED3C023B2E98B4C`；self-test `633D910E13342A4270666EB4F949163E3E91F28713E333758CF51D1333BC0EF6`；mapping JSON `CC6EB9EC8C74A3FFEC4F189F9857F654067412187C95E594BC961F948AADB18F`。

边界扫描确认本轮恢复实现不依赖旧 `demo_mapItemSubsystem`、`UWorld`、`AActor`、Spawn/ApplyDamage、RNG、Timer、Enhanced Input、Widget 或 Slate。

## 8. 构建证据

本轮使用 `-NoUBA -MaxParallelActions=1` 控制提交内存压力。首次 Editor 编译暴露 `FString::Join` 对 initializer-list 无法推导 range 的编译错误；改为显式 `TArray<FString>` 后重新编译成功。最终 Game build 逐文件编译本轮实现，Editor target 最终复核为 up to date，两个命令原生退出码均为 0。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Game Development | Succeeded | 34 / 132.90s | `2E2C6281964C86614A32D96C957B281CDA1C2B10F57E5D4210CF9E886DA445A7` |
| Editor Development final check | Succeeded | 0 (up to date) / 1.15s | `51F05E55F01A8C8DAEAB018E659D487EAEA0D33D466693DAA549D2695EE5F9E1` |

最终产物：

- `demo_map.exe`：355,935,232 bytes，SHA-256 `D77BAB50EA5031162DD4E9EECDB84A6419BA07DA6FAF70EDBC58DEDA2A543ED2`；
- `UnrealEditor-demo_map.dll`：14,560,256 bytes，SHA-256 `7B4643637EDDB66E49D184BE7AAE7D7D8467BB4397B47F35B955628610D8B56C`。

## 9. 修改范围

本轮修改 condition component、Meridian Shock Product Route / Session / Lifecycle、对应 Automation tests 与 regression mapping，并新增 recovery proof 的 header/cpp；共 12 个生产、测试及流程文件。没有修改 GameMode、PlayerController、地图、资源、存档 schema、Windows、UE Engine 或用户配置。

raw logs 仅保存在本机 `Saved/Logs`。长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 10. P/F 边界与后续

本 Report 只证明 P 阶段 C++ proof/restore contract、NullRHI 无头 Automation、静态检查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

下一阶段应由唯一 condition storage owner 定义 proof 的原子写入、加载和生命周期清理，并验证真实进程重启后的恢复链；在此之前，P16.4 只保证“可信 proof 可用时”的确定性恢复。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p16-4-meridian-shock-recovery-proof>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-4-meridian-shock-recovery-proof/Docs/Report/Dev.D.UE.0.0.10.P16.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p16-4-meridian-shock-recovery-proof/Docs/Log/Dev.D.UE.0.0.10.P16.4.r0_log.md>
