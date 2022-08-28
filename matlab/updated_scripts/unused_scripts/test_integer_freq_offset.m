clear all;

% This script is used to test time and frequency estimation / correction of
% a simulated DroneID burst.  No equalization is done in this script.
% There is a simulated fractional time offset applied, but the script will
% fail if that time offset is anything other than 0 or 0.5.  This is
% something that will likely have to be fixed by the equalizer or by
% upsampling to find the most correct fractional starting time offset

%% Parameters
sample_rate = 15.36e6;
fft_size = get_fft_size(sample_rate);
carrier_spacing = round(sample_rate / fft_size);

% Pick a worst case Parts Per Million (PPM) to support.  20 PPM is the
% value for the HackRF SDR's oscialltor.  Then choose the worst case
% frequency that the SDR will operate at.  In this case 5.9 GHz is the
% highest frequency the radio will need to tune to for DroneID.  Use that
% to then figure out what the worst case frequency offset (in Hz) can be.
% That will be used as the search space for the Integer Frequency Offset
% (IFO) which is done later.
worst_case_ppm = 20;
worst_case_freq = 5.9e9;
max_allowed_freq_offset = worst_case_freq * worst_case_ppm / 1e6;
max_allowed_int_freq_offset = ceil(max_allowed_freq_offset / carrier_spacing);

% Pick a frequency offset to test with
frequency_offset = max_allowed_freq_offset - 3.3e3;

