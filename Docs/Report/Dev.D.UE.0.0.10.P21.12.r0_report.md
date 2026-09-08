# Dev.D.UE.0.0.10.P21.12.r0 Report

## 1. 结论

P21.12 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P21.11 已可操作的飞剑输入与既有飞剑状态/威胁 HUD 合并为一条玩家可读反馈。飞剑环绕、定向、返航与归位时，原有状态和近身目标数量继续显示；可执行动作现在按当前阶段显示实际按键，而且直接读取统一输入设置，因此玩家重绑定后无需维护第二份默认键文案。

```text
Controlled-weapon threat focused:             8 Success / 0 Fail
Shanmen.0_0_10 full:                       1246 Success / 0 Fail
Required 0.0.9B compatibility groups:       123 Success / 0 Fail
Regression coverage:                         PASS (Changed=4 / Rules=2 / Required=16 / Logs=3)
Regression gate self-test:                   PASS 437/437
Game + Editor Development:                   PASS / native status 0
```

本轮未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也未执行真实键鼠、截图、Smoke、Cook 或 Package。因此证明的是状态驱动文案、实时键位来源、HUD 接线、失败关闭、回归兼容与编译闭合；不宣称实际分辨率下的可读性、视觉美术质量或玩家手感已经人工验收。

## 2. 玩家可见结果

既有飞剑 HUD 现在按阶段形成以下单行反馈：

- `Orbiting`：`飞剑 · 环绕待命 · [当前发射键] 发射`；
- `Directed`：`飞剑 · 御剑出击 · [当前改向键] 改向 · [当前发射/召回键] 召回`；
- `Returning`：`飞剑 · 返航`，不提示当前不可执行的操作；
- `Redeployed`：`飞剑 · 已归位 · [当前发射键] 再次出击`。

有近身目标时，原有 `近身目标 N` 继续位于同一行；武器投影在视口外时，原有 `屏外` 标记继续保留。默认 X/C 只是输入设置当前值，HUD 不缓存这两个字符。

## 3. 实时键位与单一来源

`Ademo_mapHUD` 在生成飞剑 readout plan 时，从现有 `Fdemo_mapInputBindingSettings` 读取：

- `ControlledWeaponLaunchRecall` 的当前显示名；
- `ControlledWeaponRedirect` 的当前显示名。

两个值连同 Actor 已公开的 flight phase、threat contact count 与投影结果传给既有纯表现 planner。没有新增输入注册表、配置缓存、飞剑查询器或 HUD 状态副本；下一帧绘制自然反映设置中的当前绑定。

## 4. 阶段约束与失败关闭

输入提示完全由已有 flight phase 派生，不改变任何产品状态：

- Returning 明确返回空提示，避免宣传不可执行命令；
- 空键名、首尾空白以及 CR/LF/Tab 键名均拒绝生成 plan；
- 失败时先清空复用的输出，避免上一帧有效文本残留；
- `IsValid()` 依据阶段、键名、接触数、位置与样式重新推导文本；
- `Matches()` 把两个键名纳入确定性比较，重绑定后的 plan 不会与旧 plan 误判相同。

为了容纳 Directed 状态下的双动作提示，默认 panel 宽度由 230 调整为 420，文字缩放由 0.82 调整为 0.72。仍使用同一个世界跟随/视口回退 panel，没有创建新面板或并行布局系统。

## 5. 产品自动化

focused 组 `Shanmen.0_0_10.Product.ControlledWeaponThreat` 执行 8 项，全部成功。新增专项用例验证：

- Directed 同时显示改向与召回；
- X/C 改为 V/Z 后，文本精确变为 `[Z] 改向 · [V] 召回`；
- 新旧键位 plan 不匹配，证明键位参与身份比较；
- Returning 文本不包含任何方括号动作提示；
- 空键名与多行键名失败关闭，复用输出被清空。

全量 `Shanmen.0_0_10` 从 P21.11 的 1245 项增加到 1246 项，最终 1246/0。日志时钟从 `2026.09.08-12.55.52:652` 到 `2026.09.08-14.10.51:098` 自然清空；有一个实际 RunTests 命令、一个成功终止标记，原生退出码为 0。

