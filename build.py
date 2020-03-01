#!/usr/bin/env python
import sys
import os
import zipfile
import urllib
import platform

sys.path.append('py')
import shell
import gluecodegen

PLATFORMS = {
    'esp32' : {
        'toolchain': {
            'esp32-toolchain': {
                'url-windows': 'https://dl.espressif.com/dl/esp32_win32_msys2_environment_and_toolchain-20181001.zip',
                'url-linux': 'https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz',
                'url-darwin': 'https://dl.espressif.com/dl/xtensa-esp32-elf-osx-1.22.0-80-g6c4433a-5.2.0.tar.gz',
                'version': '5.2.0',
            },
            'esp-idf': {
                'url': 'https://dl.espressif.com/dl/esp-idf/releases/esp-idf-v3.3.1.zip',
                'version': 'v3.3.1',
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


def progressbar(blocknum, blocksize, totalsize):
    bar_length = 50
    percent = 100.0 * blocknum * blocksize / totalsize
    hashes = '#' * int(percent / 100 * bar_length)
    spaces = ' ' * (bar_length - len(hashes))
    sys.stdout.write("\rDownloading: [%s] %.1f%%"%(hashes + spaces, percent))
    sys.stdout.flush()
    if int(percent) >= 100:
        print("\n...Done!")


def downlaodAndUnpack(toolName, tool):
    url = None
    if tool.has_key('url'):
        url = tool['url']
    else:
        url = tool['url-' + platform.system().lower()]
    
    tryMkdir('downloads')
    print('Downloading %s, version %s, URL: %s' % (toolName, tool['version'], url))
    downloadPath = 'downloads/%s-%s.zip' % (toolName, tool['version'])
    page = urllib.urlopen(url)
    file_size = int(page.info()['Content-Length'])
    page.close()

    # Avoid re-download
    if os.path.isfile(downloadPath) and os.path.getsize(downloadPath) == file_size:
        print('Using cached: %s' % downloadPath)
        pass
    else:
        urllib.urlretrieve(url, downloadPath, progressbar)
    
    unpackPath = 'toolchain/%s' % (toolName)
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
        ret += '0x%02x, ' % ord(d[i])
    ret += '};\n\n'
    return ret


def generate():
    print('Generating header file gen_js.h from builtin-js directory.')
    js = ''
    for fn in os.listdir('builtin-js'):
        if (not fn.startswith('.')) and fn.endswith('.js'):
            print(fn)
            with open('builtin-js/' + fn, 'rb') as f:
                d = f.read() + '\x00'
            js += dataToU8Array(d, 'js_%s' % fn.split('.')[0])
    tryMkdir('platform/esp32/main/__generated')
    with open('platform/esp32/main/__generated/gen_js.h', 'wb') as f:
        f.write(js)
    print('Generating glue code from modules.json')
    gluecodegen.generate('platform/esp32/main/modules.json')



def build():
    if isWindows():
        pass
    else:
        generate()
        os.system('cd platform/esp32 && make -j4 app-flash')

    
def startShellInEnvironment():
    if isWindows():
        cwd = os.getcwd()
        cwd = '/' + cwd[0].lower() + cwd[2:].replace('\\', '/')
        os.system('toolchain\\esp32-toolchain\\msys32\\mingw32.exe')
    else:
        os.system('bash')


def usage():
    print("""
Usage: python build.py COMMAND [OPTIONS]

Available commands:
    prepare-toolchain   Download and install toolchain.
    generate            Generate files for build.
    build               Build mcu.js.
    env                 Start a shell in build environment.
    shell               Start the debug shell.

""")


action = ''
os.environ['PATH'] += ':' + os.getcwd() + '/toolchain/esp32-toolchain/xtensa-esp32-elf/bin'
os.environ['IDF_PATH'] = os.getcwd() + '/toolchain/esp-idf/esp-idf-' + PLATFORMS['esp32']['toolchain']['esp-idf']['version']
if len(sys.argv) >= 2:
    action = sys.argv[1]
if action == 'prepare-toolchain':
    perpareToolchain()
elif action == 'generate':
    generate()
elif action == 'build':
    build()
elif action == 'env':
    startShellInEnvironment()
elif action == 'shell':
    shell.start()
else:
    usage()
