# Returns number of args
# Does some arithmetic with argc
_prog.run(lambda r,v: v.expecteq(r.exitcode, -17160)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, -15075), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, -12990), ['a', 'a']) # argc = 3

