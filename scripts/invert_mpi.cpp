#include <chrono>
#include <fstream>
#include <iostream>
#include <string.h>

int main(int argc, const char** argv) {

    // Get dimensions of data to be processed (input data is formatted as HEIGHT|0|WIDTH|0|[bytes for pixels])
    std::ifstream process_input("bin/temp", std::ios::binary);
    std::string image_height_str, image_width_str;
    std::getline(process_input, image_height_str, '\0');
    std::getline(process_input, image_width_str, '\0');
    unsigned long long image_height = std::stoull(image_height_str);
    unsigned long long image_width = std::stoull(image_width_str);
    unsigned long long data_length = image_height * image_width * 3; // 3 for RGB channels
    std::cerr << data_length << " bytes (" << image_height << " x " << image_width << " x 3)" << std::endl;

    // Get data (pixels) to be processed
    char* data = (char*) malloc(data_length);
    process_input.read(data, data_length);
    process_input.close();

    // Process it
    char* inverted_data = (char*) malloc(data_length);
    auto start = std::chrono::steady_clock::now();
    for (unsigned long long i = 0; i < data_length; i++) inverted_data[i] = 0xff - data[i];
    auto end = std::chrono::steady_clock::now();
    unsigned long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Write it and terminate
    std::ofstream process_output("bin/temp", std::ios::binary);
    process_output.write(inverted_data, data_length);
    process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
    process_output.flush();
    process_output.close();
    return 0;

}