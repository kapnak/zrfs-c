import os
import time
import signal
import subprocess
from sys import platform as _platform


processes = []
TEST_DIR = f'/tmp/zrfs-test-{round(time.time())}'
ZRFS = os.path.dirname(os.path.abspath(__file__)) + '/build/zrfs'


def exit_gracefully(code):
    for process in processes:
        os.killpg(os.getpgid(process.pid), signal.SIGTERM)
    os.system("kill -9 $(ps aux | grep -e zrfs | awk '{ print $3 }') > /dev/null 2>&1")
    exit(code)


def readlines(std):
    out = ''
    while True:
        line = std.readline().decode()
        if not line:
            break
        out += line
    return out


class Test:
    def __init__(self, wd=None):
        self.wd = wd

    def execute(self, label, command, validator=None, noreturn=False, should_failed=False, wd=None):
        if wd is None:
            wd = self.wd
        elif not wd or wd == '':
            wd = None

        print(f'[RUNNING] {label} ... ', end='', flush=True)
        process = subprocess.Popen(f'{command}',
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   shell=True,
                                   cwd=wd)
        try:
            stdout, stderr = process.communicate(timeout=2 if noreturn else None)
        except subprocess.TimeoutExpired:
            print(f'\r[SUCCESS] {label}     ')
            processes.append(process)
            return

        stdout = stdout.decode()
        stderr = stderr.decode()

        if (not should_failed and process.returncode != 0) or (should_failed and process.returncode == 0):
            print(f'\r[FAILED]  {label}     ')
            print(f'\nCommand \'{command}\' return code {process.returncode} {" but should fail " if should_failed else " "}'
                  f'with {"not " if stderr != "" else ""}empty stderr.')
            print(stderr)
            exit_gracefully(1)
        elif validator and not validator(stderr if should_failed else stdout):
            print(f'\r[FAILED] {label}     ')
            print(f'\nCommand \'{command}\' failed validation with output :\n{stdout}')
            exit_gracefully(1)
        else:
            print(f'\r[SUCCESS] {label}     ')


def validate_server_key(output):
    global server_pk
    server_pk = output
    return len(server_pk) == 52


def validate_client_key(output):
    global client_pk
    client_pk = output
    return len(client_pk) == 52


def validate_acl_2(output):
    return f'{client_pk}\t2' in output


def validate_acl_3(output):
    return f'{client_pk}\t3' in output


def validate_readdir(output):
    return output != ''


def validate_read(output):
    return output == content


def validate_write_denied(output):
    return 'Permission denied' in output


def validate_write_server_key(output):
    with open(f'{TEST_DIR}/data/testfile3.txt', 'r') as f:
        return f.read() == client_pk + '\n'


def validate_write(output):
    with open(f'{TEST_DIR}/data/testfile4.txt', 'r') as f:
        return f.read() == client_pk + '\n'


def validate_del_permission(output):
    return output == ''


print(f'Test dir : {TEST_DIR}\n')

chars = ''.join([chr(i) for i in range(128)])
content = 'abcd' * 10000000
server_pk = ''
client_pk = ''

# Prepare environment
os.system("kill -9 $(ps aux | grep -e zrfs | awk '{ print $3 }') > /dev/null 2>&1")
os.system('make clean')
os.system(f'rm -rf /tmp/zrfs-test-*')
os.system(f'mkdir -p {TEST_DIR}/data')
if _platform != 'cygwin':
    os.system(f'mkdir -p {TEST_DIR}/mnt {TEST_DIR}/mnt2')
with open(f'{TEST_DIR}/data/testfile.txt', 'w') as f:
    f.write(content)

# Execute tests
test = Test(TEST_DIR)
test.execute('Build CLI', 'make build-cli', wd='')
test.execute('Create server key', f'{ZRFS} key server.sk', validate_server_key)
test.execute('Create client key', f'{ZRFS} key client.sk', validate_client_key)
test.execute('Launch server', f'cd data && {ZRFS} host 127.0.0.1 6339 ../server.sk', noreturn=True)
test.execute('Set permission', f'{ZRFS} acl add data {client_pk} 2 && {ZRFS} acl list data', validate_acl_2)
test.execute('Mount FS', f'{ZRFS} mount 127.0.0.1 6339 mnt {server_pk} client.sk', noreturn=True)
test.execute('Mount FS with server key', f'{ZRFS} mount 127.0.0.1 6339 mnt2 {server_pk} server.sk', noreturn=True)
test.execute('Read dir', f'ls mnt', validate_readdir)
test.execute('Read', 'cat mnt/testfile.txt', validate_read)
test.execute('Write (denied)', 'echo "test" > mnt/testfile2.txt', validate_write_denied, should_failed=True)
test.execute('Write (With server key)', f'echo "{client_pk}" > mnt2/testfile3.txt', validate_write_server_key)
test.execute('Edit permission', f'{ZRFS} acl add data {client_pk} 3 && {ZRFS} acl list data', validate_acl_3)
test.execute('Write', f'echo "{client_pk}" > mnt/testfile4.txt', validate_write)
test.execute('Deleting permissions', f'{ZRFS} acl del data && {ZRFS} acl list data', validate_del_permission)

# Exit
exit_gracefully(0)

# TODO : Test delete one ACL.
# TODO : Test create directory.
