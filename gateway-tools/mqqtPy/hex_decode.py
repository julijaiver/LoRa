# This program decodes hex string received from mqtt and packs to json
# The endiannes of the system is important and should be specified to ensure correct byte order

import struct

# unpack data and check the identifier to see which sensor data is received
# data saved in a nested dictionary
def unpack_data(hex_str, endiannes):
    identifier = hex_str[0:2]
    if identifier == '01':
        data_type = "particulate"
        pm25 = hex_str[2:10]
        pm10 = hex_str[10:18]
        data = {"pm25": round(hex_to_float(pm25, endiannes), 2),
                "pm10": round(hex_to_float(pm10, endiannes), 2)}

    elif identifier == '02':
        data_type = "BME690"
        voc = hex_str[2:10]
        pressure = hex_str[10:18]
        humidity = hex_str[18:26]
        data = {"voc": round(hex_to_float(voc, endiannes), 2),
                "pressure": round(hex_to_float(pressure, endiannes), 2),
                "humidity": round(hex_to_float(humidity, endiannes), 2)}

    elif identifier == '03':
        data_type = "temperature"
        temperature = hex_str[2:10]
        data = {"temperature": hex_to_float(temperature, endiannes)}

    return {"type": data_type, "data": data}

def hex_to_float(hex_str, endianness):
    #int_val = int(hex_str, 16)
    byte_array = bytes.fromhex(hex_str)
    format_char = '<f' if endianness == 'little' else '>f'
    return struct.unpack(format_char, byte_array)[0]

#checking if data is ok
#it is :D
#print(round(hex_to_float("C355AD43", 'little'), 2))

#print(unpack_data("01A4705541B81E6742", 'little'))
#print(unpack_data("02C3D5AC4300507D4466663642", 'little'))
#print(unpack_data("030000B441", 'little'))

