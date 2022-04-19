% Searches through the specified file for the first ZC sequence, and extracts the full bursts (in time)
%
% It's very important that the `frequency_offset` be correct such that when applied, the signal is centered at DC (0 Hz)
% Otherwise the start sample estimate from correlating for the ZC sequence will be off in time as well as the
% correlation score being lower
%
% @param input_path File containing complex 32-bit floating point samples (interleaved I,Q,I,Q,...)
% @param sample_rate Sample rate that the file was recorded at.  Must be an integer multiple of 15.36 MSPS (the minimum 
%                    sample rate for the DroneID downlink)
% @param frequency_offset How far off from DC the signal is in the recording (set to 0 for no frequency adjustment)
% @param correlation_threshold Score on a scale from 0.0 to 1.0 where 1.0 is a perfect match with the ZC sequence.  This
%                              will determine how closely the recorded ZC sequence must match in order to be extracted
%                              as a burst.  Usually anywhere from 0.2 to 0.9 are usable values.
% @param chunk_size How many samples to process at one time.  This depends on how much RAM your system has.  This value
%                   should likely be set > 1e6 but < 20e6.  But you do you.
% @param padding How many additional samples before and after the burst to extract.  Must be >= 0
% @return bursts A matrix where each row contains one burst
function [bursts] = extract_bursts_from_file(input_path, sample_rate, frequency_offset, correlation_threshold,...
    chunk_size, padding)

    num_samples = get_sample_count_of_file(input_path);
    
    lte_carrier_spacing = 15e3;                       % OFDM carrier spacing
    fft_size = sample_rate / lte_carrier_spacing;     % Number of samples per OFDM symbol (minus cyclic prefix)
    long_cp_len = round(1/192000 * sample_rate);      % Number of samples in the long cyclic prefix
    short_cp_len = round(0.0000046875 * sample_rate); % Number of samples in the short cyclic prefix

    freq_offset_constant = 1j * pi * 2 * (frequency_offset / sample_rate);

    % The first ZC sequence is the 4th symbol, and the `find_zc_indices_by_file` function will (assuming no major
    % frequency offset) return the sample index of the first sample of the 5th OFDM symbol cyclic prefix.  So, back the
    % index off by the number of samples in the first 4 OFDM symbols and their cyclic prefixes
    zc_seq_offset = (fft_size * 4) + long_cp_len + (short_cp_len * 3);
    
    % Find all instances of the first ZC sequence
    indices = find_zc_indices_by_file(input_path, sample_rate, frequency_offset, correlation_threshold, chunk_size);

    % Number of samples that need to be extracted.  There are 9 OFDM symbols, 2 long and 7 short cyclic prefixes
    burst_sample_count = (padding * 2) + (long_cp_len * 2) + (short_cp_len * 7) + (fft_size * 9);

    freq_offset_vec = reshape(exp(freq_offset_constant * [1:burst_sample_count]), 1, []);

    bursts = zeros(length(indices), burst_sample_count); % Allocate storage for the bursts and padding for each

    for idx=1:length(indices)
        start_index = indices(idx);
        
        % Back the start index off by the padding and take `burst_sample_count` samples
        burst = read_complex_floats(input_path, start_index - padding - zc_seq_offset, burst_sample_count);
        
        bursts(idx,:) = burst .* freq_offset_vec;
    end
end

