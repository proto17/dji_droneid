%% Computes the normalized cross correlation of two vectors
% Used https://www.researchgate.net/post/How-can-one-calculate-normalized-cross-correlation-between-two-arrays
% as the reference for this implementation
function [score] = normalized_xcorr(window_one, window_two)
    assert(length(window_one) == length(window_two), "Windows must be equal length");
    assert(isrow(window_one) || iscolumn(window_one), "Windows must be row/column vectors");
    assert(isrow(window_two) || iscolumn(window_two), "Windows must be row/column vectors");

    % Make both windows zero mean
    window_one = window_one - mean(window_one);
    window_two = window_two - mean(window_two);
    
    % Cross correlate and get the average
    xcorr_value = sum(window_one .* conj(window_two)) / length(window_one);

    % Final step in normalization
    score = xcorr_value / sqrt(var(window_one) * var(window_two));
end

