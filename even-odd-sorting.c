#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void init_data(int *array, int length)
{
    srand(time(NULL));
    for (int i = 0; i < length; i++)
    {
        array[i] = rand() % 1000;
    }
}

void odd_even_sort(int *a, int n)
{
    int phase, i, temp;
    for (phase = 0; phase < n; phase++)
    {
        if (phase % 2 == 0)
        {
            for (i = 1; i < n; i += 2)
            {
                if (a[i-1] > a[i])
                {
                    temp = a[i];
                    a[i] = a[i-1];
                    a[i-1] = temp;
                }
            }
        }
        else
        {
            for (i = 1; i < n-1; i += 2)
            {
                if (a[i] > a[i+1])
                {
                    temp = a[i];
                    a[i] = a[i+1];
                    a[i+1] = temp;
                }
            }
        }
    }
}

void merge_low(int *my_keys, int *recv_keys, int *temp_keys, int local_n)
{
    int m_i, r_i, t_i;
    m_i = r_i = t_i = 0;
    while (t_i < local_n) {
        if (my_keys[m_i] <= recv_keys[r_i]) {
            temp_keys[t_i] = my_keys[m_i];
            t_i++; m_i++;
        }   else {
            temp_keys[t_i] = recv_keys[r_i];
            t_i++; r_i++;
        }
    }
    for (m_i = 0; m_i < local_n; m_i++)
        my_keys[m_i] = temp_keys[m_i];
}

void merge_high(int *my_keys, int *recv_keys, int *temp_keys, int local_n)
{
    int m_i, r_i, t_i;
    m_i = r_i = t_i = local_n - 1;
    while (t_i >= 0)
    {
        if (my_keys[m_i] >= recv_keys[r_i])
        {
            temp_keys[t_i] = my_keys[m_i];
            t_i--; m_i--;
        }
        else
        {
            temp_keys[t_i] = recv_keys[r_i];
            t_i--; r_i--;
        }
    }
    for (m_i = 0; m_i < local_n; m_i++)
        my_keys[m_i] = temp_keys[m_i];
}


int main(int argc, char *argv[])
{

    double start, finish, loc_elapsed, elapsed;
    int comm_sz; // number of processes
    int my_rank; // my process rank

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);


    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    const int arrayLength = atoi(argv[1]);

    int *arrayPtr = (int *)calloc(arrayLength, sizeof(int));
    int *localArrayPtr = (int *)calloc(arrayLength/comm_sz, sizeof(int));
    int *recvArrayPtr = (int *)calloc(arrayLength/comm_sz, sizeof(int));
    int *tempArrayPtr = (int *)calloc(arrayLength/comm_sz, sizeof(int));

    init_data(arrayPtr, arrayLength);

    MPI_Scatter(arrayPtr, arrayLength/comm_sz, MPI_INT, localArrayPtr, arrayLength/comm_sz, MPI_INT, 0, MPI_COMM_WORLD);

    odd_even_sort(localArrayPtr, arrayLength / comm_sz);

    int oddrank;
    int evenrank;

    if(my_rank%2 == 0)
    {
        oddrank = my_rank - 1;
        evenrank = my_rank + 1;
    }
    else{
        oddrank = my_rank + 1;
        evenrank = my_rank - 1;
    }

    if(oddrank == -1 || oddrank == comm_sz)
    {
        oddrank = MPI_PROC_NULL;
    }
    if(evenrank == -1 || evenrank == comm_sz)
    {
        evenrank = MPI_PROC_NULL;
    }


    for(int phase = 0; phase < comm_sz; phase++)
    {
        if (phase % 2 == 0) 
        {
            if (evenrank >= 0) 
            {
                MPI_Sendrecv(localArrayPtr, arrayLength/comm_sz, MPI_INT, evenrank, 0,
                    recvArrayPtr, arrayLength/comm_sz, MPI_INT, evenrank, 0, MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE);
            if (my_rank % 2 != 0) /* odd rank */
                merge_high(localArrayPtr, recvArrayPtr, tempArrayPtr, arrayLength/comm_sz);
            else
                merge_low(localArrayPtr, recvArrayPtr, tempArrayPtr, arrayLength/comm_sz);
            }
          
        }else 
        {
            if (oddrank >= 0) {
                MPI_Sendrecv(localArrayPtr, arrayLength/comm_sz, MPI_INT, oddrank, 0,
                    recvArrayPtr, arrayLength/comm_sz, MPI_INT, oddrank, 0, MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE);
            if (my_rank % 2 != 0)
                merge_low(localArrayPtr, recvArrayPtr, tempArrayPtr, arrayLength/comm_sz);
            else
                merge_high(localArrayPtr, recvArrayPtr, tempArrayPtr, arrayLength/comm_sz);
            }
        }
    }

    MPI_Gather(localArrayPtr, arrayLength / comm_sz, MPI_INT, arrayPtr, arrayLength / comm_sz, MPI_INT, 0, MPI_COMM_WORLD);


    
    finish = MPI_Wtime();
    loc_elapsed = finish-start;
    MPI_Reduce(&loc_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);


    if (my_rank == 0)
    {
        printf("Elapsed time = %e seconds", elapsed);
    }

    MPI_Finalize();
    return 0;
}