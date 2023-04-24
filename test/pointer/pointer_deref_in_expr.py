_prog.run(lambda r,v: v.expecteq(r.exitcode, 24))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 23), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 22), ['a', 'a'])

