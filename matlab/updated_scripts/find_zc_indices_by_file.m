% Find all instances of the first ZC sequence in the provided file.  Does not have to read the entire file in at once
%
% Uses a cross correlator to search for the first ZC sequence in DroneID.
%
% !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
% !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
% !!!  There is currently a bug in this script!  The correlation results are NOT normalized and might require the    !!!
% !!!  the user to greatly increase or decrease the correlation threshold past the expected range of 0.0 - 1.0       !!!
% !!!  In the code below there is a plotting function that is disabled by default.  Enabling it will plot the        !!!
% !!!  correlation results so that you can better set the correlation treshold.                                      !!!
% !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
% !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
    
    % The output of the ZC sequence generator needs to be conjugated to be used in a filter.  Also create a variable that
    % can hold the filter state between chunks to prevent discontinuities
    correlator_taps = conj(create_zc(fft_size, 4));
    correlator_state = [];
    
    % Figure out how many samples there are in the file
    total_samples = get_sample_count_of_file(file_path, sample_type);
    fprintf('There are %d samples in "%s"\n', total_samples, file_path);
    
    % Open the IQ recording
    file_handle = fopen(file_path, 'r');

    % Really large array to store the cross correlation results from *all* samples
    zc_scores = zeros(total_samples - length(correlator_taps), 1);
    
    sample_offset = 0;
    while (~ feof(file_handle))    
        %% Read in the next buffer
        % The `fread` command will return interleaved real, imag values, so pack those into complex samples making sure
        % that the resulting complex values are double precision (this is to prevent functions from complaining later)
        real_values = fread(file_handle, chunk_size * 2, sample_type);
        samples = double(real_values(1:2:end) + 1j * real_values(2:2:end));
        
        %% Frequency shift the input
        % This is somewhat optional, but the correlation scores will go down fast if the offset is > 1 MHz
        rotation_vector = exp(freq_offset_constant * (sample_offset:(sample_offset+length(samples)-1)));
        samples = samples .* reshape(rotation_vector, [], 1);
        
        %% Correlate for the ZC sequence
        % Use a FIR filter to search for the ZC sequence.  THIS WILL NOT NORMALIZE!!!
        % TODO(9April2022): Would be nice to use the fftfilt function, but I don't know if it has issues with keeping state
        %                   as it doesn't have a state input like the filter command.  In testing the FFT filter is twice 
        %                   as fast
        [correlation_values, correlator_state] = filter(correlator_taps, 1, samples, correlator_state);
        zc_scores(sample_offset+1:sample_offset+length(correlation_values)) = correlation_values;
        
        sample_offset = sample_offset + length(samples);
    end
    
    % Get the floating normalized correlation results
    abs_scores = abs(zc_scores).^2;
    
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
