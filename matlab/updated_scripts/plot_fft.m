function [figure_handle] = plot_fft(samples, figure_number, title)
    figure_handle = figure(figure_number);
    plot(10 * log10(abs(fftshift(fft(samples))).^2));
    set(gcf, 'Name', title);
end

