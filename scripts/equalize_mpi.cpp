#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <chrono>
#include <iostream>

const int BYTE = 256;


int main(int argc, char **argv) {


    MPI_Init(NULL, NULL);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);


    int data_height;
    int data_width;
    int pixels;
    int data_size; // 3 bytes per pixel, representing RGB channels

    // data (pixels) to be processed
    unsigned char* data;

    // cumulative probability arrays
    double local_prob[3][BYTE], global_prob[3][BYTE];


    // initialize probability arrays
    for(int ch = 0;ch<3;ch++)
    {
        std::fill(local_prob[ch], local_prob[ch]+BYTE,0.0);
        std::fill(global_prob[ch], global_prob[ch]+BYTE,0.0);
    }
    auto start = std::chrono::steady_clock::now();

    if (world_rank == 0) {
         //Get dimensions of data to be processed (input data is formatted as HEIGHT|0|WIDTH|0|[bytes for pixels])
        std::ifstream process_input("bin/temp", std::ios::binary);
        std::string data_height_str, data_width_str;
        std::getline(process_input, data_height_str, '\0');
        std::getline(process_input, data_width_str, '\0');
        data_height = std::stoull(data_height_str); // in pixels
        data_width = std::stoull(data_width_str); // in pixels
        pixels = data_height * data_width;
        data_size = pixels * 3; // 3 bytes per pixel, representing RGB channels
        std::cerr << data_size << " bytes (" << data_height << " x " << data_width << " x 3)" << std::endl;

        data = (unsigned char*) malloc(data_size);
        process_input.read((char*) &data[0], data_size);
        process_input.close();


        start = std::chrono::steady_clock::now();
    }

    MPI_Bcast(&pixels, 1, MPI_INT, 0, MPI_COMM_WORLD);
    data_size = pixels*3;


    // dividing data among processes
    int sendcounts [world_size];
    int displs [world_size];
    int res = pixels % world_size;
    int size_per_process = pixels / world_size;
    int increment = 0;
    for(int processID = 0; processID < world_size; processID++){
        displs[processID] = increment;
        sendcounts[processID] = (processID + 1 <= res) ? size_per_process + 1 : size_per_process;
        sendcounts[processID] *= 3;
        increment += sendcounts[processID];
    }
    int process_size = sendcounts[world_rank];


    auto* local_data = (unsigned char*) malloc(process_size);
    MPI_Scatterv(data, sendcounts, displs, MPI_UNSIGNED_CHAR, local_data, process_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Pixel intensity frequency calculation
    for(int i=0;i<process_size;i++)
    {
        local_prob[i%3][local_data[i]]++;
    }

    // combine all probability data from all processes into one global array
    for(int ch = 0;ch<3;ch++)
        MPI_Reduce(local_prob[ch],global_prob[ch],BYTE,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);

    if (world_rank == 0) {

        for(int ch = 0;ch<3;ch++)
        {
            // Frequency Normalization
            for(int op = 0;op<BYTE;op++)
            {
                global_prob[ch][op] /= pixels;
            }

            // Probability cumulative sum
            for(int op = 1;op<BYTE;op++)
            {
                global_prob[ch][op] += global_prob[ch][op-1];
            }
        }
    }

    for(int ch = 0;ch<3;ch++)
        MPI_Bcast(global_prob[ch], BYTE, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    auto* local_processed_data = (unsigned char*) malloc(process_size);
    auto* global_processed_data = (unsigned char*) malloc(data_size);

    // Fill the image with new pixel values
    for(int i=0;i<process_size;i++)
    {
        double newVal = global_prob[i%3][local_data[i]] * 255;
        local_processed_data[i] = (unsigned char) floor(newVal);
    }

    MPI_Gatherv(local_processed_data, process_size, MPI_UNSIGNED_CHAR, global_processed_data, sendcounts, displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {

        auto end = std::chrono::steady_clock::now();
        unsigned long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // Write it and terminate
        std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
        process_output.write((char*) &global_processed_data[0], data_size);
        process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
        process_output.flush();
        process_output.close();
    }

    MPI_Finalize();
    return 0;
}