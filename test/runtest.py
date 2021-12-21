#!/bin/python
'''
Args:
    compiler-path, required, e.g., ./out/cc
Globals
    _prog: Program, use this to perform testing
'''

import os
import sys
import traceback

compiler_path = ""

class Color:
    NONE = "\033[0m"
    OK = '\033[32m'
    ERR = '\033[0;1;31m'

def log(msg, color=Color.NONE):
    print(f'{color}{msg}{Color.NONE}')

class Program:
    '''
    Attributes
        has_exe: boolean, true if an executable was successfully generated during compilation
    '''
    def __init__(self, c_path):
        '''
        c_path: str, path to c source code
        '''
        self.c_path = c_path

    def compile(self):
        '''
        Runs the compilation process for source code
        If you are writing tests: This is called for you before the test python file is ran

        Return True if compilation succeeded, False otherwise
        '''
        shell_cmd = f"{compiler_path} {self.c_path}"
        log(shell_cmd)
        result = os.system(shell_cmd)

        # This is linux only, shift down to get exit code
        exit_code = result >> 8

        if exit_code != 0:
            log(f'Compiler exited non zero {exit_code}', Color.ERR)
            return False
        elif not os.path.isfile('a.out'):
            log('No executable generated', Color.ERR)
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
            run_cmd = f'./a.out {arg_str}'
            log(run_cmd)
            run_result = os.system(run_cmd)
            # This is Linux only
            r.exitcode = run_result >> 8

            validate(r, v)
        return True

    has_exe = False

class ProgramResult:
    '''
    Attributes
        stdout: What the program printed to stdout, "" if nothing
        exitcode: What the program exited with, None if no exit code was given
    '''
    stdout = ""
    # TODO not implemented yet
    exitcode = None

class Validator:
    def expecteq(self, a, b):
        '''
        Checks that a b are equal
        Returns the comparison result
        '''
        if a == b:
            log(f"{a} {type(a)} == {b} {type(b)}", Color.OK)
        else:
            log(f"Expected EQ\n\t{a} {type(a)}\n\t{b} {type(b)}", Color.ERR)
        return a == b

def run_py_file(c_file, py_file):
    log('========================================')
    try:
        with open(py_file) as f:
            if not f.readable:
                log(f"Unable to read {py_name}", Color.ERR)

            prog = Program(c_file)
            if prog.compile():
                log('= RUN =')
                # Globals available in the context of the test python file executed
                f_globals = {"_prog": prog}
                try:
                    exec(f.read(), f_globals)
                except Exception as e:
                    log(f'Exception during py file execution:', Color.ERR)
                    traceback.print_exc()
    except FileNotFoundError:
        log(f'No matching py file found: {py_name}', Color.ERR)

def main():
    if len(sys.argv) < 2:
        log('Missing compiler path', Color.ERR)
        return
    if len(sys.argv) > 2:
        log('Too many arguments, only compiler path needed', Color.ERR)
        return
    global compiler_path
    compiler_path = sys.argv[1]
    if len(compiler_path) == 0 or compiler_path.isspace():
        log('Bad compiler path', Color.ERR)
        return

    with os.scandir() as it:
        for entry in it:
            if entry.name.endswith('.c'):
                # Look for matching python file
                py_name = entry.name.removesuffix('.c') + '.py'
                run_py_file(entry.name, py_name)

if __name__ == '__main__':
    main()

