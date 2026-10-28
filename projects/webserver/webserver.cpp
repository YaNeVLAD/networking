#include "common/flat_map.hpp"

#include <csignal>
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <common/execution/ThreadPool.hpp>
#include <common/tcp/Listener.hpp>
#include <common/tcp/Socket.hpp>

constexpr unsigned short PORT = 8000;
constexpr size_t BUFFER_SIZE = 4096;

static volatile std::sig_atomic_t g_running = 1;

void signal_handler(const int signal)
{
	if (signal == SIGINT)
	{
		g_running = 0;
	}
}

std::string get_mime_type(const std::string_view path)
{
	const auto pos = path.find_last_of('.');
	if (pos == std::string::npos)
	{
		return "application/octet-stream";
	}

	auto ext = std::string(path.substr(pos));
	for (char& ch : ext)
	{
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}

	constexpr auto EXT_MIME_TYPE_MAP = re::make_flat_map<std::string_view, std::string_view>({
		{ ".html", "text/html; charset=utf-8" },
		{ ".css", "text/css; charset=utf-8" },
		{ ".js", "application/javascript; charset=utf-8" },
		{ ".json", "application/json; charset=utf-8" },
		{ ".png", "image/png" },
		{ ".jpg", "image/jpeg" },
		{ ".jpeg", "image/jpeg" },
		{ ".gif", "image/gif" },
		{ ".svg", "image/svg+xml" },
		{ ".txt", "text/plain; charset=utf-8" },
	});

	return std::string(EXT_MIME_TYPE_MAP.get(ext, "application/octet-stream"));
}

void handle_client(tcp::Socket client_socket)
{
	if (!client_socket.valid())
	{
		return;
	}

	std::string request = client_socket.receive(BUFFER_SIZE);
	if (request.empty())
	{
		return;
	}

	std::istringstream stream(request);
	std::string method, path;
	stream >> method >> path;

	std::cout << "[LOG] Received: " << method << " " << path << std::endl;

	if (method != "GET")
	{
		std::string response = "HTTP/1.1 405 Method Not Allowed\r\n"
							   "Content-Type: text/plain; charset=utf-8\r\n\r\n"
							   "Method Not Allowed";
		client_socket.send(response);
		return;
	}

	if (path == "/")
	{
		path = "/index.html";
	}

	std::string filename = path.substr(1);
	if (std::ifstream file(filename, std::ios::binary | std::ios::ate); !file.is_open())
	{
		std::string body = "File Not Found";
		auto response = std::format(
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Type: text/plain; charset=utf-8\r\n"
			"Content-Length: {}\r\n\r\n"
			"{}",
			body.size(), body);

		client_socket.send(response);
		std::cout << "[LOG] 404 Not Found: " << filename << std::endl;
	}
	else
	{
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<char> buffer(static_cast<size_t>(size));
		if (file.read(buffer.data(), size))
		{
			std::string body(buffer.begin(), buffer.end());
			std::string mime_type = get_mime_type(filename);
			auto response = std::format(
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: {}\r\n"
				"Content-Length: {}\r\n\r\n"
				"{}",
				mime_type, body.size(), body);

			client_socket.send(response);
			std::cout << "[LOG] 200 OK File sent: " << filename << " (" << mime_type << ")" << std::endl;
		}
		else
		{
			std::string body = "Internal Server Error";
			auto response = std::format(
				"HTTP/1.1 500 Internal Server Error\r\n"
				"Content-Type: text/plain; charset=utf-8\r\n"
				"Content-Length: {}\r\n\r\n"
				"{}",
				body.size(), body);

			client_socket.send(response);
			std::cout << "[LOG] 500 Error reading file: " << filename << std::endl;
		}
	}
}

int main()
{
	std::signal(SIGINT, signal_handler);

	try
	{
		const tcp::Listener listener;

		listener.set_reuse_address(true);
		listener.bind(PORT);
		listener.listen(10);

		execution::ThreadPool pool;

		std::cout << std::format("WEB server is running on http://localhost:{} ...\n", PORT);
		while (g_running)
		{
			tcp::Socket client_socket = listener.accept();

			if (!client_socket.valid())
			{
				if (!g_running)
				{
					break;
				}

				std::cerr << "Error: Failed to create client connection\n";
				continue;
			}

			pool.enqueue([client_socket = std::move(client_socket)]() mutable {
				handle_client(std::move(client_socket));
			});
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
		return 1;
	}

	return 0;
}