clear all

%% Signal parameters
file_path = '/opt/dji/collects/2437MHz_30.72MSPS.fc32';
sample_rate = 30.72e6;          % Collected 2x oversampled
critial_sample_rate = 15.36e6;  % The actual sample rate for this signal
carrier_spacing = 15e3;         % Per the LTE spec
signal_bandwidth = 10e6;        % Per the LTE spec
rough_frequency_offset = 7.5e6; % The collected signal is 7.5 MHz off center


%% Read in the IQ samples for a single burst
%  The burst is found using baudline and its cursor/delta time measurements
file_start_time = 0.802225;
file_start_sample_idx = uint64(file_start_time * sample_rate);

burst_duration_time = 0.000721;
burst_duration_samples = uint64(burst_duration_time * sample_rate);

samples = read_complex_floats(file_path, file_start_sample_idx, burst_duration_samples);

figure(1);
plot(10 * log10(abs(fftshift(fft(samples)))));
title('Original Samples')

%% Roughly center the signal of interest
%  The signal might have been collected at an offset, so remove that offset
rotation_vector = exp(1j * 2* pi / sample_rate * (0:length(samples)-1) * rough_frequency_offset);
samples = samples .* rotation_vector;

figure(1000);
plot(10 * log10(abs(fftshift(fft(samples)))));
title('Original Samples - Freq Shifted')

orig_fft_size = sample_rate / carrier_spacing;
orig_long_cp_len = round(1/192000 * sample_rate);
orig_short_cp_len = round(0.0000046875 * sample_rate);

%% Low pass filter the original samples
filter_taps = fir1(200, signal_bandwidth/sample_rate);
samples = filter(filter_taps, 1, samples);

%% Search for the ZC sequence in symbol 4

% Create the ZC seqeunce
zc = create_zc(orig_fft_size, 4);
zc = reshape(zc, 1, []); % Reshape to match the samples vector

% Run a normalized cross correlation searching for the ZC sequence
% TODO(8Apr2022): Search through a smaller space (no need to look through everything)
scores = zeros(1, length(samples)-length(zc));
for idx=1:length(scores)
    scores(idx) = normalized_xcorr(zc, samples(idx:idx + length(zc) - 1));
end

% Find the highest score value and index
[score, index] = max(abs(scores).^2);
if (score < 0.7)
    warning("Correlation score for the first ZC sequence was bad.  Might have errors later");
end

% The ZC sequence is the fourth symbol which means there are three FFT windows, one 
% long cyclic prefix and two short cyclic prefixes before the ZC.  But, to keep the
% correlation window as small as possible, the cyclic prefix of the ZC is ignored,
% so that adds an additional short cyclic prefix of offset
zc_seq_offset = (orig_fft_size * 3) + orig_long_cp_len + (orig_short_cp_len * 3);

% Calculate where the burst starts based on the correlation index
start_offset = round(index - zc_seq_offset);

angle(scores(index))
% samples = samples .* exp(1j * -angle(scores(index)) * (0:(length(samples)-1)));
samples = samples .* exp(1j * -angle(scores(index)));

% Trim all samples before the starting offset, and decimate at the same time.
% The decimation operation here is to get the signal back down to critical rate.
% Can get away with dropping every other sample because the signal is already filtered
samples = samples(start_offset:2:end);

%% Calculate critical rate parameters
fft_size = critial_sample_rate / carrier_spacing;
long_cp_len = round(0.0000052 * critial_sample_rate);
short_cp_len = round(0.00000469 * critial_sample_rate);

%% Plot symbol overlays
%  The logic below is for debugging only.  It draws OFDM symbol boundaries
%  over a time domain view of the burst.  It alternates between red and
%  green transparent rectangles over the time domain as a sanity check that
%  the time alignment is correct (starting at the right sample)
plot_symbol_boundaries(samples, critial_sample_rate, 4);

%% Coarse frequency adjustment

