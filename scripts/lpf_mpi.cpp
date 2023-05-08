#include <chrono>
#include <fstream>
#include <string.h>

int main(int argc, const char** argv) {

    // Get length of data to be processed (input data is formatted as LENGTH|0|[data])
    std::ifstream process_input("bin/temp", std::ios::out | std::ios::binary);
    std::string data_length_str;
    char c = 1;
    while (true) {

        c = process_input.get();
        if (c == 0) break;
        data_length_str.push_back(c);

    }
    unsigned long long data_length = std::stoull(data_length_str);

    // Get the data
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
    std::ofstream process_output("bin/temp", std::ios::out | std::ios::binary);
    process_output.write(inverted_data, data_length);
    process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
    process_output.flush();
    process_output.close();
    return 0;

}