#include <chrono>
#include <math.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <vector>
#include <omp.h>

typedef long long ll;
const int BYTE = 256;


int main(int argc, const char** argv) {

    // Get dimensions of data to be processed (input data is formatted as HEIGHT|0|WIDTH|0|[bytes for pixels])
    std::ifstream process_input("bin/temp", std::ios::binary);
    std::string data_height_str, data_width_str;
    std::getline(process_input, data_height_str, '\0');
    std::getline(process_input, data_width_str, '\0');
    ll data_height = std::stoull(data_height_str); // in pixels
    ll data_width = std::stoull(data_width_str); // in pixels
    ll pixels = data_height * data_width;
    ll data_size = pixels * 3; // 3 bytes per pixel, representing RGB channels
    std::cerr << data_size << " bytes (" << data_height << " x " << data_width << " x 3)" << std::endl;

    // Get data (pixels) to be processed
    unsigned char* data = (unsigned char*) malloc(data_size);
    process_input.read((char*) &data[0], data_size);
    process_input.close();


    // Process it

    unsigned char* proc_data = (unsigned char*) malloc(data_size);
    auto start = std::chrono::steady_clock::now();

    double prob[BYTE];

    // Process each channel separately
    for (int ch = 0;ch < 3;ch++)
    {
        std::fill(std::begin(prob), std::end(prob),0.0);


            // Pixel intensity frequency calculation
            #pragma omp parallel for reduction(+:prob[:BYTE])
            for(ll i=ch;i<data_size;i+=3)
            {
                prob[data[i]]++;
            }

            // Frequency Normalization
            #pragma omp parallel for
            for(int op = 0;op<BYTE;op++)
            {
                prob[op] /= pixels;
            }

            // Probability cumulative sum
            /*
             * This loop cannot be parallelized using conventional techniques since current value depends
             * on the previous one. But luckily the array size is constant at 256 and does not depend on image
             * size so we can safely leave this part sequential and it won't have any significant effect on performance.
             * */
            for(int op = 1;op<BYTE;op++)
            {
                prob[op] += prob[op-1];
            }

            #pragma omp parallel for
            for(ll i=ch;i<data_size;i+=3)
            {
                double newVal = prob[data[i]] * 255;
                proc_data[i] = (unsigned char) floor(newVal);
            }

    }

    auto end = std::chrono::steady_clock::now();
    unsigned long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();


    // Write it and terminate
    std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
    process_output.write((char*) &proc_data[0], data_size);
    process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
    process_output.flush();
    process_output.close();
    return 0;

}