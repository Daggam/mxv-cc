#include "utils.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

// Función para realizar el producto vector-matriz.
void multiplicar_vm(int (*m)[M],int* v, int* r)
{
    for(int j=0;j<M;j++)
    {
        int s=0;
        for(int i=0;i<N;i++)
        {
            s+=v[i]*m[i][j];
        }
        r[j] = s;
    }
}

int main(int argc, char **argv)
{
    srand(58533);
    
    MPI_Init(&argc,&argv);
    
    int* v = (int*) malloc(N*sizeof(int));
    int (*m)[M] = malloc(N * sizeof(*m));
    int* r = (int*) malloc(M*sizeof(int));
    double inicio_t = MPI_Wtime(), fin_t;
    poblar_matriz(m);
    fin_t = MPI_Wtime();
    printf("\nTiempo para poblar la matriz (secuencial): %f\n",fin_t-inicio_t);
    
    inicio_t = MPI_Wtime();
    poblar_vector(v);
    fin_t = MPI_Wtime();
    printf("\nTiempo para poblar el vector (secuencial): %f\n",fin_t-inicio_t);
    
    inicio_t = MPI_Wtime();
    multiplicar_vm(m,v,r);
    fin_t = MPI_Wtime();
    #ifdef DEBUG
        printf("Vector v de dim %d: \n\t",N);
        mostrar_vector(v,N);
        printf("Matriz m %dx%d:\n",N,M);
        mostrar_matriz(m);
        printf("Vector resultado r de dim %d: \n\t",M);
        mostrar_vector(r,M);
    #endif
    MPI_Finalize();
    
    printf("\nTiempo requerido (secuencial): %f\n",fin_t-inicio_t);
    
    return 0;
}
