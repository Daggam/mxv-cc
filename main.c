#include <stdio.h>
#define N 10
#define M 5

int main()
{
    int v[N] = {0,1,2,3,4,5,6,7,8,9};
    int m[N][M] = 
        {
            {0,1,2,3,4},
            {0,1,2,3,4},
            {0,1,2,3,4},
            {0,1,2,3,4},
            {0,1,2,3,4},
            {0,1,2,3,4},
            {0,1,2,3,4},
            {0,1,2,3,4},
            {0,1,2,3,4},
            {0,1,2,3,4},
        };
    int r[M];

    for(int i=0;i<M;i++)
    {
        int s=0;
        for(int j=0;j<N;j++)
        {
            s+=v[j]*m[j][i];
        }
        r[i] = s;
        printf("%d ",r[i]);
    }
    printf("\n");
    
    return 0;
}