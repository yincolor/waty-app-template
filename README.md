# waty-app-template

waty 组件演示模板：用同一个 waty 应用演示 **waty core**、**waty-notify**、
**waty-tray**，以及本地视频的流式播放。

## 演示内容

- **waty core**
  - 原生消息框 `dialog.showMessageBox`
  - 文件选择对话框（单选 / 多选）
  - 目录选择对话框
  - 保存文件对话框
  - `os` / `fs` / `window` 权限模块
- **waty-notify**
  - JS 通过 IPC 调用 C++ 侧 `waty::notify::send`
  - 普通通知与带按钮通知
  - 动作 / 关闭 / 错误事件回传 HTML 事件输出
- **waty-tray**
  - 托盘图标来自嵌入资源 `web/img/icon.png`
  - 右键菜单包含：显示主窗口、隐藏主窗口、关于、退出
  - 点击菜单项后通过 `window.waty.ipc.on("tray-menu-clicked", ...)` 回传并打印
- **本地视频流式播放**
  - `resource/video.mp4` 不是嵌入资源，构建后会复制到可执行文件旁边
  - 通过本地 HTTP Range 服务按需读取，`<video>` 直接播放
  - 另提供 `window.waty.fs.createReadStream` 对同一文件的流式读取探测

> **为什么播放不直接用 `fs.createReadStream`？**
> `video.mp4` 的 `moov` 位于文件末尾，且不是 fragmented MP4；WebKit 的
> MediaSource 无法按顺序把普通 MP4 的 `mdat` 直接追加播放。因此播放使用
> 本地 HTTP Range 流式服务，而 `fs.createReadStream` 按钮用于演示同一文件的
> 流式读取 API。

## 源码结构

```
src/
├── CMakeLists.txt
├── main.cpp                 # 应用装配、权限、窗口与事件订阅
├── notify_demo.hpp/.cpp     # waty-notify IPC 模块
├── tray_demo.hpp/.cpp       # waty-tray 托盘与菜单模块
├── video_demo.hpp/.cpp      # 视频路径与流服务管理
└── video_stream_server.hpp/.cpp  # 127.0.0.1 HTTP Range 流式服务
```

前端位于 `web/`，入口为 `web/page/index.html`，所有按钮与事件输出在 `web/js/index.js`。

## 构建与运行

环境要求见 waty README：C++23 编译器（GCC 13+）、CMake 3.28+；
Linux 需 GTK4 / WebKitGTK 6.0，waty-tray 还需 D-Bus / GLib / gdk-pixbuf。

默认通过 FetchContent 从 GitHub 拉取 waty、waty-tray、waty-notify：

```bash
cmake -B build -DCMAKE_CXX_COMPILER=g++-14 -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
./build/src/waty-app-template
```

### 使用本地 waty 工作区

在包含 `waty/`、`waty-tray/`、`waty-notify/` 的本地工作区中开发时，可用
FetchContent 的 `SOURCE_DIR` 覆盖避免重复拉取：

```bash
cmake -B build \
  -DCMAKE_CXX_COMPILER=g++-14 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DFETCHCONTENT_SOURCE_DIR_WATY=/path/to/C++/waty \
  -DFETCHCONTENT_SOURCE_DIR_WATY-TRAY=/path/to/C++/waty-tray \
  -DFETCHCONTENT_SOURCE_DIR_WATY-NOTIFY=/path/to/C++/waty-notify
cmake --build build -j4
```

> 生产项目建议把 `GIT_TAG` 固定到稳定 tag 或 commit，避免 main 分支变动导致构建不可复现。

## 资源说明

- `web/` 由 `waty_embed` 嵌入到可执行文件，应用图标和托盘图标都使用
  `web/img/icon.png`。
- `resource/` 不会被嵌入；构建后由 `src/CMakeLists.txt` 的 POST_BUILD
  命令复制到可执行文件同目录的 `resource/` 下。
- 视频播放依赖 `resource/video.mp4` 存在。若删除该文件，视频区域会显示
  “视频不可用”，不影响其他组件演示。

## 说明

- 页面入口在 `web/page/index.html`，对应 `main.cpp` 中的
  `window->loadFile("page/index.html")`。
- `app_opts.icon = "img/icon.png"` 使用嵌入资源图标；托盘图标通过
  `app.readEmbeddedFile("img/icon.png")` + `waty::tray::Icon::from_data` 设置。
- `Subscription` 返回值会保存在 `main.cpp` 中，避免订阅被立即取消。
- 构建产物 `build/` 由 `.gitignore` 忽略。

## 网络问题

CMake 在构建时需要从网络下载依赖库。如果遇到网络问题，可以使用下面的方法解决。

### URL 前缀代理

以下使用 `https://ghproxy.net/` 进行演示。在执行 CMake 构建前，为当前终端会话
临时设置 URL 前缀代理：

```shell
export GIT_CONFIG_COUNT=1
export GIT_CONFIG_KEY_0='url.https://ghproxy.net/https://github.com.insteadOf'
export GIT_CONFIG_VALUE_0='https://github.com'

cmake -B build -DCMAKE_CXX_COMPILER=g++-14 -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

说明：这些环境变量仅在当前终端会话中生效，关闭终端或执行
`unset GIT_CONFIG_COUNT GIT_CONFIG_KEY_0 GIT_CONFIG_VALUE_0` 后即失效。
该方式仅适用于通过 HTTPS 访问 GitHub 的场景，不适用于 SSH 地址。

### 使用网络代理工具

你应该知道怎么办。

## 许可证

waty 库采用 AGPL-3.0；本模板仅作为使用示例。
