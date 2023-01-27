% This script is for testing how to detect integer frequeny offsets (IFO) in OFDM

clear all;

%% Parameters
sample_rate = 15.36e6;
fft_size = get_fft_size(sample_rate);
carrier_spacing = sample_rate / fft_size;
data_carriers = 600;
starting_zeros = 100;
cp_len = 80;
frac_frequency_offset = 0e3;
carrier_offset = -1;
freq_offset = carrier_spacing * carrier_offset;
cp_backoff = 0;
max_allowed_int_offset = 5;
enable_plots = 0;
snr = 15;

if (abs(carrier_offset) > max_allowed_int_offset)
    warning("TEST WILL FAIL!!  OFFSET TOO HIGH");
end

%% Get Data Carrier Indices
left_carriers = (fft_size / 2) - (data_carriers / 2);
right_carriers = left_carriers - 1;
carrier_indices = (left_carriers + 1):(left_carriers + 1 + data_carriers);

%% Create OFDM Symbol

% Generate ZC sequence and zero the middle sample (will become FFT DC)
zc_seq = zadoffChuSeq(600, 601);
zc_seq((data_carriers / 2) + 1) = 0;

% Build the FFT freq domain by placing the ZC sequence in the middle with
% the nulled out ZC sample at DC
golden_freq_domain = reshape([zeros(left_carriers, 1); zc_seq; zeros(right_carriers, 1)], 1, []);

% Convert to time domain
golden_time_domain = ifft(golden_freq_domain);

% Create and prepend the cyclic prefix
cp = golden_time_domain(end-cp_len+1:end);
ofdm_symbol = [cp, golden_time_domain];

%% Create Sample Vector with Burst

% Create a "burst" with zeros on either side
samples = [zeros(1, starting_zeros), ofdm_symbol, zeros(1, starting_zeros)];

% Add some noise
samples = awgn(samples, snr, 'measured');

if (enable_plots)
    figure(3);
    plot(10 * log10(abs(samples).^2));
end

%% Apply Integer Frequency Offset

% Generate the vector that will be used to rotate each sample
freq_offset_vector = exp(1j * 2 * pi * freq_offset / sample_rate * [1:length(samples)]);
samples = samples .* freq_offset_vector;

%% Apply Fractional Frequency Offset
frac_offset_vector = exp(1j * 2 * pi * frac_frequency_offset / sample_rate * [1:length(samples)]);
samples = samples .* frac_offset_vector;

%% Find Starting Sample Index

% Use autocorrelation to search for the start of the ZC sequence.  This
% takes advantage of the fact that the ZC sequence is symmetrical about the
% center in both the time and frequency domains.  Each iteration of the
% loop below will look at one `fft_size` window of samples sliding to the
% right by one sample on each iteration.  The window is then split in two
% with the second half fliped so that it's backwards.  These two windows
% are then correlated with each other, and the score saved off
sto_search_window = fft_size;
sto_scores = zeros(1, starting_zeros * 2);
for td_idx=1:length(sto_scores)
    sto_window = samples(td_idx:td_idx + fft_size - 1);
    sto_left_window = sto_window(1:(fft_size / 2));
    sto_right_window = sto_window((fft_size / 2) + 1:end);
    sto_scores(td_idx) = xcorr(sto_left_window, fliplr(sto_right_window), 0, 'normalized');
end

if (enable_plots)
    figure(1);
    plot(abs(sto_scores)); title('STO Scores');
end

% Find the index of the maximum value from the autocorrelation above
[~, sto_est] = max(abs(sto_scores));
