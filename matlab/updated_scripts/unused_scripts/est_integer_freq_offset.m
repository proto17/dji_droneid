% Estimates the integer frequency offset using the time domain samples of
% the first ZC sequence (root index 600)
%
% @param received_zc_td Time domain of the received ZC sequence with no
%                       cyclic prefix (should be row vector)
% @param sample_rate Sample rate of the samples in Hz
% @param max_carrier_offset How much integer frequency offset to search for
%                           (must be positive non-zero value)
% @return integer_offset_hz Best estimate for the integer frequency offset
%                           in Hz
function [integer_offset_hz] = est_integer_freq_offset(received_zc_td, sample_rate, max_carrier_offset)
    fft_size = get_fft_size(sample_rate);
    
    assert(isrow(received_zc_td), "ZC time domain samples must be row vector");
    assert(length(received_zc_td) == fft_size, "ZC time domain samples must be FFT samples wide");
    assert(isnumeric(sample_rate) && sample_rate > 0, "Sample rate must be positive and non-zero");
    assert(isnumeric(max_carrier_offset) && max_carrier_offset > 0, "Max carrier offset must be positive and non-zero");
    
    % Calculate all of the cyclic shifts that will be done to search for
    % the integer frequency offset value
    shift_range = -max_carrier_offset:1:max_carrier_offset;
    
    % Allocate storage for the correlation scores from each shift
    scores = zeros(1, length(shift_range));
    
    % Move the ZC sequence to the frequency domain
    zc_fd = fft(received_zc_td);

    for idx=1:length(shift_range)
        % Circularly shift the FFT
        zc_fd_shifted = circshift(zc_fd, shift_range(idx));
        
        % Take the upper and lower `(fft_size / 2) - 1` samples 
        left = zc_fd_shifted(1:(fft_size / 2) - 1);
        right = zc_fd_shifted((fft_size / 2) + 2:end);
        
        % Correlate the lower samples with the mirror of the upper samples
        scores(idx) = xcorr(left, fliplr(right), 0);
    end
    
    % Find the index of the max score
    [~, index] = max(abs(scores));
    
    % Calculate the number of Hz between each FFT bin
    carrier_spacing = sample_rate / fft_size;
    
    % Get how many carriers the best score was shifted by and multiply that
    % value by the carrier spacing to get the integer frequency offset
    integer_offset_hz = shift_range(index) * carrier_spacing;
end

