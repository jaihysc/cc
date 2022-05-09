_prog.run(lambda r,v: v.expecteq(r.exitcode, -1)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, 0), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, 1), ['a', 'a']) # argc = 3

