function [] = write_complex_floats(output_path, samples)
    handle = fopen(output_path, 'w');
    fwrite(handle, reshape([real(samples); imag(samples)], 1, []), 'single');
    fclose(handle);
end

