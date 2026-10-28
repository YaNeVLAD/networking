#include <common/tcp/Socket.hpp>
#include <iostream>
#include <string>

void send_and_receive(const tcp::Socket& sock, const std::string& command)
{
	if (!command.empty())
	{
		std::cout << "C: " << command;
		sock.send(command);
	}
	const std::string response = sock.receive(1024);
	std::cout << "S: " << response;
}

int main()
{
	try
	{
		const tcp::Socket client_socket;

		client_socket.connect("127.0.0.1", 25);

		const std::string greeting = client_socket.receive(1024);
		std::cout << "S: " << greeting;

		send_and_receive(client_socket, "HELO client.example.com\r\n");
		send_and_receive(client_socket, "MAIL FROM: <sender@example.com>\r\n");
		send_and_receive(client_socket, "RCPT TO: <recipient@example.com>\r\n");

		send_and_receive(client_socket, "DATA\r\n");

		std::string message_body = "From: sender@example.com\r\n"
								   "To: recipient@example.com\r\n"
								   "Subject: Lab 7 Test Email (RAII Wrapper)\r\n"
								   "\r\n"
								   "Hello! This is a test message from custom C++ SMTP Client!\r\n"
								   ".\r\n";

		send_and_receive(client_socket, message_body);

		send_and_receive(client_socket, "QUIT\r\n");

		std::cout << "Session closed" << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}