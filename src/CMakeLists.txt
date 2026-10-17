# src/CMakeLists.txt

# 1. 定义可执行目标
add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
    notify_demo.cpp
    tray_demo.cpp
    video_demo.cpp
    video_stream_server.cpp
)

# 2. 头文件搜索路径
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 3. 嵌入 web/ 静态资源
#    waty_embed 由 waty 的 CMake 提供，生成 waty::embedded 目标。
waty_embed("${CMAKE_SOURCE_DIR}/web")

# 4. 链接 waty、waty-tray、waty-notify 与嵌入资源目标
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    waty::waty
    waty::embedded
    waty-tray::waty-tray
    waty-notify::waty-notify
)

# 5. 复制 resource/ 到可执行文件旁边
#    resource/video.mp4 不是嵌入资源，视频演示通过本地 HTTP Range 流式读取它。
#    app.getPath(SysPath::Executable) 的父目录即 TARGET_FILE_DIR。
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/resource"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resource/icon.png"
        "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/resource/icon.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resource/video.mp4"
        "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/resource/video.mp4"
    COMMENT "复制 resource/ 到可执行文件目录"
)

# 6. Windows：GUI 程序隐藏控制台，并链接 Winsock（本地视频流服务）
if (WIN32)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ws2_32)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
