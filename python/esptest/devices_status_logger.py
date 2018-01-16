import datetime
import boto3
from time import time as get_millis

dynamodb = boto3.resource('dynamodb')
status_logger = dynamodb.Table('EasyCoolStatusLogger')
devices = dynamodb.Table('Devices')

days_backwards = int(raw_input('Enter days back: '))

_to = int(get_millis() * 1000)
_from = int((get_millis() - (60 * 60 * 24 * days_backwards)) * 1000)

print 'from:', datetime.datetime.fromtimestamp(_from / 1000)
print 'to:', datetime.datetime.fromtimestamp(_to / 1000)
print

res_devices = devices.scan(
    AttributesToGet=[
        'DeviceId',
        'DeviceName',
        'DeviceOwner'
    ]
)

for device in res_devices['Items']:
    res_status = (status_logger.query(
        KeyConditions={
            'DeviceId': {
                'AttributeValueList': [device['DeviceId']],
                'ComparisonOperator': 'EQ'
            },
            'ChangedAt': {
                'AttributeValueList': [_from, _to],
                'ComparisonOperator': 'BETWEEN'
            }
        }
    ))
    print device['DeviceId'], device['DeviceName'], device['DeviceOwner']
    if res_status['Count'] > 0:
        for status in res_status['Items']:
            data = '\ttime: ' + datetime.datetime.fromtimestamp(status['ChangedAt'] / 1000).__str__() + ' from: ' + status[
                    'OldStatus'].__str__() +  ' to: ' + status['NewStatus'].__str__()
            if 'SMSSent' in status:
                data += ' SMS: ' + status['SMSSent'].__str__()
            print data
    else:
        print '\t', 'No changed in status'
