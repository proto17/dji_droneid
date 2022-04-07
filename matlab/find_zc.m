% Exploits the fact that the first ZC sequence is symmetrical to find where
% it starts.
%
% The basic idea is to read in FFT size blocks at each sample offset, split
% the block in half, reverse the second half, and run a normalized cross
% correlation.  The reversed version of the second half should exactly
% match the first half.  And thanks to the CAZAC (constant amplitude, zero
% autocorrelation) feature of the ZC sequence, there should be one very
% large peak
% The returned scores are the result of each cross correlation at each
% offset and are complex values.  To get the normalized score, take the
% absolute value squared.  
% 
% NOTE: The offset with the highest value is the start of the ZC sequence, 
% and NOT the start of the cyclic prefix for that OFDM symbol!!!!
function [scores] = find_zc(samples, sample_rate)
    fft_size = sample_rate / 15e3;
    short_cp_len = round(0.00000469 * sample_rate);
    
    % Buffer to store the xcorr scores.  Since a full `fft_size` number of
    % samples is needed at each start index, don't seek all the way to the
    % last sample
    scores = zeros((length(samples) - fft_size - short_cp_len), 1);
    
    % Walk through each start offset
    for start_offset=1:length(scores)
        % Skip in by one short cyclic prefix and extract `fft_size` samples
        window = samples(start_offset+short_cp_len:start_offset+short_cp_len+fft_size-1);

        % The first window is just the first half of the OFDM symbol
        window_one = window(1:(fft_size/2));
        % The second window is the second half of the OFDM symbol
        % *reversed*
        window_two = fliplr(window((fft_size/2) + 1:end));
        
        % Run a normalized cross correlation with no lag (just do one
        % xcorr)
        scores(start_offset) = normalized_xcorr(window_one, window_two);
    end
end
