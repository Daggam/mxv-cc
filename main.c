#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define N 10
#define M 5
#define RAND_N 10

void poblar_matriz(int (*m)[M]);
void poblar_vector(int* v);
void mostrar_matriz(int (*m)[M]);
void mostrar_vector(int* v,int dim);

int main()
{
    srand(time(NULL));
    int v[N];
    int m[N][M];
    int r[M];

    poblar_matriz(m);
    poblar_vector(v);
    mostrar_matriz(m);
    mostrar_vector(v,N);
    
    for(int i=0;i<M;i++)
    {
        int s=0;
        for(int j=0;j<N;j++)
        {
            s+=v[j]*m[j][i];
        }
        r[i] = s;
    }
    
    mostrar_vector(r,M);
    return 0;
}

void poblar_matriz(int (*m)[M])
{
    for(int i=0;i<N;i++)
    {
        for(int j=0;j<M;j++)
        {
            m[i][j] = rand()%RAND_N;
        }
    }
}

void poblar_vector(int* v)
{
    for(int i=0;i<N;i++)
    {
        v[i] = rand()%RAND_N;
    }
}

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
void mostrar_vector(int* v, int dim)
{
    printf("Vector de dimension %d: ",dim);
    
    for(int i=0;i<dim;i++)
    {
        printf("%d ", v[i]);
    }
    printf("\n");   
}