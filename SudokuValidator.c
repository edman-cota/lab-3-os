#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <omp.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

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

// Función para verificar todas las columnas
int checkColumns()
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

// Función principal
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

    // Validar el Sudoku
    int validRows = checkRows();
    int validColumns = checkColumns();
    int validSubgrids = checkSubgrids();

    if (validRows && validColumns && validSubgrids)
    {
        printf("El Sudoku es válido.\n");
    }
    else
    {
        printf("El Sudoku no es válido.\n");
    }

    return 0;
}