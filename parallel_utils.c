#include "config.h"
/*
    Función para comprimir la matriz en formato CSC (Compressed Sparse Column)
*/
void comprimir_matriz(int (*m)[M], int* v_com, int* vec_i, llint* vec_j)
{
    int cont=0;
    for(int j=0;j<M;j++)
    {
        vec_j[j] = cont;
        for(int i=0;i<N;i++)
        {
            if(m[i][j] != 0)
            {
                v_com[cont] = m[i][j];
                vec_i[cont] = i;
                cont++;
            }
        }
    }
    vec_j[M] = cont; 

}

/*
    Función para realizar el producto vector-matriz en formato CSC.
*/
void multiplicar_vmc(int* v_compressed, int* v_filas, llint* v_columnas, int* v, int* r,int cant_col)// mando el cant solo para la ultima fila
{

    for(int j=0;j<cant_col;j++)
    {
        int s=0;
        for(llint i=v_columnas[j];i<v_columnas[j+1];i++)
        {
            s+=v[v_filas[i]]*v_compressed[i];
        }
        
        r[j] = s;
    }
}

/*
    Función para reajustar los indices de las filas
*/
void reajustar_indices(llint* v_columnas_local,int cant_col_local,int cant_nn_local)
{
    //Reajueste de los indices del vector j 
    if(cant_col_local>0)
    {
        llint base = v_columnas_local[0];
        for(int i=0;i<=cant_col_local;i++)
        {
            v_columnas_local[i] -= base;
        }
    }
    v_columnas_local[cant_col_local]=cant_nn_local;
}

void  asignar_columnas( int id_proceso, int n_procesos, int *lim_a, int *lim_b)
{
    int cant_pasos,pasos_extra;

	cant_pasos = M / n_procesos;
	pasos_extra = M % n_procesos;
	if (pasos_extra!=0) // Distribuir los pasos extra entre los primeros procesos
	{
		if(id_proceso<pasos_extra)
		{
			*lim_a = id_proceso * cant_pasos+id_proceso;
			*lim_b = (id_proceso + 1) * cant_pasos+id_proceso+1;
		}else 
		{
			*lim_a = id_proceso * cant_pasos+pasos_extra;
			*lim_b = (id_proceso + 1) * cant_pasos+pasos_extra;
		}
	
	}else
	{
		*lim_a = id_proceso * cant_pasos;
		*lim_b = (id_proceso + 1) * cant_pasos;	
	}
}

void obtener_vector_cantidad_desplazamiento(int n_procesos,llint *v_columnas,int *vcant_nn, int *vdespl_nn, int *vcant_col,int *vdesp_col)
{
    int a, b;
    for (int i = 0; i < n_procesos; i++)
    {
        asignar_columnas(i, n_procesos, &a, &b);
        vcant_nn[i] = v_columnas[b] - v_columnas[a]; // cuántos elementos no nulos aporta el proceso i
        vdespl_nn[i] = v_columnas[a];           // desde qué posición de v_com empieza
        vcant_col[i] = b - a;   // cuántas columnas (resultados) aporta el proceso i
        vdesp_col[i]     = a;       // desde qué posición de r_final empieza
    }
}
