# HTTP Веб-сервер

## 1. Описание архитектуры

Реализован многопоточный HTTP/1.1 веб-сервер на языке C++23 с использованием
сетевых абстракций `tcp::Listener`, `tcp::Socket` и пула потоков `execution::ThreadPool`.

### Ключевые компоненты и архитектурные решения:

1. **Сетевой уровень (RAII-обёртки):**

* `tcp::Listener` — абстракция слушающего сокета. Выполняет привязку к порту (`bind`), настройку опций (`SO_REUSEADDR`)
  и прослушивание входящих соединений (`listen`).
* `tcp::Socket` — RAII-обёртка над клиентским сокетом с поддержкой move-семантики. Автоматически закрывает дескриптор в
  деструкторе, предотвращая утечки ресурсов.

2. **Параллельная обработка (Concurrent Server):**

* Для обеспечения конкурентной работы нескольких клиентов используется пул потоков `execution::ThreadPool`.
* Главный поток находится в цикле принятия соединений (`listener.accept()`). При поступлении нового клиента принятый
  `tcp::Socket` передается через `std::move` в задачу пула потоков, исключая блокировку главного цикла.

3. **Обработка HTTP-запросов и файлов:**

* **Парсинг:** Извлекается метод и путь запрашиваемого ресурса. Поддерживаются только `GET`-запросы, на прочие методы
  возвращается `405 Method Not Allowed`.
* **Определение MIME-типов:** Используется плоская хеш-таблица для быстрого сопоставления расширения файла
  с соответствующим заголовком `Content-Type` (`text/html`, `text/css`, `image/png`, `image/jpeg` и др.).
* **Чтение бинарных данных:** Файлы считываются в режиме `std::ios::binary` через бинарный вектор `std::vector<char>`,
  что обеспечивает корректную отдачу бинарных ресурсов без искажений.
* **Ответы сервера:** При успешном поиске файла формируется заголовок `HTTP/1.1 200 OK` с точным `Content-Length`. Если
  файл отсутствует, отдается `HTTP/1.1 404 Not Found`.

4. **Graceful Shutdown:**

* Сервер перехватывает сигнал `SIGINT` с помощью `std::signal`.
  При получении сигнала главный цикл штатно прерывается, завершает текущие задачи в пуле и корректно освобождает все
  ресурсы.

## 2. Структура проекта

```text
├── CMakeLists.txt
├── common/
│   ├── include/
│   │   └── common/
│   │       ├── execution/
│   │       │   ├── ThreadPool.hpp
│   │       │   └── c/
│   │       │       └── thread_pool.h
│   │       ├── flat_map.hpp
│   │       └── tcp/
│   │           ├── Listener.hpp
│   │           ├── Socket.hpp
│   │           └── socket.h
│   └── src/
│       ├── Platform/
│       │   └── Win32/
│       │       ├── socket.c
│       │       └── thread_pool.h
│       ├── Socket.cpp
│       ├── Listener.cpp
│       └── ThreadPool.cpp
└── webserver/
    ├── webserver.cpp
    └── index.html
```

## 3. Инструкция по сборке и запуску

### Требования к окружению:

* Компилятор C++ с поддержкой стандарта **C++23** (MSVC / GCC 13+ / Clang 16+).
* Система сборки **CMake** (версии 3.20 и выше).

### Сборка проекта

```bash
# 1. Создание и переход в директорию сборки
mkdir build && cd build

# 2. Генерация файлов сборки через CMake
cmake ..

# 3. Компиляция сервера
cmake --build .
```

### Запуск сервера

```bash
# Запуск на Linux / macOS
./webserver

# Запуск на Windows
.\webserver.exe
```

## 4. Примеры работы

### 1. Вывод сервера:

```text
WEB server is running on http://localhost:8000 ...
[LOG] Received: GET /index.html
[LOG] 200 OK File sent: index.html (text/html; charset=utf-8)
[LOG] Received: GET /missing-page.html
[LOG] 404 Not Found: missing-page.html
^C
```

### 2. Запрос главной страницы (`HTTP 200 OK`):

* **Запрос от браузера:** `GET /index.html HTTP/1.1`
* **Ответ сервера:**

```http
HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 1048

<!DOCTYPE html>
<html lang="ru">
...
```

### 3. Запрос несуществующего файла (`HTTP 404 Not Found`):

* **Запрос от браузера:** `GET /missing-page.html HTTP/1.1`
* **Ответ сервера:**

```http
HTTP/1.1 404 Not Found
Content-Type: text/plain; charset=utf-8
Content-Length: 14

File Not Found
```