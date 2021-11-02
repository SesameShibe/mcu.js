#!/usr/bin/env python3


import sys
import os
import zipfile
import requests
import platform
from argparse import ArgumentParser

from fnmatch import fnmatch

from serial import VERSION

sys.path.append('py')
import gluecodegen
import shell

PLATFORMS = {
    'esp32': {
        'toolchain': {
            'esp32-toolchain': {
                'url-windows': 'https://dl.espressif.com/dl/esp32_win32_msys2_environment_and_toolchain-20190611.zip',
                'url-linux': 'https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_2_0-esp-2019r2-linux-amd64.tar.gz',
                'url-darwin': 'https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_2_0-esp-2019r2-macos.tar.gz',
                'version': '2019r2',
            },
            'esp-idf': {
                'url': 'https://dl.espressif.com/dl/esp-idf/releases/esp-idf-v4.0.zip',
                'version': 'v4.0',
                # },
                # 'esp32-arduino': {
                #     'url': 'https://mcujs.org/dl/arduino-esp32-1.0.4.zip',
                #     'version': '1.0.4',
                #     'path': 'platform/esp32/components/arduino',
                #     'patch': b'PK'
            }
        }
    },
    'k210': {
        'toolchain': {
            'k210-standalone-sdk': {
                'url': 'https://s3.cn-north-1.amazonaws.com.cn/dl.kendryte.com/documents/kendryte-standalone-sdk-0.5.6.zip',
                'version': '0.5.6',
            },
            'riscv64-toolchain': {
                'url-linux': 'https://s3.cn-north-1.amazonaws.com.cn/dl.kendryte.com/documents/kendryte-toolchain-ubuntu-amd64-8.2.0-20190213.tar.gz',
                'url-darwin': 'https://s3.cn-north-1.amazonaws.com.cn/dl.kendryte.com/documents/kendryte-toolchain-osx-mojave-8.2.0-20190213.tar.gz',
                'url-windows': 'https://s3.cn-north-1.amazonaws.com.cn/dl.kendryte.com/documents/kendryte-toolchain-win-amd64-8.2.0-20190213.zip',
                'version': '20190213'
            }
        }
    }
}

CURRENT_PLATFORM = 'esp32'


def isWindows():
    return platform.system().lower() == 'windows'


def tryMkdir(dir):
    try:
        os.makedirs(dir)
        return True
    except:
        return False


def progressbar(currentSize, totalsize):
    bar_length = 50
    percent = 100.0 * currentSize / totalsize
    hashes = '#' * int(percent / 100 * bar_length)
    spaces = ' ' * (bar_length - len(hashes))
    sys.stdout.write("\rDownloading: [%s] %.1f%%" % (hashes + spaces, percent))
    sys.stdout.flush()
    if int(percent) >= 100:
        print("\n...Done!")


def downlaodAndUnpack(toolName, tool):
    url = None
    if 'url' in tool:
        url = tool['url']
    else:
        url = tool['url-' + platform.system().lower()]

    tryMkdir('downloads')
    print('Downloading %s, version %s, URL: %s' %
          (toolName, tool['version'], url))
    downloadPath = 'downloads/%s-%s.zip' % (toolName, tool['version'])
    resp = requests.get(
        url, headers={'User-Agent': 'mcujs'}, allow_redirects=True, stream=True)
    totalFileLen = int(resp.headers.get('Content-Length'))
    print('fileLen: %d' % totalFileLen)

    # Avoid re-download
    if os.path.isfile(downloadPath) and os.path.getsize(downloadPath) == totalFileLen:
        print('Using cached: %s' % downloadPath)
        resp.close()
        pass
    else:
        recvLen = 0
        with open(downloadPath, 'wb') as f:
            for data in resp.iter_content(chunk_size=100*1024):
                f.write(data)
                recvLen += len(data)
                progressbar(recvLen, totalFileLen)

    if 'patch' in tool:
        with open(downloadPath, 'r+b') as f:
            f.seek(0)
            f.write(tool['patch'])

    unpackPath = 'toolchain/%s' % (toolName)
    if 'path' in tool:
        unpackPath = tool['path']

    print('Unpacking to %s' % unpackPath)
    if url.endswith('.zip'):
        with zipfile.ZipFile(downloadPath) as zipf:
            zipf.extractall(unpackPath)
        if not isWindows():
            os.system('chmod -R +x ' + unpackPath)
    if url.endswith('.tar.gz'):
        os.system('mkdir -p ' + unpackPath)
        os.system('tar xf ' + downloadPath + ' -C ' + unpackPath)
    return True


def perpareToolchain():
    plat = PLATFORMS[CURRENT_PLATFORM]
    toolchain = plat['toolchain']
    tryMkdir('toolchain')
    for toolName in toolchain.keys():
        tool = toolchain[toolName]
        result = False
        result = downlaodAndUnpack(toolName, tool)
        if result:
            print('..OK!')
        else:
            print('...Failed!')
            return False
    print('Toolchain is ready!')


