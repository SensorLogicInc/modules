# -*- coding: utf-8 -*-
import time
import os

import numpy as np
import matplotlib.pyplot as plt
import serial


# TODO fix it so we don't timeout every single time

class xep_radar_connector:
    def __init__(self, iterations=16, pulses_per_step=300, dac_min=949, dac_max=1149, \
                 area_start=0.4, area_end=5.0, port='/dev/ttyACM0'):
        self.ser = serial.Serial()
        self.ser.port = port
        self.ser.baudrate = 115200
        self.ser.timeout = 0.1
        
        # radar parameters
        self.iterations = iterations
        self.pulses_per_step = pulses_per_step
        self.dac_min = dac_min
        self.dac_max = dac_max
        self.area_start = area_start
        self.area_end = area_end

        # self.DEV_v2_packet_type
        # self.lastError
        # self.connectorVer
        # self.isOpen
        self.usb_conn = []
        self.x4DownConverter = 0
        self.numSamplers = 0

        # TODO sometimes problems and need to replug, read this and implement:
        # https://stackoverflow.com/questions/14525977/how-do-i-recover-from-a-serialexception-using-pyserial

        self.StartServer()

    def sys_init(self):
        self.TryUpdateChip('ddc_en', 1)
        # self.TryUpdateChip('ddc_en', 0)

        self.TryUpdateChip('iterations', self.iterations)
        self.TryUpdateChip('pps', self.pulses_per_step)
        self.TryUpdateChip('dac_min', self.dac_min)
        self.TryUpdateChip('dac_max', self.dac_max)
        self.TryUpdateChip('frame_start', self.area_start)
        self.TryUpdateChip('frame_end', self.area_end)
        # self.TryUpdateChip('frame_offset', 0.18)
        self.TryUpdateChip('tx_power', 3)

        self.bin_length = self.Item('res')
        self.area_start = self.Item('frame_start')
        self.area_end = self.Item('frame_end')
        self.prf = self.Item('prf')
        self.PRF = self.Item('PRF')
        self.SamplingRateFS = self.Item('fs')
        self.rx_wait = self.Item('rx_wait')

        print('bin_length res: {}. area_start: {}. area_end: {}'.format(self.bin_length, self.area_start, self.area_end))
        print('prf: {}.'.format(self.prf))

        # self.TryUpdateChip('prf_div', 16)  # default was 16 to result in 15187500.0 PRF
        print('PRF: {}.'.format(self.Item('PRF')))

        print('SamplingRateFS: {}.'.format(self.SamplingRateFS))
        print('rx_wait: {}.'.format(self.rx_wait))

        print('numSamplers(num bins):', self.numSamplers)

        all_vars = self.ListVariables()
        print(all_vars)

        for var in ['res', 'iterations', 'pps', 'dac_min', 'dac_max', 'dac_step', 'rx_wait', 'ddc_en', 'num_samples', \
            'tx_region', 'tx_power', 
            'frame_length', 'frame_start', 'frame_end', 'frame_offset', 'unambiguous_range', 'sweep_time', 'prf', 'fs', 'SamplingRate', 'dac_step', 'tx_power']:
            print('{} = {}'.format(var, self.Item(var)))

        self.bin_length =  0.05144033

    def Open(self, connectString='X4'):
        cmd = "OpenRadar('{}')".format(connectString).encode()
        self.ser.write(cmd) # Send command
        status = self.ser.read_until()          # Wait for ACK            
        print('Status after OpenRadar(\'{}\'): {}'.format(connectString, status))
        self.isOpen = 1;                    # Set open flag

        self.sys_init()

    def Close(self):
        self.ser.write(b'Close()')
        status = self.ser.read_until()  # Wait for ACK            
        print('Status after Close(): {}'.format(status))
        self.ser.close()

    def TryUpdateChip(self, registerName, value):
        """
        TryUpdateChip Updates a register value on the chip
        based on the register name.
        """
        if registerName == 'ddc_en' or registerName == 'DownConvert':
            self.x4DownConverter = value
            print('self.x4DownConverter', self.x4DownConverter)

        cmd = "VarSetValue_ByName({}, {})".format(registerName, value).encode()
        print(cmd)
        self.ser.write(cmd)

        status = self.ser.read_until()
        print('Status after TryUpdateChip(): {}'.format(status))

        # Update the number of samplers in case it changed
        self.updateNumberOfSamplers()

    def NVA_CreateHandle(self):
        pass

    def ConnectorVersion(self):
        pass

    def delete(self):
        pass

    def EnableX4HWDDC(self):
        pass

    def Item(self, registerName):
        # Item Get a register value from the chip from its name

        cmd = "VarGetValue_ByName({})".format(registerName).encode()
        self.ser.write(cmd)

        status = self.ser.read_until()
        # print('Status after VarGetValue_ByName({}): {}'.format(registerName, status))

        # TODO: make it work for both python2 and 3
        # chr_arr = [chr(x) for x in status]
        chr_arr = status
        angle_bracket_idx = chr_arr.index(b'<') if b'<' in chr_arr else -1
 
        if angle_bracket_idx != -1:
            # str_arr = ''.join(chr_arr[:angle_bracket_idx])  # py27
            str_arr = chr_arr[:angle_bracket_idx]  # python3
            if b'.' in str_arr:
                register = float(str_arr)
            else:
                register = int(str_arr)
        else:
            register = ''
        # print('registerName: {}. value: {}'.format(registerName, register))
        return register

    def GetFrameRaw(self):
        i = 0

        self.ser.write(b'GetFrameRaw()')

        if self.x4DownConverter == 1:
            frameSize = 2 * self.numSamplers * 4 + 5
        else:
            frameSize = self.numSamplers * 4 + 5

        while True:
            if self.ser.in_waiting == frameSize:
                frame = self.ser.read(frameSize)
                frame = frame[0:-5]
                break

            # TODO timeout everytime! 
            if i == 15000:
                frame = self.ser.read(frameSize)
                frame = frame[0:-5]
                break

            i += 1

        frame_int_array = [x for x in frame]  # from bytes to int array
        uint8_arr = np.array(frame_int_array, dtype="uint8")
        uint32_arr = uint8_arr.view('float32')

        return uint32_arr

    def GetFrameRawDouble(self):
        pass

    def GetFrameNormalized(self):
        i = 0

        self.ser.write(b'GetFrameNormalized()')

        # TODO a lot of the below is the exact same as frameRaw. Should put into function
        if self.x4DownConverter == 1:
            frameSize = 2 * self.numSamplers * 4 + 5
        else:
            frameSize = self.numSamplers * 4 + 5

        while True:
            if self.ser.in_waiting == frameSize:
                frame = self.ser.read(frameSize)
                frame = frame[0:-5]
                break

            # TODO timeout everytime! Could be a problem for consistent FPS
            if i == 15000:
                frame = self.ser.read(frameSize)  # TODO 4151 ready but reading 6145 solves it?
                frame = frame[0:-5]
                break

            i += 1

        frame_int_array = [x for x in frame]  # from bytes to int array
        # frame_int_array = [int(f.encode('hex'), 16) for f in frame]  # py27
        uint8_arr = np.array(frame_int_array, dtype="uint8")
        uint32_arr = uint8_arr.view('float32')

        return uint32_arr

    def GetFrameNormalizedDouble(self):
        frame = self.GetFrameNormalized().astype('double')

        return frame
    
    def getData(self):
        pass

    def ListVariables(self):
        cmd = b'ListVariables()'

        self.ser.write(cmd)

        raw_data = self.ser.read_until()
        # TODO could remove the ACKs

        raw_data_str = str(raw_data)

        var_list = raw_data_str.split(',')
        # print(var_list)

        return var_list

    def RegisterRead(self):
        pass

    def SamplerResolution(self):
        pass

    def SamplersPerFrame(self):
        pass

    def LibraryError(self):
        pass

    def ResetAllVariablesToDefault(self):
        pass

    def parseErrReturn(self):
        pass

    def parseIsDone(self):
        pass

    def updateNumberOfSamplers(self):
        self.numSamplers = self.Item('SamplersPerFrame')

    def StartServer(self):
        print('starting server')
        print('ser open:', self.ser.is_open)

        self.ser.dtr = True
        self.ser.rts = True

        try:
            self.ser.open()
        except Exception as e:
            print('Could not open serial port')
            raise e

        self.ser.flushInput()

        self.ser.write(b'NVA_CreateHandle()')
        status = self.ser.read_until()
        print('Status after NVA_CreateHandle()', status)

    #####################
    # New functions
    #####################

    def get_apdata(self):
        # rename to get_frame
        # frame = self.GetFrameRaw()
        double_frame = self.GetFrameNormalizedDouble()

        if self.x4DownConverter == 1:
            iq_vec = double_frame[0::2] + 1j * double_frame[1::2]
        else:
            # Raw RF direct data  # TODO current not dealing with it
            pass

        # iq_vec = frame  # TODO is this best, do we ever want non normalised or double data?
        # TODO plot direct RF with downconversion to 0
        # TODO if direct RF don't do IQ stuff

        # TODO abs is same np.abs is same as euclidean distance is same as np.hypot
        # ampli_data = abs(iq_vec)                       #振幅
        ampli_data = np.abs(iq_vec)                       #振幅
        phase_data = np.angle(iq_vec)

        return ampli_data, phase_data, iq_vec, double_frame

    def get_data_matrix(self, num_rows=1020, save=False, path='none'):
        # row = sample_time * self.FPS
        # col = self.fast_sample_point
        row = num_rows  # TODO how to do this time based and FPS based. Return FPS?
        col = self.numSamplers

        amp_matrix = np.empty([row, col])
        pha_matrix = np.empty([row, col])
        for i in range(num_rows):
            start_time = time.time()
            # TODO not doing if interval > 1/17*1000, is this a problem?
            # new_time = datetime.datetime.now()
            # interval = (new_time - old_time).microseconds
            # if interval > 1 / 17 * 1000:
            #     old_time = new_time
            amp_data, pha_data, iq_vec, double_frame = self.get_apdata()
            amp_matrix[i] = amp_data
            pha_matrix[i] = pha_data
            time_taken = time.time() - start_time
            FPS = 1 / time_taken
            print(i, 'FPS: ', FPS)

        # final_FPS = int(round(FPS))  # TODO should I decide FPS or not? TODO round or not?
        final_FPS = FPS  # TODO round or not? or average? how about dat? 

        if save:
            print('Saving files to path: {}'.format(path))
            folder = os.path.exists(path)
            if not folder:
                os.mkdir(path)
                filename1 = path + '/amp_matrix.txt'
                filename2 = path + '/pha_matrix.txt'
                np.savetxt(filename1, amp_matrix)
                np.savetxt(filename2, pha_matrix)
            else:
                print('error:the folder exists!!!')

        return amp_matrix, pha_matrix, final_FPS

        

