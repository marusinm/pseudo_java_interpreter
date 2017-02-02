#!/usr/bin/python
#autor: Marek Marusin
import glob
from subprocess import call

filenames = glob.glob('tests/*.txt')
# filenames = glob.glob('*.txt')
returned_codes = list()  # list return kodov ktore vrati zavolanie programu main
expected_return_codes = list()  # ocakavane return kody sa odseprauju z nazvu testu
for x in filenames:
    returned_codes.append(call(["./main", x]))
    # returned_codes.append(call(["../main", x]))
    x = x.replace("tests/", "",1)
    expected_return_codes.append(x.split('_', 1))

for i in range(0, len(returned_codes)):
    if int(returned_codes[i]) == int(expected_return_codes[i][0]):
        CRED = '\033[6;30;42m'
        CEND = '\033[0m'
        print filenames[i], "\t\t\t returned code: ", returned_codes[i], "\t excepted: ", expected_return_codes[i][0], CRED, "success", CEND
    else:
        CRED = '\033[91m'
        CEND = '\033[0m'
        print filenames[i], "\t\t\t returned code: ", returned_codes[i], "\t excepted: ", expected_return_codes[i][0], CRED, "failed", CEND