def dataToU8Array(d, arrName):
    ret = 'const uint8_t %s[] =  {' % arrName
    for i in range(0, len(d)):
        if (i % 16 == 0):
            ret += '\n'
        ret += '0x%02x, ' % d[i]
    ret += '};\n\n'
    return ret


def walk(path, pattern='*.*'):
    for root, dirnames, filenames in os.walk(path):
        for filename in filenames:
            if fnmatch(filename, pattern):
                yield os.path.join(root, filename)


def generate():
    print('Generating header file gen_js.h from builtin-js directory.')
    js = ''
    for fn in walk('builtin-js', '*.js'):
        notdir = os.path.split(fn)[1]
        if (not notdir.startswith('.')):
            print(fn)
            with open(fn, 'rb') as f:
                d = f.read() + b'\x00'
            js += dataToU8Array(d, 'js_%s' % notdir.split('.')[0])
    tryMkdir('platform/esp32/main/__generated')
    with open('platform/esp32/main/__generated/gen_js.h', 'w') as f:
        f.write(js)
    print('Generating glue code from module json')
    gluecodegen.generate('platform/esp32/main/modules')


def build():
    generate()
    os.system('cd platform/esp32 && idf.py build')


def flash():
    generate()
    os.system('cd platform/esp32 && idf.py flash')


def flash_font():
    os.system(
        'cd font && python %s/components/partition_table/parttool.py write_partition --input font.bin --partition-name "font" '%os.environ['IDF_PATH'])

def full_flash():
    flash_font()
    flash()


def clean():
    os.system('cd platform/esp32 && idf.py clean')


def startShellInEnvironment():
    if isWindows():
        cwd = os.getcwd()
        cwd = '/' + cwd[0].lower() + cwd[2:].replace('\\', '/')
        os.system('toolchain\\esp32-toolchain\\msys32\\mingw32.exe')
    else:
        os.system('bash')


# def usage():
#     print("""
# Usage: python build.py COMMAND [OPTIONS]

# Available commands:
#     prepare-toolchain   Download and install toolchain.
#     env                 Start bash shell in build environment.
#     generate            Generate files for build.
#     build               Generate and build mcu.js.
#     flash               Generate, build and flash mcu.js.

#     shell               Start the javascript console.

# Available options:
#     --platform          Select target platform (esp32 or k210).
#     --port              Select the serial port for flash/shell. 


# """)


# action = ''
# os.environ['ESPPORT'] = os.getcwd() + '/COM6'  # set serial port name
# os.environ['IDF_PATH'] = os.getcwd() + '/toolchain/esp-idf/esp-idf-' + \
#     PLATFORMS['esp32']['toolchain']['esp-idf']['version']
# os.environ['PATH'] += ':' + \
#     os.getcwd() + '/toolchain/esp32-toolchain/xtensa-esp32-elf/bin' + ':' + \
#     os.environ['IDF_PATH'] + '/tools'

VERSION = '0.0.1'

parser = ArgumentParser(description='mcujs build tool %s'%VERSION,
                        usage='python build.py [OPTIONS] COMMAND')

parser.add_argument('--version', action='version', version='%(prog)s '+VERSION)
parser.add_argument('--platform', dest='platform', default='esp32',
                    help='Select target platform (esp32 or k210). (default: %(default)s)') # unimplemented
parser.add_argument('-p', '--port', dest='port', default='COM6',
                    help='Select the serial port for flash/shell. (default: %(default)s)')

subparser = parser.add_subparsers(dest='action', help='Available commands')

subparser.add_parser('prepare-toolchain', help='Download and install toolchain. (unimplemented)').set_defaults(
    func=perpareToolchain)  # unimplemented
subparser.add_parser(
    'env', help='Start bash shell in build environment. (unimplemented)').set_defaults(func=startShellInEnvironment)  # unimplemented

subparser.add_parser(
    'generate', help='Generate files for build.').set_defaults(func=generate)
subparser.add_parser(
    'build', help='Generate and build mcu.js.').set_defaults(func=build)
subparser.add_parser(
    'flash', help='Generate, build and flash mcu.js.').set_defaults(func=flash)
subparser.add_parser('flash-font', help='Flash font partition.').set_defaults(func=flash_font)
subparser.add_parser(
    'full-flash', help='Generate, build and flash mcu.js & font partition.').set_defaults(func=full_flash)
subparser.add_parser(
    'clean', help='Clean build directory.').set_defaults(func=clean)
subparser.add_parser(
    'shell', help='Start the javascript console.').set_defaults(func=shell.start)


args = parser.parse_args()
os.environ['ESPPORT'] = args.port
args.func()
