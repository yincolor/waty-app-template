cmake_minimum_required(VERSION 3.25)

project(whiz-app-template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ---------------------------------------------------------------------------
# 引入 whiz 库（FetchContent）
#
# 生产项目使用远端仓库：
#   FetchContent_Declare(whiz
#       GIT_REPOSITORY https://github.com/yincolor/whiz.git
#       GIT_TAG main)
#
# 本测试项目使用本地 whiz 源码（../../ 即 whiz 仓库根目录），不拉取远端，
# 以便验证本地未提交的改动。测试完成后如需恢复远端拉取，请替换回上面的写法。
# ---------------------------------------------------------------------------
FetchContent_Declare(whiz
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
FetchContent_MakeAvailable(whiz)

# whiz 提供的资源嵌入函数：把 web/ 下所有文件嵌入可执行文件，
# 并通过 embedded://root/ 协议在运行时提供（入口 HTML 由 C++ 侧决定）。
include("${whiz_SOURCE_DIR}/cmake/whiz_embed_resources.cmake")

add_subdirectory(src)

