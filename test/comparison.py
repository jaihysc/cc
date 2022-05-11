# Returns number of args
# Does some arithmetic with argc
_prog.run(lambda r,v: v.expecteq(r.exitcode, 14)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, 41), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, 178), ['a', 'a']) # argc = 3

