_prog.run(lambda r,v: v.expecteq(r.exitcode, 29)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, 36), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, 43), ['a', 'a']) # argc = 3

