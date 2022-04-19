% Used to check if the script is running in Octave or MATLAB
%
% Logic is from https://stackoverflow.com/questions/2246579/how-do-i-detect-if-im-running-matlab-or-octave
%
% @return val True if the current script is running inside Octave, false if MATLAB
function [val] = is_octave()
  val = exist('OCTAVE_VERSION', 'builtin') ~= 0;
end
