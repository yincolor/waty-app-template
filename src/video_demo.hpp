#pragma once

#include <waty/application.hpp>
#include <waty/web_window.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace demo
{
    class VideoStreamServer;

    /// 演示非嵌入资源视频的流式播放。
    ///
    /// resource/video.mp4 随 resource 目录复制到可执行文件旁；本模块启动一个仅监听
    /// 127.0.0.1 的 HTTP Range 服务，JS 侧用 <video> 直接播放，并可通过
    /// window.waty.fs.createReadStream 对同一文件做流式读取探测。
    class VideoDemo : public std::enable_shared_from_this<VideoDemo>
    {
      public:
        VideoDemo(waty::Application &app, std::shared_ptr<waty::WebWindow> window);
        ~VideoDemo();

        VideoDemo(const VideoDemo &)            = delete;
        VideoDemo &operator=(const VideoDemo &) = delete;

        void register_ipc();
        [[nodiscard]] nlohmann::json info() const;

      private:
        std::filesystem::path m_resource_dir;
        std::filesystem::path m_video_path;
        std::unique_ptr<VideoStreamServer> m_server;
        std::string m_error;
        std::weak_ptr<waty::WebWindow> m_window;
    };
} // namespace demo
