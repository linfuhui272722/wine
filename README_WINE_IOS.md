# Wine iOS - Windows Compatibility Layer for iOS

## 概述

Wine iOS 是一个完整的项目，用于在 iOS 设备上运行 Windows 可执行文件 (.exe)。该项目包含：

- **Wine iOS Core** - Wine 兼容层核心实现
- **iOS App** - 原生 iOS UI 应用
- **PE Loader** - Windows PE/COFF 可执行文件加载器
- **Graphics Layer** - 图形渲染层

## 项目结构

```
wine/
├── dlls/wineios.drv/
│   ├── wineios/                    # Wine iOS 核心实现
│   │   ├── wineios.c              - 主入口
│   │   ├── ios_syscalls.c         - 系统调用
│   │   ├── ios_graphics.c         - 图形渲染
│   │   └── ios_pe.c               - PE 加载器
│   │
│   └── ios-app/                   # iOS 应用
│       ├── main.m                 - 主入口
│       ├── WineAppDelegate.m      - 应用代理
│       ├── WineMenuViewController.m - 菜单视图
│       ├── WineViewController.m   - 图形视图
│       ├── WineBridge.m           - ObjC-C 桥接
│       ├── WineEventQueue.m       - 事件队列
│       ├── WineJIT.m              - JIT 支持
│       ├── Info.plist
│       ├── LaunchScreen.storyboard
│       └── Wine.xcodeproj/
│
├── build-wine-ios/                # 预编译的 Mach-O ARM64 文件
│   ├── objects/                   - 目标文件
│   ├── lib/                      - 静态库
│   └── include/                  - 头文件
│
├── build_linux.sh                 # Linux 交叉编译脚本
├── build_macos.sh                 # macOS 构建脚本
└── build_complete_wine_ios.sh    # 完整 macOS 构建
```

## 构建说明

### 方式一：在 macOS 上完整构建（推荐）

这是生成可运行 IPA 的推荐方式。

```bash
# 克隆仓库
git clone https://github.com/linfuhui272722/wine.git
cd wine

# 给予执行权限
chmod +x build_complete_wine_ios.sh

# 运行完整构建
./build_complete_wine_ios.sh all
```

这将：
1. 创建 Wine API 头文件
2. 编译 Wine DLL（kernel32, user32, gdi32 等）
3. 编译 Wine iOS 核心库
4. 编译 iOS App
5. 使用 Xcode 链接所有文件
6. 生成 `Wine.ipa`

### 方式二：Linux 交叉编译 + macOS 链接

```bash
# 1. 在 Linux 上交叉编译
./build_linux.sh build

# 2. 将 build-wine-ios 目录复制到 Mac
scp -r build-wine-ios user@mac:/path/to/wine/

# 3. 在 Mac 上完成链接
./build_macos.sh all
```

### 方式三：使用预编译文件

仓库中已包含预编译的 Mach-O ARM64 目标文件：

```bash
# 直接在 Mac 上链接
./build_macos.sh build
```

## 安装 IPA

1. 将 iOS 设备连接到 Mac
2. 使用 Xcode 打开 `Wine.xcodeproj`
3. 选择目标设备和配置
4. 点击 Run 或 Archive

或者使用以下命令安装：

```bash
# 转换为已签名的 IPA（需要开发者账号）
xcodebuild -exportArchive \
    -archivePath build/Release-iphoneos/Wine.xcarchive \
    -exportOptionsPlist exportOptions.plist \
    -exportPath ./SignedIPA

# 使用 ios-deploy 安装（需要越狱设备）
ios-deploy -b Wine.ipa
```

## 使用说明

### 运行 EXE 文件

1. 打开 Wine iOS 应用
2. 点击 "Open EXE File"
3. 从文件选择器中选择 .exe 文件
4. 应用会自动加载并运行

### 注意事项

- 应用会先将 EXE 文件复制到应用的文档目录
- Wine 前缀（C: 驱动器）位于应用沙盒内
- 可以通过 Files 应用访问和管理文件

## 技术实现

### PE/EXE 加载

```c
// ios_pe.c 实现了：
- DOS 头部解析
- PE 头部解析
- Section 映射
- 导入表加载
- 重定位处理
```

### Windows API 实现

```c
// 核心 Windows API：
- VirtualAlloc/VirtualFree   // 内存管理
- CreateFile/ReadFile       // 文件操作
- CreateThread/ExitThread    // 线程管理
- LoadLibrary/GetProcAddress // 模块加载
- MessageBox                // 用户界面
```

### iOS 集成

```objc
// WineBridge.m 提供了：
- ObjC 到 C 的桥接
- UIKit 事件传递到 Wine
- 文件访问和沙盒管理
```

## 系统要求

- **macOS**: 12.0+
- **Xcode**: 14.0+
- **iOS**: 13.0+
- **设备**: ARM64 (iPhone 6s 及以上)

## 构建产物大小

| 组件 | 大小 |
|------|------|
| libwineios.a | ~40KB |
| libwineapp.a | ~260KB |
| iOS App (链接后) | ~1-2MB |
| IPA 包 | ~2-3MB |

## 已知限制

1. **图形性能**: 使用 OpenGL ES，效率不如 Metal
2. **不支持 DirectX**: 需要额外的图形层
3. **部分 API 未实现**: 某些高级 Windows API 可能不可用
4. **性能**: 与原生 iOS 应用相比有性能损失

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

Wine iOS 基于 Wine 项目，采用 MIT 许可证。

## 参考资料

- [Wine 官方网站](https://www.winehq.org/)
- [iOS App 签名指南](https://developer.apple.com/documentation/xcode/preparing-your-app-for-distribution)
- [跨平台编译 iOS](https://clang.llvm.org/docs/CrossCompilation.html)
