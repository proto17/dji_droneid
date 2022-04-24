% This script takes in a floating point complex IQ recording containing DroneID bursts and demodulates each burst
% The steps are:
%   - Find all bursts in the file using the first ZC sequence
%   - Low pass filter each burst
%   - Adjust for frequency offset based on the offset found using the first OFDM symbol's cyclic prefix
%   - Extract each OFDM symbol
%   - Quantize/Demodulate all data carriers
%   - Validate that the first symbol XOR's to all zeros
%   - Pass XOR'd bits from all other data symbols to a C++ program that removes the LTE and rate matching
%   - Print out each frame in hex

%% Path Info
if (is_octave)
  this_script_path = fileparts(mfilename('fullpath'));
else
  this_script_path = fileparts(matlab.desktop.editor.getActiveFilename);
end

% Create a directory to store the constellation plots for debugging
% THIS CAN BE COMMENTED OUT IF NEEDED!!!  JUST MAKE SURE TO COMMENT OUT THE `saveas` CALL LATER AS WELL
mkdir(fullfile(this_script_path, "images"));

turbo_decoder_path = fullfile(this_script_path, filesep, '..', filesep, '..', filesep, 'cpp', filesep, 'remove_turbo');
if (~ isfile(turbo_decoder_path))
    error("Could not find Turbo decoder application at '%s'.  Check that the program has been compiled",...
        turbo_decoder_path);
end

%% File Parameters
enable_plots = 0; % Set to 0 to prevent the plots from popping up

file_path = '/opt/dji/collects/2437MHz_30.72MSPS.fc32';
file_sample_rate = 30.72e6;
file_freq_offset = 7.5e6; % This file was not recorded with the DroneID signal centered

correlation_threshold = 0.7; % The SNR is pretty good, so using a high correlation score (must be between 0.0 and 1.0)
chunk_size = 10e6;           % Number of samples to process at a time

%% Low Pass Filter Setup
signal_bandwidth = 10e6; % The actual occupied bandwidth of the DroneID signal
filter_tap_count = 50;   % Number of filter taps to use for the low pass filter
filter_taps = fir1(filter_tap_count, signal_bandwidth/file_sample_rate); % Create the low pass filter taps

