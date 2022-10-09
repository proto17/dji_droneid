function [sequence] = zadoff_chu(root, length)
    sequence = reshape(exp(-1j * pi * root * (0:length - 1) .* (1:length) / length), [], 1);
end

