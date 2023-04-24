_prog.run(lambda r,v: v.expecteq(r.exitcode, 2)) # False
_prog.run(lambda r,v: v.expecteq(r.exitcode, 1), ['a']) # True

