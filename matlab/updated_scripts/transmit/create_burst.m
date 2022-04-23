sample_rate = 15.36e6;
use_9_symbols = 0;

%% Build payload
% The `vars` definition is not in here as it contains location information used for testing.  You will need to fill in
% your own config here.
% Example: 
%   create_frame_bytes('SequenceNumber', uint16(1234), ...
%                      'SerialNumber', '!this is a test!');
frame_bytes = create_frame_bytes(vars{:});

%% Turbo encoding and rate matching

% Get the location of this script
if (is_octave)
  this_script_path = fileparts(mfilename('fullpath'));
else
  this_script_path = fileparts(matlab.desktop.editor.getActiveFilename);
end

% Make sure that the `add_turbo` binary exists.  If it doesn't then you need to make sure you have compiled the 
% dji_droneid/cpp folder
add_turbo_path = fullfile(this_script_path, '..', '..', '..', 'cpp', 'add_turbo');
if (~ isfile(add_turbo_path))
    error("Could not find Turbo encoder application at '%s'.  Check that the program has been compiled",...
        add_turbo_path);
end

% Where to store the files used to talk to the add_turbo binary
frame_bin_file = '/tmp/frame.bin';              % Will write payload bytes here
turbo_encoded_file = '/tmp/frame.bin.encoded';  % The add_turbo binary will create this

% Write out the payload to Turbo encode and rate match
file_handle = fopen(frame_bin_file, "w");
fwrite(file_handle, frame_bytes, 'uint8');
fclose(file_handle);

% Call the Turbo encoder
[retcode, out] = system(sprintf("%s '%s' '%s'", add_turbo_path, frame_bin_file, turbo_encoded_file));
if (retcode ~= 0)
    warning("Failed to run the Turbo encoder.  Details: %s", out);
end

% Verify that it created the expected output file
if (~ isfile(turbo_encoded_file))
    error("Turbo encoder did not produce an output file");
end

% Read in all bytes from the encoded output file
file_handle = fopen(turbo_encoded_file, "r");
encoded_bits = uint8(fread(file_handle, inf, 'uint8'));
fclose(file_handle);

% Make sure that there are exactly the right number of bytes in the file created by the Turbo encoder
required_bytes = 7200;
if (length(encoded_bits) ~= required_bytes)
    error("Expected %d bytes, but got %d", required_bytes, length(encoded_bits))
end

%% OFDM Symbol Creation

% There are two variants I know of right now.  One that uses 9 OFDM symbols (my DJI Mini 2) and others that use 8
% symbols.  To keep life simple, I am using the 8 symbol version as it doesn't require modulating an OFDM symbol that
% doesn't get used anyway.
% TODO(23April2022): Come back to this and support 9 symbols
assert(use_9_symbols == 0, "Currently only support creation of 8 symbols")

