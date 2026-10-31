#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <common/execution/c/thread_pool.h>
#include <common/tcp/c/socket.h>

#define SERVER_PORT 5050
#define SERVER_NAME "### TCP Socket Server ###"
#define SERVER_FIXED_NUMBER 50
#define MAX_NAME_LEN 128
#define CLIENT_TIMEOUT_MS (10000) // 10 * 1000ms
#define THREAD_POOL_SIZE 16

typedef struct
{
	char name[MAX_NAME_LEN];
	int number;
} message_packet_t;

typedef struct
{
	socket_t client_sock;
} client_task_arg_t;

static volatile int g_running = 1;
void sigint_handler(const int sig)
{
	(void)sig;
	g_running = 0;
}

void client_handler_task(void* arg)
{
	client_task_arg_t* task_arg = arg;
	if (!task_arg)
	{
		return;
	}

	const socket_t client_sock = task_arg->client_sock;
	free(task_arg);

	const int wait_res = tcp_socket_wait_for(client_sock, CLIENT_TIMEOUT_MS);
	if (wait_res <= 0)
	{
		fprintf(stderr, "[SERVER WARNING] Client timed out (no data within 10s) or connection error.\n");
		tcp_socket_close(client_sock);
		return;
	}

	message_packet_t client_msg = { 0 };
	const int bytes_received = tcp_socket_recv(client_sock, (char*)&client_msg, sizeof(client_msg));
	if (bytes_received <= 0)
	{
		fprintf(stderr, "[SERVER ERROR] Failed to receive data from client or connection closed.\n");
		tcp_socket_close(client_sock);
		return;
	}

	printf("[SERVER] Received message from client '%s' (number: %d)\n", client_msg.name, client_msg.number);

	const int sum = client_msg.number + SERVER_FIXED_NUMBER;
	message_packet_t server_resp = { 0 };
	snprintf(server_resp.name, sizeof(server_resp.name), "%s", SERVER_NAME);
	server_resp.number = SERVER_FIXED_NUMBER;

	if (tcp_socket_send(client_sock, (const char*)&server_resp, sizeof(server_resp)) <= 0)
	{
		fprintf(stderr, "[SERVER ERROR] Failed to send response to client.\n");
	}
	else
	{
		printf("[SERVER] Response sent successfully to '%s' (Sum: %d)\n", client_msg.name, sum);
	}

	tcp_socket_close(client_sock);
}

int main(void)
{
	signal(SIGINT, sigint_handler);

	printf("[SERVER] Initializing network subsystem...\n");
	if (tcp_net_init() != 0)
	{
		fprintf(stderr, "[SERVER ERROR] Failed to initialize network.\n");
		return 1;
	}

	printf("[SERVER] Creating thread pool with %d threads...\n", THREAD_POOL_SIZE);
	thread_pool_t* pool = thread_pool_create(THREAD_POOL_SIZE);
	if (!pool)
	{
		fprintf(stderr, "[SERVER ERROR] Failed to create thread pool.\n");
		tcp_net_shutdown();
		return 1;
	}

	const socket_t listen_sock = tcp_socket_create();
	if (listen_sock == NET_INVALID_SOCKET)
	{
		fprintf(stderr, "[SERVER ERROR] Failed to create listening socket.\n");
		thread_pool_destroy(pool);
		tcp_net_shutdown();
		return 1;
	}

	if (tcp_socket_set_reuse_addr(listen_sock, 1) != 0)
	{
		fprintf(stderr, "[SERVER WARNING] Failed to set socket reuse address option.\n");
	}

	printf("[SERVER] Binding socket to port %d...\n", SERVER_PORT);
	if (tcp_socket_bind(listen_sock, SERVER_PORT) != 0)
	{
		fprintf(stderr, "[SERVER ERROR] Failed to bind socket to port %d.\n", SERVER_PORT);
		tcp_socket_close(listen_sock);
		thread_pool_destroy(pool);
		tcp_net_shutdown();
		return 1;
	}

	printf("[SERVER] Starting to listen for incoming connections...\n");
	if (tcp_socket_listen(listen_sock, 64) != 0)
	{
		fprintf(stderr, "[SERVER ERROR] Failed to listen on socket.\n");
		tcp_socket_close(listen_sock);
		thread_pool_destroy(pool);
		tcp_net_shutdown();
		return 1;
	}

	printf("[SERVER] Advanced Concurrent Server '%s' is running on port %d...\n", SERVER_NAME, SERVER_PORT);
	printf("[SERVER] Press Ctrl+C to trigger Graceful Shutdown.\n\n");

	while (g_running)
	{
		const int wait_res = tcp_socket_wait_for(listen_sock, 500);
		if (wait_res <= 0)
		{
			continue;
		}

		const socket_t client_sock = tcp_socket_accept(listen_sock);
		if (client_sock == NET_INVALID_SOCKET)
		{
			if (!g_running)
			{
				break;
			}
			continue;
		}

		if (!g_running)
		{
			tcp_socket_close(client_sock);
			break;
		}

		client_task_arg_t* task_arg = malloc(sizeof(client_task_arg_t));
		if (!task_arg)
		{
			fprintf(stderr, "[SERVER ERROR] Out of memory for client task.\n");
			tcp_socket_close(client_sock);
			continue;
		}
		task_arg->client_sock = client_sock;

		if (thread_pool_enqueue(pool, client_handler_task, task_arg) != 0)
		{
			fprintf(stderr, "[SERVER ERROR] Failed to enqueue task into thread pool.\n");
			free(task_arg);
			tcp_socket_close(client_sock);
		}
	}

	printf("[SERVER] Stopping new connection acceptance...\n");

	printf("[SERVER] Waiting for active tasks to finish...\n");
	thread_pool_destroy(pool);

	printf("[SERVER] Closing listening socket...\n");
	tcp_socket_close(listen_sock);

	tcp_net_shutdown();
	printf("[SERVER] Server has shut down gracefully and cleanly.\n");

	return 0;
}