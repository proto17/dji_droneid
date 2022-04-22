function [freq_offset_est] = estimate_cp_freq_offset(samples, fft_size, short_cp_len, long_cp_len)
    
    scores = zeros(9, 1);
    sample_offset = 1;
    figure(10);
    for idx=1:9
        if (idx == 1 || idx == 9)
            cp_len = long_cp_len;
        else
            cp_len = short_cp_len;
        end
        
        symbol = samples(sample_offset:sample_offset + cp_len + fft_size - 1);

        cyclic_prefix = symbol(1:cp_len);
        end_of_symbol = symbol(end-cp_len+1:end);

%         cyclic_prefix = samples(sample_offset+skip:sample_offset + cp_len - 1 - skip);
%         end_of_symbol = samples(sample_offset+fft_size+skip:sample_offset+fft_size+cp_len - 1 - skip);
        
        subplot(3, 3, idx);
        plot(abs(cyclic_prefix).^2);
        hold on;
        plot(abs(end_of_symbol).^2);
        hold off;

        
        scores(idx) = angle(sum(cyclic_prefix .* conj(end_of_symbol))) / (2 * pi * cp_len);
%         sum(angle(cyclic_prefix .* conj(end_of_symbol))) / length(fft_size) * 15.36e6
        %scores(idx) = sum(angle(cyclic_prefix .* conj(end_of_symbol))) / length(cyclic_prefix);

        sample_offset = sample_offset + fft_size + cp_len;
    end
    
    freq_offset_est = sum(scores) / 9;
    scores * 15.36e6
    %freq_offset_est = sum(scores) / length(scores) / fft_size;
end