function [samples] = read_complex_floats(file_path, sample_offset, sample_count)
%READ_COMPLEX_FLOATS Summary of this function goes here
%   Detailed explanation goes here
    handle = fopen(file_path, 'r');
    fseek(handle, 0, "eof");
    byte_count = ftell(handle);
    complex_sample_count = byte_count / 8;

    fseek(handle, 4 * sample_offset * 2, "bof");
    reals = fread(handle, sample_count * 2, 'single');
    fclose(handle);

    samples = reshape(reals(1:2:end) + 1j * reals(2:2:end), 1, []);

end

