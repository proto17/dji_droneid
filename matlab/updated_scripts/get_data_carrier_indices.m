% Get the indices from the FFT that contain data carriers (not guards or DC)
%
% @param sample_rate Sampling rate (in Hz) that the mapping should be generated for
% @return indices Column vector of 600 integers where each integer represents the index that should be extracted from a
%                 call to `fftshift(fft(samples))`
function [indices] = get_data_carrier_indices(sample_rate)
    fft_size = get_fft_size(sample_rate);
    
    % DroneID uses 600 carriers
    data_carrier_count = 600;
    
    % Define the location of the DC carrier (which is not used as a data carrier)
    dc = (fft_size / 2) + 1;

    % Create an initial mapping of all 0's, then set then set all of the data carrier indices to 1
    mapping = zeros(fft_size, 1);

    % As an example: With an FFT size of 2048, 1025 would be DC, 725-1024 and 1026-1325 would be data carriers, and the
    % rest would be guards
    mapping(dc - (data_carrier_count / 2):dc - 1) = ones(data_carrier_count / 2, 1);
    mapping(dc + 1:dc + (data_carrier_count / 2)) = ones(data_carrier_count / 2, 1);

    % Get the indices of `mapping` that contain a 1 value (should be all of the data carriers)
    indices = find(mapping == 1);
end

