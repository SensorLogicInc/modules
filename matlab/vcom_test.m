%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% vcom_test.m
%
% This is a demonstration how to use the class *vcom_xep_radar_connector*
%
% Copyright: 2020 Sensor Logic
% Written by: Justin Hadella
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
clear;
clc;

r = vcom_xep_radar_connector('COM4'); % adjust for *your* COM port!
r.Open('X4');

% As a side-effect many settings on write will cause the numSamplers
% variable to update
fprintf('bins = %d\n', r.numSamplers);

% Actually every variable from the radar is requested in this manner.
iterations = r.Item('iterations');
fprintf('iterations = %d\n', iterations);

% Setting some variables
r.TryUpdateChip('rx_wait', 0);
r.TryUpdateChip('frame_start', 0);
r.TryUpdateChip('frame_end', 4.0);
r.TryUpdateChip('ddc_en', 1);

% Set up time plot signal
frameSize = r.numSamplers;   % Get # bins/samplers in a frame
frame = zeros(1, frameSize); % Preallocate frame
h_fig = figure;
ax1 = gca;
h1 = plot(ax1, 1:frameSize, frame);
title(ax1, 'radar time waveform');
xlabel(ax1, 'bin');
ylabel(ax1, 'amplitude');

% Plot data while window is open
while isgraphics(h_fig)
    try
%         frame = abs(r.GetFrameRawDouble);
        frame = abs(r.GetFrameNormalizedDouble);
        
        set(h1, 'xdata', 1:frameSize, 'ydata', frame);
        drawnow;
    catch ME
    end
end

r.Close();
