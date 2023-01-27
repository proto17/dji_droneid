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
enable_plots = true;        % Set to false to prevent the plots from popping up
correlation_threshold = 0.7; % The SNR is pretty good, so using a high correlation score (must be between 0.0 and 1.0)
chunk_size = 10e6;           % Number of samples to process at a time
enable_equalizer = true;     % Enable/disable the frequency domain equalizer

%% Paramters that the user must change
sample_type = 'single';
file_path = 'YOUR_FILE_NAME_HERE';
file_sample_rate = YOUR_SAMPLE_RATE_HERE;
file_freq_offset = 0e6;

%% Low Pass Filter Setup
signal_bandwidth = 10e6; % The actual occupied bandwidth of the DroneID signal
filter_tap_count = 50;   % Number of filter taps to use for the low pass filter
filter_taps = fir1(filter_tap_count, signal_bandwidth/file_sample_rate); % Create the low pass filter taps

%% Burst Extraction
[long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(file_sample_rate);
cyclic_prefix_schedule = [
    long_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    long_cp_len];
fft_size = get_fft_size(file_sample_rate);

% A correlation figure number of -1 will prevent plotting by the find_zc_indices_by_file function
correlation_fig_number = -1;
if (enable_plots)
    correlation_fig_number = 456;
end

% Making sure that the bursts that are extracted have enough padding for the low pass filter to start up and terminate
bursts = extract_bursts_from_file(file_path, file_sample_rate, file_freq_offset, correlation_threshold, chunk_size,...
    filter_tap_count, 'SampleType', sample_type, 'CorrelationFigNum', correlation_fig_number);

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
cfo_estimation_symbol_idx = 2;

%% Burst Processing
for burst_idx=1:size(bursts, 1)
    % Get the next burst
    burst = bursts(burst_idx,:);

    if (enable_plots)
        figure(43);
        subplot(2, 1, 1);
        plot(10 * log10(abs(burst).^2));
        title('Time domain abs^2 10log10 (original)');
        
        % Plot the FFT, but average it with a single pole IIR filter to make it smoother
        figure(1000);
        fft_bins = 10 * log10(abs(fftshift(fft(burst))).^2);
        running = fft_bins(1);
        beta = 0.06;
        for idx = 2:length(fft_bins)
            running = (running * (1 - beta)) + (fft_bins(idx) * beta);
            fft_bins(idx) = running;
        end
        x_axis = file_sample_rate / 2 * linspace(-1, 1, length(burst));
        plot(x_axis, fft_bins);
        title('Frequency Spectrum (averaged)');
        grid on;
    end

    %% Find Integer Frequency Offset

    % Exploiting the fact that during the first ZC sequence the DC carrier will be much lower in amplitude than the 
    % surrounding samples.  Steps:
    %   1. Extract just the time domain samples used in the first ZC sequence
    %   2. Interpolate those time domain samples to increase the frequency resolution of the measurement
    %   3. Get the power spectrum (abs squared of the FFT)
    %   4. Look N elements around the center of the FFT for the lowest point (this is the center of the signal)
    %   5. Calculate how far off from 0 Hz the lowest bin was, and frequency shift the upsampled signal by that value
    %   6. Decimate the samples back to the original sample rate for further processing

    % Calculate the first sample index for the first ZC sequence (skipping the cyclic prefix)
    offset = sum(cyclic_prefix_schedule(1:4)) + (fft_size * 3) + filter_tap_count;
    
    % Upsample (interpolate and filter) the ZC sequence samples
    interp_rate = 10;
    burst = resample(burst, interp_rate, 1);
    
    % Extract out just the samples for the first ZC sequence
    zc_samples = burst((offset * interp_rate):(offset * interp_rate) + (fft_size * interp_rate) - 1);

    % Convert the time domain ZC sequence samples to the frequency domain
    fft_bins = 10 * log10(abs(fftshift(fft(zc_samples))).^2);
    
    % Loop for the lowest bin in the middle of the frequency domain spectrum
    bin_count = 15; % How far left and right to look for the lowest carrier

    % Set all of the FFT bins on the outside to infinity so they can't possibly be the minimum value
    fft_bins(1:(fft_size * interp_rate / 2) - bin_count) = Inf;
    fft_bins((fft_size * interp_rate / 2) + bin_count - 1:end) = Inf;

    % Find the index of the FFT bin with the lowest amplitude
    [~, center_offset] = min(fft_bins);
    
    % Calculate the frequency needed to correct the integer offset, then conver that to radians
    integer_offset = ((fft_size * interp_rate / 2) - center_offset + 1) * 15e3;
    radians = 2 * pi * integer_offset / (file_sample_rate * interp_rate);
    
    % Apply a frequency adjustment
    burst = burst .* exp(1j * radians * [0:length(burst) - 1]);
    
    % Downsample (filter and decimate) the burst samples
    burst = resample(burst, 1, interp_rate);
    
    %% Apply low pass filter
    burst = filter(filter_taps, 1, burst);

    if (enable_plots)
        figure(43);
        subplot(2, 1, 2);
        plot(10 * log10(abs(burst).^2));
        title('Time domain abs^2 10log10 (filtered)')
    end

    %% Interpolate and find the true starting sample offset
    interp_factor = 1;
    burst = resample(burst, interp_factor, 1);
    true_start_index = find_sto_cp(burst, file_sample_rate * interp_factor);
    burst = resample(burst(true_start_index:end), 1, interp_factor);

    % Plot cyclic prefixes overlayed with the replica from the end of the OFDM symbol
    if (enable_plots)
        offset = 1;
        figure(7777);
        for cp_idx=1:length(cyclic_prefix_schedule)
            subplot(3, 3, cp_idx);
            symbol = burst(offset:offset + cyclic_prefix_schedule(cp_idx) + fft_size - 1);
            left = symbol(1:cyclic_prefix_schedule(cp_idx));
            right = symbol(end - cyclic_prefix_schedule(cp_idx) + 1:end);
            plot(abs(left));
            hold on
            plot(abs(right));
            hold off;
            title(['Cyclic Prefix Overlay ', mat2str(cp_idx)]);
    
            offset = offset + length(symbol);
        end
    end

    %% Coarse frequency offset adjustment using one of the OFDM symbols (see coarse_cfo_symbol_sample_offset definition)

    % Get the expected starting index of the symbol to be used for CFO estimation
    zc_start = long_cp_len + (fft_size * 3) + (short_cp_len * 3);
    
    % Extract out the full OFDM symbol (cyclic prefix included)
    cfo_est_symbol = burst(zc_start - short_cp_len:zc_start + fft_size - 1);

    % Get the cyclic prefix, and then the copy of the cyclic prefix that exists at the end of the OFDM symbol
    cyclic_prefix = cfo_est_symbol(1:short_cp_len);
    symbol_tail = cfo_est_symbol(end - short_cp_len + 1:end);

    skip = 0;
    cyclic_prefix = cyclic_prefix(skip+1:end-skip);
    symbol_tail = symbol_tail(skip+1:end-skip);
    
    % Calculate the frequency offset by taking the dot product of the two copies of the cyclic prefix and dividing out
    % the number of samples in between each cyclic prefix sample (the FFT size)
    offset_radians = angle(dot(cyclic_prefix, symbol_tail)) / fft_size;
    offset_hz = offset_radians * file_sample_rate / (2 * pi);

    if (enable_plots)
        figure(999);
        plot(abs(cyclic_prefix).^2);
        hold on;
        plot(abs(symbol_tail).^2, '*-', 'Color', 'red');
        hold off;
        title('Cyclic Prefix Overlay - CFO Estimate')
    end

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
        title('ZC Sequence 1 Channel')
        subplot(2, 1, 2);
        plot(abs(channel2).^2, '-');
        title('ZC Sequence 2 Channel')
    end

    % Only use the fisrt ZC sequence to do the initial equaliztion.  Trying to use the average of both ends up with
    % strange outliers in the constellation plot
    channel = channel1;

    % Place to store the demodulated bits
    bits = zeros(9, 1200);

    % Walk through each OFDM symbol and extract the data carriers and demodulate the QPSK inside
    % This is done for symbols 4 and 6 even though they contain ZC sequences.  It's just to keep the logic clean
    
    for idx=1:size(bits, 1)
        data_carriers = freq_domain_symbols(idx,data_carrier_indices);

        if (enable_equalizer)
            % Equalize just the data carriers
            data_carriers = data_carriers .* channel;
        end

        % Demodulate/quantize the QPSK to bits
        bits(idx,:) = quantize_qpsk(data_carriers);
        
        if (enable_plots)
            figure(1);
            subplot(3, 3, idx);
            plot(data_carriers, 'o');
            title(['Symbol ', mat2str(idx), ' IQ']);
    
            figure(111);
            subplot(3, 3, idx);
            plot(10 * log10(abs(time_domain_symbols(idx,:)).^2), '-');
            title(['Symbol ', mat2str(idx), ' Time Domain']);

            figure(112);
            subplot(3, 3, idx);
            plot(10 * log10(abs(freq_domain_symbols(idx,:)).^2));
            title(['Symbol ', mat2str(idx), ' Freq Domain']);
        end
    end
    
    if (enable_plots)
        % Save the constellation plots to disk for debugging
        % THIS CAN BE COMMENTED OUT IF NEEDED
        png_path = sprintf('%s/images/ofdm_symbol_%d.png', this_script_path, burst_idx);

        try
            saveas(gcf, png_path);
        catch
            error('Could not write out PNG file to "%s"', png_path);
        end
    end
    
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
