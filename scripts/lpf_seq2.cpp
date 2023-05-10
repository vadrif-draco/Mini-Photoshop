#include <chrono>
#include <fstream>
#include <iostream>
#include <string.h>

void apply_mask(long long (&offsets)[9], float(&mask)[9], long long(&range)[2], char* (&processed), char* (&original)) {

    for (long long i = range[0]; i < range[1]; ++i) {
        processed[i] = 
            float(
                original[i + offsets[0]] * mask[0] + original[i + offsets[1]] * mask[1] + original[i + offsets[2]] * mask[2] +
                original[i + offsets[3]] * mask[3] + original[i + offsets[4]] * mask[4] + original[i + offsets[5]] * mask[5] +
                original[i + offsets[6]] * mask[6] + original[i + offsets[7]] * mask[7] + original[i + offsets[8]] * mask[8]);
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
    char* data = (char*) malloc(data_size);
    process_input.read(data, data_size);
    process_input.close();

    // Preprocessing: Convenience naming
    long long w = data_width * 3; // data width in bytes
    long long& h = data_height; // data height in pixels
    long long& s = data_size; // data size in bytes

    // Preprocessing: Corner pixels start/end bytes (inclusive, exclusive)
    long long topleft[] = { 0, 3 };
    long long topright[] = { w - 3, w };
    long long bottleft[] = { s - w, s - w + 3 };
    long long bottright[] = { s - 3, s };

    // Preprocessing: Edge pixels start/end bytes (inclusive, exclusive)
    long long topedge[] = { 3, w - 3 };
    long long bottedge[] = { s - w + 3, s - 3 };
    long long leftedge[] = { w, s - w };
    long long rightedge[] = { w + w - 3, s - w - 3 };

    // For the aid of visualization...
    // 012 345 678 901 (w = 12, s = 36)
    // xxx xxx xxx xxx
    // xxx xxx xxx xxx
    // xxx xxx xxx xxx

    // Preprocessing: Iterators
    long long i, k;

    // Preprocessing: Filter mask
    // 1/16    1/8    1/16
    // 1/8     1/4    1/8
    // 1/16    1/8    1/16
    // float mask[9] = { 16.0f, 8.0f, 16.0f, 8.0f, 4.0f, 8.0f, 16.0f, 8.0f, 16.0f }; // Gaussian smoothing
    // float mask[9] = { 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f }; // Normal smoothing
    float mask[9] = { 
        
        0.0625f, 0.1250f, 0.0625f,
        0.1250f, 0.2500f, 0.1250f,
        0.0625f, 0.1250f, 0.0625f,
        
    }; // Gausianisni smoothing

    // Process it
    char* blurred_data = (char*) malloc(data_size);
    auto start = std::chrono::steady_clock::now();

    // Regarding offsets:
    // "- w" takes us to the pixel (and channel) directly above
    // "+ 3" takes us to the pixel (and channel) to the right
    // "+ w" takes us to the pixel (and channel) directly below
    // "- 3" takes us to the pixel (and channel) to the left

    long long topleftoffsets[9] = {

        0,    0,    0,
        0,    0,    3,
        0,    w,    w+3
        
    };
    apply_mask(topleftoffsets, mask, topleft, blurred_data, data);

    long long toprightoffsets[9] = {
        
        0,    0,    0,
        -3,   0,    0,
        w-3,  w,    0
        
    };
    apply_mask(toprightoffsets, mask, topright, blurred_data, data);

    long long bottleftoffsets[9] = { 
        
        0,    -w,   -w+3,
        0,    0,    3,
        0,    0,    0

    };
    apply_mask(bottleftoffsets, mask, bottleft, blurred_data, data);

    long long bottrightoffsets[9] = { 
        
        -w-3, -w,   0,
        -3,   0,    0,
        0,    0,    0

    };
    apply_mask(bottrightoffsets, mask, bottright, blurred_data, data);

    long long topedgeoffsets[9] = { 
        
        0,    0,    0,
        -3,   0,    3,
        w-3,  w,    w+3
    
    };
    apply_mask(topedgeoffsets, mask, topedge, blurred_data, data);

    long long bottedgeoffsets[9] = { 
        
        -w-3, -w,   -w+3,
        -3,   0,    3,
        0,    0,    0,

    };
    apply_mask(bottedgeoffsets, mask, bottedge, blurred_data, data);

    // TODO: make it use apply_mask
    long long leftedgeoffsets[9] = { 
        
        0,   -w,   -w+3,
        0,   0,    3,
        0,   w,    w+3,

    };
    for (i = leftedge[0]; i < leftedge[1]; i += w) {
        for (k = i; k < i + 3; k++) {
            blurred_data[k] =
                data[k + leftedgeoffsets[0]] * mask[0] + data[k + leftedgeoffsets[1]] * mask[1] + data[k + leftedgeoffsets[2]] * mask[2] +
                data[k + leftedgeoffsets[3]] * mask[3] + data[k + leftedgeoffsets[4]] * mask[4] + data[k + leftedgeoffsets[5]] * mask[5] +
                data[k + leftedgeoffsets[6]] * mask[6] + data[k + leftedgeoffsets[7]] * mask[7] + data[k + leftedgeoffsets[8]] * mask[8];
        }
    }

    // TODO: make it use apply_mask
    long long rightedgeoffsets[9] = { 
        
        -w-3, -w,   0,
        -3,   0,    0,
        w-3,  w,    0,

    };
    for (i = rightedge[0]; i < rightedge[1]; i += w) {
        for (k = i; k < i + 3; k++) {
            blurred_data[k] =
                data[k + rightedgeoffsets[0]] * mask[0] + data[k + rightedgeoffsets[1]] * mask[1] + data[k + rightedgeoffsets[2]] * mask[2] +
                data[k + rightedgeoffsets[3]] * mask[3] + data[k + rightedgeoffsets[4]] * mask[4] + data[k + rightedgeoffsets[5]] * mask[5] +
                data[k + rightedgeoffsets[6]] * mask[6] + data[k + rightedgeoffsets[7]] * mask[7] + data[k + rightedgeoffsets[8]] * mask[8];
        }
    }

    long long offsets[9] = { 
        
        -w-3, -w,   -w+3,
        -3,   0,    3,
        w-3,  w,    w+3,

    };
    long long iteration_range[2];
    for (i = w + 3; i < s - w; i += w) {

        iteration_range[0] = i;
        iteration_range[1] = i + w - 3 - 3;
        apply_mask(offsets, mask, iteration_range, blurred_data, data);

    }

    auto end = std::chrono::steady_clock::now();
    long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Write it and terminate
    std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
    process_output.write(blurred_data, data_size);
    process_output.write(std::to_string(t).c_str(), std::to_string(t).length());
    process_output.flush();
    process_output.close();
    return 0;

}