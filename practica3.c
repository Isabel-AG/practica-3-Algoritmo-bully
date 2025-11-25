#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define TAG_LIDER 20 // anunciar lider elegido 
#define TAG_ELECCION 30 // mensaje de eleccion
#define TAG_OK 40 // respuesta OK

// marca a un nodo como inactivo si es el ultimo nodo o con seleccionado aleatoriamente
int nodo_inactivo(int id, int total){
    int estado = 0;
    if(id == total - 1){
        estado = 1;
    } else {
        double r = (double) rand() / RAND_MAX;
        if(r <= 0.20){
            estado = 1;
        }
    }
    return estado;
}

// obtener retardo segun estado del nodo
int obtener_retardo(int inactivo){
    return (inactivo) ? 5 : 2;
}

// obtener y calcuar tiempo transcurrido
void espera_activa(int segundos){
    struct timespec ini, fin;
    clock_gettime(CLOCK_MONOTONIC, &ini);
    while(1){
        clock_gettime(CLOCK_MONOTONIC, &fin);
        double trans = (fin.tv_sec - ini.tv_sec) + (fin.tv_nsec - ini.tv_nsec)/1e9;
        if(trans >= segundos) break;
    }
}

// funcion principal
int main(int argc, char** argv){

    int total, id;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &total);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    srand(time(NULL) + id);

    int inactivo = nodo_inactivo(id, total);
    int retardo = obtener_retardo(inactivo);
    int lider = -1;

    // Imprimir los nodos caidos (no participan) 
    if(inactivo){
        printf("[Nodo %d] Estado: CAIDO - No participa en comunicacion\n", id);
        MPI_Finalize();
        return 0;
    }

    MPI_Status st;

    int soy_candidato = 1; // Inicialmente todos son candidatos

    // Enviar mensajes de elección a nodos con mayor ID
    for(int destino = id + 1; destino < total; destino++){
        MPI_Send(&id, 1, MPI_INT, destino, TAG_ELECCION, MPI_COMM_WORLD);
        
        // Implementar timeout con MPI_Irecv
        MPI_Request request;
        int respuesta;
        MPI_Irecv(&respuesta, 1, MPI_INT, destino, TAG_OK, MPI_COMM_WORLD, &request);
        
        // Esperar respuesta con timeout de 3 segundos
        int flag = 0;
        time_t inicio = time(NULL);
        while(!flag && (time(NULL) - inicio) < 3){
            MPI_Test(&request, &flag, MPI_STATUS_IGNORE);
            usleep(100000); // 0.1 segundos
        }
        
        if(!flag){
            MPI_Cancel(&request);
            // Si el nodo no responde es porque esta caido 
        } else {
            // Si recibio respuesta no puede ser lider 
            soy_candidato = 0;
        }
    }

    // mensajes de eleccion de nodos con menor ID 
    for(int origen = 0; origen < id; origen++){
        int hay_mensaje;
        MPI_Status status;
        
        // Verificar si hay mensaje de elección
        MPI_Iprobe(origen, TAG_ELECCION, MPI_COMM_WORLD, &hay_mensaje, &status);
        if(hay_mensaje){
            int id_emisor;
            MPI_Recv(&id_emisor, 1, MPI_INT, origen, TAG_ELECCION, MPI_COMM_WORLD, &status);
            
            // Responder con OK ya el del nodo ID es mayor
            int ok = 1;
            MPI_Send(&ok, 1, MPI_INT, origen, TAG_OK, MPI_COMM_WORLD);
        }
    }

    // Anunciar líder si es candidato
    if(soy_candidato){
        lider = id;
        printf("[Nodo %d] Ahora soy el líder\n", id);
        for(int p = 0; p < total; p++){
            if(p != id){  // No enviarse a sí mismo
                MPI_Send(&lider, 1, MPI_INT, p, TAG_LIDER, MPI_COMM_WORLD);
            }
        }
    }
    else { // Esperar anuncio del líder
        MPI_Recv(&lider, 1, MPI_INT, MPI_ANY_SOURCE, TAG_LIDER, MPI_COMM_WORLD, &st);
    }

    if(!inactivo){ // Solo los nodos activos imprimen el estado final
        printf("[Nodo %d] Estado: VIVO, Líder elegido: %d\n", id, lider);
    }

    MPI_Finalize();
    return 0;
} 