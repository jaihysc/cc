# Returns number of args
# Does some arithmetic with argc
_prog.run(lambda r,v: v.expecteq(r.exitcode, 28)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, 49), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, 70), ['a', 'a']) # argc = 3