## 6. 自动化、门禁与构建证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Controlled-weapon threat focused | 8 | 0 | 270,736 | `D00F34CB9F1775706EDB232282941CA6BE2BF4F44FA9A1A571F3DC84D857F689` |
| `Shanmen.0_0_10` full | 1246 | 0 | 1,906,426 | `F113E29827F3E20F40E32E7115D1B1503BFC61EE115E7E869686423320C17716` |
| `demo_map.InputRestore` | 101 | 0 | 394,907 | `486E1C501537CBAD4DCCFFD580EA7909F8F3EEE5819223539A9A68246E691DAC` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 285,294 | `178DBB54F2D9B64791764B99F4569241392EEE3EE93BCBCDDE1A5808F5E59E4B` |

四份自动化日志各有一个实际 RunTests 命令和一个成功终止标记；Fail、Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

首轮 changed-file gate 只提供 full 日志时按预期失败，准确指出 HUD 改动还要求 `demo_map.InputRestore` 和 `demo_map.V2RangedCompatibility`。补跑后最终为 `PASS Changed=4 Rules=2 Required=16 Logs=3`。

- gate initial failure：861 bytes / `CA6D29B9BAF8C3036CDC7DBCEEE96A4C96DDFF487D0FF193319866F7E3E95215`；
- gate final：2,433 bytes / `BA7ABE628377C66E82C12AAB34F424BCF30569A2E04A63F5F5577BAF93ACD13A`；
- gate self-test summary：`PASS 437/437`，218 bytes / `78C2463FEE1B3C894E6CD784F8377FFD35D85091B2A37B5FA50719518F8B426C`。

| Target | Result | Actions / Time | Bytes | Log SHA-256 |
|---|---|---|---:|---|
| Editor initial | Succeeded / native 0 | 6 / 28.26s | 2,554 | `426EB2451624BFDD0B57E96D50F7255B0DCB8D15AFC17FB99CE697CC1FD85F1B` |
| Game Development final | Succeeded / native 0 | 5 / 29.02s | 2,350 | `8398ECB5F148901349C82CB82BE1BC532CF28A07F4E61FBA04F4FD8C921A7A3A` |
| Editor Development final | Succeeded / native 0 | 0 / 1.07s | 1,021 | `498339F9ABEB88543CB6911810F21219014E3E48FD4C996AE8007DD9FD7E0BF1` |

最终产物：

- `demo_map.exe`：359,515,648 bytes，SHA-256 `A51BEE178B494799C628810C3EBD99B2973AAC9E4CB71582F328C0465E9E96EB`；
- `UnrealEditor-demo_map.dll`：18,702,848 bytes，SHA-256 `E783E40D6C1C1684212172829D863B0CEC73E4EBADE88B5095EAFF8033FC2B99`。

## 7. 静态边界

- 实现/测试 diff：4 files，`+215 / -21`；
- 87 条新增生产行中的 Timer、`SetTimer`、RNG、`ApplyDamage`、`SpawnActor`、`Destroy` 命中 0；
- 未修改输入注册、输入迁移、PlayerController、GameMode、Run Host、飞剑 Actor、移动、伤害、库存或存档 schema；
- `git diff --check` native 0，仅有工作树 LF→CRLF 提示；
- 验证结束后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0；
- 103 个用户原有 untracked 文件保持未暂存，raw evidence 位于 ignored `Saved/Codex/P21.12`。

## 8. P/F 边界与后续方向

PASS：玩家能从既有飞剑状态行看到当前阶段真正可执行的操作；提示使用实时重绑定键位；Returning 不显示不可用动作；威胁计数、世界跟随与屏外回退行为保持；focused、full、旧版兼容、覆盖门与双目标构建全部通过。

未声明：真实屏幕上的中文排版、超长本地化键名、不同 DPI/分辨率、玩家手感、动画/音效/VFX、联机同步、PIE/Standalone 或打包体验已经人工验收。

下一轮应继续增加同一条飞剑玩家闭环的可观察决策价值，优先考虑不建立新 owner 的目标选择或命中后反馈；若要确认本轮视觉尺寸与可读性，则需要另行授权一次 Editor/PIE 或产品级真实画面验收。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-12-flying-sword-context-hints>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-12-flying-sword-context-hints/Docs/Report/Dev.D.UE.0.0.10.P21.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-12-flying-sword-context-hints/Docs/Log/Dev.D.UE.0.0.10.P21.12.r0_log.md>
