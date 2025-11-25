# practica-3-Algoritmo-bully

Implementacion en C del algoritmo del abuson 

## Compilacion 

mpicc -o practica3 Practica3.c

mpicc -Wall -Wextra -g -o practica3 Practica3.c

## Ejecucion con --oversubscribed
### Con 4 procesos (maximo 1 nodo caido)
```bash
mpirun --oversubscribe -np 4 ./practica3
```

### Con 7 procesos (1-2 nodos caidos)
```bash
mpirun --oversubscribe -np 7 ./practica3
```

### Con 10 procesos (1-2 nodos caidos)
```bash
mpirun --oversubscribe -np 10 ./practica3
```

### Con 13 procesos (3-4 nodos caidos)
```bash
mpirun --oversubscribe -np 13 ./practica3
```