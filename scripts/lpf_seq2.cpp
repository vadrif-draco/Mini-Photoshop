#include <chrono>
#include <fstream>
#include <iostream>
#include <string.h>

void apply_mask_normal(float(&mask)[9], unsigned long long(&range)[2], unsigned long long(&w), char* (&processed), char* (&original)) {

    for (unsigned long long i = range[0]; i < range[1]; ++i) {
        processed[i] =
            original[i - w - 3] / mask[0] + original[i - w + 0] / mask[1] + original[i - w + 3] / mask[2] +
            original[i + 0 - 3] / mask[3] + original[i + 0 + 0] / mask[4] + original[i + 0 + 3] / mask[5] +
            original[i + w - 3] / mask[6] + original[i + w + 0] / mask[7] + original[i + w + 3] / mask[8];
    }

}

int main(int argc, const char** argv) {

    // Get dimensions of data to be processed (input data is formatted as HEIGHT|0|WIDTH|0|[bytes for pixels])
    std::ifstream process_input("bin/temp", std::ios::binary);
    std::string data_height_str, data_width_str;
    std::getline(process_input, data_height_str, '\0');
    std::getline(process_input, data_width_str, '\0');
    unsigned long long data_height = std::stoull(data_height_str); // in pixels
    unsigned long long data_width = std::stoull(data_width_str); // in pixels
    unsigned long long data_size = data_height * data_width * 3; // 3 bytes per pixel, representing RGB channels
    std::cerr << data_size << " bytes (" << data_height << " x " << data_width << " x 3)" << std::endl;

    // Get data (pixels) to be processed
    char* data = (char*) malloc(data_size);
    process_input.read(data, data_size);
    process_input.close();

    // Preprocessing: Convenience naming
    unsigned long long w = data_width * 3; // data width in bytes
    unsigned long long& h = data_height; // data height in pixels
    unsigned long long& s = data_size; // data size in bytes

    // Preprocessing: Corner pixels start/end bytes (inclusive, exclusive)
    unsigned long long topleft[] = { 0, 3 };
    unsigned long long topright[] = { w - 3, w };
    unsigned long long bottleft[] = { s - w, s - w + 3 };
    unsigned long long bottright[] = { s - 3, s };

    // Preprocessing: Edge pixels start/end bytes (inclusive, exclusive)
    unsigned long long topedge[] = { 3, w - 3 };
    unsigned long long bottedge[] = { s - w + 3, s - 3 };
    unsigned long long leftedge[] = { w, s - w };
    unsigned long long rightedge[] = { w - 3, s - w - 3 + 1 };

    // Preprocessing: Iterators
    unsigned long long i, k;

    // Preprocessing: Filter mask
    // 1/16    1/8    1/16
    // 1/8     1/4    1/8
    // 1/16    1/8    1/16
    float mask[9] = { 16.0f, 8.0f, 16.0f, 8.0f, 4.0f, 8.0f, 16.0f, 8.0f, 16.0f }; // Gaussian smoothing
    // float mask[9] = { 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f }; // Normal smoothing

    // Process it
    char* blurred_data = (char*) malloc(data_size);
    auto start = std::chrono::steady_clock::now();

    // Top left:
    //   0       0       0
    //   0       i      i+1
    //   0      i+w    i+w+1
    for (i = topleft[0]; i < topleft[1]; ++i) {
        blurred_data[i] =
            0 + 0 + 0 +
            0 + data[i + 0 + 0] / mask[4] + data[i + 0 + 3] / mask[5] +
            0 + data[i + w + 0] / mask[7] + data[i + w + 3] / mask[8];
    }


    // Top right:
    //   0       0       0
    //  i-1      i       0
    // i+w-1    i+w      0
    for (i = topright[0]; i < topright[1]; ++i) {
        blurred_data[i] =
            0 + 0 + 0 +
            data[i + 0 - 3] / mask[3] + data[i + 0 + 0] / mask[4] + 0 +
            data[i + w - 3] / mask[6] + data[i + w + 0] / mask[7] + 0;
    }

    // Bottom left:
    //   0      i-w    i-w+1
    //   0       i      i+1
    //   0       0       0
    for (i = bottleft[0]; i < bottleft[1]; ++i) {
        blurred_data[i] =
            0 + data[i - w + 0] / mask[1] + data[i - w + 3] / mask[2] +
            0 + data[i + 0 + 0] / mask[4] + data[i + 0 + 3] / mask[5] +
            0 + 0 + 0;
    }

    // Bottom right:
    // i-w-1    i-w      0
    //  i-1      i       0
    //   0       0       0
    for (i = bottright[0]; i < bottright[1]; ++i) {
        blurred_data[i] =
            data[i - w - 3] / mask[0] + data[i - w + 0] / mask[1] + 0 +
            data[i + 0 - 3] / mask[3] + data[i + 0 + 0] / mask[4] + 0 +
            0 + 0 + 0;
    }

    // Top edge:
    //   0       0       0  
    //  i-1      i      i+1
    // i+w-1    i+w    i+w+1
    for (i = topedge[0]; i < topedge[1]; ++i) {
        blurred_data[i] =
            0 + 0 + 0 +
            data[i + 0 - 3] / mask[3] + data[i + 0 + 0] / mask[4] + data[i + 0 + 3] / mask[5] +
            data[i + w - 3] / mask[6] + data[i + w + 0] / mask[7] + data[i + w + 3] / mask[8];
    }

    // Bottom edge:
    // i-w-1    i-w    i-w+1
    //  i-1      i      i+1
    //   0       0       0
    for (i = bottedge[0]; i < bottedge[1]; ++i) {
        blurred_data[i] =
            data[i - w - 3] / mask[0] + data[i - w + 0] / mask[1] + data[i - w + 3] / mask[2] +
            data[i + 0 - 3] / mask[3] + data[i + 0 + 0] / mask[4] + data[i + 0 + 3] / mask[5] +
            0 + 0 + 0;
    }

    // Left edge:
    //   0      i-w    i-w+1
    //   0       i      i+1
    //   0      i+w    i+w+1
    for (i = leftedge[0]; i < leftedge[1]; i += w) {
        for (k = i; k < i + 3; k++) {
            blurred_data[k] =
                0 + data[k - w + 0] / mask[1] + data[k - w + 3] / mask[2] +
                0 + data[k + 0 + 0] / mask[4] + data[k + 0 + 3] / mask[5] +
                0 + data[k + w + 0] / mask[7] + data[k + w + 3] / mask[8];
        }
    }

    // Right edge:
    // i-w-1    i-w      0  
    //  i-1      i       0 
    // i+w-1    i+w      0
    for (i = rightedge[0]; i < rightedge[1]; i += w) {
        for (k = i; k < i + 3; k++) {
            blurred_data[k] =
                data[k - w - 3] / mask[0] + data[k - w + 0] / mask[1] + 0 +
                data[k + 0 - 3] / mask[3] + data[k + 0 + 0] / mask[4] + 0 +
                data[k + w - 3] / mask[6] + data[k + w + 0] / mask[7] + 0;
        }
    }

    // Normal case:
    // i-w-1    i-w    i-w+1
    //  i-1      i      i+1
    // i+w-1    i+w    i+w+1
    // "+ w" takes us to the pixel (and channel) directly above
    // "+ 3" takes us to the pixel (and channel) to the right
    // "- w" takes us to the pixel (and channel) directly below
    // "- 3" takes us to the pixel (and channel) to the left
    for (i = w + 3; i < s - w; i += w) {

        unsigned long long range[2] = { i, i + w - 3 - 3 };
        apply_mask_normal(mask, range, w, blurred_data, data);

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