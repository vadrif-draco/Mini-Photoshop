#include <omp.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string.h>

int main(int argc, const char** argv) {

    // Get dimensions of data to be processed (input data is formatted as HEIGHT|0|WIDTH|0|[bytes for pixels])
    std::ifstream process_input("bin/temp", std::ios::binary);
    std::string data_height_str, data_width_str;
    std::getline(process_input, data_height_str, '\0');
    std::getline(process_input, data_width_str, '\0');
    unsigned long long data_height = std::stoull(data_height_str);
    unsigned long long data_width = std::stoull(data_width_str);
    unsigned long long data_size = data_height * data_width * 3; // 3 for RGB channels
    std::cerr << data_size << " bytes (" << data_height << " x " << data_width << " x 3)" << std::endl;

    // Get data (pixels) to be processed
    char* data = (char*) malloc(data_size);
    process_input.read(data, data_size);
    process_input.close();

    // Process it
    char* inverted_data = (char*) malloc(data_size);
    auto start = std::chrono::steady_clock::now();
    #pragma omp parallel for
    for (unsigned long long i = 0; i < data_size; i++) inverted_data[i] = 0xff - data[i];
    auto end = std::chrono::steady_clock::now();
    unsigned long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Write it and terminate
    std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
    process_output.write(inverted_data, data_size);
    process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
    process_output.flush();
    process_output.close();
    return 0;

}