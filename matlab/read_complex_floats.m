% Reads in a window of complex samples from the provided file
%
% @param file_path Path to file containing 32-bit floating point interleaved complex samples (I,Q,I,Q,...)
% @param sample_offset How many samples into the file to skip before reading in samples
% @param sample_count How many samples to extract after the `sample_offset`
% @return samples Column vector of `sample_count` complex samples (might be less if not enough samples are avilable
%                 in the provided file
function [samples] = read_complex_floats(file_path, sample_offset, sample_count)
    handle = fopen(file_path, 'r');
    if (handle == 0)
        error("Failed to open input file '%s'", file_path);
    end
    
    % Seek to the end of the file, get the byte offset (number of bytes in the file), and calculate the
    % total number of complex samples in the file
    fseek(handle, 0, "eof");
    byte_count = ftell(handle);
    complex_sample_count = byte_count / 8; % Each I/Q value is 32-bit (4 bytes), so there are 8 bytes per complex value
    
    if (sample_offset > complex_sample_count)
        error("Sample offset %d goes past the end of the file", sample_offset)
    end
    
    % Skip `sample_offset` samples from the beginning of the file
    fseek(handle, 4 * sample_offset * 2, "bof");

    % Read in `sample_count * 2` 32-bit floats since we need two floats to make a single complex value
    reals = fread(handle, sample_count * 2, 'single');
    fclose(handle);

    % Convert the interleaved samples into a column vector of complex values
    samples = reshape(reals(1:2:end) + 1j * reals(2:2:end), 1, []);
end

