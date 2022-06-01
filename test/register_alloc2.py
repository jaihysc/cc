_prog.run(lambda r,v: v.expecteq(r.exitcode, 2))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 4), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 6), ['a', 'a'])

