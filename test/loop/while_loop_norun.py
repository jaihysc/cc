_prog.run(lambda r,v: v.expecteq(r.exitcode, 2))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 2), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 2), ['a', 'a'])
