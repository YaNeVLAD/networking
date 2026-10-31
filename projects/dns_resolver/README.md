# Итеративный DNS Резолвер

## 1. Описание архитектуры

Реализовано C++23 приложение для итеративного разрешения доменных имен (DNS Resolver) с поддержкой протоколов UDP и TCP,
обработки усеченных пакетов (TC Flag), автоматического перехода на TCP (TCP Fallback) и разрешения имен DNS-серверов.

### Ключевые компоненты и архитектурные решения:

1. **Сетевой уровень (Платформо-независимые интерфейсы):**

* Использование платформ-независимых классов-обёрток `udp::Socket` и `tcp::Socket`.
* Преобразование сетевого порядка байт вынесено в модульные функции `net::conv::to_network` и `net::conv::to_host`.
* Реализация транспортного уровня в классе `dns::Client` инкапсулирует логику отправки UDP и TCP запросов, управление
  таймаутами и обработку переключения протоколов.

2. **DNS Кодек и Кодирование/Декодирование (`dns::codec`):**

* Формирование DNS-запросов и парсинг бинарных DNS-пакетов через безопасные интерфейсы `std::span<const std::byte>` и
  `std::vector<std::byte>`.
* Полная поддержка сжатия доменных имен (DNS Name Compression / Pointers `0xC0`) с защитой от циклов (Pointer loops).
* Поддержка разбора основных типов DNS-записей: `A`, `AAAA`, `NS`, `CNAME`, `SOA`, `PTR`, `MX`, `TXT`, `RRSIG`, `ANY`.
* Безопасная обработка ошибок с использованием `std::expected` (C++23) и типов `dns::Error` / `dns::ErrorCode`.

3. **Итеративное разрешение (`dns::IterativeResolver`):**

* Разрешение имён начинается с 13 корневых DNS-серверов.
* Алгоритм последовательно запрашивает authoritative/referral DNS-серверы текущего уровня TLD, обрабатывает секции
  `Answers`, `Authority` (NS-записи) и `Additional`.
* Поддержка перехода по `CNAME`-псевдонимам.
* Автоматическое рекурсивное разрешение IP-адресов NS-серверов.
* Защита от бесконечных циклов и циклической зависимости (Circular DNS dependency) с использованием ограничения по
  количеству шагов `MAX_STEPS = 32`.

4. **Обработка флага TC и TCP Fallback:**

* При получении UDP-ответа с установленным флагом усечения, клиент автоматически
  выполняет повторный запрос к тому же серверу через TCP.
* Форматирование TCP-пакета выполняется с 2-байтовым префиксом длины кадра согласно RFC 1035 / RFC 7766.

## 2. Структура проекта

```text
├── CMakeLists.txt
├── common/
│   ├── net/
│   │   └── Utils.hpp
│   ├── tcp/
│   │   └── Socket.hpp
│   └── udp/
│       └── Socket.hpp
└── smtp
    ├── include/
    │   ├── Client.hpp
    │   ├── Codec.hpp
    │   ├── IterativeResolver.hpp
    │   └── Types.hpp
    └── src/
        ├── Client.cpp
        ├── Codec.cpp
        ├── IterativeResolver.cpp
        └── dns_resolver.cpp
```

## 3. Инструкция по сборке и запуску

### Требования к окружению:

* Язык программирования: **C++23** (поддержка `std::expected`, `std::span`, `std::ranges`, `std::format`).
* Компилятор: **GCC 13+**, **Clang 16+** или **MSVC 2022+**.
* Система сборки: **CMake** (версии 3.20 и выше).

### Сборка проекта

```bash
# 1. Создание и переход в директорию сборки
mkdir build && cd build

# 2. Генерация файлов сборки через CMake
cmake ..

# 3. Компиляция программы
cmake --build .
```

### Запуск приложения

```bash
# Запуск резолвера (Linux / macOS)
./dns_resolver example.com A
./dns_resolver google.com AAAA
./dns_resolver wikipedia.org NS

# Запуск резолвера (Windows)
.\dns_resolver.exe example.com A
.\dns_resolver.exe google.com AAAA
```

## 4. Примеры работы

### 1. Успешное разрешение A-записи домена `example.com`:

```text
$ ./dns_resolver example.com A
8.6.112.0
8.47.69.0
```

### 2. Запрос AAAA (IPv6) записи:

```text
$ ./dns_resolver google.com AAAA
2a00:1450:4001:c17::66
2a00:1450:4001:c17::71
2a00:1450:4001:c17::65
2a00:1450:4001:c17::64
```

### 3. Обработка ошибки несуществующего домена (NXDOMAIN):

```text
$ ./dns_resolver non-existent-domain-12345.com A
NXDOMAIN: non-existent-domain-12345.com
```

### 4. Вывод справки при некорректных аргументах:

```text
$ ./dns_resolver
Usage: dns_resolver.exe <domain> <record_type>
Example: dns_resolver.exe example.com A
```