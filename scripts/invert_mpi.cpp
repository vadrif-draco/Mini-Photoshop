#include <mpi.h>
#include <math.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string.h>

int main(int argc, const char** argv) {

    int world_size, world_rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    unsigned long long data_size;
    unsigned char* data, * inverted_data;
    std::chrono::_V2::steady_clock::time_point start, end;
    if (world_rank == 0) {

        // Get dimensions of data to be processed (input data is formatted as HEIGHT|0|WIDTH|0|[bytes for pixels])
        std::ifstream process_input("bin/temp", std::ios::binary);
        std::string data_height_str, data_width_str;
        std::getline(process_input, data_height_str, '\0');
        std::getline(process_input, data_width_str, '\0');
        unsigned long long data_height = std::stoull(data_height_str); // in pixels
        unsigned long long data_width = std::stoull(data_width_str); // in pixels
        data_size = data_height * data_width * 3; // 3 bytes per pixel, representing RGB channels
        std::cerr << data_size << " bytes (" << data_height << " x " << data_width << " x 3)" << std::endl;

        // Get data (pixels) to be processed
        data = (unsigned char*) malloc(data_size);
        process_input.read((char*) &data[0], data_size);
        process_input.close();

        // Start the timer (to include MPI communication overhead)
        start = std::chrono::steady_clock::now();

    }

    // Broadcast data size to all processes
    MPI_Bcast(&data_size, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

    // Calculate local data size in all processes
    // It's okay to ceil, garbage processed by last process will be ignored as we know the valid data range anyway
    unsigned long long local_data_size = ceil(float(data_size) / float(world_size));

    // Create a local data buffer in each process
    unsigned char* local_data = (unsigned char*) malloc(local_data_size);
    unsigned char* local_inverted_data = (unsigned char*) malloc(local_data_size);

    // Scatter, Senbonzakura
    MPI_Scatter(data, local_data_size, MPI_UNSIGNED_CHAR, local_data, local_data_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Invert the local data for each process
    for (unsigned long long i = 0; i < local_data_size; i++) local_inverted_data[i] = 0xff - local_data[i];

    // Prepare recvbuf for rank 0
    if (world_rank == 0) {

        // Prepare the inverted_data buffer
        // Note that its size is NOT data_size because the ceil that we performed earlier
        inverted_data = (unsigned char*) malloc(local_data_size * world_size);

    }

    // Gather
    MPI_Gather(local_inverted_data, local_data_size, MPI_UNSIGNED_CHAR, inverted_data, local_data_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {

        // End the timer and calculate the duration taken
        end = std::chrono::steady_clock::now();
        unsigned long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // Write it and terminate
        std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
        process_output.write((char*) &inverted_data[0], data_size);
        process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
        process_output.flush();
        process_output.close();

    }

    MPI_Finalize();
    return 0;

}