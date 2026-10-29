# SMTF клиент

## 1. Использованный SMTP-сервер

Для тестирования и демонстрации работы SMTP-клиента используется **локальный тестовый SMTP-сервер**, написанный на языке
**Python 3** с использованием стандартного модуля `socket`.

* **Хост:** `127.0.0.1` (localhost)
* **Порт:** `25` (стандартный SMTP-порт)
* **Поддерживаемые команды:** `HELO`/`EHLO`, `MAIL FROM`, `RCPT TO`, `DATA`, `QUIT`.

Сервер из файла `smtp_server.py` поддерживает обработку перевода строк,
считывание почтовых заголовков и тела сообщения,
завершаемого точкой на отдельной строке.

## Структура проекта

```text
.
├── CMakeLists.txt
├── common/
│   ├── include/
│   │   └── common/
│   │       ├── tcp/
│   │       │   ├── socket.h
│   │       │   └── Socket.hpp
│   │       └── config.h
│   └── src/
│       ├── Platform/
│       │   └── Win32/
│       │       └── socket.c
│       └── Socket.cpp
└── smtp/
    ├── smtp_client.cpp
    └── smtp_server.py
```

## 2. Инструкция по сборке и запуску

### Шаг 1: Запуск SMTP-сервера

Запустите Python-сервер в отдельном терминале
(для привязки к порту 25 могут потребоваться права администратора / `sudo`):

```bash
# Windows (PowerShell / CMD от имени Администратора)
python smtp_server.py

# Linux / macOS
sudo python3 smtp_server.py
```

### Шаг 2: Сборка и запуск C++ клиента через CMake

#### Требования к окружению:

* Компилятор C++ с поддержкой стандарта **C++23** (MSVC / GCC 13+ / Clang 16+).
* Система сборки **CMake** (версии 3.20 и выше).

```bash
mkdir build && cd build
cmake ..
cmake --build .

./smtp_client      # Linux/macOS
.\smtp_client.exe  # Windows
```

## 3. Примеры работы

### Вывод SMTP-сервера:

```text
SMTP server is serving on port 25...

[+] Connection made with ('127.0.0.1', 54210)
C: HELO client.example.com
C: MAIL FROM: <sender@example.com>
C: RCPT TO: <recipient@example.com>
C: DATA
--- MSG RECEIVED ---
From: sender@example.com
To: recipient@example.com
Subject: Lab 7 Test Email (RAII Wrapper)

Hello! This is a test message from custom C++ SMTP Client!
--------------------
[-] Connection closed with ('127.0.0.1', 54210)
```

### Вывод C++ клиента:

```text
S: 220 localhost ESMTP Custom Server
C: HELO client.example.com
S: 250 Hello client
C: MAIL FROM: <sender@example.com>
S: 250 OK
C: RCPT TO: <recipient@example.com>
S: 250 OK
C: DATA
S: 354 Start mail input; end with <CRLF>.<CRLF>
C: From: sender@example.com
To: recipient@example.com
Subject: Lab 7 Test Email (RAII Wrapper)

Hello! This is a test message from custom C++ SMTP Client!
.
S: 250 OK: message accepted for delivery
C: QUIT
S: 221 Bye
Session closed
```

## 4. Описание архитектуры

**Обработка протокола SMTP:** Клиент последовательно проверяет ответы сервера (`220`, `250`, `354`, `221`) и
выполняет правильное завершение сессии командой `QUIT`.