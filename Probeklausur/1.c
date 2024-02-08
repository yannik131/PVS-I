#include <assert.h>
#include <mpi.h>

// Da jeder Prozess zuerst wartet, führt jede denkbare Ausführung zu einem Circular-Wait-Deadlock

int send_to_right1(int *send, int send_cnt, int *recv, int N)
{
    int rank, p, succ, pred, recv_cnt;
    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    pred = (rank - 1 + p) % p;
    succ = (rank + 1) % p;
    if (rank % 2 == 0)
    {
        MPI_Send(send, send_cnt, MPI_INT, succ, 0, MPI_COMM_WORLD);
        MPI_Recv(recv, N, MPI_INT, pred, 0, MPI_COMM_WORLD, &status);
    }
    else
    {
        MPI_Recv(recv, N, MPI_INT, pred, 0, MPI_COMM_WORLD, &status);
        MPI_Send(send, send_cnt, MPI_INT, succ, 0, MPI_COMM_WORLD);
    }

    MPI_Get_count(&status, MPI_INT, &recv_cnt);
    return recv_cnt;
}

int send_to_right2(int *send, int send_cnt, int *recv, int N)
{
    int rank, p, succ, pred, recv_cnt;
    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    pred = (rank - 1 + p) % p;
    succ = (rank + 1) % p;
    MPI_Sendrecv(send, send_cnt, MPI_INT, succ, 0, recv, recv_cnt, MPI_INT, pred, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &recv_cnt);
    return recv_cnt;
}

int main()
{
    MPI_Init(NULL, NULL);

    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    assert(worldSize == 3);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int data[1] = {rank};
    int receive[1];

    send_to_right2(data, 1, receive, 1);
}