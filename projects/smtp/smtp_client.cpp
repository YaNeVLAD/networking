#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <common/tcp/Socket.hpp>

std::string receive_smtp_response(const tcp::Socket& sock)
{
	std::string response;
	while (true)
	{
		std::string chunk = sock.receive(1024);
		if (chunk.empty())
		{
			break;
		}
		response += chunk;

		if (const size_t last_newline = response.rfind("\r\n");
			last_newline != std::string::npos && last_newline >= 2)
		{
			const size_t prev_newline = response.rfind("\r\n", last_newline - 2);
			const size_t line_start = (prev_newline == std::string::npos) ? 0 : prev_newline + 2;

			if (last_newline >= line_start + 4)
			{
				if (std::string last_line = response.substr(line_start, last_newline - line_start);
					isdigit(last_line[0]) && isdigit(last_line[1]) && isdigit(last_line[2]) && last_line[3] == ' ')
				{
					break;
				}
			}
		}
	}
	return response;
}

void send_and_check(const tcp::Socket& sock, const std::string_view command, const int expected_code)
{
	if (!command.empty())
	{
		std::cout << "C: " << command;
		sock.send(command);
	}

	const std::string response = receive_smtp_response(sock);
	std::cout << "S: " << response;

	if (response.length() >= 3)
	{
		if (const int code = std::stoi(response.substr(0, 3)); code != expected_code)
		{
			throw std::runtime_error(
				"SMTP Error: Expected code " + std::to_string(expected_code)
				+ ", but received server response code: " + std::to_string(code));
		}
	}
	else
	{
		throw std::runtime_error("SMTP Error: Invalid or empty response received from server.");
	}
}

int main()
{
	try
	{
		const tcp::Socket client_socket;

		client_socket.connect("127.0.0.1", 25);

		const std::string greeting = receive_smtp_response(client_socket);
		std::cout << "S: " << greeting;
		if (greeting.length() < 3 || std::stoi(greeting.substr(0, 3)) != 220)
		{
			throw std::runtime_error("SMTP Error: Server greeting failed (expected 220).");
		}

		send_and_check(client_socket, "EHLO client.example.com\r\n", 250);
		send_and_check(client_socket, "MAIL FROM: <sender@example.com>\r\n", 250);
		send_and_check(client_socket, "RCPT TO: <recipient@example.com>\r\n", 250);

		send_and_check(client_socket, "DATA\r\n", 354);

		constexpr std::string_view
			message_body = "From: sender@example.com\r\n"
						   "To: recipient@example.com\r\n"
						   "Subject: Lab 7 Test Email (Fixed & Robust)\r\n"
						   "\r\n"
						   "Hello! This is a test message from custom C++ SMTP Client!\r\n"
						   "."
						   "\r\n";

		send_and_check(client_socket, message_body, 250);
		send_and_check(client_socket, "QUIT\r\n", 221);

		std::cout << "Session successfully completed and closed." << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}