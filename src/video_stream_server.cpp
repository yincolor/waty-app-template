#include "video_stream_server.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <csignal>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

namespace demo
{
    namespace
    {
#if defined(_WIN32)
        using socket_t = SOCKET;
        constexpr socket_t k_invalid_socket = INVALID_SOCKET;
#else
        using socket_t = int;
        constexpr socket_t k_invalid_socket = -1;
#endif

        std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        std::string trim(std::string value)
        {
            const auto not_space = [](unsigned char c) { return !std::isspace(c); };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
            value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
            return value;
        }

        void close_socket(socket_t socket)
        {
#if defined(_WIN32)
            ::closesocket(socket);
#else
            ::close(socket);
#endif
        }

        void shutdown_socket(socket_t socket)
        {
#if defined(_WIN32)
            ::shutdown(socket, SD_BOTH);
#else
            ::shutdown(socket, SHUT_RDWR);
#endif
        }

        bool send_all(socket_t socket, const char *data, std::size_t size)
        {
            std::size_t sent = 0;
            while (sent < size)
            {
#if defined(MSG_NOSIGNAL)
                const int flags = MSG_NOSIGNAL;
#else
                const int flags = 0;
#endif
#if defined(_WIN32)
                const int n = ::send(socket, data + sent, static_cast<int>(size - sent), flags);
#else
                const ssize_t n = ::send(socket, data + sent, size - sent, flags);
#endif
                if (n <= 0)
                {
                    return false;
                }
                sent += static_cast<std::size_t>(n);
            }
            return true;
        }

        bool send_text(socket_t socket, std::string_view text)
        {
            return send_all(socket, text.data(), text.size());
        }

        struct ByteRange
        {
            std::uint64_t begin = 0;
            std::uint64_t end   = 0;
            bool valid          = false;
        };

        ByteRange parse_range(std::string_view value, std::uint64_t file_size)
        {
            ByteRange range;
            value = std::string_view(value.data(), value.size());
            if (value.rfind("bytes=", 0) != 0)
            {
                return range;
            }

            value.remove_prefix(6);
            const auto dash = value.find('-');
            if (dash == std::string_view::npos)
            {
                return range;
            }

            const std::string_view first  = value.substr(0, dash);
            const std::string_view second = value.substr(dash + 1);
            if (file_size == 0)
            {
                return range;
            }

            try
            {
                if (first.empty())
                {
                    // bytes=-N：最后 N 字节
                    const std::uint64_t suffix = std::stoull(std::string(second));
                    if (suffix == 0)
                    {
                        return range;
                    }
                    range.begin = suffix >= file_size ? 0 : file_size - suffix;
                    range.end   = file_size - 1;
                }
                else
                {
                    range.begin = std::stoull(std::string(first));
                    range.end   = second.empty() ? file_size - 1 : std::stoull(std::string(second));
                }
            }
            catch (...)
            {
                return range;
            }

            if (range.begin >= file_size || range.begin > range.end)
            {
                return range;
            }
            range.end   = std::min(range.end, file_size - 1);
            range.valid = true;
            return range;
        }

