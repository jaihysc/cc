_prog.run(lambda r,v: v.expecteq(r.exitcode, 5)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, 9), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, 49), ['a', 'a']) # argc = 3

