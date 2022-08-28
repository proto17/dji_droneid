clear all;

% This script is meant to test using the cyclic prefix to find the starting time offset (STO) of a
% DroneID burst

%% Parameters
sample_rate = 15.36e6;
fft_size = get_fft_size(sample_rate);
[long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(sample_rate);
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
num_data_carriers = 600;
left_guards = (fft_size / 2) - (num_data_carriers / 2);
right_guards = left_guards - 1;
num_ofdm_symbols = 9;
carrier_spacing = sample_rate / fft_size;
zero_padding_count = 100;
modulation_order = 4;

% User definable parameters
integer_freq_offset = -58;
snr = 0;
enable_plots = 1;

%% Create Time Domain Samples
modulator = comm.OFDMModulator( ...
    "FFTLength",            fft_size, ...
    "NumGuardBandCarriers", [left_guards; right_guards], ...
    "CyclicPrefixLength",   cyclic_prefix_length_schedule, ...
    "NumSymbols",           num_ofdm_symbols, ...
    "InsertDCNull",         true);

% Generate the PSK that will be used in the data carriers of each OFDM sym
data_carriers = pskmod(...
    randi([0, log2(modulation_order)], num_data_carriers, num_ofdm_symbols), ...
    modulation_order);

% Create the time domain samples and convert to a row vector
samples = modulator(data_carriers);
samples = reshape(samples, 1, []);

% Add in zeros to the left and right
samples = [zeros(1, zero_padding_count), samples, zeros(1, zero_padding_count)];

if (enable_plots)
    figure(1);
    plot(10 * log10(abs(samples).^2));
    title('Original Time Domain');
end

%% Apply Integer Frequency Offset
int_freq_offset_hz = integer_freq_offset * carrier_spacing;
int_offset_vec = exp(1j * 2 * pi * int_freq_offset_hz / sample_rate * [0:(length(samples)-1)]);

samples = samples .* int_offset_vec;

%% Add Noise
samples = awgn(samples, snr, 'measured');

if (enable_plots)
    figure(2);
    plot(10 * log10(abs(samples).^2));
    title('Samples With Noise & Int Freq Offset');
end

%% Find STO with Cyclic Prefix

% How far from the first sample to search for the STO
num_offsets = zero_padding_count * 1.5;
scores_cp_sto = zeros(1, num_offsets);

for idx=1:length(scores_cp_sto)
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
        scores(cp_idx) = abs(xcorr(left, right, 0, 'normalized'));
        
        % Move the sample pointer forward by the full symbol size
        offset = offset + cp_len + fft_size;
    end
    
    % In the real DroneID the first OFDM symbol needs to be ignored since
    % it isn't always present.  So, just average the correlation scores of
    % all but the first element
    scores_cp_sto(idx) = sum(scores(2:end)) / (length(scores) - 1);
end

if (enable_plots)
    figure(3);
    plot(scores_cp_sto);
    title('STO Scores (Cyclic Prefix)')
end

% Find the index of the highest score
[~, sto_est_cp] = max(scores_cp_sto);
fprintf("True offset: %d, estimated offset: %d\n", zero_padding_count + 1, sto_est_cp);

if (enable_plots)
    figure(4);
    plot(10 * log10(abs(fftshift(fft(samples))).^2));
    title('FFT of Noisy Signal');
end

% XCorr for ZC -> Extract Burst -> CP STO -> Coarse CFO -> Integer CFO -> Equalization
