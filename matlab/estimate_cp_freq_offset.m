function [freq_offset_est] = estimate_cp_freq_offset(samples, fft_size, short_cp_len, long_cp_len)
    figure(44);
    scores = zeros(9, 1);
    skip = 8;
    sample_offset = 1;
    figure(10);
    for idx=1:9
        if (idx == 1 || idx == 9)
            cp_len = long_cp_len;
        else
            cp_len = short_cp_len;
        end

        cyclic_prefix = samples(sample_offset+skip:sample_offset + cp_len - 1 - skip);
        end_of_symbol = samples(sample_offset+fft_size+skip:sample_offset+fft_size+cp_len - 1 - skip);
        
        subplot(3, 3, idx);
        plot(abs(cyclic_prefix).^2);
        hold on;
        plot(abs(end_of_symbol).^2);
        hold off;
        scores(idx) = angle(sum(cyclic_prefix .* conj(end_of_symbol))) / length(cyclic_prefix);

        sample_offset = sample_offset + fft_size + cp_len;
    end

    freq_offset_est = sum(scores) / length(scores) / fft_size;
end