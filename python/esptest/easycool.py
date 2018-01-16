import boto3
from boto3.dynamodb.conditions import Attr
from botocore.exceptions import ClientError
import json
import os
import random
import string
import esptool
from decimal import Decimal

from serial import SerialException

PROJECT_DIR = '/home/iftach/Dropbox/projects/temperature_monitor/software/pilot/v8/platformio'
CONFIG_FILE_PATH = PROJECT_DIR + '/data/key.json'
COMMAND = 'platformio run --project-dir {dir} --target {target}'
KEY_LENGTH = 64
CURRANT_VERSION = '3'

dynamodb = boto3.resource('dynamodb')
devices = dynamodb.Table('Devices')
users = dynamodb.Table('Users')


def claim_device(device_id=None, username=None, sudo=False):
    print 'Claim Device'

    if device_id is None:
        device_id = raw_input('\tEnter device ID: ')
    if username is None:
        username = raw_input('\tEnter username: ')

    try:
        if sudo:
            res_devices = devices.update_item(
                Key={'DeviceId': device_id},
                AttributeUpdates={
                    'DeviceOwner': {
                        'Action': 'PUT',
                        'Value': username
                    }
                }
            )
        else:
            res_devices = devices.update_item(
                Key={'DeviceId': device_id},
                ExpressionAttributeNames={
                    '#owner': 'DeviceOwner'
                },
                ExpressionAttributeValues={
                    ':val': username
                },
                UpdateExpression='SET #owner = :val',
                ConditionExpression=Attr('DeviceOwner').not_exists()
            )

        print '\t', 'update devices:', res_devices['ResponseMetadata']['HTTPStatusCode'] == 200
    except ClientError as e:
        print '\t', 'device already have owner:', e
        return

    res_users = users.update_item(
        Key={'UserName': username},
        AttributeUpdates={
            'Devices': {
                'Action': 'ADD',
                'Value': set([device_id])
            }
        }
    )

    print '\t', 'update users:', res_users['ResponseMetadata']['HTTPStatusCode'] == 200


def create_user(username=None, password=None, email=None, phone_number=None):
    print 'Create User'

    if username is None:
        username = raw_input('\tEnter username: ')
    if password is None:
        password = raw_input('\tEnter password: ')
    if email is None:
        email = raw_input('\tEnter email: ')
    if phone_number is None:
        phone_number = raw_input('\tEnter phone number: ')

    if not username or not password:
        print '\t', 'missing username or password'
        return

    item = {
        'UserName': username,
        'Password': password
    }

    if email:
        item['Email'] = email

    if phone_number and len(phone_number) == 10:
        item['PhoneNumbers'] = set([phone_number])

    try:
        res_users = users.put_item(
            Item=item,
            ReturnValues='NONE',
            ConditionExpression=Attr('UserName').not_exists()
        )
        print '\t', 'update users:', res_users['ResponseMetadata']['HTTPStatusCode'] == 200
    except ClientError as e:
        print '\t', 'username already exists'
        print '\t', e


def delete_user(username=None):
    print 'Delete User'

    if username is None:
        username = raw_input('\tEnter username: ')

    confirming = raw_input('\tAre you sure you want to delete user: '+username+' [y\\n]: ')

    if confirming == 'y':
        try:
            res_users = users.delete_item(
                Key={'UserName': username},
                ConditionExpression=Attr('UserName').exists()
            )
            print '\t', 'update users:', res_users['ResponseMetadata']['HTTPStatusCode'] == 200

        except ClientError as e:
            print '\t', 'user do not exist'
            print '\t', e

    else:
        print '\t', 'abourt delete user'


def change_password(username=None, new_password=None):
    print 'Change Password'

    if username is None:
        username = raw_input('\tEnter username: ')
    if new_password is None:
        new_password = raw_input('\tEnter new password: ')

    if not username or not new_password:
        print '\t', 'missing username or new password'
        return

    try:
        res_users = users.update_item(
            Key={'UserName': username},
            ConditionExpression=Attr('UserName').exists(),
            ExpressionAttributeNames={
                '#p': 'Password'
            },
            ExpressionAttributeValues={
                ':val': new_password
            },
            UpdateExpression='SET #p = :val'
        )
        print '\t', 'update users:', res_users['ResponseMetadata']['HTTPStatusCode'] == 200
    except ClientError as e:
        print '\t', 'user do not exist'
        print '\t', e


