% This script only works for MATLAB right now.  Octave doesn't like something around the filtering section :(

%% Signal parameters
% file_path = '/opt/dji/collects/2437MHz_30.72MSPS.fc32';
% file_sample_rate = 30.72e6;     % Collected 2x oversampled
% rough_frequency_offset = 7.5e6; % The collected signal is 7.5 MHz off center
% correlation_threshold = 0.7;    % Minimum correlation score to accept (0.0 - 1.0)


function [zc_indices] = find_zc_indices_by_file(file_path, sample_rate, frequency_offset, correlation_threshold, chunk_size)
    %% LTE parameters
    carrier_spacing = 15e3;
    
    % Pre-calculate the frequency offset rotation
    freq_offset_constant = 1j * pi * 2 * (frequency_offset / sample_rate);
    
    % The output of the ZC sequence generator needs to be conjugated to be used in a filter.  Also create a variable that
    % can hold the filter state between chunks to prevent discontinuities
    correlator_taps = conj(create_zc(sample_rate / carrier_spacing, 4));
    correlator_state = [];
    
    % Open the IQ recording
    file_handle = fopen(file_path, 'r');
    
    % Figure out how many samples there are in the file
    fseek(file_handle, 0, 'eof');
    total_samples = ftell(file_handle) / 4 / 2; % 4 bytes per float, 2 floats per complex sample
    fseek(file_handle, 0, 'bof');
    fprintf('There are %d samples in "%s"\n', total_samples, file_path);
    
    % Really large array to store the cross correlation results from *all* samples
    zc_scores = zeros(total_samples - length(correlator_taps), 1);
    
    sample_offset = 0;
    while (~ feof(file_handle))    
        %% Read in the next buffer
        % The `fread` command will return interleaved real, imag values, so pack those into complex samples
        floats = fread(file_handle, chunk_size * 2, 'float');
        samples = floats(1:2:end) + 1j * floats(2:2:end);
        
        %% Frequency shift the input
        % This is somewhat optional, but the correlation scores will go down fast if the offset is > 1 MHz
        rotation_vector = exp(freq_offset_constant * (sample_offset:(sample_offset+length(samples)-1)));
        samples = samples .* reshape(rotation_vector, [], 1);
        
        %% Correlate for the ZC sequence
        % Use a FIR filter to search for the ZC sequence.  The resulting values will be normalized to between 0 and 1.0 if
        % the abs^2 is taken
        % TODO(9April2022): Would be nice to use the fftfilt function, but I don't know if it has issues with keeping state
        %                   as it doesn't have a state input like the filter command.  In testing the FFT filter is twice 
        %                   as fast
        [correlation_values, correlator_state] = filter(correlator_taps, 1, samples, correlator_state);
        zc_scores(sample_offset+1:sample_offset+length(correlation_values)) = correlation_values;
        
        sample_offset = sample_offset + length(samples);
    end
    
    % Get the floating normalized correlation results
    abs_scores = abs(zc_scores).^2;
    
%     figure(1); 
%     plot(abs_scores);
%     title('Correlation Scores (normalized)')
    
    % Find all places where the correlation result meets the specified threshold
    % This is going to find duplicates because there are very likely going to be two points right next to each other that
    % meet the required threshold.  This will be dealt with later
    passing_scores = find(abs_scores > correlation_threshold);
    
    % Look through each element of the `passing_scores` vector (which is just indicies where the correlation threshold was
    % met) and pick just the highest value `search_window` elements around (`search_window/2` to the left and right) of each
    % value.  The goal here is to only end up with the best score for the starting point of each burst instead of having
    % multiple starting points for each burst.
    true_peaks = [];
    search_window = 100;
    for idx = 1:length(passing_scores)
        % Calculate how far to the left and right to look for the highest peak
        left_idx = passing_scores(idx) - (search_window / 2);
        right_idx = left_idx + search_window - 1;

        if (left_idx < 1 || right_idx > length(abs_scores))
            warning("Had to abandon searching for burst '%d' as it was too close to the end/beginning of the window", idx);
            continue
        end

        % Get the correlation scores for the samples around the current point
        window = abs_scores(left_idx:right_idx);
    
        % Find the peak in the window and use that value as the actual peak
        [value, index] = max(window);
        true_peaks = [true_peaks, left_idx + index];
    end
    
    % There are going to be duplicates in the vector, so just take the unique elements.  What's left should just be the
    % actual starting indices for each ZC sequence.
    zc_indices = unique(true_peaks);
end
