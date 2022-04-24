% Gets the number of seconds since Jan 1 1970 (epoch)
%
% @return seconds Number of seconds since epoch
function [seconds] = get_seconds_since_epoch()
    if (is_octave())
        seconds = uint64(time());
    else
        seconds = uint64(convertTo(datetime('now', 'TimeZone', 'UTC'), 'epochtime'));
    end
end

