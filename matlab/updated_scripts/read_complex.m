% Read in complex IQ samples from the specified file
%
% @param input_file Path to input file.  Must be string, char array, or cell string
% @param sample_offset How many complex samples to skip from the beginning of the file
% @param sample_count Number of samples after `sample_offset` to extract from the file.  Must be -1 or inf to read all
%                     samples after `sample_offset` or > 0 to limit the number of samples read
% @param sample_type Data type of each real/imaginary value.  Example: 'single' for 32-bit float, 'int16' for 16-bit
%                    shorts
% @return samples Samples read from the input file as a column vector of complex values
function [samples] = read_complex(input_file, sample_offset, sample_count, sample_type)
    assert(isstring(input_file) || ischar(input_file) || iscellstr(input_file), ...
        'Input file must be a string, char array, or cell string');
    assert(isnumeric(sample_offset), 'Sample offset must be a number')
    assert(sample_offset >= 0, 'Sample offset must be >= 0')
    assert(isnumeric(sample_count) || isinf(sample_count), 'Sample count must be a number or `inf`');
    assert(sample_count == -1 || sample_count > 0 || sample_count == inf, 'Sample count must be -1, inf, or > 0');
    assert(isstring(sample_type) || ischar(sample_type) || iscellstr(sample_type), ...
        'Sample type must be a string, char array, or cell string');

    % Open the sample file and verify that was successful
    file_handle = fopen(input_file, 'r');
    assert(file_handle ~= -1, "Could not open input file '%s'", input_file);
    
    try
        % Create a value using the sample type
        sample_type_example = cast(1, sample_type);

        % Get information about the created variable
        sample_type_info = whos('sample_type_example');

        % Get the number of bytes in sample_type from the type info
        bytes_per_real_sample = sample_type_info.bytes;
    catch
        error('Failed to determine byte size of type "%s"', sample_type);
    end

    % Get the total number of samples (reals and complex) in the file
    fseek(file_handle, 0, 'eof');
    total_real_samples = floor(ftell(file_handle) / bytes_per_real_sample);
    total_complex_samples = total_real_samples / 2;

    % Sanity check that the sample offset requested actually exists
    assert(sample_offset < total_complex_samples, ...
        "Asked for sample offset of %d, but there are only %d samples", sample_offset, total_complex_samples);

    % Move the file handle to the starting sample offset (each real is 2 bytes, and a complex value is 4)
    fseek(file_handle, sample_offset * (bytes_per_real_sample * 2), 'bof');

    % If the user supplied -1 or inf update the sample count to include all remaining samples in the file
    if (sample_count == -1 || sample_count == inf)
        sample_count = total_complex_samples - sample_offset;
    end

    % Warn the user when asking for more samples than are available
    if (sample_count + sample_offset > total_complex_samples)
        warning('Attempting to read %d samples with an offset of %d when only %d total samples are available', ...
            sample_count, sample_offset, total_complex_samples);
    end

    % Read in samples from the file.  These come in as individual int16 values, not complex.  Using an explicit cast
    % here because MATLAB seems to treat everything as double precision floats unless told not to
    real_samples = cast(fread(file_handle, sample_count * 2, sample_type), sample_type);
    fclose(file_handle);
    
    % Convert the interleaved samples into a column vector of complex values
    samples = complex(real_samples(1:2:end), real_samples(2:2:end));
end

