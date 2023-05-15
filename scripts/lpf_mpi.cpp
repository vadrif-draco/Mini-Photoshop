#include <chrono>
#include <math.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <mpi.h>

void apply_mask(long long(&offsets)[9], float(&mask)[9], long long(&range)[2], unsigned char* (&processed), unsigned char* (&original)) {

    for (long long i = range[0]; i < range[1]; ++i) {
        processed[i] = (float(
            original[i + offsets[0]] * mask[0] + original[i + offsets[1]] * mask[1] + original[i + offsets[2]] * mask[2] +
            original[i + offsets[3]] * mask[3] + original[i + offsets[4]] * mask[4] + original[i + offsets[5]] * mask[5] +
            original[i + offsets[6]] * mask[6] + original[i + offsets[7]] * mask[7] + original[i + offsets[8]] * mask[8]));
    }

}

void apply_maskv(long long(&offsets)[9], float(&mask)[9], long long(&range)[2], long long(&rowsize), unsigned char* (&processed), unsigned char* (&original)) {

    for (long long i = range[0]; i < range[1]; i += rowsize) {
        for (long long k = i; k < i + 3; k++) {
            processed[k] = (float(
                original[k + offsets[0]] * mask[0] + original[k + offsets[1]] * mask[1] + original[k + offsets[2]] * mask[2] +
                original[k + offsets[3]] * mask[3] + original[k + offsets[4]] * mask[4] + original[k + offsets[5]] * mask[5] +
                original[k + offsets[6]] * mask[6] + original[k + offsets[7]] * mask[7] + original[k + offsets[8]] * mask[8]));
        }
    }

}

