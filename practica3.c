#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define COORDINADOR 20 // anunciar lider elegido
#define MENSAJE 30     // mensaje de eleccion

int simula_sleep(int segundos)
{
    if (segundos <= 0)
        return 0.0;
    struct timespec req = {.tv_sec = segundos, .tv_nsec = 0};
    nanosleep(&req, NULL);
    return segundos;
}

// Función para determinar si un nodo está caído (similar a nodo_inactivo)
int nodo_caido(int id, int total)
{
    if (id == total - 1) // Si es el último nodo, cae
    {
        return 1;
    }
    double r = (double)rand() / RAND_MAX;
    return (r < 0.2) ? 1 : 0; // en otro caso, tiene 1/5 de probabilidad de caer
}

// Función para obtener el timeout basado en si el nodo está caído
int timeout(int caido)
{
    return (caido) ? 5 : 2;
}

int main(int argc, char **argv)
{

    int size, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    srand((unsigned)time(NULL) + rank * 101);

    int caido = nodo_caido(rank, size);
    int tiempo = timeout(caido);
    int coordinador = -1;

    // double tiempo_dormido = simula_sleep(tiempo);
    printf("[Nodo %d] Dormido por %d segundos (caido = %d, timeout=%d)\n", rank, simula_sleep(tiempo), caido, tiempo);
    fflush(stdout);

    MPI_Barrier(MPI_COMM_WORLD);

    /* si el nodo está caído, se "apaga": no participa (simulación) */
    if (caido)
    {
        printf("[Nodo %d] estoy caído, no participo.\n", rank);
        fflush(stdout);
        /* Simular que permanece inactivo por un tiempo mayor */
        simula_sleep(6);
        MPI_Finalize();
        return 0;
    }

    // inciamos las comunicaciones
    for (int i = rank - 1; i >= 0; i--)
    {
        // enviamos retrasos de red
        simula_sleep(0);
        MPI_Send(&tiempo, 1, MPI_INT, i, MENSAJE, MPI_COMM_WORLD);
    }

    int bully = (caido) ? 0 : 1;
    MPI_Status status;

    // Esperar mensajes de nodos con rango superior
    for (int j = rank + 1; j < size; j++)
    {
        int recibido = 0;
        int flag = 0;
        double waited = 0.0;
        int remote_timeout = 1;

        // esperar a remote_timeout si el nodo emisor tiene timeout
        // como no se conoce el timeout hasta recibirlo, usamos el limite de maxima espera simulada por el nodo

        const int MAX_WAIT = 7;
        while (waited < MAX_WAIT)
        {
            MPI_Iprobe(j, MENSAJE, MPI_COMM_WORLD, &flag, &status);
            if (flag)
            {
                MPI_Recv(&remote_timeout, 1, MPI_INT, j, MENSAJE, MPI_COMM_WORLD, &status);
                recibido = 1;
                break;
            }

            simula_sleep(1);
            waited += 1.0;
        }

        if (!recibido)
        {
            // No llegó el mensaje del nodo dentro del máximo simulado, por lo que se toma como caído
            // esto se considera para ser bully si no hay respuestas válidas
            continue;
        }
        else
        {
            if (remote_timeout <= 3)
            {
                bully = 0; // hay un nodo activo con timeout menor
                break;
            }
        }
    }

    if (bully)
    {
        printf("[Nodo %d] Soy el nuevo coordinador\n", rank);
        coordinador = rank;
        for (int i = 0; i < size; i++)
        {
            if (i != rank)
            {
                MPI_Send(&coordinador, 1, MPI_INT, i, COORDINADOR, MPI_COMM_WORLD);
            }
        }
    }
    else
    {
        int aux = -1;
        for (int i = 0; i < size; i++)
        {
            MPI_Send(&aux, 1, MPI_INT, i, COORDINADOR, MPI_COMM_WORLD);
        }

        // recibimos respuestas
        int respuesta_recibida = 0;

        while (!respuesta_recibida)
        {
            int flag = 0;
            MPI_Iprobe(MPI_ANY_SOURCE, COORDINADOR, MPI_COMM_WORLD, &flag, &status);
            if (flag)
            {
                int respuesta = -1;
                MPI_Recv(&respuesta, 1, MPI_INT, status.MPI_SOURCE, COORDINADOR, MPI_COMM_WORLD, &status);
                if (respuesta != -1)
                {
                    coordinador = respuesta;
                    respuesta_recibida = 1;
                    printf("[Nodo %d] El nuevo coordinador es el nodo %d\n", rank, coordinador);
                    break;
                }
            }
        }
    }

    MPI_Finalize();
    return 0;
}