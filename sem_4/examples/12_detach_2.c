// Поведение потока также можно настраивать через атрибуты:

void* detached_worker(void* arg) {
    while(1) {
      printf("Detached thread working\n");
      sleep(1);
    }
  pthread_exit(NULL);  // Still good practice
}

int main() {
    pthread_t thread;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    pthread_create(&thread, &attr, detached_worker, NULL);
    pthread_exit(NULL);
    
    sleep(1);
    return 0;
}
