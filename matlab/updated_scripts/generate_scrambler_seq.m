% Generates a scrambler sequence for an arbitrary number of bits given an initial state value
%
% The initial value should always be the 31 bit sequence 0x12345678 *bit reversed*
%
% @param num_bits Number of bits to generate
% @param x2_init Initial value of the second LFSR.  Must be a row vector and only contain 0/1 values
% @return bits Row vector of `num_bits` 1/0 values
function [bits] = generate_scrambler_seq(num_bits, x2_init)
    assert(isrow(x2_init), "X2 initial value must be a row vector");
    assert(length(x2_init) == 31, "The X2 initial value must be 31 bits");
    mustBeMember(x2_init, [0, 1]);

    % https://www.sharetechnote.com/html/Handbook_LTE_PseudoRandomSequence.html
    % https://edadocs.software.keysight.com/pages/viewpage.action?pageId=6076479
    % Initial condition of the polynomia x1(). This is fixed value as described in 36.211 7.2
    x1_init = [1 0 0 0 0 0 0 0 0 0 ...
               0 0 0 0 0 0 0 0 0 0 ...
               0 0 0 0 0 0 0 0 0 0 ...
               0];
   
    
    % Mpn is the length of the final sequence c()
    Mpn = num_bits;
    
    % Nc as defined in 36.211 7.2
    Nc = 1600;
    
    % Create a vector(array) for x1() and x2() all initialized with 0
    x1 = zeros(1,Nc + Mpn + 31);
    x2 = zeros(1,Nc + Mpn + 31);
    
    % Create a vector(array) for c() all initialized with 0
    c = zeros(1,Mpn);
    
    % Initialize x1() and x2()
    x1(1:31) = x1_init;
    x2(1:31) = x2_init;
    
    % generate the m-sequence : x1()
    for n = 1 : (Mpn+Nc)
       x1(n+31) = mod(x1(n+3) + x1(n), 2);
    end
    
    
    % generate the m-sequence : x2()
    for n = 1 : (Mpn+Nc)
       x2(n+31) = mod(x2(n+3) + x2(n+2) + x2(n+1) + x2(n),2);
    end
    
    % generate the resulting sequence (Gold Sequence) : c()
    for n = 1 : Mpn
        c(n)= mod(x1(n+Nc) + x2(n+Nc),2);
    end
    
    bits = c;
end
