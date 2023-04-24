# Returns number of args
# +1 since arg0 is the path
_prog.run(lambda r,v: v.expecteq(r.exitcode, 1))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 2), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 3), ['a', 'a'])

