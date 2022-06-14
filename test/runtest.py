#!/bin/python
'''
Globals
    _prog: Program, use this to perform testing
'''

import os
import subprocess
import sys
import traceback
import argparse

compiler_path = ""

class Color:
    NONE = "\033[0m"
    OK = '\033[32m'
    ERR = '\033[0;1;31m'
    WARN = "\033[0;1;33m"

def log(msg, color=Color.NONE, end='\n'):
    print(f'{color}{msg}{Color.NONE}', end=end)

class Program:
    '''
    Attributes
        has_exe: boolean, true if an executable was successfully generated during compilation
        test_failed: True if program has a test failure
        error_msg: Contains text if error occurred validating the program
        compile_msg: Contains stdout from compiler
    '''
    def __init__(self, name, c_path):
        '''
        name: Name of the test, used to give error messages, e.g., integer_arithmetic
        c_path: str, path to c source code
        '''
        self.name = name
        self.c_path = c_path

    def compile(self):
        '''
        Runs the compilation process for source code
        If you are writing tests: This is called for you before the test python file is ran

        Return True if compilation succeeded, False otherwise
        '''
        self.exe_path = f'{os.path.dirname(self.c_path)}/a.out'

        args = compiler_path.split(' ') # Separate out the compiler flags
        args.append(self.c_path)
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        self.compile_msg = result.stdout.decode('utf-8')
        log(self.compile_msg, end='')
        exit_code = result.returncode

        if exit_code != 0:
            log(f'Compiler exited non zero {exit_code}', Color.ERR)
            self.test_failed = True
            return False
        elif not os.path.isfile(self.exe_path):
            log('No executable generated', Color.ERR)
            self.test_failed = True
            return False

        self.has_exe = True
        return True

    def run(self, validate, args=[]):
        '''
        validate: function of parameter (ProgramResult, Validator), called if program successfully ran to check results
        args: list
        Returns True if run succeeded, False otherwise
        '''
        if not self.has_exe:
            log('Run failed: No executable', Color.ERR)
            return False
        else:
            r = ProgramResult()
            v = Validator()

            arg_str = ' '.join(f'"{arg}"' for arg in args)
            run_cmd = f'{self.exe_path} {arg_str}'
            log(run_cmd)
            run_result = os.system(run_cmd)
            # This is Linux only
            r.exitcode = run_result >> 8

            validate(r, v)
            self.error_msg += v.error_msg
            if v.has_failure:
                self.test_failed = True
        return True

    has_exe = False
    exe_path = '#' # Treat shell command with path as comment if no path set
    test_failed = False
    error_msg = ''
    compile_msg= ''

class ProgramResult:
    '''
    Attributes
        stdout: What the program printed to stdout, "" if nothing
        exitcode: What the program exited with, None if no exit code was given
    '''
    stdout = ""
    exitcode = None

class Validator:
    '''
    Attributes
        has_failure: True if one of the expect...() did not pass, otherwise False
        error_msg: Contains information on failed expect...()
    '''
    def expecteq(self, a, b):
        '''
        Checks that a b are equal
        Returns the comparison result
        '''
        if a == b:
            log(f"{a} {type(a)} == {b} {type(b)}", Color.OK)
            return True
        else:
            msg = f"Expected EQ\n\t{a} {type(a)}\n\t{b} {type(b)}"
            log(msg, Color.ERR)
            self.failed(msg)
            return False

    def failed(self, msg):
        '''
        Appends message into error messages and indicates test failed
        '''
        self.has_failure = True
        self.error_msg += msg
        self.error_msg += '\n'

    has_failure = False
    error_msg = ''

def run_py_file(test_name, c_file, py_file):
    '''
    Compiles and runs c_file, using py_file to validate results
    The program information is return via a Program object

    Test name taken to give error messages
    '''
    log('========================================')
    try:
        with open(py_file) as f:
            if not f.readable:
                log(f"Unable to read {py_file}", Color.ERR)

            prog = Program(test_name, c_file)
            log(prog.name)
            if prog.compile():
                # Globals available in the context of the test python file executed
                f_globals = {"_prog": prog}
                try:
                    exec(f.read(), f_globals)
                except Exception as e:
                    log(f'Exception during py file execution:', Color.ERR)
                    traceback.print_exc()
            return prog
    except FileNotFoundError:
        log(f'No matching py file found: {py_file}', Color.ERR)
    except Exception:
        log(f'Failed to open file {py_file}', Color.ERR)
        traceback.print_exc()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('compiler_path', help='Path to C compiler, flags can be passed. Example: "./out/cc -Ee -Ff"')
    parser.add_argument('-t', metavar='test', nargs='*', help='Names of tests to run (No file extensions in name). Example: integer_arithmetic')
    args = parser.parse_args()

    global compiler_path
    compiler_path = args.compiler_path
    print(compiler_path)
    if len(compiler_path) == 0 or compiler_path.isspace():
        log('Bad compiler path', Color.ERR)
        return

    # Only run tests with provided names if set
    filter_tests = args.t

    # Tests are located where the python file is
    test_dir = os.path.dirname(sys.argv[0])
    log(f'Test directory: {test_dir}')

    programs = []
    with os.scandir(test_dir) as it:
        for entry in it:
            if entry.name.endswith('.c'):
                name = entry.name.removesuffix('.c')

                if filter_tests and name not in filter_tests:
                    continue

                # Look for matching python file
                py_name = name + '.py'
                prog = run_py_file(name, f'{test_dir}/{entry.name}', f'{test_dir}/{py_name}')
                if prog != None:
                    programs.append(prog)

    # Keep the program objects, check success and failures in the objects

    # Succeeded but warnings were emitted
    warning_prog = []
    for prog in programs:
        if not prog.test_failed and prog.compile_msg:
            warning_prog.append(prog)
    if len(warning_prog) != 0:
        log('>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>')
        log(f'{len(warning_prog)} tests with warnings', Color.WARN);

        # Names of tests with warnings
        for prog in warning_prog:
            log(prog.name)

        for prog in warning_prog:
            log('-----------------------------------------')
            log(prog.name)
            log(prog.compile_msg, end='')

    # Failed tests
    log('>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>')
    failed_tests = 0
    for prog in programs:
        if prog.test_failed:
            failed_tests += 1
    if failed_tests == 0:
        log(f'0 failed tests', Color.OK);
    else:
        log(f'{failed_tests} failed tests', Color.ERR);

    # Names of failed tests
    for prog in programs:
        if prog.test_failed:
            log(prog.name)

    # Info on failed test
    for prog in programs:
        if prog.test_failed:
            log('-----------------------------------------')
            log(prog.name)
            log(prog.compile_msg, end='')
            log(prog.error_msg, Color.ERR, end='')

if __name__ == '__main__':
    main()

