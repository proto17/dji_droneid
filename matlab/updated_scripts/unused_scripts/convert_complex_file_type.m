% Convert the samples in the input file to a different format (ex: float complex to int16 complex)
%
% The input and output types need to be string names like 'single', 'int16', or 'int8'.
% To convert a file containing complex 32-bit floats to complex 16-bit ints for a radio with a 12-bit DAC:
%     convert_complex_file_type('/tmp/foo.fc32', 'single', '/tmp/foo.sc16', 'int16', 2^12);
%
% @param input_file_path Path to the input file containing complex samples of `input_type`
% @param input_type Sample type of each I and Q value in `input_file_path`
% @param output_file_path Path to the new file containing samples from the input file converted to `output_type`
% @param output_type New sample type of each I and Q value (converted from `input_type`)
% @param scalar Value to scale the input samples by before writing to disk
% @return sample_count Number of complex samples written to `output_file_path`
function [sample_count] = convert_complex_file_type(input_file_path, input_type, output_file_path, output_type, scalar)
    assert(isstring(input_file_path) || ischar(input_file_path) || iscellstr(input_file_path), ...
        'Input file path must be a string, char array, or cell string');
    assert(isstring(input_type) || ischar(input_type), 'Input type must be a string or char array');
    assert(isstring(output_file_path) || ischar(output_file_path) || iscellstr(output_file_path), ...
        'Output file path must be a string, char array, or cell string');
    assert(isstring(output_file_path) || ischar(output_file_path), 'Output type must be a string or char array');
    assert(isnumeric(scalar), 'Scalar must be a number');

    input_file_handle = fopen(input_file_path, 'r');
    assert(input_file_handle ~= -1, 'Could not open input file "%s"', input_file_path);

    output_file_handle = fopen(output_file_path, 'w');
    assert(output_file_handle ~= -1, 'Could not open output file "%s" for writing', output_file_path);

    input_file_sample_bytes = get_bytes_per_sample(input_type);

    % How many samples to work on at a time.  Keep this value as high as possible to reduce runtime.  But, don't go
    % crazy with it as it will plow through RAM
    chunk_size = 1e6;
    
    % Track the number of samples written
    sample_count = 0;
    
    % Read until there are no samples left in the input file
    while (~ feof(input_file_handle))
        % Read up to one chunk of samples (could be less if there aren't enough samples left in the file)
        input_samples = fread(input_file_handle, chunk_size * input_file_sample_bytes, input_type);
        
        % Scale and cast the input samples to the output
        output_samples = cast(input_samples * scalar, output_type);

        % Write the scaled samples to disk and update how many samples have been written
        sample_count = sample_count + fwrite(output_file_handle, output_samples, output_type);
    end
    
    % Convert to complex sample count
    sample_count = sample_count / 2;
end

