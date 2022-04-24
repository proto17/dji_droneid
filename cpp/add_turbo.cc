// To build this you must have installed turbofec [1] and downloaded the CRC.h file from CRCpp [2]
// [1] https://github.com/ttsou/turbofec
// [2] https://github.com/d-bahr/CRCpp/blob/master/inc/CRC.h

// Assuming that CRC.h is in the current directory and you installed libturbofec with defaults, 
// build with the following:
//    g++ -Wall add_turbo.cc -o add_turbo -I. -I/usr/local/include -L/usr/local/lib -lturbofec

// This line is required before the include of CRC.h so that it actually loads the CRC_24_LTEA definition
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

#include <cstring> // strlen
#include <vector>  // std::vector<>

#include <stdint.h> // *int*_t
#include <stdio.h>  // fprintf, fopen, feof, fseek, ftell
#include <stdlib.h> // exit

// The turbofec includes must be imported as extern C otherwise the linker will fail
#ifdef __cplusplus
extern "C" {
#endif

#include <turbofec/rate_match.h>
#include <turbofec/turbo.h>

#ifdef __cplusplus
}
#endif

// From https://github.com/d-bahr/CRCpp/blob/master/inc/CRC.h
#include <CRC.h>

// Number of bytes that the user must provide
// TODO(22April2022): This needs to be flexible in the future.  For now it's hard coded to support the normal DroneID frame
static const int required_bytes = 91;

void usage(const char * const program) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s <input_file_path> <output_file_path>\n", program);
    fprintf(stderr, "\n");
    fprintf(stderr, "  <input_file_path>  => Path to file containing raw bytes to Turbo encode and rate match\n");
    fprintf(stderr, "                        !!Must be %d bytes!!\n", required_bytes);
    fprintf(stderr, "  <output_file_path> => Where to write out the Turbo encoded and rate matched bits\n");
    fprintf(stderr, "\n");
    exit(1);
}

// These values always exist at the end of the frame.  Not sure what they are used for, but they don't change and
// are required for the Turbo code application.  The bytes change between power cycles.  Might just be garbage data
// from the RAM of the drone.  Zeroing out instead of using 'valid' data as it's unclear if the data I have can
// be called 'valid'.  Might also contain identifying information O.o
// If demodulation doesn't work with DJI devices then this might be the issue
const std::vector<uint8_t> tail_bytes(82, 0);

