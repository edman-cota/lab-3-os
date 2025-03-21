#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#define SIZE 9
#define SUBGRID_SIZE 3

int sudoku[SIZE][SIZE];

// Función para verificar si un arreglo de 9 elementos contiene todos los números del 1 al 9
int isValid(int *array)
{
    int check[SIZE + 1] = {0};
    for (int i = 0; i < SIZE; i++)
    {
        if (array[i] < 1 || array[i] > SIZE || check[array[i]]++)
        {
            return 0;
        }
    }
    return 1;
}

// Función para verificar todas las columnas (ejecutada por el hilo)
void *checkColumnsThread(void *arg)
{
    int column[SIZE];
    for (int j = 0; j < SIZE; j++)
    {
        for (int i = 0; i < SIZE; i++)
        {
            column[i] = sudoku[i][j];
        }
        if (!isValid(column))
        {
            pthread_exit((void *)0); // Sudoku no válido
        }
    }
    pthread_exit((void *)1); // Sudoku válido
}

// Función para verificar todas las filas
int checkRows()
{
    for (int i = 0; i < SIZE; i++)
    {
        if (!isValid(sudoku[i]))
        {
            return 0;
        }
    }
    return 1;
}

// Función para verificar todas las subcuadrículas de 3x3
int checkSubgrids()
{
    int subgrid[SIZE];
    for (int i = 0; i < SIZE; i += SUBGRID_SIZE)
    {
        for (int j = 0; j < SIZE; j += SUBGRID_SIZE)
        {
            int index = 0;
            for (int x = i; x < i + SUBGRID_SIZE; x++)
            {
                for (int y = j; y < j + SUBGRID_SIZE; y++)
                {
                    subgrid[index++] = sudoku[x][y];
                }
            }
            if (!isValid(subgrid))
            {
                return 0;
            }
        }
    }
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <archivo_sudoku>\n", argv[0]);
        return 1;
    }

    // Abrir el archivo
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        perror("Error al abrir el archivo");
        return 1;
    }

    // Mapear el archivo a memoria
    char *file_content = mmap(NULL, 81, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_content == MAP_FAILED)
    {
        perror("Error al mapear el archivo");
        close(fd);
        return 1;
    }

    // Copiar el contenido del archivo a la matriz sudoku
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            sudoku[i][j] = file_content[i * SIZE + j] - '0';
        }
    }

    // Desmapear y cerrar el archivo
    munmap(file_content, 81);
    close(fd);

    // Obtener el número de proceso padre
    pid_t parent_pid = getpid();

    // Crear un proceso hijo
    pid_t child_pid = fork();

    if (child_pid == 0)
    {
        // Proceso hijo
        char parent_pid_str[20];
        snprintf(parent_pid_str, sizeof(parent_pid_str), "%d", parent_pid);

        // Ejecutar el comando ps
        execlp("ps", "ps", "-p", parent_pid_str, "-lLf", NULL);

        // Si execlp falla
        perror("Error al ejecutar execlp");
        exit(1);
    }
    else if (child_pid > 0)
    {
        // Proceso padre

        // Crear un hilo para verificar las columnas
        pthread_t column_thread;
        pthread_create(&column_thread, NULL, checkColumnsThread, NULL);

        // Esperar a que el hilo termine
        void *thread_result;
        pthread_join(column_thread, &thread_result);

        // Mostrar el ID del hilo en ejecución
        printf("ID del hilo en ejecución: %ld\n", syscall(SYS_gettid));

        // Esperar a que el proceso hijo termine
        wait(NULL);

        // Verificar las filas
        int validRows = checkRows();

        // Verificar las subcuadrículas
        int validSubgrids = checkSubgrids();

        // Determinar si el Sudoku es válido
        if ((intptr_t)thread_result && validRows && validSubgrids)
        {
            printf("El Sudoku es válido.\n");
        }
        else
        {
            printf("El Sudoku no es válido.\n");
        }

        // Segundo fork para comparar LWP's
        pid_t second_child_pid = fork();

        if (second_child_pid == 0)
        {
            // Segundo proceso hijo
            char parent_pid_str[20];
            snprintf(parent_pid_str, sizeof(parent_pid_str), "%d", parent_pid);

            // Ejecutar el comando ps
            execlp("ps", "ps", "-p", parent_pid_str, "-lLf", NULL);

            // Si execlp falla
            perror("Error al ejecutar execlp");
            exit(1);
        }
        else if (second_child_pid > 0)
        {
            // Esperar al segundo hijo
            wait(NULL);
        }
        else
        {
            perror("Error en el segundo fork");
            return 1;
        }
    }
    else
    {
        perror("Error en el fork");
        return 1;
    }

    return 0;
}