%% Burst Extraction
[long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(file_sample_rate);
fft_size = get_fft_size(file_sample_rate);

% Making sure that the bursts that are extracted have enough padding for the low pass filter to start up and terminate
bursts = extract_bursts_from_file(file_path, file_sample_rate, file_freq_offset, correlation_threshold, chunk_size,...
    filter_tap_count);

assert(~isempty(bursts), "Did not find any bursts");

frames = {};

% Get a list of the indices from the shifted FFT outputs that contain data carriers
data_carrier_indices = get_data_carrier_indices(file_sample_rate);

% Initial value for the second LFSR in the scrambler
scrambler_x2_init = fliplr([0 0 1, 0 0 1 0, 0 0 1 1, 0 1 0 0, 0 1 0 1, 0 1 1 0, 0 1 1 1, 1 0 0 0]);

% This determines which OFDM symbol's cyclic prefix is used to determine the coarse frequency offset.  Some drones use 9
% OFDM symbols, and some use 8.  It seems that those drones that use 8 OFDM symbols have a short cyclic prefix in the
% first symbol.  Skipping the first symbol for those drones that have 9 OFDM symbols results in the new "first" symbol
% having a short cyclic prefix as well.  So, since the burst extractor always assumes that there are 9 symbols, the
% first symbol is skipped for the purposes of coarse CFO.  The second symbol is assumed to have a short cyclic prefix
coarse_cfo_symbol_sample_offset = fft_size + long_cp_len + 1;
coarse_cfo_symbol_cyclic_prefix = short_cp_len;

%% Burst Processing
for burst_idx=1:size(bursts, 1)
    % Get the next burst
    burst = bursts(burst_idx,:);

    %% Apply low pass filter
    burst = filter(filter_taps, 1, burst);

    % Remove the extra samples at the front.  
    % TODO(15April2022) Honestly not sure why this needs to be 1.5, but it does...
    offset = filter_tap_count * 1.5;
    burst = burst(offset-1:end);

    %% Coarse frequency offset adjustment using one of the OFDM symbols (see coarse_cfo_symbol_sample_offset definition)
    % Get the cyclic prefix, and then the copy of the cyclic prefix that exists at the end of the OFDM symbol
    cp = burst(...
        coarse_cfo_symbol_sample_offset:...
        coarse_cfo_symbol_sample_offset + coarse_cfo_symbol_cyclic_prefix - 1);

    copy = burst(...
        coarse_cfo_symbol_sample_offset + fft_size:...
        coarse_cfo_symbol_sample_offset + fft_size + coarse_cfo_symbol_cyclic_prefix - 1);
    
    % Calculate the frequency offset by taking the dot product of the two copies of the cyclic prefix and dividing out
    % the number of samples in between each cyclic prefix sample (the FFT size)
    offset_radians = angle(dot(cp, copy)) / fft_size;
    
    % Apply the inverse of the estimated frequency offset back to the signal
    burst = burst .* exp(1j * -offset_radians * [1:length(burst)]);
    
    %% OFDM Symbol Processing

    % Extract the individual OFDM symbols without the cyclic prefix for both time and frequency domains
    [time_domain_symbols, freq_domain_symbols] = extract_ofdm_symbol_samples(burst, file_sample_rate);
    
    % Calculate the channel for both of the ZC sequnces
    channel1 = calculate_channel(freq_domain_symbols(4,:), file_sample_rate, 4);
    channel2 = calculate_channel(freq_domain_symbols(6,:), file_sample_rate, 6);

    % Only select the data carriers from each channel estimate
    channel1 = channel1(data_carrier_indices);
    channel2 = channel2(data_carrier_indices);
    
    % Calculate the average phase offset of each channel estimate
    channel1_phase = sum(angle(channel1)) / length(data_carrier_indices);
    channel2_phase = sum(angle(channel2)) / length(data_carrier_indices);
    
    % This doesn't seem right, but taking the difference of the two channels and dividing by two yields the average
    % walking phase offset between the two.  That value can be used to correct for the phase offsets caused by not being
    % exactly spot on with the true first sample
    channel_phase_adj = (channel1_phase - channel2_phase) / 2;

    if (enable_plots)
        figure(441);
        subplot(2, 1, 1);
        plot(abs(channel1).^2, '-');
        subplot(2, 1, 2);
        plot(abs(channel2).^2, '-');
    end

    % Only use the fisrt ZC sequence to do the initial equaliztion.  Trying to use the average of both ends up with
    % strange outliers in the constellation plot
    channel = channel1;

    % Place to store the demodulated bits
    bits = zeros(9, 1200);

    % Walk through each OFDM symbol and extract the data carriers and demodulate the QPSK inside
    % This is done for symbols 4 and 6 even though they contain ZC sequences.  It's just to keep the logic clean
    
    for idx=1:size(bits, 1)
        % Equalize just the data carriers
        data_carriers = freq_domain_symbols(idx,data_carrier_indices) .* channel;

        % Adjust for the walking phase offset that will be present if the first time domain sample wasn't sampled at
        % just the right moment (fractional time offset).  If there is any fractional time offset then in the freq
        % domain there will be a phase offset that accumulates at each FFT bin.  This causes a smearing that can be
        % fixed by the channel estimation, but because there are no pilots the absolute phase is only correct for the
        % OFDM symbols next to the symbol used for equalization.  So, the absolute phase offset caused by the fractional
        % time offset is adjusted by multiplying the phase offset by how far each OFDM symbol is from the one that was
        % used to do equalization.  Using symbol 5 because it's in the middle of the two ZC sequences, and so whatever
        % phase offset was calculated between the two ZC's applies directly to OFDM symbol 5.
        data_carriers = data_carriers .* exp(1j * (-channel_phase_adj * (idx - 5)));

        % Demodulate/quantize the QPSK to bits
        bits(idx,:) = quantize_qpsk(data_carriers);
        
        if (enable_plots)
            figure(1);
            subplot(3, 3, idx);
            plot(data_carriers, 'o');
            ylim([-1, 1]);
            xlim([-1, 1]);
    
            figure(111);
            subplot(3, 3, idx);
            plot(10 * log10(abs(time_domain_symbols(idx,:)).^2), '-');
        end
    end
    
    % Save the constellation plots to disk for debugging
    % THIS CAN BE COMMENTED OUT IF NEEDED
    saveas(gcf, sprintf('%s/images/ofdm_symbol_%d.png', this_script_path, burst_idx));
    
    % The remaining bits are descrambled using the same initial value, but more bits
    second_scrambler = generate_scrambler_seq(7200, scrambler_x2_init);

    % Only descramble the remaining data symbols (ignoring the ZC sequences in 4 and 6, and the first data symbol)
    bits = bits([2,3,5,7,8,9],:);
    
    % Just converting the bits matrix into a vector to make XOR'ing easier
    bits = reshape(bits.', 1, []);

    % Run the actual XOR
    bits = bitxor(bits, second_scrambler);

    % Write the descrambled bits to disk as 8-bit integers
    handle = fopen("/tmp/bits", "wb");
    fwrite(handle, bits, 'int8');
    fclose(handle);

    % Run the Turbo decoder and rate matcher
    [retcode, out] = system(sprintf("%s %s", turbo_decoder_path, "/tmp/bits"));
    if (retcode ~= 0)
        warning("Failed to run the final processing step");
    end
    
    % Save off the hex values for the frame
    frames{burst_idx} = out;
end

% Print out all frames in hex
for idx=1:size(bursts, 1)
    frame = frames{idx};

    fprintf('FRAME: %s', frame);
end