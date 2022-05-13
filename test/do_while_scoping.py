_prog.run(lambda r,v: v.expecteq(r.exitcode, 0))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 0), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 0), ['a', 'a'])
