# Демон мониторинга файлов

Программа-демон для мониторинга изменений текстовых файлов в рабочей директории указанного процесса.

## Сборка

```bash
cd homework
cmake -S . -B build
cmake --build build
```

## Использование

### Интерактивный режим (по умолчанию)

```bash
./daemon -p <PID>
```

или

```bash
./daemon -p <PID> -i
```

### Режим демона

```bash
./daemon -p <PID> -d
```

## Команды в интерактивном режиме

- `status` - показать текущий статус
- `show_diff` - показать последний diff
- `show_k_diffs <k>` - показать последние k диффов
- `set_pid <pid>` - изменить отслеживаемый PID
- `set_period <ms>` - изменить период семплирования (в миллисекундах)
- `quit` или `exit` - выйти
- Нажать Enter - сделать снимок вручную

## Команды для демона (через FIFO)

Демон создает FIFO `/tmp/daemon_monitor_fifo` для приема команд:

```bash
echo "status" > /tmp/daemon_monitor_fifo
echo "show_diff" > /tmp/daemon_monitor_fifo
echo "show_k_diffs 5" > /tmp/daemon_monitor_fifo
echo "set_pid <pid>" > /tmp/daemon_monitor_fifo
echo "set_period <ms>" > /tmp/daemon_monitor_fifo
```

## Бекапы

Бекапы сохраняются в `/tmp/daemon_backups/`:
- `full_*` - полные копии файлов
- `diff_*_sample_<N>` - инкрементальные изменения

## Логи

Логи записываются в `debug.log` в текущей директории.

Для подробной инструкции по тестированию см. [TESTING.md](TESTING.md).


