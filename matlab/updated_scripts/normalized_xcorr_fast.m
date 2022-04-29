% A cross correlation function that takes some shortcuts to be faster than xcorr(x,y,0,'normalized') with some small
% tradeoffs in accuracy.  Return values are normalized to be between 0 and 1.0 with 1.0 being a perfect match
%
% Will return a vector that is `length(filter)` samples shorter than the input
%
% Correlation peaks point to the *beginning* of the `filter` sequence
%
% @param input_samples Complex row/column vector of samples (must have at least as many samples as `filter`)
% @param filter Complex row/column vector to correlate for in `input_samples`
% @param varargin Variable arguments (see above)
% @return scores Vector of correlation scores as complex values (use `abs(x).^2` to get score in range 0-1.0)
function [scores] = normalized_xcorr_fast(input_samples, filter, varargin)
    assert(isrow(input_samples) || iscolumn(input_samples), "Input samples must be row or column vector");
    assert(isrow(filter) || iscolumn(filter), "Filter must be a row or column vector");
    assert(mod(length(varargin), 2) == 0, "Varargs length must be a multiple of 2");
    
    % Placeholder for any varargs that might be needed in the future
    for idx=1:2:length(varargin)
        key = varargin{idx};
        val = varargin{idx+1};

        switch(key)
            otherwise
                error("Invalid vararg key '%s'", key);
        end
    end
    
    % Create the output vector using the same dimensions as the input samples vector.  Not all samples can be
    % computed, so don't include the last `length(filter)` samples
    dims = size(input_samples);
    if (dims(1) == 1)
        scores = zeros(1, length(input_samples) - length(filter));
    else
        scores = zeros(length(input_samples) - length(filter), 1);
    end

    % Make the filter zero mean
    filter = filter - mean(filter);

    % Will be using dot product, so the conjugate is needed
    filter_conj = conj(filter);

    % Pre-calculate the variance, and the square root of the variance
    filter_conj_var = var(filter_conj);
    filter_conj_var_sqrt = sqrt(filter_conj_var);

    % Pre-calculate and convert to multiplication, the divisions that will need to happen later
    window_size = length(filter);
    recip_window_size = 1 / window_size;
    recip_window_size_minus_one = 1 / (window_size - 1);
    
    % To prevent needing an if statement in the critical path, start the running sum with the first element
    % missing, and the value being removed first in the loop set to 0.  This means that on startup, the loop
    % will work properly without needing a conditional
    temp_window = input_samples(2:window_size - 1);
    running_sum = sum(temp_window);
    prev_val = 0;

    % The same trick above is applied to the 
    running_abs_sqrd = sum(real(temp_window).^2 + imag(temp_window).^2);
    running_abs_sqd_prev = 0;

    for idx=1:length(scores)
        % Get the next `window_size` samples starting at the current offset
        window = input_samples(idx:idx + window_size - 1);

        % Since the window is shifting to the right, subtract off the left-most value that was just removed and add
        % on the new value on the right
        running_sum = running_sum - prev_val + window(end);
        
        % The value that will be removed on the next iteration is the left-most value of the current window
        prev_val = window(1);

        % Make the window zero mean by subtracting the average power 
        window = window - (running_sum * recip_window_size);

        % Compute the dot product
        prod = sum(window .* filter_conj) * recip_window_size;
        
        % Compute the running abs(window).^2 estimate by removing the previous left-most value, and adding on the new
        % right-most value.  Then make the new left-most value the prev value for the next iteration
        running_abs_sqrd = running_abs_sqrd - running_abs_sqd_prev + real(window(end)).^2 + imag(window(end)).^2;
        running_abs_sqd_prev = real(window(1)).^2 + imag(window(1)).^2;

        % Get the variance of the window
        variance = running_abs_sqrd * recip_window_size_minus_one;

        % Divide the dot product result by the square root of the std deviation of both windows combined
        scores(idx) = prod / (sqrt(variance) * filter_conj_var_sqrt);
    end

end

