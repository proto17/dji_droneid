% Normalize the input samples such that the max I or Q value is within -1.0 to 1.0 (inclusive)
%
% @param samples Input samples as a row or column vector
% @param backoff Multiplier that is applied to the scalar value to bring the amplitudes down.  Must be between 0 and
%                1.0.  Primary use is to back off of the power before transmitting to prevent overdriving the amplifier
% @return scaled_samples Normalized samples
% @return scalar Scalar that was used to scale the samples (includes the backoff)
function [scaled_samples, scalar] = normalize(samples, backoff)
    assert(isrow(samples) || iscolumn(samples), "Input samples must be a row or column vector");
    assert(~isempty(samples), "Input samples vector was empty");
    assert(isnumeric(backoff), "Backoff must be a number");
    assert(backoff >= 0.0 && backoff <= 1.0, "Backoff value must be between 0.0 and 1.0 (inclusive)");

    scalar = (1 / max(abs(samples))) * backoff;

    scaled_samples = cast(samples .* scalar, class(samples));
end

