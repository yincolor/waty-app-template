# src/CMakeLists.txt

# 1. 定义可执行目标
add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
    # 以后新增 .cpp 追加在这里
)

# 2. 头文件搜索路径
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # 如果还有其他头文件目录在此追加
)

# 3. 链接 waty 库与嵌入资源目标 ( waty::waty 由根目录 FetchContent 提供) 
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE waty::waty waty::embedded)

# 4. 嵌入 web/ 静态资源
#    web/ 在项目根目录，所以用 ${CMAKE_SOURCE_DIR}
waty_embed("${CMAKE_SOURCE_DIR}/web")

# 5. 复制图标到可执行文件所在目录
#    app_opts.icon 以可执行文件目录为基准
#    注意：icon.png 在 resource/ 下
configure_file(
    "${CMAKE_SOURCE_DIR}/resource/icon.png"
    "${CMAKE_CURRENT_BINARY_DIR}/resource/icon.png"
    COPYONLY
)

# 6. Windows GUI 程序隐藏控制台（可选）
if (WIN32)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
