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

    % In the DJI Mini 2 there are 9 OFDM symbols: 2 long cyclic prefixes, 7 short.  This isn't the case on all drones.
    % For some drones there are just 8 OFDM symbols.  It looks like those drones just don't send the first OFDM symbol
    % that's present on the Mini 2.  That symbol XOR's out to all zeros anyway, so it's not important.  So, to keep
    % things consistent, the logic below will always extract out 9 OFDM symbols worth of samples.  In later steps the
    % first OFDM symbol isn't used for anything.
    burst_sample_count = (padding * 2) + (long_cp_len * 2) + (short_cp_len * 7) + (fft_size * 9);

    freq_offset_vec = reshape(exp(freq_offset_constant * [1:burst_sample_count]), 1, []);

    % It's not known right away if the first and last bursts are going to be clipped because there aren't enough
    % samples.  So, as filthy as it is, use concatenation to build up a list of starting indices that will definitely
    % have all samples present in the input file
    valid_burst_indices = [];

    for idx=1:length(indices)
        start_index = indices(idx);
        
        % Calculate when the burst will start and end
        actual_start_index = start_index - padding - zc_seq_offset;
        actual_end_index = actual_start_index + burst_sample_count;
        
        % Ensure that all samples related to this burst are present in the recording
        if (actual_start_index < 1)
            warning("Skipping burst at offset %d as the beginning of the burst has been clipped", start_index);
            continue
        end
        
        if (actual_end_index > num_samples)
            warning("Skipping burst at offset %d as the ending of the burst will be clipped", start_index);
            continue
        end
        
        % Again, concatenation is filthy, but necessary here since the actual number of bursts is unknown
        valid_burst_indices = [valid_burst_indices actual_start_index];
    end

    % Now that the true number of bursts is known, create a buffer to hold everything
    bursts = zeros(length(valid_burst_indices), burst_sample_count);

    for idx=1:length(valid_burst_indices)
        % Read in the current burst.  The starting index was calculated above
        burst = read_complex_floats(input_path, valid_burst_indices(idx), burst_sample_count);
        
        % Adjust for the user-specified frequency offset that is present in the recording and save those samples off
        bursts(idx,:) = burst .* freq_offset_vec;
    end
    
end