% My CFO detection isn't working, so rotate the constellation by hand (just eyeballing the constellation plots)
% offset_adj = -0.00005;
% samples = samples .* exp(1j * 2 * pi * offset_adj * (0:length(samples)-1));


%% Phase adjustment

% The phase angle adjustment made in the ZC sequence detection step got the QPSK to be lined up with points in 
% [1,0], [0,1], [-1,0], and [0,-1] but we want it at [1,1], [-1,1], [-1,-1], and [1,-1] to make the demodulation
% decision simpler.  That means that the constellation needs to be rotated by pi/2 (45 degrees).
% TODO(10Apr2022): Because of timing offsets there is a lot of "smearing" as the constellation rotates.  Instead of
%                  fixing that right now, the constellation is rotated a little more to keep the points in distinct
%                  quadrants.
phase_offset = pi/2 - 0.5;
samples = samples * exp(1j * phase_offset);

%% Symbol extraction

symbols_time_domain = zeros(9, fft_size);
symbols_freq_domain = zeros(9, fft_size);

sample_offset = 1;
for symbol_idx=1:9
    % OFDM symbols 1 and 9 have a long cyclic prefix
    if (symbol_idx == 1 || symbol_idx == 9)
        cyclic_prefix_len = long_cp_len;
    else
        cyclic_prefix_len = short_cp_len;
    end
    
    % Skip the cyclic prefix
    sample_offset = sample_offset + cyclic_prefix_len;

    % Extract out just the OFDM symbol ignoring the cyclic prefix
    symbol = samples(sample_offset:sample_offset + fft_size - 1);

    % Save off the original time domain and the FFT'd samples
    symbols_time_domain(symbol_idx,:) = symbol;
    symbols_freq_domain(symbol_idx,:) = fftshift(fft(symbol));

    % Skip the OFDM symbol just extracted
    sample_offset = sample_offset + fft_size;
end

% Plot the constellation diagrams for all extracted OFDM symbols
figure(5)
for idx=1:9
    subplot(3, 3, idx);
    plot(symbols_freq_domain(idx,:), 'o');
end
title('Constellations (Pre Freq Correction)')

%% Demodulation

% The spec says that the 15.36 MHz signal has 601 occupied carriers, but
% one of those is DC which doesn't appear to be used
% NOTE: I have seen someone saying that there is a specific phase present
% in some of the DC.  Not sure if this is correct, so for now assuming that
% the DC carrier is not used
occupied_carriers = 600;

% Create a list of data carrier indices
data_carriers_left = (fft_size/2)-(occupied_carriers/2)+1:fft_size/2;
data_carriers_right = (fft_size/2)+2:fft_size/2+(occupied_carriers/2)+1;

% Below is logic to write the received ZC sequence out to a file.  It is meant to be used with the
% brute_force_zc.m script
%
% % figure(1000); plot(abs(symbols_time_domain(4,:)).^2);
% file_handle = fopen('/opt/dji/collects/zc_symbol_4_15360KSPS_time_domain.fc32', 'w');
% zc_samples = symbols_freq_domain(4, [data_carriers_left, data_carriers_right]);
% reals = reshape([real(zc_samples); imag(zc_samples)], [], 1);
% fwrite(file_handle, reals, 'single');
% fclose(file_handle);
% 
% % figure(1000); plot(abs(symbols_time_domain(6,:)).^2);
% file_handle = fopen('/opt/dji/collects/zc_symbol_6_15360KSPS_time_domain.fc32', 'w');
% zc_samples = symbols_freq_domain(6, [data_carriers_left, data_carriers_right]);
% reals = reshape([real(zc_samples); imag(zc_samples)], [], 1);
% fwrite(file_handle, reals, 'single');
% fclose(file_handle);

% Place to store the data carrier samples
data_carriers = zeros(7, occupied_carriers);

% Plot and extract the data carriers from the 7 data carrying OFDM symbols
figure(6);
output_idx = 1;