int main(int argc, const char** argv) {

    MPI_Init(nullptr, nullptr);
    int rank, num_processes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    long long data_height;      // Height in pixels
    long long data_width;       // Width in pixels
    long long data_size;        // 3 bytes per pixel, representing RGB channels
    unsigned char* data;        // Data (pixels) to be processed
    long long w;                // Data width in bytes
    long long& s = data_size;   // Data size in bytes
    unsigned char* blurred_data;// Processed data
    long long t_edges;          // Time taken to process edges and corners

    // Preprocessing: Filter mask(s)
    float mask[9] = {

        0.0625f, 0.1250f, 0.0625f,
        0.1250f, 0.2500f, 0.1250f,
        0.0625f, 0.1250f, 0.0625f,

    }; // Gaussian smoothing
    // float mask[9] = {

    //     1 / 9.0f, 1 / 9.0f, 1 / 9.0f,
    //     1 / 9.0f, 1 / 9.0f, 1 / 9.0f,
    //     1 / 9.0f, 1 / 9.0f, 1 / 9.0f,

    // }; // Normal smoothing

    // More information in lines 105-117
    long long alloffsets[9] = {

        -w - 3, -w,     -w + 3,
        -3,     0,      3,
        w - 3,  w,      w + 3,

    };

    if (rank == 0) {

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
        process_input.read((char *)&data[0], data_size);
        process_input.close();

        // Preprocessing: Convenience naming
        w = data_width * 3; // data width in bytes

        // Preprocessing: Corner pixels start/end bytes (inclusive, exclusive)
        long long topleft[] = { 0, 3 };
        long long topright[] = { w - 3, w };
        long long bottleft[] = { s - w, s - w + 3 };
        long long bottright[] = { s - 3, s };

        // Preprocessing: Edge pixels start/end bytes (inclusive, exclusive)
        long long topedge[] = { 3, w - 3 };
        long long bottedge[] = { s - w + 3, s - 3 };
        long long leftedge[] = { w, s - w }; // Dealt with in pixel triples rather than bytes
        long long rightedge[] = { w + w - 3, s - 3 }; // Dealt with in pixel triples rather than bytes

        // Preprocessing: For the aid of visualization...
        //    012 345 678 901
        // 00 xxx xxx xxx xxx 11
        // 12 xxx xxx xxx xxx 23
        // 24 xxx xxx xxx xxx 35
        // 36 xxx xxx xxx xxx 47
        // 48 xxx xxx xxx xxx 59 (s = 60)

        // Preprocessing: Regarding offsets:
        // "- w" takes us to the pixel (and channel) directly above
        // "+ 3" takes us to the pixel (and channel) to the right
        // "+ w" takes us to the pixel (and channel) directly below
        // "- 3" takes us to the pixel (and channel) to the left

        long long topleftoffsets[9] = {

            0,      0,      0,
            0,      0,      3,
            0,      w,      w + 3

        };

        long long toprightoffsets[9] = {

            0,      0,      0,
            -3,     0,      0,
            w - 3,  w,      0

        };

        long long bottleftoffsets[9] = {

            0,      -w,     -w + 3,
            0,      0,      3,
            0,      0,      0

        };

        long long bottrightoffsets[9] = {

            -w - 3, -w,     0,
            -3,     0,      0,
            0,      0,      0

        };

        long long topedgeoffsets[9] = {

            0,      0,      0,
            -3,     0,      3,
            w - 3,  w,      w + 3

        };

        long long bottedgeoffsets[9] = {

            -w - 3, -w,     -w + 3,
            -3,     0,      3,
            0,      0,      0,

        };

        long long leftedgeoffsets[9] = {

            0,      -w,     -w + 3,
            0,      0,      3,
            0,      w,      w + 3,

        };

        long long rightedgeoffsets[9] = {

            -w - 3, -w,     0,
            -3,     0,      0,
            w - 3,  w,      0,

        };

        // Process it
        blurred_data = (unsigned char*) malloc(data_size);
        auto start_edges = std::chrono::steady_clock::now();
        // Processing: Corners
        apply_mask(topleftoffsets, mask, topleft, blurred_data, data);
        apply_mask(toprightoffsets, mask, topright, blurred_data, data);
        apply_mask(bottleftoffsets, mask, bottleft, blurred_data, data);
        apply_mask(bottrightoffsets, mask, bottright, blurred_data, data);
        // Processing: Edges
        apply_mask(topedgeoffsets, mask, topedge, blurred_data, data);
        apply_mask(bottedgeoffsets, mask, bottedge, blurred_data, data);
        apply_maskv(leftedgeoffsets, mask, leftedge, w, blurred_data, data);
        apply_maskv(rightedgeoffsets, mask, rightedge, w, blurred_data, data);
        auto end_edges = std::chrono::steady_clock::now();
        t_edges = std::chrono::duration_cast<std::chrono::milliseconds>(end_edges - start_edges).count();
    }

    // Important broadcasts for the code to function properly
    MPI_Bcast(&data_width, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&data_size, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    if (rank != 0) {
        w = data_width * 3;
        data = (unsigned char*) malloc(data_size);
        blurred_data = (unsigned char*) malloc(data_size);

    }
    MPI_Bcast(&data, data_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(&blurred_data, data_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Processing: Body
    long long iteration_range[2];
    // Start = w + 3
    // End = s - w
    long long iteration_size = (s - w - w - 3) / num_processes;
    long long iteration_start = w + 3 + rank * iteration_size;
    long long iteration_end = iteration_start + iteration_size;
    auto start = std::chrono::steady_clock::now();
    for (long long i = iteration_start; i < iteration_end; i += w) {

        iteration_range[0] = i;
        iteration_range[1] = i + w - 3 - 3;
        apply_mask(alloffsets, mask, iteration_range, blurred_data, data);

    }
    MPI_Gather(blurred_data, iteration_size, MPI_UNSIGNED_CHAR, blurred_data, iteration_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    auto end = std::chrono::steady_clock::now();
    long long t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (rank == 0)
    {
        // Write it and terminate
        std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
        process_output.write((char *)&blurred_data[0], data_size);
        process_output.write(std::to_string(t + t_edges).c_str(), std::to_string(t + t_edges).length());
        process_output.flush();
        process_output.close();
    }

    MPI_Finalize();
    
    return 0;

}