% Calculate channel taps based on the 4th or 6th OFDM symbol (both are ZC sequences)
%
% There is almost certainly a better way to do this, but this gets the job done
%
% @param zc_seq Frequency domain ZC sequence from OFDM symbol number 4 or 6 (all FFT bins, no cyclic prefix)
% @param sample_rate Sample rate (in Hz) of `zc_seq`
% @param symbol_idx Which symbol (must be 4 or 6) is in the `zc_seq` vector
% @return taps Result of dividing `zc_seq` into a golden reference copy of the selected ZC sequence
function [taps] = calculate_channel(zc_seq, sample_rate, symbol_idx)
    assert(symbol_idx == 4 || symbol_idx == 6, "Symbol index must be 4 or 6");
    fft_size = get_fft_size(sample_rate);
    
    % The golden reference needs to be in the frequency domain
    gold_seq = fftshift(fft(reshape(create_zc(fft_size, symbol_idx), size(zc_seq))));
    
%     figure(400);
%     subplot(1, 2, 1);
%     plot(abs(gold_seq).^2)
%     subplot(1, 2, 2);
%     plot(abs(zc_seq).^2)

    taps = gold_seq ./ zc_seq;
end