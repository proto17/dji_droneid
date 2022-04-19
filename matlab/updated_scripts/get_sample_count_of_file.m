% Get the number of complex 32-bit floating point values in the specified file
%
% @param file_path Path to the file that contains complex 32-bit floating point samples
% @return sample_count Number of complex 32-bit floating point samples in the provided input file
function [sample_count] = get_sample_count_of_file(file_path)
    handle = fopen(file_path, "r");
    if (handle == 0)
        error("Could not open input file '%s'"', file_path);
    end

    fseek(handle, 0, 'eof');
    byte_count = ftell(handle);
    fclose(handle);

    sample_count = floor(byte_count / 4 / 2);
end