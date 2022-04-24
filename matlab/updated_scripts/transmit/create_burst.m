sample_rate = 15.36e6;
show_debug_plots = 0;

%% Build payload
% Example: 
%   create_frame_bytes('SequenceNumber', uint16(1234), ...
%                      'SerialNumber', '!this is a test!');
frame_bytes = create_frame_bytes('SerialNumber', '!this is a test!');

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

%% Scrambler Application

% Convert to a matrix of 8 rows so that each symbol has it's own row
encoded_bits = reshape(encoded_bits, [], 6).';

% Initial value for the second LFSR in the scrambler
scrambler_x2_init = fliplr([0 0 1, 0 0 1 0, 0 0 1 1, 0 1 0 0, 0 1 0 1, 0 1 1 0, 0 1 1 1, 1 0 0 0]);

% Create scrambler for all data bits and reshape so that each OFDM symbol has a row
scrambler = uint8(reshape(generate_scrambler_seq(7200, scrambler_x2_init), [], 6).');

encoded_bits = [scrambler(1,:); encoded_bits];

% Apply the scrambler to each OFDM symbol's data bits
for idx=1:6
    encoded_bits(idx+1,:) = bitxor(encoded_bits(idx+1,:), scrambler(idx,:));
end


%% OFDM Symbol Creation Setup

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
    long_cp_len, ...
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

%% Bits to Symbol Mapping

% Create the QPSK samples for each OFDM symbol
symbol_data = zeros(9, data_carrier_count);

encoded_bits_ptr = 1;
for symbol_idx=1:9
    % There's no work to be done for symbols 4 and 6 as they are ZC sequences
    if (symbol_idx == 4)
    elseif (symbol_idx == 6)
    else
        % Convert from bits to QPSK constellation
        symbol_data(symbol_idx,:) = to_qpsk(encoded_bits(encoded_bits_ptr,:));
        encoded_bits_ptr = encoded_bits_ptr + 1;
    end

    if (show_debug_plots)
        figure(1);
        subplot(3, 3, symbol_idx);
        plot(symbol_data(symbol_idx,:), 'o');
    end
end

%% Frequency Domain Symbol Creation
% Create the frequency domain representation of each OFDM symbol
freq_domain_symbols = zeros(9, fft_size);

% Get the carriers that will contain the QPSK samples
data_carrier_indices = get_data_carrier_indices(sample_rate);

for symbol_idx=1:9
    % Copy the freq domain samples to the buffer
    freq_domain_symbols(symbol_idx,data_carrier_indices) = symbol_data(symbol_idx,:);

    if (show_debug_plots)
        figure(2);
        subplot(3, 3, symbol_idx);
        plot(abs(freq_domain_symbols(symbol_idx,:)).^2);
    
        figure(3);
        subplot(3, 3, symbol_idx);
        plot(freq_domain_symbols(symbol_idx,:), 'o');
    end
end

%% Time Domain Symbol Creation
% Create the time domain representation of each OFDM symbol (no cyclic prefix at this point)
time_domain_symbols = zeros(9, fft_size);

for symbol_idx=1:9
    time_domain_symbols(symbol_idx,:) = ifft(fftshift(freq_domain_symbols(symbol_idx,:)));
    
    if (show_debug_plots)
        figure(4);
        subplot(3, 3, symbol_idx);
        plot(10 * log10(abs(time_domain_symbols(symbol_idx,:)).^2))
    end
end

time_domain_symbols(4,:) = create_zc(fft_size, 4);
time_domain_symbols(6,:) = create_zc(fft_size, 6);

time_domain = [];
for symbol_idx=1:9
    cp_len = cyclic_prefix_schedule(symbol_idx);
    symbol = time_domain_symbols(symbol_idx,:);
    cp = symbol(end-cp_len+1:end);
    time_domain = [time_domain, cp, symbol];
end

time_domain = awgn([zeros(1, 10000), time_domain, zeros(1, 10000)], 60);

write_complex_floats('/tmp/burst.fc32', time_domain);
if (show_debug_plots)
    figure(7);
    plot(10 * log10(abs(time_domain).^2));
    
    filter_out = filter(conj(create_zc(fft_size, 4)), 1, time_domain);
    figure(8);
    plot(abs(filter_out).^2);
end