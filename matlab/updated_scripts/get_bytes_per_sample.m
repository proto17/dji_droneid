% Given a sample type (eg. 'single', 'int16', 'int8', etc) get how many bytes each I and Q value takes up
%
% @param sample_type MATLAB data type (ec. 'single', 'double', 'int16', etc) as a char array
% @return bytes Number of bytes required to represent the specified type
function [bytes] = get_bytes_per_sample(sample_type)
    assert(ischar(sample_type) || isstring(sample_type), "Sample type must be a string or char array");

    % Create an instance of the requested type
    try
        sample_type_ex = cast(1, sample_type);
    catch
        error('Failed to create value for type "%s"', sample_type);
    end

    % Get information about the variable created above
    sample_type_info = whos('sample_type_ex');

    bytes = sample_type_info.bytes;
end

