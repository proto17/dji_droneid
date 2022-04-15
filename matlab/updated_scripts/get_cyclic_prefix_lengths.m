% Get the size of the long and short cyclic prefixes based on the sample rate
%
% @param sample_rate Sample rate (in Hz)
% @return long_cp_len Long cyclic prefix length (in samples)
% @return short_cp_len Short cyclic prefix length (in samples)
function [long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(sample_rate)
    long_cp_len = round(1/192000 * sample_rate);
    short_cp_len = round(0.0000046875 * sample_rate);
end

