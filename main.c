#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#define N 10
#define M 5
#define RAND_N 10
#define M_DENSITY 0.2

void poblar_matriz(int (*m)[M], double densidad);
void poblar_vector(int* v);
void mostrar_matriz(int (*m)[M]);
void mostrar_vector(int* v,int dim);
void multiplicar_vm(int (*m)[M],int* v, int* r);
void multiplicar_vmc(int* v_com, int* vec_i, int* vec_j, int* v, int* r,int  lim_a, int lim_b);
void asignar_columnas( int id_proceso, int n_procesos, int *lim_a, int *lim_b);
void obtener_desplazamiento(int n_procesos, int *cant_por_columna,int *desp);

int comprimir_matriz(int (*m)[M], int* v_com, int* vec_i, int* vec_j);

int main(int argc, char **argv)
{
    //Vectores y matrices iniciales
    srand(time(NULL));
    int v[N];
    int m[N][M];
    int tam_vec=N*M*M_DENSITY;//pongo +1 por si queda con decimales, para que no se pierda el ultimo valor
    int r[M];
    //Matiz comprimida
    int v_com[tam_vec];// Ver como comprimirlo
    int vec_i[tam_vec];// Ver como comprimirlo
    int vec_j[M+1];
    int cant_no_nulos = 0;


    poblar_matriz(m,M_DENSITY);
    poblar_vector(v);
    mostrar_matriz(m);
    mostrar_vector(v,N);
    
    // multiplicar_vm(m,v,r);
    
    // mostrar_vector(r,M);

    /*Comprimir la matriz*/
    cant_no_nulos = comprimir_matriz(m,v_com,vec_i,vec_j);
    printf("\nVector comprimido: ");
    mostrar_vector(v_com,cant_no_nulos);
    printf("\nVector de indices de comlumnas: ");
    mostrar_vector(vec_i,cant_no_nulos);
    printf("\nVector de indices de columnas: ");
    mostrar_vector(vec_j,M);

     /*  MPI  */
    int id_proceso, n_procesos,tienpo_inicial,lim_a,lim_b;
	MPI_Init(&argc, &argv);              
    MPI_Comm_rank(MPI_COMM_WORLD, &id_proceso);  
    MPI_Comm_size(MPI_COMM_WORLD, &n_procesos);     

	// tiempo_inicial = MPI_Wtime();

    asignar_columnas(id_proceso, n_procesos, &lim_a, &lim_b);
    multiplicar_vmc(v_com,vec_i,vec_j,v,r,lim_a,lim_b);
    
    int *r_final = NULL;
    int *cant_por_columna = NULL;
    int *desp = NULL;
    if(id_proceso==0)
    {
        r_final = (int*)malloc(M * sizeof(int));
        cant_por_columna = (int*)malloc(n_procesos * sizeof(int));
        desp = (int*)malloc(n_procesos * sizeof(int));
        obtener_desplazamiento(n_procesos, cant_por_columna, desp);
    }
    MPI_Gatherv(r,lim_b-lim_a,MPI_INT,r_final,cant_por_columna,desp,MPI_INT,0,MPI_COMM_WORLD);
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
        //Para comparar resultados
        int r[M];
        multiplicar_vm(m,v,r);
        mostrar_vector(r,M);

        //Resultaod
        printf("\nVector resultado final: ");
        mostrar_vector(r_final,M);
        
        free(r_final);
        free(cant_por_columna);
        free(desp);
    }
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
void multiplicar_vmc(int* v_com, int* vec_i, int* vec_j, int* v, int* r,int  lim_a, int lim_b)// mando el cant solo para la ultima fila
{
    int count=0;
    for(int j=lim_a;j<lim_b;j++)
    {
        int s=0;
        for(int i=vec_j[j];i<vec_j[j+1];i++)
        {
            s+=v[vec_i[i]]*v_com[i];
        }
        
        r[count] = s;
        count++;
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

void obtener_desplazamiento(int n_procesos, int *cant_por_columna, int *desp)
{
    int a, b;
    for (int i = 0; i < n_procesos; i++)
    {
        asignar_columnas(i, n_procesos, &a, &b);
        cant_por_columna[i] = b - a;   // cuántas columnas (resultados) aporta el proceso i
        desp[i]     = a;       // desde qué posición de r_final empieza
    }
}