% Symbols 4 and 6 are ZC sequences and do not contain data
for idx=[1,2,3,5,7,8,9]
    subplot(3, 3, idx);

    % Get all of the freq domain samples for the current symbol, extract
    % out just the data carriers, and plot
    carriers = symbols_freq_domain(idx,:);
    data_carriers(output_idx,:) = carriers([data_carriers_left, data_carriers_right]);
    plot(squeeze(data_carriers(output_idx,:)), 'o');
    output_idx = output_idx + 1;
end

% Plot the data carriers as one constellation (first one with lines, second
% with dots)
figure(7);
subplot(1, 2, 1);
plot(squeeze(data_carriers));
subplot(1, 2, 2);
plot(squeeze(data_carriers), 'o');

%% Below is a brute force attempt at trying to get the first data symbol to drop out as all 0's or 1's

% Initializing to 0x12345678
scrambler_x2_init = [0 0 1, 0 0 1 0, 0 0 1 1, 0 1 0 0, 0 1 0 1, 0 1 1 0, 0 1 1 1, 1 0 0 0];

% Generate enough random for one OFDM symbol (need two bits per data carrier since there are 2 bits per sample)
scrambler_bit_count = length(data_carriers) * 2;

% Try out 4 possibilities for the scrambler.  It's unclear if the order of the initialization vector
% is correct, nor if the output of the scrambler needs to be reversed before getting applied.  So, try
% all 4 combos
scrambler_perms = [...
    generate_scrambler_seq(scrambler_bit_count, scrambler_x2_init);
    generate_scrambler_seq(scrambler_bit_count, fliplr(scrambler_x2_init));
    fliplr(generate_scrambler_seq(scrambler_bit_count, scrambler_x2_init));
    fliplr(generate_scrambler_seq(scrambler_bit_count, fliplr(scrambler_x2_init)));
];

% Number of bits to XOR and show in the terminal
xor_window = 128;

% TODO(10April2022): REMOVE THIS!!!  IT'S ONLY HERE TO TROUBLESHOOT THE DESCRAMBLER!!!
data_carriers = data_carriers(1,:);
figure(88);
% Loop over the data carriers 4 times, each time rotating the constellation by 90 degrees
% This is a brute force approach to finding the correct phase offset since it's unknown due
% to not having a known training sequence
for idx=0:3
    % Rotate all samples by 90 degrees
    offset = pi / 2;
    data_carriers = data_carriers .* exp(1j * offset);

    subplot(2, 2, idx+1);
    plot(data_carriers, 'o');

    % Demapping the constellation points by hand.  The mapping was taken from 
    % https://github.com/ttsou/openphy/blob/master/src/lte/qam.c#L35

    % Using concatenation like a chump
    demodulated_bits = [];
    for sample_idx = 1:length(data_carriers)
        sample = data_carriers(sample_idx);
        if (real(sample) > 0 && imag(sample) > 0)
            bits = [0, 0];
        elseif (real(sample) > 0 && imag(sample) < 0)
            bits = [0, 1];
        elseif (real(sample) < 0 && imag(sample) > 0)
            bits = [1, 0];
        elseif (real(sample) < 0 && imag(sample) < 0)
            bits = [1, 1];
        else
            error("Invalid coordinate!");
        end

        demodulated_bits = [demodulated_bits, bits];
    end
    
    % Try all 4 scrambler possibilities
    xor_out = [
        char('0'+bitxor(demodulated_bits(1,1:xor_window), scrambler_perms(1,1:xor_window))); 
        char('0'+bitxor(demodulated_bits(1,1:xor_window), scrambler_perms(2,1:xor_window)));
        char('0'+bitxor(demodulated_bits(1,1:xor_window), scrambler_perms(3,1:xor_window))); 
        char('0'+bitxor(demodulated_bits(1,1:xor_window), scrambler_perms(4,1:xor_window)));
    ]
end
