#!/bin/bash

# Простой скрипт для тестирования

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TEST_DIR="$SCRIPT_DIR/test_files"

cleanup() {
    kill $SERVER_PID 2>/dev/null || true
    for pid in "${CLIENT_PIDS[@]}"; do
        kill $pid 2>/dev/null || true
    done
    wait 2>/dev/null || true
    rm -rf "$TEST_DIR" "$SCRIPT_DIR/tmp" 2>/dev/null || true
}

trap cleanup EXIT INT TERM

# Проверка
if [ ! -f "$BUILD_DIR/server" ] || [ ! -f "$BUILD_DIR/client" ]; then
    echo "Ошибка: соберите проект сначала"
    exit 1
fi

# Создаем тестовые файлы
mkdir -p "$TEST_DIR"
echo "Файл 1" > "$TEST_DIR/file1.txt"
echo "Файл 2" > "$TEST_DIR/file2.txt"
dd if=/dev/urandom of="$TEST_DIR/file3.bin" bs=1024 count=5 2>/dev/null

# Очистка
rm -rf "$SCRIPT_DIR/tmp" 2>/dev/null || true

# Запуск сервера
cd "$BUILD_DIR"
./server > server.log 2>&1 &
SERVER_PID=$!
sleep 2

# Запуск клиентов
./client "$TEST_DIR/file1.txt" > client1.log 2>&1 &
CLIENT_PIDS+=($!)

sleep 1

./client "$TEST_DIR/file2.txt" "$TEST_DIR/file3.bin" > client2.log 2>&1 &
CLIENT_PIDS+=($!)

# Ждем завершения
for pid in "${CLIENT_PIDS[@]}"; do
    wait $pid 2>/dev/null || true
done

echo "Тест завершен. Логи в build/*.log"
