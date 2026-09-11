# Dev.D.UE.0.0.10.P26.5.r0 Development Log

## 1. 目标

- 让玩家在统一物品详情中读到已投放的防具抗性；
- 复用 P26.3 canonical catalog，不新增或推断平衡值；
- 保持备战条目与局内物品实例显示一致；
- 用未配置抗性的 TrainingVest 证明 no-op 边界；
- 按改动文件推导回归，并完成 Report/Log GitHub 交接。

## 2. 基线与范围

- 基线：`25b05f4d44cbf3392918243a4789b640d04ca7fa`（P26.4）；
- 分支：`agent/0.0.10-p26-5-armor-resistance-item-detail`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不改 item catalog、CombatCore、生命/物品权威、存档 schema、地图或资产；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. 实现

`demo_mapItemPresentation.cpp` 新增纯格式化辅助：

1. 将 Physical、Physical.Slash、Spirit、Mental 映射为稳定中文抗性名；
2. 其它有效通道保留完整 GameplayTag 后追加 `抗性`；
3. 把抗性 fraction 转为百分比，整数省略小数，非整数保留一位；
4. 保留既有 modifier 数量，与所有有效 `DamageResistances` 按 catalog 顺序组合；
5. 空 modifier 且空抗性时仍返回既有占位符 `—`。

`BuildDetail()` 只把原 modifier-count 分支替换为该摘要生成器，没有新增状态或写操作。

## 4. 测试

新增 `ArmorResistanceItemDetail.CanonicalAndControl`：

- 使用备战 stash row 读取一阶道袍，断言 `物理抗性 10%`；
- 使用 TrainingVest，断言有效详情中不含 `抗性`；
- 通过真实 `Fdemo_mapItemAuthority` 创建局内一阶道袍，断言与备战详情完全相同。

## 5. 首次失败与修复

首次专项运行完成 1 项并失败。测试错误地把一阶道袍的两个 `EffectParameters` 当成两个
`Modifiers`，因此期待了不存在的 modifier 数量。检查 canonical definition 后只修正测试期望；
生产实现未改。

- 首次失败：0 Success / 1 Fail；SHA-256
  `CF8B6654E769E653D460A18E4DAC69878A48CD95493EA1836D2134637CBEFA87`；
- 最终专项：1 Success / 0 Fail；SHA-256
  `B01FDFA563B321321B3D2E2DF76BD55880CC0022938A452DB648BE30847BC9E7`。

## 6. 自动化结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10` | 1,321 | 0 | `F371631760A07ED120A99034BA33DBC5D2EA258B7945BF754A06C7EC9A5FE630` |
| `Shanmen.0_0_10.Items` | 77 | 0 | `D96C8E35E6ADD79504010BD5604407885540D9ACA305D9F288054E4103B6B84A` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `3884CA8E379EDFFA96EEE96C8FB323FEEDB2719081350064F37121170390F18D` |
| `demo_map.P4.Hotbar` | 7 | 0 | `CD0B4D554E7C87ACCFC690616FAD40B60C20234580DB8101762354BB9B23B65F` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `65FA5EAD26EC015E882F98CA53A94704AB5551F134BCBA3EAA25CC015B5CDBDB` |
| `demo_map.Profile` | 211 | 0 | `7A32FDC48894109EC70D79B1DA0008A0A56725EA596B4AC14C8D6E9C7F7B4DED` |
| `demo_map.CodeB` | 60 | 0 | `4EA2F9C933F2C67019FAEDCE5F65645AAB538FA69EC5781E75A8668E515241DF` |
| `demo_map.V3.WorldInteraction` | 4 | 0 | `D41D40F7D7D4C29022BD5B88B0A08CC9C24643750C7ECA3E8842960FF480EC42` |

每份最终日志均包含唯一 RunTests 命令、至少一个 Success、0 Fail、0 Fatal、原生队列完成标记，
且进程退出码为 0。

## 7. 改动驱动覆盖

`demo_mapItemPresentation.cpp` 与 `demo_mapItemTests.cpp` 命中两条映射规则，要求 7 个测试组。
最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=2 Rules=2 Required=7 Logs=8`。

- 覆盖日志 SHA-256：
  `07C325C74CF1B50084EF6C3223565AFDF080340B3B3C013B927A17F2565D4E00`；
- 映射器自测：`457/457` PASS；SHA-256
  `8C81F7F329B2AD5EFF7334A56D12CFDA10828E94009F32C75B6E1E521AD9EBEF`。

## 8. 构建与静态检查

| Target | Result | Duration | SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 20.06s | `E1F4210254369FA80F044932D9A196B70918B0E5CEB7EAABAC6A17FC709528C6` |
| `demo_mapEditor` Win64 Development | Succeeded / 0 | 0.94s | `597C977658C25508FB7DDE978CE308CA20B1EC832BBC3BA2EA92DFF936D16285` |

最终二进制：

- `demo_map.exe`：359,986,176 bytes；SHA-256
  `E35384DAD1B692312A68FB9C07516EBD0A76443A48BD86A400EED3CA40679035`；
- `UnrealEditor-demo_map.dll`：19,306,496 bytes；SHA-256
  `434864BDD5E4FF280CEB62BBB9104046FE3B4BF7FF5222498F7ED7DA18D52FEE`。

`git diff --check` 与 regression map JSON 解析 PASS。新增运行时代码的 added-line scan 未发现
Tick、Timer、RNG、ApplyDamage、SpawnActor、NewObject、存档或库存写入。

## 9. 未执行项

没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
因此本轮只声明 P 阶段数据与契约成立，不声明四个现有 UI 表面的像素级可读性已经 F 验收。
原始证据位于 `Saved/Codex/P26.5`，不进入 Git。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapItemPresentation.cpp`
2. `Source/demo_map/demo_mapItemTests.cpp`
3. `Docs/Report/Dev.D.UE.0.0.10.P26.5.r0_report.md`
4. `Docs/Log/Dev.D.UE.0.0.10.P26.5.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-5-armor-resistance-item-detail>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-5-armor-resistance-item-detail/Docs/Report/Dev.D.UE.0.0.10.P26.5.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-5-armor-resistance-item-detail/Docs/Log/Dev.D.UE.0.0.10.P26.5.r0_log.md>
