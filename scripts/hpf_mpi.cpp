#include <chrono>
#include <math.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <mpi.h>

void apply_mask(long long(&offsets)[9], float(&mask)[9], long long(&range)[2], unsigned char* (&processed), unsigned char* (&original)) {

    for (long long i = range[0]; i < range[1]; ++i) {
        processed[i] = (float((
            original[i + offsets[0]] * mask[0] + original[i + offsets[1]] * mask[1] + original[i + offsets[2]] * mask[2] +
            original[i + offsets[3]] * mask[3] + original[i + offsets[4]] * mask[4] + original[i + offsets[5]] * mask[5] +
            original[i + offsets[6]] * mask[6] + original[i + offsets[7]] * mask[7] + original[i + offsets[8]] * mask[8])+
            8 * 255) / 16.0f);
    }

}

void apply_maskv(long long(&offsets)[9], float(&mask)[9], long long(&range)[2], long long(&rowsize), unsigned char* (&processed), unsigned char* (&original)) {

    for (long long i = range[0]; i < range[1]; i += rowsize) {
        for (long long k = i; k < i + 3; k++) {
            processed[k] = (float((
                original[k + offsets[0]] * mask[0] + original[k + offsets[1]] * mask[1] + original[k + offsets[2]] * mask[2] +
                original[k + offsets[3]] * mask[3] + original[k + offsets[4]] * mask[4] + original[k + offsets[5]] * mask[5] +
                original[k + offsets[6]] * mask[6] + original[k + offsets[7]] * mask[7] + original[k + offsets[8]] * mask[8])+
                8 * 255) / 16.0f);
        }
    }

}

