#ifndef PARALLEL_UTILS_H_
#define PARALLEL_UTILS_H_
#include "config.h"

void comprimir_matriz(int (*m)[M], int* v_com, int* vec_i, llint* vec_j);
void multiplicar_vmc(int* v_compressed, int* v_filas, llint* v_columnas, int* v, int* r,int cant_col);
void reajustar_indices(llint* v_columnas_local,int cant_col_local,int cant_nn_local);
void asignar_columnas(int id_proceso, int n_procesos, int *lim_a, int *lim_b);
void obtener_vector_cantidad_desplazamiento(int n_procesos,llint *v_columnas,int *vcant_nn, int *vdespl_nn, int *vcant_col,int *vdesp_col);

#endif