# TODO finally/try/except so deconstructor will properly kill serial port
if __name__ == "__main__":
    slmx4 = xep_radar_connector()
    slmx4.Open('X4')

    # As a side-effect many settings on write will cause the numSamplers variable to update
    print('num bins before running anything = {}'.format(slmx4.numSamplers))

    all_vars = slmx4.ListVariables()
    print(all_vars)

    # TODO float/double problems: 'prf', 'frame_start', 'frame_end', 
    for var in ['res', 'iterations', 'pps', 'dac_min', 'dac_max', 'dac_step', 'rx_wait', 'ddc_en', 'num_samples', \
        'tx_region', 'tx_power', 
        'frame_length', 'frame_start', 'frame_end', 'frame_offset', 'unambiguous_range', 'sweep_time', 'prf', 'fs', 'SamplingRate', 'dac_step', 'tx_power']:
        print('{} = {}'.format(var, slmx4.Item(var)))

    num_bins = slmx4.numSamplers
    print('num bins after updating chip = {}'.format(num_bins))

    amp_data, pha_data, iq_vec, double_frame = slmx4.get_apdata()

    # frame = slmx4.GetFrameNormalizedDouble()
    # amplitude_frame_normalized = abs(frame)
    # print(amplitude_frame_normalized)
    # # TODO what does it look like without abs in matlab + python
    # print('frame shape: {}'.format(frame.shape))
    # if len(frame) > 0:
    #     plt.title('IQ complex interleaved')
    #     plt.plot(frame)
    #     # plt.show()

    #     plt.figure()
    #     plt.title('amplitude_frame_normalized')
    #     plt.plot(amplitude_frame_normalized)
    #     # plt.show()

    #     plt.figure()
    #     plt.title('amp_data from apdata')
    #     ax_x = np.arange(0, num_bins)
    #     plt.plot(ax_x, amp_data)
        
    #     plt.figure()
    #     plt.title('phase from apdata')
    #     plt.plot(pha_data)
    #     plt.show()

    # FPS frame_rate test
    row = 1020
    col = slmx4.numSamplers

    amp_matrix = np.empty([row, col])

    for i in range(50):
        start_time = time.time()
        amp_data, pha_data, iq_vec, double_frame = slmx4.get_apdata()
        time_taken = time.time() - start_time
        FPS = 1 / time_taken
        print('Count: {}. Loop time taken: {:.3f}. FPS: {:.3f}'.format(i, time_taken, FPS))

    final_FPS = int(round(FPS))
    print('amp_data.shape:', amp_data.shape)

    slmx4.Close()