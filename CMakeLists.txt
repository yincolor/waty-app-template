cmake_minimum_required(VERSION 3.28)

project(waty-app-template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ---------------------------------------------------------------------------
# 引入 waty 及扩展组件（FetchContent）
#
# waty 仓库：https://github.com/yincolor/waty.git
# 生产项目建议把 GIT_TAG 固定到稳定 tag / commit，避免 main 分支变动导致构建不可复现。
#
# 本地开发时可用 FetchContent 的 SOURCE_DIR 覆盖，例如：
#   cmake -B build \
#     -DFETCHCONTENT_SOURCE_DIR_WATY=/path/to/C++/waty \
#     -DFETCHCONTENT_SOURCE_DIR_WATY-TRAY=/path/to/C++/waty-tray \
#     -DFETCHCONTENT_SOURCE_DIR_WATY-NOTIFY=/path/to/C++/waty-notify
# ---------------------------------------------------------------------------
FetchContent_Declare(waty
    GIT_REPOSITORY https://github.com/yincolor/waty.git
    GIT_TAG main)
FetchContent_Declare(waty-tray
    GIT_REPOSITORY https://github.com/yincolor/waty-tray.git
    GIT_TAG main)
FetchContent_Declare(waty-notify
    GIT_REPOSITORY https://github.com/yincolor/waty-notify.git
    GIT_TAG main)
FetchContent_MakeAvailable(waty waty-tray waty-notify)

# waty_embed 在 FetchContent_MakeAvailable(waty) 后即可直接使用：
# 把 web/ 下所有文件生成独立资源目标（默认 waty::embedded），
# 并通过 embedded://root/ 协议在运行时提供（入口 HTML 由 C++ 侧决定）。

add_subdirectory(src)
