%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% timer_test.m
%
% This is a demonstration how to use the class *vcom_xep_radar_connector*
% to simulate fixed radar framerate using Matlab timer.
%
% Copyright: 2020 Sensor Logic
% Written by: Justin Hadella
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
close all;
clear;
clc;

% Open com port / X4 radar
com_port = 'COM106'; % adjust for *your* COM port!
r = vcom_xep_radar_connector(com_port);
r.Open('X4');

fprintf('Connector version = %s\n', r.ConnectorVersion());

% Setting some variables
r.TryUpdateChip('pps', 26);
r.TryUpdateChip('dac_min', 949);
r.TryUpdateChip('dac_max', 1100);
r.TryUpdateChip('frame_start', 0.2);
r.TryUpdateChip('frame_end', 2.0);

% Only one of these should be uncommented...
r.TryUpdateChip('ddc_en', 0); % RF data
% r.TryUpdateChip('ddc_en', 1); % BB data

frame_start = r.Item('frame_start');
frame_end = r.Item('frame_end');
fprintf('Frame area is %f to %f\n', frame_start, frame_end);

% With these settings, just running the radar as fast as possible  results
% in ~ 60 fps.

% Lets time 1000 frames
% tic;
% for i = 1:1000
%     r.GetFrameRawDouble;
% end
% elapsed = toc;
% fprintf('measure fps = %f (no plotting)\n', 1000 / elapsed);

% The current version of the vcom usb firmware on the SLMX4 doesn't support
% data streaming. To simulate this on a PC, we can use a Matlab timer.

% Note: An earlier version attempted to manually govern the fps using a
% tic/toc mechanism in a loop. This method was not consistent across
% different frame rates!

% Note: If we ask for a framerate higher than is possible, the measured fps
% will be similar to the measured fps in the earlier block. For example,
% with the settings provided ~ 60 fps is the highest possible. If we were
% to set the target fps to 100, then the measured value would be ~ 60, not
% 100!
target_fps = 40; % we would typically set this to something lower than max

% Set up the timer's UserData struct (input and output)
user_data.rhandle = r; % The radar handle (input)
user_data.fdata = [];  % frame data (output)
% Add more fields if needed...

% Setup a timer
t = timer;
t.ExecutionMode = 'fixedRate';
t.Period = round(1 / target_fps * 1e3) / 1e3;   % Trigger radar measurements at fixed rate (timer limited to 1 millisecond precision)
t.UserData = user_data;
t.TimerFcn = @get_frame;                        % Callback to trigger radar measurement

% This example runs the timer for 10 seconds
fprintf('starting timer...\n');
start(t);
pause(10.0);
stop(t);
fprintf('stopping timer...\n');
fprintf('Average period = %f or %f fps\n', t.AveragePeriod, 1 / t.AveragePeriod);

% Display the frame data
bscan_data = t.UserData.fdata';  % save data as bscan
figure;
imagesc(bscan_data);
title('Data');
xlabel('measurement #'); 
ylabel('sample');

% Delete the timer when we are done with it...
delete(t);

% Close the radar when we are done...
r.Close();

function get_frame(obj, event)
    % Get handle to the radar
    r = obj.UserData.rhandle;
    
    % Get a frame
    frame = r.GetFrameRawDouble;
    
    % Add data processing, etc...
    % This example just appends the frames into fdata which is a matrix
    % contains all the frames
    obj.UserData.fdata = [obj.UserData.fdata; frame];
    
%     fprintf('%.2f\n', 1 / obj.InstantPeriod);
%     fprintf('Got a frame...\n');
end
