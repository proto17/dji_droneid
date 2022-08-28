function [zc_seq] = create_zc_seq(fft_size, root)
    assert(log2(fft_size) == round(log2(fft_size)), "Invalid FFT size.  Must be power of 2");

    % Would use MATLAB's zadoffChuSeq function, but Octave doesn't have that
    % The logic below was tested against the MATLAB function
    zc_seq = exp(-1j * pi * root * (0:600) .* (1:601) / 601);
end

