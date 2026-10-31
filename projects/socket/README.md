# Многопоточный TCP Клиент-Сервер

## 1. Описание архитектуры

Реализовано многопоточное клиент-серверное приложение на языке C (Си) с использованием платформо-независимых сетевых
абстракций и пула потоков.

### Ключевые компоненты и архитектурные решения:

1. **Сетевой уровень (Платформо-независимые интерфейсы):**

* Инициализация сетевой подсистемы выполняется вызовом `tcp_net_init`.
* Завершение работы сети реализуется через функцию `tcp_net_shutdown`.
* Управление соединениями использует абстрактные интерфейсы для создания, привязки, прослушивания и закрытия сокетов.

2. **Параллельная обработка:**

* Конкурентная обработка запросов клиентов обеспечивается пулом потоков `thread_pool_t`.
* Размер пула потоков фиксирован и составляет 16 потоков.
* Главный поток принимает входящие подключения и передает их задачи в пул через `thread_pool_enqueue` без блокировки
  основного цикла.

3. **Обработка сообщений и протокол взаимодействия:**

* Обмен данными между клиентом и сервером построен на структуре пакетов `message_packet_t`.
* Пакет содержит строку с именем участника и целочисленное значение.
* Клиент запрашивает у пользователя число от 1 до 100, отправляет его и выводит сумму с числом сервера.
* Сервер суммирует число клиента с фиксированным значением `50` и возвращает ответ.

4. **Таймауты и Graceful Shutdown:**

* Контроль активности клиентов реализован через функцию `tcp_socket_wait_for` с таймаутом в 10 секунд (`10000` мс).
* Прерывание работы сервера обрабатывается через перехват сигнала `SIGINT`.
* Функция `thread_pool_destroy` корректно дожидается завершения активных задач в потоках и освобождает системные
  ресурсы.

## 2. Структура проекта

```text
├── CMakeLists.txt
├── common/
│   ├── include/
│   │   └── common/
│   │       └── tcp/
│   │           └── socket.h
│   └── src/
│       └── Platform/
│           └── Win32/
│               └── socket.c
└── socket/
    ├── client.c
    └── server.c
```

## 3. Инструкция по сборке и запуску

### Требования к окружению:

* Используется язык программирования C (Си).
* Требуется система сборки **CMake** (версии 3.20 и выше).

### Сборка проекта

```bash
# 1. Создание и переход в директорию сборки
mkdir build && cd build

# 2. Генерация файлов сборки через CMake
cmake ..

# 3. Компиляция программ
cmake --build .
```

### Запуск приложения

```bash
# Запуск сервера (linux)
./socket_server
# Windows
.\socket_server.exe

# Запуск сервера (linux)
./socket_client
# Windows
.\socket_client.exe
```

## 4. Примеры работы

### 1. Вывод сервера:

```text
[SERVER] Initializing network subsystem...
[SERVER] Creating thread pool with 16 threads...
[SERVER] Binding socket to port 5050...
[SERVER] Starting to listen for incoming connections...
[SERVER] Advanced Concurrent Server '### TCP Socket Server ###' is running on port 5050...
[SERVER] Press Ctrl+C to trigger Graceful Shutdown.

[SERVER] Received message from client '*** TCP Socket Client ***' (number: 42)
[SERVER] Response sent successfully to '*** TCP Socket Client ***' (Sum: 92)

^C
[SERVER] Stopping new connection acceptance...
[SERVER] Waiting for active tasks to finish...
[SERVER] Closing listening socket...
[SERVER] Server has shut down gracefully and cleanly.

```

### 2. Вывод клиента:

```text
[CLIENT] Initializing network subsystem...
[CLIENT] Enter an integer between 1 and 100:
42
[CLIENT] Creating TCP socket...
[CLIENT] Connecting to server at 127.0.0.1:5050...
[CLIENT] Connected successfully.
[CLIENT] Sending message (Name: '*** TCP Socket Client ***', Number: 42)...
[CLIENT] Message sent successfully. Waiting for server response...

--- RESULT ON CLIENT ---
Client Name   : *** TCP Socket Client ***
Server Name   : ### TCP Socket Server ###
Client Number : 42
Server Number : 50
Sum           : 42 + 50 = 92
------------------------

[CLIENT] Closing socket...
[CLIENT] Finished successfully.

```