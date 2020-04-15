import sys
import os
import json

path = ''
modulesJson = {}

TYPE_RULES = {
    'void': 'void',
    'i32': 'number',
    'u32': 'number',
    'double': 'number',
    'bool': 'boolean',
    'str': 'string',
    'buf': 'Uint8Array'
}

def genConst(constName):
    return 'const %s:number;' % constName
    

def genFunction(funcName,typeList):
    ret = "function %s():" % funcName
    if not TYPE_RULES.has_key(typeList[0]):
        return ret + TYPE_RULES[typeList[1]] + ';'
    return ret + TYPE_RULES[typeList[0]] + ';'
    

def genModule(moduleName):
    ret = 'declare module %s {' % moduleName
    for funcName in modulesJson[moduleName]['function']:
        #print(funcName)
        ret += genFunction(funcName,modulesJson[moduleName]['function'][funcName])
    if modulesJson[moduleName].has_key('const'):
        for constName in modulesJson[moduleName]['const']:
            ret += genConst(constName)
    return ret + '}'


def genDC():
    ret = ""
    for moduleName in modulesJson:
        ret += genModule(moduleName)
    with open('mcujslib.d.ts', 'wb') as f:
        f.write(ret)


if len(sys.argv) >= 2:
    path = sys.argv[1]
else: 
    path = '../platform/esp32/main/modules.json'

with open(path, 'r') as f:
    modulesJson = json.loads(f.read())

genDC()
