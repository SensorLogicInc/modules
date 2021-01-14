classdef vcom_xep_radar_connector < handle
    % The *vcom_xep_radar_connector* allows the user to manipulate the X4
    % USB radar over a virtual COM port using USB.
    %
    % Example:
    % r = vcom_xep_radar_connector('COM106');
    % r.Open('X4');
    % plot(r.GetFrameRaw());
    % r.Close();
    %
    % Copyright (c) 2020 Sensor Logic
    
    properties
        % Is there a radar connection open at the moment
        isOpen=0;
        
        % Global Radar Parameters
        numSamplers = 0;
    end
    
    properties(Hidden)
        usb_conn;
        x4DownConverter = 0;
 
        % System options
        dirpath = fileparts(which('xep_radar_connector'));
       
        % Experimental Options (Alpha features)
        DEV_v2_packet_type = 0;
        timeout = 5;
        lastError = '';
        connectorVer = 0.00;
    end
    
    methods
        function obj = vcom_xep_radar_connector(com_port)          
            orig_state = warning;
            warning('off', 'all')

            % Turn warnings back to original state
            warning(orig_state);
            
            obj.StartServer(com_port);
        end
        
        %% Delete the radar handle/close the program
        function delete(obj)
            % Deletes a radar handle
            obj.usb_conn = [];
            obj.isOpen = 0;
        end               
             
        %% Request the connector version
        function status = ConnectorVersion(obj)
            write(obj.usb_conn, 'ConnectorVersion()', 'uint8') % Send command
            status = obj.getData();                            % Wait for an ACK
        end
        
        %% Create a radar handle
        function status = NVA_CreateHandle(obj)
            % CreateHandle Creates a handle to a radar object prior to
            % opening the radar
            write(obj.usb_conn, 'InitHandle()', 'uint8'); % Send command
            status = obj.getData();                       % Wait for an ACK
        end

        %% Open the connection to the radar
        function status = Open(obj, connectString) 
            % Open Opens a connection to a radar module such that it can be
            % accessed.
            cmd = uint8(['OpenRadar(' connectString ')']);
            write(obj.usb_conn, cmd, 'uint8'); % Send command
            status = obj.getData();            % Wait for ACK            
            obj.isOpen = 1;                    % Set open flag
            
            % Update the number of samplers
            obj.updateNumberOfSamplers();
        end
        
        %% Close the program/radar handle
        function Close(obj)          
            % Close Closes an open radar handle object, but the connection
            % to the BeagleBone platform is maintained
            write(obj.usb_conn, 'Close()', 'uint8'); % Send command
            obj.getData();                           % Wait for ACK            
            obj.isOpen = 0;                          % Set open flag
            obj.usb_conn = [];
        end

        %% Update a register on the BBB
        function status = TryUpdateChip(obj, registerName, value)
            % TryUpdateChip Updates a register value on the chip
            % based on the register name.
            %
            % Example:
            %   radar = xep_radar_connector;
            %   modules = radar.ConnectedModules;
            %   radar.Open(modules{1});
            %   radar.TryUpdateChip('Iterations', 10);
            %   radar.Close;            

            if (strcmp(registerName, 'ddc_en') || strcmp(registerName, 'DownConvert'))
                obj.x4DownConverter = value;
            end
            
            cmd = uint8(['VarSetValue_ByName(' registerName ',' num2str(value) ')']);
            write(obj.usb_conn, cmd, 'uint8'); % Send command
            status = obj.getData();
            
            % Update the number of samplers in case it changed
            obj.updateNumberOfSamplers();
        end
        
        %% Update a register on the BBB
        function status = EnableX4HWDDC(obj,value)
            status = obj.TryUpdateChip('DownConvert', value);
            obj.x4DownConverter = value;
        end
        
        %% Get a register value from the BBB
        function register = Item(obj, registerName)
            % Item Get a register value from the chip from its name
            % 
            % Example:
            %   radar = xep_radar_connector;
            %   modules = radar.ConnectedModules;
            %   radar.Open(modules{1});
            %   iterations = radar.Item('Iterations');
            %   radar.Close;
            
            cmd = uint8(['VarGetValue_ByName(' registerName ')']);
            write(obj.usb_conn, cmd, 'uint8'); % Send command
            a = obj.getData();                 % Wait for ACK
            register = str2num(char(a));
        end
        
        %% Get a raw radar frame
        function frame = GetFrameRawDouble(obj)
            frame = double(GetFrameRaw(obj));
        
            if obj.x4DownConverter == 1
                frame = frame(1:2:end)+ 1i*frame(2:2:end);
            end
        end
        
        %% Get a normalized radar frame
        function frame = GetFrameNormalizedDouble(obj)
            frame = double(GetFrameNormalized(obj));
        
            if obj.x4DownConverter == 1
                frame = frame(1:2:end)+ 1i * frame(2:2:end);
            end
        end
          
        %% Get a list of the variables on the radar
        function list = ListVariables(obj)
            % ListVariables Get a list of all the variables supported on the
            % radar chip as a matrix of strings

            write(obj.usb_conn, 'ListVariables()', 'uint8');
            list = char(obj.getData());
            list = strsplit(list, ',');
        end
        
        %% Get a list of the variables on the radar
        function register = RegisterRead(obj, address, length)
            % RegisterRead Read a register based on its address (decimal)
            % and length in bytes rather then its name
            %
            % Example:
            %   radar = xep_radar_connector;
            %   modules = radar.ConnectedModules;
            %   radar.Open(modules{1});
            %   zeros = radar.RegisterRead(0,4);    % 32 bits of the "zeros" register
            %   ones = radar.RegisterRead(1,4);     % 32 bits of the "ones" register
            %   radar.Close;
            
            cmd = uint8(['RegisterRead(', num2str(address),',',num2str(length),')']);
            write(obj.usb_conn, cmd, 'uint8');
            a = obj.getData();
            register = str2num(char(a));
        end
        
        %% Get the actual sampler resolution
        function resolution = SamplerResolution(obj)
            % SamplerResolution Get the measured sampler resolution on the
            % radar chip.  This is dependent both on settings and
            % temperature.
            resolution = obj.Item('res');
        end
        
        %% Number of samplers with the current radar settings
        function samplers = SamplersPerFrame(obj)
            % SamplersPerFrame Get the number of samplers for the radar with
            % its current settings
            samplers = double(obj.Item('SamplersPerFrame'));
        end
        
        %% Get the message from the last error that occured
        function err = LibraryError(obj)
            err = obj.lastError;
        end
    
    
        %% Set all chip registers to their default values
        function [] = ResetAllVariablesToDefault(obj)
            write(obj.usb_conn, 'VarsResetAllToDefault()', 'uint8');
            obj.getData();
            obj.updateNumberOfSamplers();
        end
    end
        
    methods(Hidden)
        %% Get a raw radar frame
        function frame = GetFrameRaw(obj)
            % GetFrameRaw Get a complete radar frame in raw count values.
            % These counts will change based on the radar settings
            %
            % Example:
            %   radar = xep_radar_connector;
            %   modules = radar.ConnectedModules;
            %   radar.Open(modules{1});
            %   frame = radar.GetFrameRaw;  
            %   radar.Close;
            
            % Binary data tranfer (faster)
            write(obj.usb_conn, 'GetFrameRaw()', 'uint8'); % Send command
            
            if obj.DEV_v2_packet_type == 1
                while (obj.usb_conn.NumBytesAvailable < 4)
                end
                packetlength = read(obj.usb_conn, 1, 'int32');
                while (obj.usb_conn.NumBytesAvailable < packetlength)
                end
                frame = read(obj.usb_conn, packetlength, 'uint8');
                obj.parseErrReturn(frame);
                frame = frame(1:end-5);
                frame = typecast(uint8(frame), 'single');
                return
            else                
                frame = [];
                i = 1;

                % Calculate the expected size of the frame in bytes
                if obj.x4DownConverter == 1
                    frameSize = 2 * obj.numSamplers * 4 + 5;
                else
                    frameSize = obj.numSamplers * 4 + 5;
                end

                while(1==1) 
                    if (obj.usb_conn.NumBytesAvailable == frameSize)
                         frame = read(obj.usb_conn, frameSize, 'uint8');
                         frame = frame(1:(end-5));
                         break;
