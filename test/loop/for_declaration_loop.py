_prog.run(lambda r,v: v.expecteq(r.exitcode, 1))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 2), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 3), ['a', 'a'])


