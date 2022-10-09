% Creates a ZC sequence that is mapped onto an OFDM symbol and FFT'd
%
% The results of this function can be used to cross correlate for the specified OFDM
% symbol (numbers 4 or 6).  There is no cyclic prefix added by this function!
%
% 601 samples are created by the ZC, but the middle sample is zeroed out.  This is done
% so that the DC carrier of the FFT is not populated
%
% @param fft_size Size of the OFDM FFT window (must be power of 2)
% @param symbol_index Which ZC sequence symbol should be created (must be 4 or 6)
%
function [samples] = create_zc(fft_size, symbol_index)
    % Validate inputs
    assert(symbol_index == 4 || symbol_index == 6, "Invalid symbol index (must be 4 or 6)");
    assert(log2(fft_size) == round(log2(fft_size)), "Invalid FFT size.  Must be power of 2");

    % Pick the correct root for the ZC sequence
    if (symbol_index == 4)
        root = 600;
    else
        root = 147;
    end

    root
    
    % Would use MATLAB's zadoffChuSeq function, but Octave doesn't have that
    % The logic below was tested against the MATLAB function
%     vals = pi * root * (0:600) .* (1:601) / 601;
%     zc = cos(vals) + (1j * sin(vals));

    zc = reshape(exp(-1j * pi * root * (0:600) .* (1:601) / 601), [], 1);
    
    % Remove the middle value (this would be DC in the FFT)
    zc(301) = [];

    % Create a buffer to hold the freq domain carriers
    samples_freq = zeros(fft_size, 1);

    % Get which FFT bins should be used for data carriers
    data_carrier_indices = get_data_carrier_indices(fft_size * 15e3);

    % Assign just the data carrier bins (left to right) the ZC sequence values
    samples_freq(data_carrier_indices) = zc;
    
    % Convert to time domain making sure to flip the spectrum left to right first
    samples = ifft(fftshift(samples_freq));
end