%                          if (obj.parseIsDone(char(frame)))
%                              frame = frame(1:(end-5));
%                              break;
%                          end
                    end
                    % Timeout
                    if (i == 50000)
                        disp('Timeout');
                        frame = read(obj.usb_conn, obj.usb_conn.NumBytesAvailable, 'uint8');
                        error(char(frame(6:end-5)));
                        frame = zeros(1, obj.numSamplers);
                    end
                    i = i + 1;                                    
                end
                frame = typecast(uint8(frame), 'single');
            end
        end
        
        %% Get a normalized radar frame
        function frame = GetFrameNormalized(obj)
            % GetFrameNormalized Get a radar frame that has been
            % normalized such that the bins will not change based on the
            % radar parameters.
            %
            % Example:
            %   radar = xep_radar_connector;
            %   modules = radar.ConnectedModules;
            %   radar.Open(modules{1});
            %   frame = radar.GetFrameNormalizedDouble;  
            %   radar.Close;
            %

            write(obj.usb_conn, 'GetFrameNormalized()', 'uint8'); % Send command
            
            if obj.DEV_v2_packet_type == 1
                while (obj.usb_conn.NumBytesAvailable < 4)
                end
                packetlength = read(obj.usb_conn, 1, 'int32');
                while (obj.usb_conn.NumBytesAvailable < packetlength)
                end
                frame = read(obj.usb_conn, packetlength, 'uint8');
                obj.parseErrReturn(frame);
                frame=frame(1:end-5);
                frame = typecast(uint8(frame), 'single');
                return
            else                
                frame = [];
                i = 1;

                % Calculate the expected size of the frame in bytes
                if obj.x4DownConverter == 1
                    frameSize = 2 * obj.numSamplers * 4 + 5;
                else
                    frameSize = obj.numSamplers * 4 + 5;
                end

                while(1==1) 
                    if (obj.usb_conn.NumBytesAvailable == frameSize)
                        frame = read(obj.usb_conn, frameSize, 'uint8');
                        frame = frame(1:(end-5));
                        break;
