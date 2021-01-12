%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% unit_test.m
%
% This is a demonstration how to use the class
% - vcom_xep_radar_connector
%
% This is just a sanity-check which sets various registers to some value
% then reads them back.
%
% Copyright: 2020 Sensor Logic
% Written by: Justin Hadella
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
clear;
clc;


r = vcom_xep_radar_connector('COM106');
r.Open('X4');

fprintf('XEP set/get unit test\n');
fprintf('---------------------\n');

% The test below contains set/get tests for all of the integer based radar
% settings. The float based set functions are not as easily testable in
% this way.

% DAC Min test
ok = 1;
r.TryUpdateChip('DACMin', 0);
ok = ok && 0 == r.Item('DACMin');
r.TryUpdateChip('dac_min', 0);
ok = ok && 0 == r.Item('dac_min');
for i = 0:11
    val = bitshift(1, i) - 1;
    r.TryUpdateChip('DACMin', val);
    ok = ok && val == r.Item('DACMin');
    
    r.TryUpdateChip('dac_min', val);
    ok = ok && val == r.Item('dac_min');
end
% Test every value (time consuming...)
% for i = 0:2047
%     r.TryUpdateChip('DACMin', i);
%     ok = ok && i == r.Item('DACMin');
%     
%     r.TryUpdateChip('dac_min', i);
%     ok = ok && i == r.Item('dac_min');
% end
fprintf('dac_min test ok? %d\n', ok);
r.TryUpdateChip('dac_min', 0);


% DAC Max test
ok = 1;
r.TryUpdateChip('DACMax', 0);
ok = ok && 0 == r.Item('DACMax');
r.TryUpdateChip('dac_max', 0);
ok = ok && 0 == r.Item('dac_max');
for i = 0:11
    val = bitshift(1, i) - 1;
    r.TryUpdateChip('DACMax', val);
    ok = ok && val == r.Item('DACMax');
    
    r.TryUpdateChip('dac_max', val);
    ok = ok && val == r.Item('dac_max');
end
% Test every value (time consuming...)
% for i = 0:2047
%     r.TryUpdateChip('DACMax', i);
%     ok = ok && i == r.Item('DACMax');
%     
%     r.TryUpdateChip('dac_max', i);
%     ok = ok && i == r.Item('dac_max');
% end
fprintf('dac_max test ok? %d\n', ok);
r.TryUpdateChip('dac_max', 2047);


% DAC Step test
ok = 1;
for i = 0:3
    r.TryUpdateChip('DACStep', i);
    ok = ok && i == r.Item('DACStep');
    
    r.TryUpdateChip('dac_step', i);
    ok = ok && i == r.Item('dac_step');
end
fprintf('dac_step test ok? %d\n', ok);
r.TryUpdateChip('dac_step', 0);


% Iterations test
ok = 1;
for i = 4:4:255
    % Iterations must be a multiple of 4 otherwise a warning message will
    % result which is considered an error!
    
    r.TryUpdateChip('Iterations', i);
    ok = ok && i == r.Item('Iterations');
    
    r.TryUpdateChip('iterations', i);
    ok = ok && i == r.Item('iterations');
end
fprintf('iterations test ok? %d\n', ok);
r.TryUpdateChip('iterations', 16);


% PPS test
ok = 1;
r.TryUpdateChip('PPS', 0);
ok = ok && 0 == r.Item('PPS');
r.TryUpdateChip('pps', 0);
ok = ok && 0 == r.Item('pps');
for i = 0:16
    val = bitshift(1, i) - 1;
    r.TryUpdateChip('PPS', val);
    ok = ok && val == r.Item('PPS');
    
    r.TryUpdateChip('pps', val);
    ok = ok && val == r.Item('pps');
end
% Test every value (time consuming...)
% for i = 0:65535
%     r.TryUpdateChip('PPS', i);
%     ok = ok && i == r.Item('PPS');
%     
%     r.TryUpdateChip('pps', i);
%     ok = ok && i == r.Item('pps');
% end
fprintf('pps test ok? %d\n', ok);
r.TryUpdateChip('pps', 8);


% prf_div test
ok = 1;
for i = 4:16
    r.TryUpdateChip('prf_div', i);
    ok = ok && i == r.Item('prf_div');
end
fprintf('prf_div test ok? %d\n', ok);
r.TryUpdateChip('prf_div', 16);


% frame length test
ok = 1;
for i = 4:16
    r.TryUpdateChip('frame_length', i);
    ok = ok && i == r.Item('frame_length');
end
fprintf('frame_length test ok? %d\n', ok);
r.TryUpdateChip('frame_length', 16);


% rx_wait test
ok = 1;
for i = 0:255
    r.TryUpdateChip('RxWait', i);
    ok = ok && i == r.Item('RxWait');
    
    r.TryUpdateChip('rx_wait', i);
    ok = ok && i == r.Item('rx_wait');
end
fprintf('rx_wait test ok? %d\n', ok);
r.TryUpdateChip('rx_wait', 0);


% tx_region test
ok = 1;
for i = 3:4
    r.TryUpdateChip('tx_region', i);
    ok = ok && i == r.Item('tx_region');
end
fprintf('tx_region test ok? %d\n', ok);
r.TryUpdateChip('tx_region', 3);


% tx_power test
ok = 1;
for i = 0:3
    r.TryUpdateChip('tx_power', i);
    ok = ok && i == r.Item('tx_power');
end
fprintf('tx_power test ok? %d\n', ok);
r.TryUpdateChip('tx_power', 2);


% DDC test
ok = 1;
r.TryUpdateChip('DownConvert', 1);
ok = ok && 1 == r.Item('DownConvert');
r.TryUpdateChip('DownConvert', 0);
ok = ok && 0 == r.Item('DownConvert');
r.TryUpdateChip('ddc_en', 1);
ok = ok && 1 == r.Item('ddc_en');
r.TryUpdateChip('ddc_en', 0);
ok = ok && 0 == r.Item('ddc_en');
r.EnableX4HWDDC(1);
ok = ok && 1 == r.Item('DownConvert');
r.EnableX4HWDDC(0);
ok = ok && 0 == r.Item('ddc_en');
fprintf('ddc_en test ok? %d\n', ok);


r.Close();
