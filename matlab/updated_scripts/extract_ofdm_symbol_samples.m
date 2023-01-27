% Given time domain samples, extracts out each of the OFDM symbols minus their cyclic prefixes for both time and
% frequency domains.  First sample must be the exact start of the burst!
%
% @param samples Time domain samples as a row/column vector
function [time_domain, freq_domain] = extract_ofdm_symbol_samples(samples, sample_rate)
    assert(isrow(samples) || iscolumn(samples), "Samples must be a row or column vector");

    fft_size = get_fft_size(sample_rate);
    [long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(sample_rate);
    
    % List of cyclic prefix lengths for each OFDM symbol
    cp_lengths = [
        long_cp_len,...
        short_cp_len,...
        short_cp_len,...
        short_cp_len,...
        short_cp_len,...
        short_cp_len,...
        short_cp_len,...
        short_cp_len,...
        long_cp_len...
    ];

    freq_domain = zeros(length(cp_lengths), fft_size);
    time_domain = zeros(length(cp_lengths), fft_size);
    
    sample_offset = 1;
    for idx=1:length(cp_lengths)
        % Skip the cyclic prefix
        symbol = samples(sample_offset:sample_offset + fft_size + cp_lengths(idx) - 1);
        symbol = symbol(cp_lengths(idx) + 1:end);

        % Extract the time domain samples for this OFDM symbol
        time_domain(idx,:) = symbol;

        % Convert the time domain samples into frequency domain
        freq_domain(idx,:) = fftshift(fft(time_domain(idx,:)));
        
        sample_offset = sample_offset + fft_size + cp_lengths(idx);
    end
end

