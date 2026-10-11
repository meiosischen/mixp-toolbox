# 0.1.2 紧凑布局验证

日期：2026-10-11。用户反馈 XP 中底部和右侧内容显示不全。本次缩小默认窗口并加入可调整大小的布局，继续使用真正的 BCB6 工具链，在 macOS Wine 中验证；真实 XP 的显示、字体和 DPI 行为仍需用户复测。

- 对话框模板由 560×383 改为 430×285 对话框单位。Wine 中默认客户区由 980×670 缩为 753×499，完整窗口约 761×533；默认窗口的边界检查按 800×600、预留任务栏空间执行。
- 工具按钮移到标题行，缩短间距和提示文字；列表、编辑区及底部状态集中排列。
- 支持拉伸、最大化，首次显示按当前显示器工作区居中和限制大小。
- 表格六列随可用宽度重新分配，避免最右列被整个隐藏；长菜单名称、路径或命令可通过悬停提示查看。
- 编辑区锚定窗口底部，窗口高度变化主要调整表格高度。注册表位置为可选中复制的只读输入框，完整命令支持自动换行和纵向滚动。
- 状态、路径、参数及环境变量选项增加悬停提示；表格行提示包含完整注册表位置及启动命令。
- 截图入口改为强制重绘后捕获客户区，避免 Wine 原生外框和异步绘制残留影响截图。

| 项目 | 结果 | 证据 |
| --- | --- | --- |
| Release 构建及功能测试 | 通过；9 组注册表测试、10 组 GUI 测试 | [本次构建与测试](verification/0.1.2-test-build.log) |
| Debug 构建 | 通过 | [Debug 日志](verification/0.1.2-debug-build.log) |
| BCB6 `.bpr` 转换及构建 | 通过 | [工程构建日志](verification/0.1.2-bpr-build.log) |
| 默认尺寸与窗口缩放 | 默认窗口小于 800×570；730×510、1060×740 及恢复默认尺寸的边界检查通过 | GUI 测试的 `compact GUI fits 800x600` 记录 |
| 控件及表格边界 | 各尺寸下可见控件位于客户区内；六列总宽不超过列表宽度；下拉列表高度保留 | 同上 |
| 内容保持 | 缩放后命令内容不变，未引入未保存修改 | GUI 创建后缩放断言 |
| 正式 EXE | Unicode 初始化、正常退出通过；未修改真实菜单 | 构建日志最后的启动检查 |
| EXE 格式及依赖 | x86 PE32、Windows GUI、仅导入系统 DLL | [PE 导入记录](verification/0.1.2-pe-imports.log) |
| 界面检查 | 已检查紧凑默认窗口及缩小后的客户区 | [默认](screenshot-0.1.2.png)、[730×510 窗口](screenshot-compact-0.1.2.png)、[新增](screenshot-new-0.1.2.png) |

截图中的中文数据通过原有 Unicode 往返测试；Wine 部分编辑框仍有中文字体回退方框现象。本次未改变中文存储方式，XP 字体显示尚未实际验证。

便携包：`dist/mixp-toolbox-0.1.2-win32.zip`，仅包含 EXE 和 UTF-16 使用说明，EXE 与经过测试的 Release 文件逐字节一致。Release SHA-256：`b33386ec58f7fc8629c1ddf134503b96d163a7ca2078c253ff19572037ae45d2`。编译产物和中间文件继续由 Git 忽略。

XP 复测时先确认页眉显示 0.1.2，在 800×600 桌面检查底部保存按钮、状态及右侧字段，再缩小、放大、最大化窗口。选中菜单后核对完整命令；用鼠标悬停查看长路径和状态。最后按已有步骤新增、修改及删除一个临时菜单。

