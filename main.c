#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#if !defined(N) || !defined(M) || !defined(RAND_N) || !defined(M_DENSITY)
    #error "Debes definir N, M, RAND_N y M_DENSITY"
#endif

void poblar_matriz(int (*m)[M], double densidad);
void poblar_vector(int* v);
void mostrar_matriz(int (*m)[M]);
void mostrar_vector(int* v,int dim);
void multiplicar_vm(int (*m)[M],int* v, int* r);
void multiplicar_vmc(int* v_com, int* vec_i, int* vec_j, int* v, int* r,int  cant_col);
void asignar_columnas( int id_proceso, int n_procesos, int *lim_a, int *lim_b);
void obtener_desplazamiento(int n_procesos,int *vec_j,int *cant_nn, int *despl_nn, int *cant_col,int *desp_col);

int comprimir_matriz(int (*m)[M], int* v_com, int* vec_i, int* vec_j);

int main(int argc, char **argv)
{
    //  MPI  
    int id_proceso, n_procesos;
	MPI_Init(&argc, &argv);              
    MPI_Comm_rank(MPI_COMM_WORLD, &id_proceso);  
    MPI_Comm_size(MPI_COMM_WORLD, &n_procesos);     

    //Vectores y matrices iniciales
    int *v = malloc(N * sizeof(int));
    int (*m)[M]=NULL;
    int tam_vec=N*M*M_DENSITY;
    int r[M];
    int *r_final = NULL;

    //Matiz comprimida
    int *v_com=NULL;
    int *vec_i=NULL;
    int *vec_j=NULL;
    int cant_nn_total = 0;

    //Vectores para la paralelizacion
    int *cant_nn = NULL;
    int *despl_nn = NULL;
    int *cant_col = NULL;
    int *desp_col = NULL;
    
    double tiempo_inicio;
    if(id_proceso==0)
    {

        int cap = N*M*M_DENSITY;

        m     = malloc(N * sizeof(*m));
        v_com = malloc((cap+1) * sizeof(int));
        vec_i = malloc((cap+1) * sizeof(int));
        vec_j = malloc((M + 1) * sizeof(int));

        srand(58533);
        
        //Poblar
        poblar_matriz(m,M_DENSITY);
        poblar_vector(v);
        /*
        mostrar_matriz(m);
        mostrar_vector(v,N);  
*/

        tiempo_inicio = MPI_Wtime();
        //Comprimir la matriz
        cant_nn_total = comprimir_matriz(m,v_com,vec_i,vec_j);
      /*
        printf("\nVector comprimido: ");
        mostrar_vector(v_com,cant_nn_total);
        printf("\nVector de indices de comlumnas: ");
        mostrar_vector(vec_i,cant_nn_total);
        printf("\nVector de indices de columnas: ");
        mostrar_vector(vec_j,M);
        */
        
        r_final = (int*)malloc(M * sizeof(int));
        cant_nn = (int*)malloc(n_procesos * sizeof(int));
        despl_nn = (int*)malloc(n_procesos * sizeof(int));
        cant_col = (int*)malloc(n_procesos * sizeof(int));
        desp_col = (int*)malloc(n_procesos * sizeof(int));
        obtener_desplazamiento(n_procesos, vec_j, cant_nn, despl_nn, cant_col, desp_col);
    }

    int cant_col_local = 0;
    int cant_nn_local = 0;
    MPI_Scatter(cant_nn, 1, MPI_INT, &cant_nn_local, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(cant_col, 1, MPI_INT, &cant_col_local, 1, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Bcast(v,N,MPI_INT,0,MPI_COMM_WORLD);

    int *v_com_local = (int*)malloc(cant_nn_local * sizeof(int));
    int *vec_i_local = (int*)malloc(cant_nn_local * sizeof(int));   
    int *vec_j_local = (int*)malloc((cant_col_local + 1) * sizeof(int));

    MPI_Scatterv(v_com, cant_nn, despl_nn, MPI_INT, v_com_local, cant_nn_local, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(vec_i, cant_nn, despl_nn, MPI_INT, vec_i_local, cant_nn_local, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(vec_j, cant_col, desp_col, MPI_INT, vec_j_local, cant_col_local , MPI_INT, 0, MPI_COMM_WORLD);

    //Reajueste de los indices del vector j 
    if(cant_col_local>0)
    {
        int base = vec_j_local[0];
        for(int i=0;i<=cant_col_local;i++)
        {
            vec_j_local[i] -= base;
        }
    }
    vec_j_local[cant_col_local]=cant_nn_local;

    multiplicar_vmc(v_com_local,vec_i_local,vec_j_local,v,r,cant_col_local);

    MPI_Gatherv(r,cant_col_local,MPI_INT,r_final,cant_col,desp_col,MPI_INT,0,MPI_COMM_WORLD);
    free(v_com_local);
    free(vec_i_local);
    free(vec_j_local);
    /*
    MPI_Gather junta todos los vectores y esta funci[on epsra bloques simetricos por lo que si hay procesos con pasos extra, se les pide un numero extra los ultimos procesos
    Por eso se usa MPI_Gatherv que permite especificar la cantidad de elementos a recibir de cada proceso y el desplazamiento en el vector final.
    Estos son los 9 parámetros que requiere, aplicados directamente al contexto de tu programa:
        sendbuf (Buffer de envío): El puntero al arreglo local que cada proceso calculó. En tu código es r.
        sendcount (Cantidad a enviar): La cantidad de elementos que el proceso actual va a mandar. En tu código es r_local_size.
        sendtype (Tipo de dato a enviar): El tipo de dato de los elementos enviados. En tu caso, MPI_INT.
        recvbuf (Buffer de recepción): El arreglo global donde se ensamblarán todos los datos. Solo es relevante en el proceso raíz. En tu código es r_final.
        recvcounts (Arreglo de conteos de recepción): Un arreglo (exclusivo del raíz) donde cada índice i indica exactamente la cantidad de elementos que el proceso i le va a enviar. Solo el maestro lee esto.
        displs (Arreglo de desplazamientos): Un arreglo (exclusivo del raíz) donde cada índice i indica en qué posición exacta de recvbuf debe empezar a escribir los datos que lleguen del proceso i.
        recvtype (Tipo de dato a recibir): El tipo de dato que el proceso raíz espera. Debe coincidir con el de envío, en este caso MPI_INT.
        root (Proceso raíz): El identificador (rank) del proceso encargado de juntar la información. En tu código es 0.
        comm (Comunicador): El grupo de procesos que participan en la operación. Generalmente MPI_COMM_WORLD.
    */
    if(id_proceso==0)
    {
        
        //Resultado
        double tiempo_fin = MPI_Wtime();

        double tmseq_inicio = MPI_Wtime(), tmseq_fin;
        //Para comparar resultados
        int r[M];
        multiplicar_vm(m,v,r);
        tmseq_fin = MPI_Wtime();
        
        printf("\nVector resultado final (paralelizado): \n");
        mostrar_vector(r_final,M);
        printf("\nVector resultado final (secuencial): \n");
        mostrar_vector(r,M);
        
        printf("Tiempo paralelizado: %f\n",tiempo_fin-tiempo_inicio);
        printf("Tiempo secuencial: %f\n",tmseq_fin-tmseq_inicio);

        free(r_final);
        free(cant_nn);
        free(despl_nn);
        free(cant_col);
        free(desp_col);
    }
    free(v);

    MPI_Finalize();
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
/*
    Función para comprimir la matriz en formato Comrpimido por comlumnas CSC.
*/
int comprimir_matriz(int (*m)[M], int* v_com, int* vec_i, int* vec_j)
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
    return cont;
}
/*
    Función para realizar el producto vector-matriz en formato CSC.
*/
void multiplicar_vmc(int* v_com, int* vec_i, int* vec_j, int* v, int* r,int  cant_col)// mando el cant solo para la ultima fila
{

    for(int j=0;j<cant_col;j++)
    {
        int s=0;
        for(int i=vec_j[j];i<vec_j[j+1];i++)
        {
            s+=v[vec_i[i]]*v_com[i];
        }
        
        r[j] = s;
    }
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

void obtener_desplazamiento(int n_procesos,int *vec_j,int *cant_nn, int *despl_nn, int *cant_col,int *desp_col)
{
    int a, b;
    for (int i = 0; i < n_procesos; i++)
    {
        asignar_columnas(i, n_procesos, &a, &b);
        cant_nn[i] = vec_j[b] - vec_j[a]; // cuántos elementos no nulos aporta el proceso i
        despl_nn[i] = vec_j[a];           // desde qué posición de v_com empieza
        cant_col[i] = b - a;   // cuántas columnas (resultados) aporta el proceso i
        desp_col[i]     = a;       // desde qué posición de r_final empieza
    }
}