# This program decodes hex string received from mqtt and packs to a dictionary
# The endiannes of the system is important and should be specified to ensure correct byte order

import struct

# unpack data. No identifier bytes, just the data values in a unified format
# since data is hex, two 
def unpack_data(data_hex, deveui, endianness):
    hex_str = data_hex.upper()
    
    sps_pm25_numerical = data_hex[0:8]
    sps_pm25_mass = data_hex[8:16]
    sps_pm10_numerical = data_hex[16:24]
    sps_pm10_mass = data_hex[24:32]

    bmv_pm25_numerical = data_hex[32:40]
    bmv_pm25_mass = data_hex[40:48]
    bmv_pm10_numerical = data_hex[48:56]
    bmv_pm10_mass = data_hex[56:64]

    bme_voc = data_hex[64:72]
    bme_pressure = data_hex[72:80]
    bme_humidity = data_hex[80:88]

    temperature = data_hex[88:96]

    data = {"SPS30": {"pm25": {"numerical": round(hex_to_float(sps_pm25_numerical, endianness), 2),
                            "mass": round(hex_to_float(sps_pm25_mass, endianness), 2)},
                    "pm10": {"numerical": round(hex_to_float(sps_pm10_numerical, endianness), 2),
                            "mass": round(hex_to_float(sps_pm10_mass, endianness), 2)}},
            "BMV080": {"pm25": {"numerical": round(hex_to_float(bmv_pm25_numerical, endianness), 2),
                            "mass": round(hex_to_float(bmv_pm25_mass, endianness), 2)},
                    "pm10": {"numerical": round(hex_to_float(bmv_pm10_numerical, endianness), 2),
                            "mass": round(hex_to_float(bmv_pm10_mass, endianness), 2)}},
            "BME690": {"voc": round(hex_to_float(bme_voc, endianness), 2),
                    "pressure": round(hex_to_float(bme_pressure, endianness), 2),
                    "humidity": round(hex_to_float(bme_humidity, endianness), 2)},
            "temperature": round(hex_to_float(temperature, endianness), 2)}

    return {"devEui": deveui, "data": data}

def hex_to_float(hex_str, endianness):
    #int_val = int(hex_str, 16)
    byte_array = bytes.fromhex(hex_str)
    format_char = '<f' if endianness == 'little' else '>f'
    return struct.unpack(format_char, byte_array)[0]