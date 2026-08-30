# Dev.D.UE.0.0.10.P9.2.r0 Development Log

## 目标

把 P9.0/P9.1 留给产品 owner 的 duration 边界实现为独立、确定、可重放的 external deadline gate，同时不在 CombatRuntime 内引入时钟、Timer、输入、能量或第二套 shield lifecycle。

## 设计决策

1. deadline contract 从 activation receipt、TimelineId、StartTick、DeadlineTick 确定性派生；
2. tick 单位与推进由调用方拥有，CombatRuntime 只验证 identity 与 monotonic boundary；
3. observation 是显式输入，不在 gate 内轮询或读取 World time；
4. `DurationElapsed` 从公开任意 reason seam 中移除，只能由绑定 gate 写入；
5. gate 复用 P9.0 deactivation receipt，不复制 lifecycle；
6. exact terminal observation 可重放，不同 terminal observation 不可改写历史；
7. deadline 只结束 projection eligibility，不修改 P9.1 capacity 数值；
8. 产品源码没有 SpiritShield call site 或 typed spirit-energy authority，因此不伪造产品 adapter。

## 执行序列

1. 审查 P9.0 runtime、P9.1 capacity authority、产品输入/属性/资源写 seam 与全部 SpiritShield 引用。
2. 确认产品层除测试字符串外没有 SpiritShield 调用点，也没有可提交的灵力/法力权威。
3. 冻结 contract、observation、elapsed receipt、structured result 与 gate 接口。
4. 实现 deterministic IDs、early/foreign/conflict 拒绝、exact replay 与 lifecycle interlock。
5. 收紧原 runtime：公开 `TryDeactivate(DurationElapsed)` 失败关闭；private friend seam 复用既有内部 deactivation。
6. 增加 6 个 focused deadline tests，并更新既有 explicit deactivation test。
7. 增加 `SpiritShieldDeadlineGate` changed-path 规则；mapping 从 `71` 增至 `72`，self-test 从 `104` 增至 `106`。
8. 首次 Editor candidate：生产 gate 编译成功，test 因缺少 `ShanmenCombatResolver.h` 报 C2653/C3861；保留失败日志。
9. 加入直接 include 后 Editor recovery 成功。
10. 收紧 DurationElapsed 唯一入口后完成 Editor rebuild；最终 Editor 检查为 up-to-date success。
11. 最终 Automation：Deadline `6/6`、Capacity `6/6`、SpiritShield `17/17`、CombatCore `9/9`、CombatRuntime `51/51`、full `354/354`。
12. changed-file gate `Changed=8 / Rules=3 / Required=5 / Logs=6` 通过；边界扫描与 `git diff --check` 通过。
13. Game final 9 actions，原生退出 `0`。
14. 生成同名 Report/Log，执行 exact-stage、cached diff、commit、push 与远端 SHA 核验。

## 核心状态迁移

```text
ActivationReceipt + TimelineId + StartTick + DeadlineTick
  -> immutable DeadlineContract
  -> explicit TimelineObservation
       foreign timeline => rejected; no mutation
       observed < due   => rejected; shield remains Active
       observed >= due  => private deadline deactivation
                            + immutable ElapsedReceipt
       exact replay     => original receipt
       different replay => DeadlineConflict

Public TryDeactivate(DurationElapsed)
  -> rejected (deadline proof required)
```

## Automation 证据

| Log | Success | Fail | Queue | Fatal | SHA-256 |
|---|---:|---:|---:|---:|---|
| `SpiritShieldDeadline-final.log` | 6 | 0 | yes | 0 | `02D72B6276727C833B779F9A66857180161FAF2F13C03A747AE9AA4F5EAD0B4A` |
| `SpiritShieldCapacity-final.log` | 6 | 0 | yes | 0 | `96C0276ADEA42885E121F67804BD87D165C3265557182EB6EE2E388FFC1774E7` |
| `SpiritShield-final.log` | 17 | 0 | yes | 0 | `94CCDC6999C40DB2705DB4BDC2C01FA0F98486617954105B2045352337F5B504` |
| `CombatCore-final.log` | 9 | 0 | yes | 0 | `4738AD553FF87A8DC8852970087A57B04620897D19149948667D213AAE724F7E` |
| `CombatRuntime-final.log` | 51 | 0 | yes | 0 | `F539D1DB8461CE3F2787FAB08C5313A2866E65CD5F0346465650786023299493` |
| `Shanmen-0_0_10-final.log` | 354 | 0 | yes | 0 | `CB758E122EBFE3DFAF50FE50CF001961218D2215CAB6F5E3E7BCB4E29BACDEA1` |

## 门禁与静态结果

```text
REGRESSION_MAP_JSON: PASS Rules=72
SELF_TEST: PASS 106/106
REGRESSION_COVERAGE (implementation): PASS Changed=8 Rules=3 Required=5 Logs=6
REGRESSION_COVERAGE (exact stage): PASS Changed=10 Rules=3 Required=5 Logs=6
git diff --check: PASS
BOUNDARY_SCAN_MATCHES=0
```

- required groups：Deadline、Capacity、SpiritShield、CombatRuntime、CombatCore；
- mapping SHA-256：`328E90668CBE6AB33FFBE8AE242DD0F9AC7A166ADB284F50A659F4052BBA1F13`；
- self-test SHA-256：`62171B0EA1B8D01EA4CDF55B48B5637229F5A04009119E7B2DB280C4AC523969`；
- code/scripts：`8 files / +904 / -4`。

## 构建证据

| Build | Actions | Time | Exit | SHA-256 |
|---|---:|---:|---|---|
| Editor candidate | 6 | 34.00s | UBT 6; runner 1 | `97BFB2BA07820DA047BEB4B60B7CB6FAD028037B3714FBFC8F8981C09B067138` |
| Editor recovery | 4 | 4.55s | 0 | `1BC973544DBE9018EBE550F3039A246198394D763F3211D86D98FC60421A5AE7` |
| Editor final | 0 | 0.91s | 0 | `BB7C3599B353A4A99B914956235823022A682152EB2BDA740DC402E0ADA927F4` |
| Game final | 9 | 35.78s | 0 | `625204CED916B11DD36BAE063A7198436FFC6E7981B37631B8165CB5AE6888D9` |

## 真实异常与修复

首次失败是新增测试编译单元缺少 `FShanmenCombatIdFactory` 的直接声明头文件，产生 C2653/C3861；生产 deadline gate 已在该次构建成功编译。加入 `ShanmenCombatResolver.h` 后相同标准 Editor 构建成功，后续 Game 与全部 Automation 成功。该异常未描述为环境或内存故障。

## P/F 边界

仅执行 P 阶段纯值实现、无头测试、静态/门禁和 Editor/Game Development 构建。没有启动 UI、PIE、Standalone、产品 exe、真实输入、Smoke、Cook 或 Package。
