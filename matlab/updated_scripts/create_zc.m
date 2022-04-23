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
    
    zc = reshape(create_zc_seq(fft_size, root), [], 1);
    
    % Figure out how many guard carriers there should be (purposely ignoring DC here)
    guard_carriers = fft_size - 600;

    % The left side will end up with one more guard than the right
    % This may not be correct as it's a guess and the other way around failed to correlate
    % as well as this way.
    left_guards = (guard_carriers / 2);
    right_guards = (guard_carriers / 2) - 1;
    
    % Populate all of the FFT bins with the ZC sequence in the middle (including the DC carrier)
    samples_freq = [zeros(left_guards, 1); zc; zeros(right_guards, 1)];

    % Null out the DC carrier
    samples_freq(fft_size/2) = 0;

    samples_freq
    
    % Convert to time domain making sure to flip the spectrum left to right first
    samples = ifft(fftshift(samples_freq)) / fft_size;
end