% Calculate the long and short cyclic prefix lengths
[long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(sample_rate);

% Below are the cyclic prefix lengths for each OFDM symbol
cyclic_prefixes = [
    long_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    long_cp_len
];

% Calculate how many samples there are in one DroneID burst
total_burst_sample_count = (fft_size * length(cyclic_prefixes)) + sum(cyclic_prefixes);

% Calculate how many guard carriers there are in one OFDM symbol
[left_guards, right_guards] = get_num_guard_carriers(sample_rate);

% Number of constellation points in the underlying PSK
modulation_order = 4;

% Number of occupied OFDM carriers per symbol
num_data_carriers = 600;

%% OFDM Time Domain Creation
modulator = comm.OFDMModulator( ...
    "FFTLength", fft_size, ...
    "NumGuardBandCarriers", [left_guards; right_guards], ...
    "CyclicPrefixLength", cyclic_prefixes, ...
    "NumSymbols", length(cyclic_prefixes), ...
    "InsertDCNull", true);

% Modulate 9 random OFDM symbols worth of PSK samples
symbols = pskmod( ...
    randi([0, modulation_order - 1], num_data_carriers, length(cyclic_prefixes)), ...
    modulation_order);

% Create the OFDM time domain
modulated_samples = modulator(symbols);

%% Insert ZC Sequence at symbol 4

% Figure out where in the modulated samples that the ZC sequence should
% start (this includes the cyclic prefix)
symbol_4_offset = (fft_size * 3) + sum(cyclic_prefixes(1:3));

% Create the time domain ZC sequence for symbol 4
zc_seq = create_zc(fft_size, 4);

% Tack on the cyclic prefix to build the full symbol
full_zc_seq = [zc_seq(end-cyclic_prefixes(4)+1:end); zc_seq];

% Insert the full ZC sequence symbol in place of the modulated symbol 4
modulated_samples(symbol_4_offset:symbol_4_offset + length(full_zc_seq) - 1) = full_zc_seq;

% Convert to row vector
modulated_samples = reshape(modulated_samples, 1, []);

%% Prepend/Append Zeros

% These zeros are needed to simulate an actual received sample and the
% ambiguity of where the burst actually starts.  The number of samples
% isn't critical, but should be something > about 30 samples
num_zeros = 50;

zero_vec = zeros(1, num_zeros);
modulated_samples = [zero_vec, modulated_samples, zero_vec];

%% Add in a 0.5 sample time offset

% This simulates the clocks of the transmitter and receiver not being
% perfectly synced up.  Upsample by 4, then skip the first two samples, and
% downsample by 4
modulated_samples = resample(modulated_samples, 4, 1);
modulated_samples = [modulated_samples(2:end), 0, 0];
modulated_samples = resample(modulated_samples, 1, 4);

%% Add Noise

% This is not an exact SNR.  Just add gaussian noise to the signal to
% simulate real world reception
modulated_samples = awgn(modulated_samples, 10, 'measured');

figure(1);
plot(10 * log10(abs(modulated_samples).^2));
title('Time Domain with Noise');
%% Apply Frequency Offset

freq_offset_vector = exp(1j * 2 * pi * frequency_offset / sample_rate * [1:length(modulated_samples)]);
freq_offset_samples = modulated_samples .* freq_offset_vector;

figure(2);
subplot(3, 1, 1); plot(10 * log10(abs(fftshift(fft(modulated_samples))).^2));
title('Original Modulated Samples (FFT)')
subplot(3, 1, 2); plot(10 * log10(abs(fftshift(fft(freq_offset_vector))).^2));
title('Frequency Offset Vector (FFT)')
subplot(3, 1, 3); plot(10 * log10(abs(fftshift(fft(freq_offset_samples))).^2));
title('Frequency Offset Samples (FFT)')

%% Find the ZC sequence

% Find the start of the burst by looking at the cyclic prefixes.  Make sure
% to look PAST `num_zeros` since in the real world it will not be known
% where the burst actually starts, so the search space will be rather large
sto_estimate = est_sto(freq_offset_samples, sample_rate, num_zeros * 1.5);

% Calculate how many samples after the start of the burst that the ZC
% sequence should be located (this offset includes the ZC sequence cyclic
% prefix)
zc_seq_offset = (fft_size * 3) + long_cp_len + (short_cp_len * 3);

% Calculate where in the sample vector the ZC sequence starts
auto_corr_index = sto_estimate + zc_seq_offset;

fprintf("Sample offset error: %d\n", sto_estimate - num_zeros - 1);
%% Estimate coarse freq offset and adjust

% Use the cyclic prefix of the ZC sequence to do coarse frequency offset
% estimation (the ZC sequence is the 4th OFDM symbol).  
coarse_cp_len = cyclic_prefixes(4);
cp_len_window_size = coarse_cp_len;

% The auto_corr_index points to the start of the ZC sequence, so back off
% by the length of the 4th cyclic prefix
coarse_offset_start = auto_corr_index - coarse_cp_len;

% Extract out `fft_size + coarse_cp_len` samples
window = freq_offset_samples(coarse_offset_start:coarse_offset_start + fft_size + coarse_cp_len - 1);

% Extract the first and last `coarse_cp_len` samples from the window
window_left = window(1:coarse_cp_len);
window_right = window(end - coarse_cp_len + 1:end);

% Calculate the coarse frequency offset in radians and Hz by taking the dot
% product of the two windows
cfo_est_radians = angle(sum(window_left .* conj(window_right))) / fft_size;
cfo_est_hz = cfo_est_radians * sample_rate / (2 * pi);

% Adjust for the frequency offset
cfo_est_adj_vector = exp(1j * cfo_est_radians * [1:length(freq_offset_samples)]);
freq_offset_samples = freq_offset_samples .* cfo_est_adj_vector;

%% Integer frequency offset estimation
% The ZC sequence will be used to find the integer frequency offset.  So,
% extract out just the samples for the ZC sequence (not including the
% cyclic prefix)
received_zc_sym = freq_offset_samples(auto_corr_index:auto_corr_index + fft_size - 1);

figure(3); 
plot(abs(received_zc_sym));
title('ZC Sequence (Time Domain Magnitude)');

% Estimate the integer frequency offset
est_int_offset_hz = est_integer_freq_offset(received_zc_sym, sample_rate, max_allowed_int_freq_offset);

% Adjust for the integer frequency offset
ifo_est_adj_vector = exp(1j * 2 * pi * est_int_offset_hz / sample_rate * [1:length(freq_offset_samples)]);
freq_offset_samples = freq_offset_samples .* ifo_est_adj_vector;

% Print out how far off the combination of coarse and integer freq offset
% was from the true value
residual_freq_error_hz = est_int_offset_hz + cfo_est_hz + frequency_offset;
fprintf("Remaining Frequency Offset (Hz): %f\n", residual_freq_error_hz);

%% Extract OFDM Data Carriers
demod = comm.OFDMDemodulator( ...
    "FFTLength", fft_size, ...
    "NumGuardBandCarriers", [left_guards; right_guards], ...
    "CyclicPrefixLength", cyclic_prefixes, ...
    "NumSymbols", length(cyclic_prefixes), ...
    "RemoveDCCarrier", true);

% Extract out just the samples in the burst and run those through the OFDM
% demodulator
burst_samples = freq_offset_samples(sto_estimate:sto_estimate + total_burst_sample_count - 1);
demod_symbols = demod(reshape(burst_samples, [], 1));

% Plot just the data carrying OFDM symbol constellations (the 4th symbol is
% the ZC sequence)
figure(4);
plot(demod_symbols(:,[1,2,3,5,6,7,8,9]), 'o');
title('Demodulated Constellations')
