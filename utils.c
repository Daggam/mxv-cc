#include <stdlib.h>
#include <stdio.h>
#define llint long long int
/*
    Función para poblar la matriz.
    Inicializo todos con 0's y luego con el valor densidad le asigno aleatoriamente los valores.
    Densidad es un valor que varia desde 0.0 hasta 1.0. A mayor densidad, mayor será la cantidad de valores no nulos.
*/
void poblar_matriz(int (*m)[M])
{
    //Inicializamos la matrices con 0.
    for(int i=0;i<N;i++)
        for(int j=0;j<M;j++)
            m[i][j] = 0;
    
    if (M_DENSITY==0.0) return;

    llint* posiciones = (llint*)malloc(N*M*sizeof(llint));
    for(llint i=0;i<N*M;i++) posiciones[i] = i;
    
    //Algoritmo Fisher-Yates para mezclar el arreglo en O(n)
    for(llint i=N*M-1;i>0;i--)
    {
        llint j = rand()%(i+1);
        llint aux = posiciones[j];
        posiciones[j] = posiciones[i];
        posiciones[i] = aux;
    }
    //Seleccionamos la cantidad de elementos aleatorios a poblar.
    llint lim = (N*M)*M_DENSITY;
    for(llint i=0;i<lim;i++)
    {
        int *p = (&m[0][0]) + posiciones[i];
        *p = 1 + rand()%(RAND_N-1);
    }
}

/*
    Función para poblar un vector con valores aleatorios.
*/
void poblar_vector(int* v)
{
    for(int i=0;i<N;i++)
    {
        v[i] = rand()%RAND_N;
    }
}

/*
    Función para mostrar una matriz por consola.
*/
void mostrar_matriz(int (*m)[M])
{
    for(int i=0;i<N;i++)
    {
        printf("\t");
        for(int j=0;j<M;j++)
        {
            printf("%d ", m[i][j]);
        }
        printf("\n");
    }
    printf("\n");

}

/*
    Función para mostrar un vector por consola.
*/
void mostrar_vector(int* v, int dim)
{
    for(int i=0;i<dim;i++)
    {
        printf("%d ", v[i]);
    }
    printf("\n");   
}

