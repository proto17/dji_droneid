% Get the FFT size based on the sampling rate
%
% @param sample_rate Sample rate (in Hz)
% @return fft_size FFT size required for the provided sample rate
function [fft_size] = get_fft_size(sample_rate)
    % DroneID's carrier spacing is the same as LTE
    fft_size = sample_rate / 15e3;
end

