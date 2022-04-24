% Reads in a window of complex samples from the provided file
%
% @param file_path Path to file containing 32-bit floating point interleaved complex samples (I,Q,I,Q,...)
% @param sample_offset How many samples into the file to skip before reading in samples
% @param sample_count How many samples to extract after the `sample_offset`
% @return samples Column vector of `sample_count` complex samples (might be less if not enough samples are avilable
%                 in the provided file
function [samples] = read_complex_floats(file_path, sample_offset, sample_count)
    samples = read_complex(file_path, sample_offset, sample_count, 'single');
end