int main(int argc, const char ** argv) {
    // Make sure all required parameters were provided
    if (argc != 3) {
        fprintf(stderr, "[ERROR] Invalid number of arguments\n");
        usage(argv[0]);
    }
    
    // Open the input file
    FILE * handle = fopen(argv[1], "rb");
    if (! handle) {
        fprintf(stderr, "[ERROR] File '%s' does not exist or is not readable\n", argv[1]);
        usage(argv[0]);
    }

    // Calculate the number of bytes in the file
    fseek(handle, 0, SEEK_END);
    const auto num_bytes = ftell(handle);
    fseek(handle, 0, SEEK_SET);

    // Sanity check that all bytes are there
    if (num_bytes != required_bytes) {
        fprintf(stderr, "[ERROR] Invalid number of bytes.  Expected %d, got %ld\n", required_bytes, num_bytes);
        usage(argv[0]);
    }
    
    // Read the bytes into a vector
    std::vector<uint8_t> payload(required_bytes);
    const auto total_read = fread(&payload[0], sizeof(uint8_t), required_bytes, handle);
    if (total_read != required_bytes) {
        fprintf(stderr, "[ERROR] Failed to read all bytes.  Expected %d, read %ld\n", required_bytes, total_read);
        usage(argv[0]);
    }
    
    // Print the bytes that were read from the file as a sanity check
    fprintf(stdout, "Payload: 0x");
    for (const auto & val : payload) {
        fprintf(stdout, "%02x", val);
    }
    fprintf(stdout, "\n");

    // Add on the additional payload bytes that are not a part of the DroneID protocol
    payload.insert(payload.end(), tail_bytes.begin(), tail_bytes.end());
    fprintf(stdout, "Full payload: 0x");
    for (const auto & val : payload) {
        fprintf(stdout, "%02x", val);
    }
    fprintf(stdout, "\n");
    
    // Make sure that all bytes are accounted for prior to calculating the CRC
    const auto pre_crc_byte_count = 173;
    if (payload.size() != pre_crc_byte_count) {
        fprintf(stderr, "[ERROR] Number of total payload bytes is not correct.  Expected %u, got %ld\n", 
            pre_crc_byte_count, payload.size());
        exit(1);
    }
    
    // Calculate the CRC and add it to the payload vector
    const uint32_t calculated_crc = CRC::Calculate(&payload[0], payload.size(), CRC::CRC_24_LTEA());

    fprintf(stdout, "CRC: %06x\n", calculated_crc);

    payload.push_back((calculated_crc >> 16) & 0xff);
    payload.push_back((calculated_crc >>  8) & 0xff);
    payload.push_back((calculated_crc >>  0) & 0xff);

    // Specify the number of bits and bytes that are expected in the steps below.  The bit count has an additional
    // four bits for the Turbo coder tail
    const auto total_payload_bytes = pre_crc_byte_count + 3;
    const auto turbo_encoder_bit_count = (total_payload_bytes * 8) + 4;

    // Allocate buffers for the Turbo decoder.  After rate matching there will be 7200 bits
    std::vector<int8_t> d1(turbo_encoder_bit_count, 0);
    std::vector<int8_t> d2(turbo_encoder_bit_count, 0);
    std::vector<int8_t> d3(turbo_encoder_bit_count, 0);
    std::vector<uint8_t> unpacked_bits;
    std::vector<int8_t> rate_matcher_output(7200, 0);

    // Create the required structure to run the Turbo decoder
    struct lte_turbo_code turbo_encoder {
        .n = 2,
        .k = 4,
        .len = total_payload_bytes * 8,
        .rgen = 11,
        .gen = 13,
    };
    
    // Unpack the bytes into bits
    for (const auto & val : payload) {
        for (int idx = 0; idx < 8; idx++) {
            unpacked_bits.push_back((val >> (7-idx)) & 0x1);
        }
    }
    
    // Using old school casting because for some reason lte_turbo_encode takes a different set of `d` types
    // than the lte_rate_matcher_io struct :(
    lte_turbo_encode(&turbo_encoder, &unpacked_bits[0], (uint8_t*)&d1[0], (uint8_t*)&d2[0], (uint8_t*)&d3[0]);
    
    // Create the struct that controls the rate matcher
    struct lte_rate_matcher_io rate_matcher_io = {
        .D = turbo_encoder_bit_count,
        .E = static_cast<int>(rate_matcher_output.size()),
        .d = {&d1[0], &d2[0], &d3[0]},
        .e = &rate_matcher_output[0]
    };
    
    struct lte_rate_matcher* rate_matcher = lte_rate_matcher_alloc();

    lte_rate_match_fw(rate_matcher, &rate_matcher_io, 0);
    
    // Write out the 7200 bits to the user specified file.  Each bit will be written as a 0x01 or 0x00.
    FILE * output_handle = fopen(argv[2], "wb");
    if (! output_handle) {
        fprintf(stderr, "[ERROR] Could not open output file '%s' for writing\n", argv[2]);
        exit(1);
    }
    
    const auto written = 
        fwrite(&rate_matcher_output[0], sizeof(rate_matcher_output[0]), rate_matcher_output.size(), output_handle);
    
    // Sanity check that all bytes were written
    if (written != rate_matcher_output.size()) {
        fprintf(stderr, "[ERROR] Did not write all data.  Expected %ld elements, got %lu\n", rate_matcher_output.size(), written);
        exit(1);
    }
        
    // Free LTE related pointers
    lte_rate_matcher_free(rate_matcher);
}       
