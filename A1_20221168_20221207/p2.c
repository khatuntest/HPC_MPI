// Menna Ali Abd elBaky 20221168

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_LEN 1024

void caesar_cipher(char *str, int len, int mode) {
    for (int i = 0; i < len; i++) {
        if (str[i] >= 32 && str[i] <= 126) {  // Printable ASCII range
            if (mode == 1) {  // Encryption
                if(str[i] >= 'a' && str[i] <= 'z'){
                    str[i] = ((str[i] - 'a' + 3) % 26) +'a';
                }
                else if (str[i] >= 'A' && str[i] <= 'Z')
                    str[i] = ((str[i] - 'A' + 3) % 26) +'A';
            }
            else if (mode == 2) {  // Decryption
                if(str[i] >= 'a' && str[i] <= 'z')
                    str[i] = ((str[i] -'a'- 3 + 26) % 26) + 'a';
                else if(str[i] >= 'A' && str[i] <= 'Z')
                    str[i] = ((str[i] -'A'- 3 + 26) % 26) + 'A';
            }
        }
    }
}


int main(int argc , char ** argv) {
    int rank, size , total_len , mode;
    char *input = malloc(MAX_LEN * sizeof(char));  // Allocate memory for input
    char *result = NULL;

    if (input == NULL) {  // Check if memory allocation failed
        printf("Memory allocation failed for input!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);  // Exit if memory allocation fails
    }

    MPI_Init(&argc , &argv);
    MPI_Comm_rank(MPI_COMM_WORLD , &rank);
    MPI_Comm_size(MPI_COMM_WORLD , &size);

    // Master process (rank 0)
    if (rank == 0) {
        printf("Choose input mode:\n");
        printf("1. Console input\n");
        printf("2. File input\n");
        int input_mode;
        scanf("%d", &input_mode);
        getchar();  // remove newline

        if (input_mode == 1) {
            printf("Enter the string: ");
            fgets(input, MAX_LEN, stdin);
            input[strcspn(input, "\n")] = '\0';  // remove newline
        }
        else {
            FILE* fp = fopen("input.txt", "r");
            if (!fp) {
                printf("Error opening file.\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            fgets(input, MAX_LEN, fp);
            input[strcspn(input, "\n")] = '\0';
            fclose(fp);
        }
        printf("1. Enter 1 for Encryption\n");
        printf("2. Enter 2 for Decryption\n");
        scanf("%d", &mode);
        total_len = strlen(input);

        // Send mode and total_len to all other processes
        for (int i = 1; i < size; i++) {
            MPI_Send(&mode, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&total_len, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        // Split the string and send each chunk to the other processes
        int chunk_size = total_len / size;
        int rem = total_len % size;
        int start = 0;

        for (int i = 1; i < size; i++) {
            int send_len = chunk_size + (rem > 0 ? 1 : 0);
            MPI_Send(&input[start], send_len, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            start += send_len;
            rem--;
        }

        // Process the first chunk for rank 0
        int my_chunk_size = chunk_size + (rem > 0 ? 1 : 0);
        char *my_chunk = malloc((my_chunk_size + 1) * sizeof(char));
        strncpy(my_chunk, input, my_chunk_size);
        my_chunk[my_chunk_size] = '\0';

        // Encrypt or decrypt this chunk
        caesar_cipher(my_chunk, my_chunk_size, mode);

        // Prepare to collect results
        result = malloc((total_len + 1) * sizeof(char));  // Allocate memory for result
        strncpy(result, my_chunk, my_chunk_size); // Copy processed chunk into result
        int offset = my_chunk_size;

        // Receive processed chunks from other processes
        for (int i = 1; i < size; i++) {
            int recv_len = chunk_size + (i < rem ? 1 : 0);
            MPI_Recv(&result[offset], recv_len, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            offset += recv_len;
        }
        result[total_len] = '\0';
        if (mode == 1)
            printf("Encrypted String: %s\n", result);
        else
            printf("Decrypted String: %s\n", result);

        free(result);
        free(my_chunk);

    }
    else {  // Worker processes
        // Receive mode and total_len from rank 0
        MPI_Recv(&mode, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&total_len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Determine chunk size and receive the corresponding chunk of the string
        int chunk_size = total_len / size;
        int rem = total_len % size;
        int recv_len = chunk_size + (rank <= rem ? 1 : 0);
        char *my_chunk = malloc((recv_len + 1) * sizeof(char));

        MPI_Recv(my_chunk, recv_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        my_chunk[recv_len] = '\0';

        // Process the chunk (encryption/decryption)
        caesar_cipher(my_chunk, recv_len, mode);

        // Send the processed chunk back to rank 0
        MPI_Send(my_chunk, recv_len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

        free(my_chunk);
    }

    // Finalize MPI
    MPI_Finalize();

    // Free memory for input
    free(input);

    return 0;
}
