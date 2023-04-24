_prog.run(lambda r,v: v.expecteq(r.exitcode, 9))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 12), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 15), ['a', 'a'])
