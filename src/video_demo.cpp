#include "video_demo.hpp"

#include "video_stream_server.hpp"

#include <future>
#include <string>
#include <utility>

namespace demo
{
    VideoDemo::VideoDemo(waty::Application &app, std::shared_ptr<waty::WebWindow> window)
        : m_window(std::move(window))
    {
        // getPath(Executable) 返回可执行文件完整路径；resource 与其位于同一目录。
        const std::filesystem::path executable = app.getPath(waty::SysPath::Executable);
        m_resource_dir = executable.parent_path() / "resource";
        m_video_path   = m_resource_dir / std::filesystem::path(u8"video.mp4");

        m_server = std::make_unique<VideoStreamServer>(m_video_path);
        m_server->start(m_error);
    }

    VideoDemo::~VideoDemo() = default;

    void VideoDemo::register_ipc()
    {
        std::weak_ptr<VideoDemo> weak_self = weak_from_this();

        if (auto window = m_window.lock())
        {
            window->ipcHandle("video.info",
                              [weak_self](const nlohmann::json &) -> std::future<nlohmann::json> {
                                  return std::async(std::launch::async, [weak_self]() -> nlohmann::json {
                                      if (auto self = weak_self.lock())
                                      {
                                          return self->info();
                                      }
                                      return {{"ok", false}, {"error", "VideoDemo 已销毁"}};
                                  });
                              });
        }
    }

    nlohmann::json VideoDemo::info() const
    {
        std::error_code ec;
        const bool exists = std::filesystem::is_regular_file(m_video_path, ec);
        return {
            {"ok", exists && m_error.empty()},
            {"exists", exists},
            {"path", m_video_path.string()},
            {"resourceDir", m_resource_dir.string()},
            {"url", m_server ? m_server->url() : std::string{}},
            {"size", m_server ? m_server->file_size() : 0},
            {"streaming", "http-range"},
            {"error", m_error},
        };
    }
} // namespace demo
