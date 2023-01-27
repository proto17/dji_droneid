% Get the number of complex values in the specified file using the provided sample type
%
% @param file_path Path to the file that contains complex 32-bit floating point samples
% @param sample_type MATLAB numeric class type for each I and Q value in the file.  Ex: 'single', 'int16', 'int8', etc
% @return sample_count Number of complex 32-bit floating point samples in the provided input file
function [sample_count] = get_sample_count_of_file(file_path, sample_type)
    handle = fopen(file_path, "r");
    if (handle == 0)
        error("Could not open input file '%s'"', file_path);
    end
    
    % Get how many bytes are required to make a single I or Q value
    bytes_per_sample = get_bytes_per_sample(sample_type);
    
    % Move to the end of the file
    fseek(handle, 0, 'eof');

    % Ask how many bytes into the file the pointer is currently
    byte_count = ftell(handle);
    fclose(handle);

    % Get the number of complex samples in the file.  Flooring to ensure that if there are bytes missing those are not
    % counted towards the total number of samples
    sample_count = floor(byte_count / bytes_per_sample / 2);
end