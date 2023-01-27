% Find all instances of the first ZC sequence in the provided file.  Does not have to read the entire file in at once
%
% Uses a normalized cross correlator to search for the first ZC sequence in DroneID.
%
% Varags:
%   - SampleType: MATLAB numeric type that the samples in the provided file are stored as (ex: 'single', 'int16', etc)
%                 Defaults to 'single'
%   - CorrelationFigNum: Figure number to use when plotting the correlation results.  Defaults to not showing the
%                        correlation results at all.  Must be > 0 to show the plot, or -1 to disable
%
% @param file_path Path to the complex IQ file to be processed
% @param sample_rate Sample rate that the input file was recorded at
% @param frequency_offset Amount of offset (in Hz, positive or negative) that the recording needs to be shifted by
%                         before processing the samples.  This is useful when the recording was taken at an offset to
%                         remove the DC spike
% @param correlation_threshold Minimum correlation magnitude required to classify a score as containing a valid ZC
%                              sequence.  Should be between 0.0 and 1.0.  Frequency offset (not accounted for by the
%                              `frequency_offset` parameter) and low SNR can cause this value to need to be set lower
%                              than normal.  A good starting value is 0.5
% @param chunk_size Number of complex IQ samples to read into memory at a time.  Set this value as high as you can
%                   without running out of memory.  The larger this buffer the faster the file will be processed
% @param varargin (see above)
% @return zc_indices A row vector containing the samples offsets where correlation scores were seen at or above the
%                    specified threshold
function [zc_indices] = find_zc_indices_by_file(file_path, sample_rate, frequency_offset, correlation_threshold, ...
    chunk_size, varargin)

    assert(isstring(file_path) || ischar(file_path), "Input file path must be a string or char array");
    assert(isnumeric(sample_rate), "Sample rate must be numeric");
    assert(sample_rate > 0, "Sample rate must be > 0")
    assert(isnumeric(frequency_offset), "Frequency offset must be numeric");
    assert(isnumeric(correlation_threshold), "Correlation threshold must be numeric");
    assert(isnumeric(chunk_size), "Chunk size must be numeric");
    assert(chunk_size > 0, "Chunk size must be > 0");
    assert(mod(length(varargin), 2) == 0, "Varargs length must be a multiple of 2");
    
    sample_type = 'single';
    correlation_fig_num = -1;

    for idx=1:2:length(varargin)
        key = varargin{idx};
        val = varargin{idx+1};

        switch (key)
            case 'SampleType'
                sample_type = val;
            case 'CorrelationFigNum'
                correlation_fig_num = val;
            otherwise
                error('Invalid varargs key "%s"', key);
        end
    end

    assert(ischar(sample_type) || isstring(sample_type), "SampleType must be a string or char array");
    assert(isnumeric(correlation_fig_num), "CorrelationFigNum must be numeric");
    assert(correlation_fig_num == -1 || correlation_fig_num > 0, "CorrelationFigNum must be -1, or > 0");

    %% LTE parameters
    fft_size = get_fft_size(sample_rate);
    
    % Pre-calculate the frequency offset rotation
    freq_offset_constant = 1j * pi * 2 * (frequency_offset / sample_rate);
    
    % The correlator will be searching for the first ZC sequence (which resides in the 4th/3rd OFDM symbol depending on
    % which drone model is in use.)  No need to conjugate here as that will be done in the correlator
    correlator_taps = create_zc(fft_size, 4);
    
    % Figure out how many samples there are in the file
    total_samples = get_sample_count_of_file(file_path, sample_type);
    fprintf('There are %d samples in "%s"\n', total_samples, file_path);
    
    % Open the IQ recording
    file_handle = fopen(file_path, 'r');

    % Really large array to store the cross correlation results from *all* samples
    zc_scores = zeros(total_samples - length(correlator_taps), 1);

    % Default to no leftover samples for the first iteration
    leftover_samples = [];
    
    sample_offset = 0;
    zc_scores_ptr = 1;
    while (~ feof(file_handle))    
        %% Read in the next buffer
        % The `fread` command will return interleaved real, imag values, so pack those into complex samples making sure
        % that the resulting complex values are double precision (this is to prevent functions from complaining later)
        real_values = double(fread(file_handle, chunk_size * 2, sample_type));

        % Don't continue processing if there aren't enough samples remaining keeping in mind that there might be some
        % samples leftover from the last iteration of the loop
        if ((length(real_values) / 2) + leftover_samples <= length(correlator_taps))
            break;
        end

        % Convert from a vector of reals (I,Q,I,Q,I,Q,...) to complex
        samples = real_values(1:2:end) + 1j * real_values(2:2:end);
        
        %% Frequency shift the input
        % This is somewhat optional, but the correlation scores will go down fast if the offset is > 1 MHz
        rotation_vector = exp(freq_offset_constant * (sample_offset:(sample_offset+length(samples)-1)));
        samples = samples .* reshape(rotation_vector, [], 1);

        %% Handle unprocessed samples from last iteration

        % The correlation function cannot process all samples as it looks forward in time.  So, after each iteration of
        % this loop some samples don't get processed, and those samples are added to the `leftover_samples` vector.
        % Those samples need to be added to the beginning of the vector of samples that were read from the file.
        % Additionally, the leftover samples have already been frequency corrected, so they must be ignored when
        % applying frequency correction (done above).
        % TODO(30April2022): This isn't ideal as resizing the sample vector each time is not good for performance
        samples = [leftover_samples; samples];

        % The end of the current sample vector will be the new leftover samples for the next iteration
        leftover_samples = samples(end - length(correlator_taps) + 1:end);
        
        %% Correlate for the ZC sequence
        % Run a normalized cross correlation
        correlation_values = normalized_xcorr_fast(samples, correlator_taps);
        
        % Not all samples in the `samples` vector were processed by the correlator, so only update the samples that were
        % processed by using the `zc_scores_ptr` which points to where in the `zc_scores` the next values should go.
        zc_scores(zc_scores_ptr:zc_scores_ptr+length(correlation_values)-1) = correlation_values;
        zc_scores_ptr = zc_scores_ptr + length(correlation_values);
        
        % Move the sample counter forward so that the frequency offset adjustment logic works properly
        sample_offset = sample_offset + length(samples);
    end
    
    % Get the floating normalized correlation results
    abs_scores = abs(zc_scores).^2;
    
    % Plot if requested
    if (correlation_fig_num > 0)
        figure(correlation_fig_num); 
        plot(abs_scores);
        title('Correlation Scores (normalized)')
    end
    
    % Find all places where the correlation result meets the specified threshold
    % This is going to find duplicates because there are very likely going to be two points right next to each other that
    % meet the required threshold.  This will be dealt with later
    passing_scores = find(abs_scores > correlation_threshold);
    
    % Look through each element of the `passing_scores` vector (which is just indicies where the correlation threshold was
    % met) and pick just the highest value `search_window` elements around (`search_window/2` to the left and right) of each
    % value.  The goal here is to only end up with the best score for the starting point of each burst instead of having
    % multiple starting points for each burst.
    true_peaks = [];
    search_window = 10;
    
    for idx = 1:length(passing_scores)
        % Calculate how far to the left and right to look for the highest peak
        left_idx = passing_scores(idx) - (search_window / 2);
        right_idx = left_idx + search_window - 1;
        
        % Move to the next index if there aren't enough samples to find the max peak
        if (left_idx < 1 || right_idx > length(abs_scores))
            warning("Had to abandon searching for burst '%d' as it was too close to the end/beginning of the window", idx);
            continue
        end

        % Get the correlation scores for the samples around the current point
        window = abs_scores(left_idx:right_idx);
    
        % Find the index of the peak in the window and use that value as the actual peak
        [~, index] = max(window);
        true_peaks = [true_peaks, left_idx + index];
    end
    
    % There are going to be duplicates in the vector, so just take the unique elements.  What's left should just be the
    % actual starting indices for each ZC sequence.
    zc_indices = unique(true_peaks);
end
