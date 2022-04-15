% Get the indices from the FFT that contain data carriers (not guards or DC)
%
% @param sample_rate Sampling rate (in Hz) that the mapping should be generated for
% @return indices Column vector of 600 integers where each integer represents the index that should be extracted from a
%                 call to `fftshift(fft(samples))`
function [indices] = get_data_carrier_indices(sample_rate)
    fft_size = get_fft_size(sample_rate);
    
    % DroneID uses 600 carriers
    data_carrier_count = 600;

    % The left side has one more guard carrier than the right
    left_guards = ((fft_size - data_carrier_count) / 2);
    right_guards = left_guards - 1;

    % Create an initial mapping of all 1's, then set the guards to 0, and finally the DC carrier to 0
    mapping = ones(fft_size, 1);
    mapping(1:left_guards) = 0;
    mapping(end-right_guards+1:end) = 0;
    mapping((fft_size/2)) = 0;

    % Get the indices of `mapping` that contain a 1 value (should be all of the data carriers)
    indices = find(mapping == 1);
end

