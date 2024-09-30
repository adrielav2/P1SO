#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h> 
#include <limits.h>


// Estructura para pasar datos a los hilos
typedef struct {
    char **file_list;
    int *current_index;
    int total_files;
    char *dest_dir;
    pthread_mutex_t *mutex;
} thread_data_t;

// Función para copiar un archivo
int copy_file(const char *source, const char *destination) {
    int src_fd = open(source, O_RDONLY);
    if (src_fd == -1) {
        perror("open() source error");
        return -1;
    }

    int dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        perror("open() destination error");
        close(src_fd);
        return -1;
    }

    char buffer[BUFSIZ];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(src_fd, buffer, BUFSIZ)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("write() error");
            close(src_fd);
            close(dest_fd);
            return -1;
        }
    }

    close(src_fd);
    close(dest_fd);

    return 0;
}

// Función para leer directorios y subdirectorios recursivamente
void read_directory(const char *path, char ***file_list, int *file_count) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(path)) == NULL) {
        perror("opendir() error");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat statbuf;
        if (stat(full_path, &statbuf) == -1) {
            perror("stat() error");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Procesa subdirectorios recursivamente
            read_directory(full_path, file_list, file_count);
        } else if (S_ISREG(statbuf.st_mode)) {
            // Agrega el archivo a la lista
            (*file_list) = realloc((*file_list), sizeof(char*) * (*file_count + 1));
            (*file_list)[*file_count] = strdup(full_path);
            (*file_count)++;
        }
    }

    closedir(dir);
}

// Función para que los hilos copien los archivos
void *copy_file_thread(void *arg) {

    thread_data_t *data = (thread_data_t *)arg;
    int index;

    while (1) {
        // Sección para obtener el siguiente archivo
        pthread_mutex_lock(data->mutex);
        if (*(data->current_index) >= data->total_files) {
            pthread_mutex_unlock(data->mutex);
            break; // No hay más archivos por copiar
        }
        index = (*(data->current_index))++;
        pthread_mutex_unlock(data->mutex);

        // Copiar el archivo
        char *source_file = data->file_list[index];

        // Construir la ruta del archivo destino
        char dest_file[PATH_MAX];
        snprintf(dest_file, sizeof(dest_file), "%s/%s", data->dest_dir, strrchr(source_file, '/') + 1);

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start); // Inicio de la medición del tiempo

        if (copy_file(source_file, dest_file) == -1) {
            fprintf(stderr, "Error copiando el archivo: %s\n", source_file);
        } else {
            // Obtener tamaño del archivo
            struct stat statbuf;
            if (stat(source_file, &statbuf) == -1) {
                perror("stat() error");
                continue;
            }

            clock_gettime(CLOCK_MONOTONIC, &end); // Fin de la medición del tiempo
            double time_taken = (end.tv_sec - start.tv_sec) * 1e9;
            time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;

            // Imprimir el nombre del archivo y su tamaño
            printf("Archivo copiado: %s (%ld bytes)\n", source_file, statbuf.st_size);

            // Registrar en el logfile
            FILE *logfile = fopen("logfile.csv", "a");
            if (logfile == NULL) {
                perror("fopen() error");
            } else {
                fprintf(logfile, "%s,%lu,%lf\n", source_file, pthread_self(), time_taken);
                fclose(logfile);
            }
        }
    }

    pthread_exit(NULL);
}

// Función principal
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s directorio_origen directorio_destino\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *src_dir = argv[1];
    char *dest_dir = argv[2];

    // Crear directorio destino si no existe
    struct stat st = {0};
    if (stat(dest_dir, &st) == -1) {
        if (mkdir(dest_dir, 0755) == -1) {
            perror("mkdir() error");
            exit(EXIT_FAILURE);
        }
    }

    // Leer archivos del directorio origen
    char **file_list = NULL;
    int file_count = 0;
    read_directory(src_dir, &file_list, &file_count);

    // Variables de sincronización mutex
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    int current_index = 0;

    // Crear pool de hilos
    int num_threads = 4; 
    pthread_t threads[num_threads];
    thread_data_t thread_data = { file_list, &current_index, file_count, dest_dir, &mutex };

    // Crear y lanzar hilos
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, copy_file_thread, (void *)&thread_data);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Liberar memoria
    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);

    printf("Copia completa.\n");
    return 0;
}