int main(int argc, const char** argv) {

    MPI_Init(nullptr, nullptr);
    int rank, num_processes;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

    long long data_height;          // Height in pixels
    long long data_width;           // Width in pixels
    long long data_size;            // 3 bytes per pixel, representing RGB channels
    unsigned char* data;            // Data (pixels) to be processed
    unsigned char* features_data;   // Processed data (gray in areas with no change)
    unsigned char* sharpened_data;  // Sharpened data after applying the filter to original image
    long long t_features;
    long long t_features_edges;


    // Preprocessing: Filter mask(s)
    float mask[9] = {

        -1.0f, -1.0f, -1.0f,
        -1.0f, 8.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

    }; // Features detection mask
    
    // float mask[9] = {

    //     0.0f, -1.0f, 0.0f,
    //     -1.0f, 4.0f, -1.0f,
    //     0.0f, -1.0f, 0.0f,

    // }; // Edge detection mask

    if (rank == 0) {
        // Get dimensions of data to be processed (input data is formatted as HEIGHT|0|WIDTH|0|[bytes for pixels])
        std::ifstream process_input("bin/temp", std::ios::binary);
        std::string data_height_str, data_width_str;
        std::getline(process_input, data_height_str, '\0');
        std::getline(process_input, data_width_str, '\0');
        data_height = std::stoll(data_height_str); // in pixels
        data_width = std::stoll(data_width_str); // in pixels
        data_size = data_height * data_width * 3; // 3 bytes per pixel, representing RGB channels
        fprintf(stderr, "%lld bytes (%lld x %lld x 3)\n", data_size, data_height, data_width);
        fflush(stderr);

        // Get data (pixels) to be processed
        data = (unsigned char*) malloc(data_size);
        process_input.read((char *)&data[0], data_size);
        process_input.close();
    }

    features_data = (unsigned char*) malloc(data_size);
    sharpened_data = (unsigned char*) malloc(data_size);

    // Important broadcasts for the code to function properly
    MPI_Bcast(&data_height, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&data_width, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&data_size, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    if (rank != 0) data = (unsigned char*) malloc(data_size);
    MPI_Bcast(data, data_size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Preprocessing: Convenience naming
    long long w = data_width * 3; // data width in bytes
    long long& h = data_height; // data height in pixels
    long long& s = data_size; // data size in bytes

    // Preprocessing: Offsets (more information in other "preprocessing" lines later)
    long long alloffsets[9] = {

        -w - 3, -w,     -w + 3,
        -3,     0,      3,
        w - 3,  w,      w + 3,

    };

    // Processing: Body
    long long iteration_range[2];
    long long num_of_rows = (data_height - 2) / num_processes; // Number of rows to be processed per process
    long long remainder_rows = (data_height - 2) % num_processes; // Number of remaining rows due to individibility
    long long first_row = 1 + rank * num_of_rows;
    long long last_row = first_row + num_of_rows;
    unsigned char* local_features_data = (unsigned char*) malloc(data_size);
    auto start = std::chrono::steady_clock::now();
    for (long long i = first_row; i < last_row; ++i) {

        iteration_range[0] = i * w + 3; // + 3 to exclude the first pixel
        iteration_range[1] = i * w + w - 3; // - 3 to exclude the last pixel
        apply_mask(alloffsets, mask, iteration_range, local_features_data, data);

    }

    int* displs = (int*) malloc(num_processes * sizeof(int));
    int* rcounts = (int*) malloc(num_processes * sizeof(int));
    for (int r = 0; r < num_processes; ++r) { // r for rank
        rcounts[r] = num_of_rows * w; // Receiving entire rows
        displs[r] = r * num_of_rows * w; // How many bytes to skip for this process
    }

    // MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    int ret = MPI_Gatherv(

        &local_features_data[first_row * w], // The beginning of the local data buffer from which we're sending elements to the root
        num_of_rows * w, // The number of elements to be sent (note that we're sending entire rows, including edge pixels!)
        // This is intentional, and that is why we process edges later in the code, to overwrite the garbage here
        MPI_UNSIGNED_CHAR, // The type of the elements

        &features_data[w], // The root's data buffer that we're receiving into (starting from w to skip first row)
        rcounts, // The number of elements to be received (per process)
        displs, // The offset (displacement) into the root's data buffer (per process)
        MPI_UNSIGNED_CHAR, // The type of the elements

        0, // Za root
        MPI_COMM_WORLD // Za warudo

    );

    // switch (ret) {
    //     case MPI_SUCCESS: fprintf(stderr, "Rank %d: MPI_Gatherv returned MPI_SUCCESS\n", rank); break;
    //     case MPI_ERR_COMM: fprintf(stderr, "Rank %d: MPI_Gatherv returned MPI_ERR_COMM\n", rank); break;
    //     case MPI_ERR_TYPE: fprintf(stderr, "Rank %d: MPI_Gatherv returned MPI_ERR_TYPE\n", rank); break;
    //     case MPI_ERR_BUFFER: fprintf(stderr, "Rank %d: MPI_Gatherv returned MPI_ERR_BUFFER\n", rank); break;
    //     default: break;
    // }
    // fflush(stderr);

    if (rank == 0)
    {

        // Calculating the remaining rows
        int starting_remainder_row = data_height - remainder_rows - 1;
        for (int i = starting_remainder_row; i < data_height - 1; ++i) {

            iteration_range[0] = i * w + 3; // + 3 to exclude the first pixel
            iteration_range[1] = i * w + w - 3; // - 3 to exclude the last pixel
            apply_mask(alloffsets, mask, iteration_range, features_data, data);

        }

        // Calculate end and time taken by processing body
        auto end = std::chrono::steady_clock::now();
        t_features = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

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

        auto start_edges = std::chrono::steady_clock::now();
        // Processing: Corners
        apply_mask(topleftoffsets, mask, topleft, features_data, data);
        apply_mask(toprightoffsets, mask, topright, features_data, data);
        apply_mask(bottleftoffsets, mask, bottleft, features_data, data);
        apply_mask(bottrightoffsets, mask, bottright, features_data, data);
        // Processing: Edges
        apply_mask(topedgeoffsets, mask, topedge, features_data, data);
        apply_mask(bottedgeoffsets, mask, bottedge, features_data, data);
        apply_maskv(leftedgeoffsets, mask, leftedge, w, features_data, data);
        apply_maskv(rightedgeoffsets, mask, rightedge, w, features_data, data);
        auto end_edges = std::chrono::steady_clock::now();
        t_features_edges = std::chrono::duration_cast<std::chrono::milliseconds>(end_edges - start_edges).count();
    }

    long long pixels_per_process = data_size / num_processes;
    unsigned char* local_sharpened_data = (unsigned char*) malloc(data_size);

    MPI_Bcast(&features_data, data_size, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    auto start_sharpen = std::chrono::steady_clock::now();
    MPI_Scatter(
        
        features_data, 
        pixels_per_process, 
        MPI_UNSIGNED_CHAR, 
        local_features_data, 
        pixels_per_process, 
        MPI_UNSIGNED_CHAR, 
        0, 
        MPI_COMM_WORLD
        
    );

    float pixel_val;
    for (long long i = 0; i < pixels_per_process; ++i) {
        pixel_val = (local_features_data[i] > 124 && local_features_data[i] < 130) ? 
                    data[rank * pixels_per_process + i] : 
                    0.05 * local_features_data[i] + data[rank * pixels_per_process + i];
        local_sharpened_data[i] = pixel_val > 255 ? 255 : pixel_val;
    }
    
    MPI_Gather(

        local_sharpened_data,
        pixels_per_process,
        MPI_UNSIGNED_CHAR,
        sharpened_data,
        pixels_per_process,
        MPI_UNSIGNED_CHAR,
        0,
        MPI_COMM_WORLD

    );

    if (rank == 0)
    {
        auto end_sharpen = std::chrono::steady_clock::now();
        auto t_sharpen = std::chrono::duration_cast<std::chrono::milliseconds>(end_sharpen - start_sharpen).count();
        // Write it and terminate
        std::ofstream process_output("bin/temp", std::ios::trunc | std::ios::binary);
        process_output.write((char *)&sharpened_data[0], data_size);
        process_output.write(std::to_string(t_features + t_features_edges + t_sharpen).c_str(), std::to_string(t_features + t_features_edges + t_sharpen).length());
        process_output.flush();
        process_output.close();
    }
    
    MPI_Finalize();

    return 0;

}