        void serve_client(std::filesystem::path file, std::uint64_t file_size, socket_t client)
        {
            std::string request;
            char buffer[4096];
            while (request.find("\r\n\r\n") == std::string::npos && request.size() < 32768)
            {
                const int n = ::recv(client, buffer, static_cast<int>(sizeof(buffer)), 0);
                if (n <= 0)
                {
                    close_socket(client);
                    return;
                }
                request.append(buffer, static_cast<std::size_t>(n));
            }

            std::istringstream request_stream(request);
            std::string method;
            std::string target;
            std::string version;
            request_stream >> method >> target >> version;

            const bool is_head = method == "HEAD";
            if (method != "GET" && !is_head)
            {
                send_text(client, "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                close_socket(client);
                return;
            }

            const auto query = target.find('?');
            if (query != std::string::npos)
            {
                target.resize(query);
            }
            if (target != "/video")
            {
                send_text(client, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                close_socket(client);
                return;
            }

            bool has_range = false;
            ByteRange range;
            std::istringstream header_stream(request);
            std::string line;
            while (std::getline(header_stream, line))
            {
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                const auto colon = line.find(':');
                if (colon == std::string::npos)
                {
                    continue;
                }
                if (to_lower(trim(line.substr(0, colon))) == "range")
                {
                    has_range = true;
                    range     = parse_range(trim(line.substr(colon + 1)), file_size);
                }
            }

            if (has_range && !range.valid)
            {
                send_text(client,
                          "HTTP/1.1 416 Range Not Satisfiable\r\n"
                          "Content-Range: bytes */" + std::to_string(file_size) + "\r\n"
                          "Content-Length: 0\r\n"
                          "Connection: close\r\n\r\n");
                close_socket(client);
                return;
            }

            const std::uint64_t begin = has_range ? range.begin : 0;
            const std::uint64_t end   = has_range ? range.end : (file_size - 1);
            const std::uint64_t length = end - begin + 1;

            std::string headers;
            if (has_range)
            {
                headers = "HTTP/1.1 206 Partial Content\r\n";
                headers += "Content-Range: bytes " + std::to_string(begin) + "-" +
                           std::to_string(end) + "/" + std::to_string(file_size) + "\r\n";
            }
            else
            {
                headers = "HTTP/1.1 200 OK\r\n";
            }
            headers += "Content-Type: video/mp4\r\n";
            headers += "Accept-Ranges: bytes\r\n";
            headers += "Access-Control-Allow-Origin: *\r\n";
            headers += "Cache-Control: no-store\r\n";
            headers += "Content-Length: " + std::to_string(length) + "\r\n";
            headers += "Connection: close\r\n\r\n";

            if (!send_text(client, headers))
            {
                close_socket(client);
                return;
            }

            if (!is_head)
            {
                std::ifstream input(file, std::ios::binary);
                if (!input)
                {
                    close_socket(client);
                    return;
                }
                input.seekg(static_cast<std::streamoff>(begin));

                std::vector<char> chunk(64 * 1024);
                std::uint64_t remaining = length;
                while (remaining > 0 && input)
                {
                    const std::size_t want = static_cast<std::size_t>(
                        std::min<std::uint64_t>(remaining, chunk.size()));
                    input.read(chunk.data(), static_cast<std::streamsize>(want));
                    const std::size_t got = static_cast<std::size_t>(input.gcount());
                    if (got == 0)
                    {
                        break;
                    }
                    if (!send_all(client, chunk.data(), got))
                    {
                        break;
                    }
                    remaining -= got;
                }
            }

            shutdown_socket(client);
            close_socket(client);
        }
    } // namespace

    VideoStreamServer::VideoStreamServer(std::filesystem::path file)
        : m_file(std::move(file))
    {
        std::error_code ec;
        if (std::filesystem::is_regular_file(m_file, ec))
        {
            m_file_size = std::filesystem::file_size(m_file, ec);
        }
    }

    VideoStreamServer::~VideoStreamServer()
    {
        if (m_thread.joinable())
        {
            m_thread.request_stop();
        }
        if (m_listen_socket >= 0)
        {
            shutdown_socket(static_cast<socket_t>(m_listen_socket));
            close_socket(static_cast<socket_t>(m_listen_socket));
            m_listen_socket = -1;
        }
        if (m_thread.joinable())
        {
            m_thread.join();
        }
#if defined(_WIN32)
        WSACleanup();
#endif
    }

    bool VideoStreamServer::start(std::string &error)
    {
        if (m_thread.joinable())
        {
            return true;
        }
        if (m_file_size == 0)
        {
            error = "视频文件不存在或为空: " + m_file.string();
            return false;
        }

#if defined(_WIN32)
        WSADATA wsa_data{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            error = "WSAStartup 失败";
            return false;
        }
        const socket_t listen_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
        std::signal(SIGPIPE, SIG_IGN);
        const socket_t listen_socket = ::socket(AF_INET, SOCK_STREAM, 0);
#endif
        if (listen_socket == k_invalid_socket)
        {
            error = "创建监听 socket 失败";
            return false;
        }

        int reuse = 1;
        ::setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<const char *>(&reuse), sizeof(reuse));

        sockaddr_in address{};
        address.sin_family      = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port        = 0;
        if (::bind(listen_socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0)
        {
            error = "绑定 127.0.0.1 失败";
            close_socket(listen_socket);
            return false;
        }
        if (::listen(listen_socket, 16) != 0)
        {
            error = "listen 失败";
            close_socket(listen_socket);
            return false;
        }

#if defined(_WIN32)
        int address_length = sizeof(address);
#else
        socklen_t address_length = sizeof(address);
#endif
        if (::getsockname(listen_socket, reinterpret_cast<sockaddr *>(&address), &address_length) != 0)
        {
            error = "getsockname 失败";
            close_socket(listen_socket);
            return false;
        }

        m_port          = ntohs(address.sin_port);
        m_listen_socket = static_cast<std::intptr_t>(listen_socket);
        m_thread = std::jthread([this, listen = m_listen_socket](std::stop_token stop) {
            accept_loop(stop, listen);
        });
        return true;
    }

    std::string VideoStreamServer::url() const
    {
        if (m_port == 0)
        {
            return {};
        }
        return "http://127.0.0.1:" + std::to_string(m_port) + "/video";
    }

    void VideoStreamServer::accept_loop(std::stop_token stop, std::intptr_t listen_socket)
    {
        while (!stop.stop_requested())
        {
            sockaddr_in peer{};
#if defined(_WIN32)
            int peer_length = sizeof(peer);
#else
            socklen_t peer_length = sizeof(peer);
#endif
            const socket_t client =
                ::accept(static_cast<socket_t>(listen_socket), reinterpret_cast<sockaddr *>(&peer), &peer_length);
            if (client == k_invalid_socket)
            {
                if (stop.stop_requested())
                {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
                continue;
            }

            // 每个连接独立线程；只捕获文件路径、大小和 socket，避免访问已销毁的 this。
            std::thread([file = m_file, size = m_file_size, client] {
                serve_client(file, size, client);
            }).detach();
        }
    }
} // namespace demo
