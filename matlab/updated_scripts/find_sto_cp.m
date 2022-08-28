%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Find the start time offset based on the cyclic prefix
%
% This method is immune to the issues that plague the ZC sequences (frequency offset causes a time shift in the
% correlation results)
%
% It's best to provide this function an upsampled copy of the burst to help fix any fractional time offset that might be
% present
%
% @param samples Complex IQ samples that make up the full burst
% @param sample_rate Sample rate (in Hz) of the provided samples
% @return start_offset Sample index that the burst starts at (first sample of the first cyclic prefix)
function [start_offset] = find_sto_cp(samples, sample_rate)
    [long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(sample_rate);
    fft_size = get_fft_size(sample_rate);
    cyclic_prefix_length_schedule = [...
        long_cp_len, ...
        short_cp_len, ...
        short_cp_len, ...
        short_cp_len, ...
        short_cp_len, ...
        short_cp_len, ...
        short_cp_len, ...
        short_cp_len, ...
        long_cp_len];
    num_ofdm_symbols = length(cyclic_prefix_length_schedule);

    full_burst_len = sum(cyclic_prefix_length_schedule) + (fft_size * num_ofdm_symbols);
    num_tests = length(samples) - full_burst_len;
    scores_cp_sto = zeros(1, num_tests);

    for idx=1:num_tests
        offset = idx;
        scores = zeros(1, num_ofdm_symbols);
    
        % Extract and correlate the samples that each cyclic prefix is expected
        % to be at
        for cp_idx=1:num_ofdm_symbols
            cp_len = cyclic_prefix_length_schedule(cp_idx);
    
            % Extract the full OFDM symbol including cyclic prefix
            window = samples(offset:offset + fft_size + cp_len - 1);
    
            % Extract the cyclic prefix and the final samples of the symbol
            left = window(1:cp_len);
            right = window(end - cp_len + 1:end);
    
            % Correlate the two windows
            scores(cp_idx) = abs(xcorr(left, right, 0));
    
            % Move the sample pointer forward by the full symbol size
            offset = offset + cp_len + fft_size;
        end
    
        % In the real DroneID the first OFDM symbol needs to be ignored since
        % it isn't always present.  So, just average the correlation scores of
        % all but the first element
        scores_cp_sto(idx) = sum(scores(2:end)) / (length(scores) - 1);

    end

    % Find the index of the highest score
    [~, start_offset] = max(scores_cp_sto);
end

