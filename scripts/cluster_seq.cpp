#include <chrono>
#include <vector>
#include <cstdlib>
#include <math.h>
#include <fstream>
#include <iostream>
#include <string.h>
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

void kmeans_clustering(unsigned char* data, int width, int height) 
{
    // Initialize the cluster centers randomly
    unsigned char centers[K];
    srand(time(NULL));
    for (int i = 0; i < K; i++) {
        centers[i] = data[3 * (rand() % (width * height))];
    }

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

    // Get dimensions of data to be processed (input data is formatted as HEIGHT|0|WIDTH|0|[bytes for pixels])
    std::ifstream process_input("bin/temp", std::ios::binary);
    std::string data_height_str, data_width_str;
    std::getline(process_input, data_height_str, '\0');
    std::getline(process_input, data_width_str, '\0');
    long long data_height = std::stoll(data_height_str); // in pixels
    long long data_width = std::stoll(data_width_str); // in pixels
    long long data_size = data_height * data_width * 3; // 3 bytes per pixel, representing RGB channels
    std::cerr << data_size << " bytes (" << data_height << " x " << data_width << " x 3)" << std::endl;

    // Get data (pixels) to be processed
    unsigned char* data = (unsigned char*) malloc(data_size);
    process_input.read((char *)&data[0], data_size); // each 3 slots is a single pixel R G B value
    process_input.close();


    // Process it
    auto start = std::chrono::steady_clock::now();

    //todo actually process
    rgb_to_gray(data,data_width,data_height);
    kmeans_clustering(data,data_width,data_height);

    auto end = std::chrono::steady_clock::now();
    long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Write it and terminate
    std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
    process_output.write((char *)&data[0], data_size);
    process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
    process_output.flush();
    process_output.close();
    return 0;
}