% Get some required params
fft_size = get_fft_size(sample_rate);
[long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(sample_rate);

% Define the size of each symbols cyclic prefix
cyclic_prefix_schedule = [
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    short_cp_len, ...
    long_cp_len ...
];

% Number of data carriers per OFDM symbol
data_carrier_count = 600;

% Convert to a matrix of 8 rows so that each symbol has it's own row
encoded_bits = reshape(encoded_bits, [], 6).';

% Create both ZC sequences (these are time domain already, but no cyclic prefix)
zc_1 = create_zc(fft_size, 4);
zc_2 = create_zc(fft_size, 6);


% Create the QPSK samples for each OFDM symbol
symbol_data = zeros(8, data_carrier_count);

encoded_bits_ptr = 1;
for symbol_idx=1:8
    % There's no work to be done for symbols 3 and 5 as they are ZC sequences
    if (symbol_idx == 3)
        zc = create_zc_seq(fft_size, 600);
        zc(data_carrier_count/2) = [];
        symbol_data(symbol_idx,:) = zc;
    elseif (symbol_idx == 5)
        zc = create_zc_seq(fft_size, 147);
        zc(data_carrier_count/2) = [];
        symbol_data(symbol_idx,:) = zc;
    else
        % Convert from bits to QPSK constellation
        symbol_data(symbol_idx,:) = to_qpsk(encoded_bits(encoded_bits_ptr,:));
        encoded_bits_ptr = encoded_bits_ptr + 1;
    end
end

% Create the frequency domain representation of each OFDM symbol
freq_domain_symbols = zeros(8, fft_size);

% Get the carriers that will contain the QPSK samples
data_carrier_indices = get_data_carrier_indices(sample_rate);

figure(1);
for symbol_idx=1:8
    % Create a vector of all zeros to start with
    symbol = zeros(1, fft_size);

    % Map in the QPSK samples to the data carrier indices
    symbol(data_carrier_indices) = symbol_data(symbol_idx,:);

    % Copy the freq domain samples to the buffer
    freq_domain_symbols(symbol_idx,:) = symbol;

    % Debug plotting.  Can be removed
    subplot(3, 3, symbol_idx);
    plot(abs(freq_domain_symbols(symbol_idx,:)).^2);
end

% Create the time domain representation of each OFDM symbol (no cyclic prefix at this point)
time_domain_symbols = zeros(8, fft_size);

figure(2);
for symbol_idx=1:8
    % Add the correct ZC sequence for symbols 3 and 5
    if (symbol_idx == 3)
        time_domain_symbols(symbol_idx,:) = zc_1;
    elseif (symbol_idx == 5)
        time_domain_symbols(symbol_idx,:) = zc_2;
    else
        % 
        time_domain_symbols(symbol_idx,:) = ifft(fftshift(freq_domain_symbols(symbol_idx,:))) / fft_size;
    end

    subplot(3, 3, symbol_idx);
    plot((abs(time_domain_symbols(symbol_idx,:)).^2))
end

total_sample_count = (fft_size * size(time_domain_symbols, 1)) + sum(cyclic_prefix_schedule);
time_domain_samples = zeros(1, total_sample_count);
time_domain_samples_ptr = 1;
for symbol_idx=1:8
    cyclic_prefix_length = cyclic_prefix_schedule(symbol_idx);
    cyclic_prefix = time_domain_symbols(symbol_idx, end-cyclic_prefix_length+1:end);
    time_domain_samples(time_domain_samples_ptr:time_domain_samples_ptr + cyclic_prefix_length - 1) = cyclic_prefix;
    time_domain_samples_ptr = time_domain_samples_ptr + cyclic_prefix_length;
    time_domain_samples(time_domain_samples_ptr:time_domain_samples_ptr + fft_size - 1) = time_domain_symbols(symbol_idx,:);
    time_domain_samples_ptr = time_domain_samples_ptr + fft_size;
end

figure(3);
plot(10 * log10(abs(time_domain_samples).^2))


%% Verification and testing
mod = comm.OFDMModulator();
mod.FFTLength = fft_size;
mod.InsertDCNull = true;
mod.NumGuardBandCarriers = [212; 211];
mod.NumSymbols = 8;
mod.CyclicPrefixLength = cyclic_prefix_schedule;

output = step(mod, reshape(symbol_data, [], 8));
output = awgn([zeros(1000, 1); output; zeros(1000, 1)], 50);
figure(5);
plot(10 * log10(abs(output).^2))

demod = comm.OFDMDemodulator();
demod.FFTLength = fft_size;
demod.NumGuardBandCarriers = [212; 211];
demod.NumSymbols = 8;
demod.RemoveDCCarrier = 1;
demod.CyclicPrefixLength = cyclic_prefix_schedule;

% demod_out = step(demod, output(1000:1000+total_sample_count-1));
demod_out = step(demod, reshape(time_domain_samples, [], 1));

figure(9);
plot(demod_out([1,2,4,6,7,8],:), 'o')
% time_domain_samples = zeros((fft_size * 8)