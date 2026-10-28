#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <string>

#include <common/tcp/Listener.hpp>
#include <common/tcp/Socket.hpp>

constexpr unsigned short PORT = 8000;
constexpr size_t BUFFER_SIZE = 4096;

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
	if (std::ifstream file(filename, std::ios::binary); !file.is_open())
	{
		std::string body = "File Not Found";
		std::string response = "HTTP/1.1 404 Not Found\r\n"
							   "Content-Type: text/plain; charset=utf-8\r\n"
							   "Content-Length: "
			+ std::to_string(body.size()) + "\r\n\r\n" + body;
		client_socket.send(response);
		std::cout << "[LOG] 404 Not Found: " << filename << std::endl;
	}
	else
	{
		std::ostringstream file_stream;
		file_stream << file.rdbuf();
		std::string body = file_stream.str();

		std::string response = "HTTP/1.1 200 OK\r\n"
							   "Content-Type: text/html; charset=utf-8\r\n"
							   "Content-Length: "
			+ std::to_string(body.size()) + "\r\n\r\n" + body;
		client_socket.send(response);
		std::cout << "[LOG] 200 OK File sent: " << filename << std::endl;
	}
}

int main()
{
	try
	{
		const tcp::Listener listener;

		listener.set_reuse_address(true);
		listener.bind(PORT);
		listener.listen(10);

		std::cout << std::format("WEB server is running on http://localhost:{} ...\n", PORT);

		while (true)
		{
			tcp::Socket client_socket = listener.accept();
			if (!client_socket.valid())
			{
				std::cerr << "Error: Failed to create client connection" << std::endl;
				continue;
			}

			handle_client(std::move(client_socket));
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}