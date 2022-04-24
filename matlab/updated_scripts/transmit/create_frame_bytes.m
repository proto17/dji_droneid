% Valid ending arguments:
% - SequenceNumber (0 - 65535)
% - StateInfo0 (1 byte)
% - StateInfo1 (1 byte)
% - SerialNumber (14-16 characters)
% - Longitude (double)
% - Latitude (double)
% - Height (-32768 to 32767)
% - Altitude (-32768 to 32767)
% - VelocityNorth (-32768 to 32767)
% - VelocityEast (-32768 to 32767)
% - VelocityUp (-32768 to 32767)
% - Yaw (-32768 to 32767)
% - PhoneAppGPSTime (milliseconds since Epoch)
% - PhoneAppLatitude (double)
% - PhoneAppLongitude (double)
% - HomeLatitude (double)
% - HomeLongitude (double)
% - ProductType (1 byte)
% - UUID (19 bytes)

function [frame] = create_frame_bytes(varargin)
%     MIN_SAMPLE_RATE = 15.36e6;
%     
%     assert(sample_rate >= MIN_SAMPLE_RATE, "Sample rate must be at least 15.36 MSPS");
%     assert(round(sample_rate / MIN_SAMPLE_RATE) * MIN_SAMPLE_RATE == sample_rate, ...
%         "Sample rate must be multiple of %d", MIN_SAMPLE_RATE);

    assert(isempty(varargin) || mod(length(varargin), 2) == 0, ...
        "Variable argument list must have even number of elements");
    
    sequence_number = 0;
    state_info = [0, 0];
    serial_number = '0123456789abcd';
    longitude = 0.0;
    latitude = 0.0;
    height = 0;
    altitude = 0;
    velocity_north = 0;
    velocity_east = 0;
    velocity_up = 0;
    yaw = 0;
    phone_app_gps_time = 0;
    phone_app_latitude = 0.0;
    phone_app_longitude = 0.0;
    home_latitude = 0.0;
    home_longitude = 0.0;
    product_type = 0;
    uuid = zeros(1, 19);
    

    for idx=1:2:length(varargin)
        key = varargin{idx};
        val = varargin{idx + 1};

        switch(key)
            case 'SequenceNumber'
                assert(isnumeric(val), 'Sequence number must be an integer');
                assert(val >= -32768 && val <= 32767, 'Invalid sequence number');

                sequence_number = uint16(val);
            case 'StateInfo0'
                assert(isinteger(val), 'StateInfo0 must be an integer');
                assert(val >= 0 && val <= 255, 'Invalid StateInfo0 value');

                state_info(1) = uint8(val);
            case 'StateInfo1'
                assert(isinteger(val), 'StateInfo1 must be an integer');
                assert(val >= 0 && val <= 255, 'Invalid StateInfo1 value');

                state_info(2) = uint8(val);
            case 'SerialNumber'
                assert(isstring(val) || ischar(val), 'SerialNumber must be a string')
                assert(length(val) >= 14 && length(val) <= 16, ...
                    'SerialNumber length %d is not valid.  Must be between 14 and 16 chars', length(val))
                
                serial_number = val;
            case 'Longitude'
                assert(isnumeric(val), 'Longitude must be a number')
                assert(abs(val) <= 180, 'Longitude must be between -180 and 180')

                longitude = double(val);
            case 'Latitude'
                assert(isnumeric(val), 'Latitude must be a number')
                assert(abs(val) <= 90, 'Latitude must be between -90 and 90')
                
                latitude = double(val);
            case 'Height'
                assert(isinteger(val), 'Height must be an integer')
                assert(val >= -32768 && val <= 32767, 'Height must be between -32768 and 32767')

                height = int16(val);
            case 'Altitude'
                assert(isinteger(val), 'Altitude must be an integer')
                assert(val >= -32768 && val <= 32767, 'Altitude must be between -32768 and 32767')

                altitude = int16(val);
            case 'VelocityNorth'
                assert(isinteger(val), 'VelocityNorth must be an integer')
                assert(val >= -32768 && val <= 32767, 'VelocityNorth must be between -32768 and 32767')

                velocity_north = int16(val);
            case 'VelocityEast'
                assert(isinteger(val), 'VelocityEast must be an integer')
                assert(val >= -32768 && val <= 32767, 'VelocityEast must be between -32768 and 32767')

                velocity_east = int16(val);
            case 'VelocityUp'
                assert(isinteger(val), 'VelocityUp must be an integer')
                assert(val >= -32768 && val <= 32767, 'VelocityUp must be between -32768 and 32767')

                velocity_up = int16(val);
            case 'Yaw'
                assert(isinteger(val), 'Yaw must be an integer')
                assert(val >= -32768 && val <= 32767, 'Yaw must be between -32768 and 32767')

                yaw = int16(val);
            case 'PhoneAppGPSTime'
                assert(isinteger(val), 'PhoneAppGPSTime must be an integer')
                assert(val >= 0 && val <= (2^64)-1, 'PhoneAppGPSTime must be between 0 and (2^64)-1')

                phone_app_gps_time = uint64(val);
            case 'PhoneAppLatitude'
                assert(isnumeric(val), 'PhoneAppLatitude must be a number')
                assert(abs(val) <= 90, 'PhoneAppLatitude must be between -90 and 90')
                
                phone_app_latitude = double(val);
            case 'PhoneAppLongitude'
                assert(isnumeric(val), 'PhoneAppLongitude must be a number')
                assert(abs(val) <= 180, 'PhoneAppLongitude must be between -180 and 180')
                
                phone_app_longitude = double(val);
            case 'HomeLatitude'
                assert(isnumeric(val), 'HomeLatitude must be a number')
                assert(abs(val) <= 90, 'HomeLatitude must be between -90 and 90')
                
                home_latitude = double(val);
            case 'HomeLongitude'
                assert(isnumeric(val), 'HomeLongitude must be a number')
                assert(abs(val) <= 180, 'HomeLongitude must be between -180 and 180')
                
                home_longitude = double(val);
            case 'ProductType'
                assert(isinteger(val), 'ProductType must be an integer')
                assert(val >= 0 && val <= 255, 'ProductType must be between 0 and 255')

                product_type = uint8(val);
            case 'UUID'
                assert(isstring(val) || ischar(val), 'UUID must be a string')
                assert(length(val) == 19, 'UUID must be 19 characters long (THIS IS TEMPORARY!!)')

                uuid = val;
            otherwise
                error('Invalid key "%s"', key);
        end
    end

    coord_adj = 10000000 / 57.2957795785523;
    
    % Pack the fields into bytes
    sequence_number_bytes = to_bytes(sequence_number, 2);
    state_info_bytes = state_info;
    serial_number_bytes = [uint8(serial_number) zeros(1, 16 - length(serial_number))];
    longitude_bytes = to_bytes(int32(round(longitude * coord_adj)), 4);
    latitude_bytes = to_bytes(int32(round(latitude * coord_adj)), 4);
    height_bytes = to_bytes(height, 2);
    altitude_bytes = to_bytes(altitude, 2);
    velocity_north_bytes = to_bytes(velocity_north, 2);
    velocity_east_bytes = to_bytes(velocity_east, 2);
    velocity_up_bytes = to_bytes(velocity_up, 2);
    yaw_bytes = to_bytes(yaw, 2);
    phone_app_gps_time_bytes = to_bytes(phone_app_gps_time, 8);
    phone_app_latitude_bytes = to_bytes(int32(round(phone_app_latitude * coord_adj)), 4);
    phone_app_longitude_bytes = to_bytes(int32(round(phone_app_longitude * coord_adj)), 4);
    home_latitude_bytes = to_bytes(int32(round(home_latitude * coord_adj)), 4);
    home_longitude_bytes = to_bytes(int32(round(home_longitude * coord_adj)), 4);
    product_type_bytes = product_type;
    uuid_bytes = uint8(uuid);

    version = uint8(2);
    message_type = uint8(16);
    uuid_len = uint8(length(uuid_bytes));


    frame = [ ...
        message_type, version, sequence_number_bytes, state_info_bytes, serial_number_bytes, longitude_bytes, ...
        latitude_bytes, height_bytes, altitude_bytes, velocity_north_bytes, velocity_east_bytes, velocity_up_bytes, ...
        yaw_bytes, phone_app_gps_time_bytes, phone_app_latitude_bytes, phone_app_longitude_bytes, home_longitude_bytes, ...
        home_latitude_bytes, product_type_bytes, uuid_len, uuid_bytes, uint8(0) ...
    ];

    frame = [uint8(length(frame)) frame];

    crc_bytes = to_bytes(calculate_crc(frame), 2);
    frame = [frame, crc_bytes];
end