_prog.run(lambda r,v: v.expecteq(r.exitcode, 4))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 3), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 2), ['a', 'a'])

