% Applies a frequency shift to time domain samples
%
% @param samples Input samples (must be row or column vector)
% @param sample_rate Sample rate (in Hz) that the frequency offset should be relative to
% @param freq_offset Frequency offset (in Hz) to apply
% @return samples Frequency shifted time domain samples
function [samples] = apply_freq_offset_time_domain(samples, sample_rate, freq_offset)
    assert(isrow(samples) || iscolumn(samples), "Samples must be a row or column vector");

    if (isrow(samples))
        samples = samples .* exp(1j * 2 * pi * freq_offset / sample_rate * [0:length(samples)-1]);
    else
        samples = samples .* exp(1j * 2 * pi * freq_offset / sample_rate * [0:length(samples)-1].');
    end
end

