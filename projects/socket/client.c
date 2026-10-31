#include <stdio.h>
#include <stdlib.h>

#include <common/tcp/c/socket.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5050
#define CLIENT_NAME "*** TCP Socket Client ***"
#define MAX_NAME_LEN 128

typedef struct
{
	char name[MAX_NAME_LEN];
	int number;
} message_packet_t;

int main(void)
{
	printf("[CLIENT] Initializing network subsystem...\n");
	if (tcp_net_init() != 0)
	{
		fprintf(stderr, "[CLIENT ERROR] Failed to initialize network.\n");
		return 1;
	}

	int user_number = 0;
	while (1)
	{
		printf("[CLIENT] Enter an integer between 1 and 100:\n");
		if (scanf_s("%d", &user_number) != 1)
		{
			fprintf(stderr, "[CLIENT ERROR] Invalid input. Please enter a valid integer.\n");
			int c;
			while ((c = getchar()) != '\n' && c != EOF)
				;
			continue;
		}
		if (user_number >= 1 && user_number <= 100)
		{
			break;
		}
		printf("[CLIENT] Number out of range (1-100). Please try again.\n");
	}

	message_packet_t client_msg = { 0 };
	snprintf(client_msg.name, sizeof(client_msg.name), CLIENT_NAME);
	client_msg.number = user_number;

	printf("[CLIENT] Creating TCP socket...\n");
	const socket_t sock = tcp_socket_create();
	if (sock == NET_INVALID_SOCKET)
	{
		fprintf(stderr, "[CLIENT ERROR] Failed to create socket.\n");
		tcp_net_shutdown();
		return 1;
	}

	printf("[CLIENT] Connecting to server at %s:%d...\n", SERVER_IP, SERVER_PORT);
	if (tcp_socket_connect(sock, SERVER_IP, SERVER_PORT) != 0)
	{
		fprintf(stderr, "[CLIENT ERROR] Failed to connect to server.\n");
		tcp_socket_close(sock);
		tcp_net_shutdown();
		return 1;
	}
	printf("[CLIENT] Connected successfully.\n");

	printf("[CLIENT] Sending message (Name: '%s', Number: %d)...\n", client_msg.name, client_msg.number);
	if (tcp_socket_send(sock, (const char*)&client_msg, sizeof(client_msg)) <= 0)
	{
		fprintf(stderr, "[CLIENT ERROR] Failed to send data to server.\n");
		tcp_socket_close(sock);
		tcp_net_shutdown();
		return 1;
	}
	printf("[CLIENT] Message sent successfully. Waiting for server response...\n");

	message_packet_t server_resp = { 0 };
	const int bytes_received = tcp_socket_recv(sock, (char*)&server_resp, sizeof(server_resp));
	if (bytes_received <= 0)
	{
		fprintf(stderr, "[CLIENT ERROR] Failed to receive response from server or connection closed.\n");
		tcp_socket_close(sock);
		tcp_net_shutdown();
		return 1;
	}

	const int sum = client_msg.number + server_resp.number;
	printf("\n--- RESULT ON CLIENT ---\n");
	printf("Client Name   : %s\n", client_msg.name);
	printf("Server Name   : %s\n", server_resp.name);
	printf("Client Number : %d\n", client_msg.number);
	printf("Server Number : %d\n", server_resp.number);
	printf("Sum           : %d + %d = %d\n", client_msg.number, server_resp.number, sum);
	printf("------------------------\n\n");

	printf("[CLIENT] Closing socket...\n");
	tcp_socket_close(sock);
	tcp_net_shutdown();
	printf("[CLIENT] Finished successfully.\n");

	return 0;
}