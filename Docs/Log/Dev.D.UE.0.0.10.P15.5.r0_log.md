# Dev.D.UE.0.0.10.P15.5.r0 Development Log

## 1. 目标

收敛 legacy `InputRestore` 的三项陈旧 source/artifact 断言；保持产品输入 gate 与 terminal marker 契约不降级，并为修改文件建立精确回归证据映射。

## 2. 基线与根因

- focused baseline：`98 Success / 3 Fail / 101 Total`，SHA `2083E1DD36B42295C6F03531D5FECD3A628488C76DA72D179D479EBA0C161BEC`；
- case 26 查找已删除的 `UseHotbarSlot(Intent, IsGameplayInputAllowed())` 调用形式，但当前 gate 已内聚到 hotbar、bound attack 和 direct attack 入口；
- case 100/101 读取不存在、未版本化的 0.0.5 `Saved/Automation/.../GenerateP818Plans.ps1`，并断言旧任务号、slot count 与脚本文本；
- 两类失败都是 fixture 证据陈旧，生产 gate 与当前 terminal emitter 契约仍然存在。

## 3. 实现

- case 26 改为函数块局部顺序检查：gate 必须先于 thrown/quick-slot route、ground-circle/attack route 与 cooldown mutation；
- case 100 用两个独立 emitter 实例证明同输入得到完全相同的 canonical terminal line；
- case 101 直接证明专用 `INPUT_RESTORE_TERMINAL_STATE` 不依赖通用 `INPUT_CONTEXT_TRANSITION`；
- 移除 `demo_mapInputRestoreTests.cpp` 对旧 Saved 生成器的依赖；
- 新增 `demo_mapInputRestoreTests.cpp -> demo_map.InputRestore` exact mapping，并补正反 self-test。

## 4. 验证

```text
InputRestore: 98/3 -> 101/0
legacy demo_map: 1318/12 -> 1321/9, Total=1330
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 256/256
JSON_PARSE: PASS Rules=153
git diff --check: PASS
```

| Evidence | Result | SHA-256 |
|---|---|---|
| focused baseline | 98 Success / 3 Fail | `2083E1DD36B42295C6F03531D5FECD3A628488C76DA72D179D479EBA0C161BEC` |
| focused after | 101 Success / 0 Fail | `416FC5A5F640318E626AE093BE5226EB4676B11DD1337552A15F5F4F0A543D11` |
| legacy after | 1321 Success / 9 Fail | `C67CCCA31E0EC62C582CB6B7F29DDCA9615A35C679C05B386DF0D83D805B5453` |
| full 0.0.10 | 712 Success / 0 Fail | `F3B4C392B57A03C2120090E8DB04636545339DBC6ED0D21AF77BC6FCD50F9FFA` |

流程日志 SHA：gate `55D5736EB7349DA49051ADFD13982FE8F8443114F35F2E0D885FA9D58D6FC845`；self-test `2766FC234CD0FF722B1E41F1B878F30B564706E140F5279932BA4DFE62FD9BD2`。

## 5. 构建

- initial Editor：4 actions / 24.51s / succeeded；
- final Game：3 actions / 23.72s / status 0 / log SHA `1A4910E6548417E188652E2E71DBE94E615AA9CC931521FFF27F71F133B5C01C`；
- final Editor：up to date / 0 actions / 0.94s / status 0 / log SHA `019B490E2DF87E15469D94D5B4AE86CEE3B388FDDD7A89BD6EA56210590C9B80`；
- Game EXE：355,744,768 bytes / SHA `4B6B5232689DFEEC910DDBB01D49304EF24DF93153782770E9D6CB7DA3A9B1D5`；
- Editor DLL：14,330,368 bytes / SHA `5565C90671A222D7750783BFB0719FC8085A5987966CC44F772ED1102C830230`。

## 6. 剩余失败与范围

legacy 父组剩余 9 项：RootBoundary 1、EnemyRouteLoot 1、P1 SectNavigation 1、P5RuntimeInterface 1、P6 4、V3 Lifecycle 1。它们未被跳过或归入本阶段。

本轮仅 P 阶段 fixture、回归门禁、NullRHI Automation 与 Development build；production C++ 零修改。未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户文件未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-5-input-restore-contract-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-5-input-restore-contract-regression/Docs/Report/Dev.D.UE.0.0.10.P15.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-5-input-restore-contract-regression/Docs/Log/Dev.D.UE.0.0.10.P15.5.r0_log.md>