下拉框高度保留使用 [CB_GETDROPPEDCONTROLRECT](https://learn.microsoft.com/en-us/windows/win32/controls/about-combo-boxes)；完整文本提示使用 [TTN_GETDISPINFO 回调](https://learn.microsoft.com/en-us/windows/desktop/Controls/ttn-getdispinfo)。这些定义已在实际 BCB6 头文件中核对并编译通过。

## 0.1.1 验证记录

日期：2026-10-11。使用真正的 Borland C++Builder 6 工具链，在 macOS Wine 中构建并操作真实窗口；当前仍未在真实 XP 中执行。

本次处理用户在 XP 上报告的“刷新表格为空”和“新增无明显反应”。用户确认点击新增前后，菜单名称及程序路径都可以填写。源码确认 0.1.0 默认只查询 `Software\Classes\*\shell`，该位置可以为空；“新增”复用初始空编辑区，缺少明确模式变化。原版隔离 GUI 测试能成功保存菜单，未在本地复现按钮事件完全失效。修复针对已确认的查询范围和界面反馈问题。

- 启动默认汇总各注册表类型下的 `shell` 菜单，保留 HKCU/HKLM 来源及作用类型；支持按类型搜索。
- 空表和搜索无匹配时，在表格上方显示明确提示。
- 点击新增启用编辑区，显示新增模式，将提交按钮改为“添加菜单”；完整填写后才能添加。
- “全部范围”中的新增自动选择“所有文件”作为目标。
- 保存后清除搜索条件，必要时调整来源筛选，显示并选中保存项。
- 相同内部标识存在于多个文件类型时，编辑后仍选中准确的作用类型。

| 项目 | 结果 | 证据 |
| --- | --- | --- |
| Release 构建 | 通过 | [本次构建及测试日志](verification/0.1.1-test-build.log) |
| Debug 构建 | 通过 | [本次 Debug 日志](verification/0.1.1-debug-build.log) |
| `.bpr` 转换及构建 | BPR2MAK 转换成功，生成的 Makefile 构建通过 | [工程构建日志](verification/0.1.1-bpr-build.log) |
| 注册表集成测试 | 9 组通过 | 本次构建及测试日志 |
| GUI 自动测试 | 9 组通过 | 本次构建及测试日志 |
| 正式 Release EXE | 启动、Unicode 初始化及正常退出通过 | 日志最后的 `shipped Release executable` 记录 |
| EXE 格式及依赖 | x86 PE32、Windows GUI、仅导入 6 个系统 DLL | [本次 PE 导入记录](verification/0.1.1-pe-imports.log) |
| 实际窗口截图 | 检查空表提示、新增模式及各类型菜单显示 | [空表](screenshot-empty.png)、[新增](screenshot-new.png)、[汇总](screenshot-overview.png) |

新增回归场景从空测试注册表启动；先刷新空表，再在外部创建文件夹和 ProgID 菜单，验证“所有文件”为空时刷新仍能查到其他范围。首次点击新增后核对输入框启用、模式提示、按钮文字及注册表尚未写入，再通过真实窗口新增、修改、搜索和删除。另验证不同作用类型的同名标识不会误选，以及来源或搜索过滤不再隐藏新保存的菜单。

测试仍只写入隔离的 `Software\MixpToolboxTests\...` 分支并清理。正式 EXE 启动检查不修改实际 Classes 菜单。

界面页眉和 EXE 版本资源显示 **0.1.1**，便携包为 `dist/mixp-toolbox-0.1.1-win32.zip`。编译目标和中间文件保持在忽略的 `build/`、`dist/` 中。

正式 EXE 的 SHA-256：`b7cb35d43217ef4a43126875e6bbc3060f83bd617064b2628493f9bfda8b9366`。ZIP 中的 EXE 与经过测试的 Release 文件逐字节一致。

XP 上请使用 0.1.1 EXE 复测：先在“全部范围”刷新，再点击新增，确认下方进入新增模式；填写名称、真实程序路径和参数后点击“添加菜单”，确认表格出现新项，并重新打开 Explorer 的右键菜单验证启动。这里只管理静态菜单，COM 动态扩展和系统内置菜单行为不在本次管理范围中。

## 0.1.0 初始版验证记录

日期：2026-10-11。源码和产物位于 `mixp-toolbox` 项目目录。

### 环境和结果

使用 BCB6 安装介质中的真正 Win32 工具，通过 macOS Wine 执行：Borland C++ 5.6、MAKE 5.2、BRCC32 5.40、ILINK32 5.60 和 BPR2MAK。没有用现代编译器替代 BCB6。

| 项目 | 结果 | 证据 |
| --- | --- | --- |
| Release 构建 | 通过 | [Release 与测试日志](verification/test-build.log) |
| Debug 构建 | 通过 | [Debug 日志](verification/debug-build.log) |
| BCB6 `.bpr` 解析及构建 | BPR2MAK 转换成功，生成的 Makefile 构建成功 | [工程构建日志](verification/bpr-build.log) |
| 注册表集成测试 | 8 组通过 | [Release 与测试日志](verification/test-build.log) |
| GUI 自动测试 | 6 组通过 | [Release 与测试日志](verification/test-build.log) |
| 正式 Release EXE | 启动、Unicode 界面初始化及正常退出通过 | 日志最后的 `shipped Release executable` 记录 |
| EXE 格式和依赖 | x86 PE32、Windows GUI、仅导入系统 DLL | [PE 头和导入表](verification/pe-imports.log) |
| 界面布局检查 | 已检查实际窗口客户区；Wine 部分中文输入栏字体回退异常 | [初始版截图](screenshot-0.1.0.png) |

Release 优化时有 BCB6 的 W8027 内联提示，编译和链接没有错误。CRT 静态链接，不需要 `cc3260mt.dll` 或 VCL BPL。

导入的系统 DLL 为 ADVAPI32、KERNEL32、COMCTL32、COMDLG32、SHELL32 和 USER32。使用 XP 时代的 API，递归删除通过 `RegDeleteKeyW` 实现，未导入 Vista 才提供的 `RegDeleteTreeW`。

### 已验证的行为

注册表测试使用真实注册表 API，在 HKCU/HKLM 各自的 `Software\MixpToolboxTests\...` 分支建立临时数据并清理。覆盖：

- 五种作用范围，文件类型的 HKCU 优先和 HKLM 回退，拒绝未注册扩展名。
- 中文名称、中文程序路径及参数完整往返；来源分开列出；重复标识不覆盖。
- 修改状态、Shift 标记和 `REG_EXPAND_SZ` 命令类型。
- 自动备份后调用真实 `regedit /S`，核对恢复后的整棵子树和原快照一致。
- 修改后导入备份能够删除新加的标记；删除后导入恢复整个分支。
- DWORD、含反斜杠和引号的字符串、跨行二进制及 REG_MULTI_SZ 完整恢复。
- 只删除指定来源的菜单分支；外部修改后拒绝过期保存/删除。
- 备份位置不可写时不执行修改或删除。
- DDE、DropTarget、执行处理程序和无 command 的项不作为普通静态命令编辑。
- 拒绝无效标识和空命令；带空格路径正确加引号；无法可靠拆分的旧命令保留完整编辑方式。

GUI 测试通过控件消息操作真实窗口，覆盖启动、中文标签、创建带中文路径和默认参数的项、只改名称时保持命令不变、即时搜索、状态开关、来源选择、确认删除及退出。测试构建复用正式界面代码，只额外替换注册表根路径并提供测试截图入口；正式构建没有这些入口。

最后启动正式 Release EXE，确认初始化和正常退出，未对实际 Classes 菜单配置执行写操作。

Wine 环境补充了中文字体及 Tahoma 字体回退，字体仅用于本地验证，便携包不包含字体。当前 Wine 的部分输入栏中文字仍显示方框；通过 WM_GETTEXT、注册表读取和快照对比已核对中文数据无损，真实 XP 中的字形显示尚未验证。截图来自真实控件，裁去 Wine 未捕获的窗口外框。测试中保存的是命令配置，不会真的运行示例中的 JOE。

### XP 中的验收步骤

本次没有在真实 Windows XP 中运行程序，也没有在 XP 的 BCB6 IDE 界面中实际打开工程。BPR2MAK 解析成功不等同于已经完成这两项验证。建议在你的 XP 虚拟机按以下步骤验收：

1. 打开 `MixpToolbox.bpr`，Build；确认中文界面、全部按钮和下方编辑区正常显示。
2. 在“所有文件 / 当前用户”新增 `Mixp.XpCheck`，指向真实存在的 JOE EXE，参数使用该版本支持的选项并包含 `"%1"`。
3. 右键一个带空格和中文文件名的文件，点击新增菜单；确认文件路径与参数正确传入。
4. 修改菜单名称；切换禁用和仅 Shift 显示；重新打开菜单确认变化。
5. 手动导出、删除、导入该项的备份，确认菜单和命令恢复。
6. 另测文件夹、驱动器和已有文件关联的扩展名；同一 ProgID 的扩展名会共享菜单项。
7. 管理员账户下验证 HKLM；普通账户验证 HKCU，确认无权限时有提示。
8. 完成后删除 `Mixp.XpCheck`，仅处理本次创建的项。

当前不覆盖 COM 动态菜单扩展、XP 文件夹空白处菜单、64 位 XP 注册表双视图、不同 DPI 的实际布局，以及 Explorer 多选文件的调用行为。
