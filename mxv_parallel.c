#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "utils.h"
#include "parallel_utils.h"

int main(int argc, char **argv)
{
    srand(58533);
    int id_proceso, n_procesos;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_proceso);  
    MPI_Comm_size(MPI_COMM_WORLD, &n_procesos);    
    double inicio_t, fin_t;
    int* v = (int*) malloc(N*sizeof(int));
    int* r=NULL;
    // Representación CSC global de la matriz.
    int *v_compresed=NULL, *v_filas=NULL;
    llint *v_columnas=NULL; // Pues columnas tiene punteros a v_filas que tiene com máximo N*M elementos.

    // Vectores de cantidades no nulas y de desplazamiento.
    int *vcant_nn=NULL, *vdespl_nn=NULL,*vcant_col=NULL,*vdespl_col=NULL;
        
    //El proceso 0 se encarga de cargar la matriz y el vector en memoria.
    if (id_proceso==0)
    {
        llint cant_no_nulos = N*M*M_DENSITY;
        int (*m)[M] = malloc(N * sizeof(*m));
        r = (int*) malloc(M*sizeof(int));
        v_compresed = (int*) malloc((cant_no_nulos+1)*sizeof(int));
        v_filas = (int*) malloc((cant_no_nulos+1)*sizeof(int));
        v_columnas = (llint*)malloc((M+1)*sizeof(llint));
        
        inicio_t = MPI_Wtime();
        poblar_matriz(m);
        fin_t = MPI_Wtime();
        printf("\nTiempo para poblar la matriz (secuencial): %f\n",fin_t-inicio_t);

        inicio_t = MPI_Wtime();
        poblar_vector(v);
        fin_t = MPI_Wtime();
        printf("\nTiempo para poblar el vector (secuencial): %f\n",fin_t-inicio_t);

        inicio_t = MPI_Wtime();
        comprimir_matriz(m,v_compresed,v_filas,v_columnas);
        free(m);

        
        vcant_nn = (int*)malloc(n_procesos * sizeof(int));
        vdespl_nn = (int*)malloc(n_procesos * sizeof(int));
        vcant_col = (int*)malloc(n_procesos * sizeof(int));
        vdespl_col = (int*)malloc(n_procesos * sizeof(int));
        obtener_vector_cantidad_desplazamiento(n_procesos, v_columnas, vcant_nn, vdespl_nn, vcant_col, vdespl_col);

    }

    
    int cant_col_local = 0;
    int cant_nn_local = 0;
    MPI_Scatter(vcant_nn, 1, MPI_INT, &cant_nn_local, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(vcant_col, 1, MPI_INT, &cant_col_local, 1, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Bcast(v,N,MPI_INT,0,MPI_COMM_WORLD);

    // Vectores CSC locales de la matriz. Son para la paralelización.
    int *v_compressed_local = (int*)malloc(cant_nn_local * sizeof(int));
    int *v_filas_local = (int*)malloc(cant_nn_local * sizeof(int));   
    llint *v_columnas_local = (llint*)malloc((cant_col_local + 1) * sizeof(llint));
    int *r_local = (int*)malloc(cant_col_local * sizeof(int));

    MPI_Scatterv(v_compresed, vcant_nn, vdespl_nn, MPI_INT, v_compressed_local, cant_nn_local, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(v_filas, vcant_nn, vdespl_nn, MPI_INT, v_filas_local, cant_nn_local, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(v_columnas, vcant_col, vdespl_col, MPI_LONG_LONG_INT, v_columnas_local, cant_col_local , MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);

    reajustar_indices(v_columnas_local,cant_col_local,cant_nn_local);

    multiplicar_vmc(v_compressed_local,v_filas_local,v_columnas_local,v,r_local,cant_col_local);

    free(v_compressed_local);
    free(v_filas_local);
    free(v_columnas_local);
    
    MPI_Gatherv(r_local,cant_col_local,MPI_INT,r,vcant_col,vdespl_col,MPI_INT,0,MPI_COMM_WORLD);

    free(r_local);
    
    free(v);
    if(id_proceso==0)
    {
        fin_t = MPI_Wtime();

        #ifdef DEBUG
            printf("\nVector resultado final (paralelizado): \n\t");
            mostrar_vector(r,M);
        #endif

        printf("\nTiempo requerido (paralelizado): %f\n",fin_t-inicio_t);
        
        free(vcant_nn);
        free(vdespl_nn);
        free(vcant_col);
        free(vdespl_col);
        
        free(r);
        free(v_compresed);
        free(v_columnas);
        free(v_filas);
    }
    MPI_Finalize();
    return 0;
}