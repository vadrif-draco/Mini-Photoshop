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
    char* blurred_data = (char*) malloc(data_size);
    auto start = std::chrono::steady_clock::now();
    for (unsigned long long i = 0; i < data_size; i += 3) {
        // Filter:
        // 1/16    1/8    1/16
        // 1/8     1/4    1/8
        // 1/16    1/8    1/16
        
        if (i > 0 && i < ((data_width - 1) * 3)) {
        //     // Top edge:
        //     //   0       0       0  
        //     //  i-1      i      i+1
        //     // i+w-1    i+w    i+w+1
        //     blurred_data[i] = 
        //                      0                       +                 0                  +                 0                    +
        //                1 / 8 * data[i - 3]           +           1 / 4 * data[i]          +           1 / 8 * data[i + 3]        +
        //         1 / 16 * data[i + data_width - 3]    +    1 / 8 * data[i + data_width]    +    1 / 16 * data[i + data_width + 3] ;
        //     blurred_data[i + 1] = 
        //                      0                       +                 0                  +                 0                    +
        //                1 / 8 * data[i - 2]           +           1 / 4 * data[i]          +           1 / 8 * data[i + 4]        +
        //         1 / 16 * data[i + data_width - 2]    +    1 / 8 * data[i + data_width]    +    1 / 16 * data[i + data_width + 4] ;
        //     blurred_data[i + 2] = 
        //                      0                       +                 0                  +                 0                    +
        //                1 / 8 * data[i - 1]           +           1 / 4 * data[i]          +           1 / 8 * data[i + 5]        +
        //         1 / 16 * data[i + data_width - 1]    +    1 / 8 * data[i + data_width]    +    1 / 16 * data[i + data_width + 5] ;
        }
        else if (i > ((data_height - 1) * (data_width - 1) * 3) && i < ((data_height * data_width - 1) * 3)) {
        //     // Bottom edge:
        //     // i-w-1    i-w    i-w+1
        //     //  i-1      i      i+1
        //     //   0       0       0  
        //     blurred_data[i] = 
        //         1 / 16 * data[i - data_width - 3]    +    1 / 8 * data[i - data_width]    +    1 / 16 * data[i - data_width + 3] +
        //                1 / 8 * data[i - 3]           +           1 / 4 * data[i]          +           1 / 8 * data[i + 3]        +
        //                      0                       +                 0                  +                 0                    ;
        //     blurred_data[i + 1] = 
        //         1 / 16 * data[i - data_width - 2]    +    1 / 8 * data[i - data_width]    +    1 / 16 * data[i - data_width + 4] +
        //                1 / 8 * data[i - 2]           +           1 / 4 * data[i]          +           1 / 8 * data[i + 4]        +
        //                      0                       +                 0                  +                 0                    ;
        //     blurred_data[i + 2] = 
        //         1 / 16 * data[i - data_width - 1]    +    1 / 8 * data[i - data_width]    +    1 / 16 * data[i - data_width + 5] +
        //                1 / 8 * data[i - 1]           +           1 / 4 * data[i]          +           1 / 8 * data[i + 5]        +
        //                      0                       +                 0                  +                 0                    ;
        }
        else if (i > 0 && i < ((data_height - 1) * (data_width - 1) * 3) && i%(data_width * 3) == 1) {
        //     // Left edge:
        //     //   0      i-w    i-w+1
        //     //   0       i      i+1
        //     //   0      i+w    i+w+1
        //     blurred_data[i] = 
        //         1 / 16 * data[i - data_width - 3]    +    1 / 8 * data[i - data_width]    +                 0                    +
        //                1 / 8 * data[i - 3]           +           1 / 4 * data[i]          +                 0                    +
        //         1 / 16 * data[i + data_width - 3]    +    1 / 8 * data[i + data_width]    +                 0                    ;
        //     blurred_data[i + 1] = 
        //         1 / 16 * data[i - data_width - 2]    +    1 / 8 * data[i - data_width]    +                 0                    +
        //                1 / 8 * data[i - 2]           +           1 / 4 * data[i]          +                 0                    +
        //         1 / 16 * data[i + data_width - 2]    +    1 / 8 * data[i + data_width]    +                 0                    ;
        //     blurred_data[i + 2] = 
        //         1 / 16 * data[i - data_width - 1]    +    1 / 8 * data[i - data_width]    +                 0                    +
        //                1 / 8 * data[i - 1]           +           1 / 4 * data[i]          +                 0                    +
        //         1 / 16 * data[i + data_width - 1]    +    1 / 8 * data[i + data_width]    +                 0                    ;
        }
        else if (i > ((data_width - 1) * 3) && i < ((data_height * data_width - 1) * 3) && i%(data_width * 3) == 0) {
        //     // Right edge:
        //     // i-w-1    i-w      0  
        //     //  i-1      i       0 
        //     // i+w-1    i+w      0  
        //     blurred_data[i] = 
        //                      0                       +    1 / 8 * data[i - data_width]    +    1 / 16 * data[i - data_width + 3] +
        //                      0                       +           1 / 4 * data[i]          +           1 / 8 * data[i + 3]        +
        //                      0                       +    1 / 8 * data[i + data_width]    +    1 / 16 * data[i + data_width + 3] ;
        //     blurred_data[i + 1] = 
        //                      0                       +    1 / 8 * data[i - data_width]    +    1 / 16 * data[i - data_width + 4] +
        //                      0                       +           1 / 4 * data[i]          +           1 / 8 * data[i + 4]        +
        //                      0                       +    1 / 8 * data[i + data_width]    +    1 / 16 * data[i + data_width + 4] ;
        //     blurred_data[i + 2] = 
        //                      0                       +    1 / 8 * data[i - data_width]    +    1 / 16 * data[i - data_width + 5] +
        //                      0                       +           1 / 4 * data[i]          +           1 / 8 * data[i + 5]        +
        //                      0                       +    1 / 8 * data[i + data_width]    +    1 / 16 * data[i + data_width + 5] ;
        }
        else if (i == 0) {
        //     // Top left:
        //     //   0       0       0
        //     //   0       i      i+1
        //     //   0      i+w    i+w+1
        //     blurred_data[i] = 
        //                      0                       +                 0                  +                 0                    +
        //                      0                       +           1 / 4 * data[i]          +           1 / 8 * data[i + 3]        +
        //                      0                       +    1 / 8 * data[i + data_width]    +    1 / 16 * data[i + data_width + 3] ;
        //     blurred_data[i + 1] = 
        //                      0                       +                 0                  +                 0                    +
        //                      0                       +           1 / 4 * data[i]          +           1 / 8 * data[i + 4]        +
        //                      0                       +    1 / 8 * data[i + data_width]    +    1 / 16 * data[i + data_width + 4] ;
        //     blurred_data[i + 2] = 
        //                      0                       +                 0                  +                 0                    +
        //                      0                       +           1 / 4 * data[i]          +           1 / 8 * data[i + 5]        +
        //                      0                       +    1 / 8 * data[i + data_width]    +    1 / 16 * data[i + data_width + 5] ;
        }
        else if (i == ((data_width - 1) * 3)) {
        //     // Top right:
        //     //   0       0       0
        //     //  i-1      i       0
        //     // i+w-1    i+w      0
        //     blurred_data[i] = 
        //                      0                       +                 0                  +                 0                    +
        //                 1 / 8 * data[i - 3]          +           1 / 4 * data[i]          +                 0                    +
        //         1 / 16 * data[i + data_width - 3]    +    1 / 8 * data[i + data_width]    +                 0                    ;
        //     blurred_data[i + 1] = 
        //                      0                       +                 0                  +                 0                    +
        //                 1 / 8 * data[i - 2]          +           1 / 4 * data[i]          +                 0                    +
        //         1 / 16 * data[i + data_width - 2]    +    1 / 8 * data[i + data_width]    +                 0                    ;
        //     blurred_data[i + 2] = 
        //                      0                       +                 0                  +                 0                    +
        //                 1 / 8 * data[i - 1]          +           1 / 4 * data[i]          +                 0                    +
        //         1 / 16 * data[i + data_width - 1]    +    1 / 8 * data[i + data_width]    +                 0                    ;
        }
        else if (i == ((data_height - 1) * (data_width - 1) * 3)) {
        //     // Bottom left:
        //     //   0      i-w    i-w+1
        //     //   0       i      i+1
        //     //   0       0       0
        //     blurred_data[i] = 
        //                      0                       +    1 / 8 * data[i - data_width]    +    1 / 16 * data[i - data_width + 3] +
        //                      0                       +           1 / 4 * data[i]          +           1 / 8 * data[i + 3]        +
        //                      0                       +                 0                  +                 0                    ;
        //     blurred_data[i + 1] = 
        //                      0                       +    1 / 8 * data[i - data_width]    +    1 / 16 * data[i - data_width + 4] +
        //                      0                       +           1 / 4 * data[i]          +           1 / 8 * data[i + 4]        +
        //                      0                       +                 0                  +                 0                    ;
        //     blurred_data[i + 2] = 
        //                      0                       +    1 / 8 * data[i - data_width]    +    1 / 16 * data[i - data_width + 5] +
        //                      0                       +           1 / 4 * data[i]          +           1 / 8 * data[i + 5]        +
        //                      0                       +                 0                  +                 0                    ;
        }
        else if (i == ((data_height * data_width - 1) * 3)) {
        //     // Bottom right:
        //     // i-w-1    i-w      0
        //     //  i-1      i       0
        //     //   0       0       0
        //     blurred_data[i] = 
        //         1 / 16 * data[i - data_width - 3]    +    1 / 8 * data[i - data_width]    +                 0                    +
        //                1 / 8 * data[i - 3]           +           1 / 4 * data[i]          +                 0                    +
        //                      0                       +                 0                  +                 0                    ;
        //     blurred_data[i + 1] = 
        //         1 / 16 * data[i - data_width - 2]    +    1 / 8 * data[i - data_width]    +                 0                    +
        //                1 / 8 * data[i - 2]           +           1 / 4 * data[i]          +                 0                    +
        //                      0                       +                 0                  +                 0                    ;
        //     blurred_data[i + 2] = 
        //         1 / 16 * data[i - data_width - 1]    +    1 / 8 * data[i - data_width]    +                 0                    +
        //                1 / 8 * data[i - 1]           +           1 / 4 * data[i]          +                 0                    +
        //                      0                       +                 0                  +                 0                    ;
        }
        else {
            // Normal case:
            // i-w-1    i-w    i-w+1
            //  i-1      i      i+1
            // i+w-1    i+w    i+w+1
            int w = data_width * 3; // data_width is in pixels, not bytes; each pixel has 3 bytes for R G B
            blurred_data[i] = 
                data[i - w - 3] / 16.0    +    data[i - w]     / 8.0    +    data[i - w + 3] / 16.0    +
                data[i - 3]     / 8.0     +    data[i]         / 4.0    +    data[i + 3]     / 8.0     +
                data[i + w - 3] / 16.0    +    data[i + w]     / 8.0    +    data[i + w + 3] / 16.0    ;
            blurred_data[i + 1] = 
                data[i - w - 2] / 16.0    +    data[i - w + 1] / 8.0    +    data[i - w + 4] / 16.0    +
                data[i - 2]     / 8.0     +    data[i + 1]     / 4.0    +    data[i + 4]     / 8.0     +
                data[i + w - 2] / 16.0    +    data[i + w + 1] / 8.0    +    data[i + w + 4] / 16.0    ;
            blurred_data[i + 2] = 
                data[i - w - 1] / 16.0    +    data[i - w + 2] / 8.0    +    data[i - w + 5] / 16.0    +
                data[i - 1]     / 8.0     +    data[i + 2]     / 4.0    +    data[i + 5]     / 8.0     +
                data[i + w - 1] / 16.0    +    data[i + w + 2] / 8.0    +    data[i + w + 5] / 16.0    ;
        }
    }
    auto end = std::chrono::steady_clock::now();
    unsigned long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Write it and terminate
    std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
    process_output.write(blurred_data, data_size);
    process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
    process_output.flush();
    process_output.close();
    return 0;

}