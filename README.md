# Proyecto Multihilos para Copia de Archivos

## Descripción
Este proyecto implementa una versión multihilos del programa `Proyecto.c`, que permite copiar el contenido de un directorio completo de manera eficiente utilizando múltiples hilos. El programa maneja recursivamente tanto archivos como subdirectorios, con el objetivo de determinar la cantidad de hilos ideal para maximizar el rendimiento en términos de tiempo de ejecución.

## Requisitos
- Sistema operativo Unix
- Compilador de C (gcc)

## Compilación
Para compilar el programa, abre una terminal y navega al directorio donde se encuentra `Proyecto.c`. Luego, ejecuta el siguiente comando:
```bash
gcc -o Proyecto Proyecto.c -lpthread
```

## Ejecución
Para ejecutar el programa, utiliza el siguiente comando: 
```bash
./Proyecto <directorio_origen> <directorio_destino>
```

Para cambiar el número de hilos se hace dentro del codigo en main() en la variable num_threads.

