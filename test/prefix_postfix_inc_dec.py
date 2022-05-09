_prog.run(lambda r,v: v.expecteq(r.exitcode, 9)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, 14), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, 19), ['a', 'a']) # argc = 3

