# Инструкция по тестированию демона

### Вариант 1: Запуск процесса в test_monitor и мониторинг его PID

```bash
# Терминал 1: Создайте тестовую директорию и запустите процесс в ней
mkdir -p ~/test_monitor
cd ~/test_monitor
echo "Hello World" > file1.txt
echo "Test line" > file2.txt

# Запустите процесс, который будет работать в этой директории
# Например, запустите sleep в фоне и запомните его PID
sleep 3600 &
TEST_PID=$!
echo "Test process PID: $TEST_PID"
echo "Current directory: $(pwd)"

# Терминал 2: Запустите демон с этим PID
cd /home/rach/rAch-kaplin/LinuxCourse/3sem_25/task_6/homework/build
./daemon -p $TEST_PID -d

# Или в интерактивном режиме:
./daemon -p $TEST_PID -i
```

### Вариант 2: Использование текущей оболочки

```bash
# В терминале перейдите в test_monitor
cd ~/test_monitor
mkdir -p ~/test_monitor
echo "Hello World" > file1.txt
echo "Test line" > file2.txt

# Запустите демон с PID текущей оболочки ($$)
cd /home/rach/rAch-kaplin/LinuxCourse/3sem_25/task_6/homework/build
./daemon -p $$ -i

# Теперь в другом терминале изменяйте файлы в ~/test_monitor
```

## Проверка работы

После запуска демона:

1. **Проверьте логи** - должны быть сообщения о мониторинге правильной директории:

2. **Создайте/измените файлы** в `~/test_monitor`:
   ```bash
   echo "Another line" >> ~/test_monitor/file1.txt
   echo "Modified content" > ~/test_monitor/file2.txt
   touch ~/test_monitor/file3.txt
   echo "New file content" > ~/test_monitor/file3.txt
   ```

3. **Проверьте бекапы**:
   ```bash
   ls /tmp/daemon_backups/
   # Должны быть:
   # full_file1.txt
   # full_file2.txt
   # full_file3.txt
   # diff_file1.txt_sample_N
   # diff_file2.txt_sample_N
   # diff_file3.txt_sample_N
   ```

4. **Проверьте содержимое бекапов**:
   ```bash
   cat /tmp/daemon_backups/full_file1.txt
   cat /tmp/daemon_backups/diff_file1.txt_sample_1
   ```

## Команды для демона (через FIFO)

```bash
# Статус
echo "status" > /tmp/daemon_monitor_fifo

# Показать последний diff
echo "show_diff" > /tmp/daemon_monitor_fifo

# Показать последние 2 diff-а
echo "show_k_diffs 2" > /tmp/daemon_monitor_fifo

# Изменить период семплирования на 2 секунды
echo "set_period 2000" > /tmp/daemon_monitor_fifo
```
