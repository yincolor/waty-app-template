#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <thread>

namespace demo
{
    /// 为本地视频文件提供 127.0.0.1 HTTP Range 流式服务。
    ///
    /// <video> 无法直接从 embedded:// 页面访问 file:// 文件，因此模板用一个极小的
    /// 本地 HTTP 服务按浏览器请求的范围读取文件，实现真正的按需流式播放。
    class VideoStreamServer
    {
      public:
        explicit VideoStreamServer(std::filesystem::path file);
        ~VideoStreamServer();

        VideoStreamServer(const VideoStreamServer &)            = delete;
        VideoStreamServer &operator=(const VideoStreamServer &) = delete;

        bool start(std::string &error);

        [[nodiscard]] std::string url() const;
        [[nodiscard]] std::uint64_t file_size() const noexcept
        {
            return m_file_size;
        }

      private:
        void accept_loop(std::stop_token stop, std::intptr_t listen_socket);

        std::filesystem::path m_file;
        std::uint64_t m_file_size = 0;
        std::uint16_t m_port      = 0;
        std::intptr_t m_listen_socket = -1;
        std::jthread m_thread;
    };
} // namespace demo