def delete_device(device_id=None):
    print 'Deleting device'

    if device_id is None:
        device_id = raw_input('\tEnter device ID: ')

    res_devices = devices.get_item(
        Key={
            'DeviceId': device_id
        }
    )

    if 'Item' in res_devices:
        if 'DeviceOwner' in res_devices['Item']:
            print 'deleting device from user:', res_devices['Item']['DeviceOwner']

            res_users = users.update_item(
                Key={'UserName': res_devices['Item']['DeviceOwner']},
                AttributeUpdates={
                    'Devices': {
                        'Action': 'DELETE',
                        'Value': set([device_id])
                    }
                }
            )

            print '\tupdate users:', res_users['ResponseMetadata']['HTTPStatusCode'] == 200
        else:
            print 'device doesnt have owner'

        print '\tdeleting device from devices table'

        res_devices = devices.delete_item(
            Key={
                'DeviceId': device_id
            }
        )

        print '\tupdate deivces:', res_devices['ResponseMetadata']['HTTPStatusCode'] == 200
    else:
        print '\tdevice doesnt exist'


def create_device_in_table(_device_id, _key, _version):
    devices_res = devices.put_item(
        Item={
            'DeviceId': _device_id,
            'DeviceName': _device_id,
            'DeviceType': Decimal(0),
            'LastStatus': Decimal(0),
            'DeviceStatus': Decimal(2),
            'DeviceKey': _key,
            'DeviceMemory': Decimal(0),
            'DeviceVersion': _version,
            'LastTemp':Decimal(0),
            'LastTime': Decimal(0)
        },
        ReturnValues='NONE'
    )
    return devices_res['ResponseMetadata']['HTTPStatusCode'] == 200


def change_device_settings(device_id=None, name=None, type=None):
    print 'Settings device'

    if device_id is None:
        device_id = raw_input('\tEnter device ID: ')
    if name is None:
        name = raw_input('\tEnter new name: ')
        if not name:
            name = None
    if type is None:
        type = raw_input('\tEnter new type [0,1,2]: ')
        if not type:
            type = None

    attributes = {}

    if name is not None:
        attributes['DeviceName'] = {
            'Action': 'PUT',
            'Value': name
        }

    if type is not None:
        attributes['DeviceType'] = {
            'Action': 'PUT',
            'Value': int(type)
        }

    res_devices = devices.update_item(
        Key={'DeviceId': device_id},
        AttributeUpdates=attributes
    )

    print 'update devices table:', res_devices['ResponseMetadata']['HTTPStatusCode'] == 200


def get_device_id(port='/dev/ttyUSB0'):
    while True:
        try:
            esp = esptool.ESPROM(port, esptool.ESPROM.ESP_ROM_BAUD)
            esp.connect()
            return str(format(esp.chip_id(), 'x'))
        except:
            port_list = list(port)
            if port_list[-1] == '0':
                port_list[-1] = '1'
            else:
                port_list[-1] = '0'
            port = ''.join(port_list)
            print 'port,', port
            res = raw_input('connect device fail, retry? y/n: ')
            if res == 'n':
                break
            elif res == 'q':
                exit()


def generate_key(key_length=KEY_LENGTH):
    return ''.join(random.SystemRandom().choice(string.ascii_lowercase + string.ascii_uppercase + string.digits) for _ in range(key_length))


def upload_flash(_project_dir=PROJECT_DIR):
    return os.system(COMMAND.format(dir=_project_dir, target='upload')).real == 0


def upload_spiff(_project_dir=PROJECT_DIR):
    return os.system(COMMAND.format(dir=_project_dir, target='uploadfs')).real == 0


def handle_until_true(func, name):
    while not func():
        res = raw_input(name + ' fail, retry? y/n: ')
        if res == 'n':
            break
        elif res == 'q':
            exit()


def save_device_json(_config_file_path=CONFIG_FILE_PATH, data=None):
    with open(_config_file_path, 'w') as json_file:
        json.dump(data, json_file)


def upload_new_device():
    handle_until_true(upload_flash, 'upload firmware')

    device_id = get_device_id()
    print 'device id:', device_id
    key = generate_key()
    version = '3'

    create_device_in_table(device_id, key, version)
    save_device_json(data={'key': key})
    handle_until_true(upload_spiff, 'upload key')
