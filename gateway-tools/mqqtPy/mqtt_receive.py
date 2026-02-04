import paho.mqtt.client as mqtt
import json
import base64
import hex_decode

def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    client.subscribe("application/+/device/+/event/up")

def on_message(client, userdata, msg):
    json_data = json.loads(msg.payload)
    #print(f"{json_data}")
    base64_data = json_data['data']

    # there are two values in received data that can be used for identification:
    # ['deviceProfileName'] and ['devEui']
    device_info = json_data['deviceInfo']
    device_name = device_info['deviceProfileName']
    device_eui = device_info['devEui']

    hex_str = base64.b64decode(base64_data).hex().upper()
    print(f"Hex data: {hex_str}")

    print("Decoded: ")
    print(hex_decode.unpack_data(hex_str, device_eui, 'little'))

client = mqtt.Client(
    client_id="mac-python-subscriber",
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2
)

client.on_connect = on_connect
client.on_message = on_message

client.connect("192.168.1.73", 1883, 60)
client.loop_forever()
