% Write arbitrary type complex samples as interleaved I/Q values (Real(1),Imag(1),Real(2),Imag(2),...)
%
% Variable arguments are as follows:
%   - Append: Can be '1' or 'true' to append to an existing file, or '0' or 'false' to overwrite.  Defaults to overwrite
%
% @param output_path Path to output the samples to.  Must be string, char array, or cell string
% @param samples Vector (row or column) of samples to write
% @param sample_type Data type the samples should be written out as.  Example: 'single' for floats, 'int16' for shorts
% @param varargin Variable arguments (see above)
% @return Number of complex samples written

function [written] = write_complex(output_path, samples, sample_type, varargin)
    assert(isstring(output_path) || ischar(output_path) || iscellstr(output_path), ...
        'Output path must be a string, char array, or cell string');
    assert(~ isempty(output_path), 'Output path was empty');
    assert(~ isempty(samples), 'No samples provided');
    assert(iscolumn(samples) || isrow(samples), 'Input samples must be row or column vector');
    assert(isstring(sample_type) || ischar(sample_type) || iscellstr(sample_type), ...
        'Sample type must be string, char array, or cell string');
    assert(mod(length(varargin), 2) == 0, 'Variable argument length must be even.  Got %d arguments', length(varargin));

    % Default to overwriting the output file
    fopen_mode = 'w';

    % Parse varargs
    for idx=1:2:length(varargin)
        key = varargin{idx};
        value = varargin{idx+1};

        switch(key)
            case 'Append'
                if (value == 1 || value == true)
                    % When appending in MATLAB the mode must be 'a', not 'wa'
                    fopen_mode = 'a';
                elseif (value ~= 0 && value ~= false)
                    error('Invalid value "%s" for Append', mat2str(value));
                end
        end
    end
    
    % Open the output file and check the operation succeeded
    handle = fopen(output_path, fopen_mode);
    assert(handle ~= -1, 'Could not open output file "%s" for writing', output_path);

    % Attempt to write out the samples as interleaved I/Q
    written = fwrite(handle, reshape([real(samples); imag(samples)], 1, []), sample_type);

    % Warn the user if not all samples could be written
    if (written ~= length(samples) * 2)
        warning('Attempted to write %d samples to "%s" but only wrote %d', length(samples * 2), output_path, written);
    end

    fclose(handle);
end

