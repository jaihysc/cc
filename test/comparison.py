_prog.run(lambda r,v: v.expecteq(r.exitcode, 14)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, 45), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, 54), ['a', 'a']) # argc = 3

