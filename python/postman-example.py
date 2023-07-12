#!/usr/bin/python

from postman import *

pm = Postman("/dev/ttyUSB0")
# time.sleep(1.5)     # workaround for Arduino bootloader bug

pm.debug = True
response = pm.get([])
if response[0] == PM_205_Content:
    resources = response[1]
    for resource in resources:
        print(resource + ":")
        response = pm.get([resource])
        if response[0] == PM_205_Content:
            print(repr(response[1]))
        else:
            print("Cannot get the %s resource." % resource)
else:
    print("Cannot get the resource index.")
