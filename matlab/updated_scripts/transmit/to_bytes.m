function [byte_array] = to_bytes(value, byte_count)
    byte_array = zeros(1, byte_count);
    
    % Octave doesn't like negative values being used with dec2bin.  So, for negative values typecast them to their
    % unsigned counterpart.
    % https://www.mathworks.com/matlabcentral/answers/98649-how-can-i-convert-a-negative-integer-to-a-binary-string-in-other-words-how-can-i-find-two-s-comple
    if (value < 0)
        value = typecast(value, ['u' class(value)]);
    end
    
    bits = dec2bin(value, byte_count * 8);
    
    for idx=1:byte_count
        bits = circshift(bits, 8);
        byte_array(idx) = uint8(bin2dec(bits(1:8)));
    end
end

