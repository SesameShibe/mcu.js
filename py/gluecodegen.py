#!/usr/bin/env python
import sys
import json
import os


TYPE_RULES = {
    'void': ('void ', '', ''),
    'int': ('int32_t ', 'duk_to_int32', 'duk_push_int'),
    'i32': ('int32_t ', 'duk_to_int32', 'duk_push_int'),
    'u32': ('uint32_t ', 'duk_to_uint32', 'duk_push_uint'),
    'u8': ('uint8_t ', 'duk_to_uint32', 'duk_push_uint'),
    'u16': ('uint16_t ', 'duk_to_uint16', 'duk_push_uint'),
    'i16': ('int16_t ', 'duk_to_uint16', 'duk_push_uint'),
    'double': ('double ', 'duk_to_number', 'duk_push_number'),
    'bool': ('int ', 'duk_to_boolean', 'duk_push_boolean'),
    'str': ('const char *', 'duk_to_string', 'duk_push_string'),
    'buf': ('JS_BUFFER ', 'duk_get_buffer_data', ''),
}


def localVarName(idx):
    return 'arg%d' % idx


MODULE_NAME = ''


def usage():
    print('''
Usage: python gluecodegen.py JsonFilePath
---------------------------
Json   |    C Type
---------------------------
void        void
i32         int32_t
u32         uint32_t
f64         double
bool        int
str         const char *
buf         buffer{
              uint8_t *buf;
              size_t size;
            }
ptr         void *
---------------------------
''')


def fatal(msg):
    print(msg)
    sys.exit(1)


def transType(jsonType):
    for i in TYPE_RULES:
        if jsonType == i:
            return TYPE_RULES[i][0]
    fatal("[error] type '%s' is not defined.\n at module '%s':'function'" %
          (jsonType, MODULE_NAME))


def transTypeList(typeList):
    ret = []
    for i in range(len(typeList)):
        ret.append(transType(typeList[i]))
    return ret


def setVar(varType):
    if TYPE_RULES[varType][2] == '':
        fatal("[error] type '%s' setter is not defined.\n at module '%s':'function'" % (
            varType, MODULE_NAME))
    #if varType == 'buf':
    #    return '''    uint32_t *buf = %s(ctx, ret.size, 0);\n    *buf = *ret.buf;\n''' % TYPE_RULES[varType][2]
    else:
        return '''    %s(ctx, ret);\n''' % TYPE_RULES[varType][2]


def getVar(varType, num):
    if TYPE_RULES[varType][1] == '':
        fatal("[error] type '%s' getter is not defined.\n at module '%s':'function'" % (
            varType, MODULE_NAME))

    if varType == 'buf':
        return '''
    %s.buf = (u8*) %s(ctx, %d, &%s.size);
''' % (localVarName(num), TYPE_RULES[varType][1], num, localVarName(num))
    elif varType == 'i16':
        # if varType in ('i32','u32','f64','bool','str','ptr'):
        return '''
    %s = (int16_t) %s(ctx, %d);
''' % (localVarName(num), TYPE_RULES[varType][1], num)
    else:
        # if varType in ('i32','u32','f64','bool','str','ptr'):
        return '''
    %s = %s(ctx, %d);
''' % (localVarName(num), TYPE_RULES[varType][1], num)


def camelFormat(str):
    ret = str[:1].upper() + str[1:]
    if '_' in str:
        spl = str.split('_')
        ret = camelFormat(spl[0])
        for x in spl[1:]:
            ret += '_' + camelFormat(x)
    return ret


def genFunctionCallAndResult(funcName, targetFuncName, typeList):
    ret = '''    '''
    if typeList[0] != 'void':
        ret += 'ret = '
    ret += '%s(' % targetFuncName
    #ret += 'hal%s%s(' %(camelFormat(MODULE_NAME),camelFormat(funcName))
    for i in range(len(typeList[1:])):
        if i == 0:
            ret += localVarName(i)
            continue
        ret += ', ' + localVarName(i)
    ret += ');\n'
    if typeList[0] == 'void':
        ret += '''    return 0;'''
    else:
        if typeList[0] == 'buf':
            ret += '''duk_push_external_buffer(ctx);\n
duk_config_buffer(ctx, -1, ret.buf, ret.size);\n'''
        else:
            ret += setVar(typeList[0])
        ret += '''    return 1;'''
    return ret


