#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define N 10
#define M 5
#define RAND_N 10
#define M_DENSITY 1.0

void poblar_matriz(int (*m)[M], double densidad);
void poblar_vector(int* v);
void mostrar_matriz(int (*m)[M]);
void mostrar_vector(int* v,int dim);
void multiplicar_vm(int (*m)[M],int* v, int* r);

int main()
{
    srand(time(NULL));
    int v[N];
    int m[N][M];
    int r[M];

    poblar_matriz(m,M_DENSITY);
    poblar_vector(v);
    mostrar_matriz(m);
    mostrar_vector(v,N);
    
    multiplicar_vm(m,v,r);
    
    mostrar_vector(r,M);
    return 0;
}

/*
    Función para poblar la matriz.
    Inicializo todos con 0's y luego con el valor densidad le asigno aleatoriamente los valores.
    Densidad es un valor que varia desde 0.0 hasta 1.0. A mayor densidad, mayor será la cantidad de valores no nulos.
*/
void poblar_matriz(int (*m)[M],double densidad)
{
    //Inicializamos la matrices con 0.
    for(int i=0;i<N;i++)
        for(int j=0;j<M;j++)
            m[i][j] = 0;
    
    if (densidad==0.0) return;

    int posiciones[N*M];
    for(int i=0;i<N*M;i++) posiciones[i] = i;
   
    //Algoritmo Fisher-Yates para mezclar el arreglo en O(n)
    for(int i=N*M-1;i>0;i--)
    {
        int j = rand()%(i+1);
        int aux = posiciones[j];
        posiciones[j] = posiciones[i];
        posiciones[i] = aux;
    }
    //Seleccionamos la cantidad de elementos aleatorios a poblar.
    int lim = (N*M)*densidad;
    for(int i=0;i<lim;i++)
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
    printf("Matriz %dx%d:\n",N,M);
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
    printf("Vector de dimension %d: ",dim);
    
    for(int i=0;i<dim;i++)
    {
        printf("%d ", v[i]);
    }
    printf("\n");   
}

/*
    Función para realizar el producto vector-matriz.
*/
void multiplicar_vm(int (*m)[M],int* v, int* r)
{
    for(int i=0;i<M;i++)
    {
        int s=0;
        for(int j=0;j<N;j++)
        {
            s+=v[j]*m[j][i];
        }
        r[i] = s;
    }
}
