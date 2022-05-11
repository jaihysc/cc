_prog.run(lambda r,v: v.expecteq(r.exitcode, 35)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, 80), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, 125), ['a', 'a']) # argc = 3

