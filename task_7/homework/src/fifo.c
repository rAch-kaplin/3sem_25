#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include "log.h"

// Функция для создания FIFO с предварительным созданием директорий
int create_fifo_with_path(const char *fifo_path) {
    // Сначала создаем директории, если их нет
    char path_copy[256];
    strncpy(path_copy, fifo_path, sizeof(path_copy));

    // Находим последний слэш, чтобы отделить имя файла от пути
    char *last_slash = strrchr(path_copy, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';  // Обрезаем после последнего слэша

        // Рекурсивно создаем директории
        char *current = path_copy;
        while (*current == '/') current++;  // Пропускаем начальные слэши

        while (1) {
            // Ищем следующий слэш
            char *next_slash = strchr(current, '/');
            if (next_slash != NULL) {
                *next_slash = '\0';  // Временно обрезаем строку
            }

            // Пытаемся создать директорию
            mkdir(path_copy, 0755);

            if (next_slash == NULL) break;  // Больше нет слэшей

            *next_slash = '/';  // Восстанавливаем слэш
            current = next_slash + 1;  // Переходим к следующей части пути
        }
    }

    // Теперь создаем сам FIFO (именованный канал)
    // mkfifo создает специальный файл типа FIFO (именованный канал)
    if (mkfifo(fifo_path, 0666) == -1) {
        if (errno != EEXIST) {  // EEXIST значит FIFO уже существует - это нормально
            ELOG("mkfifo failed for %s: %s", fifo_path, strerror(errno));
            return -1;
        }
        ILOG("FIFO already exists: %s", fifo_path);
    } else {
        ILOG("Created FIFO: %s", fifo_path);
    }

    return 0;
}

// Проверка существования файла
int file_exists(const char *path) {
    if (!path) {
        ELOG("file_exists: null path");
        return 0;
    }

    // access() проверяет доступ к файлу по указанному пути
    // F_OK проверяет существование файла
    int result = access(path, F_OK);
    if (result == -1 && errno != ENOENT) {
        ELOG("access() failed for %s: %s", path, strerror(errno));
    }
    return result != -1;
}

// Удаление FIFO и пустых директорий
void cleanup_fifo(const char *fifo_path) {
    if (!fifo_path) {
        ELOG("cleanup_fifo: null path");
        return;
    }

    // Удаляем сам FIFO файл
    if (unlink(fifo_path) == -1) {
        if (errno != ENOENT) {
            ELOG("Failed to unlink FIFO %s: %s", fifo_path, strerror(errno));
        }
    } else {
        ILOG("Removed FIFO: %s", fifo_path);
    }

    // Теперь удаляем пустые директории выше по пути
    char path_copy[256];
    strncpy(path_copy, fifo_path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    while (1) {
        // Находим последний слэш
        char *last_slash = strrchr(path_copy, '/');
        if (last_slash == NULL) break;

        *last_slash = '\0';  // Обрезаем после слэша

        // Проверяем, пустая ли директория
        DIR *dir = opendir(path_copy);
        if (dir == NULL) break;

        int is_empty = 1;
        struct dirent *entry;

        // Читаем содержимое директории
        while ((entry = readdir(dir)) != NULL) {
            // Пропускаем текущую и родительскую директории
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                is_empty = 0;  // Нашли файл - директория не пустая
                break;
            }
        }

        closedir(dir);

        // Если директория не пустая, прекращаем удаление
        if (!is_empty) break;

        // Удаляем пустую директорию
        if (rmdir(path_copy) == -1) {
            if (errno != ENOENT && errno != ENOTEMPTY) {
                ELOG("Failed to rmdir %s: %s", path_copy, strerror(errno));
            }
            break;
        }
    }
}
