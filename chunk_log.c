#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_THREADS 8
#define BUFFER_SIZE 4096  // Increased buffer size for long lines

typedef struct {
    char *filename;
    char *keyword;
    long start;
    long end;
    int count;
} ThreadData;

// Thread function
void *search_chunk(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    // Open file in binary mode for reliable ftell and fseek
    FILE *file = fopen(data->filename, "rb"); // CHANGED
    if (!file) {
        perror("File open error");
        pthread_exit(NULL);
    }

    fseek(file, data->start, SEEK_SET);

    // Skip partial line if not at the start
    if (data->start != 0) {  // FIXED
        char c;
        while (fread(&c, 1, 1, file) == 1 && c != '\n');
    }

    long pos = ftell(file);
    int local_count = 0;
    int key_len = strlen(data->keyword);
    char buffer[BUFFER_SIZE];

    while (pos < data->end && fgets(buffer, sizeof(buffer), file)) {
        char *temp = buffer;
        while ((temp = strstr(temp, data->keyword)) != NULL) {
            local_count++;
            temp += key_len;
        }
        pos = ftell(file);
    }

    data->count = local_count;
    fclose(file);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <file> <keyword> <threads>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    char *keyword = argv[2];
    int num_threads = atoi(argv[3]);

    if (num_threads > MAX_THREADS) {
        num_threads = MAX_THREADS;
    }

    // Open in binary mode for accurate file size
    FILE *file = fopen(filename, "rb"); // CHANGED
    if (!file) {
        perror("File error");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fclose(file);

    pthread_t threads[num_threads];
    ThreadData data[num_threads];

    long chunk_size = file_size / num_threads;

    for (int i = 0; i < num_threads; i++) {
        data[i].filename = filename;
        data[i].keyword = keyword;
        data[i].start = i * chunk_size;
        data[i].end = (i == num_threads - 1) ? file_size : (i + 1) * chunk_size;
        data[i].count = 0;

        // Check pthread_create return value
        if (pthread_create(&threads[i], NULL, search_chunk, &data[i]) != 0) { // FIXED
            perror("Thread creation failed");
            return 1;
        }
    }

    int total = 0;

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total += data[i].count;
    }

    printf("Total occurrences of '%s': %d\n", keyword, total);

    return 0;
}