def genLocalVarDefine(typeList):
    ret = ''
    if typeList[0] != "void ":
        ret = '''    %sret;\n''' % typeList[0]
    for i in range(len(typeList[1:])):
        ret += '''    %s''' % (typeList[i+1]+localVarName(i)+';\n')
    return ret


def genArgumentFetchCode(typeList):
    ret = ''
    for i in range(len(typeList[1:])):
        ret += '%s' % getVar(typeList[i+1], i)
    return ret


def genFunction(funcName, targetFuncName, typeList):
    return '''
static duk_ret_t glue%s%s(duk_context *ctx)
{
%s
%s
%s
}
''' % (camelFormat(MODULE_NAME), camelFormat(funcName), genLocalVarDefine(transTypeList(typeList)), genArgumentFetchCode(typeList), genFunctionCallAndResult(funcName, targetFuncName, typeList))

def arity(typeList):
    if not TYPE_RULES.has_key(typeList[0]):
        typeList = typeList[1:]
    return len(typeList) - 1

# magic: function 0, const 1.
def genRegList(moduleDict, magic):
    ret = ''
    if magic == 0:
        funcDict = moduleDict['function']
        for func in funcDict:
            ret += '''    { "%s", glue%s%s , %d },\n''' % (func, camelFormat(
                MODULE_NAME), camelFormat(func), arity(funcDict[func]))
    if magic == 1:
        if "const" in moduleDict:
            for constName in moduleDict['const']:
                ret += '''    { "%s", %s },\n''' % (
                    constName, moduleDict['const'][constName])
    return ret


def genRegisterCode(moduleDict):
    return '''
const duk_function_list_entry module_%s_funcs[] = {
%s
    { NULL, NULL, 0 }
};

const duk_number_list_entry module_%s_consts[] = {
%s
    { NULL, 0.0 }
};

void module_%s_init(duk_context *ctx)
{
    duk_push_object(ctx);

    duk_put_function_list(ctx, -1, module_%s_funcs);
    duk_put_number_list(ctx, -1, module_%s_consts);

	duk_put_global_string(ctx, "%s");
}
''' % (MODULE_NAME, genRegList(moduleDict, 0), MODULE_NAME, genRegList(moduleDict, 1), MODULE_NAME, MODULE_NAME, MODULE_NAME, MODULE_NAME)


def genHalFuncDef(funcName, typeList):
    ret = ''
    ret += '%shal%s%s(' % (typeList[0],
                           camelFormat(MODULE_NAME), camelFormat(funcName))
    for i in range(len(typeList[1:])):
        if i == 0:
            ret += (typeList[i+1]+localVarName(i))
            continue
        ret += ', ' + (typeList[i+1]+localVarName(i))
    ret += ');\n'
    return ret


def genJSInit(modulesJson):
    global MODULE_NAME

    ret = ''
    for moduleName in modulesJson:
        MODULE_NAME = moduleName
        ret += genModule(modulesJson[moduleName])
        #ret += '#include "glue_%s.h"' % moduleName
        #ret += 'void module_%s_init(duk_context *ctx);\n' % moduleName
    ret += '\n\nvoid genJSInit(duk_context* ctx){\n'
    for moduleName in modulesJson:
        ret += 'module_%s_init(ctx);\n' % moduleName
    ret += '}\n'
    with open('platform/esp32/main/__generated/gen_jsmods.h', 'wb') as f:
        f.write(ret)


def genModule(moduleDict):
    ret = ''

    funcDict = moduleDict['function']
    ret += '/* File generated automatically by the gluecodegen. */\n'
    ret += ('#include "duktape.h"\n#include "global.h"\n')
    # for funcName in funcDict:
    #    f.write(genHalFuncDef(funcName, transTypeList(funcDict[funcName])))
    for funcName in funcDict:
        typeList = funcDict[funcName]
        targetFuncName = 'hal%s%s' % (
            camelFormat(MODULE_NAME), camelFormat(funcName))
        if not TYPE_RULES.has_key(typeList[0]):
            # the first item is the function name
            targetFuncName = typeList[0]
            typeList = typeList[1:]
        ret += (genFunction(funcName, targetFuncName, typeList))
    ret += (genRegisterCode(moduleDict))
    return ret

def generate(path):
    #usage()
    modulesJson = {}
    for fname in os.listdir(path):
        if fname[0] == '.' :
            continue
        if fname[-5:] != '.json':
            continue
        print(fname)
        with open(path + '/' + fname, 'r') as f:
            modulesJson[fname[:-5]] = json.loads(f.read())
    genJSInit(modulesJson)
    print('...Done!')
