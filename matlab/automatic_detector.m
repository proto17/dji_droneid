clear all

%% Signal parameters
file_path = '/opt/dji/collects/5782GHz_30720KSPS.fc32';
sample_rate = 30.72e6;          % Collected 2x oversampled
critial_sample_rate = 15.36e6;  % The actual sample rate for this signal
carrier_spacing = 15e3;         % Per the LTE spec
signal_bandwidth = 10e6;        % Per the LTE spec
rough_frequency_offset = 5.5e6; % The collected signal is 5.5 MHz off center


%% Read in the IQ samples for a single burst
%  The burst is found using baudline and its cursor/delta time measurements
file_start_time = 6.507678;
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

%% Interpolate the signal
%  The reason for interpolation is that it makes finding the correct
%  starting sample index more accurate.  The starting offset needs to be
%  very, very close so that pilots aren't needed.  A time offset results in
%  a walking phase offset after the FFT which is hard to remove without
%  pilots.  The higher the interpolation factor the better for getting the
%  correct offset, but that adds to computation time and memory use

% Keep in mind that the signal may already be interpolated (ie not
% critically sampled) at this point.  If the signal was recorded 2x
% oversampled, then an interpolation of 2 here means that the signal is 4x
% oversampled.
interpolation = 2;
interpolated_samples = zeros(1, length(samples) * interpolation);
interpolated_samples(1:interpolation:end) = samples;

% Define the interpolation filter.  The passband is 1/2 of the bandwidth
% (there are plenty of guard carriers to cover the stop-band).  Allowing
% for 400 KHz of stop-band.  Using 200 taps as that seems to work well
passband_edge = signal_bandwidth/2;
stopband_edge = signal_bandwidth/2 + 400e3;
filter_taps = fir1(200, signal_bandwidth/2/sample_rate*interpolation/2);
interpolated_samples = filter(filter_taps, 1, interpolated_samples);

% Calculate new rate dependant variables
interpolated_fft_size = sample_rate * interpolation / carrier_spacing;
interpolated_long_cp = round(1/192000 * sample_rate * interpolation);
interpolated_short_cp = round(0.0000046875 * sample_rate * interpolation);

figure(2);
plot(10 * log10(abs(fftshift(fft(interpolated_samples)))));
title('Interpolated Rate')

% No reason to search *all* of the samples.  The ZC sequence is the third
% OFDM symbol, so search for up to 5 OFDM symbols.  The first OFDM symbol
% has a long cyclic prefix, and the remaining 4 use the short sequence
search_window_length = (interpolated_long_cp + (interpolated_short_cp * 4)) + (interpolated_fft_size * 5);
[scores] = find_zc(interpolated_samples(1:search_window_length), sample_rate * interpolation);
[score, index] = max(abs(scores).^2);
if (score < 0.8)
    error('Failed to find the first ZC sequence');
end

% Calculate where in the interpolated samples the burst should start
% The calculation is just backing up by 2 short cyclic prefixed, 1 long
% cyclic prefix, and 3 FFT sizes
start_offset = index - (((interpolated_short_cp + interpolated_fft_size) * 2) + ...
    interpolated_long_cp + interpolated_fft_size);

% The signal is already filtered, so just throw away every N samples to
% decimate down to critical rate
decimation_rate = interpolation * (sample_rate / critial_sample_rate);
samples = interpolated_samples(start_offset:decimation_rate:end);

figure(3);
plot(10 * log10(abs(fftshift(fft(samples)))));
title('Critical Rate')

%% Calculate critical rate parameters
fft_size = critial_sample_rate / carrier_spacing;
long_cp_len = round(0.0000052 * critial_sample_rate);
short_cp_len = round(0.00000469 * critial_sample_rate);

true_start_index = round(start_offset/decimation_rate);

%% Plot symbol overlays
%  The logic below is for debugging only.  It draws OFDM symbol boundaries
%  over a time domain view of the burst.  It alternates between red and
%  green transparent rectangles over the time domain as a sanity check that
%  the time alignment is correct (starting at the right sample)
plot_symbol_boundaries(samples, critial_sample_rate, 4);

%% Coarse frequency adjustment
offset_freq = -.156e3;
offset_adj = offset_freq / critial_sample_rate;
% samples = samples .* exp(1j * 2 * pi * offset_adj * (0:length(samples)-1));

[offset_est] = estimate_cp_freq_offset(samples, fft_size, short_cp_len, long_cp_len);

% DANGER: I have no idea why this is necessary or why it's equal to the
% number of OFDM symbols, but here it is.  It's likely that my CFO
% estimation logic is bad.  But, this works for the moment, and temporary
% fixes are never actually kept around in code forever...
offset_est = offset_est * 9;

% Apply the offset correction
samples = samples .* exp(1j * 2 * pi * offset_est * (0:length(samples)-1));

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

% Demodulate the QPSK and assume it's gray coded
demodulated_bits = pskdemod(data_carriers, 2, 0, 'gray');

%% Descrambler
% Oof... this part's gonna be interesting