%                          if (obj.parseIsDone(char(frame)))
%                              frame = frame(1:(end-5));
%                              break;
%                          end
                    end
                    % Timeout
                    if (i == 50000)
                        disp('Timeout');
                        frame = read(obj.usb_conn, obj.usb_conn.NumBytesAvailable, 'uint8');
                        error(char(frame(6:end-5)));
                        frame = zeros(1, obj.numSamplers);
                    end
                    i = i+1;                                    
                end
                frame = typecast(uint8(frame), 'single');
            end
        end
        
        %% Function to get a full frame of data from the radar and check it for errors
        function a = getData(obj)
            if obj.DEV_v2_packet_type == 0
                i = 1;                                    %Iteration value for length of string
                a = [];
                while(1==1)                                 %Loop until ack recieved
                     bytes = obj.usb_conn.NumBytesAvailable;
                     if (bytes > 0)                           %If char in buffer
                         temp = read(obj.usb_conn, bytes, 'uint8');    %read it and put it on "stack"
                         a = [a, temp]; %#ok                %read it and put it on "stack"                     
                         if (obj.parseIsDone(a))
                             a=a(1:end-5);                  %Pull the ack out of the signal
                             obj.parseErrReturn(a);
                             break;                         %break the loop and continue program
                         end
                         i = i+1;                       %else, get ready for the next char
                     end
                end
            else
                packetlength = read(obj.usb_conn, 1, 'int32');
                a = read(obj.usb_conn, packetlength, 'uint8');
                obj.parseErrReturn(a);
                a=a(1:end-5);
            end
        end
        
        %% Test the return to test and pull out error messages
        function parseErrReturn(obj, returnString)
            if (length(returnString) > 6)
                if(strcmp(returnString(1:5), '<ERR>'))
                    obj.lastError = returnString(6:(end-5));
                    error(returnString(6:(end)));
                end
                if(strcmp(returnString(1:5), '<WRN>'))
                    obj.lastError = returnString(6:(end-5));
                    warning(returnString(6:(end)));
                end
            end
        end
        
        %% Test to see if the ACK string has been received
        function [isDone] = parseIsDone(~, returnString)
            isDone = 0;
            if (length(returnString) > 4)
                if(strcmp(char(returnString((end-4):end)), '<ACK>'))
                    isDone=1;
                end
            end
        end
        
        %% Update "Number of Samplers" interval var
        function [] = updateNumberOfSamplers(obj)
            obj.numSamplers = obj.Item('SamplersPerFrame');
        end

        %% Start an instance of the connector
        function StartServer(obj, com_port)
            % Establish a socket connection to the BBB
            for i = 1:20
                try
                    obj.usb_conn = serialport(com_port, 115200, 'Timeout', 1);
                    
                    setDTR(obj.usb_conn, true);
                    setRTS(obj.usb_conn, true);
                    break
                catch
                    disp('trying to connect....');
                end
            end
            if (i == 20)
                error('Could not connect to usb')
            end

            write(obj.usb_conn, 'NVA_CreateHandle()', 'uint8');
            obj.getData();
        end
    end
end
