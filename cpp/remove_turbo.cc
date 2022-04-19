// To build this you must have installed turbofec [1] and downloaded the CRC.h file from CRCpp [2]
// [1] https://github.com/ttsou/turbofec
// [2] https://github.com/d-bahr/CRCpp/blob/master/inc/CRC.h

// Assuming that CRC.h is in the current directory and you installed libturbofec with defaults, 
// build with the following:
//    g++ -Wall remove_turbo.cc -o remove_turbo -I. -I/usr/local/include -L/usr/local/lib -lturbofec

// This line is required before the include of CRC.h so that it actually loads the CRC_24_LTEA definition
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

#include <vector> // std::vector<>

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

void usage(const char * program) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s <input_file>\n", program);
    fprintf(stderr, "\n");
    fprintf(stderr, "  <input_file>  => Path to file containing 7200 ASCII '1' and '0' values\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  This program will remove the Turbo coding and rate matching found in DJI DroneID signals\n");
    fprintf(stderr, "  and output the decoded frame in hex to the terminal\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  It has only been tested against the DJI Mini 2\n");
    fprintf(stderr, "\n\n");

    exit(1);
}

int main(int argc, const char ** argv) {
    const size_t input_file_bit_count = 7200;

    if (argc != 2) {
        usage(argv[0]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Open and validate input file
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FILE * input_handle = fopen(argv[1], "r");
    if (! input_handle) {
        fprintf(stderr, "[ERROR] Failed to open input file '%s'\n", argv[1]);
        usage(argv[0]);
    }
    
    // Get the number of bytes present in the file
    fseek(input_handle, 0, SEEK_END);
    const size_t byte_count = ftell(input_handle);
    fseek(input_handle, 0, SEEK_SET);
    
    // Make sure that the number of bytes in the file is correct
    if (byte_count != input_file_bit_count) {
        fprintf(stderr, "[ERROR] Invalid number of bytes in input file '%s'.  Expected %ld, got %ld\n", 
            argv[1], input_file_bit_count, byte_count);
        usage(argv[0]);
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read bits from input file
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<char> input_file_bits(input_file_bit_count);
    size_t bytes_read = fread(&input_file_bits[0], sizeof(char), input_file_bit_count, input_handle);

    if (bytes_read != input_file_bit_count) {
        fprintf(stderr, "[ERROR] Expected to read %ld bytes, but got %ld\n", input_file_bit_count, bytes_read);
        usage(argv[0]);
    }

    fclose(input_handle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Validate and convert bits from input file
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Load bearing constants.  Do not change!!
    int8_t bit_value_lut[2] = {-63, 63};

    std::vector<int8_t> turbo_decoder_input(input_file_bit_count);

    for (size_t idx = 0; idx < input_file_bit_count; idx++) {
        const int8_t value = (int8_t)input_file_bits[idx];
        
        // Don't let values other than 0 or 1 past as these will walk past the edge of the LUT
        if (value != 1 && value != 0) {
            fprintf(stderr, "Invalid bit value '%02x' at offset %ld.  Must be 0x1 or 0x0\n", value, idx);
            usage(argv[0]);
        }
        
        // Map the 1/0 value to the correct magic number from the lookup table
        turbo_decoder_input[idx] = bit_value_lut[value];
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Setup and run the Turbo decoder
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const int turbo_iterations = 4;
    const int turbo_decoder_bit_count = 1412;  // Number of bits that the Turbo decoder will take in
    const int expected_payload_bytes = 176;    // Number of bytes that the Turbo decoder will output
    const int expected_payload_bits = expected_payload_bytes * 8;
    
    // Allocate buffers for the Turbo decoder
    std::vector<int8_t> d1(turbo_decoder_bit_count);
    std::vector<int8_t> d2(turbo_decoder_bit_count);
    std::vector<int8_t> d3(turbo_decoder_bit_count);
    std::vector<uint8_t> decoded_bytes(expected_payload_bytes);
    
    // Create the required structures to run the Turbo decoder
    struct lte_rate_matcher * rate_matcher = lte_rate_matcher_alloc();
    struct tdecoder * turbo_decoder = alloc_tdec();
    struct lte_rate_matcher_io rate_matcher_io = {
        .D = turbo_decoder_bit_count,
        .E = input_file_bit_count,
        .d = {&d1[0], &d2[0], &d3[0]},
        .e = &turbo_decoder_input[0]
    };
    
    // Setup the rate matching logic
    lte_rate_match_rv(rate_matcher, &rate_matcher_io, 0);

    // Run the turbo decoder (will do rate matching as well)
    const int decode_status = lte_turbo_decode(turbo_decoder, expected_payload_bits, turbo_iterations,
        &decoded_bytes[0], &d1[0], &d2[0], &d3[0]);

    if (decode_status != 0) {
        fprintf(stderr, "[ERROR] Failed to remove Turbo coder.  Exit code %d\n", decode_status);
        usage(argv[0]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Validate the CRC24 at the end of the Turbo decoded data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const uint32_t calculated_crc = CRC::Calculate(&decoded_bytes[0], decoded_bytes.size() - 3, CRC::CRC_24_LTEA());
    const uint32_t expected_crc = (decoded_bytes[expected_payload_bytes-3] << 16) | 
                                  (decoded_bytes[expected_payload_bytes-2] << 8 ) |
                                  (decoded_bytes[expected_payload_bytes-1] << 0 );
    
    if (calculated_crc != expected_crc) {
        fprintf(stderr, "[ERROR] CRC did not match.  Got %06x, expected %06x\n", calculated_crc, expected_crc);
        return 1;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Print the decoded frame in hex
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const uint32_t payload_crc_byte_len = 2;     // There is a 16-bit CRC at the end of the payload
    const uint32_t payload_padding_byte_len = 1; // There is a single byte of 0x00 just before the 16-bit CRC
    const uint32_t payload_length = decoded_bytes[0] + payload_crc_byte_len + payload_padding_byte_len;
    
    // Print out the frame in hex
    for (uint32_t idx = 0; idx < payload_length; idx++) {
        fprintf(stdout, "%02x", decoded_bytes[idx]);
    }
    fprintf(stdout, "\n");
    
    return 0;
}
