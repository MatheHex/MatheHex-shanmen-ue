# Dev.D.UE.0.0.10.P15.8.r0 Development Log

## 1. 目标

收敛 P5 Runtime Interface 与 P6 Dual Loot 中三项同源的旧四装备角色 fixture，并为两个测试文件建立精确的改动—回归映射。

## 2. 基线与根因

- P5 baseline：`8 Success / 1 Fail / 9 Total`，SHA `FE06BC565899DEBA8593FFD7BA3FAD2CA28D9E22386EEDF24474918565B9765A`；
- P6 Dual Loot baseline：`6 Success / 2 Fail / 8 Total`，SHA `0E3E2F809D8C787D836FB2A360413D305739C6CD6657A9333685AA6F775B58FA`；
- canonical equipment roles 已为 Weapon、Armor、Accessory、SpatialRing、Backpack 五项；
- P5 仍固定断言 4 cells；P6 case 03 把 Backpack 写入 SpatialRing 的 slot 3；case 04 缺真实 Accessory，并只接受 slot 0..3；
- production authority 与 P15.7 已验证的五角色契约一致，因此只修 fixture。

## 3. 实现

- P5 Equipment cell count 改为读取 canonical role count；
- P6 case 03 从 role catalog 派生 Backpack slot，从 Definition 派生空间容量，并验证动态区域起点；
- P6 case 04 分别覆盖 Accessory、SpatialRing、Backpack，遍历验证每个 canonical slot；
- 保留 duplicate Weapon 降级为普通携带物的原契约；
- 为 P5 Runtime Interface 与 P6 Dual Loot 新增 exact mapping，并从两个宽泛映射中移除 fixture；
- 新增四项门禁自检：两项成功覆盖、两项不相关证据失败关闭；
- production C++ 零修改。

## 4. 验证

```text
P5 Runtime Interface: 8/1 -> 9/0
P6 Dual Loot:         6/2 -> 8/0
legacy demo_map:      1323/7 -> 1326/4, Total=1330
Shanmen.0_0_10:       712/0
REGRESSION_COVERAGE: PASS Changed=4 Rules=2 Required=2 Logs=2
SELF_TEST: PASS 262/262
JSON_PARSE: PASS Rules=156
git diff --check: PASS
```

| Evidence | Result | SHA-256 |
|---|---|---|
| P5 baseline | 8 Success / 1 Fail | `FE06BC565899DEBA8593FFD7BA3FAD2CA28D9E22386EEDF24474918565B9765A` |
| P5 after | 9 Success / 0 Fail | `88D329C5FBA0DDCC9505CF618111174D3704F2D6F32861979FE27FAE507F4481` |
| P6 baseline | 6 Success / 2 Fail | `0E3E2F809D8C787D836FB2A360413D305739C6CD6657A9333685AA6F775B58FA` |
| P6 after | 8 Success / 0 Fail | `B102DD89922FB601FF1CB0C6D615047A882026EC2EF89E10DB156BF0C98866AF` |
| legacy after | 1326 Success / 4 Fail | `0A44835AE3380FA7918E49EA8FD0E593F3DBC26B55FAFAC64639817409A3B7D4` |
| full 0.0.10 | 712 Success / 0 Fail | `9D745573A66B766C0AA242C937D1461DCF5DE99AC49C09372A2008DA355DB8E4` |

流程日志 SHA：gate `EBFA2F36260B5A0FD6020C6F4296D651274C5348092614562194CFA284152085`；self-test `D840E610D11FDC94A58F03CEE9BC5FD3DC05C9A895B6ABD2DE0058C062035ECD`；JSON parse `C0E104D261CD67449699034727FF72657D928A0950E8FF461A2634F6A3C7C3F9`。

## 5. 构建

- initial Editor：5 actions / 26.83s / status 0 / SHA `ECF811E45489ADAC88F70E3DB193A85D1A44718B93635A88E15B70373B036C48`；
- first Game attempt：主机提交内存超阈值，UBA 产生 286 次 low-memory kill；有界中止，保留日志 SHA `973CB620A60F35C7F0E36C4497D66C303B71E52B8EAFF79ADBFEFD5404946273`；
- recovered Game：`-NoUBA -MaxParallelActions=1`，4 actions / 32.03s / status 0 / SHA `41DD989A9BCB78457395E23DF6B1FC60A1394FCEA48F87E0DD35B0B5A9D88402`；
- final Editor：up to date / 0 actions / 1.13s / status 0 / SHA `C82A66F8CACBC28A2348CC3A6A35A9FF2C5F4946B605F3993CC914790A9E192F`；
- Game EXE：355,743,744 bytes / SHA `0FAAFC6469A7329234B38ABB73E4D0FA44309E8E5B90C804C314A43DE8147281`；
- Editor DLL：14,330,368 bytes / SHA `2BFF817E3C952F650D206D325031D1DC29179C03A057F91740AA6C842555D2D8`。

## 6. 剩余失败与范围

legacy 父组剩余 4 项：P1 SectNavigation 1、P6 Integration 2、V3 Lifecycle 1。它们未被跳过或归入本阶段。

本轮仅 P 阶段 fixture、回归门禁、NullRHI Automation、静态检查与 Development build；未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户文件未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-8-equipment-role-contract-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-8-equipment-role-contract-regression/Docs/Report/Dev.D.UE.0.0.10.P15.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-8-equipment-role-contract-regression/Docs/Log/Dev.D.UE.0.0.10.P15.8.r0_log.md>
