function [] = create_gnuradio_zc(fft_size, symbol_number)
    zc = conj(create_zc(fft_size, symbol_number));

    fprintf('[');
    for idx=1:length(zc)
        fprintf('(%0.4f+%0.4fj)', real(zc(idx)), imag(zc(idx)));
        if (idx ~= length(zc))
            fprintf(',')
        end
    end
    fprintf(']\n');
end

