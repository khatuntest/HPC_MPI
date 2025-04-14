// 20221207 Mariam Mahmoud Ibrahim
#include <mpi.h>
#include <stdio.h>
#include <malloc.h>
#include <limits.h>
// mpiexec -n 6 "D:\CS material\2nd\HPC\assignment1\cmake-build-debug\assignment1.exe"
int main(int argc, char** argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // printf("Hello from process %d of %d\n", rank, size);
    if(size<2){
        if(rank==0)
            printf("run the code with at least 2 processes!\n");
        MPI_Finalize();
        return 0;
    }
    // master process
    if(rank==0){
        printf("Hello from master process.\n");
        fflush(stdout); // i used it to clean up

        printf("Number of slave processes is %d\n",size-1);
        fflush(stdout);

        printf("Please enter size of array...\n");
        fflush(stdout);

        int n;
        scanf("%d",&n); // get size of array

        int* array = (int*)malloc(n * sizeof(int)); // dynamic allocation
        printf("Please enter array elements ...\n");
        fflush(stdout);

        for(int i=0;i<n;++i)
            scanf("%d",&array[i]); // get array items

        int part=n/(size-1),reminder=n%(size-1),begin=0; // divide array into equal parts and get reminder to distribute excess items
        for(int i=1;i<size;++i) {
            int chunk;
            if(i>n) chunk =0 ;
            // each process will take chunk and if rank less than reminder we will give it one more
            else chunk = part + (reminder >= i ? 1 : 0);
            // send size
            MPI_Send(&chunk, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            // send array elements starting from index begin
            if (chunk>0)
                MPI_Send(&array[begin], chunk, MPI_INT, i, 0, MPI_COMM_WORLD);
            // increment begin by chunk every time
            begin+=chunk;
        }

        // initialize global maximum & global index & current begin
        int global_maxi=INT_MIN,global_index=-1,current_begin=0;
        for(int i=1;i<size;++i){
            int l_maxi,l_index;
            // for each slave receive maximum and index
            MPI_Recv(&l_maxi,1,MPI_INT,i,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&l_index,1,MPI_INT,i,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            // calculate actual index in array that master divide
            int real = current_begin+l_index;
            // check if local maximum from slave # i is greater than global maximum
            if(l_maxi>global_maxi){
                // update globals (index & maxi)
                global_maxi=l_maxi;
                global_index=real;
            }
            // update begin in array
            if(i<=n) current_begin += part+(reminder>=i ? 1:0);
        }

        // print maximum in whole array
        printf("Master process announce the final max which is %d and its index is %d.\n\n",global_maxi,global_index);
        fflush(stdout);

        printf("Thanks for using our program.");
        fflush(stdout);

        free(array); // deallocate
    }
    else{
        int count;
        // receive partition size
        MPI_Recv(&count,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        if (count == 0) {
            // This process is not needed
            int dummy_max = INT_MIN, dummy_index = 0;
            MPI_Send(&dummy_max, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&dummy_index, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Finalize();
            return 0;
        }
        int* local = (int*)malloc(count * sizeof(int)); // dynamic allocation
        // receive part of array
        MPI_Recv(local,count,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        int maxi=local[0],index=0;
        for(int i=1;i<count;++i)
            // get maximum
            if(local[i]>maxi) {
                // update index and maximum value
                maxi = local[i];
                index = i;
            }
        // print local maximum and index of it
        printf("Hello from slave# %d Max number in my partition is %d and index is %d.\n",rank,maxi,index);
        fflush(stdout);
        // send local max to master
        MPI_Send(&maxi,1,MPI_INT,0,0,MPI_COMM_WORLD);
        // send local index to master
        MPI_Send(&index,1,MPI_INT,0,0,MPI_COMM_WORLD);
        free(local); // deallocate
    }
    MPI_Finalize();
    return 0;
}
