# whiz-app-template

whiz 的样例模板项目：一个可运行的桌面应用骨架，展示如何用 whiz 组织
C++ 主程序、前端页面与应用图标。

前端已就绪于 `web/`，构建时由 `whiz_embed` 嵌入到可执行文件；
应用图标为项目根目录的 `resource/icon.png`。

## 构建与运行

环境要求见 whiz README：C++23 编译器（GCC 13+）、CMake 3.25+；
Linux 需 GTK4 / WebKitGTK 6.0。

```bash
cmake -B build -DCMAKE_CXX_COMPILER=g++-14 -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

`CMakeLists.txt` 通过 FetchContent 拉取 whiz：

```cmake
FetchContent_Declare(whiz
    GIT_REPOSITORY git@github.com:yincolor/whiz.git
    GIT_TAG main)
```

> 生产项目建议把 `GIT_TAG` 固定到稳定 tag 或 commit，避免 main 分支变动导致不可复现构建。

## 说明

- 页面入口在 `web/page/index.html`，对应 `main.cpp` 中的 `window->loadFile("page/index.html")`。
- `web/js/whiz.js` 演示了 whiz JS 桥：`ipc.invoke` / `ipc.on`、`os.*`、`fs.*`、`window.*`。
  `main.cpp` 注册了 `greet` IPC 处理器，并通过 `onEventLoopTick` 每 50 轮推送一次
  `server-ping` 事件。
- 构建产物 `build/` 由 `.gitignore` 忽略。

## 网络问题

CMake 在构建时需要从网络下载依赖库。如果遇到网络问题，可以使用下面的方法解决。

### URL前缀代理

以下使用`https://ghproxy.net/`进行演示

在执行 CMake 构建前，运行以下命令，为当前终端会话临时设置 URL 前缀代理：

```shell
export GIT_CONFIG_COUNT=1
export GIT_CONFIG_KEY_0='url.https://ghproxy.net/https://github.com.insteadOf'
export GIT_CONFIG_VALUE_0='https://github.com'

cmake -B build -DCMAKE_CXX_COMPILER=g++-14 -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

说明：这些环境变量仅在当前终端会话中生效，关闭终端或执行 unset GIT_CONFIG_COUNT GIT_CONFIG_KEY_0 GIT_CONFIG_VALUE_0 后即失效。该方式仅适用于通过 HTTPS 访问 GitHub 的场景，不适用于 SSH 地址

### 使用网络代理工具

你应该知道怎么办

## 许可证

whiz 库采用 AGPL-3.0；本模板仅作为使用示例。
