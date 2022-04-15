% Create a plot containing the `10*log10(abs(fftshift(fft(samples))).^2))` power spectrum with the X axis in Hz
%
% @param samples Input complex samples (must be row/column vector)
function [figure_handle] = plot_fft(samples, figure_number, title, sample_rate)
    assert(isrow(samples) || iscolumn(samples), "Samples must be a row/column vector");

    figure_handle = figure(figure_number);

    sample_count = length(samples);
    x_axis_values = sample_rate * (-sample_count/2:sample_count/2-1)/sample_count; 
    plot(x_axis_values, 10 * log10(abs(fftshift(fft(samples))).^2));
    set(gcf, 'Name', title);
end

