#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <numeric>

using namespace std;
void rgb_to_gray(unsigned char* data, int width, int height) {
    // Loop over each pixel
    for (int i = 0; i < width * height; i++) {
        // Calculate the grayscale value using the luminosity method
        unsigned char r = data[3 * i];
        unsigned char g = data[3 * i + 1];
        unsigned char b = data[3 * i + 2];
        unsigned char gray = 0.299 * r + 0.587 * g + 0.114 * b;

        // Set the pixel to the grayscale value
        data[3 * i] = gray;
        data[3 * i + 1] = gray;
        data[3 * i + 2] = gray;
    }
}

const int K = 4; // Number of clusters

void kmeans_clustering(unsigned char* data, int width, int height, int rank) 
{
    //Rank 0 Initializes the cluster centers randomly
    unsigned char centers[K];
    if (rank == 0) {
        srand(time(NULL));
        for (int i = 0; i < K; i++) {
            centers[i] = data[3 * (rand() % (width * height))];
        }
    }
    
    // Broadcast centers array to all processes
    MPI_Bcast(centers, K, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Repeat until convergence
    bool converged = false;
    while (!converged) {
        // Assign each pixel to the closest cluster center
        vector<int> labels(width * height);
        for (int i = 0; i < width * height; i++) {
            int closest_center = 0;
            int closest_distance = abs(data[3 * i] - centers[0]);
            for (int j = 1; j < K; j++) {
                int distance = abs(data[3 * i] - centers[j]);
                if (distance < closest_distance) {
                    closest_center = j;
                    closest_distance = distance;
                }
            }
            labels[i] = closest_center;
        }
        // Update the cluster centers as the mean of the assigned pixels
        int counts[K] = {0};
        unsigned int sums[K] = {0};
        for (int i = 0; i < width * height; i++) {
            int label = labels[i];
            counts[label]++;
            sums[label] += data[3 * i];
        }
        // Reduce counts and sums arrays across all processes
        MPI_Allreduce(MPI_IN_PLACE, counts, K, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, sums, K, MPI_UNSIGNED, MPI_SUM, MPI_COMM_WORLD);

        converged = true;
        for (int i = 0; i < K; i++) {
            if (counts[i] > 0) {
                unsigned char new_center = (unsigned char)(sums[i] / counts[i]);
                if (new_center != centers[i]) {
                    centers[i] = new_center;
                    converged = false;
                }
            }
        }
    }
    // Map each pixel to its corresponding cluster center
    for (int i = 0; i < width * height; i++) {
        int closest_center = 0;
        int closest_distance = abs(data[3 * i] - centers[0]);
        for (int j = 1; j < K; j++) {
            int distance = abs(data[3 * i] - centers[j]);
            if (distance < closest_distance) {
                closest_center = j;
                closest_distance = distance;
            }
        }
        data[3 * i] = centers[closest_center];
        data[3 * i + 1] = centers[closest_center];
        data[3 * i + 2] = centers[closest_center];
    }
}


int main(int argc, const char** argv) {

    MPI_Init(NULL, NULL);
    int rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    long data_height;
    long data_width;
    long data_size;
    unsigned char* data;
    auto start = std::chrono::steady_clock::now();
    
    if(rank == 0)
    {
        // Get dimensions of data to be processed (input data is formatted as HEIGHT|0|WIDTH|0|[bytes for pixels])
        std::ifstream process_input("bin/temp", std::ios::binary);
        std::string data_height_str, data_width_str;
        std::getline(process_input, data_height_str, '\0');
        std::getline(process_input, data_width_str, '\0');
        data_height = std::stoll(data_height_str); // in pixels
        data_width = std::stoll(data_width_str); // in pixels
        data_size = data_height * data_width * 3; // 3 bytes per pixel, representing RGB channels
        std::cerr << data_size << " bytes (" << data_height << " x " << data_width << " x 3)" << std::endl;

        // Get data (pixels) to be processed
        data = (unsigned char*) malloc(data_size);
        process_input.read((char *)&data[0], data_size); // each 3 slots is a single pixel R G B value
        process_input.close();
        start = std::chrono::steady_clock::now();
    }

    MPI_Bcast(&data_size, 1, MPI_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&data_height, 1, MPI_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&data_width, 1, MPI_LONG, 0, MPI_COMM_WORLD);
    // Allocate memory for the local portion of the image data
    long local_size = ((data_size/3) / world_size)*3;
    unsigned char *local_data = (unsigned char *) malloc(local_size * sizeof(unsigned char));

    // Scatter the image data to all processes
    MPI_Scatter(data, local_size, MPI_UNSIGNED_CHAR,
                local_data, local_size, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    //calculate local width and height
    long local_width = data_width / world_size;
    long local_height = data_height;
    // Process it
    rgb_to_gray(local_data,local_width,local_height);
    kmeans_clustering(local_data, local_width, local_height ,rank);

    // Gather the processed image data on rank 0
    unsigned char *final_data = NULL;
    if (rank == 0) {
        final_data = (unsigned char *) malloc(data_size * sizeof(unsigned char));
    }
    MPI_Gather(local_data, local_size, MPI_UNSIGNED_CHAR,
               final_data, local_size, MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);

    // Output the processed image data on rank 0
    if (rank == 0) {
        auto end = std::chrono::steady_clock::now();
        long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        // Write it and terminate
        std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
        process_output.write((char *)&final_data[0], data_size);
        process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
        process_output.flush();
        process_output.close();
    }
    MPI_Finalize();
    return 0;
}











