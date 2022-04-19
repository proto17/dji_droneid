% Replacement for the mustBeMember function that's in MATLAB, but not Octave
%
% Throws an assertion failure if `vector` contains any values not in `valid_values`
%
% @param vector Vector/Matrix of values to check against the list of valid values
% @param valid_values List of value values that can be in any one of the elements of `vector`
function [] = must_be_member(vector, valid_values)
    assert(isequal(ismember(vector, valid_values), ones(size(vector))));
end
