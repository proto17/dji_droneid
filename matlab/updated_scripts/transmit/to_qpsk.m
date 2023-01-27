function [samples] = to_qpsk(bitvec)
    assert(iscolumn(bitvec) || isrow(bitvec), "Bit vector must be a row or column vector");
    assert(mod(length(bitvec), 2) == 0, "Length of bit vector must be an even number")

    % Keep the power of each point at 1.0
    amplitude = 1/sqrt(2);
    
    samples = zeros(length(bitvec) / 2, 1);

    for idx=1:2:length(bitvec)
        bits = bitvec(idx:idx+1);

        if (bits == [0, 0])
            sample = amplitude + amplitude * 1j;
        elseif (bits == [0 1])
            sample = amplitude - amplitude * 1j;
        elseif (bits == [1 0])
            sample = -amplitude + amplitude * 1j;
        else
            sample = -amplitude - amplitude * 1j;
        end

        samples((idx + 1) / 2) = sample;
    end
end

