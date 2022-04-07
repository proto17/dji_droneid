function [] = plot_symbol_boundaries(samples, sample_rate, figure_num)
    long_cp_len = round(1/192000 * sample_rate);
    short_cp_len = round(0.0000046875 * sample_rate);
    fft_size = sample_rate / 15e3;

    % Plot the time domain
    figure(figure_num);
    plot(10 * log10(abs(samples)));
    
    % Tranceparency of each box (so the waveform is still visible)
    face_opacity = 0.25;
    
    % Don't show the lines between the boxes.  Just gets in the way
    edge_color = [0, 0, 0, 0];
    
    % Define the box colors (with transparency)
    face_color_red   = [1, 0, 0];
    face_color_green = [0, 1, 0];
    
    % The Y axis of the plot is in dB, so draw a box that starts at -30 dB and
    % goes up by 25 dB (-5 dB).  These numbers just happened to work out with
    % the test data and will likely change for other collects
    box_y_start = -30;
    box_y_height = 25;
    
    for symbol_idx=0:8
        if (mod(symbol_idx, 2) == 0)
            color = face_color_red;
        else
            color = face_color_green;
        end
    
        relative_offset = symbol_idx * (fft_size + short_cp_len);
        r = rectangle('Position', [1 + relative_offset, box_y_start, fft_size + short_cp_len, box_y_height]);

        if (exist('OCTAVE_VERSION', 'builtin'))
            set(r, "FaceColor", color);
            set(get(r, 'children'), "facealpha", face_opacity);
        else
            set(r, "FaceColor", [color, face_opacity]);
        end
    end

    title('OFDM Symbol Boundaries')
end