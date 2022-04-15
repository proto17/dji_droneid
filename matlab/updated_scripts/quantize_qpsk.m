% Take in a vector of complex samples representing individual QPSK constellation points with the constellation rotated
% such that the points are ideally at 1+i, -1+i, -1-i, and 1-i.
%
% This function uses hard decision, so it's not what you want to use in low SNR environments
%
% The constellation mapping is:
%     1+i == 0b00
%     1-i == 0b01
%    -1+i == 0b10
%    -1-i == 0b11
%
% Which comes from https://github.com/ttsou/openphy/blob/master/src/lte/qam.c#L35
%
% @param data_carriers Row or column vector of complex samples
% @return quantized_bits Vector of 1/0 values that make up the bits demapped from the provided sample vector
function [quantized_bits] = quantize_qpsk(data_carriers)
    assert(iscolumn(data_carriers) || isrow(data_carriers), "Data carriers must be row/column vector");
    
    quantized_bits = zeros(length(data_carriers), 1);

    % Track where in the `quantized_bits` vector the new bits should be placed
    bits_offset = 1;
    
    % Walk through each complex sample in the input vector
    for sample_idx = 1:length(data_carriers)
        sample = data_carriers(sample_idx);

        % Determine bit mapping based on the quadrant that the sample is located
        if (real(sample) > 0 && imag(sample) > 0)
            bits = [0, 0];
        elseif (real(sample) > 0 && imag(sample) < 0)
            bits = [0, 1];
        elseif (real(sample) < 0 && imag(sample) > 0)
            bits = [1, 0];
        elseif (real(sample) < 0 && imag(sample) < 0)
            bits = [1, 1];
        else
            error("Invalid coordinate!");
        end
        
        % Save off the quatized bits and move the counter ahead by 2
        quantized_bits(bits_offset:bits_offset+1) = bits;
        bits_offset = bits_offset + 2;
    end
end

