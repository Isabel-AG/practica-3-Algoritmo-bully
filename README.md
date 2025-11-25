# practica-3-Algoritmo-bully

Implementacion en C del algoritmo del abuson 

## Compilacion 

mpicc -o practica3 Practica3.c

mpicc -Wall -Wextra -g -o practica3 Practica3.c

## Ejecucion con --oversubscribed
### Con 4 procesos (1 traidor máximo)
```bash
mpirun --oversubscribe -np 4 ./practica3
```

### Con 7 procesos (1 traidor máximo)
```bash
mpirun --oversubscribe -np 7 ./practica3
```

### Con 10 procesos (2 traidores máximo)
```bash
mpirun --oversubscribe -np 10 ./practica3
```

### Con 13 procesos (3 traidores máximo)
```bash
mpirun --oversubscribe -np 13 ./practica3
```